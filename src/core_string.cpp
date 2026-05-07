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

#include <core_string.h>
#include <debug.h>
#include <typecast.inl>
#include <temp_storage.h>
#include <defer.h>
#include <core_helpers.h>
#include <linear_allocator.h>

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

static String string_vprintf( LinearAllocator *allocator, const char *fmt, va_list args ) {
	assert( allocator );
	assert( fmt );

	String result;

	va_list args_copy;
	va_copy( args_copy, args );

	result.count = cast( u64, vsnprintf( NULL, 0, fmt, args ) );

	result.data = cast( char *, linear_allocator_alloc( allocator, ( result.count + 1 ) * sizeof( char ) ) );
	vsnprintf( result.data, result.count + 1, fmt, args_copy );
	result.data[result.count] = 0;

	va_end( args_copy );

	return result;
}

/*
================================================================================================

	String

================================================================================================
*/

String string_set( LinearAllocator *allocator, const char *str ) {
	return string_set( allocator, str, strlen( str ) );
}

String string_set( LinearAllocator *allocator, const char *str, const u64 length ) {
	String result = {
		.data	= cast( char *, linear_allocator_alloc( allocator, ( length + 1 ) * sizeof( char ) ) ),
		.count	= length,
	};

	memcpy( result.data, str, ( length + 1 ) * sizeof( char ) );
	result.data[length] = 0;

	return result;
}

String string_printf( LinearAllocator *allocator, const char *fmt, ... ) {
	va_list args;
	va_start( args, fmt );
	String result = string_vprintf( allocator, fmt, args );
	va_end( args );

	return result;
}

String string_copy( LinearAllocator *allocator, String *src ) {
	return string_set( allocator, src->data, src->count );
}

bool8 string_equals( const char *lhs, const char *rhs ) {
	assert( lhs );
	assert( rhs );

	u64 lhs_len = strlen( lhs );
	return lhs_len == strlen( rhs ) && strncmp( lhs, rhs, lhs_len ) == 0;
}

bool8 string_starts_with( const char *str, const char *prefix ) {
	assert( str );
	assert( prefix );

	return strncmp( str, prefix, strlen( prefix ) ) == 0;
}

bool8 string_ends_with( const char *str, const char end ) {
	assert( str );

	return str[strlen( str ) - 1] == end;
}

bool8 string_ends_with( const char *str, const char *suffix ) {
	assert( str );
	assert( suffix );

	u64 suffix_length = strlen( suffix );
	return strncmp( str + strlen( str ) - suffix_length, suffix, suffix_length ) == 0;
}

bool8 string_contains( const char *str, const char *substring ) {
	assert( str );
	assert( substring );

	return strstr( str, substring ) != NULL;
}

void string_replace( String *str, const char old_char, const char new_char ) {
	For ( u32, char_index, 0, str->count ) {
		if ( str->data[char_index] == old_char ) {
			str->data[char_index] = new_char;
		}
	}
}

String temp_printf( const char *fmt, ... ) {
	va_list args;
	va_start( args, fmt );
	String result = string_vprintf( g_temp_storage, fmt, args );
	va_end( args );

	return result;
}


// DM!!! just use string_set( g_temp_storage ) ?

char *temp_c_string( const char *from ) {
	return temp_c_string( from, strlen( from ) );
}

char *temp_c_string( const char *from, const u64 num_chars ) {
	char *result = cast( char *, mem_temp_alloc( ( num_chars + 1 ) * sizeof( char ) ) );
	strncpy( result, from, num_chars * sizeof( char ) );
	result[num_chars] = 0;

	return result;
}
