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

#include <builder.h>

BUILDER_CALLBACK void set_builder_options( BuilderOptions* options ) {
	//
	// test_dll
	//
	BuildConfig test_dll_common = {
		.source_files			= { "tests/test_dll.c" },
		.defines				= { "TEST_DLL_EXPORTS" },
		.binary_name			= "test_dll",
		.binary_type			= BINARY_TYPE_DYNAMIC_LIBRARY,
		.warnings_as_errors		= true
	};

	BuildConfig test_dll_debug = test_dll_common;
	test_dll_debug.name = "test-dll-debug";
	test_dll_debug.defines.push_back( "_DEBUG" );
	test_dll_debug.additional_libs.push_back( "msvcrtd.lib" );
	test_dll_debug.binary_folder = "bin/win64/debug";

	BuildConfig test_dll_release = test_dll_common;
	test_dll_release.name = "test-dll-release";
	test_dll_release.optimization_level = OPTIMIZATION_LEVEL_O3;
	test_dll_release.defines.push_back( "NDEBUG" );
	test_dll_release.additional_libs.push_back( "msvcrt.lib" );
	test_dll_release.binary_folder = "bin/win64/release";


	//
	// tests
	//
	BuildConfig tests_common = {
		.binary_name			= "core-tests",
		.source_files			= { "tests/tests.cpp", "src/*.cpp" },
		.defines				= { "_CRT_SECURE_NO_WARNINGS", "LOG_SHOW_FUNCTIONS" },
		.additional_includes	= { "include" },
		.additional_libs		= { "DbgHelp.lib", "Shlwapi.lib" },
		.warning_levels			= { "-Wall", "-Weverything", "-Wextra", "-Wpedantic" },
		.ignore_warnings		= { "-Wno-switch-default" },
		.warnings_as_errors		= true
	};

	BuildConfig tests_debug = tests_common;
	tests_debug.depends_on = { test_dll_debug };
	tests_debug.name = "win64-debug";
	tests_debug.binary_folder = "bin/win64/debug";
	tests_debug.defines.push_back( "_DEBUG" );
	tests_debug.additional_libs.push_back( "msvcrtd.lib" );
	add_build_config( options, &tests_debug );

	BuildConfig tests_release = tests_common;
	tests_release.depends_on = { test_dll_release };
	tests_release.name = "win64-release";
	tests_release.binary_folder = "bin/win64/release";
	tests_release.optimization_level = OPTIMIZATION_LEVEL_O3;
	tests_release.defines.push_back( "NDEBUG" );
	tests_release.additional_libs.push_back( "msvcrt.lib" );
	add_build_config( options, &tests_release );


	//
	// visual studio
	//
	options->generate_solution = true;
	options->solution = {
		.name = "Core",
		.path = "visual_studio",
		.platforms = { "x64" },
		.projects = {
			{
				.name = "core",
				.code_folders = { "src", "include", "tests" },
				.file_extensions = { "cpp", "c", "h", "inl" },
				.configs = {
					{ "debug",   tests_debug,   { /* debugger arguments */ } },
					{ "release", tests_release, { /* debugger arguments */ } }
				}
			},
		},
	};
}