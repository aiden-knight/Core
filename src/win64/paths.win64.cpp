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

#include <paths.h>

#include <core_helpers.h>
#include <temp_storage.h>
#include <debug.h>
#include <typecast.inl>
#include <core_string.h>

#include <Windows.h>
#include <Shlwapi.h>

/*
================================================================================================

	Paths

	Win64-specific functionality

================================================================================================
*/

const char *Path_AppPath() {
	char *appFullPath = Cast( char *, Mem_TempAlloc( MAX_PATH * sizeof( char ) ) );
	GetModuleFileNameA( NULL, appFullPath, MAX_PATH );

	return appFullPath;
}

const char *Path_CurrentWorkingDirectory() {
	char *cwd = Cast( char *, Mem_TempAlloc( MAX_PATH * sizeof( char ) ) );
	DWORD length = GetCurrentDirectory( MAX_PATH, cwd );
	cwd[length] = 0;

	return cwd;
}

const char *Path_AbsolutePath( const char *file ) {
	assert( file );

	char *absolutePath = Cast( char *, Mem_TempAlloc( MAX_PATH * sizeof( char ) ) );
	DWORD length = GetFullPathName( file, MAX_PATH, absolutePath, NULL );
	absolutePath[length] = 0;

	return absolutePath;
}

bool8 Path_IsAbsolute( const char *path ) {
	if ( !path || strlen( path ) < 3 ) {
		return false;
	}

	return isalpha( path[0] ) && path[1] == ':' && ( path[2] == '\\' || path[2] == '/' );
}

const char *Path_Canonicalise( const char *path ) {
	assert( path );

	const char *pathCopy = Path_FixSlashes( path );

	u64 maxPathLength = ( strlen( pathCopy ) + 1 ) * sizeof( char );

	char *result = Cast( char *, Mem_TempAlloc( maxPathLength ) );

	BOOL success = PathCanonicalizeA( result, pathCopy );
	assert( success );
	Unused( success );

	result[maxPathLength - 1] = 0;

	if ( *result == '/' ) {
		result++;
	} else if ( *result == '\\' ) {
		result++;
	}

	return result;
}

const char *Path_FixSlashes( const char *path ) {
	u64 pathLength = strlen( path );
	char *result = Str_TempCString( path, pathLength );

	For ( u64, charIndex, 0, pathLength ) {
		if ( result[charIndex] == '/' ) {
			result[charIndex] = '\\';
		}
	}

	return result;
}

// TODO: DM: 10/10/2025: I'm very tempted to remove this implementation and make the linux one work cross platform
// for some reason this windows implementation cares whether or not the files we are referencing are files or directories and it starts returning very wrong answers if we are wrong about the file type we specify vs the actual path
// the linux implementation doesnt care about any of that and instead just compares the strings (which is all it needs to do)
char *Path_RelativePathTo( const char *pathFrom, const char *pathTo ) {
	assert( pathFrom );
	assert( pathTo );

	char *result = Cast( char *, Mem_TempAlloc( MAX_PATH * sizeof( char ) ) );

	if ( !PathRelativePathTo( result, Path_FixSlashes( pathFrom ), FILE_ATTRIBUTE_DIRECTORY, Path_FixSlashes( pathTo ), FILE_ATTRIBUTE_DIRECTORY ) ) {
		Error( "Unable to compute relative path, ensure provided paths exist.\nFrom Path: %s\nTo Path: %s", pathFrom, pathTo );
	}

	return result;
}

bool8 Path_SetCurrentDirectory( const char *path ) {
	assert( path );

	return Cast( bool8, SetCurrentDirectory( path ) );
}

#endif // _WIN32
