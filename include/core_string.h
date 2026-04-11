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

#include "int_types.h"
#include "dll_export.h"

struct linearAllocator_t;

/*
================================================================================================

	String

	Container type used to hold a contiguous block of text.

	Unlike std::strings, Core Strings can't be appended or resized.  If you want to do
	that, use StringBuilder.

================================================================================================
*/

struct string_t {
	linearAllocator_t	*allocator;
	char				*data;
	u64					count;
};

CORE_API void				Str_Init( string_t *outStr, linearAllocator_t *allocator );
CORE_API void				Str_Zero( string_t *outStr );

CORE_API void				Str_Printf( string_t *outStr, const char *fmt, ... );

CORE_API void				Str_Copy( string_t *dst, string_t *src );

CORE_API void				Str_CopyFromCString( string_t *outStr, const char *cStr );
CORE_API void				Str_CopyFromCString( string_t *outStr, const char *cStr, const u64 length );

// Returns true if the contents of string 'lhs' are EXACTLY the same as the contents of string 'rhs'.  Case sensitive.
CORE_API bool8				Str_Equals( const char *lhs, const char *rhs );

// Returns true if the first characters of string 'str' are EXACTLY the same as string 'prefix'.  Case sensitive.
CORE_API bool8				Str_StartsWith( const char *str, const char *prefix );

// Returns true if the last character of 'str' is the value of 'end'.  Case sensitive.
CORE_API bool8				Str_EndsWith( const char *str, const char end );

// Returns true if the last characters of 'str' are EXACTLY the same as string 'suffix'.  Case sensitive.
CORE_API bool8				Str_EndsWith( const char *str, const char *suffix );

// Returns true if string 'str' has EXACTLY the contents of 'substring' somewhere in it.  Case sensitive.
CORE_API bool8				Str_Contains( const char *str, const char *substring );

CORE_API const char			*Str_Replace( const char *str, const char oldChar, const char newChar );

// Returns a printf-formatted string with the given format string and var args that's been allocated via temp storage.
CORE_API const char			*Str_TempPrintf( const char *fmt, ... );

// Returns a copy of 'from' that has been allocated on temp storage.
CORE_API char				*Str_TempCString( const char *from );

// Copies 'length' characters from 'from' and allocates it on temp storage.
CORE_API char				*Str_TempCString( const char *from, const u64 length );
