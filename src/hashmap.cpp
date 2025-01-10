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

Hashmap* hashmap_create( const u32 count ) {
	assert( count );

	Hashmap* map = cast( Hashmap* ) mem_alloc( sizeof( Hashmap ) );
	map->capacity = count;
	map->usage_count = 0U;
	map->tombstone_count = 0U;
	map->keys = cast( u64* ) mem_alloc( count * sizeof( u64 ) );
	map->values = cast( u32* ) mem_alloc( count * sizeof( u32 ) );

	hashmap_reset( map );

	return map;
}

void hashmap_destroy( Hashmap* map ) {
	assert( map );

	mem_free( map->values );
	map->values = NULL;

	mem_free( map->keys );
	map->keys = NULL;

	mem_free( map );
	map = NULL;
}

void hashmap_reset( Hashmap* map ) {
	memset( map->keys, 0x0U, map->capacity * sizeof( u64 ) );
	memset( map->values, 0xFFU, map->capacity * sizeof( u32 ) ); // 0xFF makes HASHMAP_INVALID_VALUE which is 0xFFFFFFF
}

u32 hashmap_get_value( const Hashmap* map, const u64 key ) {
	assertf(key != HASHMAP_UNUSED_BUCKET, "Key cannot equal empty bucket value (0u)");
	assertf(key != HASHMAP_TOMBSTONE_BUCKET, "Key cannot equal Tombstone (u32 MAX)");

	u32 i = key % map->capacity;

	// Note(Tom): I think this is a legit use of const cast since it's purely for telemetry
	const_cast<Hashmap*>(map)->last_linear_probe = 0U;
	while ( map->keys[i] != key && map->keys[i] != HASHMAP_UNUSED_BUCKET  && map->last_linear_probe < map->capacity) {
		i = ( i + 1 ) % map->capacity;
		const_cast<Hashmap*>(map)->last_linear_probe++;
	}

	if(map->keys[i] != key)
	{
		warning("GET: Key %d not found in hashmap\n", key);
		return HASHMAP_INVALID_VALUE;
	}

	return map->values[i];
}

void hashmap_set_value( Hashmap* map, const u64 key, const u32 value ) {
	u32 i = key % map->capacity;
	map->last_linear_probe = 0;
	while ( map->keys[i] != key && map->keys[i] != HASHMAP_UNUSED_BUCKET  && map->last_linear_probe < map->capacity) {
		i = ( i + 1 ) % map->capacity;
		map->last_linear_probe++;
	}

	if(map->keys[i] != key && map->keys[i] != HASHMAP_UNUSED_BUCKET)
	{
		warning("SET: Key %d or empty space not found in hashmap\n", key);
		return;
	}

	map->keys[i] = key;
	map->values[i] = value;
}

void hashmap_remove_key( Hashmap* map, const u64 key ){
	assertf(key != HASHMAP_UNUSED_BUCKET, "Key cannot equal empty bucket value (0u)");
	assertf(key != HASHMAP_TOMBSTONE_BUCKET, "Key cannot equal Tombstone (u32 MAX)");

	u32 i = key % map->capacity;

	map->last_linear_probe = 0U;
	while ( map->keys[i] != key && map->keys[i] != HASHMAP_UNUSED_BUCKET  && map->last_linear_probe < map->capacity) {
		i = ( i + 1 ) % map->capacity;
		map->last_linear_probe++;
	}

	if(map->keys[i] != key)
	{
		warning("REMOVE: Key %d not found in hashmap\n", key);
		return;
	}

	u32 next = (i + 1) % map->capacity;
	if(map->keys[next] != HASHMAP_UNUSED_BUCKET)
	{
		map->keys[i] = HASHMAP_TOMBSTONE_BUCKET;
		map->tombstone_count++;
	}
	else
	{
		map->keys[i] = HASHMAP_UNUSED_BUCKET;
		i = (i - 1) % map->capacity;

 		while (map->keys[i] == HASHMAP_TOMBSTONE_BUCKET)
        {
            map->keys[i] = HASHMAP_UNUSED_BUCKET;
			map->tombstone_count--;
            i = (i - 1) % map->capacity;
        }
	}
}