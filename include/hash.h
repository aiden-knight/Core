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


// Creates a hash based on the given 32 bit 'data' that is 'length' bytes long.
// If 'seed' is zero then will not use a pre-existing seed as a base for the hash.
CORE_API u32		Hash_32( const void *data, const u64 length, const u32 seed );

// Creates a hash based on the given 64 bit 'data' that is 'length' bytes long.
// If 'seed' is zero then will not use a pre-existing seed as a base for the hash.
CORE_API u64		Hash_64( const void *data, const u64 length, const u64 seed );

// Returns a 64 bit hash based on the given string.
// If 'seed' is zero then will not use a pre-existing seed as a base for the hash.
CORE_API u64		Hash_String( const char *string, const u64 seed );


struct hasher_t;

CORE_API hasher_t	*Hasher_Create( const u64 seed );
CORE_API void		Hasher_Destroy( hasher_t *hasher );

CORE_API void		Hasher_Reset( hasher_t *hasher, const u64 seed );

// DM: dont think generalizing this to a pointer here is actually the best thing to do
// lets just have the following functions:
//
//	Hasher_Hash1
//	Hasher_Hash2
//	Hasher_Hash4
//	Hasher_Hash8
//	Hasher_Hash16
//	Hasher_Hash32
//	Hasher_Hash64
CORE_API void		Hasher_Hash( hasher_t *hasher, const void *ptr, const u64 size );

CORE_API u64		Hasher_GetHash( hasher_t *hasher );
