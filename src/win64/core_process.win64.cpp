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

#include <core_process.h>

#include <typecast.inl>
#include <string_builder.h>
#include <core_array.inl>
#include <core_helpers.h>
#include <linear_allocator.h>
#include <temp_storage.h>
#include <defer.h>

#include <Windows.h>

/*
================================================================================================

	Process

================================================================================================
*/

struct process_t {
	PROCESS_INFORMATION	processInfo;
	HANDLE				stdoutRead;
	HANDLE				stdoutWrite;
	HANDLE				eventStdout;
};

process_t* Process_Create( linearAllocator_t *allocator, Array<const char *> *args, Array<const char *> *environmentVariables, const processFlags_t flags ) {
	assert( allocator );
	assert( args );
	assert( args->count > 0 );

	Unused( environmentVariables );

	process_t *process = Cast( process_t *, Mem_Alloc( allocator, sizeof( process_t ) ) );

	SECURITY_ATTRIBUTES secAttr = { sizeof( SECURITY_ATTRIBUTES ), NULL, TRUE };

	// stdout
	if ( !CreatePipe( &process->stdoutRead, &process->stdoutWrite, &secAttr, 0 ) ) {
		Error( "CreatePipe call failed: 0x%X.\n", GetLastError() );
		return NULL;
	}

	STARTUPINFO startInfo = { sizeof( startInfo ) };
	startInfo.dwFlags = STARTF_USESTDHANDLES;
	startInfo.hStdOutput = process->stdoutWrite;
	startInfo.hStdError = NULL;//process->stdoutWrite;

	char *combinedArgs = NULL;
	{
		u64 offset = 0;

		u64 combinedArgsLength = 0;

		For ( u64, argIndex, 0, args->count ) {
			combinedArgsLength += strlen( ( *args )[argIndex] );
		}
		combinedArgsLength += args->count - 1;	// one space between each argument
		combinedArgsLength += 1;				// null terminator

		combinedArgs = Cast( char *, Mem_TempAlloc( combinedArgsLength * sizeof( char ) ) );

		For ( u64, argIndex, 0, args->count ) {
			const char *arg = ( *args )[argIndex];

			u64 argLen = strlen( arg );

			strncpy( combinedArgs + offset, arg, argLen * sizeof( char ) );
			offset += argLen;

			combinedArgs[offset] = ' ';
			offset += 1;
		}
		combinedArgs[combinedArgsLength - 1] = 0;
	}

	if ( flags & PROCESS_FLAG_ASYNC ) {
		process->eventStdout = CreateEvent( &secAttr, 1, 1, NULL );
	}

	BOOL created = CreateProcess(
		NULL,
		const_cast<LPSTR>( combinedArgs ),
		NULL,
		NULL,
		true,
		CREATE_NO_WINDOW,
		NULL,
		NULL,
		&startInfo,
		&process->processInfo
	);

	if ( !created ) {
		Error( "CreateProcess() failed: 0x%X.\n", GetLastError() );
		return NULL;
	}

#if 1
	CloseHandle( startInfo.hStdOutput );
	//startInfo.hStdOutput = NULL;
#else
	CloseHandle( process->stdoutWrite );
	//process->stdoutWrite = NULL;
#endif

	CloseHandle( process->processInfo.hThread );
	//process->processInfo.hThread = NULL;

	return process;
}

void Process_Destroy( process_t* process ) {
	assert( process );

	if ( process->stdoutRead ) {
		CloseHandle( process->stdoutRead );
		process->stdoutRead = NULL;
	}

	if ( process->processInfo.hProcess ) {
		CloseHandle( process->processInfo.hProcess );
		process->processInfo.hProcess = NULL;
	}

	if ( process->processInfo.hThread ) {
		CloseHandle( process->processInfo.hThread );
		process->processInfo.hThread = NULL;
	}

	if ( process->eventStdout ) {
		CloseHandle( process->eventStdout );
		process->eventStdout = NULL;
	}
}

s32 Process_Join( process_t* process ) {
	assert( process );

	CloseHandle( process->stdoutRead );
	process->stdoutRead = NULL;

	WaitForSingleObject( process->processInfo.hProcess, INFINITE );

	DWORD exitCode = 0;
	BOOL gotExitCode = GetExitCodeProcess( process->processInfo.hProcess, &exitCode );

	assert( gotExitCode );
	Unused( gotExitCode );

	return TruncCast( s32, exitCode );
}

u32 Process_ReadStdout( process_t *process, char *outBuffer, const u32 count ) {
	assert( process );
	assert( outBuffer );
	assert( count > 0 );

	OVERLAPPED overlapped = {};
	overlapped.hEvent = process->eventStdout;

	DWORD bytesRead = 0;
	BOOL read = ReadFile( process->stdoutRead, outBuffer, count, &bytesRead, &overlapped );

	if ( !read ) {
		DWORD lastError = GetLastError();

		if ( lastError == ERROR_IO_PENDING ) {
			if ( !GetOverlappedResult( process->stdoutRead, &overlapped, &bytesRead, 1 ) ) {
				lastError = GetLastError();

				if ( ( lastError != ERROR_IO_INCOMPLETE ) && ( lastError != ERROR_HANDLE_EOF ) ) {
					Error( "Failed to read stdout of subprocess: 0x%X.\n", GetLastError() );
					return 0;
				}
			}
		}
	}

	return bytesRead;
}

#endif // _WIN32
