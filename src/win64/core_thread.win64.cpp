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
#ifdef _WIN32

#include <core_thread.h>

#include <debug.h>
#include <typecast.inl>
#include <core_helpers.h>

#include <Windows.h>

#include <stdio.h>
#include <malloc.h>

struct threadBootstrapData_t {
	threadFunc_t	threadFunc;
	void			*data;
};

static DWORD ThreadBootstrap( void *data ) {
	assert( data );

	threadBootstrapData_t *bootstrap = Cast( threadBootstrapData_t *, data );

	assert( bootstrap->threadFunc );

	s32 exitCode = bootstrap->threadFunc( bootstrap->data );

	DWORD exitCodeDword = Cast( DWORD, exitCode );

	return exitCodeDword;
}

thread_t Thread_Create( threadFunc_t threadFunc, void *data, const bool8 runImmediately ) {
	assert( threadFunc );

	// bootstrap data cant be local
	// could go out of scope by the time the thread actually fires
	// TODO: DM: 24/03/2026: do we just pass allocator here so people can specify what allocator this goes on to?
	// if NULL allocator then just malloc?
	threadBootstrapData_t *bootstrap = Cast( threadBootstrapData_t *, malloc( sizeof( threadBootstrapData_t ) ) );
	bootstrap->threadFunc = threadFunc;
	bootstrap->data = data;

	DWORD creationFlags = 0;

	if ( !runImmediately ) {
		creationFlags |= CREATE_SUSPENDED;
	}

	HANDLE handle = CreateThread( NULL, 0, ThreadBootstrap, bootstrap, creationFlags, NULL );

	return { handle };
}

void Thread_Destroy( thread_t *thread ) {
	assert( thread );
	assert( thread->ptr );

	CloseHandle( Cast( HANDLE, thread->ptr ) );
	thread->ptr = NULL;
}

s32 Thread_Wait( thread_t *thread ) {
	assert( thread );
	assert( thread->ptr );

	HANDLE handle = Cast( HANDLE, thread->ptr );

	DWORD result = WaitForSingleObject( handle, INFINITE );

	assert( result != WAIT_FAILED );
	Unused( result );

	DWORD exitCode = S32_MAX;
	if ( !GetExitCodeThread( handle, &exitCode ) ) {
		// TODO: DM: 24/03/2026: handle errors etc.
	}

	return TruncCast( s32, exitCode );
}

bool8 Thread_Suspend( thread_t *thread ) {
	return SuspendThread( thread->ptr ) == -1;
}

bool8 Thread_Resume( thread_t *thread ) {
	return ResumeThread( thread->ptr ) == -1;
}

bool8 Semaphore_Create( semaphore_t *semaphore ) {
	HANDLE handle = CreateSemaphore( NULL, 0, 1, NULL );

	if ( !handle ) {
		return false;
	}

	semaphore->ptr = handle;

	return true;
}

bool8 Semaphore_Destroy( semaphore_t *semaphore ) {
	if ( !CloseHandle( semaphore->ptr ) ) {
		return false;
	}

	semaphore->ptr = NULL;

	return true;
}

void Semaphore_Signal( semaphore_t *semaphore ) {
	ReleaseSemaphore( semaphore->ptr, 1, NULL );
}

s32 Semaphore_Wait( semaphore_t *semaphore ) {
	return TruncCast( s32, WaitForSingleObjectEx( semaphore->ptr, INFINITE, TRUE ) );
}

u32 AtomicIncrement( atomic32_t *atomic ) {
	return InterlockedIncrement( &atomic->value );
}

u32 AtomicDecrement( atomic32_t* atomic ) {
	return InterlockedDecrement( &atomic->value );
}

u32 AtomicCompareExchange( atomic32_t *dst, const u32 compare, const u32 exchange ) {
	return InterlockedCompareExchange( &dst->value, exchange, compare );
}

#endif // _WIN32
