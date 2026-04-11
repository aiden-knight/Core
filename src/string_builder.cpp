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

#include <string_builder.h>

#include <linear_allocator.h>
#include <debug.h>
#include <typecast.inl>

#include <stdio.h>
#include <stdarg.h>
#include <memory.h>
#include <string.h>

/*
================================================================================================

	String Builder

================================================================================================
*/

void StringBuilder_Init( stringBuilder_t *builder, linearAllocator_t *allocator ) {
	assert( builder );
	assert( allocator );

	builder->allocator = allocator;
	builder->head = NULL;
	builder->tail = NULL;
}

void StringBuilder_Appendf( stringBuilder_t *builder, const char *fmt, ... ) {
	assert( builder );
	assert( fmt );

	va_list args;
	va_start( args, fmt );

#if 0
	StringBuilder_Appendfv( builder, fmt, args );
#else
	stringBuilderBuffer_t *buffer = Cast( stringBuilderBuffer_t *, Mem_Alloc( builder->allocator, sizeof( stringBuilderBuffer_t ) ) );
	memset( buffer, 0, sizeof( stringBuilderBuffer_t ) );

	buffer->length = TruncCast( u32, vsnprintf( NULL, 0, fmt, args ) );

	buffer->data = Cast( char *, Mem_Alloc( builder->allocator, ( buffer->length + 1 ) * sizeof( char ), 1 ) );
	vsnprintf( buffer->data, buffer->length + 1, fmt, args );
	buffer->data[buffer->length] = 0;

	// if no head then this is the first element
	if ( !builder->head ) {
		builder->head = buffer;
		builder->tail = buffer;
	}

	builder->tail->next = buffer;
	builder->tail = buffer;
	builder->tail->next = NULL;
#endif

	va_end( args );
}

const char* StringBuilder_ToString( stringBuilder_t *builder ) {
	char* result = NULL;
	u64 totalLength = 0;
	u64 offset = 0;

	stringBuilderBuffer_t *current = builder->head;

	if ( !current ) {
		return NULL;
	}

	while ( current ) {
		totalLength += current->length;

		current = current->next;
	}

	totalLength += 1;

	result = Cast( char *, Mem_Alloc( builder->allocator, totalLength * sizeof( char ), 1 ) );

	current = builder->head;

	while ( current ) {
		strncpy( result + offset, current->data, current->length );

		offset += current->length;

		current = current->next;
	}

	result[totalLength - 1] = 0;

	return result;
}
