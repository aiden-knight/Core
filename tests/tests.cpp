/*
===========================================================================

Core

Copyright (c) 2025 - present Dan Moody

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

#include "test_dll/test_dll.h"
#include "test_exe/test_exe.h"

#include "../include/int_types.h"
#include "../include/typecast.inl"
#include "../include/core_helpers.h"
#include "../include/linear_allocator.h"
#include "../include/defer.h"
#include "../include/core_string.h"
#include "../include/file.h"
#include "../include/core_math.h"
#include "../include/debug.h"
#include "../include/hash.h"
#include "../include/hashmap.h"
#include "../include/random.h"
#include "../include/timer.h"
#include "../include/string_builder.h"
#include "../include/library.h"
#include "../include/temp_storage.h"
#include "../include/core_thread.h"
#include "../include/core_array.inl"
#include "../include/paths.h"
#include "../include/core_process.h"

#define TEMPER_IMPLEMENTATION
#define TEMPERDEV_ASSERT assert
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "temper/temper.h"

#define PRINT_HASHMAP_PROBE_RESULTS 0

// temp storage is thread local and test runs in its own thread
// so each test needs to initialize their own version of temp storage
static void init_test_thread() {
	mem_init_temp_storage( MEM_KILOBYTES( 64 ) );
}

static void shutdown_test_thread() {
	mem_shutdown_temp_storage();
}

#define TEST( test_name, run_flag )					TEMPER_TEST_C( test_name, init_test_thread, shutdown_test_thread, run_flag )

#define TEST_PARAMETRIC( test_name, run_flag, ... )	TEMPER_TEST_PARAMETRIC_C( test_name, init_test_thread, shutdown_test_thread, run_flag, __VA_ARGS__ )

/*
================================================================================================

	core types

================================================================================================
*/

TEST( number_types_sizes, TEMPER_FLAG_SHOULD_RUN ) {
	TEMPER_CHECK_TRUE( sizeof( s8 )  == 1 );
	TEMPER_CHECK_TRUE( sizeof( s16 ) == 2 );
	TEMPER_CHECK_TRUE( sizeof( s32 ) == 4 );
	TEMPER_CHECK_TRUE( sizeof( s64 ) == 8 );

	TEMPER_CHECK_TRUE( sizeof( u8 )  == 1 );
	TEMPER_CHECK_TRUE( sizeof( u16 ) == 2 );
	TEMPER_CHECK_TRUE( sizeof( u32 ) == 4 );
	TEMPER_CHECK_TRUE( sizeof( u64 ) == 8 );

	TEMPER_CHECK_TRUE( sizeof( float32 ) == 4 );
	TEMPER_CHECK_TRUE( sizeof( float64 ) == 8 );
}

TEST( number_types_ranges, TEMPER_FLAG_SHOULD_RUN ) {
	TEMPER_CHECK_TRUE( S8_MIN  == ( -127 - 1 ) );
	TEMPER_CHECK_TRUE( S16_MIN == ( -32767 - 1 ) );
	TEMPER_CHECK_TRUE( S32_MIN == ( -2147483647 - 1 ) );
	TEMPER_CHECK_TRUE( S64_MIN == ( -9223372036854775807 - 1 ) );

	TEMPER_CHECK_TRUE( S8_MAX  == 127 );
	TEMPER_CHECK_TRUE( S16_MAX == 32767 );
	TEMPER_CHECK_TRUE( S32_MAX == 2147483647 );
	TEMPER_CHECK_TRUE( S64_MAX == 9223372036854775807 );

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wfloat-equal"
	TEMPER_CHECK_TRUE( FLOAT32_MIN == 1.17549435e-38f );
	TEMPER_CHECK_TRUE( FLOAT32_MAX == 3.40282347e+38f );

	TEMPER_CHECK_TRUE( FLOAT64_MIN == 2.2250738585072014e-308 );
	TEMPER_CHECK_TRUE( FLOAT64_MAX == 1.7976931348623157e+308 );
#pragma clang diagnostic pop
}


/*
================================================================================================

	core helpers

================================================================================================
*/

TEST( test_align_up, TEMPER_FLAG_SHOULD_RUN ) {
	// already aligned - should return same value
	TEMPER_CHECK_TRUE( align_up(  0, 8 ) ==  0 );
	TEMPER_CHECK_TRUE( align_up(  8, 8 ) ==  8 );
	TEMPER_CHECK_TRUE( align_up( 16, 8 ) == 16 );

	// not aligned - should round up to next multiple
	TEMPER_CHECK_TRUE( align_up(  1, 8 ) ==  8 );
	TEMPER_CHECK_TRUE( align_up(  7, 8 ) ==  8 );
	TEMPER_CHECK_TRUE( align_up(  9, 8 ) == 16 );

	// larger alignments
	TEMPER_CHECK_TRUE( align_up(   1, 4096 ) == 4096 );
	TEMPER_CHECK_TRUE( align_up( 100, 4096 ) == 4096 );
	TEMPER_CHECK_TRUE( align_up( 4096, 4096 ) == 4096 );
	TEMPER_CHECK_TRUE( align_up( 4097, 4096 ) == 8192 );
}


/*
================================================================================================

	linear allocator

================================================================================================
*/

static LinearAllocator *g_test_linear_allocator = NULL;

struct Thing {
	int			x;
	const char	*msg;
};

TEST_PARAMETRIC( test_linear_allocator_create, TEMPER_FLAG_SHOULD_RUN, LinearAllocator **allocator, const u64 reserved_bytes ) {
	*allocator = linear_allocator_create( reserved_bytes );

	TEMPER_CHECK_TRUE( *allocator != NULL );
	TEMPER_CHECK_TRUE( ( *allocator )->ptr != NULL );
	TEMPER_CHECK_TRUE( ( *allocator )->offset == 0 );
	TEMPER_CHECK_TRUE( ( *allocator )->reserved_bytes == reserved_bytes );
	TEMPER_CHECK_TRUE( ( *allocator )->comitted_bytes == 0 );
	TEMPER_CHECK_TRUE( ( *allocator )->virtual_memory_page_size > 0 );
}

TEST_PARAMETRIC( test_linear_allocator_alloc, TEMPER_FLAG_SHOULD_RUN, LinearAllocator *allocator, const Thing thing ) {
	u64 offset_before = linear_allocator_tell( allocator );
	TEMPER_CHECK_TRUE( offset_before == allocator->offset );

	Thing *ptr = cast( Thing *, linear_allocator_alloc( allocator, sizeof( Thing ) ) );
	*ptr = thing;

	TEMPER_CHECK_TRUE( ptr != NULL );
	TEMPER_CHECK_TRUE( allocator->offset == align_up( offset_before, 8U ) + sizeof( Thing ) );
	TEMPER_CHECK_TRUE( linear_allocator_tell( allocator ) == allocator->offset );
	TEMPER_CHECK_TRUE( ptr->x == thing.x );
	TEMPER_CHECK_TRUE( string_equals( ptr->msg, thing.msg ) );
}

TEST_PARAMETRIC( test_linear_allocator_reset, TEMPER_FLAG_SHOULD_RUN, LinearAllocator *allocator ) {
	u8 *ptr_before = allocator->ptr;
	u64 reserved_bytes_before = allocator->reserved_bytes;
	u64 comitted_bytes_before = allocator->comitted_bytes;
	u64 virtual_memory_page_size_before = allocator->virtual_memory_page_size;

	linear_allocator_reset( allocator );

	TEMPER_CHECK_TRUE( allocator->offset == 0 );
	TEMPER_CHECK_TRUE( allocator->ptr == ptr_before );
	TEMPER_CHECK_TRUE( allocator->reserved_bytes == reserved_bytes_before );
	TEMPER_CHECK_TRUE( allocator->comitted_bytes == comitted_bytes_before );
	TEMPER_CHECK_TRUE( allocator->virtual_memory_page_size == virtual_memory_page_size_before );
}

TEST_PARAMETRIC( test_linear_allocator_rewind_to, TEMPER_FLAG_SHOULD_RUN, LinearAllocator *allocator, const u64 alloc_size ) {
	u64 offset_before = allocator->offset;

	linear_allocator_alloc( allocator, alloc_size );

	linear_allocator_rewind_to( allocator, offset_before );

	TEMPER_CHECK_TRUE( allocator->offset == offset_before );
}

TEST_PARAMETRIC( test_linear_allocator_rewind_by, TEMPER_FLAG_SHOULD_RUN, LinearAllocator *allocator, const u64 alloc_size ) {
	u64 offset_before = allocator->offset;

	linear_allocator_alloc( allocator, alloc_size );

	u64 offset_after = allocator->offset;

	linear_allocator_rewind_by( allocator, offset_after - offset_before );

	TEMPER_CHECK_TRUE( allocator->offset == offset_before );
}

TEST_PARAMETRIC( test_linear_allocator_destroy, TEMPER_FLAG_SHOULD_RUN, LinearAllocator **allocator ) {
	TEMPER_CHECK_TRUE( *allocator != NULL );

	linear_allocator_destroy( *allocator );
	*allocator = NULL;

	TEMPER_CHECK_TRUE( *allocator == NULL );
}

TEMPER_INVOKE_PARAMETRIC_TEST( test_linear_allocator_create, &g_test_linear_allocator, 1024 * 1024 );

TEMPER_INVOKE_PARAMETRIC_TEST( test_linear_allocator_alloc, g_test_linear_allocator, { 1,   "hello"          } );
TEMPER_INVOKE_PARAMETRIC_TEST( test_linear_allocator_alloc, g_test_linear_allocator, { 2,   "world"          } );
TEMPER_INVOKE_PARAMETRIC_TEST( test_linear_allocator_alloc, g_test_linear_allocator, { 100, "this is a test" } );

TEMPER_INVOKE_PARAMETRIC_TEST( test_linear_allocator_reset, g_test_linear_allocator );

TEMPER_INVOKE_PARAMETRIC_TEST( test_linear_allocator_alloc, g_test_linear_allocator, { 256,  "after reset"         } );
TEMPER_INVOKE_PARAMETRIC_TEST( test_linear_allocator_alloc, g_test_linear_allocator, { 512,  "still going"         } );
TEMPER_INVOKE_PARAMETRIC_TEST( test_linear_allocator_alloc, g_test_linear_allocator, { 1024, "another one"         } );
TEMPER_INVOKE_PARAMETRIC_TEST( test_linear_allocator_alloc, g_test_linear_allocator, { 2048, "keeps on allocating" } );
TEMPER_INVOKE_PARAMETRIC_TEST( test_linear_allocator_alloc, g_test_linear_allocator, { 4096, "last one"            } );

TEMPER_INVOKE_PARAMETRIC_TEST( test_linear_allocator_rewind_to, g_test_linear_allocator, 64 );
TEMPER_INVOKE_PARAMETRIC_TEST( test_linear_allocator_rewind_to, g_test_linear_allocator, 128 );
TEMPER_INVOKE_PARAMETRIC_TEST( test_linear_allocator_rewind_to, g_test_linear_allocator, 256 );

TEMPER_INVOKE_PARAMETRIC_TEST( test_linear_allocator_rewind_by, g_test_linear_allocator, 32 );
TEMPER_INVOKE_PARAMETRIC_TEST( test_linear_allocator_rewind_by, g_test_linear_allocator, 96 );
TEMPER_INVOKE_PARAMETRIC_TEST( test_linear_allocator_rewind_by, g_test_linear_allocator, 512 );

TEMPER_INVOKE_PARAMETRIC_TEST( test_linear_allocator_destroy, &g_test_linear_allocator );


/*
================================================================================================

	Debug

================================================================================================
*/

TEST( test_get_callstack, TEMPER_FLAG_SHOULD_RUN ) {
	LinearAllocator *allocator = linear_allocator_create( 1024 * 1024 );
	defer { linear_allocator_destroy( allocator ); };

	Array<String> callstack = get_callstack( allocator );

	String expected = string_set( "test_get_callstack" );

	TEMPER_CHECK_TRUE( callstack.count > 0 );
	TEMPER_CHECK_TRUE( string_contains( &callstack[0], &expected ) );
}


/*
================================================================================================

	String

================================================================================================
*/

TEST( test_string_defaults, TEMPER_FLAG_SHOULD_RUN ) {
	String msg = {};

	TEMPER_CHECK_TRUE( msg.count == 0 );
	TEMPER_CHECK_TRUE( msg.data == NULL );
}

TEST_PARAMETRIC( test_string_set_c_string, TEMPER_FLAG_SHOULD_RUN, const char *str ) {
	String msg = string_set( str );

	TEMPER_CHECK_TRUE( string_equals( msg.data, str ) );
	TEMPER_CHECK_TRUE( msg.count == strlen( str ) );
}

TEMPER_INVOKE_PARAMETRIC_TEST( test_string_set_c_string, "test" );
TEMPER_INVOKE_PARAMETRIC_TEST( test_string_set_c_string, "this is only a test" );
TEMPER_INVOKE_PARAMETRIC_TEST( test_string_set_c_string, "" );
TEMPER_INVOKE_PARAMETRIC_TEST( test_string_set_c_string, "." );

TEST_PARAMETRIC( test_string_alloc, TEMPER_FLAG_SHOULD_RUN, const char *str ) {
	LinearAllocator *allocator = linear_allocator_create( 1024 * 1024 );
	defer { linear_allocator_destroy( allocator ); };

	String msg = string_alloc( allocator, str );

	TEMPER_CHECK_TRUE( string_equals( msg.data, str ) );
	TEMPER_CHECK_TRUE( msg.count == strlen( str ) );
}

TEMPER_INVOKE_PARAMETRIC_TEST( test_string_alloc, "test" );
TEMPER_INVOKE_PARAMETRIC_TEST( test_string_alloc, "this is only a test" );
// TEMPER_INVOKE_PARAMETRIC_TEST( test_string_alloc, "" );
TEMPER_INVOKE_PARAMETRIC_TEST( test_string_alloc, "." );

TEST_PARAMETRIC( test_string_copy, TEMPER_FLAG_SHOULD_RUN, const char *str ) {
	LinearAllocator *allocator = linear_allocator_create( 1024 * 1024 );
	defer { linear_allocator_destroy( allocator ); };

	String msg = string_alloc( allocator, str );
	String copy = string_copy( allocator, &msg );

	// check msg == str
	TEMPER_CHECK_TRUE( string_equals( string_cstr( &msg ), str ) );
	TEMPER_CHECK_TRUE( msg.count == strlen( str ) );

	// check copy == str
	TEMPER_CHECK_TRUE( string_equals( copy.data, str ) );
	TEMPER_CHECK_TRUE( copy.count == strlen( str ) );

	// check copy == msg
	TEMPER_CHECK_TRUE( string_equals( &copy, &msg ) );
	TEMPER_CHECK_TRUE( copy.count == msg.count );

	// check that the copy allocation is different from the original string allocation
	TEMPER_CHECK_TRUE( copy.data != msg.data );
}

TEMPER_INVOKE_PARAMETRIC_TEST( test_string_copy, "A" );
TEMPER_INVOKE_PARAMETRIC_TEST( test_string_copy, "This is a test" );
TEMPER_INVOKE_PARAMETRIC_TEST( test_string_copy, "This test . has a dot in it" );
TEMPER_INVOKE_PARAMETRIC_TEST( test_string_copy, "What about \t escape\n characters?" );
TEMPER_INVOKE_PARAMETRIC_TEST( test_string_copy, "      " );
// TEMPER_INVOKE_PARAMETRIC_TEST( test_string_copy, "" );

TEST_PARAMETRIC( test_string_starts_with, TEMPER_FLAG_SHOULD_RUN, const char *msg, const char *prefix, const bool8 should_match ) {
	LinearAllocator *allocator = linear_allocator_create( 1024 * 1024 );
	defer { linear_allocator_destroy( allocator ); };

	String str = string_alloc( allocator, msg );
	String expected_prefix = string_alloc( allocator, prefix );

	TEMPER_CHECK_TRUE( string_starts_with( msg, prefix ) == should_match );
	TEMPER_CHECK_TRUE( string_starts_with( &str, &expected_prefix ) == should_match );
}

TEMPER_INVOKE_PARAMETRIC_TEST( test_string_starts_with, "This is only a test", "T",                    true  );
TEMPER_INVOKE_PARAMETRIC_TEST( test_string_starts_with, "This is only a test", "This",                 true  );
TEMPER_INVOKE_PARAMETRIC_TEST( test_string_starts_with, "This is only a test", "This ",                true  );
TEMPER_INVOKE_PARAMETRIC_TEST( test_string_starts_with, "This is only a test", "This is",              true  );
TEMPER_INVOKE_PARAMETRIC_TEST( test_string_starts_with, "This is only a test", "this",                 false );
TEMPER_INVOKE_PARAMETRIC_TEST( test_string_starts_with, "This is only a test", "this is",              false );
TEMPER_INVOKE_PARAMETRIC_TEST( test_string_starts_with, "This is only a test", "this is ",             false );
TEMPER_INVOKE_PARAMETRIC_TEST( test_string_starts_with, "This is only a test", "w42950tuweiojfgase",   false );
// TEMPER_INVOKE_PARAMETRIC_TEST( test_string_starts_with, "This is only a test", "",                     true  );
TEMPER_INVOKE_PARAMETRIC_TEST( test_string_starts_with, "abc",                 "abcdef",               false );

TEMPER_TEST_PARAMETRIC( test_string_ends_with, TEMPER_FLAG_SHOULD_RUN, const char *msg, const char *suffix, const bool8 should_match ) {
	String str = string_set( msg );
	String expected_suffix = string_set( suffix );

	TEMPER_CHECK_TRUE( string_ends_with( msg, suffix ) == should_match );
	TEMPER_CHECK_TRUE( string_ends_with( &str, &expected_suffix ) == should_match );
}

TEMPER_INVOKE_PARAMETRIC_TEST( test_string_ends_with, "This is only a test", "t",     true  );
TEMPER_INVOKE_PARAMETRIC_TEST( test_string_ends_with, "This is only a test", "test",  true  );
TEMPER_INVOKE_PARAMETRIC_TEST( test_string_ends_with, "This is only a test", " test", true  );
TEMPER_INVOKE_PARAMETRIC_TEST( test_string_ends_with, "This is only a test", "T",     false );
TEMPER_INVOKE_PARAMETRIC_TEST( test_string_ends_with, "This is only a test", "tesT",  false );
TEMPER_INVOKE_PARAMETRIC_TEST( test_string_ends_with, "This is only a test", " TEST", false );
// TEMPER_INVOKE_PARAMETRIC_TEST( test_string_ends_with, "This is only a test", "",      true  );
TEMPER_INVOKE_PARAMETRIC_TEST( test_string_ends_with, "abc",                 "abcdef",false );

TEMPER_TEST_PARAMETRIC( test_string_ends_with_char, TEMPER_FLAG_SHOULD_RUN, const char *msg, const char end, const bool8 should_match ) {
	String str = string_set( msg );

	TEMPER_CHECK_TRUE( string_ends_with( msg, end ) == should_match );
	TEMPER_CHECK_TRUE( string_ends_with( &str, end ) == should_match );
}

TEMPER_INVOKE_PARAMETRIC_TEST( test_string_ends_with_char, "test", 't', true  );
TEMPER_INVOKE_PARAMETRIC_TEST( test_string_ends_with_char, "test", 's', false );
TEMPER_INVOKE_PARAMETRIC_TEST( test_string_ends_with_char, "test", 'T', false );
TEMPER_INVOKE_PARAMETRIC_TEST( test_string_ends_with_char, "a",    'a', true  );
TEMPER_INVOKE_PARAMETRIC_TEST( test_string_ends_with_char, "",     'x', false );

TEMPER_TEST_PARAMETRIC( test_string_contains, TEMPER_FLAG_SHOULD_RUN, const char *msg, const char *substring, const bool8 should_contain ) {
	String str = string_set( msg );
	String sub = string_set( substring );

	TEMPER_CHECK_TRUE( string_contains( msg, substring ) == should_contain );
	TEMPER_CHECK_TRUE( string_contains( &str, &sub ) == should_contain );
}

TEMPER_INVOKE_PARAMETRIC_TEST( test_string_contains, "This is only a test", "This is",             true  );
TEMPER_INVOKE_PARAMETRIC_TEST( test_string_contains, "This is only a test", "is only",             true  );
TEMPER_INVOKE_PARAMETRIC_TEST( test_string_contains, "This is only a test", " a test",             true  );
TEMPER_INVOKE_PARAMETRIC_TEST( test_string_contains, "This is only a test", " ",                   true  );
TEMPER_INVOKE_PARAMETRIC_TEST( test_string_contains, "This is only a test", "This is only a test", true  );
TEMPER_INVOKE_PARAMETRIC_TEST( test_string_contains, "This is only a test", "",                    true  );
TEMPER_INVOKE_PARAMETRIC_TEST( test_string_contains, "This is only a test", "this",                false );
TEMPER_INVOKE_PARAMETRIC_TEST( test_string_contains, "This is only a test", "ONLY",                false );
TEMPER_INVOKE_PARAMETRIC_TEST( test_string_contains, "abc",                 "abcdef",              false );

TEST_PARAMETRIC( test_string_replace, TEMPER_FLAG_SHOULD_RUN, const char *str, const char replace_old, const char replace_new, const char *expected_result ) {
	LinearAllocator *allocator = linear_allocator_create( 1024 * 1024 );
	defer { linear_allocator_destroy( allocator ); };

	String the_string = string_set( str );
	String actual_result = string_replace( allocator, &the_string, replace_old, replace_new );

	TEMPER_CHECK_TRUE( string_equals( actual_result.data, expected_result ) );
}

TEMPER_INVOKE_PARAMETRIC_TEST( test_string_replace, "this is only a test", ' ', '_', "this_is_only_a_test" );
TEMPER_INVOKE_PARAMETRIC_TEST( test_string_replace, "this is only a test", 't', 'T', "This is only a TesT" );
TEMPER_INVOKE_PARAMETRIC_TEST( test_string_replace, "this is only a test", 's', 'x', "thix ix only a text" );
TEMPER_INVOKE_PARAMETRIC_TEST( test_string_replace, "this is only a test", 'z', 'Z', "this is only a test" );
TEMPER_INVOKE_PARAMETRIC_TEST( test_string_replace, "this is only a test", 't', 't', "this is only a test" );

TEMPER_TEST_PARAMETRIC( test_string_find_from_left, TEMPER_FLAG_SHOULD_RUN, const char *str, const char c, const bool8 should_find, const u64 expected_index ) {
	String s = string_set( str );
	u64 index = 0;
	bool8 found = string_find_from_left( &s, c, &index );

	TEMPER_CHECK_TRUE( found == should_find );
	if ( should_find ) {
		TEMPER_CHECK_TRUE( index == expected_index );
	}
}

TEMPER_INVOKE_PARAMETRIC_TEST( test_string_find_from_left, "hello", 'h', true,  0 );
TEMPER_INVOKE_PARAMETRIC_TEST( test_string_find_from_left, "hello", 'e', true,  1 );
TEMPER_INVOKE_PARAMETRIC_TEST( test_string_find_from_left, "hello", 'o', true,  4 );
TEMPER_INVOKE_PARAMETRIC_TEST( test_string_find_from_left, "hello", 'l', true,  2 );
TEMPER_INVOKE_PARAMETRIC_TEST( test_string_find_from_left, "hello", 'z', false, 0 );
TEMPER_INVOKE_PARAMETRIC_TEST( test_string_find_from_left, "hello", 'H', false, 0 );

TEMPER_TEST_PARAMETRIC( test_string_find_from_right, TEMPER_FLAG_SHOULD_RUN, const char *str, const char c, const bool8 should_find, const u64 expected_index ) {
	String s = string_set( str );
	u64 index = 0;
	bool8 found = string_find_from_right( &s, c, &index );

	TEMPER_CHECK_TRUE( found == should_find );
	if ( should_find ) {
		TEMPER_CHECK_TRUE( index == expected_index );
	}
}

TEMPER_INVOKE_PARAMETRIC_TEST( test_string_find_from_right, "hello", 'o', true,  4 );
TEMPER_INVOKE_PARAMETRIC_TEST( test_string_find_from_right, "hello", 'e', true,  1 );
TEMPER_INVOKE_PARAMETRIC_TEST( test_string_find_from_right, "hello", 'h', true,  0 );
TEMPER_INVOKE_PARAMETRIC_TEST( test_string_find_from_right, "hello", 'l', true,  3 );
TEMPER_INVOKE_PARAMETRIC_TEST( test_string_find_from_right, "hello", 'z', false, 0 );
TEMPER_INVOKE_PARAMETRIC_TEST( test_string_find_from_right, "hello", 'H', false, 0 );

TEMPER_TEST_PARAMETRIC( test_string_equals, TEMPER_FLAG_SHOULD_RUN, const char *lhs, const char *rhs, const bool8 should_match ) {
	String lhs_str = string_set( lhs );
	String rhs_str = string_set( rhs );

	TEMPER_CHECK_TRUE( string_equals( lhs, rhs ) == should_match );
	TEMPER_CHECK_TRUE( string_equals( &lhs_str, &rhs_str ) == should_match );
}

TEMPER_INVOKE_PARAMETRIC_TEST( test_string_equals, "test",  "test",    true  );
TEMPER_INVOKE_PARAMETRIC_TEST( test_string_equals, "test",  "Test",    false );
TEMPER_INVOKE_PARAMETRIC_TEST( test_string_equals, "test",  "testing", false );
TEMPER_INVOKE_PARAMETRIC_TEST( test_string_equals, "test",  "tent",    false );
TEMPER_INVOKE_PARAMETRIC_TEST( test_string_equals, "",      "",        true  );
TEMPER_INVOKE_PARAMETRIC_TEST( test_string_equals, "",      "test",    false );
TEMPER_INVOKE_PARAMETRIC_TEST( test_string_equals, "test",  "",        false );

TEST( test_string_printf, TEMPER_FLAG_SHOULD_RUN ) {
	LinearAllocator *allocator = linear_allocator_create( 1024 * 1024 );
	defer { linear_allocator_destroy( allocator ); };

	{
		String result = string_printf( allocator, "hello %s", "world" );
		TEMPER_CHECK_TRUE( result.count == 11 );
		TEMPER_CHECK_TRUE( string_equals( string_cstr( &result ), "hello world" ) );
	}

	{
		String result = string_printf( allocator, "%d + %d = %d", 1, 2, 3 );
		TEMPER_CHECK_TRUE( result.count == 9 );
		TEMPER_CHECK_TRUE( string_equals( string_cstr( &result ), "1 + 2 = 3" ) );
	}

	{
		String result = string_printf( allocator, "no format args" );
		TEMPER_CHECK_TRUE( result.count == 14 );
		TEMPER_CHECK_TRUE( string_equals( string_cstr( &result ), "no format args" ) );
	}
}

TEST( test_temp_printf, TEMPER_FLAG_SHOULD_RUN ) {
	{
		const char *result = temp_printf( "hello %s", "world" );
		TEMPER_CHECK_TRUE( string_equals( result, "hello world" ) );
		TEMPER_CHECK_TRUE( strlen( result ) == 11 );
	}

	{
		const char *result = temp_printf( "%d + %d = %d", 1, 2, 3 );
		TEMPER_CHECK_TRUE( string_equals( result, "1 + 2 = 3" ) );
		TEMPER_CHECK_TRUE( strlen( result ) == 9 );
	}

	mem_reset_temp_storage();
}

TEST_PARAMETRIC( test_temp_c_string, TEMPER_FLAG_SHOULD_RUN, const char *str ) {
	char *result = temp_c_string( str );
	TEMPER_CHECK_TRUE( string_equals( result, str ) );
	mem_reset_temp_storage();
}

TEMPER_INVOKE_PARAMETRIC_TEST( test_temp_c_string, "hello" );
TEMPER_INVOKE_PARAMETRIC_TEST( test_temp_c_string, "this is only a test" );
TEMPER_INVOKE_PARAMETRIC_TEST( test_temp_c_string, "." );

TEST( test_temp_c_string_with_length, TEMPER_FLAG_SHOULD_RUN ) {
	char *result = temp_c_string( "hello world", 5 );
	TEMPER_CHECK_TRUE( string_equals( result, "hello" ) );
	mem_reset_temp_storage();
}


/*
================================================================================================

	hash32 and hash64

================================================================================================
*/

TEST_PARAMETRIC( hash_string_equals_hash32, TEMPER_FLAG_SHOULD_RUN, const char *string, const u32 seed, const u32 expected_hash ) {
	const u64 string_length = strlen( string );

	TEMPER_CHECK_TRUE( hash32( string, string_length, seed ) == expected_hash );
}

TEST_PARAMETRIC( hash_string_equals_hash64, TEMPER_FLAG_SHOULD_RUN, const char *string, const u64 seed, const u64 expected_hash ) {
	const u64 string_length = strlen( string );

	TEMPER_CHECK_TRUE( hash64( string, string_length, seed ) == expected_hash );
}

TEST_PARAMETRIC( hash32_is_deterministic, TEMPER_FLAG_SHOULD_RUN, const char *string, const u32 seed ) {
	const u64 length = strlen( string );

	TEMPER_CHECK_TRUE( hash32( string, length, seed ) == hash32( string, length, seed ) );
}

TEST_PARAMETRIC( hash64_is_deterministic, TEMPER_FLAG_SHOULD_RUN, const char *string, const u64 seed ) {
	const u64 length = strlen( string );

	TEMPER_CHECK_TRUE( hash64( string, length, seed ) == hash64( string, length, seed ) );
}

TEST_PARAMETRIC( hash32_seed_changes_output, TEMPER_FLAG_SHOULD_RUN, const char *string, const u32 seed_a, const u32 seed_b ) {
	const u64 length = strlen( string );

	TEMPER_CHECK_TRUE( hash32( string, length, seed_a ) != hash32( string, length, seed_b ) );
}

TEST_PARAMETRIC( hash64_seed_changes_output, TEMPER_FLAG_SHOULD_RUN, const char *string, const u64 seed_a, const u64 seed_b ) {
	const u64 length = strlen( string );

	TEMPER_CHECK_TRUE( hash64( string, length, seed_a ) != hash64( string, length, seed_b ) );
}

TEST_PARAMETRIC( hash32_length_changes_output, TEMPER_FLAG_SHOULD_RUN, const char *data, const u64 length_a, const u64 length_b ) {
	TEMPER_CHECK_TRUE( hash32( data, length_a, 0 ) != hash32( data, length_b, 0 ) );
}

TEST_PARAMETRIC( hash64_length_changes_output, TEMPER_FLAG_SHOULD_RUN, const char *data, const u64 length_a, const u64 length_b ) {
	TEMPER_CHECK_TRUE( hash64( data, length_a, 0 ) != hash64( data, length_b, 0 ) );
}

TEST_PARAMETRIC( hash_string_matches_hash64, TEMPER_FLAG_SHOULD_RUN, const char *string, const u64 seed ) {
	TEMPER_CHECK_TRUE( hash_string( string, seed ) == hash64( string, strlen( string ), seed ) );
}

TEST_PARAMETRIC( hasher_single_chunk_matches_hash64, TEMPER_FLAG_SHOULD_RUN, const char *data, const u64 seed ) {
	const u64 length = strlen( data );

	Hasher *hasher = hasher_create( mem_get_temp_storage(), seed );
	TEMPER_CHECK_TRUE_A( hasher );

	hasher_hash( hasher, data, length );
	const u64 result = hasher_get_hash( hasher );

	hasher_destroy( hasher );

	TEMPER_CHECK_TRUE( result == hash64( data, length, seed ) );

	mem_reset_temp_storage();
}

TEST_PARAMETRIC( hasher_multi_chunk_matches_single_chunk, TEMPER_FLAG_SHOULD_RUN, const char *data, const u64 seed ) {
	const u64 length = strlen( data );
	const u64 half   = length / 2;

	Hasher *hasher = hasher_create( mem_get_temp_storage(), seed );
	TEMPER_CHECK_TRUE_A( hasher );

	hasher_hash( hasher, data,        half );
	hasher_hash( hasher, data + half, length - half );
	const u64 multi_chunk = hasher_get_hash( hasher );

	hasher_reset( hasher, seed );

	hasher_hash( hasher, data, length );
	const u64 single_chunk = hasher_get_hash( hasher );

	hasher_destroy( hasher );

	TEMPER_CHECK_TRUE( multi_chunk == single_chunk );

	mem_reset_temp_storage();
}

TEST_PARAMETRIC( hasher_reset_gives_same_result, TEMPER_FLAG_SHOULD_RUN, const char *data, const u64 seed ) {
	const u64 length = strlen( data );

	Hasher *hasher = hasher_create( mem_get_temp_storage(), seed );
	TEMPER_CHECK_TRUE_A( hasher );

	hasher_hash( hasher, data, length );
	const u64 first_hash = hasher_get_hash( hasher );

	hasher_reset( hasher, seed );

	hasher_hash( hasher, data, length );
	const u64 second_hash = hasher_get_hash( hasher );

	hasher_destroy( hasher );

	TEMPER_CHECK_TRUE( first_hash == second_hash );

	mem_reset_temp_storage();
}

TEMPER_INVOKE_PARAMETRIC_TEST( hash_string_equals_hash32, "test_hash_string_value", 0, 163121569U );
TEMPER_INVOKE_PARAMETRIC_TEST( hash_string_equals_hash64, "test_hash_string_value", 0, 17747826225912071899ULL );

TEMPER_INVOKE_PARAMETRIC_TEST( hash32_is_deterministic, "hello",                  0  );
TEMPER_INVOKE_PARAMETRIC_TEST( hash32_is_deterministic, "hello",                  42 );
TEMPER_INVOKE_PARAMETRIC_TEST( hash32_is_deterministic, "test_hash_string_value", 0  );

TEMPER_INVOKE_PARAMETRIC_TEST( hash64_is_deterministic, "hello",                  0  );
TEMPER_INVOKE_PARAMETRIC_TEST( hash64_is_deterministic, "hello",                  42 );
TEMPER_INVOKE_PARAMETRIC_TEST( hash64_is_deterministic, "test_hash_string_value", 0  );

TEMPER_INVOKE_PARAMETRIC_TEST( hash32_seed_changes_output, "hello", 0, 1 );
TEMPER_INVOKE_PARAMETRIC_TEST( hash32_seed_changes_output, "hello", 0, 0xDEADBEEF );

TEMPER_INVOKE_PARAMETRIC_TEST( hash64_seed_changes_output, "hello", 0, 1 );
TEMPER_INVOKE_PARAMETRIC_TEST( hash64_seed_changes_output, "hello", 0, 0xDEADBEEFBAADF00DULL );

TEMPER_INVOKE_PARAMETRIC_TEST( hash32_length_changes_output, "foobar", 3, 6 );
TEMPER_INVOKE_PARAMETRIC_TEST( hash32_length_changes_output, "foobar", 1, 6 );

TEMPER_INVOKE_PARAMETRIC_TEST( hash64_length_changes_output, "foobar", 3, 6 );
TEMPER_INVOKE_PARAMETRIC_TEST( hash64_length_changes_output, "foobar", 1, 6 );

TEMPER_INVOKE_PARAMETRIC_TEST( hash_string_matches_hash64, "hello", 0  );
TEMPER_INVOKE_PARAMETRIC_TEST( hash_string_matches_hash64, "hello", 42 );
TEMPER_INVOKE_PARAMETRIC_TEST( hash_string_matches_hash64, "",      0  );

TEMPER_INVOKE_PARAMETRIC_TEST( hasher_single_chunk_matches_hash64, "Hello, world!", 0  );
TEMPER_INVOKE_PARAMETRIC_TEST( hasher_single_chunk_matches_hash64, "Hello, world!", 42 );

TEMPER_INVOKE_PARAMETRIC_TEST( hasher_multi_chunk_matches_single_chunk, "Hello, world!", 0  );
TEMPER_INVOKE_PARAMETRIC_TEST( hasher_multi_chunk_matches_single_chunk, "Hello, world!", 42 );

TEMPER_INVOKE_PARAMETRIC_TEST( hasher_reset_gives_same_result, "Hello, world!", 0  );
TEMPER_INVOKE_PARAMETRIC_TEST( hasher_reset_gives_same_result, "Hello, world!", 42 );


/*
================================================================================================

	Hashmap

================================================================================================
*/

TEST( test_hashmap_combine, TEMPER_FLAG_SHOULD_RUN ) {
	u32 lo_part = 0xDEADBEEF;
	u32 hi_part = 0xBAADF00D;

	u64 combined = hashmap_internal_combine( hi_part, lo_part );
	TEMPER_CHECK_TRUE_A( combined == 0xDEADBEEFBAADF00D );
	TEMPER_CHECK_TRUE_A( hashmap_internal_get_hi_part( combined ) == hi_part );
	TEMPER_CHECK_TRUE_A( hashmap_internal_get_lo_part( combined ) == lo_part );
}

TEST_PARAMETRIC( test_hashmap_create, TEMPER_FLAG_SHOULD_RUN, LinearAllocator **allocator, Hashmap **hashmap, const u32 count ) {
	TEMPER_CHECK_TRUE( allocator );
	TEMPER_CHECK_TRUE( !*allocator );
	TEMPER_CHECK_TRUE( hashmap );
	TEMPER_CHECK_TRUE( !*hashmap );

	TEMPER_CHECK_TRUE( count );

	*allocator = linear_allocator_create( 1024 * 1024 );

	*hashmap = hashmap_create( *allocator, count );

	TEMPER_CHECK_TRUE( ( *hashmap )->capacity == count );

	For ( u32, i, 0, ( *hashmap )->capacity ) {
		HashmapBucket& bucket = ( *hashmap )->buckets[i];
		TEMPER_CHECK_TRUE_A( hashmap_internal_combine( bucket.key_hi, bucket.key_lo ) == HASHMAP_UNUSED_BUCKET );
		TEMPER_CHECK_TRUE_A( bucket.value == HASHMAP_INVALID_VALUE );
	}
}

TEST_PARAMETRIC( test_hashmap_set_and_get_value, TEMPER_FLAG_SHOULD_RUN, Hashmap *hashmap, const char *name, const u32 age ) {
	//LogVerbosity previous_log_verbosity = get_log_verbosity();
	//set_log_verbosity( LOG_VERBOSITY_ERROR ); //Hiding warnings from getting values that don't exist as this is intentionally checking this behaviour

	TEMPER_CHECK_TRUE( hashmap );

	const u32 name_hash = hash32( name, strlen( name ), 0 );

	TEMPER_CHECK_TRUE( hashmap_get_value( hashmap, name_hash ) == HASHMAP_INVALID_VALUE );

	hashmap_set_value( hashmap, name_hash, age );

	TEMPER_CHECK_TRUE( hashmap_get_value( hashmap, name_hash ) == age );
	TEMPER_CHECK_TRUE( hashmap_get_value( hashmap, name_hash ) != HASHMAP_INVALID_VALUE );

	//set_log_verbosity( previous_log_verbosity );
}

TEST_PARAMETRIC( test_hashmap_reset, TEMPER_FLAG_SHOULD_RUN, Hashmap *hashmap ) {
	TEMPER_CHECK_TRUE( hashmap );

	u32 old_count = hashmap->capacity;

	hashmap_reset( hashmap );

	TEMPER_CHECK_TRUE( hashmap->capacity == old_count );

	For ( u32, i, 0, hashmap->capacity ) {
		HashmapBucket& bucket = hashmap->buckets[i];
		TEMPER_CHECK_TRUE_A( hashmap_internal_combine( bucket.key_hi, bucket.key_lo ) == HASHMAP_UNUSED_BUCKET );
		TEMPER_CHECK_TRUE_A( bucket.value == HASHMAP_INVALID_VALUE );
	}

	TEMPER_CHECK_TRUE_A(hashmap->usage_count == 0U);
	TEMPER_CHECK_TRUE_A(hashmap->tombstone_count == 0U);
}

TEST_PARAMETRIC( test_hashmap_remove, TEMPER_FLAG_SHOULD_RUN, Hashmap *hashmap ) {
	TEMPER_CHECK_TRUE( hashmap );

	hashmap_reset( hashmap );

	u32 count = hashmap->capacity;
	u32 first_key = count; 		// Bucket index % = 0
	u32 second_key = count * 2; // Bucket index % = 0
	u32 third_key = count + 1;	// Bucket index % = 1;

	u32 first_value = 69;
	u32 second_value = 70;
	u32 third_value = 90;

	hashmap_set_value( hashmap, first_key, first_value); 	// Actual bucket pos 0
	TEMPER_CHECK_TRUE_A( hashmap_internal_combine_at_index( hashmap, 0 ) == first_key );
	hashmap_set_value( hashmap, second_key, second_value ); // Actual bucket pos 1 (wanted 0)
	TEMPER_CHECK_TRUE_A( hashmap_internal_combine_at_index( hashmap, 1 ) == second_key );
	hashmap_set_value( hashmap, third_key, third_value );	// Actual bucket pos 2 (wanted 1)
	TEMPER_CHECK_TRUE_A( hashmap_internal_combine_at_index( hashmap, 2 ) == third_key );

	hashmap_remove_key( hashmap, second_key );

	// Check the tombstone from the second value is in place to allow proper gets
	TEMPER_CHECK_TRUE_A( hashmap_get_value( hashmap, third_key ) == third_value );

	hashmap_remove_key( hashmap, third_key );
	// Check tombstones were removed
	TEMPER_CHECK_TRUE_A( hashmap_internal_combine_at_index( hashmap, 2 ) == HASHMAP_UNUSED_BUCKET );
	TEMPER_CHECK_TRUE_A( hashmap_internal_combine_at_index( hashmap, 1 ) == HASHMAP_UNUSED_BUCKET );

	hashmap_set_value( hashmap, third_key, third_value );

	// Check that add won't probe passed removed tombstone
	TEMPER_CHECK_TRUE_A( hashmap_get_value( hashmap, third_key ) == third_value );
	TEMPER_CHECK_TRUE_A( hashmap_internal_combine_at_index( hashmap, 1 ) == third_key );
}

TEST_PARAMETRIC( test_hashmap_linear_probe_telemetry, TEMPER_FLAG_SHOULD_RUN, u32 number_of_buckets, float utilisation ) {
	LinearAllocator *allocator = linear_allocator_create( 1024 * 1024 );
	defer { linear_allocator_destroy( allocator ); };

	Hashmap* hashmap = hashmap_create( allocator, number_of_buckets, utilisation, false );
	u64 hash_seed = 0x9E3779B97F4A7C15;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wimplicit-int-float-conversion"
	u32 intended_fill = static_cast<u32>( number_of_buckets * utilisation );
#pragma GCC diagnostic pop

	auto get_hash_at_sequence = [&]( u32 sequence ) -> u64 {
		assert( sequence < hashmap->usage_count );

		u32 running_hash_count = -1U;
		u64 hash = HASHMAP_TOMBSTONE_BUCKET;

		For ( u32, key_index, 0, hashmap->capacity ) {
			u64 key_at_index = hashmap_internal_combine_at_index( hashmap, trunc_cast( u32, key_index ) );

			if ( key_at_index != HASHMAP_UNUSED_BUCKET && key_at_index != HASHMAP_TOMBSTONE_BUCKET ) {
				running_hash_count++;

				if ( running_hash_count == sequence ) {
					hash = key_at_index;
					break;
				}
			}
		}

		return hash;
	};

	//Setup test by bringing the utilisation up to utilitsation
	{
		u64 i = 0U;
		while ( hashmap->usage_count + hashmap->tombstone_count < intended_fill ) {
			if ( i % 3 != 2 ) {
				// Two adds for one remove
				constexpr u32 LENGTH_OF_RANDOM_FLOATS = 16u;
				float random_data[LENGTH_OF_RANDOM_FLOATS];

				For ( u32, float_index, 0, LENGTH_OF_RANDOM_FLOATS ) {
					random_data[float_index] = random_float32( 0.f, 1.f );
				}

				u64 hash = hash64( random_data, sizeof( float ) * LENGTH_OF_RANDOM_FLOATS, hash_seed );

				hashmap_set_value( hashmap, hash, 69 );
			} else {
				u32 to_remove = cast( u32, random_float32( 0, cast( float32, hashmap->usage_count - 1 ) ) );
				u64 hash = get_hash_at_sequence( to_remove );

				hashmap_remove_key( hashmap, hash );
			}

			i++;
		}
	}

	// get a bunch of random hashes and grab the results
	Array<u32> linear_probe_length = {};
	linear_probe_length.init( allocator );

	linear_probe_length.reserve( intended_fill );

	For ( u32, i, 0, hashmap->capacity ) {
		u64 hash = hashmap_internal_combine_at_index( hashmap, trunc_cast( u32, i ) );
		if ( hash == HASHMAP_TOMBSTONE_BUCKET || hash == HASHMAP_UNUSED_BUCKET ) {
			continue;
		}

		hashmap_get_value( hashmap, hash );
		linear_probe_length.add( hashmap->last_linear_probe );
	}

	// Analyze
	u32 biggest = 0U;
	float mean = 0.0f;
	float none_zero_mean = 0.0f;
	u32 num_zero_probes = 0U;
	For ( u64, i, 0, linear_probe_length.count ) {
		u32 probe = linear_probe_length[i];

		if ( probe > biggest ) {
			biggest = probe;
		}

		if ( probe == 0U ) {
			num_zero_probes++;
		} else {
			none_zero_mean += cast( float32, probe );
		}

		mean += cast( float32, probe );
		//warning( "%d", probe );
	}

	mean = mean / cast( float32, linear_probe_length.count );

	none_zero_mean = none_zero_mean / cast( float32, linear_probe_length.count - num_zero_probes );

#if PRINT_HASHMAP_PROBE_RESULTS
	print(
		"\n===\nPROBE RESULTS for %f pc utilization on %u buckets:\naverage probe length was %f, average of non zero was %f, biggest was %u. Num that were zero: %u\n Tombstone:Used: %u:%u\n",
		utilisation * 100.0f, number_of_buckets, mean, none_zero_mean, biggest, num_zero_probes, hashmap->tombstone_count, hashmap->usage_count
	);
#endif
}

static LinearAllocator *g_hashmap_allocator = NULL;
static Hashmap* g_hashmap = NULL;

TEMPER_INVOKE_PARAMETRIC_TEST( test_hashmap_create, &g_hashmap_allocator, &g_hashmap, 10 );

TEMPER_INVOKE_PARAMETRIC_TEST( test_hashmap_set_and_get_value, g_hashmap, "Dan",     27 );
TEMPER_INVOKE_PARAMETRIC_TEST( test_hashmap_set_and_get_value, g_hashmap, "Tom",     27 ); //Don't forget your buddy :) -- never <3
TEMPER_INVOKE_PARAMETRIC_TEST( test_hashmap_set_and_get_value, g_hashmap, "Mum",     53 );
TEMPER_INVOKE_PARAMETRIC_TEST( test_hashmap_set_and_get_value, g_hashmap, "Dad",     62 );
TEMPER_INVOKE_PARAMETRIC_TEST( test_hashmap_set_and_get_value, g_hashmap, "Grandad", 89 );

TEMPER_INVOKE_PARAMETRIC_TEST( test_hashmap_reset, g_hashmap );

TEMPER_INVOKE_PARAMETRIC_TEST( test_hashmap_remove, g_hashmap );

TEMPER_INVOKE_PARAMETRIC_TEST( test_linear_allocator_destroy, &g_hashmap_allocator );

TEST( test_hashmap_growing, TEMPER_FLAG_SHOULD_RUN ) {
	LinearAllocator *allocator = linear_allocator_create( 1024 * 1024 );
	defer { linear_allocator_destroy( allocator ); };

	Hashmap* map = hashmap_create( allocator, 10, 0.5f, true );
	u64 key = 10;
	u32 value = 10;

	hashmap_set_value( map, key++, value++ );
	hashmap_set_value( map, key++, value++ );
	hashmap_set_value( map, key++, value++ );
	hashmap_set_value( map, key++, value++ );
	hashmap_set_value( map, key++, value++ );
	// Should caust a regrow
	hashmap_set_value( map, key++, value++ );
	TEMPER_CHECK_TRUE_A( map->capacity == 15 );

	// In the new map, 10 is a value bucket index
	TEMPER_CHECK_TRUE( hashmap_internal_combine_at_index( map, 10 ) == 10 );
	// 15 isn't so it should wrap around
	TEMPER_CHECK_TRUE( hashmap_internal_combine_at_index( map, 0 ) == 15 );
}

TEMPER_INVOKE_PARAMETRIC_TEST( test_hashmap_linear_probe_telemetry, 10000, 0.75f );
TEMPER_INVOKE_PARAMETRIC_TEST( test_hashmap_linear_probe_telemetry, 10000, 0.5f  );
TEMPER_INVOKE_PARAMETRIC_TEST( test_hashmap_linear_probe_telemetry, 10000, 0.3f  );
TEMPER_INVOKE_PARAMETRIC_TEST( test_hashmap_linear_probe_telemetry, 10000, 0.1f  );


/*
================================================================================================

	File

================================================================================================
*/

TEST_PARAMETRIC( test_file_exists, TEMPER_FLAG_SHOULD_RUN, const char *filename, const bool8 expected ) {
	TEMPER_CHECK_TRUE( file_exists( filename ) == expected );
}

TEMPER_INVOKE_PARAMETRIC_TEST( test_file_exists, "build.cpp",                true  );
TEMPER_INVOKE_PARAMETRIC_TEST( test_file_exists, "include/file.h",           true  );
TEMPER_INVOKE_PARAMETRIC_TEST( test_file_exists, "include/paths.h",          true  );
TEMPER_INVOKE_PARAMETRIC_TEST( test_file_exists, "src/linear_allocator.cpp", true  );
TEMPER_INVOKE_PARAMETRIC_TEST( test_file_exists, "this_file_does_not_exist", false );
TEMPER_INVOKE_PARAMETRIC_TEST( test_file_exists, "include/fake_header.h",    false );
TEMPER_INVOKE_PARAMETRIC_TEST( test_file_exists, "src/fake_source_file.cpp", false );

static File        g_test_file              = {};
static File        g_test_file2             = {};
static const char *g_test_folder_path       = "bin/debug/core_test_folder";
static const char *g_test_subfolder_path    = "bin/debug/core_test_folder/subdir";
static const char *g_test_file_path         = "bin/debug/core_test_folder/core_test_file.txt";
static const char *g_test_file_entire_path  = "bin/debug/core_test_folder/core_test_file_entire.txt";
static const char *g_test_file_renamed_path = "bin/debug/core_test_folder/core_test_file_renamed.txt";
static const char *g_test_file_copy_path    = "bin/debug/core_test_folder/core_test_file_copy.txt";
static const char *g_test_subdir_file_path  = "bin/debug/core_test_folder/subdir/subdir_file.txt";

static void count_visited_files( const FileInfo *file_info, void *user_data ) {
	unused( file_info );
	u32 *count = cast( u32 *, user_data );
	*count += 1;
}

TEST_PARAMETRIC( test_folder_create, TEMPER_FLAG_SHOULD_RUN, const char *path ) {
	TEMPER_CHECK_TRUE_A( folder_create_if_it_doesnt_exist( path ) );
	TEMPER_CHECK_TRUE( folder_exists( path ) );
}

TEST_PARAMETRIC( test_folder_delete, TEMPER_FLAG_SHOULD_RUN, const char *path ) {
	TEMPER_CHECK_TRUE_A( folder_delete( path ) );
	TEMPER_CHECK_TRUE( !folder_exists( path ) );
}

TEST_PARAMETRIC( test_file_open, TEMPER_FLAG_SHOULD_RUN, File *file, const char *filename ) {
	*file = file_open( filename, FILE_OPEN_READ );
	TEMPER_CHECK_TRUE_A( file->handle != INVALID_FILE_HANDLE );
}

TEST_PARAMETRIC( test_file_close, TEMPER_FLAG_SHOULD_RUN, File *file ) {
	TEMPER_CHECK_TRUE( file_close( file ) );
}

TEST_PARAMETRIC( test_file_open_or_create, TEMPER_FLAG_SHOULD_RUN, File *file, const char *filename ) {
	*file = file_open_or_create( filename );
	TEMPER_CHECK_TRUE_A( file->handle != INVALID_FILE_HANDLE );
}

TEST_PARAMETRIC( test_file_write, TEMPER_FLAG_SHOULD_RUN, File *file, const char *content ) {
	TEMPER_CHECK_TRUE_A( file_write( file, content ) );
}

TEST_PARAMETRIC( test_file_write_buffer, TEMPER_FLAG_SHOULD_RUN, File *file, const void *data, const u64 size ) {
	TEMPER_CHECK_TRUE_A( file_write( file, data, size ) );
}

TEST_PARAMETRIC( test_file_write_at_offset, TEMPER_FLAG_SHOULD_RUN, File *file, const void *data, const u64 offset, const u64 size ) {
	TEMPER_CHECK_TRUE_A( file_write( file, data, offset, size ) );
}

TEST_PARAMETRIC( test_file_write_line, TEMPER_FLAG_SHOULD_RUN, File *file, const char *content ) {
	TEMPER_CHECK_TRUE_A( file_write_line( file, content ) );
}

TEST_PARAMETRIC( test_file_write_entire, TEMPER_FLAG_SHOULD_RUN, const char *filename, const void *data, const u64 size ) {
	TEMPER_CHECK_TRUE_A( file_write_entire( filename, data, size ) );
	TEMPER_CHECK_TRUE( file_exists( filename ) );
}

TEST_PARAMETRIC( test_file_get_size, TEMPER_FLAG_SHOULD_RUN, const char *filename, const u64 expected_size ) {
	u64 size = 0;
	TEMPER_CHECK_TRUE_A( file_get_size( filename, &size ) );
	TEMPER_CHECK_TRUE( size == expected_size );
}

TEST_PARAMETRIC( test_file_get_last_write_time, TEMPER_FLAG_SHOULD_RUN, const char *filename ) {
	u64 write_time = 0;
	TEMPER_CHECK_TRUE_A( file_get_last_write_time( filename, &write_time ) );
	TEMPER_CHECK_TRUE( write_time != 0 );
}

TEST_PARAMETRIC( test_file_read, TEMPER_FLAG_SHOULD_RUN, File *file, const u64 offset, const u64 size, const char *expected ) {
	char *buffer = cast( char *, mem_temp_alloc( ( size + 1 ) * sizeof( char ) ) );
	buffer[size] = 0;

	TEMPER_CHECK_TRUE_A( file_read( file, offset, size, buffer ) );
	TEMPER_CHECK_TRUE( string_equals( buffer, expected ) );

	mem_reset_temp_storage();
}

TEST_PARAMETRIC( test_file_read_sequential, TEMPER_FLAG_SHOULD_RUN, File *file, const u64 size, const char *expected ) {
	char *buffer = cast( char *, mem_temp_alloc( ( size + 1 ) * sizeof( char ) ) );
	buffer[size] = 0;

	TEMPER_CHECK_TRUE_A( file_read( file, size, buffer ) );
	TEMPER_CHECK_TRUE( string_equals( buffer, expected ) );

	mem_reset_temp_storage();
}

TEST_PARAMETRIC( test_file_read_entire, TEMPER_FLAG_SHOULD_RUN, const char *filename, const char *expected, const u64 expected_length ) {
	String buffer = {};

	TEMPER_CHECK_TRUE_A( file_read_entire( filename, &buffer ) );
	TEMPER_CHECK_TRUE( buffer.count == expected_length );
	TEMPER_CHECK_TRUE( string_equals( buffer.data, expected ) );

	file_free_buffer( &buffer );
	TEMPER_CHECK_TRUE( buffer.data == NULL );
	TEMPER_CHECK_TRUE( buffer.count == 0 );
}

TEST_PARAMETRIC( test_file_copy, TEMPER_FLAG_SHOULD_RUN, const char *src, const char *dst ) {
	TEMPER_CHECK_TRUE_A( file_copy( src, dst ) );
	TEMPER_CHECK_TRUE( file_exists( dst ) );
}

TEST_PARAMETRIC( test_file_get_all_files_in_folder, TEMPER_FLAG_SHOULD_RUN, const char *path, const FileVisitFlags visit_flags, const u32 expected_count ) {
	u32 count = 0;
	TEMPER_CHECK_TRUE_A( file_get_all_files_in_folder( path, visit_flags, count_visited_files, &count ) );
	TEMPER_CHECK_TRUE( count == expected_count );
}

TEST_PARAMETRIC( test_file_rename, TEMPER_FLAG_SHOULD_RUN, File *file, const char *old_path, const char *new_path ) {
	TEMPER_CHECK_TRUE_A( file_close( file ) );
	TEMPER_CHECK_TRUE_A( file_rename( old_path, new_path ) );
	TEMPER_CHECK_TRUE( file_exists( new_path ) );
	TEMPER_CHECK_TRUE( !file_exists( old_path ) );
}

TEST_PARAMETRIC( test_file_delete, TEMPER_FLAG_SHOULD_RUN, const char *filename ) {
	TEMPER_CHECK_TRUE_A( file_delete( filename ) );
	TEMPER_CHECK_TRUE( !file_exists( filename ) );
}

// folder: create
TEMPER_INVOKE_PARAMETRIC_TEST( test_folder_create, g_test_folder_path );

// create
TEMPER_INVOKE_PARAMETRIC_TEST( test_file_open_or_create, &g_test_file, g_test_file_path );

// write string content: "Hello, world!Goodbye, world!\n" = 29 bytes
TEMPER_INVOKE_PARAMETRIC_TEST( test_file_write,      &g_test_file, "Hello, world!" );
TEMPER_INVOKE_PARAMETRIC_TEST( test_file_write_line, &g_test_file, "Goodbye, world!\n" );

// write entire (creates a second file: "Written entirely" = 16 bytes)
TEMPER_INVOKE_PARAMETRIC_TEST( test_file_write_entire, g_test_file_entire_path, "Written entirely", 16 );

// size
TEMPER_INVOKE_PARAMETRIC_TEST( test_file_get_size, g_test_file_path,        30 );
TEMPER_INVOKE_PARAMETRIC_TEST( test_file_get_size, g_test_file_entire_path, 16 );

// last write time
TEMPER_INVOKE_PARAMETRIC_TEST( test_file_get_last_write_time, g_test_file_path );

// read with explicit offset
TEMPER_INVOKE_PARAMETRIC_TEST( test_file_read, &g_test_file, 0,  13, "Hello, world!"   );
TEMPER_INVOKE_PARAMETRIC_TEST( test_file_read, &g_test_file, 13, 15, "Goodbye, world!" );

// open a fresh read-only handle, do sequential reads, then close it
TEMPER_INVOKE_PARAMETRIC_TEST( test_file_open,            &g_test_file2, g_test_file_path      );
TEMPER_INVOKE_PARAMETRIC_TEST( test_file_read_sequential, &g_test_file2, 13, "Hello, world!"   );
TEMPER_INVOKE_PARAMETRIC_TEST( test_file_read_sequential, &g_test_file2, 15, "Goodbye, world!" );
TEMPER_INVOKE_PARAMETRIC_TEST( test_file_close,           &g_test_file2                        );

// read entire
TEMPER_INVOKE_PARAMETRIC_TEST( test_file_read_entire, g_test_file_entire_path, "Written entirely", 16 );

// write buffer: at g_test_file's current offset, overwrites "\n" with "!"
// file: "Hello, world!Goodbye, world!\!" (30 bytes)
TEMPER_INVOKE_PARAMETRIC_TEST( test_file_write_buffer, &g_test_file, "!", 1 );
TEMPER_INVOKE_PARAMETRIC_TEST( test_file_read,         &g_test_file, 27, 1, "!" );

// write at offset: patch "world" at offset 7 with "WORLD"
// file: "Hello, WORLD!Goodbye, world!\!" (30 bytes)
TEMPER_INVOKE_PARAMETRIC_TEST( test_file_write_at_offset, &g_test_file, "WORLD", 7, 5 );
TEMPER_INVOKE_PARAMETRIC_TEST( test_file_read,            &g_test_file, 7, 5, "WORLD"  );

// copy
TEMPER_INVOKE_PARAMETRIC_TEST( test_file_copy, g_test_file_path, g_test_file_copy_path );

// subfolder: create and populate for folder visit tests
TEMPER_INVOKE_PARAMETRIC_TEST( test_folder_create,       g_test_subfolder_path                    );
TEMPER_INVOKE_PARAMETRIC_TEST( test_file_open_or_create, &g_test_file2, g_test_subdir_file_path   );
TEMPER_INVOKE_PARAMETRIC_TEST( test_file_close,          &g_test_file2                             );

// folder: visit - files only in root (3: original, entire, copy)
TEMPER_INVOKE_PARAMETRIC_TEST( test_file_get_all_files_in_folder, g_test_folder_path, FILE_VISIT_FILES,   3 );

// folder: visit - folders only in root (1: subdir)
TEMPER_INVOKE_PARAMETRIC_TEST( test_file_get_all_files_in_folder, g_test_folder_path, FILE_VISIT_FOLDERS, 1 );

// folder: visit - files recursively (3 root + 1 subdir = 4)
TEMPER_INVOKE_PARAMETRIC_TEST( test_file_get_all_files_in_folder, g_test_folder_path, cast( FileVisitFlags, FILE_VISIT_FILES | FILE_VISIT_RECURSIVE ), 4 );

// folder: visit - files + folders recursively (4 files + 1 folder = 5)
TEMPER_INVOKE_PARAMETRIC_TEST( test_file_get_all_files_in_folder, g_test_folder_path, cast( FileVisitFlags, FILE_VISIT_FILES | FILE_VISIT_FOLDERS | FILE_VISIT_RECURSIVE ), 5 );

// cleanup: subfolder file then subfolder
TEMPER_INVOKE_PARAMETRIC_TEST( test_file_delete,   g_test_subdir_file_path );
TEMPER_INVOKE_PARAMETRIC_TEST( test_folder_delete, g_test_subfolder_path   );

// rename
TEMPER_INVOKE_PARAMETRIC_TEST( test_file_rename, &g_test_file, g_test_file_path, g_test_file_renamed_path );

// delete files
TEMPER_INVOKE_PARAMETRIC_TEST( test_file_delete, g_test_file_renamed_path );
TEMPER_INVOKE_PARAMETRIC_TEST( test_file_delete, g_test_file_copy_path    );
TEMPER_INVOKE_PARAMETRIC_TEST( test_file_delete, g_test_file_entire_path  );

// folder: delete
TEMPER_INVOKE_PARAMETRIC_TEST( test_folder_delete, g_test_folder_path );


/*
================================================================================================

	Paths

================================================================================================
*/

TEST( test_path_app_path, TEMPER_FLAG_SHOULD_RUN ) {
	String app_path = path_app_path( mem_get_temp_storage() );

	TEMPER_CHECK_TRUE( app_path.count != 0 );
	TEMPER_CHECK_TRUE( app_path.data != NULL );

	TEMPER_CHECK_TRUE( path_is_absolute( app_path.data ) );
#if defined( _WIN32 )
	TEMPER_CHECK_TRUE( string_ends_with( app_path.data, "core-tests.exe" ) );
#elif defined( __linux__ )
	TEMPER_CHECK_TRUE( string_ends_with( app_path.data, "core-tests" ) );
#endif

	mem_reset_temp_storage();
}

TEST( test_path_current_working_directory_matches_absolute_dot, TEMPER_FLAG_SHOULD_RUN ) {
	String cwd = path_get_cwd( mem_get_temp_storage() );
	String absolute_dot = path_absolute_path( mem_get_temp_storage(), "." );

	TEMPER_CHECK_TRUE( string_equals( &cwd, &absolute_dot ) );

	mem_reset_temp_storage();
}

TEST_PARAMETRIC( test_path_current_working_directory_set_then_get, TEMPER_FLAG_SHOULD_RUN, const char *folder ) {
	bool8 set_cwd = false;

	String original_cwd = path_get_cwd( mem_get_temp_storage() );

	String known_path = path_absolute_path( mem_get_temp_storage(), folder );
	set_cwd = path_set_cwd( string_cstr( &known_path ) );
	TEMPER_CHECK_TRUE( set_cwd );

	String cwd = path_get_cwd( mem_get_temp_storage() );
	TEMPER_CHECK_TRUE( string_equals( &known_path, &cwd ) );

	set_cwd = path_set_cwd( string_cstr( &original_cwd ) );
	TEMPER_CHECK_TRUE_M( set_cwd, "Failed to revert test back to the original cwd.  Tests that run after this one may fail.\n" );

	mem_reset_temp_storage();
}

TEMPER_INVOKE_PARAMETRIC_TEST( test_path_current_working_directory_set_then_get, "bin"            );
TEMPER_INVOKE_PARAMETRIC_TEST( test_path_current_working_directory_set_then_get, ".builder"       );
TEMPER_INVOKE_PARAMETRIC_TEST( test_path_current_working_directory_set_then_get, "editor_support" );

TEST_PARAMETRIC( test_path_absolute_path_from_relative, TEMPER_FLAG_SHOULD_RUN, const char *relative_path ) {
	String absolute = path_absolute_path( mem_get_temp_storage(), relative_path );

	TEMPER_CHECK_TRUE( path_is_absolute( absolute.data ) );

	mem_reset_temp_storage();
}

TEMPER_INVOKE_PARAMETRIC_TEST( test_path_absolute_path_from_relative, "."              );
TEMPER_INVOKE_PARAMETRIC_TEST( test_path_absolute_path_from_relative, "bin"            );
TEMPER_INVOKE_PARAMETRIC_TEST( test_path_absolute_path_from_relative, "src"            );
TEMPER_INVOKE_PARAMETRIC_TEST( test_path_absolute_path_from_relative, "include"        );
TEMPER_INVOKE_PARAMETRIC_TEST( test_path_absolute_path_from_relative, "editor_support" );

TEST( test_path_absolute_path_already_absolute, TEMPER_FLAG_SHOULD_RUN ) {
	String cwd = path_get_cwd( mem_get_temp_storage() );
	String result = path_absolute_path( mem_get_temp_storage(), string_cstr( &cwd ) );

	TEMPER_CHECK_TRUE( string_equals( &cwd, &result ) );

	mem_reset_temp_storage();
}

TEST_PARAMETRIC( test_path_remove_file_from_path, TEMPER_FLAG_SHOULD_RUN, const char *path, const char *expected_path_without_file ) {
	LinearAllocator *allocator = linear_allocator_create( 1024 * 1024 );
	defer { linear_allocator_destroy( allocator ); };

	String expected = string_set( expected_path_without_file );

	String path_str = string_set( path );
	String actual_path_without_file = path_remove_file_from_path( &path_str );

	if ( expected_path_without_file == NULL ) {
		TEMPER_CHECK_TRUE( string_equals( actual_path_without_file.data, path ) );
	} else {
		TEMPER_CHECK_TRUE( string_equals( &expected, &actual_path_without_file ) );
	}
}

TEMPER_INVOKE_PARAMETRIC_TEST( test_path_remove_file_from_path, "/usr/bin/cat.jpg",                   "/usr/bin"              );
TEMPER_INVOKE_PARAMETRIC_TEST( test_path_remove_file_from_path, "./some/path/program.d/settings.cfg", "./some/path/program.d" );
TEMPER_INVOKE_PARAMETRIC_TEST( test_path_remove_file_from_path, "./script.sh",                        "."                     );
TEMPER_INVOKE_PARAMETRIC_TEST( test_path_remove_file_from_path, "game.exe",                           "game.exe"              );
TEMPER_INVOKE_PARAMETRIC_TEST( test_path_remove_file_from_path, "file",                               "file"                  );

TEMPER_TEST_PARAMETRIC( test_path_remove_path_from_file, TEMPER_FLAG_SHOULD_RUN, const char *path, const char *expected_file_without_path ) {
	String expected = string_set( expected_file_without_path );

	String actual_file_without_path = string_set( path );
	actual_file_without_path = path_remove_path_from_file( &actual_file_without_path );

	TEMPER_CHECK_TRUE( string_equals( &expected, &actual_file_without_path ) );
}

TEMPER_INVOKE_PARAMETRIC_TEST( test_path_remove_path_from_file, "/usr/bin/cat.jpg",                   "cat.jpg"      );
TEMPER_INVOKE_PARAMETRIC_TEST( test_path_remove_path_from_file, "./some/path/program.d/settings.cfg", "settings.cfg" );
TEMPER_INVOKE_PARAMETRIC_TEST( test_path_remove_path_from_file, "./script.sh",                        "script.sh"    );
TEMPER_INVOKE_PARAMETRIC_TEST( test_path_remove_path_from_file, "game.exe",                           "game.exe"     );
TEMPER_INVOKE_PARAMETRIC_TEST( test_path_remove_path_from_file, "file",                               "file"         );

TEST_PARAMETRIC( test_path_remove_file_extension, TEMPER_FLAG_SHOULD_RUN, const char *file_with_extension, const char *expected_file_without_extension ) {
	String expected = string_set( expected_file_without_extension );

	String actual_file_without_extension = string_set( file_with_extension );
	actual_file_without_extension = path_remove_file_extension( &actual_file_without_extension );

	TEMPER_CHECK_TRUE( string_equals( &expected, &actual_file_without_extension ) );

	mem_reset_temp_storage();
}

TEMPER_INVOKE_PARAMETRIC_TEST( test_path_remove_file_extension, "test.txt",                    "test"                    );
TEMPER_INVOKE_PARAMETRIC_TEST( test_path_remove_file_extension, "cat.jpg",                     "cat"                     );
TEMPER_INVOKE_PARAMETRIC_TEST( test_path_remove_file_extension, "dog.fw.png",                  "dog.fw"                  );
TEMPER_INVOKE_PARAMETRIC_TEST( test_path_remove_file_extension, "doom/src/r_bsp.c",            "doom/src/r_bsp"          );
TEMPER_INVOKE_PARAMETRIC_TEST( test_path_remove_file_extension, "home/program.d/settings.cfg", "home/program.d/settings" );
TEMPER_INVOKE_PARAMETRIC_TEST( test_path_remove_file_extension, "docs/diary",                  "docs/diary"              );

TEST_PARAMETRIC( test_path_is_absolute, TEMPER_FLAG_SHOULD_RUN, const char *path, const bool8 should_be_absolute ) {
	TEMPER_CHECK_TRUE( path_is_absolute( path ) == should_be_absolute );
}

#if defined( __linux__ )
TEMPER_INVOKE_PARAMETRIC_TEST( test_path_is_absolute, "/usr/bin/",  true  );
TEMPER_INVOKE_PARAMETRIC_TEST( test_path_is_absolute, "./usr/bin/", false );
TEMPER_INVOKE_PARAMETRIC_TEST( test_path_is_absolute, "usr/bin/",   false );
TEMPER_INVOKE_PARAMETRIC_TEST( test_path_is_absolute, "\\usr/bin/", false );
TEMPER_INVOKE_PARAMETRIC_TEST( test_path_is_absolute, ".usr/bin/",  false );
#elif defined( _WIN32 )
TEMPER_INVOKE_PARAMETRIC_TEST( test_path_is_absolute, "C:/Program Files (x86)/Steam/steamapps/common", true  );
TEMPER_INVOKE_PARAMETRIC_TEST( test_path_is_absolute, "C/Program Files (x86)/Steam/steamapps/common",  false );
TEMPER_INVOKE_PARAMETRIC_TEST( test_path_is_absolute, ":/Program Files (x86)/Steam/steamapps/common",  false );
TEMPER_INVOKE_PARAMETRIC_TEST( test_path_is_absolute, "./Program Files (x86)/Steam/steamapps/common",  false );
TEMPER_INVOKE_PARAMETRIC_TEST( test_path_is_absolute, "/Program Files (x86)/Steam/steamapps/common",   false );
#endif

TEST_PARAMETRIC( test_path_fix_slashes, TEMPER_FLAG_SHOULD_RUN, const char *path ) {
	LinearAllocator *allocator = linear_allocator_create( 1024 * 1024 );
	defer { linear_allocator_destroy( allocator ); };

	String path_copy = string_set( path );

#if defined( __linux__ )
	char wrong_slash = '\\';
#elif defined( _WIN32 )
	char wrong_slash = '/';
#endif

	String expected_fixed_path = string_replace( allocator, &path_copy, wrong_slash, PATH_SEPARATOR );

	String actual_fixed_path = path_fix_slashes( allocator, &path_copy );

	TEMPER_CHECK_TRUE( string_equals( &expected_fixed_path, &actual_fixed_path ) );
}

TEMPER_INVOKE_PARAMETRIC_TEST( test_path_fix_slashes, "C:/Users/dan\\Documents\\" );
TEMPER_INVOKE_PARAMETRIC_TEST( test_path_fix_slashes, "C:/Users/dan/\\Documents" );
TEMPER_INVOKE_PARAMETRIC_TEST( test_path_fix_slashes, "/usr/bin/program" );
TEMPER_INVOKE_PARAMETRIC_TEST( test_path_fix_slashes, "cat.jpg" );
TEMPER_INVOKE_PARAMETRIC_TEST( test_path_fix_slashes, "./" );

TEST_PARAMETRIC( test_path_get_relative_path, TEMPER_FLAG_SHOULD_RUN, const char *from, const char *to, const char *expected_relative_path ) {
	LinearAllocator *allocator = linear_allocator_create( 1024 * 1024 );
	defer { linear_allocator_destroy( allocator ); };

	String expected = string_set( expected_relative_path );
	String actual = path_relative_path_to( allocator, from, to );

	// use path_fix_slashes here because Windows can return backslashes in the resultant path (like it should)
	// but its just easier to specify forward slashes in the test parameters
	// so convert them internally to what the OS expects
	expected = path_fix_slashes( allocator, &expected );
	actual = path_fix_slashes( allocator, &actual );

	TEMPER_CHECK_TRUE_M( string_equals( &expected, &actual ), "Expected relative path of \"%s\", instead got \"%s\".\n", string_cstr( &expected ), string_cstr( &actual ) );

	mem_reset_temp_storage();
}

TEMPER_INVOKE_PARAMETRIC_TEST( test_path_get_relative_path, "/home/docs/diary.txt",             "/home/images/cat.jpg",           "../images/cat.jpg" );
TEMPER_INVOKE_PARAMETRIC_TEST( test_path_get_relative_path, "/home/docs/",                      "/home/images/",                  "../images/" );
TEMPER_INVOKE_PARAMETRIC_TEST( test_path_get_relative_path, "/home/docs",                       "/home/images",                   "../images" );
TEMPER_INVOKE_PARAMETRIC_TEST( test_path_get_relative_path, "docs",                             "images",                         "../images" );
TEMPER_INVOKE_PARAMETRIC_TEST( test_path_get_relative_path, "C:/Users/Dan/Documents/diary.txt", "C:/Users/Tom/Pictures/cats.jpg", "../../Tom/Pictures/cats.jpg" );
TEMPER_INVOKE_PARAMETRIC_TEST( test_path_get_relative_path, "C:/Users/Dan/Documents",           "C:/Users/Tom/Pictures",          "../../Tom/Pictures" );
TEMPER_INVOKE_PARAMETRIC_TEST( test_path_get_relative_path, "C:/Users/Dan/Documents",           "C:/Users/Dan",                   ".." );
TEMPER_INVOKE_PARAMETRIC_TEST( test_path_get_relative_path, "C:/Users/Dan",                     "C:/Users/Dan/Documents",         "Documents" );

TEST( test_path_join, TEMPER_FLAG_SHOULD_RUN ) {
	// single component - no separator added
	{
		String result = path_join( mem_get_temp_storage(), "foo" );
		TEMPER_CHECK_TRUE( string_equals( result.data, "foo" ) );
	}

	// two components
	{
#ifdef _WIN32
		const char *expected = "usr\\bin";
#else
		const char *expected = "usr/bin";
#endif
		String result = path_join( mem_get_temp_storage(), "usr", "bin" );
		TEMPER_CHECK_TRUE( string_equals( result.data, expected ) );
	}

	// three components
	{
#ifdef _WIN32
		const char *expected = "home\\dan\\docs";
#else
		const char *expected = "home/dan/docs";
#endif
		String result = path_join( mem_get_temp_storage(), "home", "dan", "docs" );
		TEMPER_CHECK_TRUE( string_equals( result.data, expected ) );
	}

	// four components
	{
#ifdef _WIN32
		const char *expected = "C:\\Users\\your_mother\\videos";
#else
		const char *expected = "C:/Users/your_mother/videos";
#endif
		String result = path_join( mem_get_temp_storage(), "C:", "Users", "your_mother", "videos" );
		TEMPER_CHECK_TRUE( string_equals( result.data, expected ) );
	}

	mem_reset_temp_storage();
}


/*
================================================================================================

	String Builder

================================================================================================
*/

TEST( string_builder, TEMPER_FLAG_SHOULD_RUN ) {
	StringBuilder builder = string_builder_create( mem_get_temp_storage() );

	TEMPER_CHECK_TRUE( builder.head == NULL );
	TEMPER_CHECK_TRUE( builder.tail == NULL );

	string_builder_appendf( &builder, "this " );
	string_builder_appendf( &builder, "is" );
	string_builder_appendf( &builder, " only " );
	string_builder_appendf( &builder, "a " );
	string_builder_appendf( &builder, "test" );

	const char *actualString = string_builder_to_string( &builder );

	TEMPER_CHECK_TRUE( string_equals( actualString, "this is only a test" ) );

	mem_reset_temp_storage();
}

TEST( string_builder_appendf_string, TEMPER_FLAG_SHOULD_RUN ) {
	StringBuilder builder = string_builder_create( mem_get_temp_storage() );

	string_builder_appendf( &builder, "Hello, %S!", string_set( "World" ) );
	string_builder_appendf( &builder, " From %S.", string_set( "Core" ) );

	const char *result = string_builder_to_string( &builder );

	TEMPER_CHECK_TRUE( string_equals( result, "Hello, World! From Core." ) );

	mem_reset_temp_storage();
}


/*
================================================================================================

	Random

================================================================================================
*/

// TODO: 23/12/2025: need tests that actually make sure the min and max values are actually hit when generating random numbers
// how do we test that?

TEST_PARAMETRIC( random_float32_within_range, TEMPER_FLAG_SHOULD_RUN, const float32 low, const float32 high ) {
	For ( u32, i, 0, 1000000 ) {
		float32 random_float = random_float32( low, high );

		TEMPER_CHECK_TRUE( random_float >= low );
		TEMPER_CHECK_TRUE( random_float <= high );
	}
}

TEMPER_INVOKE_PARAMETRIC_TEST( random_float32_within_range, 0.0f, 1.0f );
TEMPER_INVOKE_PARAMETRIC_TEST( random_float32_within_range, 1.0f, 2.0f );
TEMPER_INVOKE_PARAMETRIC_TEST( random_float32_within_range, 1.0f, 3.0f );
TEMPER_INVOKE_PARAMETRIC_TEST( random_float32_within_range, 2.0f, 3.0f );
TEMPER_INVOKE_PARAMETRIC_TEST( random_float32_within_range, 1.0f, 10.0f );
TEMPER_INVOKE_PARAMETRIC_TEST( random_float32_within_range, 100.0f, 150.0f );
TEMPER_INVOKE_PARAMETRIC_TEST( random_float32_within_range, 100.0f, 250.0f );
TEMPER_INVOKE_PARAMETRIC_TEST( random_float32_within_range, 1000.0f, 2000.0f );
TEMPER_INVOKE_PARAMETRIC_TEST( random_float32_within_range, 0.0f, FLOAT32_MAX );


/*
================================================================================================

	Library

================================================================================================
*/

TEST( load_library_get_symbol_and_unload_again, TEMPER_FLAG_SHOULD_RUN ) {
	// load
#if defined( _WIN32 )
	const char *filename = "test_dll.dll";
#else
	const char *filename = "test_dll.so";
#endif

	Library library = library_load( filename );
	TEMPER_CHECK_TRUE( library.ptr != NULL );

	// get symbol
	{
		typedef const char * ( *GetDLLNameFunc )( void );

		GetDLLNameFunc get_dll_name_func = cast( GetDLLNameFunc, library_get_symbol( library, "get_dll_name" ) );
		TEMPER_CHECK_TRUE( get_dll_name_func );

		const char *dll_name = get_dll_name_func();

		TEMPER_CHECK_TRUE( string_equals( dll_name, TEST_DLL_NAME ) );
	}

	// unload
	TEMPER_CHECK_TRUE( library_unload( &library ) );
	TEMPER_CHECK_TRUE( library.ptr == NULL );
}


/*
================================================================================================

	Threads

================================================================================================
*/

static void sleep_ms( const u32 ms ) {
#ifdef _WIN32
	Sleep( ms );
#else
	struct timespec ts = {};
	ts.tv_sec = ms / 1000;
	ts.tv_nsec = ( ms % 1000 ) * 1000000;
	nanosleep( &ts, NULL );
#endif
}

static s32 thread_basic_test_func( void* data ) {
	unused( data );

	return 69;
}

TEST( test_thread_create_and_destroy, TEMPER_FLAG_SHOULD_RUN ) {
	Thread thread = thread_create( thread_basic_test_func, NULL );

	TEMPER_CHECK_TRUE( thread.ptr != NULL );

	s32 exit_code = thread_wait( &thread );

	s32 expected_exit_code = 69;
	TEMPER_CHECK_TRUE_M( exit_code == expected_exit_code, "Thread was expected to return with exit code %d actually returned with %d.\n", expected_exit_code, exit_code );

	thread_destroy( &thread );

	TEMPER_CHECK_TRUE( thread.ptr == NULL );
}

struct ThreadJob {
	Atomic32	completed;
	const char	*msg;
};

struct ThreadPool {
	Thread		threads[4];
	Semaphore	sema;
	Atomic32	jobs_read_pos;
	Atomic32	jobs_write_pos;
	Atomic32	num_completed_jobs;
	ThreadJob	jobs[1024];
	bool8		running;
};

struct ThreadContext {
	u32			logical_thread_index;
	ThreadPool	*thread_pool;
};

static void add_thread_job( ThreadPool *thread_pool, const char *msg ) {
	u32 job_index = atomic_increment( &thread_pool->jobs_write_pos ) - 1;
	ThreadJob *job = &thread_pool->jobs[job_index];
	job->completed = { 0 };
	job->msg = msg;

	semaphore_signal( &thread_pool->sema );
}

static s32 test_thread_pool_job_func( void *data ) {
	ThreadContext *context = cast( ThreadContext *, data );

	ThreadPool *thread_pool = context->thread_pool;

	while ( thread_pool->running ) {
		u32 cached_read_pos = thread_pool->jobs_read_pos.value;
		if ( cached_read_pos < thread_pool->jobs_write_pos.value ) {
			u32 read_pos = atomic_compare_exchange( &thread_pool->jobs_read_pos, cached_read_pos, cached_read_pos + 1 );

			if ( read_pos == cached_read_pos ) {
				ThreadJob* job = &thread_pool->jobs[read_pos];

				sleep_ms( 100 );

				const char *threadnumstr = NULL;
				switch ( context->logical_thread_index ) {
					case 0: threadnumstr = "0"; break;
					case 1: threadnumstr = "1"; break;
					case 2: threadnumstr = "2"; break;
					case 3: threadnumstr = "3"; break;
				}

				char msg[1024] = {};
				strcat( msg, "Thread " );
				strcat( msg, threadnumstr );
				strcat( msg, ": " );
				strcat( msg, job->msg );

// #ifdef _WIN32
// 				OutputDebugString( msg );
// #else
				puts( msg );
// #endif

				atomic_increment( &job->completed );
				atomic_increment( &thread_pool->num_completed_jobs );
			}
		} else {
			semaphore_wait( &thread_pool->sema );
		}
	}

	// deliberately set to this value
	// when we shut the scheduler down we want to test that the threads actually shut down like they were supposed to
	return 42;
}

TEST( test_thread_pool, TEMPER_FLAG_SHOULD_RUN ) {
	ThreadPool thread_pool = {};
	thread_pool.running = true;
	semaphore_create( &thread_pool.sema );

	ThreadContext contexts[4] = {};

	memset( thread_pool.threads, 0, count_of( thread_pool.threads ) * sizeof( Thread ) );

	For ( u32, thread_index, 0, count_of( thread_pool.threads ) ) {
		contexts[thread_index].thread_pool = &thread_pool;
		contexts[thread_index].logical_thread_index = thread_index;

		thread_pool.threads[thread_index] = thread_create( test_thread_pool_job_func, &contexts[thread_index] );

		TEMPER_CHECK_TRUE( thread_pool.threads[thread_index].ptr != NULL );
	}

	add_thread_job( &thread_pool, "Job A0\n" );
	add_thread_job( &thread_pool, "Job A1\n" );
	add_thread_job( &thread_pool, "Job A2\n" );
	add_thread_job( &thread_pool, "Job A3\n" );
	add_thread_job( &thread_pool, "Job A4\n" );
	add_thread_job( &thread_pool, "Job A5\n" );
	add_thread_job( &thread_pool, "Job A6\n" );
	add_thread_job( &thread_pool, "Job A7\n" );

	sleep_ms( 1000 );

	TEMPER_CHECK_TRUE_AM( thread_pool.num_completed_jobs.value == 8, "num_completed_jobs was actually %d", thread_pool.num_completed_jobs.value );
	TEMPER_CHECK_TRUE_A( thread_pool.jobs_read_pos.value == thread_pool.jobs_write_pos.value );

	For ( u32, job_index, 0, thread_pool.num_completed_jobs.value ) {
		TEMPER_CHECK_TRUE( thread_pool.jobs[job_index].completed.value == 1 );
	}

	add_thread_job( &thread_pool, "Job B0\n" );
	add_thread_job( &thread_pool, "Job B1\n" );
	add_thread_job( &thread_pool, "Job B2\n" );
	add_thread_job( &thread_pool, "Job B3\n" );
	add_thread_job( &thread_pool, "Job B4\n" );
	add_thread_job( &thread_pool, "Job B5\n" );
	add_thread_job( &thread_pool, "Job B6\n" );
	add_thread_job( &thread_pool, "Job B7\n" );

	sleep_ms( 1000 );

	TEMPER_CHECK_TRUE_AM( thread_pool.num_completed_jobs.value == 16, "num_completed_jobs was actually %d", thread_pool.num_completed_jobs.value );
	TEMPER_CHECK_TRUE_A( thread_pool.jobs_read_pos.value == thread_pool.jobs_write_pos.value );

	For ( u32, job_index, 0, thread_pool.num_completed_jobs.value ) {
		TEMPER_CHECK_TRUE( thread_pool.jobs[job_index].completed.value == 1 );
	}

	// all threads could be sleeping because the work is done
	// so allow them to shutdown, THEN wake them up
	thread_pool.running = false;

	// each thread checks the semaphore, so calling this once will only wake up one thread
	// so we need to call this once per thread
	For ( u32, thread_index, 0, count_of( thread_pool.threads ) ) {
		semaphore_signal( &thread_pool.sema );
	}

	For ( u32, thread_index, 0, count_of( thread_pool.threads ) ) {
		s32 exit_code = thread_wait( &thread_pool.threads[thread_index] );

		TEMPER_CHECK_TRUE( exit_code == 42 );

		thread_destroy( &thread_pool.threads[thread_index] );

		TEMPER_CHECK_TRUE( thread_pool.threads[thread_index].ptr == NULL );
	}

	semaphore_destroy( &thread_pool.sema );
	TEMPER_CHECK_TRUE( thread_pool.sema.ptr == NULL );
}

struct TempStorageThreadContext {
	u32				num_ints_to_alloc;
	LinearAllocator	*storage_ptr;
	u64				tell_before;
	u64				tell_after;
	Semaphore		*ready_sem;
	Semaphore		*go_sem;
};

static s32 thread_temp_storage_func( void *data ) {
	TempStorageThreadContext *context = cast( TempStorageThreadContext *, data );

	context->storage_ptr = mem_get_temp_storage();
	context->tell_before = mem_temp_tell();

	For ( u32, i, 0, context->num_ints_to_alloc ) {
		mem_temp_alloc( sizeof( u32 ) );
	}

	context->tell_after = mem_temp_tell();

	// tell main weve captured our state, then wait before exiting
	// so main can compare pointers while our temp storage is still alive
	// once the thread exits, the OS can reuse the freed allocator address
	semaphore_signal( context->ready_sem );
	semaphore_wait( context->go_sem );

	return 0;
}

TEST( test_temp_storage_is_thread_local, TEMPER_FLAG_SHOULD_RUN ) {
	LinearAllocator *main_storage = mem_get_temp_storage();

	Semaphore ready_sems[4] = {};
	Semaphore go_sems[4] = {};
	TempStorageThreadContext contexts[4] = {};
	Thread threads[4] = {};

	For ( u32, thread_index, 0, count_of( threads ) ) {
		semaphore_create( &ready_sems[thread_index] );
		semaphore_create( &go_sems[thread_index] );
		contexts[thread_index].num_ints_to_alloc = thread_index + 1;
		contexts[thread_index].ready_sem = &ready_sems[thread_index];
		contexts[thread_index].go_sem = &go_sems[thread_index];
		threads[thread_index] = thread_create( thread_temp_storage_func, &contexts[thread_index] );
	}

	// wait until all threads have captured their storage pointer
	For ( u32, thread_index, 0, count_of( threads ) ) {
		semaphore_wait( &ready_sems[thread_index] );
	}

	// compare while all threads are still alive so their allocators cant be freed and reused
	For ( u32, thread_index, 0, count_of( threads ) ) {
		TempStorageThreadContext *ctx = &contexts[thread_index];

		TEMPER_CHECK_TRUE( ctx->storage_ptr != NULL );
		TEMPER_CHECK_TRUE( ctx->storage_ptr != main_storage );
		TEMPER_CHECK_TRUE( ctx->tell_after > ctx->tell_before );

		For ( u32, other_thread_index, 0, count_of( threads ) ) {
			if ( other_thread_index != thread_index ) {
				TEMPER_CHECK_TRUE( ctx->storage_ptr != contexts[other_thread_index].storage_ptr );
			}
		}
	}

	For ( u32, thread_index, 0, count_of( threads ) ) {
		semaphore_signal( &go_sems[thread_index] );
	}

	For ( u32, thread_index, 0, count_of( threads ) ) {
		thread_wait( &threads[thread_index] );
		thread_destroy( &threads[thread_index] );
		semaphore_destroy( &ready_sems[thread_index] );
		semaphore_destroy( &go_sems[thread_index] );
	}
}


/*
================================================================================================

	Process

================================================================================================
*/

static const char *get_test_exe_path( void ) {
	String app_dir = path_app_path( mem_get_temp_storage() );
	app_dir = path_remove_file_from_path( &app_dir );

	String result = path_join( mem_get_temp_storage(), string_cstr( &app_dir ), TEST_EXE_FILENAME );

	return result.data;
}

TEST( test_process_sync_exit_code_zero, TEMPER_FLAG_SHOULD_RUN ) {
	LinearAllocator *allocator = linear_allocator_create( 1024 * 1024 );
	defer { linear_allocator_destroy( allocator ); };

	Array<const char *> args = {};
	args.init( allocator );
	args.add( get_test_exe_path() );

	Process *process = process_create( allocator, &args, NULL, 0 );
	TEMPER_CHECK_TRUE_A( process != NULL );

	s32 exit_code = process_join( process );
	TEMPER_CHECK_TRUE( exit_code == 0 );

	TEMPER_CHECK_TRUE( process_destroy( process ) );
	process = NULL;

	mem_reset_temp_storage();
}

TEST( test_process_sync_exit_code_nonzero, TEMPER_FLAG_SHOULD_RUN ) {
	LinearAllocator *allocator = linear_allocator_create( 1024 * 1024 );
	defer { linear_allocator_destroy( allocator ); };

	Array<const char *> args = {};
	args.init( allocator );
	args.add( get_test_exe_path() );
	args.add( "--exit" );
	args.add( "42" );

	Process *process = process_create( allocator, &args );
	TEMPER_CHECK_TRUE_A( process != NULL );

	s32 exit_code = process_join( process );
	TEMPER_CHECK_TRUE_M( exit_code == 42, "Exit code was actually %d\n", exit_code );

	TEMPER_CHECK_TRUE( process_destroy( process ) );
	process = NULL;

	mem_reset_temp_storage();
}

TEST( test_process_async_exit_code, TEMPER_FLAG_SHOULD_RUN ) {
	LinearAllocator *allocator = linear_allocator_create( 1024 * 1024 );
	defer { linear_allocator_destroy( allocator ); };

	Array<const char *> args = {};
	args.init( allocator );
	args.add( get_test_exe_path() );
	args.add( "--exit" );
	args.add( "0" );

	Process *process = process_create( allocator, &args, NULL, PROCESS_FLAG_ASYNC );
	TEMPER_CHECK_TRUE_A( process != NULL );

	s32 exit_code = process_join( process );
	TEMPER_CHECK_TRUE( exit_code == 0 );

	TEMPER_CHECK_TRUE( process_destroy( process ) );
	process = NULL;

	mem_reset_temp_storage();
}

TEST( test_process_read_stdout, TEMPER_FLAG_SHOULD_RUN ) {
	LinearAllocator *allocator = linear_allocator_create( 1024 * 1024 );
	defer { linear_allocator_destroy( allocator ); };

	Array<const char *> args = {};
	args.init( allocator );
	args.add( get_test_exe_path() );
	args.add( "--stdout" );
	args.add( "hello" );

	Process *process = process_create( allocator, &args, NULL, PROCESS_FLAG_ASYNC );
	TEMPER_CHECK_TRUE_A( process != NULL );

	char buffer[1024] = {};
	u32 bytes_read = process_read_stdout( process, buffer, cast( u32, count_of( buffer ) - 1 ) );

	TEMPER_CHECK_TRUE( bytes_read > 0 );
	TEMPER_CHECK_TRUE( string_starts_with( buffer, "hello" ) );

	process_join( process );

	TEMPER_CHECK_TRUE( process_destroy( process ) );
	process = NULL;

	mem_reset_temp_storage();
}

TEST( test_process_read_stderr, TEMPER_FLAG_SHOULD_RUN ) {
	LinearAllocator *allocator = linear_allocator_create( 1024 * 1024 );
	defer { linear_allocator_destroy( allocator ); };

	Array<const char *> args = {};
	args.init( allocator );
	args.add( get_test_exe_path() );
	args.add( "--stderr" );
	args.add( "hello_stderr" );

	Process *process = process_create( allocator, &args, NULL, PROCESS_FLAG_ASYNC );
	TEMPER_CHECK_TRUE_A( process != NULL );

	char buffer[1024] = {};
	u32 bytes_read = process_read_stderr( process, buffer, cast( u32, count_of( buffer ) - 1 ) );

	TEMPER_CHECK_TRUE( bytes_read > 0 );
	TEMPER_CHECK_TRUE( string_starts_with( buffer, "hello_stderr" ) );

	process_join( process );

	TEMPER_CHECK_TRUE( process_destroy( process ) );
	process = NULL;

	mem_reset_temp_storage();
}

TEST( test_process_combine_stdout_and_stderr, TEMPER_FLAG_SHOULD_RUN ) {
	LinearAllocator *allocator = linear_allocator_create( 1024 * 1024 );
	defer { linear_allocator_destroy( allocator ); };

	Array<const char *> args = {};
	args.init( allocator );
	args.add( get_test_exe_path() );
	args.add( "--stderr" );
	args.add( "stderr_output" );

	Process *process = process_create( allocator, &args, NULL, cast( ProcessFlags, PROCESS_FLAG_ASYNC | PROCESS_FLAG_COMBINE_STDOUT_AND_STDERR ) );
	TEMPER_CHECK_TRUE_A( process != NULL );

	char buffer[256] = {};
	u32 bytes_read = process_read_stdout( process, buffer, cast( u32, sizeof( buffer ) - 1 ) );

	TEMPER_CHECK_TRUE( bytes_read > 0 );
	TEMPER_CHECK_TRUE( string_starts_with( buffer, "stderr_output" ) );

	process_join( process );

	TEMPER_CHECK_TRUE( process_destroy( process ) );
	process = NULL;

	mem_reset_temp_storage();
}


/*
================================================================================================

	Timer

	Leave these tests last because they each need you to wait for a fixed span of time.

================================================================================================
*/

TEST( test_timer_seconds, TEMPER_FLAG_SHOULD_RUN ) {
	float64 start = time_seconds();

	sleep_ms( 1000 );

	float64 end = time_seconds();

	TEMPER_CHECK_TRUE( ( end - start ) >= 1.0 );
	TEMPER_CHECK_TRUE( ( end - start ) <  1.1 );
}

TEST( test_timer_milliseconds, TEMPER_FLAG_SHOULD_RUN ) {
	float64 start = time_ms();

	sleep_ms( 1000 );

	float64 end = time_ms();

	TEMPER_CHECK_TRUE( ( end - start ) >= 1000.0 );
	TEMPER_CHECK_TRUE( ( end - start ) <  1100.0 );
}

TEST( test_timer_microseconds, TEMPER_FLAG_SHOULD_RUN ) {
	float64 start = time_us();

	sleep_ms( 1000 );

	float64 end = time_us();

	TEMPER_CHECK_TRUE( ( end - start ) >= 1000000.0 );
	TEMPER_CHECK_TRUE( ( end - start ) <  1100000.0 );
}

TEST( test_timer_nanoseconds, TEMPER_FLAG_SHOULD_RUN ) {
	float64 start = time_ns();

	sleep_ms( 1000 );

	float64 end = time_ns();

	TEMPER_CHECK_TRUE( ( end - start ) >= 1000000000.0 );
	TEMPER_CHECK_TRUE( ( end - start ) <  1100000000.0 );
}


//================================================================

#define TEST_PADDING "................................................................"

static void on_before_test( const temperTestInfo_t* test_info ) {
	const int pad_length_max = cast( int, strlen( TEST_PADDING ) );

	const int dot_length = pad_length_max - cast( int, strlen( test_info->testNameStr ) );
	assert( dot_length );

	print( "%s %*.*s ", test_info->testNameStr, dot_length, dot_length, TEST_PADDING );
}

static void on_after_test( const temperTestInfo_t* test_info ) {
	unused( test_info );

	if ( test_info->testingFlag == TEMPER_FLAG_SHOULD_SKIP ) {
		TemperSetTextColorInternal( TEMPERDEV_COLOR_YELLOW );
		print( "SKIPPED\n" );
		TemperSetTextColorInternal( TEMPERDEV_COLOR_DEFAULT );
	} else {
		if ( g_temperTestContext.currentTestErrorCount == 0 ) {
			TemperSetTextColorInternal( TEMPERDEV_COLOR_GREEN );
			print( "OK" );
			TemperSetTextColorInternal( TEMPERDEV_COLOR_DEFAULT );
		} else {
			TemperSetTextColorInternal( TEMPERDEV_COLOR_RED );
			print( "FAILED" );
			TemperSetTextColorInternal( TEMPERDEV_COLOR_DEFAULT );
		}

		print( " (%f %s)\n", test_info->testTimeTaken, TemperGetTimeUnitStringInternal( g_temperTestContext.timeUnit ) );
	}
}

int main( int argc, char **argv ) {
	mem_init_temp_storage( MEM_KILOBYTES( 64 ) );
	defer { mem_shutdown_temp_storage(); };

	g_temperTestContext.callbacks.OnBeforeTest = on_before_test;
	g_temperTestContext.callbacks.OnAfterTest = on_after_test;

	TEMPER_RUN( argc, argv );

	int exit_code = TEMPER_GET_EXIT_CODE();

#ifdef _DEBUG
	if ( exit_code != 0 ) {
		debug_break();
	}
#endif

	return exit_code;
}
