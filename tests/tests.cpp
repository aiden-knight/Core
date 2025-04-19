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

#ifdef CORE_SUC
#include "../src/core.suc.cpp"
#else
#include "../include/allocation_context.h"
#include "../include/allocator_malloc.h"
#include "../include/allocator_linear.h"
#include "../include/array.inl"
#include "../include/cmd_line_args.h"
#include "../include/core_process.h"
#include "../include/core_string.h"
#include "../include/core_types.h"
#include "../include/date_and_time.h"
#include "../include/debug.h"
#include "../include/defer.h"
#include "../include/file.h"
#include "../include/hash.h"
#include "../include/hashmap.h"
#include "../include/library.h"
#include "../include/memory_units.h"
#include "../include/paths.h"
#include "../include/random.h"
#include "../include/ring.inl"
#include "../include/string_builder.h"
#include "../include/string_helpers.h"
#include "../include/temp_storage.h"
#include "../include/timer.h"
#include "../include/typecast.inl"
#endif

#define TEMPER_IMPLEMENTATION
#define TEMPERDEV_ASSERT assert
#define NOMINMAX
#include "temper/temper.h"

#include <float.h>	// FLT_MAX


/*
================================================================================================

	IntAndFloat Types

================================================================================================
*/

//static bool8 doubleeq( const float64 lhs, const float64 rhs ) {
//	return fabs( lhs - rhs ) < 1e-5;
//}

TEMPER_TEST( number_types_sizes, TEMPER_FLAG_SHOULD_RUN ) {
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

TEMPER_TEST( number_types_ranges, TEMPER_FLAG_SHOULD_RUN ) {
	TEMPER_CHECK_TRUE( S8_MIN  == ( -127 - 1 ) );
	TEMPER_CHECK_TRUE( S16_MIN == ( -32767 - 1 ) );
	TEMPER_CHECK_TRUE( S32_MIN == ( -2147483647 - 1 ) );
	TEMPER_CHECK_TRUE( S64_MIN == ( -9223372036854775807 - 1 ) );

	TEMPER_CHECK_TRUE( S8_MAX  == 127 );
	TEMPER_CHECK_TRUE( S16_MAX == 32767 );
	TEMPER_CHECK_TRUE( S32_MAX == 2147483647 );
	TEMPER_CHECK_TRUE( S64_MAX == 9223372036854775807 );

	// TODO(DM): FLOAT32_MIN, FLOAT32_MAX, FLOAT64_MIN, FLOAT64_MAX
}


/*
================================================================================================

	Logging

================================================================================================
*/

TEMPER_TEST( print_functions, TEMPER_FLAG_SHOULD_SKIP ) {
	printf( "Test printf() call.  Please ignore.\n" );
	warning( "Test warning() call.  Please ignore.\n" );
	error( "Test error() call.  Please ignore.\n" );

	printf( "\n" );
}


/*
================================================================================================

	String

================================================================================================
*/

TEMPER_TEST( string_defaults, TEMPER_FLAG_SHOULD_RUN ) {
	String s;

	TEMPER_CHECK_TRUE( s.data == NULL );
	TEMPER_CHECK_TRUE( s.count == 0 );
	TEMPER_CHECK_TRUE( s.allocator == NULL );
}

TEMPER_TEST_PARAMETRIC( string_assignment, TEMPER_FLAG_SHOULD_RUN, const char* str ) {
	size_t str_length = strlen( str );

	String actual_string = str;

	TEMPER_CHECK_TRUE( actual_string.count == str_length );
	TEMPER_CHECK_TRUE( strcmp( cast( char*, actual_string.data ), str ) == 0 );
	TEMPER_CHECK_TRUE( memcmp( actual_string.data, str, actual_string.count ) == 0 );
	TEMPER_CHECK_TRUE( actual_string.data[actual_string.count] == 0 );
}

TEMPER_TEST_PARAMETRIC( string_copy_other_string, TEMPER_FLAG_SHOULD_RUN, const String& a ) {
	String b = a;

	TEMPER_CHECK_TRUE( a.count == b.count );
	TEMPER_CHECK_TRUE( strcmp( a.data, b.data ) == 0 );
	TEMPER_CHECK_TRUE( memcmp( a.data, b.data, a.count ) == 0 );
	TEMPER_CHECK_TRUE( b.data[b.count] == 0 );
}

TEMPER_INVOKE_PARAMETRIC_TEST( string_assignment, "a" );
TEMPER_INVOKE_PARAMETRIC_TEST( string_assignment, "Test" );
TEMPER_INVOKE_PARAMETRIC_TEST( string_assignment, "This is" );
TEMPER_INVOKE_PARAMETRIC_TEST( string_assignment, "Wake up, Neo" );
TEMPER_INVOKE_PARAMETRIC_TEST( string_assignment, "Follow the White Rabbit" );
TEMPER_INVOKE_PARAMETRIC_TEST( string_assignment, "Knock knock, Neo" );
TEMPER_INVOKE_PARAMETRIC_TEST( string_assignment, "Free your mind, Neo" );
TEMPER_INVOKE_PARAMETRIC_TEST( string_assignment, "I need a longer string but idk what to type so I'm just gonna keep typing the thoughts that pop into my mind cat dog yeah no whatever" );

TEMPER_INVOKE_PARAMETRIC_TEST( string_copy_other_string, "a" );
TEMPER_INVOKE_PARAMETRIC_TEST( string_copy_other_string, "Test" );
TEMPER_INVOKE_PARAMETRIC_TEST( string_copy_other_string, "This is" );
TEMPER_INVOKE_PARAMETRIC_TEST( string_copy_other_string, "Wake up, Neo" );
TEMPER_INVOKE_PARAMETRIC_TEST( string_copy_other_string, "Follow the White Rabbit" );
TEMPER_INVOKE_PARAMETRIC_TEST( string_copy_other_string, "Knock knock, Neo" );
TEMPER_INVOKE_PARAMETRIC_TEST( string_copy_other_string, "Free your mind, Neo" );
TEMPER_INVOKE_PARAMETRIC_TEST( string_copy_other_string, "I need a longer string but idk what to type so I'm just gonna keep typing the thoughts that pop into my mind cat dog yeah no whatever" );

static void test_string_printf_charptr_s32( const char* name, const s32 age ) {
	const char* fmt = "%s %d";

	char expected_string[1024] = {};
	int expected_string_length = sprintf( expected_string, fmt, name, age );

	String actual_string;
	string_printf( &actual_string, fmt, name, age );

	TEMPER_CHECK_TRUE( string_equals( expected_string, (char*) actual_string.data ) );
	TEMPER_CHECK_TRUE( cast( u64, expected_string_length ) == actual_string.count );
}

TEMPER_TEST( string_printf, TEMPER_FLAG_SHOULD_RUN ) {
	test_string_printf_charptr_s32( "Dan",   29 );
	test_string_printf_charptr_s32( "Tom",   34 );
	test_string_printf_charptr_s32( "Zack",  30 );
	test_string_printf_charptr_s32( "Ben",   30 );
	test_string_printf_charptr_s32( "Adam",  32 );
	test_string_printf_charptr_s32( "Niall", 31 );

	// TODO(DM): add more tests covering more use cases
}


/*
================================================================================================

	String Helpers

================================================================================================
*/

TEMPER_TEST( string_helpers_string_equals, TEMPER_FLAG_SHOULD_RUN ) {
	const char* string_a = "This is a string";
	const char* string_b = "This is a string";

	const char* string_c = "tHiS iS a StRiNg";
	const char* string_d = "hoolaba)";

	TEMPER_CHECK_TRUE(  string_equals( string_a, string_b ) );
	TEMPER_CHECK_TRUE( !string_equals( string_a, string_c ) );
	TEMPER_CHECK_TRUE( !string_equals( string_a, string_d ) );
	TEMPER_CHECK_TRUE( !string_equals( string_b, string_c ) );
	TEMPER_CHECK_TRUE( !string_equals( string_b, string_d ) );
	TEMPER_CHECK_TRUE( !string_equals( string_c, string_d ) );
}

TEMPER_TEST_PARAMETRIC( string_helpers_string_starts_with, TEMPER_FLAG_SHOULD_RUN, const char* string, const char* prefix, const bool8 should_start_with ) {
	TEMPER_CHECK_TRUE( string_starts_with( string, prefix ) == should_start_with );
}

static const char* g_test_string_leaves = "There are 69,105 leaves on the pile.";
static const char* g_test_string_matrix = "I'm going to be as forthcoming as I can be, Mr Anderson.";

TEMPER_INVOKE_PARAMETRIC_TEST( string_helpers_string_starts_with, g_test_string_leaves, "T",          true  );
TEMPER_INVOKE_PARAMETRIC_TEST( string_helpers_string_starts_with, g_test_string_leaves, "Th",         true  );
TEMPER_INVOKE_PARAMETRIC_TEST( string_helpers_string_starts_with, g_test_string_leaves, "There",      true  );
TEMPER_INVOKE_PARAMETRIC_TEST( string_helpers_string_starts_with, g_test_string_leaves, "There ",     true  );
TEMPER_INVOKE_PARAMETRIC_TEST( string_helpers_string_starts_with, g_test_string_leaves, "There are",  true  );
TEMPER_INVOKE_PARAMETRIC_TEST( string_helpers_string_starts_with, g_test_string_leaves, "There are ", true  );
TEMPER_INVOKE_PARAMETRIC_TEST( string_helpers_string_starts_with, g_test_string_leaves, "are ",       false );
TEMPER_INVOKE_PARAMETRIC_TEST( string_helpers_string_starts_with, g_test_string_leaves, " are ",      false );
TEMPER_INVOKE_PARAMETRIC_TEST( string_helpers_string_starts_with, g_test_string_leaves, "69,105",     false );

TEMPER_TEST_PARAMETRIC( string_helpers_string_ends_with, TEMPER_FLAG_SHOULD_RUN, const char* string, const char* prefix, const bool8 should_end_with ) {
	TEMPER_CHECK_TRUE( string_ends_with( string, prefix ) == should_end_with );
}

TEMPER_INVOKE_PARAMETRIC_TEST( string_helpers_string_ends_with, g_test_string_matrix, ", Mr Anderson.", true  );
TEMPER_INVOKE_PARAMETRIC_TEST( string_helpers_string_ends_with, g_test_string_matrix, "Anderson.",      true  );
TEMPER_INVOKE_PARAMETRIC_TEST( string_helpers_string_ends_with, g_test_string_matrix, "son.",           true  );
TEMPER_INVOKE_PARAMETRIC_TEST( string_helpers_string_ends_with, g_test_string_matrix, ".",              true  );
TEMPER_INVOKE_PARAMETRIC_TEST( string_helpers_string_ends_with, g_test_string_matrix, "Anderson",       false );
TEMPER_INVOKE_PARAMETRIC_TEST( string_helpers_string_ends_with, g_test_string_matrix, "son",            false );
TEMPER_INVOKE_PARAMETRIC_TEST( string_helpers_string_ends_with, g_test_string_matrix, "forthcoming",    false );

TEMPER_TEST_PARAMETRIC( string_helpers_string_contains, TEMPER_FLAG_SHOULD_RUN, const char* string, const char* substring, const bool8 should_contain ) {
	TEMPER_CHECK_TRUE( string_contains( string, substring ) == should_contain );
}

TEMPER_INVOKE_PARAMETRIC_TEST( string_helpers_string_contains, g_test_string_matrix, "I'm",            true  );
TEMPER_INVOKE_PARAMETRIC_TEST( string_helpers_string_contains, g_test_string_matrix, "I'm ",           true  );
TEMPER_INVOKE_PARAMETRIC_TEST( string_helpers_string_contains, g_test_string_matrix, "to be as",       true  );
TEMPER_INVOKE_PARAMETRIC_TEST( string_helpers_string_contains, g_test_string_matrix, "to be as ",      true  );
TEMPER_INVOKE_PARAMETRIC_TEST( string_helpers_string_contains, g_test_string_matrix, " to be as",      true  );
TEMPER_INVOKE_PARAMETRIC_TEST( string_helpers_string_contains, g_test_string_matrix, " to be as ",     true  );
TEMPER_INVOKE_PARAMETRIC_TEST( string_helpers_string_contains, g_test_string_matrix, "forthcoming",    true  );
TEMPER_INVOKE_PARAMETRIC_TEST( string_helpers_string_contains, g_test_string_matrix, " as I can be",   true  );
TEMPER_INVOKE_PARAMETRIC_TEST( string_helpers_string_contains, g_test_string_matrix, " ",              true  );
TEMPER_INVOKE_PARAMETRIC_TEST( string_helpers_string_contains, g_test_string_matrix, ", Mr Anderson.", true  );
TEMPER_INVOKE_PARAMETRIC_TEST( string_helpers_string_contains, g_test_string_matrix, "Anderson.",      true  );
TEMPER_INVOKE_PARAMETRIC_TEST( string_helpers_string_contains, g_test_string_matrix, "son.",           true  );
TEMPER_INVOKE_PARAMETRIC_TEST( string_helpers_string_contains, g_test_string_matrix, "son",            true  );
TEMPER_INVOKE_PARAMETRIC_TEST( string_helpers_string_contains, g_test_string_matrix, ".",              true  );
TEMPER_INVOKE_PARAMETRIC_TEST( string_helpers_string_contains, g_test_string_matrix, "White rabbit",   false );
TEMPER_INVOKE_PARAMETRIC_TEST( string_helpers_string_contains, g_test_string_matrix, "Im",             false );
TEMPER_INVOKE_PARAMETRIC_TEST( string_helpers_string_contains, g_test_string_matrix, "frothcoming",    false );
TEMPER_INVOKE_PARAMETRIC_TEST( string_helpers_string_contains, g_test_string_matrix, "Adnerson",       false );
TEMPER_INVOKE_PARAMETRIC_TEST( string_helpers_string_contains, g_test_string_matrix, ")",              false );
TEMPER_INVOKE_PARAMETRIC_TEST( string_helpers_string_contains, g_test_string_matrix, "asdf",           false );
TEMPER_INVOKE_PARAMETRIC_TEST( string_helpers_string_contains, g_test_string_matrix, "123456",         false );

TEMPER_TEST_PARAMETRIC( string_helpers_string_substring, TEMPER_FLAG_SHOULD_RUN, const char* string, const u64 start, const u64 count, const char* expected_substring ) {
	char* actual_susbtring = cast( char*, mem_temp_alloc( count * sizeof( char ) ) );

	string_substring( string, start, count, actual_susbtring );

	TEMPER_CHECK_TRUE( string_equals( expected_substring, actual_susbtring ) );
}

TEMPER_INVOKE_PARAMETRIC_TEST( string_helpers_string_substring, "As you can see we've had our eye on you for some time now, Mr Anderson.", 0,  15, "As you can see" );
TEMPER_INVOKE_PARAMETRIC_TEST( string_helpers_string_substring, "As you can see we've had our eye on you for some time now, Mr Anderson.", 15, 25, "we've had our eye on you" );
TEMPER_INVOKE_PARAMETRIC_TEST( string_helpers_string_substring, "As you can see we've had our eye on you for some time now, Mr Anderson.", 40, 18, "for some time now" );
TEMPER_INVOKE_PARAMETRIC_TEST( string_helpers_string_substring, "As you can see we've had our eye on you for some time now, Mr Anderson.", 57, 15, ", Mr Anderson." );
TEMPER_INVOKE_PARAMETRIC_TEST( string_helpers_string_substring, "As you can see we've had our eye on you for some time now, Mr Anderson.", 59, 12, "Mr Anderson" );


/*
================================================================================================

	AllocatorLinearData

================================================================================================
*/

struct TestStruct {
	u32			x;
	const char*	string;
};

TEMPER_TEST_PARAMETRIC( test_linear_allocator_create, TEMPER_FLAG_SHOULD_RUN, LinearAllocator** allocator, const u64 size_bytes ) {
	TEMPER_CHECK_TRUE( allocator );
	TEMPER_CHECK_TRUE( !*allocator );

	*allocator = linear_allocator_create( size_bytes );

	TEMPER_CHECK_TRUE( ( *allocator )->offset == 0 );
	TEMPER_CHECK_TRUE( ( *allocator )->size_bytes == size_bytes );
}

TEMPER_TEST_PARAMETRIC( test_linear_allocator_allocate, TEMPER_FLAG_SHOULD_RUN, LinearAllocator* allocator, const u32 x, const char* string ) {
	TEMPER_CHECK_TRUE( allocator );

	u64 old_offset = allocator->offset;
	u64 old_size = allocator->size_bytes;

	TestStruct* test_struct = cast( TestStruct*, linear_allocator_alloc( allocator, sizeof( TestStruct ), MEMORY_ALIGNMENT_EIGHT ) );
	test_struct->x = x;
	test_struct->string = string;

	TEMPER_CHECK_TRUE( test_struct );
	TEMPER_CHECK_TRUE( test_struct->x == x );
	TEMPER_CHECK_TRUE( string_equals( test_struct->string, string ) );

	u64 padding = padding_up( old_offset, MEMORY_ALIGNMENT_EIGHT );
	u64 expected_new_offset = old_offset + padding + sizeof( TestStruct );

	TEMPER_CHECK_TRUE( allocator->offset == expected_new_offset );
	TEMPER_CHECK_TRUE( allocator->offset != old_offset );
	TEMPER_CHECK_TRUE( allocator->size_bytes == old_size );
}

TEMPER_TEST_PARAMETRIC( test_linear_allocator_reset, TEMPER_FLAG_SHOULD_RUN, LinearAllocator* allocator ) {
	TEMPER_CHECK_TRUE( allocator );

	u64 old_offset 	= allocator->offset;
	u64 old_size 	= allocator->size_bytes;

	linear_allocator_reset( allocator );

	TEMPER_CHECK_TRUE( allocator->offset == 0 );
	TEMPER_CHECK_TRUE( allocator->offset != old_offset );
	TEMPER_CHECK_TRUE( allocator->size_bytes == old_size );
}

TEMPER_TEST_PARAMETRIC( test_linear_allocator_destroy, TEMPER_FLAG_SHOULD_RUN, LinearAllocator* allocator ) {
	TEMPER_CHECK_TRUE( allocator );

	linear_allocator_destroy( allocator );
	allocator = NULL;

	TEMPER_CHECK_TRUE( !allocator );
}

static LinearAllocator* g_allocator = NULL;

TEMPER_INVOKE_PARAMETRIC_TEST( test_linear_allocator_create, &g_allocator, MEM_KILOBYTES( 1 ) );

TEMPER_INVOKE_PARAMETRIC_TEST( test_linear_allocator_allocate, g_allocator, 100, "this is a test string" );
TEMPER_INVOKE_PARAMETRIC_TEST( test_linear_allocator_allocate, g_allocator, 200, "this is another test string" );
TEMPER_INVOKE_PARAMETRIC_TEST( test_linear_allocator_allocate, g_allocator, 300, "and another one" );
TEMPER_INVOKE_PARAMETRIC_TEST( test_linear_allocator_allocate, g_allocator, 400, "one last time" );
TEMPER_INVOKE_PARAMETRIC_TEST( test_linear_allocator_allocate, g_allocator, 500, "ok im done now" );

TEMPER_INVOKE_PARAMETRIC_TEST( test_linear_allocator_reset, g_allocator );

TEMPER_INVOKE_PARAMETRIC_TEST( test_linear_allocator_allocate, g_allocator, 555, "should be starting from the beginning again now" );
TEMPER_INVOKE_PARAMETRIC_TEST( test_linear_allocator_allocate, g_allocator, 666, "your message here" );
TEMPER_INVOKE_PARAMETRIC_TEST( test_linear_allocator_allocate, g_allocator, 777, "dont look directly into the bugs" );
TEMPER_INVOKE_PARAMETRIC_TEST( test_linear_allocator_allocate, g_allocator, 888, "rzcore is great" );
TEMPER_INVOKE_PARAMETRIC_TEST( test_linear_allocator_allocate, g_allocator, 888, "rzcore? great throwback Dan" );
TEMPER_INVOKE_PARAMETRIC_TEST( test_linear_allocator_allocate, g_allocator, 999, "Ross Kemp" );

TEMPER_INVOKE_PARAMETRIC_TEST( test_linear_allocator_destroy, g_allocator );



/*
================================================================================================

	Array

================================================================================================
*/

static Array<s32> g_test_array;
static Array<s32> g_test_array_copy;

TEMPER_TEST_PARAMETRIC( array_get_element_at_index, TEMPER_FLAG_SHOULD_RUN, Array<s32>* array, const u64 index, const s32 expected_element ) {
	TEMPER_CHECK_TRUE( (*array)[index] == expected_element );
}

TEMPER_TEST_PARAMETRIC( test_array_zero, TEMPER_FLAG_SHOULD_RUN, Array<s32>* out_array ) {
	out_array->zero();

	TEMPER_CHECK_TRUE( out_array->data == NULL );
	TEMPER_CHECK_TRUE( out_array->count == 0 );
	TEMPER_CHECK_TRUE( out_array->alloced == 0 );
}

TEMPER_TEST_PARAMETRIC( test_array_add, TEMPER_FLAG_SHOULD_RUN, Array<s32>* array, s32 element ) {
	u64 old_count = array->count;
	u64 old_alloced = array->alloced;

	array->add( element );

	TEMPER_CHECK_TRUE( array->data[array->count - 1] == element );

	TEMPER_CHECK_TRUE( array->data != NULL );
	TEMPER_CHECK_TRUE( array->count == old_count + 1 );

	if ( old_count == old_alloced ) {
		u64 new_alloced = next_multiple_of_4_up( old_alloced + 1 );

		TEMPER_CHECK_TRUE( array->alloced == new_alloced );
	} else {
		TEMPER_CHECK_TRUE( array->alloced == old_alloced );
	}
}

TEMPER_TEST_PARAMETRIC( test_array_reset, TEMPER_FLAG_SHOULD_RUN, Array<s32>* array ) {
	array->reset();

	TEMPER_CHECK_TRUE( array->count == 0 );
}

TEMPER_TEST_PARAMETRIC( test_array_resize, TEMPER_FLAG_SHOULD_RUN, Array<s32>* array, const u64 new_count ) {
	u64 old_count = array->count;
	u64 old_alloced = array->alloced;

	array->resize( new_count );

	u64 new_alloced = 0;
	if ( new_count >= old_alloced ) {
		new_alloced = next_multiple_of_4_up( new_count );
	} else {
		new_alloced = old_alloced;
	}

	TEMPER_CHECK_TRUE( array->count == new_count );
	TEMPER_CHECK_TRUE( array->alloced == new_alloced );
}

TEMPER_TEST_PARAMETRIC( test_array_reserve, TEMPER_FLAG_SHOULD_RUN, Array<s32>* array, const u64 new_count ) {
	u64 old_count = array->count;
	u64 old_alloced = array->alloced;

	array->reserve( new_count );

	TEMPER_CHECK_TRUE( array->count == old_count );

	if ( new_count > old_alloced ) {
		TEMPER_CHECK_TRUE( array->alloced == next_multiple_of_4_up( new_count ) );
	} else {
		TEMPER_CHECK_TRUE( array->alloced == old_alloced );
	}
}

TEMPER_TEST_PARAMETRIC( test_array_copy, TEMPER_FLAG_SHOULD_RUN, Array<s32>* original_array, Array<s32>* new_array ) {
	new_array->copy( original_array );

	TEMPER_CHECK_TRUE( new_array->count == original_array->count );
	TEMPER_CHECK_TRUE( new_array->alloced == next_multiple_of_4_up( new_array->count ) );

	For ( i, new_array->count ) {
		TEMPER_CHECK_TRUE_M( ( *new_array )[i] == ( *original_array )[i], "new_array[%llu] != original_array[%llu] when it should!", i, i );
	}
}

TEMPER_INVOKE_PARAMETRIC_TEST( test_array_zero, &g_test_array );

TEMPER_INVOKE_PARAMETRIC_TEST( test_array_add, &g_test_array, 123 );
TEMPER_INVOKE_PARAMETRIC_TEST( test_array_add, &g_test_array, 69 );
TEMPER_INVOKE_PARAMETRIC_TEST( test_array_add, &g_test_array, 420 );
TEMPER_INVOKE_PARAMETRIC_TEST( test_array_add, &g_test_array, 666 );

TEMPER_INVOKE_PARAMETRIC_TEST( array_get_element_at_index, &g_test_array, 0, 123 );
TEMPER_INVOKE_PARAMETRIC_TEST( array_get_element_at_index, &g_test_array, 1, 69 );
TEMPER_INVOKE_PARAMETRIC_TEST( array_get_element_at_index, &g_test_array, 2, 420 );
TEMPER_INVOKE_PARAMETRIC_TEST( array_get_element_at_index, &g_test_array, 3, 666 );

TEMPER_INVOKE_PARAMETRIC_TEST( test_array_reset, &g_test_array );

TEMPER_INVOKE_PARAMETRIC_TEST( test_array_resize, &g_test_array, 5 );
TEMPER_INVOKE_PARAMETRIC_TEST( test_array_resize, &g_test_array, 10 );
TEMPER_INVOKE_PARAMETRIC_TEST( test_array_resize, &g_test_array, 6 );

TEMPER_INVOKE_PARAMETRIC_TEST( test_array_reserve, &g_test_array, 20 );
TEMPER_INVOKE_PARAMETRIC_TEST( test_array_reserve, &g_test_array, 10 );

TEMPER_INVOKE_PARAMETRIC_TEST( test_array_copy, &g_test_array, &g_test_array_copy );


/*
================================================================================================

	hash32 and hash64

================================================================================================
*/

TEMPER_TEST_PARAMETRIC( hash_string_equals_hash32, TEMPER_FLAG_SHOULD_RUN, const char* string, const u32 seed, const u32 expected_hash ) {
	const u64 string_length = strlen( string );

	TEMPER_CHECK_TRUE( hash32( string, string_length, seed ) == expected_hash );
}

TEMPER_TEST_PARAMETRIC( hash_string_equals_hash64, TEMPER_FLAG_SHOULD_RUN, const char* string, const u64 seed, const u64 expected_hash ) {
	const u64 string_length = strlen( string );

	TEMPER_CHECK_TRUE( hash64( string, string_length, seed ) == expected_hash );
}

TEMPER_INVOKE_PARAMETRIC_TEST( hash_string_equals_hash32, "test_hash_string_value", 0, 163121569U );
TEMPER_INVOKE_PARAMETRIC_TEST( hash_string_equals_hash64, "test_hash_string_value", 0, 17747826225912071899ULL );


/*
================================================================================================

	Hashmap

================================================================================================
*/

TEMPER_TEST( test_hashmap_combine, TEMPER_FLAG_SHOULD_RUN ) {
	u32 lo_part = 0xFAFAFAFA;
	u32 hi_part = 0xAFAFAFAF;

	u64 combined = hashmap_internal_combine( hi_part, lo_part );
	TEMPER_CHECK_TRUE_A( combined == 0xFAFAFAFAAFAFAFAF );
	TEMPER_CHECK_TRUE_A( hashmap_internal_get_hi_part( combined ) == hi_part );
	TEMPER_CHECK_TRUE_A( hashmap_internal_get_lo_part( combined ) == lo_part );
}

TEMPER_TEST_PARAMETRIC( test_hashmap_create, TEMPER_FLAG_SHOULD_RUN, Hashmap** hashmap, const u32 count ) {
	TEMPER_CHECK_TRUE( hashmap );
	TEMPER_CHECK_TRUE( !*hashmap );

	TEMPER_CHECK_TRUE( count );

	*hashmap = hashmap_create( count );

	TEMPER_CHECK_TRUE( ( *hashmap )->capacity == count );

	For ( i, ( *hashmap )->capacity ) {
		HashmapBucket& bucket = ( *hashmap )->buckets[i];
		TEMPER_CHECK_TRUE_A( hashmap_internal_combine(bucket.key_hi, bucket.key_lo) == HASHMAP_UNUSED_BUCKET );
		TEMPER_CHECK_TRUE_A( bucket.value == HASHMAP_INVALID_VALUE );
	}
}

TEMPER_TEST_PARAMETRIC( test_hashmap_set_and_get_value, TEMPER_FLAG_SHOULD_RUN, Hashmap* hashmap, const char* name, const u32 age ) {
	
	LogVerbosity previous_log_verbosity = get_log_verbosity();
	set_log_verbosity(LOG_VERBOSITY_ERROR); //Hiding warnings from getting values that don't exist as this is intentionally checking this behaviour
	
	TEMPER_CHECK_TRUE( hashmap );

	const u32 name_hash = hash32( name, strlen( name ), 0 );

	TEMPER_CHECK_TRUE( hashmap_get_value( hashmap, name_hash ) == HASHMAP_INVALID_VALUE );

	hashmap_set_value( hashmap, name_hash, age );

	TEMPER_CHECK_TRUE( hashmap_get_value( hashmap, name_hash ) == age );
	TEMPER_CHECK_TRUE( hashmap_get_value( hashmap, name_hash ) != HASHMAP_INVALID_VALUE );

	set_log_verbosity(previous_log_verbosity);
}

TEMPER_TEST_PARAMETRIC( test_hashmap_reset, TEMPER_FLAG_SHOULD_RUN, Hashmap* hashmap ) {
	TEMPER_CHECK_TRUE( hashmap );

	u32 old_count = hashmap->capacity;

	hashmap_reset( hashmap );

	TEMPER_CHECK_TRUE( hashmap->capacity == old_count );

	For ( i, hashmap->capacity ) {
		HashmapBucket& bucket = hashmap->buckets[i];
		TEMPER_CHECK_TRUE_A( hashmap_internal_combine(bucket.key_hi, bucket.key_lo) == HASHMAP_UNUSED_BUCKET );
		TEMPER_CHECK_TRUE_A( bucket.value == HASHMAP_INVALID_VALUE );
	}

	TEMPER_CHECK_TRUE_A(hashmap->usage_count == 0U);
	TEMPER_CHECK_TRUE_A(hashmap->tombstone_count == 0U);
}

TEMPER_TEST_PARAMETRIC( test_hashmap_remove, TEMPER_FLAG_SHOULD_RUN, Hashmap* hashmap ) {
	TEMPER_CHECK_TRUE( hashmap );

	hashmap_reset(hashmap);

	u32 count = hashmap->capacity;
	u32 first_key = count; 		// Bucket index % = 0
	u32 second_key = count * 2; // Bucket index % = 0
	u32 third_key = count + 1;	// Bucket index % = 1;

	u32 first_value = 69;
	u32 second_value = 70;
	u32 third_value = 90;

	hashmap_set_value(hashmap, first_key, first_value); 	// Actual bucket pos 0
	TEMPER_CHECK_TRUE_A(hashmap_internal_combine_at_index(hashmap, 0) == first_key);
	hashmap_set_value(hashmap, second_key, second_value); // Actual bucket pos 1 (wanted 0)
	TEMPER_CHECK_TRUE_A(hashmap_internal_combine_at_index(hashmap, 1) == second_key);
	hashmap_set_value(hashmap, third_key, third_value);	// Actual bucket pos 2 (wanted 1)
	TEMPER_CHECK_TRUE_A(hashmap_internal_combine_at_index(hashmap, 2) == third_key);

	hashmap_remove_key(hashmap, second_key);

	// Check the tombstone from the second value is in place to allow proper gets
	TEMPER_CHECK_TRUE_A(hashmap_get_value(hashmap, third_key) == third_value);

	hashmap_remove_key(hashmap, third_key);
	// Check tombstones were removed
	TEMPER_CHECK_TRUE_A(hashmap_internal_combine_at_index(hashmap, 2) == HASHMAP_UNUSED_BUCKET);
	TEMPER_CHECK_TRUE_A(hashmap_internal_combine_at_index(hashmap, 1) == HASHMAP_UNUSED_BUCKET);

	hashmap_set_value(hashmap, third_key, third_value);

	// Check that add won't probe passed removed tombstone
	TEMPER_CHECK_TRUE_A(hashmap_get_value(hashmap, third_key) == third_value);
	TEMPER_CHECK_TRUE_A(hashmap_internal_combine_at_index(hashmap, 1) == third_key);
}

TEMPER_TEST_PARAMETRIC( test_hashmap_linear_probe_telemetry, TEMPER_FLAG_SHOULD_RUN, u32 number_of_buckets, float utilisation ) {
	Hashmap* hashmap = hashmap_create( number_of_buckets, utilisation, false );
	u64 hash_seed = 0x9E3779B97F4A7C15;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wimplicit-int-float-conversion"
	u32 intended_fill = static_cast<u32>( number_of_buckets * utilisation );
#pragma GCC diagnostic pop

	auto get_hash_at_sequence = [&]( u32 sequence ) -> u64 {
		assert( sequence < hashmap->usage_count );

		u32 running_hash_count = -1U;
		u64 hash = HASHMAP_TOMBSTONE_BUCKET;

		For ( key_index, hashmap->capacity ) {
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

				For ( float_index, LENGTH_OF_RANDOM_FLOATS ) {
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
	Array<u32> linear_probe_length;
	linear_probe_length.reserve( intended_fill );
	For ( i, hashmap->capacity ) {
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
	For ( i, linear_probe_length.count ) {
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

	info(
		"\n===\nPROBE RESULTS for %f pc utilization on %d buckets:\naverage probe length was %f, average of non zero was %f, biggest was %d. Num that were zero: %d\n Tombstone:Used: %d:%d\n",
		utilisation * 100.0f, number_of_buckets, mean, none_zero_mean, biggest, num_zero_probes, hashmap->tombstone_count, hashmap->usage_count
	);

	hashmap_destroy( hashmap );
}

static Hashmap* g_hashmap = NULL;

TEMPER_INVOKE_PARAMETRIC_TEST( test_hashmap_create, &g_hashmap, 10 );

TEMPER_INVOKE_PARAMETRIC_TEST( test_hashmap_set_and_get_value, g_hashmap, "Dan",     27 );
TEMPER_INVOKE_PARAMETRIC_TEST( test_hashmap_set_and_get_value, g_hashmap, "Tom",     27 ); //Don't forget your buddy :) -- never <3
TEMPER_INVOKE_PARAMETRIC_TEST( test_hashmap_set_and_get_value, g_hashmap, "Mum",     53 );
TEMPER_INVOKE_PARAMETRIC_TEST( test_hashmap_set_and_get_value, g_hashmap, "Dad",     62 );
TEMPER_INVOKE_PARAMETRIC_TEST( test_hashmap_set_and_get_value, g_hashmap, "Grandad", 89 );

TEMPER_INVOKE_PARAMETRIC_TEST( test_hashmap_reset, g_hashmap );

TEMPER_INVOKE_PARAMETRIC_TEST( test_hashmap_remove, g_hashmap );

TEMPER_TEST( test_hashmap_growing, TEMPER_FLAG_SHOULD_RUN ) {
	Hashmap* map = hashmap_create( 10, 0.5f, true );
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
TEMPER_INVOKE_PARAMETRIC_TEST( test_hashmap_linear_probe_telemetry, 10000, 0.5f );
TEMPER_INVOKE_PARAMETRIC_TEST( test_hashmap_linear_probe_telemetry, 10000, 0.3f );
TEMPER_INVOKE_PARAMETRIC_TEST( test_hashmap_linear_probe_telemetry, 10000, 0.1f );

/*
================================================================================================

	ring_t

================================================================================================
*/

enum {
	RING_SIZE = 10,
};

static Ring<TestStruct, RING_SIZE>	g_test_ring;

TEMPER_TEST_PARAMETRIC( test_ring_init, TEMPER_FLAG_SHOULD_RUN, Ring<TestStruct, RING_SIZE>* ring ) {
	ring->init();

	TEMPER_CHECK_TRUE( ring->read_pos == 0 );
	TEMPER_CHECK_TRUE( ring->write_pos == 0 );
	TEMPER_CHECK_TRUE( ring->current_elements == 0 );

	TEMPER_CHECK_TRUE( ring->data.count == RING_SIZE + 1 );

	TEMPER_CHECK_TRUE( ring->empty() );
}

TEMPER_TEST_PARAMETRIC( test_ring_reset, TEMPER_FLAG_SHOULD_RUN, Ring<TestStruct, RING_SIZE>* ring ) {
	ring->reset();

	TEMPER_CHECK_TRUE( ring->read_pos == 0 );
	TEMPER_CHECK_TRUE( ring->write_pos == 0 );
	TEMPER_CHECK_TRUE( ring->current_elements == 0 );

	TEMPER_CHECK_TRUE( ring->empty() );
}

TEMPER_TEST_PARAMETRIC( test_ring_push, TEMPER_FLAG_SHOULD_RUN, Ring<TestStruct, RING_SIZE>* ring, const TestStruct test_struct, const u32 expected_write_pos ) {
	u64 old_read_pos = ring->read_pos;
	u64 old_write_pos = ring->write_pos;

	ring->push( test_struct );

	u64 new_read_pos = ring->read_pos;
	u64 new_write_pos = ring->write_pos;

	// ring should never be empty after pushing on to it
	TEMPER_CHECK_TRUE( !ring->empty() );

	TEMPER_CHECK_TRUE( new_read_pos == old_read_pos );
	TEMPER_CHECK_TRUE( new_write_pos != old_write_pos );
	TEMPER_CHECK_TRUE( new_write_pos == expected_write_pos );
}

TEMPER_TEST_PARAMETRIC( test_ring_pop, TEMPER_FLAG_SHOULD_RUN, Ring<TestStruct, RING_SIZE>* ring, const TestStruct expected_test_struct, const u32 expected_read_pos, const bool8 expected_empty ) {
	u64 old_read_pos = ring->read_pos;
	u64 old_write_pos = ring->write_pos;

	TestStruct test_struct = ring->pop();

	TEMPER_CHECK_TRUE( test_struct.x == expected_test_struct.x );
	TEMPER_CHECK_TRUE( string_equals( test_struct.string, expected_test_struct.string ) );

	TEMPER_CHECK_TRUE( ring->empty() == expected_empty );

	u64 new_read_pos = ring->read_pos;
	u64 new_write_pos = ring->write_pos;

	TEMPER_CHECK_TRUE( new_read_pos != old_read_pos );
	TEMPER_CHECK_TRUE( new_write_pos == old_write_pos );
	TEMPER_CHECK_TRUE( new_read_pos == expected_read_pos );
}

TEMPER_TEST_PARAMETRIC( test_ring_empty, TEMPER_FLAG_SHOULD_RUN, Ring<TestStruct, RING_SIZE>* ring, const bool8 expected_empty ) {
	TEMPER_CHECK_TRUE( ring->empty() == expected_empty );
}

TEMPER_TEST_PARAMETRIC( test_ring_full, TEMPER_FLAG_SHOULD_RUN, Ring<TestStruct, RING_SIZE>* ring, const bool8 expected_full ) {
	bool8 is_full = ring->full();
	TEMPER_CHECK_TRUE( is_full == expected_full );
}

TEMPER_INVOKE_PARAMETRIC_TEST( test_ring_init,  &g_test_ring );

TEMPER_INVOKE_PARAMETRIC_TEST( test_ring_empty, &g_test_ring, true );	// should be empty given nothing has been added to it
TEMPER_INVOKE_PARAMETRIC_TEST( test_ring_full,  &g_test_ring, false );	// therefore should not be full

TEMPER_INVOKE_PARAMETRIC_TEST( test_ring_push,  &g_test_ring, { 10, "test string" }, 1 );
TEMPER_INVOKE_PARAMETRIC_TEST( test_ring_push,  &g_test_ring, { 11, "another test string" }, 2 );
TEMPER_INVOKE_PARAMETRIC_TEST( test_ring_push,  &g_test_ring, { 12, "and again" }, 3 );
TEMPER_INVOKE_PARAMETRIC_TEST( test_ring_push,  &g_test_ring, { 13, "and again" }, 4 );
TEMPER_INVOKE_PARAMETRIC_TEST( test_ring_push,  &g_test_ring, { 14, "test string" }, 5 );
TEMPER_INVOKE_PARAMETRIC_TEST( test_ring_push,  &g_test_ring, { 15, "test string" }, 6 );
TEMPER_INVOKE_PARAMETRIC_TEST( test_ring_push,  &g_test_ring, { 16, "test string" }, 7 );
TEMPER_INVOKE_PARAMETRIC_TEST( test_ring_push,  &g_test_ring, { 17, "test string" }, 8 );
TEMPER_INVOKE_PARAMETRIC_TEST( test_ring_push,  &g_test_ring, { 18, "test string" }, 9 );
TEMPER_INVOKE_PARAMETRIC_TEST( test_ring_push,  &g_test_ring, { 19, "test string" }, 10 );
TEMPER_INVOKE_PARAMETRIC_TEST( test_ring_empty, &g_test_ring, false );	// should not be empty after filling it up
TEMPER_INVOKE_PARAMETRIC_TEST( test_ring_full,  &g_test_ring, true );	// and should be full
TEMPER_INVOKE_PARAMETRIC_TEST( test_ring_pop,   &g_test_ring, { 10, "test string" }, 1, false );
TEMPER_INVOKE_PARAMETRIC_TEST( test_ring_pop,   &g_test_ring, { 11, "another test string" }, 2, false );
TEMPER_INVOKE_PARAMETRIC_TEST( test_ring_pop,   &g_test_ring, { 12, "and again" }, 3, false );
TEMPER_INVOKE_PARAMETRIC_TEST( test_ring_pop,   &g_test_ring, { 13, "and again" }, 4, false );
TEMPER_INVOKE_PARAMETRIC_TEST( test_ring_pop,   &g_test_ring, { 14, "test string" }, 5, false );
TEMPER_INVOKE_PARAMETRIC_TEST( test_ring_pop,   &g_test_ring, { 15, "test string" }, 6, false );
TEMPER_INVOKE_PARAMETRIC_TEST( test_ring_pop,   &g_test_ring, { 16, "test string" }, 7, false );
TEMPER_INVOKE_PARAMETRIC_TEST( test_ring_pop,   &g_test_ring, { 17, "test string" }, 8, false );
TEMPER_INVOKE_PARAMETRIC_TEST( test_ring_pop,   &g_test_ring, { 18, "test string" }, 9, false );
TEMPER_INVOKE_PARAMETRIC_TEST( test_ring_pop,   &g_test_ring, { 19, "test string" }, 10, true );
TEMPER_INVOKE_PARAMETRIC_TEST( test_ring_empty, &g_test_ring, true );

TEMPER_INVOKE_PARAMETRIC_TEST( test_ring_push,  &g_test_ring, { 10, "test string" }, 0 );
TEMPER_INVOKE_PARAMETRIC_TEST( test_ring_push,  &g_test_ring, { 11, "another test string" }, 1 );
TEMPER_INVOKE_PARAMETRIC_TEST( test_ring_push,  &g_test_ring, { 12, "and again" }, 2 );
TEMPER_INVOKE_PARAMETRIC_TEST( test_ring_push,  &g_test_ring, { 13, "and again" }, 3 );
TEMPER_INVOKE_PARAMETRIC_TEST( test_ring_push,  &g_test_ring, { 14, "test string" }, 4 );
TEMPER_INVOKE_PARAMETRIC_TEST( test_ring_push,  &g_test_ring, { 15, "test string" }, 5 );
TEMPER_INVOKE_PARAMETRIC_TEST( test_ring_push,  &g_test_ring, { 16, "test string" }, 6 );
TEMPER_INVOKE_PARAMETRIC_TEST( test_ring_push,  &g_test_ring, { 17, "test string" }, 7 );
TEMPER_INVOKE_PARAMETRIC_TEST( test_ring_push,  &g_test_ring, { 18, "test string" }, 8 );
TEMPER_INVOKE_PARAMETRIC_TEST( test_ring_push,  &g_test_ring, { 19, "test string" }, 9 );

TEMPER_INVOKE_PARAMETRIC_TEST( test_ring_empty, &g_test_ring, false );	// should not be empty after filling it up
TEMPER_INVOKE_PARAMETRIC_TEST( test_ring_full,  &g_test_ring, true );	// and should be full

TEMPER_INVOKE_PARAMETRIC_TEST( test_ring_reset, &g_test_ring );

TEMPER_INVOKE_PARAMETRIC_TEST( test_ring_empty, &g_test_ring, true );
TEMPER_INVOKE_PARAMETRIC_TEST( test_ring_full,  &g_test_ring, false );


/*
================================================================================================

	File

================================================================================================
*/

TEMPER_TEST_PARAMETRIC( test_file_open_or_create, TEMPER_FLAG_SHOULD_RUN, File* file, const char* filename ) {
	TEMPER_CHECK_TRUE( filename );
	TEMPER_CHECK_TRUE( file->ptr == INVALID_HANDLE_VALUE );

	*file = file_open_or_create( filename );

	TEMPER_CHECK_TRUE( file->ptr != INVALID_HANDLE_VALUE );
	TEMPER_CHECK_TRUE( file->offset == 0 );
}

TEMPER_TEST_PARAMETRIC( test_file_open, TEMPER_FLAG_SHOULD_RUN, File* file, const char* filename, const bool8 should_exist ) {
	TEMPER_CHECK_TRUE( file );
	TEMPER_CHECK_TRUE( file->ptr == INVALID_HANDLE_VALUE );

	*file = file_open( filename );

	bool8 file_exists = file->ptr != INVALID_HANDLE_VALUE;

	if ( should_exist ) {
		TEMPER_CHECK_TRUE( file->ptr != INVALID_HANDLE_VALUE );
	} else {
		TEMPER_CHECK_TRUE( file->ptr == INVALID_HANDLE_VALUE );
	}

	TEMPER_CHECK_TRUE( file_exists == should_exist );
	TEMPER_CHECK_TRUE( file->offset == 0 );
}

TEMPER_TEST_PARAMETRIC( test_file_close, TEMPER_FLAG_SHOULD_RUN, File* file ) {
	TEMPER_CHECK_TRUE( file );
	TEMPER_CHECK_TRUE( file->ptr );

	bool8 closed = file_close( file );

	TEMPER_CHECK_TRUE( closed );

	TEMPER_CHECK_TRUE( file->ptr == INVALID_HANDLE_VALUE );
}

TEMPER_TEST_PARAMETRIC( test_file_write_entire, TEMPER_FLAG_SHOULD_RUN, const char* filename, const char* data_to_write ) {
	TEMPER_CHECK_TRUE( filename );
	TEMPER_CHECK_TRUE( data_to_write );

	u64 num_bytes_to_write = strlen( data_to_write );

	TEMPER_CHECK_TRUE( num_bytes_to_write );

	bool8 written = file_write_entire( filename, data_to_write, num_bytes_to_write );

	TEMPER_CHECK_TRUE( written == true );
}

TEMPER_TEST_PARAMETRIC( test_file_write, TEMPER_FLAG_SHOULD_RUN, File* file, const char* data_to_write, const u64 offset ) {
	TEMPER_CHECK_TRUE( file && file->ptr );
	TEMPER_CHECK_TRUE( data_to_write );

	u64 num_bytes_to_write = strlen( data_to_write );

	TEMPER_CHECK_TRUE( num_bytes_to_write );

	bool8 written = file_write( file, data_to_write, offset, num_bytes_to_write );

	TEMPER_CHECK_TRUE( written == true );

	// read that data back and check that it was actually written
	{
		char* data_read_from_file = cast( char*, malloc( ( num_bytes_to_write + 1 ) * sizeof( char ) ) );

		bool8 read = file_read( file, offset, num_bytes_to_write, data_read_from_file );
		data_read_from_file[num_bytes_to_write] = 0;

		TEMPER_CHECK_TRUE( read );

		TEMPER_CHECK_TRUE( memcmp( data_read_from_file, data_to_write, num_bytes_to_write * sizeof( char ) ) == 0 );
		TEMPER_CHECK_TRUE( string_equals( data_read_from_file, data_to_write ) );

		free( data_read_from_file );
		data_read_from_file = NULL;
	}
}

TEMPER_TEST_PARAMETRIC( test_file_read_entire, TEMPER_FLAG_SHOULD_RUN, const char* filename, const char* expected_file_data ) {
	TEMPER_CHECK_TRUE( filename );
	TEMPER_CHECK_TRUE( expected_file_data );

	u64 expected_file_data_length = strlen( expected_file_data );

	TEMPER_CHECK_TRUE( expected_file_data_length );

	char* actual_file_data = NULL;

	// test reading file
	{
		u64 bytes_read = 0;
		bool8 read = file_read_entire( filename, &actual_file_data, &bytes_read );

		TEMPER_CHECK_TRUE( read == true );
		TEMPER_CHECK_TRUE( actual_file_data != NULL );
		TEMPER_CHECK_TRUE( bytes_read == expected_file_data_length );

		TEMPER_CHECK_TRUE( string_equals( expected_file_data, actual_file_data ) );
	}

	// test freeing file buffer
	{
		file_free_buffer( &actual_file_data );

		TEMPER_CHECK_TRUE( !actual_file_data );
	}
}

TEMPER_TEST_PARAMETRIC( test_file_get_size, TEMPER_FLAG_SHOULD_RUN, const char* filename, const u64 expected_size ) {
	TEMPER_CHECK_TRUE( filename );
	TEMPER_CHECK_TRUE( expected_size );

	File file = file_open( filename );

	TEMPER_CHECK_TRUE( file.ptr );

	u64 file_size = file_get_size( file );

	TEMPER_CHECK_TRUE( file_size );
	TEMPER_CHECK_TRUE( file_size == expected_size );

	file_close( &file );

	TEMPER_CHECK_TRUE( file.ptr == INVALID_HANDLE_VALUE );
}

TEMPER_TEST_PARAMETRIC( test_file_rename, TEMPER_FLAG_SHOULD_RUN, const char* filename_old, const char* filename_new ) {
	TEMPER_CHECK_TRUE( filename_old );
	TEMPER_CHECK_TRUE( filename_new );

	// read old file contents
	// we will use this after renaming to test file contents is the same between the two
	char* old_file_buffer = NULL;
	u64 old_bytes_to_read = file_read_entire( filename_old, &old_file_buffer );

	TEMPER_CHECK_TRUE( old_bytes_to_read != 0 );

	File old_file = file_open( filename_old );

	TEMPER_CHECK_TRUE( old_file.ptr );

	u64 old_file_size = file_get_size( old_file );

	TEMPER_CHECK_TRUE( old_file_size );

	file_close( &old_file );

	// check that the function returned what we expect
	{
		bool8 renamed = file_rename( filename_old, filename_new );

		TEMPER_CHECK_TRUE( renamed );
	}

	// check the file infos match
	{
		File new_file = file_open( filename_new );

		TEMPER_CHECK_TRUE( new_file.ptr );

		// check file sizes are the same
		u64 new_file_size = file_get_size( new_file );

		TEMPER_CHECK_TRUE( new_file_size );
		TEMPER_CHECK_TRUE( new_file_size == old_file_size );

		file_close( &new_file );
	}

	// check that file contents are the same
	{
		char* new_file_buffer = NULL;
		u64 new_bytes_read = file_read_entire( filename_new, &new_file_buffer );

		TEMPER_CHECK_TRUE( new_bytes_read != 0 );

		TEMPER_CHECK_TRUE( old_bytes_to_read == new_bytes_read );

		TEMPER_CHECK_TRUE( memcmp( old_file_buffer, new_file_buffer, new_bytes_read ) == 0 );
		TEMPER_CHECK_TRUE( string_equals( old_file_buffer, new_file_buffer ) );
	}

	file_free_buffer( &old_file_buffer );
}

TEMPER_TEST_PARAMETRIC( test_file_delete, TEMPER_FLAG_SHOULD_RUN, const char* filename ) {
	TEMPER_CHECK_TRUE( filename );

	File file;

	// first check its there to delete
	file = file_open( filename );
	TEMPER_CHECK_TRUE( file.ptr );

	// delete it
	bool8 deleted = file_delete( filename );
	TEMPER_CHECK_TRUE( deleted );

	// now check it doesnt actually exist anymore
	file = file_open( filename );
	TEMPER_CHECK_TRUE( file.ptr == INVALID_HANDLE_VALUE );
}

TEMPER_TEST_PARAMETRIC( test_folder_exists, TEMPER_FLAG_SHOULD_RUN, const char* path, const bool8 should_exist ) {
	TEMPER_CHECK_TRUE( path );

	bool8 exists = folder_exists( path );

	TEMPER_CHECK_TRUE( exists == should_exist );
}

TEMPER_TEST_PARAMETRIC( test_folder_create, TEMPER_FLAG_SHOULD_RUN, const char* path ) {
	TEMPER_CHECK_TRUE( path );

	bool8 exists = folder_exists( path );
	TEMPER_CHECK_TRUE( !exists );

	bool8 created = folder_create_if_it_doesnt_exist( path );
	TEMPER_CHECK_TRUE( created );
}

TEMPER_TEST_PARAMETRIC( test_folder_delete, TEMPER_FLAG_SHOULD_RUN, const char* path ) {
	TEMPER_CHECK_TRUE( path );

	bool8 exists = false;

	// first check its there to delete
	exists = folder_exists( path );
	TEMPER_CHECK_TRUE( exists );

	// delete it
	bool8 deleted = folder_delete( path );
	TEMPER_CHECK_TRUE( deleted );

	// now check it doesnt actually exist anymore
	exists = folder_exists( path );
	TEMPER_CHECK_TRUE( !exists );
}

#define TEST_FILENAME			"test_file.txt"
#define TEST_FILENAME_RENAMED	"test_file_renamed.txt"

#define TEST_FOLDER_NAME		"test_folder"

#define TEST_LINE_1				"this is an example line in the file\n"
#define TEST_LINE_2				"this is another line in the file\n"
#define TEST_LINE_3				"oh what fun we are having\n"

#define TEST_LINE_1_ALTERED		"this is an altered line in the file\n"

#define TEST_LINE_OVERWRITE		"this file has now been overwritten"

static File						g_test_file = { INVALID_HANDLE_VALUE };

TEMPER_INVOKE_PARAMETRIC_TEST( test_file_open, &g_test_file, TEST_FILENAME, false );

TEMPER_INVOKE_PARAMETRIC_TEST( test_file_open_or_create, &g_test_file, TEST_FILENAME );
TEMPER_INVOKE_PARAMETRIC_TEST( test_file_close, &g_test_file );

TEMPER_INVOKE_PARAMETRIC_TEST( test_file_open, &g_test_file, TEST_FILENAME, true );
TEMPER_INVOKE_PARAMETRIC_TEST( test_file_close, &g_test_file );

TEMPER_INVOKE_PARAMETRIC_TEST( test_file_open, &g_test_file, TEST_FILENAME, true );
TEMPER_INVOKE_PARAMETRIC_TEST( test_file_write, &g_test_file, TEST_LINE_1, 0 );
TEMPER_INVOKE_PARAMETRIC_TEST( test_file_write, &g_test_file, TEST_LINE_2, strlen( TEST_LINE_1 ) );
TEMPER_INVOKE_PARAMETRIC_TEST( test_file_write, &g_test_file, TEST_LINE_3, strlen( TEST_LINE_1 ) + strlen( TEST_LINE_2 ) );
TEMPER_INVOKE_PARAMETRIC_TEST( test_file_close, &g_test_file );

TEMPER_INVOKE_PARAMETRIC_TEST( test_file_read_entire, TEST_FILENAME, TEST_LINE_1 TEST_LINE_2 TEST_LINE_3 );

TEMPER_INVOKE_PARAMETRIC_TEST( test_file_get_size, TEST_FILENAME, strlen( TEST_LINE_1 TEST_LINE_2 TEST_LINE_3 ) );

TEMPER_INVOKE_PARAMETRIC_TEST( test_file_write_entire, TEST_FILENAME, TEST_LINE_OVERWRITE );
TEMPER_INVOKE_PARAMETRIC_TEST( test_file_read_entire, TEST_FILENAME, TEST_LINE_OVERWRITE );

TEMPER_INVOKE_PARAMETRIC_TEST( test_file_get_size, TEST_FILENAME, strlen( TEST_LINE_OVERWRITE ) );

TEMPER_INVOKE_PARAMETRIC_TEST( test_file_write_entire, TEST_FILENAME, TEST_LINE_1 );
TEMPER_INVOKE_PARAMETRIC_TEST( test_file_read_entire, TEST_FILENAME, TEST_LINE_1 );

TEMPER_INVOKE_PARAMETRIC_TEST( test_file_get_size, TEST_FILENAME, strlen( TEST_LINE_1 ) );

TEMPER_INVOKE_PARAMETRIC_TEST( test_file_open, &g_test_file, TEST_FILENAME, true );
TEMPER_INVOKE_PARAMETRIC_TEST( test_file_write, &g_test_file, "altered", strlen( "this is an " ) );
TEMPER_INVOKE_PARAMETRIC_TEST( test_file_close, &g_test_file );

TEMPER_INVOKE_PARAMETRIC_TEST( test_file_read_entire, TEST_FILENAME, TEST_LINE_1_ALTERED );

TEMPER_INVOKE_PARAMETRIC_TEST( test_file_rename, TEST_FILENAME, TEST_FILENAME_RENAMED );
TEMPER_INVOKE_PARAMETRIC_TEST( test_file_rename, TEST_FILENAME_RENAMED, TEST_FILENAME );

TEMPER_INVOKE_PARAMETRIC_TEST( test_file_delete, TEST_FILENAME );

TEMPER_INVOKE_PARAMETRIC_TEST( test_folder_exists, TEST_FOLDER_NAME, false );

TEMPER_INVOKE_PARAMETRIC_TEST( test_folder_create, TEST_FOLDER_NAME );
TEMPER_INVOKE_PARAMETRIC_TEST( test_folder_exists, TEST_FOLDER_NAME, true );
TEMPER_INVOKE_PARAMETRIC_TEST( test_folder_delete, TEST_FOLDER_NAME );


/*
================================================================================================

	Timer

================================================================================================
*/

TEMPER_TEST( test_timer_seconds, TEMPER_FLAG_SHOULD_SKIP ) {
	float64 start = time_seconds();

#ifdef _WIN32
	Sleep( 1000 );
#endif

	float64 end = time_seconds();

	TEMPER_CHECK_TRUE( doubleeq( end - start, 1.0 ) );
}

TEMPER_TEST( test_timer_milliseconds, TEMPER_FLAG_SHOULD_SKIP ) {
	float64 start = time_ms();

#ifdef _WIN32
	Sleep( 1000 );
#endif

	float64 end = time_ms();

	TEMPER_CHECK_TRUE( doubleeq( end - start, 1000.0 ) );
}

TEMPER_TEST( test_timer_microseconds, TEMPER_FLAG_SHOULD_SKIP ) {
	float64 start = time_us();

#ifdef _WIN32
	Sleep( 1000 );
#endif

	float64 end = time_us();

	TEMPER_CHECK_TRUE( doubleeq( end - start, 1000000.0 ) );
}

TEMPER_TEST( test_timer_nanoseconds, TEMPER_FLAG_SHOULD_SKIP ) {
	float64 start = time_ns();

#ifdef _WIN32
	Sleep( 1000 );
#endif

	float64 end = time_ns();

	TEMPER_CHECK_TRUE( doubleeq( end - start, 1000000000.0 ) );
}


/*
================================================================================================

	String Builder

================================================================================================
*/

TEMPER_TEST( string_builder, TEMPER_FLAG_SHOULD_RUN ) {
	StringBuilder builder = {};

	string_builder_reset( &builder );

	TEMPER_CHECK_TRUE( builder.head == NULL );
	TEMPER_CHECK_TRUE( builder.tail == NULL );

	defer( string_builder_destroy( &builder ) );

	string_builder_appendf( &builder, "this " );
	string_builder_appendf( &builder, "is " );
	string_builder_appendf( &builder, "only " );
	string_builder_appendf( &builder, "a " );
	string_builder_appendf( &builder, "test" );

	const char* actualString = string_builder_to_string( &builder );

	TEMPER_CHECK_TRUE( string_equals( actualString, "this is only a test" ) );
}


/*
================================================================================================

	Random

================================================================================================
*/

TEMPER_TEST_PARAMETRIC( random_float_within_range, TEMPER_FLAG_SHOULD_RUN, const float32 low, const float32 high ) {
	For ( i, 1000000 ) {
		float32 random_float = random_float32( low, high );

		TEMPER_CHECK_TRUE( random_float >= low );
		TEMPER_CHECK_TRUE( random_float <= high );
	}
}

TEMPER_INVOKE_PARAMETRIC_TEST( random_float_within_range, 0.0f, 1.0f );
TEMPER_INVOKE_PARAMETRIC_TEST( random_float_within_range, 1.0f, 2.0f );
TEMPER_INVOKE_PARAMETRIC_TEST( random_float_within_range, 1.0f, 3.0f );
TEMPER_INVOKE_PARAMETRIC_TEST( random_float_within_range, 2.0f, 3.0f );
TEMPER_INVOKE_PARAMETRIC_TEST( random_float_within_range, 1.0f, 10.0f );
TEMPER_INVOKE_PARAMETRIC_TEST( random_float_within_range, 100.0f, 150.0f );
TEMPER_INVOKE_PARAMETRIC_TEST( random_float_within_range, 100.0f, 250.0f );
TEMPER_INVOKE_PARAMETRIC_TEST( random_float_within_range, 1000.0f, 2000.0f );
TEMPER_INVOKE_PARAMETRIC_TEST( random_float_within_range, 0.0f, FLT_MAX );


/*
================================================================================================

	Library

================================================================================================
*/

#include "test_dll.h"

typedef const char* ( *GetDLLNameFunc )( void );

static Library g_test_dll;

TEMPER_TEST( test_library_load, TEMPER_FLAG_SHOULD_RUN ) {
	g_test_dll = library_load( "test_dll.dll" );
	TEMPER_CHECK_TRUE( g_test_dll.ptr != NULL );
}

TEMPER_TEST( test_library_get_proc_address, TEMPER_FLAG_SHOULD_RUN ) {
	GetDLLNameFunc fp_get_dll_name = cast( GetDLLNameFunc, library_get_proc_address( g_test_dll, "get_dll_name" ) );
	TEMPER_CHECK_TRUE( fp_get_dll_name != NULL );

	const char* name = fp_get_dll_name();
	TEMPER_CHECK_TRUE( string_equals( name, TEST_DLL_NAME ) );
}

TEMPER_TEST( test_library_unload, TEMPER_FLAG_SHOULD_RUN ) {
	TEMPER_CHECK_TRUE( g_test_dll.ptr != NULL );

	library_unload( &g_test_dll );

	TEMPER_CHECK_TRUE( g_test_dll.ptr == NULL );
}


//================================================================


#define TEST_PADDING "................................................................"

static void on_before_test( const temperTestInfo_t* test_info ) {
	const int pad_length_max = cast( int, strlen( TEST_PADDING ) );

	const int dot_length = pad_length_max - cast( int, strlen( test_info->testNameStr ) );
	assert( dot_length );

	printf( "%s %*.*s ", test_info->testNameStr, dot_length, dot_length, TEST_PADDING );
}

static void on_after_test( const temperTestInfo_t* test_info ) {
	unused( test_info );

	if ( test_info->testingFlag == TEMPER_FLAG_SHOULD_SKIP ) {
		TemperSetTextColorInternal( TEMPERDEV_COLOR_YELLOW );
		printf( "SKIPPED\n" );
		TemperSetTextColorInternal( TEMPERDEV_COLOR_DEFAULT );
	} else {
		if ( g_temperTestContext.currentTestErrorCount == 0 ) {
			TemperSetTextColorInternal( TEMPERDEV_COLOR_GREEN );
			printf( "OK" );
			TemperSetTextColorInternal( TEMPERDEV_COLOR_DEFAULT );
		} else {
			TemperSetTextColorInternal( TEMPERDEV_COLOR_RED );
			printf( "FAILED" );
			TemperSetTextColorInternal( TEMPERDEV_COLOR_DEFAULT );
		}

		printf( " (%f %s)\n", test_info->testTimeTaken, TemperGetTimeUnitStringInternal( g_temperTestContext.timeUnit ) );
	}
}

int main( int argc, char** argv ) {
	// TODO(DM): 19/1/2023: this wants to be in a test
	core_init();
	defer( core_shutdown() );

	g_temperTestContext.callbacks.OnBeforeTest = on_before_test;
	g_temperTestContext.callbacks.OnAfterTest = on_after_test;

	TEMPER_RUN( argc, argv );

	int exitCode = TEMPER_GET_EXIT_CODE();

	return exitCode;
}