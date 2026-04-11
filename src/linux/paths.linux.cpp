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

#include <paths.h>

#include <core_helpers.h>
#include <debug.h>
#include <paths.h>
#include <temp_storage.h>
#include <string_builder.h>
#include <typecast.inl>
#include <core_string.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>

/*
================================================================================================

	Paths

	Linux-specific functionality

================================================================================================
*/

const char *Path_AppPath() {
	char *result = Cast( char *, Mem_TempAlloc( PATH_MAX * sizeof( char ) ) );
	s64 length = readlink( "/proc/self/exe", result, PATH_MAX );

	if ( length == -1 ) {
		int err = errno;
		FatalError( "Failed to get app path: %s.\n", strerror( err ) );
	}

	result[length] = 0;

	return result;
}

const char *Path_CurrentWorkingDirectory() {
	char temp[PATH_MAX];

	const char *cwd = getcwd( temp, sizeof( temp ) );

	if ( !cwd ) {
		int err = errno;
		FatalError( "Failed to get CWD: %s.\n", strerror( err ) );
	}

	return cwd;
}

const char *Path_AbsolutePath( const char *file ) {
	Unused( file );

	assert( false );

	return NULL;
}

bool8 Path_IsAbsolute( const char *path ) {
	assert( path );

	return path[0] == '/';
}

const char *Path_Canonicalise( const char *path ) {
	assert( path );

	const char *pathCopy = Str_TempCString( path, PATH_MAX );

	const char *result = realpath( pathCopy, NULL );
	if ( !result ) {
		int err = errno;
		printf( "Failed to get real path of \"%s\": %s.\n", path, strerror( err ) );
		return NULL;
	}

	return result;
}

const char *Path_FixSlashes( const char *path ) {
	u64 pathLength = strlen( path );
	char* result = Str_TempCString( path, pathLength );

	For ( u64, charIndex, 0, pathLength ) {
		if ( result[charIndex] == '\\' ) {
			result[charIndex] = '/';
		}
	}

	return result;
}

char* Path_RelativePathTo( const char *pathFrom, const char *pathTo ) {
	assert( pathFrom );
	assert( pathTo );

	const char *pathFromCopy = pathFrom;
	const char *pathToCopy = pathTo;

	u32 numSameChars = 0;
	u32 numBacks = 0;

	while ( pathFromCopy[numSameChars] && pathToCopy[numSameChars] && pathFromCopy[numSameChars] == pathToCopy[numSameChars] ) {
		numSameChars += 1;
	}

	pathFromCopy = pathFrom + numSameChars;
	pathToCopy = pathTo + numSameChars;

	// skip the first one of these if there is one
	/*if ( *pathFromCopy == '/' ) {
		pathFromCopy += 1;
	}*/

	while ( *pathFromCopy ) {
		if ( *pathFromCopy == '/' ) {
			numBacks += 1;
		}
		pathFromCopy += 1;
	}

	stringBuilder_t sb = {};
	StringBuilder_Init( &sb, g_tempStorage );

	For ( u32, backIndex, 0, numBacks ) {
		StringBuilder_Appendf( &sb, "../" );
	}

	StringBuilder_Appendf( &sb, pathTo + numSameChars );

	//char* result = Cast( char*, Mem_TempAlloc( PATH_MAX * sizeof( char ) ) );
	char* result = Cast( char*, StringBuilder_ToString( &sb ) );

	return result;
}

bool8 Path_SetCurrentDirectory( const char *path ) {
	if ( chdir( path ) != 0 ) {
		int err = errno;
		FatalError( "Failed to set current directory: %s.\n", strerror( err ) );

		return false;
	}

	return true;
}

#endif // __linux__
