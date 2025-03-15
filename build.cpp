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
		.source_files			= { "tests/tests.cpp" },
		.additional_includes	= { "include" },
		.additional_libs		= { "DbgHelp.lib", "Shlwapi.lib" },
		.ignore_warnings		= { "-Wno-switch-default" },
		.warnings_as_errors		= true
	};

	BuildConfig tests_common_non_suc = tests_common;
	tests_common_non_suc.binary_name = "core-tests-non-suc";
	tests_common_non_suc.source_files.push_back( "src/*.cpp" );

	// non SUC configs
	BuildConfig tests_win64_debug_non_suc = tests_common_non_suc;
	tests_win64_debug_non_suc.depends_on = { test_dll_debug };
	tests_win64_debug_non_suc.name = "win64-debug-non-suc";
	tests_win64_debug_non_suc.binary_folder = "bin/win64/debug";
	tests_win64_debug_non_suc.defines.push_back( "_DEBUG" );
	tests_win64_debug_non_suc.additional_libs.push_back( "msvcrtd.lib" );
	add_build_config( options, &tests_win64_debug_non_suc );

	BuildConfig tests_win64_release_non_suc = tests_common_non_suc;
	tests_win64_release_non_suc.depends_on = { test_dll_release };
	tests_win64_release_non_suc.name = "win64-release-non-suc";
	tests_win64_release_non_suc.binary_folder = "bin/win64/release";
	tests_win64_release_non_suc.optimization_level = OPTIMIZATION_LEVEL_O3;
	tests_win64_release_non_suc.defines.push_back( "NDEBUG" );
	tests_win64_release_non_suc.additional_libs.push_back( "msvcrt.lib" );
	add_build_config( options, &tests_win64_release_non_suc );


	// SUC configs
	BuildConfig tests_common_suc = tests_common;
	tests_common_suc.binary_name = "core-tests-suc";
	tests_common_suc.defines.push_back( "CORE_SUC" );

	BuildConfig tests_win64_debug_suc = tests_common_suc;
	tests_win64_debug_suc.depends_on = { test_dll_debug };
	tests_win64_debug_suc.name = "win64-debug-suc";
	tests_win64_debug_suc.binary_folder = tests_win64_debug_non_suc.binary_folder;
	tests_win64_debug_suc.defines.push_back( "_DEBUG" );
	tests_win64_debug_suc.additional_libs.push_back( "msvcrtd.lib" );
	add_build_config( options, &tests_win64_debug_suc );

	BuildConfig tests_win64_release_suc = tests_common_suc;
	tests_win64_release_suc.depends_on = { test_dll_release };
	tests_win64_release_suc.name = "win64-release-suc";
	tests_win64_release_suc.binary_folder = tests_win64_release_non_suc.binary_folder;
	tests_win64_release_suc.defines.push_back( "NDEBUG" );
	tests_win64_release_suc.additional_libs.push_back( "msvcrt.lib" );
	tests_win64_release_suc.optimization_level = OPTIMIZATION_LEVEL_O3;
	add_build_config( options, &tests_win64_release_suc );


	//
	// visual studio
	//
	options->generate_solution = true;
	options->solution.name = "Core";
	options->solution.path = "visual_studio";
	options->solution.platforms = { "x64" };
	options->solution.projects = {
		{
			.name = "core",
			.code_folders = { "src", "include", "tests" },
			.file_extensions = { "cpp", "c", "h", "inl" },
			.configs = {
				{ "debug-suc",       tests_win64_debug_suc,       { /* debugger arguments */ } },
				{ "release-suc",     tests_win64_release_suc,     { /* debugger arguments */ } },
				{ "debug-non-suc",   tests_win64_debug_non_suc,   { /* debugger arguments */ } },
				{ "release-non-suc", tests_win64_release_non_suc, { /* debugger arguments */ } }
			}
		},
	};
}