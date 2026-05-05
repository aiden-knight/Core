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

#include "test_exe.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

int main( int argc, char **argv ) {
	int exit_code = 0;

	for ( int arg_index = 1; arg_index < argc; arg_index++ ) {
		const char *arg = argv[arg_index];

		bool has_next_arg = arg_index + 1 < argc;

		const char *next_arg = ( has_next_arg ) ? argv[arg_index + 1] : NULL;

		if ( strcmp( arg, "--exit" ) == 0 && has_next_arg ) {
			exit_code = atoi( next_arg );
		} else if ( strcmp( arg, "--stdout" ) == 0 && has_next_arg ) {
			printf( "%s\n", next_arg );
			// fflush( stdout );
		} else if ( strcmp( arg, "--stderr" ) == 0 && has_next_arg ) {
			fprintf( stderr, "%s\n", next_arg );
			// fflush( stderr );
		}
	}

	return exit_code;
}
