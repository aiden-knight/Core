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
	BuildConfig tests_common = {
		.source_files			= { "tests/tests.cpp" },
		.defines				= { "_CRT_SECURE_NO_WARNINGS", "LOG_SHOW_FUNCTIONS" },
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
	tests_win64_debug_non_suc.name			= "win64-debug-non-suc";
	tests_win64_debug_non_suc.binary_folder	= "bin/win64/debug";
	add_build_config( options, &tests_win64_debug_non_suc );

	BuildConfig tests_win64_release_non_suc = tests_common_non_suc;
	tests_win64_release_non_suc.name				= "win64-release-non-suc";
	tests_win64_release_non_suc.binary_folder		= "bin/win64/release";
	tests_win64_release_non_suc.optimization_level	= OPTIMIZATION_LEVEL_O3;
	add_build_config( options, &tests_win64_release_non_suc );


	// SUC configs
	BuildConfig tests_common_suc = tests_common;
	tests_common_suc.binary_name = "core-tests-suc";
	tests_common_suc.defines.push_back( "CORE_SUC" );

	BuildConfig tests_win64_debug_suc = tests_common_suc;
	tests_win64_debug_suc.name = "win64-debug-suc";
	tests_win64_debug_suc.binary_folder = tests_win64_debug_non_suc.binary_folder;
	add_build_config( options, &tests_win64_debug_suc );

	BuildConfig tests_win64_release_suc = tests_common_suc;
	tests_win64_release_suc.name = "win64-release-suc";
	tests_win64_release_suc.binary_folder = tests_win64_release_non_suc.binary_folder;
	tests_win64_release_suc.optimization_level = OPTIMIZATION_LEVEL_O3;
	add_build_config( options, &tests_win64_release_suc );

	 options->generate_solution = true;

	 options->solution.name = "test-sln";
	 options->solution.path = "../visual_studio";
	 options->solution.platforms = { "x64" };
	 options->solution.projects = {
	 	{
	 		.name = "core",
	 		.code_folders = { "/src", "/tests", "/include" },
	 		.file_extensions = { "cpp", "h", "inl" },
	 		.configs = {
	 			{ "debug", tests_win64_debug_suc,   { /* debugger arguments */ } },
	 			{ "release", tests_win64_release_suc, { /* debugger arguments */ } },
	 		}
	 	}
	 };
}