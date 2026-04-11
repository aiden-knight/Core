/*
===========================================================================

Core

Copyright (c) 2025 Dan Moody

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

===========================================================================
*/

#include <hashmap.h>

#include <core_helpers.h>
#include <defer.h>
#include <debug.h>
#include <typecast.inl>
#include <core_math.h>

#include <memory.h>	// memset
#include <malloc.h>

/*
================================================================================================

	Hashmap

================================================================================================
*/

hashmap_t *Hashmap_Create( const u32 startingCapacity, float32 normalizedMaxUtilisation, bool8 shouldGrow ) {
	assert( startingCapacity );
	assert( normalizedMaxUtilisation > 0.0f );
	assert( normalizedMaxUtilisation <= 1.0f );

	hashmap_t *map = Cast( hashmap_t *, malloc( sizeof( hashmap_t ) ) );
	map->capacity = startingCapacity;
	map->buckets = Cast( hashmapBucket_t *, malloc( startingCapacity * sizeof( hashmapBucket_t ) ) );
	map->shouldGrow = shouldGrow;
	map->maxUtilisation = normalizedMaxUtilisation;

	Hashmap_Reset( map );

	return map;
}

void Hashmap_Destroy( hashmap_t *map ) {
	assert( map );

	free( map->buckets );
	map->buckets = NULL;

	free( map );
	map = NULL;
}

inline void SetKeyAtIndex( hashmap_t *map, u32 index, u64 key ) {
	map->buckets[index].keyHi = Hashmap_InternalGetHiPart( key );
	map->buckets[index].keyLo = Hashmap_InternalGetLoPart( key );
}

void Hashmap_Reset( hashmap_t *map ) {
	For ( u32, i, 0, map->capacity ) {
		SetKeyAtIndex( map, TruncCast( u32, i ), HASHMAP_UNUSED_BUCKET );
		map->buckets[i].value = HASHMAP_INVALID_VALUE;
	}

	map->usageCount = 0;
	map->tombstoneCount = 0;
}

inline u32 TryGetIndexOfHash( const hashmap_t *map, const u64 key ) {
	u32 i = key % map->capacity;

	// Note(Tom): I think this is a legit use of const cast since it's purely for telemetry
	const_cast<hashmap_t *>( map )->lastLinearProbe = 0;

	u64 recombinedHash = Hashmap_InternalCombineAtIndex( map, i );

	while ( recombinedHash != key && recombinedHash != HASHMAP_UNUSED_BUCKET && map->lastLinearProbe < map->capacity ) {
		i = ( i + 1 ) % map->capacity;
		recombinedHash = Hashmap_InternalCombineAtIndex( map, i );
		const_cast<hashmap_t *>( map )->lastLinearProbe++;
	}

	return i;
}

u32 Hashmap_GetValue( const hashmap_t *map, const u64 key ) {
	assert( key != HASHMAP_UNUSED_BUCKET && "Key cannot equal empty bucket value (0)" );
	assert( key != HASHMAP_TOMBSTONE_BUCKET && "Key cannot equal Tombstone (u32 MAX)" );

	u32 i = TryGetIndexOfHash( map, key );

	if ( Hashmap_InternalCombineAtIndex( map, i ) != key ) {
#ifndef HASHMAP_HIDE_MISSING_KEY_WARNING
		Warning( "GET: Key %llu not found in hashmap\n", key );
#endif
		return HASHMAP_INVALID_VALUE;
	}

	return map->buckets[i].value;
}

void Hashmap_SetValue( hashmap_t *map, const u64 key, const u32 value ) {
	u32 i = TryGetIndexOfHash( map, key );
	u64 keyAtLocation = Hashmap_InternalCombineAtIndex( map, i );

	if ( keyAtLocation != key && keyAtLocation != HASHMAP_UNUSED_BUCKET ) {
		Warning( "SET: Key %llu or empty space not found in hashmap\n", key );
		return;
	}

	if ( keyAtLocation == HASHMAP_UNUSED_BUCKET ) {
		map->usageCount++;

		float32 utilization = Cast( float32, map->usageCount + map->tombstoneCount ) / Cast( float32, map->capacity );
		if ( utilization > map->maxUtilisation ) {
			if ( map->shouldGrow ) {
				hashmapBucket_t* oldBuckets = map->buckets;
				defer { free( oldBuckets ); };

				u32 oldCapacity = map->capacity;
				map->capacity = Max( Cast( u32, Cast( float32, map->capacity ) * 1.5f ), 2U );
				map->buckets = Cast( hashmapBucket_t *, malloc( map->capacity * sizeof( hashmapBucket_t ) ) );
				// Note(Tom): I don't love that this isn't a memset anymore. this isn's possible if we keep caring about values of unused buckets: unused value != empty bucket.
				// I suggest we start leaving them untouched. Yes they have stale old data in them, but so long as people are using set that should never be an issue
				// (we're testing this right?)
				// Alternatively we could swap tombstone and unused values round and then unused value == empty bucket and zero the entire damn thing :)
				Hashmap_Reset( map );

				For ( u32, oldBucketIndex, 0, oldCapacity ) {
					u64 keyInBucket = Hashmap_InternalCombine( oldBuckets[oldBucketIndex].keyHi, oldBuckets[oldBucketIndex].keyLo );
					if ( keyInBucket != HASHMAP_UNUSED_BUCKET && keyInBucket != HASHMAP_TOMBSTONE_BUCKET ) {
						Hashmap_SetValue( map, keyInBucket, oldBuckets[oldBucketIndex].value );
					}
				}

				// Finally add this one
				Hashmap_SetValue( map, key, value );
				return;
			} else {
				Warning( "Hashmap is above utilization of %f with %u buckets", map->maxUtilisation, map->capacity );
			}
		}
	}

	SetKeyAtIndex( map, i, key );
	map->buckets[i].value = value;
}

void Hashmap_RemoveKey( hashmap_t *map, const u64 key ) {
	assert( key != HASHMAP_UNUSED_BUCKET && "Key cannot equal empty bucket value (0)" );
	assert( key != HASHMAP_TOMBSTONE_BUCKET && "Key cannot equal Tombstone (u32 MAX)" );

	u32 i = TryGetIndexOfHash( map, key );
	u64 keyAtLocation = Hashmap_InternalCombineAtIndex( map, i );

	if ( keyAtLocation != key ) {
		Warning( "REMOVE: Key %llu not found in hashmap\n", key );
		return;
	}

	u32 next = ( i + 1 ) % map->capacity;
	if ( Hashmap_InternalCombineAtIndex( map, next ) != HASHMAP_UNUSED_BUCKET ) {
		SetKeyAtIndex( map, i, HASHMAP_TOMBSTONE_BUCKET );
		map->tombstoneCount++;
	} else {
		SetKeyAtIndex( map, i, HASHMAP_UNUSED_BUCKET );
		i = ( i - 1 ) % map->capacity;

 		while ( Hashmap_InternalCombineAtIndex( map, i ) == HASHMAP_TOMBSTONE_BUCKET ) {
			SetKeyAtIndex( map, i, HASHMAP_UNUSED_BUCKET );
			map->tombstoneCount--;
			i = ( i - 1 ) % map->capacity;
		}
	}

	map->usageCount--;
}

u64 Hashmap_InternalCombine( const u32 hi, const u32 lo ) {
	return ( Cast( u64, lo ) << 32 ) | hi;
}

u64 Hashmap_InternalCombineAtIndex( const hashmap_t *map, const u32 index ) {
	return Hashmap_InternalCombine( map->buckets[index].keyHi, map->buckets[index].keyLo );
}

u32 Hashmap_InternalGetLoPart( const u64 key ) {
	return TruncCast( u32, key >> 32 );
}

u32 Hashmap_InternalGetHiPart( const u64 key ) {
	return TruncCast( u32, key & 0xFFFFFFFF );
}
