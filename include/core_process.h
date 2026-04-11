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
#include "core_array.h"	// DM!!! is this ok?
#include "dll_export.h"

struct linearAllocator_t;

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wc++98-compat-pedantic"
#endif

struct process_t;

enum processFlagBits_t {
	PROCESS_FLAG_ASYNC	= 1,
	PROCESS_FLAG_COMBINE_STDOUT_AND_STDERR,
};
typedef u32 processFlags_t;


CORE_API process_t	*Process_Create( linearAllocator_t *allocator, Array<const char *> *args, Array<const char *> *environmentVariables, const processFlags_t flags );

CORE_API void		Process_Destroy( process_t *process );

CORE_API s32		Process_Join( process_t *process );

CORE_API u32		Process_ReadStdout( process_t *process, char *outBuffer, const u32 count );

#ifdef __clang__
#pragma clang diagnostic pop
#endif
