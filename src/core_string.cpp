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

#include <string.h>

#if defined( __clang__ )
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wzero-as-null-pointer-constant"
#pragma clang diagnostic ignored "-Wunsafe-buffer-usage"
#pragma clang diagnostic ignored "-Wunsafe-buffer-usage-in-libc-call"
#endif

bool8 string_equals( const char* lhs, const char* rhs ) {
	assert( lhs );
	assert( rhs );

	u64 lhs_len = strlen( lhs );
	return lhs_len == strlen( rhs ) && strncmp( lhs, rhs, lhs_len ) == 0;
}

bool8 string_starts_with( const char* str, const char* prefix ) {
	assert( str );
	assert( prefix );

	return strncmp( str, prefix, strlen( prefix ) ) == 0;
}

bool8 string_ends_with( const char* str, const char end ) {
	assert( str );

	return str[strlen( str ) - 1] == end;
}

bool8 string_ends_with( const char* str, const char* suffix ) {
	assert( str );
	assert( suffix );

	u64 suffix_length = strlen( suffix );
	return strncmp( str + strlen( str ) - suffix_length, suffix, suffix_length ) == 0;
}

bool8 string_contains( const char* str, const char* substring ) {
	assert( str );
	assert( substring );

	return strstr( str, substring ) != NULL;
}

#if defined( __clang__ )
#pragma clang diagnostic pop
#endif
