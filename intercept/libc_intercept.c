#include "wrapper.h"
#include "utility.h"
#include "child_stack_management.h"
#include <sys/mman.h>
#include <dlfcn.h>

void *(*real_mmap)(void *addr, size_t length, int prot, int flags, int fd, off_t offset) = NULL; 
int (*real_mprotect)(void *addr, size_t len, int prot) = NULL;

extern __thread BOOL is_in_pthread_create;
INT32 child_stack_idx = 0;

void init_libc_wrapper(void) 
{
	real_mmap = (void *(*)(void *addr, size_t length, int prot, int flags, int fd, off_t offset))dlsym(RTLD_NEXT, "mmap");
	real_mprotect = (int (*)(void *addr, size_t len, int prot))dlsym(RTLD_NEXT, "mprotect");
}

int wrapper_mprotect(void *addr, size_t len, int prot)
{
	if(is_in_pthread_create){
		SC_INFO("in pthread create(mprotect)\n");
		return 0;
	}else
		return real_mprotect(addr, len, prot);
}

void *wrapper_mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset)
{
	if(is_in_pthread_create){
		ASSERT(length==CHILD_STACK_SIZE && fd==-1 && offset==0 && !addr);
		ADDR stack_start = 0;
		ADDR stack_end = 0;
		child_stack_idx = allocate_child_stack_memory(&stack_start, &stack_end);
		SC_INFO("in pthread create(mmap)\n");
		return (void*)stack_start;
	}else
		return real_mmap(addr, length, prot, flags, fd, offset);
}

