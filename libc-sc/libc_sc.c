
#include "type.h"
#include "utility.h"

#include <unistd.h>
#include	<dlfcn.h>
#include	<stdio.h>
#include	<stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

const char *libc_sc_name = "/tmp/libc-sc.so";

// memory operation
void * (* libsc_mmap)(void *addr, size_t length, int prot, int flags, int fd, off_t offset) = NULL;
int (* libsc_munmap)(void *addr, size_t length) = NULL;
int (* libsc_mprotect)(const void *addr, size_t len, int prot) = NULL;

// string operation
void * (* libsc_memcpy)(void *dest, const void *src, size_t n) = NULL;

void *handler = NULL;

void copy_sc_libc()
{
	int   in_fd, out_fd, n_chars; 
	char  buf[4096]; 
	
#ifdef LIBC_PATH
	const char *libc_pathname = LIBC_PATH;
#else
	const char *libc_pathname = NULL;
#endif

	ASSERT(libc_pathname);
	if (access(libc_sc_name, F_OK) == -1) {
		if ((in_fd=open(libc_pathname, O_RDONLY)) == -1){ 
			SC_ERR("Can not open %s!\n", libc_pathname);
			exit(-1);
		} 
	
		if ((out_fd=creat(libc_sc_name, 0755)) == -1){ 
			SC_ERR("Can not create %s!\n", libc_sc_name);
			exit(-1);
		} 
	
		while ( (n_chars = read(in_fd , buf, 4096)) > 0 ){ 
			if (write( out_fd, buf, n_chars ) != n_chars ){ 
				SC_ERR("Write %s error!\n", libc_sc_name);
				exit(-1);
			} 
		} 

		if(n_chars==-1){
			SC_ERR("Read error from %s!\n", libc_pathname); 
		}
		
		if((close(in_fd) == -1)||(close(out_fd) == -1)){
			SC_ERR("Close files failed!\n"); 
		}
	}	
}

void libc_sc_load(){
	//1.copy libc.so to current_dir
	copy_sc_libc();
	//2.load libc-sc.so
	handler = dlopen(libc_sc_name, RTLD_NOW | RTLD_DEEPBIND);
	ASSERT(handler);
	//3.dlsym
	libsc_mmap = dlsym(handler, "mmap");
	libsc_munmap = dlsym(handler, "munmap");
	libsc_mprotect = dlsym(handler, "mprotect");
	libsc_memcpy = dlsym(handler, "memcpy");
}

void libc_sc_unload()
{
	dlclose(handler);
}

