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

#include "test_dll/test_dll.h"

#include "../include/int_types.h"
#include "../include/typecast.inl"
#include "../include/core_helpers.h"
#include "../include/linear_allocator.h"
#include "../include/defer.h"
#include "../include/core_string.h"
#include "../include/file.h"
#include "../include/core_math.h"
#include "../include/debug.h"
#include "../include/hash.h"
#include "../include/hashmap.h"
#include "../include/random.h"
#include "../include/timer.h"
#include "../include/string_builder.h"
#include "../include/library.h"
#include "../include/temp_storage.h"
#include "../include/core_thread.h"
#include "../include/core_array.inl"
#include "../include/paths.h"

#define TEMPER_IMPLEMENTATION
#define TEMPERDEV_ASSERT assert
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "temper/temper.h"

#define PRINT_HASHMAP_PROBE_RESULTS 0


/*
================================================================================================

	core types

================================================================================================
*/

TEMPER_TEST( Test_NumberTypes_Sizes, TEMPER_FLAG_SHOULD_RUN ) {
	TEMPER_CHECK_TRUE( sizeof( s8 )  == 1 );
	TEMPER_CHECK_TRUE( sizeof( s16 ) == 2 );
	TEMPER_CHECK_TRUE( sizeof( s32 ) == 4 );
	TEMPER_CHECK_TRUE( sizeof( s64 ) == 8 );

	TEMPER_CHECK_TRUE( sizeof( u8 )  == 1 );
	TEMPER_CHECK_TRUE( sizeof( u16 ) == 2 );
	TEMPER_CHECK_TRUE( sizeof( u32 ) == 4 );
	TEMPER_CHECK_TRUE( sizeof( u64 ) == 8 );

	TEMPER_CHECK_TRUE( sizeof( float32 ) == 4 );
	TEMPER_CHECK_TRUE( sizeof( float64 ) == 8 );
}

TEMPER_TEST( Test_NumberTypes_Ranges, TEMPER_FLAG_SHOULD_RUN ) {
	TEMPER_CHECK_TRUE( S8_MIN  == ( -127 - 1 ) );
	TEMPER_CHECK_TRUE( S16_MIN == ( -32767 - 1 ) );
	TEMPER_CHECK_TRUE( S32_MIN == ( -2147483647 - 1 ) );
	TEMPER_CHECK_TRUE( S64_MIN == ( -9223372036854775807 - 1 ) );

	TEMPER_CHECK_TRUE( S8_MAX  == 127 );
	TEMPER_CHECK_TRUE( S16_MAX == 32767 );
	TEMPER_CHECK_TRUE( S32_MAX == 2147483647 );
	TEMPER_CHECK_TRUE( S64_MAX == 9223372036854775807 );

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wfloat-equal"
	TEMPER_CHECK_TRUE( FLOAT32_MIN == 1.17549435e-38f );
	TEMPER_CHECK_TRUE( FLOAT32_MAX == 3.40282347e+38f );

	TEMPER_CHECK_TRUE( FLOAT64_MIN == 2.2250738585072014e-308 );
	TEMPER_CHECK_TRUE( FLOAT64_MAX == 1.7976931348623157e+308 );
#pragma clang diagnostic pop
}


/*
================================================================================================

	core helpers

================================================================================================
*/

TEMPER_TEST( Test_AlignUp, TEMPER_FLAG_SHOULD_RUN ) {
	// already aligned - should return same value
	TEMPER_CHECK_TRUE( AlignUp(  0, 8 ) ==  0 );
	TEMPER_CHECK_TRUE( AlignUp(  8, 8 ) ==  8 );
	TEMPER_CHECK_TRUE( AlignUp( 16, 8 ) == 16 );

	// not aligned - should round up to next multiple
	TEMPER_CHECK_TRUE( AlignUp(  1, 8 ) ==  8 );
	TEMPER_CHECK_TRUE( AlignUp(  7, 8 ) ==  8 );
	TEMPER_CHECK_TRUE( AlignUp(  9, 8 ) == 16 );

	// larger alignments
	TEMPER_CHECK_TRUE( AlignUp(   1, 4096 ) == 4096 );
	TEMPER_CHECK_TRUE( AlignUp( 100, 4096 ) == 4096 );
	TEMPER_CHECK_TRUE( AlignUp( 4096, 4096 ) == 4096 );
	TEMPER_CHECK_TRUE( AlignUp( 4097, 4096 ) == 8192 );
}


/*
================================================================================================

	linear allocator

================================================================================================
*/

static linearAllocator_t *g_testLinearAllocator = NULL;

struct Thing {
	int			x;
	const char	*msg;
};

TEMPER_TEST_PARAMETRIC( Test_LinearAllocator_Create, TEMPER_FLAG_SHOULD_RUN, linearAllocator_t **allocator, const u64 reservedBytes ) {
	*allocator = Mem_CreateAllocator( reservedBytes );

	TEMPER_CHECK_TRUE( *allocator != NULL );
	TEMPER_CHECK_TRUE( ( *allocator )->ptr != NULL );
	TEMPER_CHECK_TRUE( ( *allocator )->offset == 0 );
	TEMPER_CHECK_TRUE( ( *allocator )->reservedBytes == reservedBytes );
	TEMPER_CHECK_TRUE( ( *allocator )->comittedBytes == 0 );
	TEMPER_CHECK_TRUE( ( *allocator )->virtualMemoryPageSize > 0 );
}

TEMPER_TEST_PARAMETRIC( Test_LinearAllocator_Alloc, TEMPER_FLAG_SHOULD_RUN, linearAllocator_t *allocator, const Thing thing ) {
	u64 offsetBefore = Mem_Tell( allocator );
	TEMPER_CHECK_TRUE( offsetBefore == allocator->offset );

	Thing *ptr = Cast( Thing *, Mem_Alloc( allocator, sizeof( Thing ) ) );
	*ptr = thing;

	TEMPER_CHECK_TRUE( ptr != NULL );
	TEMPER_CHECK_TRUE( allocator->offset == AlignUp( offsetBefore, 8U ) + sizeof( Thing ) );
	TEMPER_CHECK_TRUE( Mem_Tell( allocator ) == allocator->offset );
	TEMPER_CHECK_TRUE( ptr->x == thing.x );
	TEMPER_CHECK_TRUE( Str_Equals( ptr->msg, thing.msg ) );
}

TEMPER_TEST_PARAMETRIC( Test_LinearAllocator_Reset, TEMPER_FLAG_SHOULD_RUN, linearAllocator_t *allocator ) {
	u8 *ptrBefore = allocator->ptr;
	u64 reservedBytesBefore = allocator->reservedBytes;
	u64 comittedBytesBefore = allocator->comittedBytes;
	u64 virtualMemoryPageSizeBefore = allocator->virtualMemoryPageSize;

	Mem_ResetAllocator( allocator );

	TEMPER_CHECK_TRUE( allocator->offset == 0 );
	TEMPER_CHECK_TRUE( allocator->ptr == ptrBefore );
	TEMPER_CHECK_TRUE( allocator->reservedBytes == reservedBytesBefore );
	TEMPER_CHECK_TRUE( allocator->comittedBytes == comittedBytesBefore );
	TEMPER_CHECK_TRUE( allocator->virtualMemoryPageSize == virtualMemoryPageSizeBefore );
}

TEMPER_TEST_PARAMETRIC( Test_LinearAllocator_RewindTo, TEMPER_FLAG_SHOULD_RUN, linearAllocator_t *allocator, const u64 allocSize ) {
	u64 offsetBefore = allocator->offset;

	Mem_Alloc( allocator, allocSize );

	Mem_RewindTo( allocator, offsetBefore );

	TEMPER_CHECK_TRUE( allocator->offset == offsetBefore );
}

TEMPER_TEST_PARAMETRIC( Test_LinearAllocator_RewindBy, TEMPER_FLAG_SHOULD_RUN, linearAllocator_t *allocator, const u64 allocSize ) {
	u64 offsetBefore = allocator->offset;

	Mem_Alloc( allocator, allocSize );

	u64 offsetAfter = allocator->offset;

	Mem_RewindBy( allocator, offsetAfter - offsetBefore );

	TEMPER_CHECK_TRUE( allocator->offset == offsetBefore );
}

TEMPER_TEST_PARAMETRIC( Test_LinearAllocator_Destroy, TEMPER_FLAG_SHOULD_RUN, linearAllocator_t **allocator ) {
	TEMPER_CHECK_TRUE( *allocator != NULL );

	Mem_DestroyAllocator( *allocator );
	*allocator = NULL;

	TEMPER_CHECK_TRUE( *allocator == NULL );
}

TEMPER_INVOKE_PARAMETRIC_TEST( Test_LinearAllocator_Create, &g_testLinearAllocator, 1024 * 1024 );

TEMPER_INVOKE_PARAMETRIC_TEST( Test_LinearAllocator_Alloc, g_testLinearAllocator, { 1,   "hello"          } );
TEMPER_INVOKE_PARAMETRIC_TEST( Test_LinearAllocator_Alloc, g_testLinearAllocator, { 2,   "world"          } );
TEMPER_INVOKE_PARAMETRIC_TEST( Test_LinearAllocator_Alloc, g_testLinearAllocator, { 100, "this is a test" } );

TEMPER_INVOKE_PARAMETRIC_TEST( Test_LinearAllocator_Reset, g_testLinearAllocator );

TEMPER_INVOKE_PARAMETRIC_TEST( Test_LinearAllocator_Alloc, g_testLinearAllocator, { 256,  "after reset"         } );
TEMPER_INVOKE_PARAMETRIC_TEST( Test_LinearAllocator_Alloc, g_testLinearAllocator, { 512,  "still going"         } );
TEMPER_INVOKE_PARAMETRIC_TEST( Test_LinearAllocator_Alloc, g_testLinearAllocator, { 1024, "another one"         } );
TEMPER_INVOKE_PARAMETRIC_TEST( Test_LinearAllocator_Alloc, g_testLinearAllocator, { 2048, "keeps on allocating" } );
TEMPER_INVOKE_PARAMETRIC_TEST( Test_LinearAllocator_Alloc, g_testLinearAllocator, { 4096, "last one"            } );

TEMPER_INVOKE_PARAMETRIC_TEST( Test_LinearAllocator_RewindTo, g_testLinearAllocator, 64 );
TEMPER_INVOKE_PARAMETRIC_TEST( Test_LinearAllocator_RewindTo, g_testLinearAllocator, 128 );
TEMPER_INVOKE_PARAMETRIC_TEST( Test_LinearAllocator_RewindTo, g_testLinearAllocator, 256 );

TEMPER_INVOKE_PARAMETRIC_TEST( Test_LinearAllocator_RewindBy, g_testLinearAllocator, 32 );
TEMPER_INVOKE_PARAMETRIC_TEST( Test_LinearAllocator_RewindBy, g_testLinearAllocator, 96 );
TEMPER_INVOKE_PARAMETRIC_TEST( Test_LinearAllocator_RewindBy, g_testLinearAllocator, 512 );

TEMPER_INVOKE_PARAMETRIC_TEST( Test_LinearAllocator_Destroy, &g_testLinearAllocator );


/*
================================================================================================

	Debug

================================================================================================
*/

TEMPER_TEST( Test_GetCallstack, TEMPER_FLAG_SHOULD_RUN ) {
	linearAllocator_t *allocator = Mem_CreateAllocator( 1024 * 1024 );
	defer { Mem_DestroyAllocator( allocator ); };

	Array<const char *> callstack = GetCallstack( allocator );

	TEMPER_CHECK_TRUE( callstack.count > 0 );
	TEMPER_CHECK_TRUE( Str_Contains( callstack[0], "Test_GetCallstack" ) );
}


/*
================================================================================================

	String

================================================================================================
*/

TEMPER_TEST( Test_String_Defaults, TEMPER_FLAG_SHOULD_RUN ) {
	string_t msg = {};

	TEMPER_CHECK_TRUE( msg.count == 0 );
	TEMPER_CHECK_TRUE( msg.data == NULL );
}

TEMPER_TEST( Test_String_Zero, TEMPER_FLAG_SHOULD_RUN ) {
	string_t msg;
	Str_Zero( &msg );

	TEMPER_CHECK_TRUE( msg.count == 0 );
	TEMPER_CHECK_TRUE( msg.data == NULL );
}

TEMPER_TEST_PARAMETRIC( Test_String_CopyFromCString, TEMPER_FLAG_SHOULD_RUN, const char *str ) {
	linearAllocator_t *allocator = Mem_CreateAllocator( 1024 * 1024 );
	defer { Mem_DestroyAllocator( allocator ); };

	string_t msg = {};
	Str_Init( &msg, allocator );
	Str_CopyFromCString( &msg, str );

	TEMPER_CHECK_TRUE( Str_Equals( msg.data, str ) );
}

TEMPER_INVOKE_PARAMETRIC_TEST( Test_String_CopyFromCString, "test" );
TEMPER_INVOKE_PARAMETRIC_TEST( Test_String_CopyFromCString, "this is only a test" );
TEMPER_INVOKE_PARAMETRIC_TEST( Test_String_CopyFromCString, "" );
TEMPER_INVOKE_PARAMETRIC_TEST( Test_String_CopyFromCString, "." );

TEMPER_TEST_PARAMETRIC( Test_String_Copy, TEMPER_FLAG_SHOULD_RUN, const char *str ) {
	linearAllocator_t *allocator = Mem_CreateAllocator( 1024 * 1024 );
	defer { Mem_DestroyAllocator( allocator ); };

	string_t msg = {};
	Str_Init( &msg, allocator );
	Str_CopyFromCString( &msg, str );

	string_t actualCopy = {};
	Str_Init( &actualCopy, allocator );
	Str_Copy( &actualCopy, &msg );

	TEMPER_CHECK_TRUE( Str_Equals( actualCopy.data, str ) );
	TEMPER_CHECK_TRUE( actualCopy.count == strlen( str ) );

	TEMPER_CHECK_TRUE( Str_Equals( actualCopy.data, msg.data ) );
	TEMPER_CHECK_TRUE( actualCopy.count == msg.count );
}

TEMPER_INVOKE_PARAMETRIC_TEST( Test_String_Copy, "A" );
TEMPER_INVOKE_PARAMETRIC_TEST( Test_String_Copy, "This is a test" );
TEMPER_INVOKE_PARAMETRIC_TEST( Test_String_Copy, "This test . has a dot in it" );
TEMPER_INVOKE_PARAMETRIC_TEST( Test_String_Copy, "What about \t escape\n characters?" );
TEMPER_INVOKE_PARAMETRIC_TEST( Test_String_Copy, "      " );

TEMPER_TEST_PARAMETRIC( Test_String_StartsWith, TEMPER_FLAG_SHOULD_RUN, const char *str, const char *expectedPrefix, const bool8 shouldMatch ) {
	TEMPER_CHECK_TRUE( Str_StartsWith( str, expectedPrefix ) == shouldMatch );
}

TEMPER_INVOKE_PARAMETRIC_TEST( Test_String_StartsWith, "This is only a test", "T",                  true  );
TEMPER_INVOKE_PARAMETRIC_TEST( Test_String_StartsWith, "This is only a test", "This",               true  );
TEMPER_INVOKE_PARAMETRIC_TEST( Test_String_StartsWith, "This is only a test", "This ",              true  );
TEMPER_INVOKE_PARAMETRIC_TEST( Test_String_StartsWith, "This is only a test", "This is",            true  );
TEMPER_INVOKE_PARAMETRIC_TEST( Test_String_StartsWith, "This is only a test", "this",               false );
TEMPER_INVOKE_PARAMETRIC_TEST( Test_String_StartsWith, "This is only a test", "this is",            false );
TEMPER_INVOKE_PARAMETRIC_TEST( Test_String_StartsWith, "This is only a test", "this is ",           false );
TEMPER_INVOKE_PARAMETRIC_TEST( Test_String_StartsWith, "This is only a test", "w42950tuweiojfgase", false );

TEMPER_TEST_PARAMETRIC( Test_String_EndsWith, TEMPER_FLAG_SHOULD_RUN, const char *str, const char *expectedSuffix, const bool8 shouldMatch ) {
	TEMPER_CHECK_TRUE( Str_EndsWith( str, expectedSuffix ) == shouldMatch );
}

TEMPER_INVOKE_PARAMETRIC_TEST( Test_String_EndsWith, "This is only a test", "t",     true  );
TEMPER_INVOKE_PARAMETRIC_TEST( Test_String_EndsWith, "This is only a test", "test",  true  );
TEMPER_INVOKE_PARAMETRIC_TEST( Test_String_EndsWith, "This is only a test", " test", true  );
TEMPER_INVOKE_PARAMETRIC_TEST( Test_String_EndsWith, "This is only a test", "T",     false );
TEMPER_INVOKE_PARAMETRIC_TEST( Test_String_EndsWith, "This is only a test", "tesT",  false );
TEMPER_INVOKE_PARAMETRIC_TEST( Test_String_EndsWith, "This is only a test", " TEST", false );

TEMPER_TEST_PARAMETRIC( Test_String_Contains, TEMPER_FLAG_SHOULD_RUN, const char *str, const char *substring, const bool8 shouldContain ) {
	TEMPER_CHECK_TRUE( Str_Contains( str, substring ) == shouldContain );
}

TEMPER_INVOKE_PARAMETRIC_TEST( Test_String_Contains, "This is only a test", "This is", true  );
TEMPER_INVOKE_PARAMETRIC_TEST( Test_String_Contains, "This is only a test", "is only", true  );
TEMPER_INVOKE_PARAMETRIC_TEST( Test_String_Contains, "This is only a test", " a test", true  );
TEMPER_INVOKE_PARAMETRIC_TEST( Test_String_Contains, "This is only a test", " ",       true  );
TEMPER_INVOKE_PARAMETRIC_TEST( Test_String_Contains, "This is only a test", "this",    false );
TEMPER_INVOKE_PARAMETRIC_TEST( Test_String_Contains, "This is only a test", "ONLY",    false );

TEMPER_TEST_PARAMETRIC( Test_String_Replace, TEMPER_FLAG_SHOULD_RUN, const char *str, const char replaceOld, const char replaceNew, const char *expectedResult ) {
	const char *actualResult = Str_Replace( str, replaceOld, replaceNew );

	TEMPER_CHECK_TRUE( Str_Equals( actualResult, expectedResult ) );

	Mem_ResetTempStorage();
}

TEMPER_INVOKE_PARAMETRIC_TEST( Test_String_Replace, "this is only a test", ' ', '_', "this_is_only_a_test" );
TEMPER_INVOKE_PARAMETRIC_TEST( Test_String_Replace, "this is only a test", 't', 'T', "This is only a TesT" );
TEMPER_INVOKE_PARAMETRIC_TEST( Test_String_Replace, "this is only a test", 's', 'x', "thix ix only a text" );


/*
================================================================================================

	hash32 and hash64

================================================================================================
*/

TEMPER_TEST_PARAMETRIC( Test_Hash_StringEqualsHash32, TEMPER_FLAG_SHOULD_RUN, const char *string, const u32 seed, const u32 expectedHash ) {
	const u64 stringLength = strlen( string );

	TEMPER_CHECK_TRUE( Hash_32( string, stringLength, seed ) == expectedHash );
}

TEMPER_TEST_PARAMETRIC( Test_Hash_StringEqualsHash64, TEMPER_FLAG_SHOULD_RUN, const char *string, const u64 seed, const u64 expectedHash ) {
	const u64 stringLength = strlen( string );

	TEMPER_CHECK_TRUE( Hash_64( string, stringLength, seed ) == expectedHash );
}

TEMPER_TEST_PARAMETRIC( Test_Hash32_IsDeterministic, TEMPER_FLAG_SHOULD_RUN, const char *string, const u32 seed ) {
	const u64 length = strlen( string );

	TEMPER_CHECK_TRUE( Hash_32( string, length, seed ) == Hash_32( string, length, seed ) );
}

TEMPER_TEST_PARAMETRIC( Test_Hash64_IsDeterministic, TEMPER_FLAG_SHOULD_RUN, const char *string, const u64 seed ) {
	const u64 length = strlen( string );

	TEMPER_CHECK_TRUE( Hash_64( string, length, seed ) == Hash_64( string, length, seed ) );
}

TEMPER_TEST_PARAMETRIC( Test_Hash32_SeedChangesOutput, TEMPER_FLAG_SHOULD_RUN, const char *string, const u32 seedA, const u32 seedB ) {
	const u64 length = strlen( string );

	TEMPER_CHECK_TRUE( Hash_32( string, length, seedA ) != Hash_32( string, length, seedB ) );
}

TEMPER_TEST_PARAMETRIC( Test_Hash64_SeedChangesOutput, TEMPER_FLAG_SHOULD_RUN, const char *string, const u64 seedA, const u64 seedB ) {
	const u64 length = strlen( string );

	TEMPER_CHECK_TRUE( Hash_64( string, length, seedA ) != Hash_64( string, length, seedB ) );
}

TEMPER_TEST_PARAMETRIC( Test_Hash32_LengthChangesOutput, TEMPER_FLAG_SHOULD_RUN, const char *data, const u64 lengthA, const u64 lengthB ) {
	TEMPER_CHECK_TRUE( Hash_32( data, lengthA, 0 ) != Hash_32( data, lengthB, 0 ) );
}

TEMPER_TEST_PARAMETRIC( Test_Hash64_LengthChangesOutput, TEMPER_FLAG_SHOULD_RUN, const char *data, const u64 lengthA, const u64 lengthB ) {
	TEMPER_CHECK_TRUE( Hash_64( data, lengthA, 0 ) != Hash_64( data, lengthB, 0 ) );
}

TEMPER_TEST_PARAMETRIC( Test_Hash_StringMatchesHash64, TEMPER_FLAG_SHOULD_RUN, const char *string, const u64 seed ) {
	TEMPER_CHECK_TRUE( Hash_String( string, seed ) == Hash_64( string, strlen( string ), seed ) );
}

TEMPER_TEST_PARAMETRIC( Test_Hasher_SingleChunkMatchesHash64, TEMPER_FLAG_SHOULD_RUN, const char *data, const u64 seed ) {
	const u64 length = strlen( data );

	hasher_t *hasher = Hasher_Create( seed );
	TEMPER_CHECK_TRUE_A( hasher );

	Hasher_Hash( hasher, data, length );
	const u64 result = Hasher_GetHash( hasher );

	Hasher_Destroy( hasher );

	TEMPER_CHECK_TRUE( result == Hash_64( data, length, seed ) );
}

TEMPER_TEST_PARAMETRIC( Test_Hasher_MultiChunkMatchesSingleChunk, TEMPER_FLAG_SHOULD_RUN, const char *data, const u64 seed ) {
	const u64 length = strlen( data );
	const u64 half   = length / 2;

	hasher_t *hasher = Hasher_Create( seed );
	TEMPER_CHECK_TRUE_A( hasher );

	Hasher_Hash( hasher, data,        half );
	Hasher_Hash( hasher, data + half, length - half );
	const u64 multiChunk = Hasher_GetHash( hasher );

	Hasher_Reset( hasher, seed );

	Hasher_Hash( hasher, data, length );
	const u64 singleChunk = Hasher_GetHash( hasher );

	Hasher_Destroy( hasher );

	TEMPER_CHECK_TRUE( multiChunk == singleChunk );
}

TEMPER_TEST_PARAMETRIC( Test_Hasher_ResetGivesSameResult, TEMPER_FLAG_SHOULD_RUN, const char *data, const u64 seed ) {
	const u64 length = strlen( data );

	hasher_t *hasher = Hasher_Create( seed );
	TEMPER_CHECK_TRUE_A( hasher );

	Hasher_Hash( hasher, data, length );
	const u64 firstHash = Hasher_GetHash( hasher );

	Hasher_Reset( hasher, seed );

	Hasher_Hash( hasher, data, length );
	const u64 secondHash = Hasher_GetHash( hasher );

	Hasher_Destroy( hasher );

	TEMPER_CHECK_TRUE( firstHash == secondHash );
}

TEMPER_INVOKE_PARAMETRIC_TEST( Test_Hash_StringEqualsHash32, "test_hash_string_value", 0, 163121569U );
TEMPER_INVOKE_PARAMETRIC_TEST( Test_Hash_StringEqualsHash64, "test_hash_string_value", 0, 17747826225912071899ULL );

TEMPER_INVOKE_PARAMETRIC_TEST( Test_Hash32_IsDeterministic, "hello",                  0  );
TEMPER_INVOKE_PARAMETRIC_TEST( Test_Hash32_IsDeterministic, "hello",                  42 );
TEMPER_INVOKE_PARAMETRIC_TEST( Test_Hash32_IsDeterministic, "test_hash_string_value", 0  );

TEMPER_INVOKE_PARAMETRIC_TEST( Test_Hash64_IsDeterministic, "hello",                  0  );
TEMPER_INVOKE_PARAMETRIC_TEST( Test_Hash64_IsDeterministic, "hello",                  42 );
TEMPER_INVOKE_PARAMETRIC_TEST( Test_Hash64_IsDeterministic, "test_hash_string_value", 0  );

TEMPER_INVOKE_PARAMETRIC_TEST( Test_Hash32_SeedChangesOutput, "hello", 0, 1 );
TEMPER_INVOKE_PARAMETRIC_TEST( Test_Hash32_SeedChangesOutput, "hello", 0, 0xDEADBEEF );

TEMPER_INVOKE_PARAMETRIC_TEST( Test_Hash64_SeedChangesOutput, "hello", 0, 1 );
TEMPER_INVOKE_PARAMETRIC_TEST( Test_Hash64_SeedChangesOutput, "hello", 0, 0xDEADBEEFBAADF00DULL );

TEMPER_INVOKE_PARAMETRIC_TEST( Test_Hash32_LengthChangesOutput, "foobar", 3, 6 );
TEMPER_INVOKE_PARAMETRIC_TEST( Test_Hash32_LengthChangesOutput, "foobar", 1, 6 );

TEMPER_INVOKE_PARAMETRIC_TEST( Test_Hash64_LengthChangesOutput, "foobar", 3, 6 );
TEMPER_INVOKE_PARAMETRIC_TEST( Test_Hash64_LengthChangesOutput, "foobar", 1, 6 );

TEMPER_INVOKE_PARAMETRIC_TEST( Test_Hash_StringMatchesHash64, "hello", 0  );
TEMPER_INVOKE_PARAMETRIC_TEST( Test_Hash_StringMatchesHash64, "hello", 42 );
TEMPER_INVOKE_PARAMETRIC_TEST( Test_Hash_StringMatchesHash64, "",      0  );

TEMPER_INVOKE_PARAMETRIC_TEST( Test_Hasher_SingleChunkMatchesHash64, "Hello, world!", 0  );
TEMPER_INVOKE_PARAMETRIC_TEST( Test_Hasher_SingleChunkMatchesHash64, "Hello, world!", 42 );

TEMPER_INVOKE_PARAMETRIC_TEST( Test_Hasher_MultiChunkMatchesSingleChunk, "Hello, world!", 0  );
TEMPER_INVOKE_PARAMETRIC_TEST( Test_Hasher_MultiChunkMatchesSingleChunk, "Hello, world!", 42 );

TEMPER_INVOKE_PARAMETRIC_TEST( Test_Hasher_ResetGivesSameResult, "Hello, world!", 0  );
TEMPER_INVOKE_PARAMETRIC_TEST( Test_Hasher_ResetGivesSameResult, "Hello, world!", 42 );


/*
================================================================================================

	Hashmap

================================================================================================
*/

TEMPER_TEST( Test_Hashmap_Combine, TEMPER_FLAG_SHOULD_RUN ) {
	u32 loPart = 0xDEADBEEF;
	u32 hiPart = 0xBAADF00D;

	u64 combined = Hashmap_InternalCombine( hiPart, loPart );
	TEMPER_CHECK_TRUE_A( combined == 0xDEADBEEFBAADF00D );
	TEMPER_CHECK_TRUE_A( Hashmap_InternalGetHiPart( combined ) == hiPart );
	TEMPER_CHECK_TRUE_A( Hashmap_InternalGetLoPart( combined ) == loPart );
}

TEMPER_TEST_PARAMETRIC( Test_Hashmap_Create, TEMPER_FLAG_SHOULD_RUN, hashmap_t **hashmap, const u32 count ) {
	TEMPER_CHECK_TRUE( hashmap );
	TEMPER_CHECK_TRUE( !*hashmap );

	TEMPER_CHECK_TRUE( count );

	*hashmap = Hashmap_Create( count );

	TEMPER_CHECK_TRUE( ( *hashmap )->capacity == count );

	For ( u32, i, 0, ( *hashmap )->capacity ) {
		hashmapBucket_t& bucket = ( *hashmap )->buckets[i];
		TEMPER_CHECK_TRUE_A( Hashmap_InternalCombine( bucket.keyHi, bucket.keyLo ) == HASHMAP_UNUSED_BUCKET );
		TEMPER_CHECK_TRUE_A( bucket.value == HASHMAP_INVALID_VALUE );
	}
}

TEMPER_TEST_PARAMETRIC( Test_Hashmap_SetAndGetValue, TEMPER_FLAG_SHOULD_RUN, hashmap_t *hashmap, const char *name, const u32 age ) {
	TEMPER_CHECK_TRUE( hashmap );

	const u32 nameHash = Hash_32( name, strlen( name ), 0 );

	TEMPER_CHECK_TRUE( Hashmap_GetValue( hashmap, nameHash ) == HASHMAP_INVALID_VALUE );

	Hashmap_SetValue( hashmap, nameHash, age );

	TEMPER_CHECK_TRUE( Hashmap_GetValue( hashmap, nameHash ) == age );
	TEMPER_CHECK_TRUE( Hashmap_GetValue( hashmap, nameHash ) != HASHMAP_INVALID_VALUE );
}

TEMPER_TEST_PARAMETRIC( Test_Hashmap_Reset, TEMPER_FLAG_SHOULD_RUN, hashmap_t *hashmap ) {
	TEMPER_CHECK_TRUE( hashmap );

	u32 oldCount = hashmap->capacity;

	Hashmap_Reset( hashmap );

	TEMPER_CHECK_TRUE( hashmap->capacity == oldCount );

	For ( u32, i, 0, hashmap->capacity ) {
		hashmapBucket_t& bucket = hashmap->buckets[i];
		TEMPER_CHECK_TRUE_A( Hashmap_InternalCombine( bucket.keyHi, bucket.keyLo ) == HASHMAP_UNUSED_BUCKET );
		TEMPER_CHECK_TRUE_A( bucket.value == HASHMAP_INVALID_VALUE );
	}

	TEMPER_CHECK_TRUE_A( hashmap->usageCount == 0U );
	TEMPER_CHECK_TRUE_A( hashmap->tombstoneCount == 0U );
}

TEMPER_TEST_PARAMETRIC( Test_Hashmap_Remove, TEMPER_FLAG_SHOULD_RUN, hashmap_t *hashmap ) {
	TEMPER_CHECK_TRUE( hashmap );

	Hashmap_Reset( hashmap );

	u32 count = hashmap->capacity;
	u32 firstKey = count; 		// Bucket index % = 0
	u32 secondKey = count * 2;	// Bucket index % = 0
	u32 thirdKey = count + 1;	// Bucket index % = 1;

	u32 firstValue = 69;
	u32 secondValue = 70;
	u32 thirdValue = 90;

	Hashmap_SetValue( hashmap, firstKey, firstValue );	// Actual bucket pos 0
	TEMPER_CHECK_TRUE_A( Hashmap_InternalCombineAtIndex( hashmap, 0 ) == firstKey );
	Hashmap_SetValue( hashmap, secondKey, secondValue );	// Actual bucket pos 1 (wanted 0)
	TEMPER_CHECK_TRUE_A( Hashmap_InternalCombineAtIndex( hashmap, 1 ) == secondKey );
	Hashmap_SetValue( hashmap, thirdKey, thirdValue );	// Actual bucket pos 2 (wanted 1)
	TEMPER_CHECK_TRUE_A( Hashmap_InternalCombineAtIndex( hashmap, 2 ) == thirdKey );

	Hashmap_RemoveKey( hashmap, secondKey );

	// Check the tombstone from the second value is in place to allow proper gets
	TEMPER_CHECK_TRUE_A( Hashmap_GetValue( hashmap, thirdKey ) == thirdValue );

	Hashmap_RemoveKey( hashmap, thirdKey );
	// Check tombstones were removed
	TEMPER_CHECK_TRUE_A( Hashmap_InternalCombineAtIndex( hashmap, 2 ) == HASHMAP_UNUSED_BUCKET );
	TEMPER_CHECK_TRUE_A( Hashmap_InternalCombineAtIndex( hashmap, 1 ) == HASHMAP_UNUSED_BUCKET );

	Hashmap_SetValue( hashmap, thirdKey, thirdValue );

	// Check that add won't probe passed removed tombstone
	TEMPER_CHECK_TRUE_A( Hashmap_GetValue( hashmap, thirdKey ) == thirdValue );
	TEMPER_CHECK_TRUE_A( Hashmap_InternalCombineAtIndex( hashmap, 1 ) == thirdKey );
}

TEMPER_TEST_PARAMETRIC( Test_Hashmap_LinearProbeTelemetry, TEMPER_FLAG_SHOULD_RUN, u32 numberOfBuckets, float utilisation ) {
	hashmap_t* hashmap = Hashmap_Create( numberOfBuckets, utilisation, false );
	u64 hashSeed = 0x9E3779B97F4A7C15;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wimplicit-int-float-conversion"
	u32 intendedFill = static_cast<u32>( numberOfBuckets * utilisation );
#pragma GCC diagnostic pop

	auto getHashAtSequence = [&]( u32 sequence ) -> u64 {
		assert( sequence < hashmap->usageCount );

		u32 runningHashCount = -1U;
		u64 hash = HASHMAP_TOMBSTONE_BUCKET;

		For ( u32, keyIndex, 0, hashmap->capacity ) {
			u64 keyAtIndex = Hashmap_InternalCombineAtIndex( hashmap, TruncCast( u32, keyIndex ) );

			if ( keyAtIndex != HASHMAP_UNUSED_BUCKET && keyAtIndex != HASHMAP_TOMBSTONE_BUCKET ) {
				runningHashCount++;

				if ( runningHashCount == sequence ) {
					hash = keyAtIndex;
					break;
				}
			}
		}

		return hash;
	};

	//Setup test by bringing the utilisation up to utilitsation
	{
		u64 i = 0U;
		while ( hashmap->usageCount + hashmap->tombstoneCount < intendedFill ) {
			if ( i % 3 != 2 ) {
				// Two adds for one remove
				constexpr u32 LENGTH_OF_RANDOM_FLOATS = 16u;
				float randomData[LENGTH_OF_RANDOM_FLOATS];

				For ( u32, floatIndex, 0, LENGTH_OF_RANDOM_FLOATS ) {
					randomData[floatIndex] = RNG_GetFloat32( 0.f, 1.f );
				}

				u64 hash = Hash_64( randomData, sizeof( float ) * LENGTH_OF_RANDOM_FLOATS, hashSeed );

				Hashmap_SetValue( hashmap, hash, 69 );
			} else {
				u32 toRemove = Cast( u32, RNG_GetFloat32( 0, Cast( float32, hashmap->usageCount - 1 ) ) );
				u64 hash = getHashAtSequence( toRemove );

				Hashmap_RemoveKey( hashmap, hash );
			}

			i++;
		}
	}

	// get a bunch of random hashes and grab the results
	linearAllocator_t *probeAllocator = Mem_CreateAllocator( 1024 * 1024 );
	defer { Mem_DestroyAllocator( probeAllocator ); };

	Array<u32> linearProbeLength = {};
	linearProbeLength.Init( probeAllocator );

	linearProbeLength.Reserve( intendedFill );

	For ( u32, i, 0, hashmap->capacity ) {
		u64 hash = Hashmap_InternalCombineAtIndex( hashmap, TruncCast( u32, i ) );
		if ( hash == HASHMAP_TOMBSTONE_BUCKET || hash == HASHMAP_UNUSED_BUCKET ) {
			continue;
		}

		Hashmap_GetValue( hashmap, hash );
		linearProbeLength.Add( hashmap->lastLinearProbe );
	}

	// Analyze
	u32 biggest = 0U;
	float mean = 0.0f;
	float noneZeroMean = 0.0f;
	u32 numZeroProbes = 0U;
	For ( u64, i, 0, linearProbeLength.count ) {
		u32 probe = linearProbeLength[i];

		if ( probe > biggest ) {
			biggest = probe;
		}

		if ( probe == 0U ) {
			numZeroProbes++;
		} else {
			noneZeroMean += Cast( float32, probe );
		}

		mean += Cast( float32, probe );
		//Warning( "%d", probe );
	}

	mean = mean / Cast( float32, linearProbeLength.count );

	noneZeroMean = noneZeroMean / Cast( float32, linearProbeLength.count - numZeroProbes );

#if PRINT_HASHMAP_PROBE_RESULTS
	printf(
		"\n===\nPROBE RESULTS for %f pc utilization on %u buckets:\naverage probe length was %f, average of non zero was %f, biggest was %u. Num that were zero: %u\n Tombstone:Used: %u:%u\n",
		utilisation * 100.0f, numberOfBuckets, mean, noneZeroMean, biggest, numZeroProbes, hashmap->tombstoneCount, hashmap->usageCount
	);
#endif

	Hashmap_Destroy( hashmap );
}

static hashmap_t* g_hashmap = NULL;

TEMPER_INVOKE_PARAMETRIC_TEST( Test_Hashmap_Create, &g_hashmap, 10 );

TEMPER_INVOKE_PARAMETRIC_TEST( Test_Hashmap_SetAndGetValue, g_hashmap, "Dan",     27 );
TEMPER_INVOKE_PARAMETRIC_TEST( Test_Hashmap_SetAndGetValue, g_hashmap, "Tom",     27 ); //Don't forget your buddy :) -- never <3
TEMPER_INVOKE_PARAMETRIC_TEST( Test_Hashmap_SetAndGetValue, g_hashmap, "Mum",     53 );
TEMPER_INVOKE_PARAMETRIC_TEST( Test_Hashmap_SetAndGetValue, g_hashmap, "Dad",     62 );
TEMPER_INVOKE_PARAMETRIC_TEST( Test_Hashmap_SetAndGetValue, g_hashmap, "Grandad", 89 );

TEMPER_INVOKE_PARAMETRIC_TEST( Test_Hashmap_Reset, g_hashmap );

TEMPER_INVOKE_PARAMETRIC_TEST( Test_Hashmap_Remove, g_hashmap );

TEMPER_TEST( Test_Hashmap_Growing, TEMPER_FLAG_SHOULD_RUN ) {
	hashmap_t* map = Hashmap_Create( 10, 0.5f, true );
	u64 key = 10;
	u32 value = 10;

	Hashmap_SetValue( map, key++, value++ );
	Hashmap_SetValue( map, key++, value++ );
	Hashmap_SetValue( map, key++, value++ );
	Hashmap_SetValue( map, key++, value++ );
	Hashmap_SetValue( map, key++, value++ );
	// Should caust a regrow
	Hashmap_SetValue( map, key++, value++ );
	TEMPER_CHECK_TRUE_A( map->capacity == 15 );

	// In the new map, 10 is a value bucket index
	TEMPER_CHECK_TRUE( Hashmap_InternalCombineAtIndex( map, 10 ) == 10 );
	// 15 isn't so it should wrap around
	TEMPER_CHECK_TRUE( Hashmap_InternalCombineAtIndex( map, 0 ) == 15 );
}

TEMPER_INVOKE_PARAMETRIC_TEST( Test_Hashmap_LinearProbeTelemetry, 10000, 0.75f );
TEMPER_INVOKE_PARAMETRIC_TEST( Test_Hashmap_LinearProbeTelemetry, 10000, 0.5f  );
TEMPER_INVOKE_PARAMETRIC_TEST( Test_Hashmap_LinearProbeTelemetry, 10000, 0.3f  );
TEMPER_INVOKE_PARAMETRIC_TEST( Test_Hashmap_LinearProbeTelemetry, 10000, 0.1f  );


/*
================================================================================================

	File

================================================================================================
*/

TEMPER_TEST_PARAMETRIC( Test_File_Exists, TEMPER_FLAG_SHOULD_RUN, const char *filename, const bool8 expected ) {
	TEMPER_CHECK_TRUE( FS_FileExists( filename ) == expected );
}

TEMPER_INVOKE_PARAMETRIC_TEST( Test_File_Exists, "build.cpp",                true  );
TEMPER_INVOKE_PARAMETRIC_TEST( Test_File_Exists, "include/file.h",           true  );
TEMPER_INVOKE_PARAMETRIC_TEST( Test_File_Exists, "include/paths.h",          true  );
TEMPER_INVOKE_PARAMETRIC_TEST( Test_File_Exists, "src/linear_allocator.cpp", true  );
TEMPER_INVOKE_PARAMETRIC_TEST( Test_File_Exists, "this_file_does_not_exist", false );
TEMPER_INVOKE_PARAMETRIC_TEST( Test_File_Exists, "include/fake_header.h",    false );
TEMPER_INVOKE_PARAMETRIC_TEST( Test_File_Exists, "src/fake_source_file.cpp", false );

static file_t		g_testFile				= {};
static file_t		g_testFile2				= {};
static const char	*g_testFolderPath		= "bin/debug/core_test_folder";
static const char	*g_testSubfolderPath	= "bin/debug/core_test_folder/subdir";
static const char	*g_testFilePath			= "bin/debug/core_test_folder/core_test_file.txt";
static const char	*g_testFileEntirePath	= "bin/debug/core_test_folder/core_test_file_entire.txt";
static const char	*g_testFileRenamedPath	= "bin/debug/core_test_folder/core_test_file_renamed.txt";
static const char	*g_testFileCopyPath		= "bin/debug/core_test_folder/core_test_file_copy.txt";
static const char	*g_testSubdirFilePath	= "bin/debug/core_test_folder/subdir/subdir_file.txt";

static void CountVisitedFiles( const fileInfo_t *fileInfo, void *userData ) {
	Unused( fileInfo );
	u32 *count = Cast( u32 *, userData );
	*count += 1;
}

TEMPER_TEST_PARAMETRIC( Test_Folder_Create, TEMPER_FLAG_SHOULD_RUN, const char *path ) {
	TEMPER_CHECK_TRUE_A( FS_CreateFolderIfItDoesntExist( path ) );
	TEMPER_CHECK_TRUE( FS_FolderExists( path ) );
}

TEMPER_TEST_PARAMETRIC( Test_Folder_Delete, TEMPER_FLAG_SHOULD_RUN, const char *path ) {
	TEMPER_CHECK_TRUE_A( FS_DeleteFolder( path ) );
	TEMPER_CHECK_TRUE( !FS_FolderExists( path ) );
}

TEMPER_TEST_PARAMETRIC( Test_File_Open, TEMPER_FLAG_SHOULD_RUN, file_t *file, const char *filename ) {
	*file = FS_OpenFile( filename, FILE_OPEN_READ );
	TEMPER_CHECK_TRUE_A( file->handle != INVALID_FILE_HANDLE );
}

TEMPER_TEST_PARAMETRIC( Test_File_Close, TEMPER_FLAG_SHOULD_RUN, file_t *file ) {
	TEMPER_CHECK_TRUE( FS_CloseFile( file ) );
}

TEMPER_TEST_PARAMETRIC( Test_File_OpenOrCreate, TEMPER_FLAG_SHOULD_RUN, file_t *file, const char *filename ) {
	*file = FS_OpenOrCreateFile( filename );
	TEMPER_CHECK_TRUE_A( file->handle != INVALID_FILE_HANDLE );
}

TEMPER_TEST_PARAMETRIC( Test_File_Write, TEMPER_FLAG_SHOULD_RUN, file_t *file, const char *content ) {
	TEMPER_CHECK_TRUE_A( FS_WriteFile( file, content ) );
}

TEMPER_TEST_PARAMETRIC( Test_File_WriteBuffer, TEMPER_FLAG_SHOULD_RUN, file_t *file, const void *data, const u64 size ) {
	TEMPER_CHECK_TRUE_A( FS_WriteFile( file, data, size ) );
}

TEMPER_TEST_PARAMETRIC( Test_File_WriteAtOffset, TEMPER_FLAG_SHOULD_RUN, file_t *file, const void *data, const u64 offset, const u64 size ) {
	TEMPER_CHECK_TRUE_A( FS_WriteFile( file, data, offset, size ) );
}

TEMPER_TEST_PARAMETRIC( Test_File_WriteLine, TEMPER_FLAG_SHOULD_RUN, file_t *file, const char *content ) {
	TEMPER_CHECK_TRUE_A( FS_WriteFileLine( file, content ) );
}

TEMPER_TEST_PARAMETRIC( Test_File_WriteEntire, TEMPER_FLAG_SHOULD_RUN, const char *filename, const void *data, const u64 size ) {
	TEMPER_CHECK_TRUE_A( FS_WriteEntireFile( filename, data, size ) );
	TEMPER_CHECK_TRUE( FS_FileExists( filename ) );
}

TEMPER_TEST_PARAMETRIC( Test_File_GetSize, TEMPER_FLAG_SHOULD_RUN, const char *filename, const u64 expectedSize ) {
	u64 size = 0;
	TEMPER_CHECK_TRUE_A( FS_GetFileSize( filename, &size ) );
	TEMPER_CHECK_TRUE( size == expectedSize );
}

TEMPER_TEST_PARAMETRIC( Test_File_GetLastWriteTime, TEMPER_FLAG_SHOULD_RUN, const char *filename ) {
	u64 writeTime = 0;
	TEMPER_CHECK_TRUE_A( FS_GetFileLastWriteTime( filename, &writeTime ) );
	TEMPER_CHECK_TRUE( writeTime != 0 );
}

TEMPER_TEST_PARAMETRIC( Test_File_Read, TEMPER_FLAG_SHOULD_RUN, file_t *file, const u64 offset, const u64 size, const char *expected ) {
	char *buffer = Cast( char *, Mem_TempAlloc( ( size + 1 ) * sizeof( char ) ) );
	buffer[size] = 0;

	TEMPER_CHECK_TRUE_A( FS_ReadFile( file, offset, size, buffer ) );
	TEMPER_CHECK_TRUE( Str_Equals( buffer, expected ) );

	Mem_ResetTempStorage();
}

TEMPER_TEST_PARAMETRIC( Test_File_ReadSequential, TEMPER_FLAG_SHOULD_RUN, file_t *file, const u64 size, const char *expected ) {
	char *buffer = Cast( char *, Mem_TempAlloc( ( size + 1 ) * sizeof( char ) ) );
	buffer[size] = 0;

	TEMPER_CHECK_TRUE_A( FS_ReadFile( file, size, buffer ) );
	TEMPER_CHECK_TRUE( Str_Equals( buffer, expected ) );

	Mem_ResetTempStorage();
}

TEMPER_TEST_PARAMETRIC( Test_File_ReadEntire, TEMPER_FLAG_SHOULD_RUN, const char *filename, const char *expected, const u64 expectedLength ) {
	char *buffer = NULL;
	u64 length = 0;

	TEMPER_CHECK_TRUE_A( FS_ReadEntireFile( filename, &buffer, &length ) );
	TEMPER_CHECK_TRUE( length == expectedLength );
	TEMPER_CHECK_TRUE( Str_Equals( buffer, expected ) );

	FS_FreeFileBuffer( &buffer );
	TEMPER_CHECK_TRUE( buffer == NULL );
}

TEMPER_TEST_PARAMETRIC( Test_File_Copy, TEMPER_FLAG_SHOULD_RUN, const char *src, const char *dst ) {
	TEMPER_CHECK_TRUE_A( FS_CopyFile( src, dst ) );
	TEMPER_CHECK_TRUE( FS_FileExists( dst ) );
}

TEMPER_TEST_PARAMETRIC( Test_File_GetAllFilesInFolder, TEMPER_FLAG_SHOULD_RUN, const char *path, const fileVisitFlags_t visitFlags, const u32 expectedCount ) {
	u32 count = 0;
	TEMPER_CHECK_TRUE_A( FS_GetAllFilesInFolder( path, visitFlags, CountVisitedFiles, &count ) );
	TEMPER_CHECK_TRUE( count == expectedCount );
}

TEMPER_TEST_PARAMETRIC( Test_File_Rename, TEMPER_FLAG_SHOULD_RUN, file_t *file, const char *oldPath, const char *newPath ) {
	TEMPER_CHECK_TRUE_A( FS_CloseFile( file ) );
	TEMPER_CHECK_TRUE_A( FS_RenameFile( oldPath, newPath ) );
	TEMPER_CHECK_TRUE( FS_FileExists( newPath ) );
	TEMPER_CHECK_TRUE( !FS_FileExists( oldPath ) );
}

TEMPER_TEST_PARAMETRIC( Test_File_Delete, TEMPER_FLAG_SHOULD_RUN, const char *filename ) {
	TEMPER_CHECK_TRUE_A( FS_DeleteFile( filename ) );
	TEMPER_CHECK_TRUE( !FS_FileExists( filename ) );
}

// folder: create
TEMPER_INVOKE_PARAMETRIC_TEST( Test_Folder_Create, g_testFolderPath );

// create
TEMPER_INVOKE_PARAMETRIC_TEST( Test_File_OpenOrCreate, &g_testFile, g_testFilePath );

// write string content: "Hello, world!Goodbye, world!\n" = 29 bytes
TEMPER_INVOKE_PARAMETRIC_TEST( Test_File_Write,      &g_testFile, "Hello, world!" );
TEMPER_INVOKE_PARAMETRIC_TEST( Test_File_WriteLine, &g_testFile, "Goodbye, world!\n" );

// write entire (creates a second file: "Written entirely" = 16 bytes)
TEMPER_INVOKE_PARAMETRIC_TEST( Test_File_WriteEntire, g_testFileEntirePath, "Written entirely", 16 );

// size
TEMPER_INVOKE_PARAMETRIC_TEST( Test_File_GetSize, g_testFilePath,        30 );
TEMPER_INVOKE_PARAMETRIC_TEST( Test_File_GetSize, g_testFileEntirePath,  16 );

// last write time
TEMPER_INVOKE_PARAMETRIC_TEST( Test_File_GetLastWriteTime, g_testFilePath );

// read with explicit offset
TEMPER_INVOKE_PARAMETRIC_TEST( Test_File_Read, &g_testFile, 0,  13, "Hello, world!"   );
TEMPER_INVOKE_PARAMETRIC_TEST( Test_File_Read, &g_testFile, 13, 15, "Goodbye, world!" );

// open a fresh read-only handle, do sequential reads, then close it
TEMPER_INVOKE_PARAMETRIC_TEST( Test_File_Open,            &g_testFile2, g_testFilePath        );
TEMPER_INVOKE_PARAMETRIC_TEST( Test_File_ReadSequential, &g_testFile2, 13, "Hello, world!"   );
TEMPER_INVOKE_PARAMETRIC_TEST( Test_File_ReadSequential, &g_testFile2, 15, "Goodbye, world!" );
TEMPER_INVOKE_PARAMETRIC_TEST( Test_File_Close,           &g_testFile2                        );

// read entire
TEMPER_INVOKE_PARAMETRIC_TEST( Test_File_ReadEntire, g_testFileEntirePath, "Written entirely", 16 );

// write buffer: at g_testFile's current offset, overwrites "\n" with "!"
// file: "Hello, world!Goodbye, world!\!" (30 bytes)
TEMPER_INVOKE_PARAMETRIC_TEST( Test_File_WriteBuffer, &g_testFile, "!", 1 );
TEMPER_INVOKE_PARAMETRIC_TEST( Test_File_Read,         &g_testFile, 27, 1, "!" );

// write at offset: patch "world" at offset 7 with "WORLD"
// file: "Hello, WORLD!Goodbye, world!\!" (30 bytes)
TEMPER_INVOKE_PARAMETRIC_TEST( Test_File_WriteAtOffset, &g_testFile, "WORLD", 7, 5 );
TEMPER_INVOKE_PARAMETRIC_TEST( Test_File_Read,            &g_testFile, 7, 5, "WORLD"  );

// copy
TEMPER_INVOKE_PARAMETRIC_TEST( Test_File_Copy, g_testFilePath, g_testFileCopyPath );

// subfolder: create and populate for folder visit tests
TEMPER_INVOKE_PARAMETRIC_TEST( Test_Folder_Create,       g_testSubfolderPath                    );
TEMPER_INVOKE_PARAMETRIC_TEST( Test_File_OpenOrCreate, &g_testFile2, g_testSubdirFilePath      );
TEMPER_INVOKE_PARAMETRIC_TEST( Test_File_Close,          &g_testFile2                            );

// folder: visit - files only in root (3: original, entire, copy)
TEMPER_INVOKE_PARAMETRIC_TEST( Test_File_GetAllFilesInFolder, g_testFolderPath, FILE_VISIT_FILES,   3 );

// folder: visit - folders only in root (1: subdir)
TEMPER_INVOKE_PARAMETRIC_TEST( Test_File_GetAllFilesInFolder, g_testFolderPath, FILE_VISIT_FOLDERS, 1 );

// folder: visit - files recursively (3 root + 1 subdir = 4)
TEMPER_INVOKE_PARAMETRIC_TEST( Test_File_GetAllFilesInFolder, g_testFolderPath, Cast( fileVisitFlags_t, FILE_VISIT_FILES | FILE_VISIT_RECURSIVE ), 4 );

// folder: visit - files + folders recursively (4 files + 1 folder = 5)
TEMPER_INVOKE_PARAMETRIC_TEST( Test_File_GetAllFilesInFolder, g_testFolderPath, Cast( fileVisitFlags_t, FILE_VISIT_FILES | FILE_VISIT_FOLDERS | FILE_VISIT_RECURSIVE ), 5 );

// cleanup: subfolder file then subfolder
TEMPER_INVOKE_PARAMETRIC_TEST( Test_File_Delete,   g_testSubdirFilePath );
TEMPER_INVOKE_PARAMETRIC_TEST( Test_Folder_Delete, g_testSubfolderPath  );

// rename
TEMPER_INVOKE_PARAMETRIC_TEST( Test_File_Rename, &g_testFile, g_testFilePath, g_testFileRenamedPath );

// delete files
TEMPER_INVOKE_PARAMETRIC_TEST( Test_File_Delete, g_testFileRenamedPath );
TEMPER_INVOKE_PARAMETRIC_TEST( Test_File_Delete, g_testFileCopyPath    );
TEMPER_INVOKE_PARAMETRIC_TEST( Test_File_Delete, g_testFileEntirePath  );

// folder: delete
TEMPER_INVOKE_PARAMETRIC_TEST( Test_Folder_Delete, g_testFolderPath );


/*
================================================================================================

	Paths

================================================================================================
*/

TEMPER_TEST( Test_Path_AppPath, TEMPER_FLAG_SHOULD_RUN ) {
	const char *appPath = Path_AppPath();

	TEMPER_CHECK_TRUE( appPath != NULL );
	TEMPER_CHECK_TRUE( Path_IsAbsolute( appPath ) );
#if defined( _WIN32 )
	TEMPER_CHECK_TRUE( Str_EndsWith( appPath, "core-tests.exe" ) );
#elif defined( __linux__ )
	TEMPER_CHECK_TRUE( Str_EndsWith( appPath, "core-tests" ) );
#endif

	Mem_ResetTempStorage();
}

TEMPER_TEST( Test_Path_CurrentWorkingDirectoryMatchesAbsoluteDot, TEMPER_FLAG_SHOULD_RUN ) {
	const char *cwd = Path_CurrentWorkingDirectory();
	const char *absoluteDot = Path_AbsolutePath( "." );

	TEMPER_CHECK_TRUE( Str_Equals( cwd, absoluteDot ) );

	Mem_ResetTempStorage();
}

TEMPER_TEST_PARAMETRIC( Test_Path_CurrentWorkingDirectorySetThenGet, TEMPER_FLAG_SHOULD_RUN, const char *folder ) {
	const char *originalCwd = Path_CurrentWorkingDirectory();
	defer { Path_SetCurrentDirectory( originalCwd ); };

	const char *knownPath = Path_AbsolutePath( folder );
	Path_SetCurrentDirectory( knownPath );

	const char *cwd = Path_CurrentWorkingDirectory();
	TEMPER_CHECK_TRUE( Str_Equals( knownPath, cwd ) );

	Mem_ResetTempStorage();
}

TEMPER_INVOKE_PARAMETRIC_TEST( Test_Path_CurrentWorkingDirectorySetThenGet, "bin"            );
TEMPER_INVOKE_PARAMETRIC_TEST( Test_Path_CurrentWorkingDirectorySetThenGet, ".builder"       );
TEMPER_INVOKE_PARAMETRIC_TEST( Test_Path_CurrentWorkingDirectorySetThenGet, "editor_support" );

TEMPER_TEST_PARAMETRIC( Test_Path_AbsolutePathFromRelative, TEMPER_FLAG_SHOULD_RUN, const char *relativePath ) {
	const char *absolute = Path_AbsolutePath( relativePath );

	TEMPER_CHECK_TRUE( Path_IsAbsolute( absolute ) );

	Mem_ResetTempStorage();
}

TEMPER_INVOKE_PARAMETRIC_TEST( Test_Path_AbsolutePathFromRelative, "."              );
TEMPER_INVOKE_PARAMETRIC_TEST( Test_Path_AbsolutePathFromRelative, "bin"            );
TEMPER_INVOKE_PARAMETRIC_TEST( Test_Path_AbsolutePathFromRelative, "src"            );
TEMPER_INVOKE_PARAMETRIC_TEST( Test_Path_AbsolutePathFromRelative, "include"        );
TEMPER_INVOKE_PARAMETRIC_TEST( Test_Path_AbsolutePathFromRelative, "editor_support" );

TEMPER_TEST( Test_Path_AbsolutePathAlreadyAbsolute, TEMPER_FLAG_SHOULD_RUN ) {
	const char *cwd = Path_CurrentWorkingDirectory();
	const char *result = Path_AbsolutePath( cwd );

	TEMPER_CHECK_TRUE( Str_Equals( cwd, result ) );

	Mem_ResetTempStorage();
}

TEMPER_TEST_PARAMETRIC( Test_Path_RemoveFileFromPath, TEMPER_FLAG_SHOULD_RUN, const char *path, const char *expectedPathWithoutFile ) {
	const char *actualPathWithoutFile = Path_RemoveFileFromPath( path );

	if ( expectedPathWithoutFile == NULL ) {
		TEMPER_CHECK_TRUE( expectedPathWithoutFile == NULL && actualPathWithoutFile == NULL );
	} else {
		TEMPER_CHECK_TRUE( Str_Equals( expectedPathWithoutFile, actualPathWithoutFile ) );
	}

	Mem_ResetTempStorage();
}

TEMPER_INVOKE_PARAMETRIC_TEST( Test_Path_RemoveFileFromPath, "/usr/bin/cat.jpg",                   "/usr/bin"              );
TEMPER_INVOKE_PARAMETRIC_TEST( Test_Path_RemoveFileFromPath, "./some/path/program.d/settings.cfg", "./some/path/program.d" );
TEMPER_INVOKE_PARAMETRIC_TEST( Test_Path_RemoveFileFromPath, "./script.sh",                        "."                     );
TEMPER_INVOKE_PARAMETRIC_TEST( Test_Path_RemoveFileFromPath, "game.exe",                           NULL                    );
TEMPER_INVOKE_PARAMETRIC_TEST( Test_Path_RemoveFileFromPath, "file",                               NULL                    );

TEMPER_TEST_PARAMETRIC( Test_Path_RemovePathFromFile, TEMPER_FLAG_SHOULD_RUN, const char *path, const char *expectedFileWithoutPath ) {
	const char *actualFileWithoutPath = Path_RemovePathFromFile( path );

	TEMPER_CHECK_TRUE( Str_Equals( expectedFileWithoutPath, actualFileWithoutPath ) );
}

TEMPER_INVOKE_PARAMETRIC_TEST( Test_Path_RemovePathFromFile, "/usr/bin/cat.jpg",                   "cat.jpg"      );
TEMPER_INVOKE_PARAMETRIC_TEST( Test_Path_RemovePathFromFile, "./some/path/program.d/settings.cfg", "settings.cfg" );
TEMPER_INVOKE_PARAMETRIC_TEST( Test_Path_RemovePathFromFile, "./script.sh",                        "script.sh"    );
TEMPER_INVOKE_PARAMETRIC_TEST( Test_Path_RemovePathFromFile, "game.exe",                           "game.exe"     );
TEMPER_INVOKE_PARAMETRIC_TEST( Test_Path_RemovePathFromFile, "file",                               "file"         );

TEMPER_TEST_PARAMETRIC( Test_Path_RemoveFileExtension, TEMPER_FLAG_SHOULD_RUN, const char *fileWithExtension, const char *expectedFileWithoutExtension ) {
	const char *actualFileWithoutExtension = Path_RemoveFileExtension( fileWithExtension );

	TEMPER_CHECK_TRUE( Str_Equals( expectedFileWithoutExtension, actualFileWithoutExtension ) );

	Mem_ResetTempStorage();
}

TEMPER_INVOKE_PARAMETRIC_TEST( Test_Path_RemoveFileExtension, "test.txt",                    "test"                    );
TEMPER_INVOKE_PARAMETRIC_TEST( Test_Path_RemoveFileExtension, "cat.jpg",                     "cat"                     );
TEMPER_INVOKE_PARAMETRIC_TEST( Test_Path_RemoveFileExtension, "dog.fw.png",                  "dog.fw"                  );
TEMPER_INVOKE_PARAMETRIC_TEST( Test_Path_RemoveFileExtension, "doom/src/r_bsp.c",            "doom/src/r_bsp"          );
TEMPER_INVOKE_PARAMETRIC_TEST( Test_Path_RemoveFileExtension, "home/program.d/settings.cfg", "home/program.d/settings" );
TEMPER_INVOKE_PARAMETRIC_TEST( Test_Path_RemoveFileExtension, "docs/diary",                  "docs/diary"              );

TEMPER_TEST_PARAMETRIC( Test_Path_IsAbsolute, TEMPER_FLAG_SHOULD_RUN, const char *path, const bool8 shouldBeAbsolute ) {
	TEMPER_CHECK_TRUE( Path_IsAbsolute( path ) == shouldBeAbsolute );
}

#if defined( __linux__ )
TEMPER_INVOKE_PARAMETRIC_TEST( Test_Path_IsAbsolute, "/usr/bin/",  true  );
TEMPER_INVOKE_PARAMETRIC_TEST( Test_Path_IsAbsolute, "./usr/bin/", false );
TEMPER_INVOKE_PARAMETRIC_TEST( Test_Path_IsAbsolute, "usr/bin/",   false );
TEMPER_INVOKE_PARAMETRIC_TEST( Test_Path_IsAbsolute, "\\usr/bin/", false );
TEMPER_INVOKE_PARAMETRIC_TEST( Test_Path_IsAbsolute, ".usr/bin/",  false );
#elif defined( _WIN32 )
TEMPER_INVOKE_PARAMETRIC_TEST( Test_Path_IsAbsolute, "C:/Program Files (x86)/Steam/steamapps/common", true  );
TEMPER_INVOKE_PARAMETRIC_TEST( Test_Path_IsAbsolute, "C/Program Files (x86)/Steam/steamapps/common",  false );
TEMPER_INVOKE_PARAMETRIC_TEST( Test_Path_IsAbsolute, ":/Program Files (x86)/Steam/steamapps/common",  false );
TEMPER_INVOKE_PARAMETRIC_TEST( Test_Path_IsAbsolute, "./Program Files (x86)/Steam/steamapps/common",  false );
TEMPER_INVOKE_PARAMETRIC_TEST( Test_Path_IsAbsolute, "/Program Files (x86)/Steam/steamapps/common",   false );
#endif

TEMPER_TEST_PARAMETRIC( Test_Path_FixSlashes, TEMPER_FLAG_SHOULD_RUN, const char *path ) {
#ifdef __linux__
	const char *expectedFixedPath = Str_Replace( path, '\\', '/' );
#else
	const char *expectedFixedPath = Str_Replace( path, '/', '\\' );
#endif

	const char *actualFixedPath = Path_FixSlashes( path );

	TEMPER_CHECK_TRUE( Str_Equals( expectedFixedPath, actualFixedPath ) );

	Mem_ResetTempStorage();
}

TEMPER_INVOKE_PARAMETRIC_TEST( Test_Path_FixSlashes, "C:/Users/dan\\Documents\\" );
TEMPER_INVOKE_PARAMETRIC_TEST( Test_Path_FixSlashes, "C:/Users/dan/\\Documents" );
TEMPER_INVOKE_PARAMETRIC_TEST( Test_Path_FixSlashes, "/usr/bin/program" );
TEMPER_INVOKE_PARAMETRIC_TEST( Test_Path_FixSlashes, "cat.jpg" );
TEMPER_INVOKE_PARAMETRIC_TEST( Test_Path_FixSlashes, "./" );

TEMPER_TEST_PARAMETRIC( Test_Path_GetRelativePath, TEMPER_FLAG_SHOULD_SKIP, const char *from, const char *to, const char *expectedRelativePath ) {
	const char *actualRelativePath = Path_RelativePathTo( from, to );

	TEMPER_CHECK_TRUE( Str_Equals( expectedRelativePath, actualRelativePath ) );
}

TEMPER_INVOKE_PARAMETRIC_TEST( Test_Path_GetRelativePath, "/home/docs/diary.txt", "/home/images/cat.jpg", "../images/cat.jpg" );
TEMPER_INVOKE_PARAMETRIC_TEST( Test_Path_GetRelativePath, "C:/Users/Dan/Documents/diary.txt", "C:/Users/Tom/Pictures/cats.jpg", "../../Tom/Pictures/cats.jpg" );

TEMPER_TEST( Test_Path_Join, TEMPER_FLAG_SHOULD_RUN ) {
#ifdef _WIN32
	const char *expectedPath = "C:\\Users\\your_mother\\videos";
#else
	const char *expectedPath = "C:/Users/your_mother/videos";
#endif

	const char *actualPath = Path_Join( "C:", "Users", "your_mother", "videos" );

	TEMPER_CHECK_TRUE( Str_Equals( expectedPath, actualPath ) );
}


/*
================================================================================================

	String Builder

================================================================================================
*/

TEMPER_TEST( Test_StringBuilder, TEMPER_FLAG_SHOULD_RUN ) {
	stringBuilder_t builder = {};
	StringBuilder_Init( &builder, g_tempStorage );

	TEMPER_CHECK_TRUE( builder.head == NULL );
	TEMPER_CHECK_TRUE( builder.tail == NULL );

	StringBuilder_Appendf( &builder, "this " );
	StringBuilder_Appendf( &builder, "is" );
	StringBuilder_Appendf( &builder, " only " );
	StringBuilder_Appendf( &builder, "a " );
	StringBuilder_Appendf( &builder, "test" );

	const char* actualString = StringBuilder_ToString( &builder );

	TEMPER_CHECK_TRUE( Str_Equals( actualString, "this is only a test" ) );

	Mem_ResetTempStorage();
}


/*
================================================================================================

	Random

================================================================================================
*/

// TODO: 23/12/2025: need tests that actually make sure the min and max values are actually hit when generating random numbers
// how do we test that?

TEMPER_TEST_PARAMETRIC( Test_Random_Float32WithinRange, TEMPER_FLAG_SHOULD_RUN, const float32 low, const float32 high ) {
	For ( u32, i, 0, 1000000 ) {
		float32 randomFloat = RNG_GetFloat32( low, high );

		TEMPER_CHECK_TRUE( randomFloat >= low );
		TEMPER_CHECK_TRUE( randomFloat <= high );
	}
}

TEMPER_INVOKE_PARAMETRIC_TEST( Test_Random_Float32WithinRange, 0.0f, 1.0f );
TEMPER_INVOKE_PARAMETRIC_TEST( Test_Random_Float32WithinRange, 1.0f, 2.0f );
TEMPER_INVOKE_PARAMETRIC_TEST( Test_Random_Float32WithinRange, 1.0f, 3.0f );
TEMPER_INVOKE_PARAMETRIC_TEST( Test_Random_Float32WithinRange, 2.0f, 3.0f );
TEMPER_INVOKE_PARAMETRIC_TEST( Test_Random_Float32WithinRange, 1.0f, 10.0f );
TEMPER_INVOKE_PARAMETRIC_TEST( Test_Random_Float32WithinRange, 100.0f, 150.0f );
TEMPER_INVOKE_PARAMETRIC_TEST( Test_Random_Float32WithinRange, 100.0f, 250.0f );
TEMPER_INVOKE_PARAMETRIC_TEST( Test_Random_Float32WithinRange, 1000.0f, 2000.0f );
TEMPER_INVOKE_PARAMETRIC_TEST( Test_Random_Float32WithinRange, 0.0f, FLOAT32_MAX );


/*
================================================================================================

	Library

================================================================================================
*/

TEMPER_TEST( Test_Library_LoadGetSymbolAndUnloadAgain, TEMPER_FLAG_SHOULD_RUN ) {
	// load
#if defined( _WIN32 )
	const char *filename = "test_dll.dll";
#else
	const char *filename = "test_dll.so";
#endif

	library_t library = Library_Load( filename );
	TEMPER_CHECK_TRUE( library.ptr != NULL );

	// get symbol
	{
		typedef const char * ( *GetDLLNameFunc )( void );

		GetDLLNameFunc getDllNameFunc = Cast( GetDLLNameFunc, Library_GetSymbol( library, "get_dll_name" ) );
		TEMPER_CHECK_TRUE( getDllNameFunc );

		const char* dllName = getDllNameFunc();

		TEMPER_CHECK_TRUE( Str_Equals( dllName, TEST_DLL_NAME ) );
	}

	// unload
	TEMPER_CHECK_TRUE( Library_Unload( &library ) );
	TEMPER_CHECK_TRUE( library.ptr == NULL );
}


/*
================================================================================================

	Threads

================================================================================================
*/

static s32 ThreadBasicTestFunc( void* data ) {
	Unused( data );

	return 69;
}

TEMPER_TEST( Test_Thread_CreateAndDestroy, TEMPER_FLAG_SHOULD_RUN ) {
	thread_t thread = Thread_Create( ThreadBasicTestFunc, NULL );

	TEMPER_CHECK_TRUE( thread.ptr != NULL );

	s32 exitCode = Thread_Wait( &thread );

	s32 expectedExitCode = 69;
	TEMPER_CHECK_TRUE_M( exitCode == expectedExitCode, "Thread was expected to return with exit code %d actually returned with %d.\n", expectedExitCode, exitCode );

	Thread_Destroy( &thread );

	TEMPER_CHECK_TRUE( thread.ptr == NULL );
}

struct threadJob_t {
	atomic32_t	completed;
	const char	*msg;
};

struct scheduler_t {
	thread_t	threads[4];
	semaphore_t	sema;
	atomic32_t	jobsReadPos;
	atomic32_t	jobsWritePos;
	atomic32_t	numCompletedJobs;
	threadJob_t	jobs[1024];
	bool8		running;
};

struct threadContext_t {
	u32				logicalThreadIndex;
	scheduler_t		*scheduler;
};

static void AddThreadJob( scheduler_t *scheduler, const char *msg ) {
	u32 jobIndex = AtomicIncrement( &scheduler->jobsWritePos ) - 1;
	threadJob_t *job = &scheduler->jobs[jobIndex];
	job->completed = { 0 };
	job->msg = msg;

	Semaphore_Signal( &scheduler->sema );
}

static s32 ThreadJobFunc( void *data ) {
	threadContext_t *context = Cast( threadContext_t *, data );

	scheduler_t *scheduler = context->scheduler;

	while ( scheduler->running ) {
		u32 cachedReadPos = scheduler->jobsReadPos.value;
		if ( cachedReadPos < scheduler->jobsWritePos.value ) {
			u32 readPos = AtomicCompareExchange( &scheduler->jobsReadPos, cachedReadPos, cachedReadPos + 1 );

			if ( readPos == cachedReadPos ) {
				threadJob_t* job = &scheduler->jobs[readPos];

				Sleep( 100 );

				char msg[1024] = {};

				const char* threadNumStr = NULL;
				switch ( context->logicalThreadIndex ) {
					case 0: threadNumStr = "0"; break;
					case 1: threadNumStr = "1"; break;
					case 2: threadNumStr = "2"; break;
					case 3: threadNumStr = "3"; break;
				}

				strcat( msg, "Thread " );
				strcat( msg, threadNumStr );
				strcat( msg, ": " );
				strcat( msg, job->msg );

				OutputDebugString( msg );

				AtomicIncrement( &job->completed );
				AtomicIncrement( &scheduler->numCompletedJobs );
			}
		} else {
			Semaphore_Wait( &scheduler->sema );
		}
	}

	// deliberately set to this value
	// when we shut the scheduler down we want to test that the threads actually shut down like they were supposed to
	return 42;
}

TEMPER_TEST( Test_Thread_Pool, TEMPER_FLAG_SHOULD_RUN ) {
	scheduler_t scheduler = {};
	scheduler.running = true;
	Semaphore_Create( &scheduler.sema );

	threadContext_t contexts[4] = {};

	memset( scheduler.threads, 0, CountOf( scheduler.threads ) * sizeof( thread_t ) );

	For ( u32, threadIndex, 0, CountOf( scheduler.threads ) ) {
		contexts[threadIndex].scheduler = &scheduler;
		contexts[threadIndex].logicalThreadIndex = threadIndex;

		scheduler.threads[threadIndex] = Thread_Create( ThreadJobFunc, &contexts[threadIndex] );

		TEMPER_CHECK_TRUE( scheduler.threads[threadIndex].ptr != NULL );
	}

	AddThreadJob( &scheduler, "Job A0\n" );
	AddThreadJob( &scheduler, "Job A1\n" );
	AddThreadJob( &scheduler, "Job A2\n" );
	AddThreadJob( &scheduler, "Job A3\n" );
	AddThreadJob( &scheduler, "Job A4\n" );
	AddThreadJob( &scheduler, "Job A5\n" );
	AddThreadJob( &scheduler, "Job A6\n" );
	AddThreadJob( &scheduler, "Job A7\n" );

	//while ( scheduler.numCompletedJobs.value != 8 );
	Sleep( 1000 );

	TEMPER_CHECK_TRUE( scheduler.numCompletedJobs.value == 8 );
	TEMPER_CHECK_TRUE( scheduler.jobsReadPos.value == scheduler.jobsWritePos.value );

	For ( u32, jobIndex, 0, scheduler.numCompletedJobs.value ) {
		TEMPER_CHECK_TRUE( scheduler.jobs[jobIndex].completed.value == 1 );
	}

	AddThreadJob( &scheduler, "Job B0\n" );
	AddThreadJob( &scheduler, "Job B1\n" );
	AddThreadJob( &scheduler, "Job B2\n" );
	AddThreadJob( &scheduler, "Job B3\n" );
	AddThreadJob( &scheduler, "Job B4\n" );
	AddThreadJob( &scheduler, "Job B5\n" );
	AddThreadJob( &scheduler, "Job B6\n" );
	AddThreadJob( &scheduler, "Job B7\n" );

	//while ( scheduler.numCompletedJobs.value != 16 );
	Sleep( 1000 );

	TEMPER_CHECK_TRUE( scheduler.numCompletedJobs.value == 16 );
	TEMPER_CHECK_TRUE( scheduler.jobsReadPos.value == scheduler.jobsWritePos.value );

	For ( u32, jobIndex, 0, scheduler.numCompletedJobs.value ) {
		TEMPER_CHECK_TRUE( scheduler.jobs[jobIndex].completed.value == 1 );
	}

	// all threads could be sleeping because the work is done
	// so allow them to shutdown, THEN wake them up
	scheduler.running = false;

	// each thread checks the semaphore, so calling this once will only wake up one thread
	// so we need to call this once per thread
	For ( u32, threadIndex, 0, CountOf( scheduler.threads ) ) {
		Semaphore_Signal( &scheduler.sema );
	}

	For ( u32, threadIndex, 0, CountOf( scheduler.threads ) ) {
		s32 exitCode = Thread_Wait( &scheduler.threads[threadIndex] );

		TEMPER_CHECK_TRUE( exitCode == 42 );

		Thread_Destroy( &scheduler.threads[threadIndex] );

		TEMPER_CHECK_TRUE( scheduler.threads[threadIndex].ptr == NULL );
	}

	Semaphore_Destroy( &scheduler.sema );
	TEMPER_CHECK_TRUE( scheduler.sema.ptr == NULL );
}


/*
================================================================================================

	Process

================================================================================================
*/

// TODO: DM: 23/12/2025: the API is fine, just write the tests


/*
================================================================================================

	Timer

	Leave these tests last because they each need you to wait for a fixed span of time.

================================================================================================
*/

TEMPER_TEST( Test_Timer_Seconds, TEMPER_FLAG_SHOULD_RUN ) {
	float64 start = TimeSeconds();

#ifdef _WIN32
	Sleep( 1000 );
#endif

	float64 end = TimeSeconds();

	TEMPER_CHECK_TRUE( ( end - start ) >= 1.0 );
	TEMPER_CHECK_TRUE( ( end - start ) <  1.1 );
}

TEMPER_TEST( Test_Timer_Milliseconds, TEMPER_FLAG_SHOULD_RUN ) {
	float64 start = TimeMS();

#ifdef _WIN32
	Sleep( 1000 );
#endif

	float64 end = TimeMS();

	TEMPER_CHECK_TRUE( ( end - start ) >= 1000.0 );
	TEMPER_CHECK_TRUE( ( end - start ) <  1100.0 );
}

TEMPER_TEST( Test_Timer_Microseconds, TEMPER_FLAG_SHOULD_RUN ) {
	float64 start = TimeUS();

#ifdef _WIN32
	Sleep( 1000 );
#endif

	float64 end = TimeUS();

	TEMPER_CHECK_TRUE( ( end - start ) >= 1000000.0 );
	TEMPER_CHECK_TRUE( ( end - start ) <  1100000.0 );
}

TEMPER_TEST( Test_Timer_Nanoseconds, TEMPER_FLAG_SHOULD_RUN ) {
	float64 start = TimeNS();

#ifdef _WIN32
	Sleep( 1000 );
#endif

	float64 end = TimeNS();

	TEMPER_CHECK_TRUE( ( end - start ) >= 1000000000.0 );
	TEMPER_CHECK_TRUE( ( end - start ) <  1100000000.0 );
}


//================================================================

#define TEST_PADDING "................................................................"

static void OnBeforeTest( const temperTestInfo_t* testInfo ) {
	const int padLengthMax = Cast( int, strlen( TEST_PADDING ) );

	const int dotLength = padLengthMax - Cast( int, strlen( testInfo->testNameStr ) );
	//assert( dotLength );	// DM!!! assert!

	printf( "%s %*.*s ", testInfo->testNameStr, dotLength, dotLength, TEST_PADDING );
}

static void OnAfterTest( const temperTestInfo_t* testInfo ) {
	Unused( testInfo );

	if ( testInfo->testingFlag == TEMPER_FLAG_SHOULD_SKIP ) {
		TemperSetTextColorInternal( TEMPERDEV_COLOR_YELLOW );
		printf( "SKIPPED\n" );
		TemperSetTextColorInternal( TEMPERDEV_COLOR_DEFAULT );
	} else {
		if ( g_temperTestContext.currentTestErrorCount == 0 ) {
			TemperSetTextColorInternal( TEMPERDEV_COLOR_GREEN );
			printf( "OK" );
			TemperSetTextColorInternal( TEMPERDEV_COLOR_DEFAULT );
		} else {
			TemperSetTextColorInternal( TEMPERDEV_COLOR_RED );
			printf( "FAILED" );
			TemperSetTextColorInternal( TEMPERDEV_COLOR_DEFAULT );
		}

		printf( " (%f %s)\n", testInfo->testTimeTaken, TemperGetTimeUnitStringInternal( g_temperTestContext.timeUnit ) );
	}
}

int main( int argc, char **argv ) {
	Mem_InitTempStorage( MEM_KILOBYTES( 8 ) );
	defer { Mem_ShutdownTempStorage(); };

	g_temperTestContext.callbacks.OnBeforeTest = OnBeforeTest;
	g_temperTestContext.callbacks.OnAfterTest = OnAfterTest;

	TEMPER_RUN( argc, argv );

	int exitCode = TEMPER_GET_EXIT_CODE();

#ifdef _DEBUG
	if ( exitCode != 0 ) {
		debug_break();
	}
#endif

	return exitCode;
}
