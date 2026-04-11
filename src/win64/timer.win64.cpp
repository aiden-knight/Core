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

#ifdef _WIN32

#include <timer.h>
#include <typecast.inl>

#include <Windows.h>

/*
================================================================================================

	Timer

================================================================================================
*/

static s64 GetFrequency( void ) {
	LARGE_INTEGER frequency = {};
	QueryPerformanceFrequency( &frequency );
	return frequency.QuadPart;
}

s64 TimeCycles( void ) {
	LARGE_INTEGER now = {};
	QueryPerformanceCounter( &now );
	return now.QuadPart;
}

float64 TimeSeconds( void ) {
	return Cast( float64, TimeCycles() ) / Cast( float64, GetFrequency() );
}

float64 TimeMS( void ) {
	return Cast( float64, TimeCycles() * 1000 ) / GetFrequency();
}

float64 TimeUS( void ) {
	return Cast( float64, TimeCycles() * 1000000 ) / GetFrequency();
}

float64 TimeNS( void ) {
	return Cast( float64, TimeCycles() * 1000000000 ) / GetFrequency();
}

#endif // _WIN32
