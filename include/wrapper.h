#ifndef _WRAPPER_H_
#define _WRAPPER_H_

#define wrapper_sigaction sigaction
#define wrapper_signal signal

#define wrapper_pthread_create pthread_create
#define wrapper_pthread_join pthread_join
#define wrapper_mmap mmap
#define wrapper_mprotect mprotect

extern void init_libc_wrapper(void);
extern void init_pthread_wrapper(void);
#endif
