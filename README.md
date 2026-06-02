# Core

## WARNING: This project is in heavy, active development.  I am still figuring things out, and everything is subject to change.

Core is my set of containers and OS wrapper functions for C++ made to my taste.

Currently it only supports Windows and Linux.

Core has:
* Virtual-memory backed linear allocator.
* "Temp storage" (linear scratch allocator).
* `Array` and `String` container types that are POD (no RAII here).
* OS-agnostic file IO.
* Defer (courtesy of the [deferpp library](https://github.com/olvap80/deferpp).
* A bunch of other stuff.

## Libraries

Core uses the following 3rdparty libraries:
* [deferpp](https://github.com/olvap80/deferpp).
* [xxHash](https://github.com/Cyan4973/xxHash).

## Motivation

The STL is slow and, in my opinion, not very ergonomic (there's lots about modern C++ I don't like: Templates, RAII, the list goes on), and I wanted to make something that I thought was better.  Core is my effort to make something that I think is better.
