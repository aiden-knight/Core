#ifdef __linux__

#include <core_memory.h>

#include <stddef.h>

#include <sys/mman.h>

void	*virtual_reserve( const u64 size_bytes ) {
	return mmap( NULL, size_bytes, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0 );
}

void	*virtual_commit( void *ptr, const u64 size_bytes ) {
	if ( mprotect( ptr, size_bytes, PROT_READ | PROT_WRITE ) == -1 ) {
		return NULL;
	}

	return ptr;	// TODO(DM): 21/03/2026: is that correct?
}

void	virtual_decommit( void *ptr, const u64 size_bytes ) {
	if ( madvise( ptr, size_bytes, MADV_DONTNEED ) == -1 ) {
		// TODO(DM): 21/03/2026: handle error
	}
}

void	virtual_free( void *ptr ) {
	// TODO(DM): 21/03/2026: is -1 OK here?
	if ( munmap( ptr, 0 ) == -1 ) {
		// TODO(DM): 21/03/2026: handle error
	}
}

#endif // __linux__
