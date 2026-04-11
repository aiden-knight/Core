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

#include "int_types.h"
#include "dll_export.h"

struct thread_t {
	void	*ptr;
};

struct semaphore_t {
	void	*ptr;
};

struct atomic32_t {
	volatile u32	value;
};

typedef s32 ( *threadFunc_t )( void *data );

// Creates and immediately executes a thread that runs 'threadFunc' with 'data' passed through.
CORE_API thread_t	Thread_Create( threadFunc_t threadFunc, void *data, const bool8 runImmediately = true );

// Waits for the thread to stop running, then destroys it.
CORE_API void		Thread_Destroy( thread_t *thread );

// Waits for the thread to stop running, returning the exit code when it finished.
CORE_API s32		Thread_Wait( thread_t *thread );

// Returns true if the thread was successfully suspended, otherwise returns false.
CORE_API bool8		Thread_Suspend( thread_t *thread );

// Returns true if the thread was successfully resumed, otherwise returns false.
CORE_API bool8		Thread_Resume( thread_t *thread );

// Returns true if the semaphore could be successfully created, otherwise returns false.
CORE_API bool8		Semaphore_Create( semaphore_t *semaphore );

// Returns true if the semaphore could be sucessfully destroyed, otherwise returns false.
CORE_API bool8		Semaphore_Destroy( semaphore_t *semaphore );

// Any threads blocked by this semaphore will become unblocked.
CORE_API void		Semaphore_Signal( semaphore_t *semaphore );

// Blocks the thread until the semaphore "wakes up" via a call to 'Semaphore_Signal'.
CORE_API s32		Semaphore_Wait( semaphore_t *semaphore );

// Performs an atomic increment.
CORE_API u32		AtomicIncrement( atomic32_t *atomic );

// Performs an atomic decrement.
CORE_API u32		AtomicDecrement( atomic32_t *atomic );

// If the value of 'dst' is the same as 'compare', then 'dst' gets set to 'exchange'.
// Returns the value of 'dst' before it got potentially changed.
CORE_API u32		AtomicCompareExchange( atomic32_t *dst, const u32 compare, const u32 exchange );
