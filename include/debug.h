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

#pragma once

#if defined( __clang__ )
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wc++98-compat-pedantic"
#endif

#include "int_types.h"
#include "dll_export.h"

struct linearAllocator_t;

template<class T> struct Array;

#ifdef _WIN32
	#define debug_break	__debugbreak
#elif defined(__linux__)
	#define debug_break	__builtin_trap
#else
	#error Unrecognised platform!
#endif

#ifdef _DEBUG
	#define assert( condition ) \
		do { \
			if ( !(condition) ) { \
				AssertInternal( __FILE__, __LINE__, #condition ); \
				debug_break(); \
			} \
		} while ( 0 )
#else
	#define assert( condition )
#endif

enum consoleTextColor_t {
	CONSOLE_TEXT_COLOR_DEFAULT	= 0,
	CONSOLE_TEXT_COLOR_RED,
	CONSOLE_TEXT_COLOR_YELLOW,
	CONSOLE_TEXT_COLOR_BLUE,
	CONSOLE_TEXT_COLOR_BRIGHT_BLUE,
	CONSOLE_TEXT_COLOR_LIGHT_GRAY,
};

CORE_API Array<const char *>	GetCallstack( linearAllocator_t *allocator );
CORE_API void					DumpCallstack();

CORE_API s32					GetLastErrorCode();

CORE_API void					SetConsoleTextColor( const consoleTextColor_t color );

CORE_API void					Warning( const char *fmt, ... );
CORE_API void					Error( const char *fmt, ... );
CORE_API void					FatalError( const char *fmt, ... );

// do not call this one directly
// call assert() instead
CORE_API void					AssertInternal( const char *file, const int line, const char *fmt, ... );

#if defined( __clang__ )
#pragma clang diagnostic pop
#endif
