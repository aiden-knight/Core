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

#include <debug.h>

#include <core_array.inl>
#include <core_helpers.h>
#include <linear_allocator.h>
#include <temp_storage.h>

#include <stdio.h>
#include <errno.h>

Array<const char *> GetCallstack( linearAllocator_t *allocator ) {
	// TODO: DM: 05/04/2026: implement
	Unused( allocator );
	assert( false );

	Array<const char *> callstack;
	callstack.Init( allocator );

	return callstack;
}

void DumpCallstack() {
	// TODO: DM: 05/04/2026: implement
	assert( false );
}

void SetConsoleTextColor( const consoleTextColor_t color ) {
	const char* colorLinux = NULL;

	switch ( color ) {
		case CONSOLE_TEXT_COLOR_DEFAULT:		colorLinux = "\033[0m"; break;
		case CONSOLE_TEXT_COLOR_RED:			colorLinux = "\033[0;31m"; break;
		case CONSOLE_TEXT_COLOR_YELLOW:			colorLinux = "\033[0;32m"; break;
		case CONSOLE_TEXT_COLOR_BLUE:			colorLinux = "\033[1;34m"; break;
		case CONSOLE_TEXT_COLOR_BRIGHT_BLUE:	colorLinux = "\033[1;94m"; break;
		case CONSOLE_TEXT_COLOR_LIGHT_GRAY:		colorLinux = "\033[1;37m"; break;
	}

	assert( colorLinux != NULL );

	printf( "%s", colorLinux );
}

s32 GetLastErrorCode() {
	// errno is a global and therefore not thread safe
	// so it MUST ALWAYS be cached ASAP
	int err = errno;
	return err;
}

#endif // __linux__
