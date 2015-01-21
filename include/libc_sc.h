#ifndef _LIBC_SC_H_
#define _LIBC_SC_H_

// memory operation
extern void * (* libsc_mmap)(void *addr, size_t length, int prot, int flags, int fd, off_t offset);
extern int (* libsc_munmap)(void *addr, size_t length);
extern int (* libsc_mprotect)(const void *addr, size_t len, int prot);

// string operation
extern void * (* libsc_memcpy)(void *dest, const void *src, size_t n);

extern void libc_sc_load();
extern void libc_sc_unload();

#endif
