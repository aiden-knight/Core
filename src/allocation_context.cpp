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

#include <allocation_context.h>
#include <allocator_malloc.h>
#include <allocator_linear.h>
#include <debug.h>
#include <cmd_line_args.h>

#include "core_local.h"

#include <memory.h>	// memcpy
#include <array.h>
#include <hashmap.h>

/*
================================================================================================

	allocation context

================================================================================================
*/

extern void core_init_platform();
extern void core_shutdown_platform();

// implicit context
// TODO(DM): 19/1/2023: this should be one per thread
static CoreContext g_core_context = {};
CoreContext* g_core_ptr = nullptr;


#ifdef CORE_MEMORY_TRACKING
	static MemoryTracking* init_memory_tracking();
	static void start_tracking_allocator(Allocator* allocator); 
#endif

void mem_allocator_intitialize(Allocator* allocator, u64 total_size){
	assert(!allocator->data);

	allocator->data = allocator->init(total_size);

#ifdef CORE_MEMORY_TRACKING
	start_tracking_allocator(allocator);
#endif
}
static Allocator get_bottom_allocator()
{
	Allocator malloc_allocator;
	// Note(Tom): Unlike other allocators malloc is stateless and doesn't need initting
	malloc_allocator_create_generic_interface(malloc_allocator);
	return malloc_allocator;
}

void core_init( const u64 allocator_size, const u64 temp_storage_size ) {
	assert( allocator_size );
	assert( temp_storage_size );
	//Note(TOM) unused for now. My thoughts are that perhaps you configure your programs element 1 allocator youself
	unused(allocator_size);
	g_core_ptr = &g_core_context;

	g_core_context.current_stack_size = 0U;
	For(u32, i, 0U, MAX_ALLOCATOR_STACK_SIZE){
		g_core_context.allocator_stack[i] = nullptr;
	}

	static Allocator s_bottom_allocator = get_bottom_allocator();
	mem_push_allocator(&s_bottom_allocator);

	linear_allocator_create_generic_interface(g_core_context.temp_storage);
	mem_allocator_intitialize(&g_core_context.temp_storage, temp_storage_size);

#ifdef CORE_MEMORY_TRACKING
	g_core_context.memory_tracking = init_memory_tracking();
#endif

	core_init_platform();
}

void mem_push_allocator(Allocator* allocator){
	assert(g_core_ptr);

	assert(g_core_ptr->current_stack_size + 1 < MAX_ALLOCATOR_STACK_SIZE);

	g_core_ptr->allocator_stack[g_core_ptr->current_stack_size++] = allocator;
}
void mem_pop_allocator(){
	assert(g_core_ptr);
	assertf(g_core_ptr->current_stack_size > 1, "Cannot pop passed the bottom allocator");
	g_core_ptr->current_stack_size--;
	g_core_ptr->allocator_stack[g_core_ptr->current_stack_size] = nullptr;
}

void core_shutdown() {
	assert(g_core_ptr);
	core_shutdown_platform();

	// shutdown default allocators
	{

	}
}

void core_hook( CoreContext* context ) {
	assert( context );

	g_core_ptr = context;
}

void* mem_alloc_internal( const u64 size ) {
	assert(g_core_ptr);
	Allocator* current = g_core_ptr->allocator_stack[g_core_ptr->current_stack_size - 1];
	return current->allocate(current->data, size);
}

void* mem_alloc_aligned_internal( const u64 size, const MemoryAlignment alignment ) {
	assert(g_core_ptr);
	Allocator* current = g_core_ptr->allocator_stack[g_core_ptr->current_stack_size-1];
	return current->allocate_aligned(current->data, size, alignment);
}

void* mem_realloc_internal( void* ptr, const u64 size ) {
	assert(g_core_ptr);
	Allocator* current = g_core_ptr->allocator_stack[g_core_ptr->current_stack_size - 1];
	return current->reallocate(current->data, ptr, size);
}

void* mem_realloc_aligned_internal( void* ptr, const u64 size, const MemoryAlignment alignment ){
	assert(g_core_ptr);
	Allocator* current = g_core_ptr->allocator_stack[g_core_ptr->current_stack_size - 1];
	return current->reallocate_aligned(current->data, ptr, size, alignment);
}

void mem_free_internal( void* ptr ) {
	assert(g_core_ptr);
	Allocator* current = g_core_ptr->allocator_stack[g_core_ptr->current_stack_size - 1];
	return current->free(current->data, ptr);
}

/*
Note(Tom): Resetting and shutting down allocators. Simple enough operations. However there are some gnarly cases to consider:
-Bottom Level Allocator (BLA) is initialized
-A linear allocator is built on top of the BLA to hold program lifetime data and be able to insta reset program memory
-A generic allocator is throw on top of linear for dynamic allocations

Note that this hierarchical structure does NOT reflect the allocator stack strucutre which closer resembles how the callstack
is switching contexts. When an allocator is popped it is not destroyed, merely no longer useful in the current context.
A copy of BLA's ptr is thrown onto the top of the stack whenever file IO functions are called for example, even though it's the foundation of the allocator hierarchy,
then popped at the end of those functions. Containers should (TODO: will) add their allocators before any realloc or free calls to make sure they are using the correct allocator they were created with
Therefore there is nothing STOPPING a programmer from pushing multiple copies* of an allocator onto the stack (and indeed if we forbade it, things like fileIO would currently break, so too would arrays maps etc).

So what should happen if linear allocator wants to reset- but generic allocator is still on the stack? 
-BLA (can't be reset, shutdown or popped)
-Linear
-Generic <--going to be a dangling ptr
-Linear pushed in order to reset it

That's a problem, since the generic allocator's ptr will remain on the allocator stack and when linear resets be dangling
Sure if you observe the stack you can see that rather than pushing linear you could just as easily pop generic in order to reset, but this example is 
contrived for simplicity. In reality there could be a complex multigenerational parent -> grandchild issue here that isn't so staightforward.

At the very least should we cleanup the allocator stack, removing dangling children and shifting down?
This might cause issues however, as the callstack unwinds a function may have had it's context pulled out from under it and not be able to function.

This feels like a footgun.

What I suggest is that if you have any children or decendents when shutting down or resetting we yell at you loudly and assert. This forces you to both shutdown dedendants and perhaps have them
e popped off the stack before you can reset or shutdown them down. However that sort of defeats the point of that linear allocator, that sits there to be able to wipe and resart your app without
having to "destruct" everything in your program. So, perhaps a "TRUST ME BRO" flag should be passed through as an argument- in which case the decendants will just be removed from the stack
and you gotta deal with the fallout if there is any.

If you write good procedural code with a short call stack and little context ping ponging you may never encounter a problem. But at the very least we should have some sort of mechanism in place
to be able to TELL you if you are aiming a desert eagle at your foot, if only to aid debugging issues that arise.

It could be argued that a free could cause the same issue- what happens if you free an allocator currently in use? <y gut is currently saying that that isn't something that we should (or even could)do anything about.

It's a managed language. Get gud

* "Therefore there is nothing STOPPING a programmer from pushing multiple copies of an allocator onto the stack". Maybe there should be. 
Maybe the default case is "oi this is already on the stack". Then you get a "Trust me bro" flag for exceptions like file io and containers?
Maybe that's the simplest solution to this limb remover. This would mean we wouldn't need to track parent/child relationships.

*/

void mem_reset_allocator_internal(){
	Allocator* current = g_core_ptr->allocator_stack[g_core_ptr->current_stack_size - 1];
	current->reset(current->data);
}

void mem_shutdown_allocator_internal(){
	Allocator* current = g_core_ptr->allocator_stack[g_core_ptr->current_stack_size - 1];
	current->shutdown(current->data);
}

#ifdef CORE_MEMORY_TRACKING

enum MemoryTrackingFlag
{
	MTF_IGNORE = 1,
	MTF_IS_ALLOCATOR = 2,
	MTF_ALLOW_ALLOCATOR_NUKING = 4
};

struct Allocation
{
	const char* function;
	u32 line;
	void* ptr;
	bool is_allocator;
};

struct AllocatorTrackingData
{
	Array<Allocation> allocations;
	Hashmap* allocation_lookup;
};

struct MemoryTracking
{
	Array<AllocatorTrackingData> allocator_tracking_data;
	Hashmap* allocator_tracking_lookup;
	u32 flags;
};

static MemoryTracking* init_memory_tracking(){
	g_core_ptr->memory_tracking = cast(MemoryTracking*)mem_alloc_internal(sizeof(MemoryTracking));

	g_core_ptr->memory_tracking->allocator_tracking_lookup = hashmap_create(32U);
	g_core_ptr->memory_tracking->allocator_tracking_data = Array<AllocatorTrackingData>();
}

static void set_memeory_tracking_flag(MemoryTrackingFlag flag, bool active){
	if(active)
	{
		g_core_ptr->memory_tracking->flags |= flag;
	}
	else
	{
		g_core_ptr->memory_tracking->flags &= ~flag;
	}
}

static bool is_memeory_tracking_flag_active(MemoryTrackingFlag flag){
	return (g_core_ptr->memory_tracking->flags & flag) != 0;
}

static void start_tracking_allocator(Allocator* allocator){
	
	MemoryTracking* memory_tracking = g_core_ptr->memory_tracking;
	assert(hashmap_get_value(memory_tracking->allocator_tracking_lookup, cast(u64)allocator) == HASHMAP_INVALID_VALUE);

	AllocatorTrackingData tracking;
	tracking.allocations = Array<Allocation>();
	tracking.allocation_lookup = hashmap_create(64U);

	memory_tracking->allocator_tracking_data.add(tracking);
	u32 index = memory_tracking->allocator_tracking_data.count-1;
	hashmap_set_value(memory_tracking->allocator_tracking_lookup, cast(u64)allocator, index);
}

static AllocatorTrackingData* get_current_tracking_data()
{
	MemoryTracking* memory_tracking = g_core_ptr->memory_tracking;
	Allocator* current_allocator = g_core_ptr->allocator_stack[g_core_ptr->current_stack_size-1];
	u32 index = hashmap_get_value(memory_tracking->allocator_tracking_lookup, cast(u64)current_allocator);
	assert(index != HASHMAP_INVALID_VALUE);
	return &memory_tracking->allocator_tracking_data[index];
}

void* track_allocation_internal(void* allocation, char* function, u32 line_number){
	
	AllocatorTrackingData* allocator_data = get_current_tracking_data();
	
	assert(hashmap_get_value(allocator_data->allocation_lookup, cast(u64)allocation) == HASHMAP_INVALID_VALUE);

	Allocation allocation_data = {function, line_number, allocation, is_memeory_tracking_flag_active(MTF_IS_ALLOCATOR)};

	allocator_data->allocations.add(allocation_data);
	u32 index = allocator_data->allocations.count -1U;

	hashmap_set_value(allocator_data->allocation_lookup, cast(u64)allocation, index);
}

void track_free_internal(void* free){

	AllocatorTrackingData* allocator_data = get_current_tracking_data();
	u32 index = hashmap_get_value(allocator_data->allocation_lookup, cast(u64)free);
	assertf(index != HASHMAP_INVALID_VALUE, "Pointer freed was never allocated in the first place");
	
	if(allocator_data->allocations[index].is_allocator)
	{
		// TODO(Tom):Check safety
	}

	allocator_data->allocations.swap_remove_at(index);
	
	// Patch up the allocators lookup info that got swaped into index's place
	if(index < allocator_data->allocations.count){
		hashmap_set_value(allocator_data->allocation_lookup, cast(u64)allocator_data->allocations[index].ptr, index );
	}
}

void mem_allow_allocator_nuking(bool allow){
	set_memeory_tracking_flag(MTF_ALLOW_ALLOCATOR_NUKING, allow);
}

void track_free_whole_allocator_internal(bool stop_tracking){
	unused(stop_tracking);
}
#endif