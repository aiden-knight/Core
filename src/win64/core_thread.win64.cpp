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

struct ThreadBootstrapData {
	ThreadFunc	thread_func;
	void		*data;
};

static DWORD thread_bootstrap( void *data ) {
	assert( data );

	ThreadBootstrapData *bootstrap = cast( ThreadBootstrapData *, data );

	assert( bootstrap->thread_func );

	s32 exit_code = bootstrap->thread_func( bootstrap->data );

	DWORD exit_code_dword = cast( DWORD, exit_code );

	return exit_code_dword;
}

Thread thread_create( ThreadFunc thread_func, void *data ) {
	assert( thread_func );
	//assert( data );

	// bootstrap data cant be local
	// could go out of scope by the time the thread actually fires
	// TODO(DM): 24/03/2026: do we just pass allocator here so people can specify what allocator this goes on to?
	// if NULL allocator then just malloc?
	ThreadBootstrapData *bootstrap = cast( ThreadBootstrapData *, malloc( sizeof( ThreadBootstrapData ) ) );
	bootstrap->thread_func = thread_func;
	bootstrap->data = data;

	HANDLE handle = CreateThread( NULL, 0, thread_bootstrap, bootstrap, 0, 0 );

	if ( handle == NULL ) {
		return { NULL };
	}

	return { handle };
}

void thread_destroy( Thread *thread ) {
	assert( thread );
	assert( thread->ptr );

	// TODO(DM): 03/02/2026: is this expected behaviour user-side?
	// do we make users do this themselves?
	thread_wait( thread );

	CloseHandle( cast( HANDLE, thread->ptr ) );
	thread->ptr = NULL;
}

s32 thread_wait( Thread *thread ) {
	assert( thread );
	assert( thread->ptr );

	HANDLE handle = cast( HANDLE, thread->ptr );

	DWORD result = WaitForSingleObjectEx( handle, INFINITE, TRUE );

	assert( result != WAIT_FAILED );
	unused( result );

	DWORD exit_code = S32_MAX;
	if ( !GetExitCodeThread( handle, &exit_code ) ) {
		// TODO(DM): 24/03/2026: handle errors etc.
	}

	return trunc_cast( s32, exit_code );
}

#endif // _WIN32
