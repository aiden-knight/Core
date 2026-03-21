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

static bool HasCommandLineArg( CommandLineArgs *args, const char *arg ) {
	for ( int argIndex = 0; argIndex < args->argc; argIndex++ ) {
		if ( strcmp( args->argv[argIndex], arg ) == 0 ) {
			return true;
		}
	}

	return false;
}

BUILDER_CALLBACK void SetBuilderOptions( BuilderOptions *options, CommandLineArgs *args ) {
	options->consolidateCompilerArgs = true;

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
		testDLL.additionalLibs.push_back( "msvcrt.lib" );
#endif
	} else {
		testDLL.binaryFolder = "bin/debug";
		testDLL.defines.push_back( "_DEBUG" );
#ifdef _WIN32
		testDLL.additionalLibs.push_back( "msvcrtd.lib" );
#endif
	}

	AddBuildConfig( options, &testDLL );


	//
	// tests
	//
	BuildConfig tests = {
		.name				= "tests",
		.languageVersion	= LANGUAGE_VERSION_CPP20,
		.dependsOn			= { testDLL },
		.intermediateFolder	= "intermediate",
		.binaryName			= "core-tests",
		.sourceFiles		= { "tests/tests.cpp", "src/*.cpp" },
		.defines			= { "_CRT_SECURE_NO_WARNINGS", "LOG_SHOW_FUNCTIONS" },
		.additionalIncludes	= { "include" },
#if defined( _WIN32 )
		.additionalLibs		= { "DbgHelp.lib", "Shlwapi.lib" },
#elif defined( __linux__ )
		.additionalLibs		= { "stdc++" },
#endif
		.warningLevels		= { "-Wall", "-Weverything", "-Wextra", "-Wpedantic" },
		.ignoreWarnings		= { "-Wno-switch-default" },
		.warningsAsErrors	= true
	};

	if ( HasCommandLineArg( args, "--release" ) ) {
		tests.optimizationLevel = OPTIMIZATION_LEVEL_O3;
		tests.binaryFolder = "bin/release";
		tests.defines.push_back( "NDEBUG" );
#ifdef _WIN32
		tests.additionalLibs.push_back( "msvcrt.lib" );
#endif
	} else {
		tests.binaryFolder = "bin/debug";
		tests.defines.push_back( "_DEBUG" );
#ifdef _WIN32
		tests.additionalLibs.push_back( "msvcrtd.lib" );
#endif
	}

	AddBuildConfig( options, &tests );


	//
	// visual studio
	//
	// options->generateSolution = true;
	options->solution = {
		.name = "Core",
		.path = "visual_studio",
		.platforms = { "x64" },
		.projects = {
			{
				.name = "core",
				.codeFolders = { "src", "include", "tests" },
				.configs = {
					{ "debug",   tests, {             }, { /* debugger arguments */ } },
					{ "release", tests, { "--release" }, { /* debugger arguments */ } }
				}
			},
		},
	};
}