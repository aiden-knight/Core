# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build System

This project uses a custom build tool (`builder.exe`) configured via `build.cpp`. Debug is the default; pass `--release` for an optimized build.

```bash
# Debug build (builds test-dll + core.dll dependencies, then core-tests)
builder.exe build.cpp --config=tests

# Release build
builder.exe build.cpp --config=tests --release

# Build just the core library
builder.exe build.cpp --config=core
builder.exe build.cpp --config=core --release

# Generate Visual Studio solution
builder.exe build.cpp --sln
```

Build outputs go to `bin/debug/` or `bin/release/`. Intermediates go to `intermediate/`.

### Running Tests

```bash
.\bin\debug\core-tests.exe
.\bin\release\core-tests.exe
```

`ci.bat` does both builds and runs both executables in sequence.

## Architecture

Core is a C/C++ utility library. The code is organized into three layers:

- **`include/`** — Public API headers only.
- **`src/*.cpp`** — Platform-agnostic implementations.
- **`src/win64/`** and **`src/linux/`** — Platform-specific implementations for file I/O, timers, paths, memory, threads, process spawning, debug output, and dynamic library loading.
- **`src/xxhash/`** — Vendored xxHash library used by `hash.cpp`.

The build produces `core.dll` (the library) and `core-tests.exe` (test runner) as separate binaries. The tests executable links against `core.dll`.

### Key Design Patterns

- **POD structs only**: All structs in Core's public API must be Plain Old Data — no constructors, destructors, or virtual functions. RAII is banned.
- **Arena-first allocation**: There are no `malloc`/`free` calls in the codebase. All allocations go through a `LinearAllocator`. Short-lived allocations use `g_temp_storage` directly; long-lived allocations use a caller-provided `LinearAllocator *`. When adding new code, follow this pattern.
- **Custom types**: All code uses typedefs from `int_types.h` (`s8`, `u8`, `s16`, `u16`, `s32`, `u32`, `s64`, `u64`, `float32`, `float64`, `bool8`) — never `int`, `float`, etc.
- **`Array<T>`**: Primary dynamic array container (`core_array.h` / `core_array.inl`). Requires `init(LinearAllocator *)` before use.
- **`String`**: Non-owning string type with length. Operations are free functions in `core_string.h`. Requires `string_init(String *, LinearAllocator *)` before use.
- **`StringBuilder`**: Linked-buffer string builder for incremental string construction (`string_builder.h`). Requires `string_builder_reset(StringBuilder *)` before use.
- **`Hashmap`**: Open-addressing hash table (`hashmap.h` / `hashmap.cpp`).
- **`LinearAllocator`**: Stack-based bump allocator — allocate forward, free everything at once.
- **`TempStorage`**: A global scratch allocator (`g_temp_storage`) backed by a `LinearAllocator`. The per-thread design is not yet implemented.
- **`defer.h`**: Go-style scope-exit cleanup via macros. Used internally in implementation files; not part of the public API.

### Header Inclusion Rules

Headers in `include/` are only allowed to include `int_types.h` and `dll_export.h`. They must not include any other headers, including other Core headers or standard library headers. Some headers currently violate this rule and need to be fixed. When adding new headers, enforce this strictly.

`.inl` files are exempt from this rule — they are template implementation files meant to be included directly in source files, not traditional headers.

### Compiler and Warning Settings

Clang is the primary compiler. Warnings are set to `-Wall -Weverything -Wextra -Wpedantic` with `-Werror`. Per-warning suppressions are managed via the `ignoreWarnings` array in each `BuildConfig` in `build.cpp` — not via `#pragma clang diagnostic` blocks in source files.

### Tests

Tests live in `tests/tests.cpp` and use the Temper single-header testing framework (`tests/temper/temper.h`, v2.0.1). There is also a `test_dll.c` used to test dynamic library loading. Temper supports filtering via `-t <test>`, `-s <suite>`, and `-p` (partial match) flags passed to the test executable.
