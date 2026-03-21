#ifdef _WIN32

#include <os.h>

#ifndef WIN32_LEAN_AND_MEAN
	#define WIN32_LEAN_AND_MEAN
#endif

#ifndef NOMINMAX
	#define NOMINMAX
#endif

#include <Windows.h>

u32 os_get_virtual_memory_page_size() {
	// TODO(DM): 29/12/2025: obviously bad, but having core cache state at startup caused problems before
	// so I'm gun-shy about doing that again
	SYSTEM_INFO sys_info = {};
	GetSystemInfo( &sys_info );
	return sys_info.dwPageSize;
}

#endif // _WIN32
