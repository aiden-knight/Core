#pragma once

#include "core_string.h"

#include "string_builder.h"
#include "defer.h"

#if defined( __clang__ )
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wc++98-compat"
#endif

template<class... Args>
inline const char *string_join( const char *separator, const bool8 separator_before, const bool8 separator_after, const char *first, Args... args ) {
	StringBuilder sb = {};
	string_builder_reset( &sb );
	defer { string_builder_destroy( &sb ); };

	if ( separator_before ) {
		string_builder_appendf( &sb, separator );
	}

	string_builder_appendf( &sb, first );
	( string_builder_appendf( &sb, "%s%s", separator, args ), ... );

	if ( separator_after ) {
		string_builder_appendf( &sb, separator );
	}

	return string_builder_to_string( &sb );
}

#if defined( __clang__ )
#pragma clang diagnostic pop
#endif
