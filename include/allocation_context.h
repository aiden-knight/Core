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

#pragma once

#include "core_types.h"
#include "memory_units.h"

struct AllocatorGeneric;
struct AllocatorLinearData;

struct Paths;

constexpr MemoryAlignment DEFAULT_MEMORY_ALIGNMENT = MEMORY_ALIGNMENT_EIGHT;

typedef void*	( *allocator_init )( const u64 max_size );
typedef void	( *allocator_shutdown )( void* allocator_data );
typedef void*	( *allocator_allocate_aligned )( void* allocator_data, const u64 size, const MemoryAlignment alignment );
typedef void*	( *allocator_reallocate_aligned )( void* allocator_data, void* ptr, const u64 new_size, const MemoryAlignment alignment );
typedef void	( *allocator_free )( void* allocator_data, void* ptr );
typedef void	( *allocator_reset )( void* allocator_data );

struct Allocator {
	allocator_init init;
	allocator_shutdown	shutdown;
	void*	allocate( void* allocator_data, const u64 size ) {return allocate_aligned(allocator_data, size, DEFAULT_MEMORY_ALIGNMENT);}
	allocator_allocate_aligned allocate_aligned;
	void*	reallocate ( void* allocator_data, void* ptr, const u64 new_size ){return reallocate_aligned(allocator_data, ptr, new_size, DEFAULT_MEMORY_ALIGNMENT);}
	allocator_reallocate_aligned reallocate_aligned;
	allocator_free free;
	allocator_reset reset;

	void* data = nullptr;
};

void allocator_intitialize(Allocator* allocator, u64 total_size);

// implicit context
constexpr u32 MAX_ALLOCATOR_STACK_SIZE = 32;
struct CoreContext {
	Allocator*									allocator_stack[MAX_ALLOCATOR_STACK_SIZE];
	u32											current_stack_size;
	Allocator									temp_storage;

	Paths*										paths;
};

extern CoreContext*								g_core_ptr;

void											core_init( const u64 allocator_size, const u64 temp_storage_size );
void											core_shutdown( void );

void											core_hook( CoreContext* context );

void											mem_set_allocator( Allocator* allocator, void* allocator_data );

// DO NOT CALL THE INTERNAL FUNCTIONS
void*											mem_alloc_internal( const u64 size );
void*											mem_alloc_aligned_internal( const u64 size, const MemoryAlignment alignment );
void*											mem_realloc_internal( void* ptr, const u64 size );
void*											mem_realloc_aligned_internal( void* ptr, const u64 size, const MemoryAlignment alignment );
void											mem_free_internal( void* ptr );

void											mem_push_allocator(Allocator* allocator);
void											mem_pop_allocator();

// call these ones instead!
#define mem_alloc( size )							mem_alloc_internal( (size))
#define mem_alloc_aligned( size, alignment )		mem_alloc_aligned_internal( (size), cast( MemoryAlignment ) (alignment) )
#define mem_realloc( ptr, size )					mem_realloc_internal( (ptr), (size) )
#define mem_realloc_aligned( ptr, size, alignment )	mem_realloc_aligned_internal( (ptr), (size), cast( MemoryAlignment ) (alignment) )
#define mem_free( ptr )								mem_free_internal( (ptr) )