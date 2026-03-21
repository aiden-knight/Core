#ifdef __linux__

#include <os.h>
#include <typecast.inl>

#include <unistd.h>

u32	os_get_virtual_memory_page_size() {
	long pageSize = sysconf( _SC_PAGESIZE );
	return trunc_cast( u32, pageSize );
}

#endif // __linux__
