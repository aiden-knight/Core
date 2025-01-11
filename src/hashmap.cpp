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

#include <debug.h>
#include <allocation_context.h>

#include <memory.h>	// memset

/*
================================================================================================

	Hashmap

================================================================================================
*/

Hashmap* hashmap_create( const u32 capacity ) {
	assert( capacity );

	Hashmap* map = cast( Hashmap* ) mem_alloc( sizeof( Hashmap ) );
	map->capacity = capacity;
	map->usage_count = 0U;
	map->tombstone_count = 0U;
	map->buckets = cast( HashmapBucket*)mem_alloc(capacity * sizeof( HashmapBucket ) );

	hashmap_reset( map );

	return map;
}

void hashmap_destroy( Hashmap* map ) {
	assert( map );

	mem_free( map->buckets );
	map->buckets = NULL;

	mem_free( map );
	map = NULL;
}

inline void set_key_at_index(Hashmap* map, u32 index, u64 key)
{
	map->buckets[index].key_hi = hashmap_get_hi_part(key);
	map->buckets[index].key_lo = hashmap_get_lo_part(key);
}

void hashmap_reset( Hashmap* map ) {

	For(u32, i, 0, map->capacity)
	{
		set_key_at_index(map, i, HASHMAP_UNUSED_BUCKET);
		map->buckets[i].value = HASHMAP_INVALID_VALUE;
	}
}

inline u32 try_get_index_of_hash(const Hashmap* map, const u64 key)
{
	u32 i = key % map->capacity;

	// Note(Tom): I think this is a legit use of const cast since it's purely for telemetry
	const_cast<Hashmap*>(map)->last_linear_probe = 0U;
	u64 recombined_hash = hashmap_combine_at_index(map, i);
	while ( recombined_hash != key && recombined_hash != HASHMAP_UNUSED_BUCKET && map->last_linear_probe < map->capacity) {
		i = ( i + 1 ) % map->capacity;
		recombined_hash = hashmap_combine_at_index(map, i);
		const_cast<Hashmap*>(map)->last_linear_probe++;
	}

	return i;
}

u32 hashmap_get_value( const Hashmap* map, const u64 key ) {
	assertf(key != HASHMAP_UNUSED_BUCKET, "Key cannot equal empty bucket value (0u)");
	assertf(key != HASHMAP_TOMBSTONE_BUCKET, "Key cannot equal Tombstone (u32 MAX)");

	u32 i = try_get_index_of_hash(map, key);

	if(hashmap_combine_at_index(map, i) != key)
	{
		warning("GET: Key %llu not found in hashmap\n", key);
		return HASHMAP_INVALID_VALUE;
	}

	return map->buckets[i].value;
}

void hashmap_set_value( Hashmap* map, const u64 key, const u32 value ) {
	u32 i = try_get_index_of_hash(map, key);
	u64 key_at_location = hashmap_combine_at_index(map, i);

	if(key_at_location != key && key_at_location != HASHMAP_UNUSED_BUCKET)
	{
		warning("SET: Key %llu or empty space not found in hashmap\n", key);
		return;
	}

	if (key_at_location == HASHMAP_UNUSED_BUCKET)
	{
		map->usage_count++;
	}	

	set_key_at_index(map, i, key);
	map->buckets[i].value = value;
}

void hashmap_remove_key( Hashmap* map, const u64 key ){
	assertf(key != HASHMAP_UNUSED_BUCKET, "Key cannot equal empty bucket value (0u)");
	assertf(key != HASHMAP_TOMBSTONE_BUCKET, "Key cannot equal Tombstone (u32 MAX)");

	u32 i = try_get_index_of_hash(map, key);
	u64 key_at_location = hashmap_combine_at_index(map, i);

	if(key_at_location != key)
	{
		warning("REMOVE: Key %llu not found in hashmap\n", key);
		return;
	}

	u32 next = (i + 1) % map->capacity;
	if(hashmap_combine(map->buckets[next].key_hi, map->buckets[next].key_lo) != HASHMAP_UNUSED_BUCKET)
	{
		set_key_at_index(map, i, HASHMAP_TOMBSTONE_BUCKET);
		map->tombstone_count++;
	}
	else
	{
		set_key_at_index(map, i, HASHMAP_UNUSED_BUCKET);
		i = (i - 1) % map->capacity;

 		while (hashmap_combine_at_index(map, i) == HASHMAP_TOMBSTONE_BUCKET)
        {
            set_key_at_index(map, i, HASHMAP_UNUSED_BUCKET);
			map->tombstone_count--;
            i = (i - 1) % map->capacity;
        }
	}
	map->usage_count--;
}