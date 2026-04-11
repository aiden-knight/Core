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

static void StrReallocInternal( string_t *outStr, const u64 length ) {
	if ( length > outStr->count ) {
		char *newData = Cast( char *, Mem_Alloc( outStr->allocator, length * sizeof( char ) ) );
		memcpy( newData, outStr->data, outStr->count * sizeof( char ) );
		outStr->data = newData;
	}
}

/*
================================================================================================

	String

================================================================================================
*/

void Str_Init( string_t *outStr, linearAllocator_t *allocator ) {
	outStr->allocator = allocator;
	outStr->data = NULL;
	outStr->count = 0;
}

void Str_Zero( string_t *outStr ) {
	outStr->allocator = NULL;
	outStr->data = NULL;
	outStr->count = 0;
}

void Str_Printf( string_t *outStr, const char *fmt, ... ) {
	assert( outStr );
	assert( fmt );

	va_list args;
	va_start( args, fmt );
	defer { va_end( args ); };

	u64 length = Cast( u64, vsnprintf( NULL, 0, fmt, args ) );

	StrReallocInternal( outStr, length + 1 );
	vsnprintf( outStr->data, length, fmt, args );
	outStr->data[length] = 0;
}

void Str_Copy( string_t *dst, string_t *src ) {
	Str_CopyFromCString( dst, src->data, src->count );
}

void Str_CopyFromCString( string_t *outStr, const char *cStr ) {
	Str_CopyFromCString( outStr, cStr, strlen( cStr ) );
}

void Str_CopyFromCString( string_t *outStr, const char *cStr, const u64 length ) {
	StrReallocInternal( outStr, length + 1 );
	memcpy( outStr->data, cStr, length );
	outStr->data[length] = 0;
	outStr->count = length;
}

bool8 Str_Equals( const char *lhs, const char *rhs ) {
	assert( lhs );
	assert( rhs );

	u64 lhsLen = strlen( lhs );
	return lhsLen == strlen( rhs ) && strncmp( lhs, rhs, lhsLen ) == 0;
}

bool8 Str_StartsWith( const char *str, const char *prefix ) {
	assert( str );
	assert( prefix );

	return strncmp( str, prefix, strlen( prefix ) ) == 0;
}

bool8 Str_EndsWith( const char *str, const char end ) {
	assert( str );

	return str[strlen( str ) - 1] == end;
}

bool8 Str_EndsWith( const char *str, const char *suffix ) {
	assert( str );
	assert( suffix );

	u64 suffixLength = strlen( suffix );
	return strncmp( str + strlen( str ) - suffixLength, suffix, suffixLength ) == 0;
}

bool8 Str_Contains( const char *str, const char *substring ) {
	assert( str );
	assert( substring );

	return strstr( str, substring ) != NULL;
}

const char *Str_Replace( const char *str, const char oldChar, const char newChar ) {
	char* result = Str_TempCString( str );

	For ( u32, i, 0, strlen( str ) ) {
		if ( result[i] == oldChar ) {
			result[i] = newChar;
		}
	}

	return result;
}

const char* Str_TempPrintf( const char* fmt, ... ) {
	assert( fmt );

	va_list args;
	va_start( args, fmt );

	u64 stringLength = Cast( u64, vsnprintf( NULL, 0, fmt, args ) );
	stringLength += 1;	// + 1 for null terminator

	char* outString = Cast( char*, Mem_TempAlloc( stringLength * sizeof( char ) ) );

	vsnprintf( outString, stringLength, fmt, args );
	outString[stringLength - 1] = 0;

	va_end( args );

	return outString;
}

char *Str_TempCString( const char *from ) {
	return Str_TempCString( from, strlen( from ) );
}

char *Str_TempCString( const char *from, const u64 numChars ) {
	char *result = Cast( char *, Mem_TempAlloc( ( numChars + 1 ) * sizeof( char ) ) );
	strncpy( result, from, numChars * sizeof( char ) );
	result[numChars] = 0;

	return result;
}
