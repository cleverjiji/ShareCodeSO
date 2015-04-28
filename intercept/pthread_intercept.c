#include <pthread.h>
#include <dlfcn.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/syscall.h>   /* For SYS_xxx definitions */

#include "type.h"
#include "utility.h"
#include "wrapper.h"
#include "child_stack_management.h"

__thread BOOL is_in_pthread_create = false;
__thread BOOL can_be_interrupted = false;

int (*real_pthread_create)(pthread_t *thread, const pthread_attr_t *attr, void *(*start_routine) (void *), void *arg) = NULL; 
int (*real_pthread_join)(pthread_t thread, void **retval) = NULL;
extern INT32 child_stack_idx;

pid_t sc_gettid()
{
	return syscall(SYS_gettid);
}

void init_pthread_wrapper(void) 
{
	real_pthread_create = (int (*)(pthread_t *thread, const pthread_attr_t *attr, void *(*start_routine) (void *), void *arg))dlsym(RTLD_NEXT, "pthread_create");
	real_pthread_join = (int (*)(pthread_t thread, void **retval))dlsym(RTLD_NEXT, "pthread_join");
	can_be_interrupted = true;
}

void *(*start_routine_bk)(void*) = NULL;
static INT32 flag = 0;

void *wrapper_child_thread(void *arg)
{
	is_in_pthread_create = false;
	pid_t tid = sc_gettid();
	pthread_t pt_t = pthread_self();
 	set_thread_id(child_stack_idx, tid, pt_t);
	flag = 0;
	can_be_interrupted = true;
	void *ret = start_routine_bk(arg);
	free_child_stack(pt_t);
	return ret;
}

int wrapper_pthread_create(pthread_t *thread, const pthread_attr_t *attr, void *(*start_routine) (void *), void *arg)
{
	is_in_pthread_create = true;
	can_be_interrupted = false;
	start_routine_bk = start_routine;
	flag = 1;
	SC_INFO("in pthread create!\n");
	int ret = real_pthread_create(thread, attr, wrapper_child_thread, arg);
	SC_INFO("out pthread create!\n");
	while(flag){;}
	can_be_interrupted = true;
	is_in_pthread_create = false;
	return ret;
}

