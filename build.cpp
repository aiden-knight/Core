#include <builder.h>

BUILDER_CALLBACK void set_builder_options( BuilderOptions* options ) {
	BuildConfig tests_common = {
		.source_files		= { "tests/tests.cpp" },
		.defines			= { "_CRT_SECURE_NO_WARNINGS" },
		.additional_libs	= { "DbgHelp.lib", "Shlwapi.lib" },
		.ignore_warnings	= { "-Wno-switch-default" },
		.warnings_as_errors	= true
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
}