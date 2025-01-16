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

#include <temp_storage.h>
#include <allocation_context.h>
#include <allocator_linear.h>

#include <debug.h>
/*
================================================================================================

	temp storage

================================================================================================
*/

u64 mem_tell_temp_storage( void ){
	assert(g_core_ptr);
	return linear_allocator_tell(g_core_ptr->temp_storage);
}
void mem_rewind_temp_storage( const u64 position ){
	assert(g_core_ptr);
	linear_allocator_rewind(g_core_ptr->temp_storage, position );

}

void mem_reset_temp_storage() {
	assert(g_core_ptr);
	linear_allocator_reset(g_core_ptr->temp_storage);
}

void* mem_temp_alloc_internal( const u64 size) {
	assert(g_core_ptr);
	return linear_allocator_alloc(g_core_ptr->temp_storage, size, DEFAULT_MEMORY_ALIGNMENT);
}

void* mem_temp_alloc_aligned_internal( const u64 size, const MemoryAlignment alignment ){
	assert(g_core_ptr);
	return linear_allocator_alloc(g_core_ptr->temp_storage, size, alignment);
}