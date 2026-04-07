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

BUILDER_CALLBACK void SetBuilderOptions( BuilderOptions *options, CommandLineArgs *args ) {
	options->consolidateCompilerArgs = true;

	//
	// test DLL
	//
	BuildConfig testDLL = {
		.name				= "test-dll",
		.languageVersion	= LANGUAGE_VERSION_C99,
		.sourceFiles		= { "tests/test_dll.c" },
		.defines			= { "TEST_DLL_EXPORTS" },
		.intermediateFolder	= "intermediate",
		.binaryName			= "test_dll",
		.binaryType			= BINARY_TYPE_DYNAMIC_LIBRARY,
		.warningsAsErrors	= true
	};

	if ( HasCommandLineArg( args, "--release" ) ) {
		testDLL.optimizationLevel = OPTIMIZATION_LEVEL_O3;
		testDLL.binaryFolder = "bin/release";
		testDLL.defines.push_back( "NDEBUG" );
#ifdef _WIN32
		testDLL.additionalLibs.push_back( "msvcrt" );
#endif
	} else {
		testDLL.binaryFolder = "bin/debug";
		testDLL.defines.push_back( "_DEBUG" );
#ifdef _WIN32
		testDLL.additionalLibs.push_back( "msvcrtd" );
#endif
	}

	AddBuildConfig( options, &testDLL );


	//
	// core
	//
	BuildConfig core = {
		.name				= "core",
		.binaryType			= BINARY_TYPE_DYNAMIC_LIBRARY,
		.intermediateFolder	= "intermediate",
		.binaryName			= "core",
		.sourceFiles		= { "src/*.cpp" },
		.defines			= { "CORE_EXPORTS", "HASHMAP_HIDE_MISSING_KEY_WARNING" },
		.additionalIncludes = { "include" },
		.additionalLibs		= { "Shlwapi", "DbgHelp" },
		.warningLevels		= { "-Wall", "-Weverything", "-Wextra", "-Wpedantic" },
		.ignoreWarnings = {
			"-Wno-c++98-compat",
			"-Wno-c++98-compat-pedantic",
			"-Wno-pre-c++20-compat-pedantic",
			"-Wno-c++20-designator",
			"-Wno-reorder-init-list",
			"-Wno-zero-as-null-pointer-constant",
			"-Wno-unsafe-buffer-usage",
			"-Wno-unsafe-buffer-usage-in-libc-call",
			"-Wno-old-style-cast",
			"-Wno-format-nonliteral",
			"-Wno-deprecated-declarations",
			"-Wno-double-promotion",
			"-Wno-missing-field-initializers",
			"-Wno-switch-default",
			"-Wno-cast-qual",
			"-Wno-implicit-int-float-conversion",
			"-Wno-padded",
			"-Wno-float-equal",
			"-Wno-sign-compare",
		},
		.warningsAsErrors	= true,
	};

#ifdef _WIN32
	core.defines.push_back( "WIN32_LEAN_AND_MEAN" );
	core.defines.push_back( "NOMINMAX" );
#endif

	if ( HasCommandLineArg( args, "--release" ) ) {
		core.optimizationLevel = OPTIMIZATION_LEVEL_O3;
		core.binaryFolder = "bin/release";
		core.defines.push_back( "NDEBUG" );
#ifdef _WIN32
		core.additionalLibs.push_back( "msvcrt" );
		core.additionalLibs.push_back( "ucrt" );
#endif
	} else {
		core.binaryFolder = "bin/debug";
		core.defines.push_back( "_DEBUG" );
#ifdef _WIN32
		core.additionalLibs.push_back( "msvcrtd" );
		core.additionalLibs.push_back( "ucrtd" );
#endif
	}

	AddBuildConfig( options, &core );


	//
	// tests
	//
	BuildConfig tests = {
		.name				= "tests",
		.dependsOn			= { core, testDLL },
		.intermediateFolder	= "intermediate",
		.binaryName			= "core-tests",
		.sourceFiles		= { "tests/tests.cpp" },
		.defines			= { "_CRT_SECURE_NO_WARNINGS", "LOG_SHOW_FUNCTIONS" },
		.additionalIncludes	= { "include" },
		.additionalLibs		= { "core" },
		.warningLevels		= { "-Wall", "-Weverything", "-Wextra", "-Wpedantic" },
		.ignoreWarnings = {
			"-Wno-switch-default",
			"-Wno-c++98-compat",
			"-Wno-c++98-compat-pedantic",
			"-Wno-old-style-cast",
			"-Wno-zero-as-null-pointer-constant",
			"-Wno-unsafe-buffer-usage-in-libc-call",
			"-Wno-double-promotion",
			"-Wno-unsafe-buffer-usage",
		},
		.warningsAsErrors	= true
	};

#if defined( __linux__ )
	tests.additionalLibs.push_back( "stdc++" );
#endif

	if ( HasCommandLineArg( args, "--release" ) ) {
		tests.optimizationLevel = OPTIMIZATION_LEVEL_O3;
		tests.binaryFolder = "bin/release";
		tests.defines.push_back( "NDEBUG" );
		tests.additionalLibPaths.push_back( "bin/release" );
//#ifdef _WIN32
//		tests.additionalLibs.push_back( "msvcrt" );
//#endif
	} else {
		tests.binaryFolder = "bin/debug";
		tests.defines.push_back( "_DEBUG" );
		tests.additionalLibPaths.push_back( "bin/debug" );
//#ifdef _WIN32
//		tests.additionalLibs.push_back( "msvcrtd" );
//#endif
	}

	AddBuildConfig( options, &tests );


	//
	// visual studio
	//
	options->solution = {
		.name = "Core",
		.path = "visual_studio",
		.platforms = { "x64" },
		.projects = {
			{
				.name = "core",
				.codeFolders = { "src", "include" },
				.configs = {
					{ "debug",   core,  {             }, { /* debugger arguments */ } },
					{ "release", core,  { "--release" }, { /* debugger arguments */ } },
				},
			},

			{
				.name = "tests",
				.configs = {
					{ "debug",   tests, {             }, { /* debugger arguments */ } },
					{ "release", tests, { "--release" }, { /* debugger arguments */ } },
				},
			},
		},
	};

	if ( HasCommandLineArg( args, "--sln" ) ) {
		options->generateSolution = true;
	}
}
