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

#ifdef __linux__

#include <core_thread.h>

#include <core_helpers.h>
#include <debug.h>
#include <defer.h>
#include <typecast.inl>

#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>

struct ThreadBootstrapData {
	ThreadFunc	thread_func;
	void		*data;
};

static void *thread_bootstrap( void *data ) {
	ThreadBootstrapData *bootstrap = cast( ThreadBootstrapData *, data );

	s32 exit_code = bootstrap->thread_func( bootstrap->data );

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wint-to-pointer-cast"
	s32 *exit_code_ptr = cast( s32 *, exit_code );
#pragma clang diagnostic pop

	return cast( void *, exit_code_ptr );
}

Thread		thread_create( ThreadFunc thread_func, void *data, const bool8 run_immediately ) {
	unused( run_immediately );
	printf( "TODO: DM: implement run_immediately\n" );

	pthread_t thread_linux;

	pthread_attr_t attribs = {};
	pthread_attr_init( &attribs );
	defer { pthread_attr_destroy( &attribs ); };

	ThreadBootstrapData bootstrap_data = {
		.thread_func	= thread_func,
		.data			= data,
	};

	if ( pthread_create( &thread_linux, &attribs, &thread_bootstrap, &bootstrap_data ) != 0 ) {
		int err = errno;
		fatal_error( "Failed to create thread: %s\n", strerror( err ) );

		return { NULL };
	}

	return { cast( void *, thread_linux ) };
}

void		thread_destroy( Thread *thread ) {
	pthread_t *pthread_linux = cast( pthread_t *, thread->ptr );

	pthread_cancel( *pthread_linux );

	thread->ptr = NULL;
}

s32		thread_wait( Thread *thread ) {
	pthread_t *pthread_linux = cast( pthread_t *, thread->ptr );

	s32 *exit_code = NULL;

	if ( !pthread_join( *pthread_linux, cast( void **, &exit_code ) ) ) {
		int err = errno;
		fatal_error( "Failed to join thread: %s\n", strerror( err ) );

		return -1;
	}

	return *exit_code;
}

bool8		thread_suspend( Thread *thread ) {
	unused( thread );
	assert( false && "TODO: DM: this" );
	return false;
}

bool8		thread_resume( Thread *thread ) {
	unused( thread );
	assert( false && "TODO: DM: this" );
	return false;
}

bool8		semaphore_create( Semaphore *semaphore ) {
	unused( semaphore );
	assert( false && "TODO: DM: this" );
	return false;
}

bool8		semaphore_destroy( Semaphore *semaphore ) {
	unused( semaphore );
	assert( false && "TODO: DM: this" );
	return false;
}

void		semaphore_signal( Semaphore *semaphore ) {
	assert( semaphore );
}

s32		semaphore_wait( Semaphore *semaphore ) {
	unused( semaphore );
	assert( false && "TODO: DM: this" );
	return 0;
}

u32		atomic_increment( Atomic32 *atomic ) {
	unused( atomic );
	assert( false && "TODO: DM: this" );
	return 0;
}

u32		atomic_decrement( Atomic32* atomic ) {
	unused( atomic );
	assert( false && "TODO: DM: this" );
	return 0;
}

u32		atomic_compare_exchange( Atomic32* dst, const u32 compare, const u32 exchange ) {
	unused( dst );
	unused( compare );
	unused( exchange );
	assert( false && "TODO: DM: this" );
	return 0;
}

#endif
