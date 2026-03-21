#pragma once

#include "paths.h"

#include "core_string.inl"

#if defined( __clang__ )
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wc++98-compat"
#endif

// Takes a variable number of strings and separates each one with a slash (back slash on Windows, forward slash on all other platforms).
template<class... Args>
inline const char *path_join( const char *first, Args... args ) {
	return string_join( PATH_SEPARATOR, false, false, first, args... );
}

#if defined( __clang__ )
#pragma clang diagnostic pop
#endif
