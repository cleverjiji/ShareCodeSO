#include <sys/mman.h>
#include "atomic.h"
#include "utility.h"
#include "type.h"
#include "shfile.h"
#include "communication.h"

#define THREAD_MAX_NUM 128
extern char process_name[1024];

typedef struct child_stack_table_item{
	ADDR stack_start;
	ADDR stack_end;
	BOOL is_allocated;
	pthread_t pt_t;
	COMMUNICATION_INFO *contact_info;
}CHILD_STACK_TABLE_ITEM;
CHILD_STACK_TABLE_ITEM global_child_stack_table[THREAD_MAX_NUM];

spinlock_t stack_lock = 0;

void init_child_stack(void)
{
	SIZE child_stack_size = THREAD_MAX_NUM*CHILD_STACK_SIZE;	
	INT32 child_stack_fd = init_child_group_stack_shm(process_name, child_stack_size);
	void *child_stack_start = mmap(NULL, child_stack_size, PROT_READ|PROT_WRITE, MAP_SHARED, child_stack_fd, 0);
	PERROR(child_stack_start!=MAP_FAILED, "mmap child group stack failed!\n");
	//init stack
	ADDR allocate_stack_start = (ADDR)child_stack_start;
	set_child_group_stack_start(allocate_stack_start);
	INT32 idx;
	for(idx = 0; idx<THREAD_MAX_NUM; idx++){
		global_child_stack_table[idx].stack_start = allocate_stack_start;
		global_child_stack_table[idx].stack_end = allocate_stack_start+CHILD_STACK_SIZE;
		global_child_stack_table[idx].is_allocated = false;
		global_child_stack_table[idx].pt_t = 0;
		global_child_stack_table[idx].contact_info = (COMMUNICATION_INFO *)allocate_stack_start;
		global_child_stack_table[idx].contact_info->origin_rbp = 0;
		global_child_stack_table[idx].contact_info->origin_uc = NULL;
		global_child_stack_table[idx].contact_info->process_id = 0;
		global_child_stack_table[idx].contact_info->flag = 0;
		allocate_stack_start += CHILD_STACK_SIZE;
	}
}

INT32 allocate_child_stack_memory(ADDR *stack_start, ADDR *stack_end)
{
	spin_lock(&stack_lock);
	INT32 idx;
	for(idx = 0; idx<THREAD_MAX_NUM; idx++){
		if(!global_child_stack_table[idx].is_allocated){
			*stack_start = global_child_stack_table[idx].stack_start;
			*stack_end = global_child_stack_table[idx].stack_end;
			global_child_stack_table[idx].is_allocated = true;
			global_child_stack_table[idx].pt_t = -1;
			global_child_stack_table[idx].contact_info->origin_rbp = 0;
			global_child_stack_table[idx].contact_info->origin_uc = NULL;
			global_child_stack_table[idx].contact_info->process_id = -1;
			global_child_stack_table[idx].contact_info->flag = 0;
			spin_unlock(&stack_lock);
			return idx;
		}
	}
	spin_unlock(&stack_lock);
	ASSERT(0);
	return 0;
}

void set_thread_id(INT32 idx, pid_t thread_id, pthread_t thread)
{
	spin_lock(&stack_lock);
	global_child_stack_table[idx].pt_t = thread;
	global_child_stack_table[idx].contact_info->process_id = thread_id;
	spin_unlock(&stack_lock);
	return ;
}

void free_child_stack(pthread_t thread_id)
{
	spin_lock(&stack_lock);
	INT32 idx;
	for(idx=0; idx<THREAD_MAX_NUM; idx++){
		if(global_child_stack_table[idx].pt_t == thread_id){
			global_child_stack_table[idx].is_allocated = false;
			global_child_stack_table[idx].pt_t = 0;
			global_child_stack_table[idx].contact_info->origin_rbp = 0;
			global_child_stack_table[idx].contact_info->origin_uc = NULL;
			global_child_stack_table[idx].contact_info->process_id = 0;
			global_child_stack_table[idx].contact_info->flag = 0;
			spin_unlock(&stack_lock);
			return ;
		}
	}
	spin_unlock(&stack_lock);
	ASSERTM(0, "free child stack 0x%lx\n", thread_id);
	return ;
}

COMMUNICATION_INFO *find_child_info(pid_t thread_id)
{
	spin_lock(&stack_lock);
	INT32 idx;
	for(idx=0; idx<THREAD_MAX_NUM; idx++){
		if(global_child_stack_table[idx].contact_info->process_id == thread_id){
			ASSERT(global_child_stack_table[idx].is_allocated);
			spin_unlock(&stack_lock);
			return global_child_stack_table[idx].contact_info;
		}
	}
	spin_unlock(&stack_lock);
	ASSERTM(0, "find no child info, thread_id = 0x%x\n", thread_id);
	return NULL;	
}
