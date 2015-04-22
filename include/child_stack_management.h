#ifndef _CHILD_STACK_MANAGEMENT_
#define _CHILD_STACK_MANAGEMENT_

#include "type.h"
#include "communication.h"
#include <pthread.h>
#include <sys/types.h>


extern INT32 allocate_child_stack_memory(ADDR * stack_start,ADDR * stack_end);
extern void set_thread_id(INT32 idx,pid_t thread_id,pthread_t thread);
extern void free_child_stack(pthread_t thread_id);
COMMUNICATION_INFO *find_child_info(pid_t thread_id);


#endif
