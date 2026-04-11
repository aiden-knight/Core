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

#include <file.h>
#include "../file_local.h"
#include <paths.h>
#include <core_array.inl>
#include <defer.h>
#include <core_string.h>
#include <temp_storage.h>

#include <unistd.h>
#include <fcntl.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>

/*
================================================================================================

	Linux File IO implementations

================================================================================================
*/

static file_t OpenFileInternal( const char *filename, int flags ) {
	assert( filename );

	int handle = open( filename, flags, S_IRWXU | S_IRWXG | S_IRWXO );
	if ( handle == -1 ) {
		return { INVALID_FILE_HANDLE, 0 };
	}

	return { TruncCast( u64, handle ), 0 };
}

file_t FS_OpenFile( const char *filename, const fileOpenFlags_t openFlags ) {
	assert( filename );

	int openFlagsLinux;
	if ( ( openFlags & FILE_OPEN_READ ) && ( openFlags & FILE_OPEN_WRITE ) ) {
		openFlagsLinux = O_RDWR;
	} else if ( openFlags & FILE_OPEN_WRITE ) {
		openFlagsLinux = O_WRONLY;
	} else {
		openFlagsLinux = O_RDONLY;
	}

	return OpenFileInternal( filename, openFlagsLinux );
}

file_t FS_OpenOrCreateFile( const char *filename, const bool8 keepExistingContent ) {
	assert( filename );

	int flags = O_CREAT | O_RDWR;

	if ( !keepExistingContent ) {
		flags |= O_TRUNC;
	}

	return OpenFileInternal( filename, flags );
}

bool8 FS_CloseFile( file_t* file ) {
	assert( file );
	assert( file->handle != INVALID_FILE_HANDLE );

	if ( close( TruncCast( int, file->handle ) ) != 0 ) {
		int err = errno;

		Error( "Failed to close file \"%s\": %s\n", strerror( err ) );

		return false;
	}

	return true;
}

bool8 FS_CopyFile( const char *originalPath, const char *newPath ) {
	assert( originalPath );
	assert( newPath );

	char *buffer = NULL;
	if ( !FS_ReadEntireFile( originalPath, &buffer ) ) {
		return false;
	}

	defer { FS_FreeFileBuffer( &buffer ); };

	if ( !FS_WriteEntireFile( newPath, buffer, strlen( buffer ) ) ) {
		return false;
	}

	return true;
}

bool8 FS_RenameFile( const char *oldFilename, const char *newFilename ) {
	assert( oldFilename );
	assert( newFilename );

	int result = rename( oldFilename, newFilename );
	if ( !result ) {
		int err = errno;
		FatalError( "Failed to rename file \"%s\" to \"%s\": %s.\n", strerror( err ) );
		return false;
	}

	return true;
}

bool8 FS_ReadFile( file_t* file, const u64 offset, const u64 size, void* outData ) {
	assert( file && file->handle != INVALID_FILE_HANDLE );
	assert( size );
	assert( outData );

	ssize_t bytesRead = pread( TruncCast( int, file->handle ), outData, size, TruncCast( off_t, offset ) );

	return TruncCast( u64, bytesRead ) == size;
}

bool8 FS_WriteFile( file_t* file, const void* data, const u64 offset, const u64 size ) {
	assert( file && file->handle != INVALID_FILE_HANDLE );
	assert( data );
	assert( size );

	ssize_t bytesWritten = pwrite( TruncCast( int, file->handle ), data, size, TruncCast( off_t, offset ) );

	return TruncCast( u64, bytesWritten ) == size;
}

bool8 FS_DeleteFile( const char *filename ) {
	assert( filename );

	int result = remove( filename );

	if ( result != 0 ) {
		int err = errno;
		FatalError( "Failed to delete file \"%s\": %s.\n", strerror( err ) );
	}

	return result == 0;
}

bool8 FS_GetFileSize( const char *filename, u64 *outSize ) {
	assert( filename );
	assert( outSize );

	struct stat fileStat = {};
	if ( stat( filename, &fileStat ) != 0 ) {
		return false;
	}

	*outSize = TruncCast( u64, fileStat.st_size );

	return true;
}

bool8 FS_GetFileLastWriteTime( const char *filename, u64 *outLastWriteTime ) {
	assert( filename );
	assert( outLastWriteTime );

	struct stat fileStat = {};
	if ( stat( filename, &fileStat ) != 0 ) {
		return false;
	}

	*outLastWriteTime = TruncCast( u64, fileStat.st_mtime );

	return true;
}

bool8 FS_GetAllFilesInFolder( const char *path, const fileVisitFlags_t visitFlags, fileVisitCallback_t visitCallback, void *userData ) {
	assert( path );
	assert( visitCallback );

	Array<const char *> directories;
	directories.Init( g_tempStorage );
	directories.Add( path );

	u32 dirIndex = 0;

	while ( dirIndex < directories.count ) {
		const char *directory = directories[dirIndex];

		//printf( "Scanning directory \"%s\"\n", directory );

		DIR *dir = opendir( directory );
		defer { closedir( dir ); };

		dirIndex += 1;

		if ( !dir ) {
			int err = errno;
			printf( "Can't open dir \"%s\": %s\n", directory, strerror( err ) );
			return false;
		}

		struct dirent *entry = NULL;
		while ( ( entry = readdir( dir ) ) != NULL ) {
			if ( Str_Equals( entry->d_name, "." ) || Str_Equals( entry->d_name, ".." ) ) {
				continue;
			}

			const char *fullFilename = Str_TempPrintf( "%s%c%s", directory, PATH_SEPARATOR, entry->d_name );

			struct stat fileStat = {};
			if ( stat( fullFilename, &fileStat ) != 0 ) {
				/*int err = errno;
				printf( "Can't stat \"%s\": %s\n", fullFilename, strerror( err ) );*/
				return false;
			}

			fileInfo_t fileInfo = {
				.sizeBytes		= TruncCast( u64, fileStat.st_size ),
				.lastWriteTime	= TruncCast( u64, fileStat.st_mtime ),
				.isDirectory	= S_ISDIR( fileStat.st_mode ),
				.filename		= entry->d_name,
				.fullFilename	= fullFilename,
			};

			if ( fileInfo.isDirectory ) {
				if ( visitFlags & FILE_VISIT_FOLDERS ) {
					visitCallback( &fileInfo, userData );
				}

				if ( visitFlags & FILE_VISIT_RECURSIVE ) {
					directories.Add( fullFilename );
				}
			} else if ( visitFlags & FILE_VISIT_FILES ) {
				visitCallback( &fileInfo, userData );
			}
		}
	}

	return true;
}

bool8 FS_FileExists( const char *filename ) {
	assert( filename );

	return access( filename, 0 ) == 0;
}

bool8 CreateFolderInternal( const char *path ) {
	int result = mkdir( path, S_IRWXU | S_IRWXG | S_IRWXO );
	int err = errno;

	if ( ( result != 0 ) && ( err != EEXIST ) ) {
		FatalError( "ERROR: Failed to create directory \"%s\": %s\n", path, strerror( err ) );
	}

	return result == 0;
}

bool8 FS_DeleteFolder( const char *path ) {
	assert( path );

	int result = rmdir( path );

	if ( result != 0 ) {
		int err = errno;
		FatalError( "Failed to delete folder \"%s\": %s.\n", strerror( err ) );
	}

	return result == 0;
}

bool8 FS_FolderExists( const char *path ) {
	assert( path );

	DIR* dir = opendir( path );
	int err = errno;

	if ( dir ) {
		closedir( dir );
		return true;
	} else if ( err == ENOENT ) {
		return false;
	}

	FatalError( "Failed to check if the folder exists: %s\n", strerror( err ) );

	return false;
}

#endif // __linux__
