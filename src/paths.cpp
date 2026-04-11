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

#include <paths.h>

#include <core_helpers.h>
#include <temp_storage.h>
#include <typecast.inl>
#include <core_string.h>
#include <string_builder.h>
#include <defer.h>

#include <stdarg.h>
#include <string.h>

static const char *GetLastSlash( const char *path ) {
	const char *lastSlash = NULL;
	const char *lastBackSlash = strrchr( path, '\\' );
	const char *lastForwardSlash = strrchr( path, '/' );

	if ( !lastBackSlash && !lastForwardSlash ) {
		return NULL;
	}

	if ( Cast( u64, lastBackSlash ) > Cast( u64, lastForwardSlash ) ) {
		lastSlash = lastBackSlash;
	} else {
		lastSlash = lastForwardSlash;
	}

	return lastSlash;
}

/*
================================================================================================

	Paths

================================================================================================
*/

const char *Path_RemoveFileFromPath( const char *path ) {
	const char *lastSlash = GetLastSlash( path );

	if ( !lastSlash ) {
		return NULL;
	}

	u64 pathLength = Cast( u64, lastSlash ) - Cast( u64, path );

	return Str_TempCString( path, pathLength );
}

const char *Path_RemovePathFromFile( const char *path ) {
	const char *lastSlash = GetLastSlash( path );

	if ( !lastSlash ) {
		lastSlash = path;
	} else {
		lastSlash++;
	}

	return lastSlash;
}

const char *Path_RemoveFileExtension( const char *filename ) {
	const char *dot = strrchr( filename, '.' );

	if ( !dot ) {
		return filename;
	}

	u64 resultLength = Cast( u64, dot ) - Cast( u64, filename );

	return Str_TempCString( filename, resultLength );
}

static const char *Path_JoinInternalV( const int count, va_list args ) {
	stringBuilder_t builder = {};
	StringBuilder_Init( &builder, g_tempStorage );

	For ( int, argIndex, 0, count ) {
		if ( argIndex > 0 ) {
			StringBuilder_Appendf( &builder, PATH_SEPARATOR );
		}

		const char* part = va_arg( args, const char * );

		StringBuilder_Appendf( &builder, part );
	}

	return StringBuilder_ToString( &builder );
}

const char *Path_JoinInternal( const int count, ... ) {
	va_list args;
	va_start( args, count );
	const char *result = Path_JoinInternalV( count, args );
	va_end( args );

	return result;
}
