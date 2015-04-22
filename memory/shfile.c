#include <sys/mman.h>
#include <sys/stat.h>		 
#include <fcntl.h>	
#include <unistd.h>
#include <sys/types.h>
#include <string.h>

#include "type.h"
#include "utility.h"

#define SHM_MAX_NUM 100
typedef struct shm_item{
	char *shm_name;
	char *code_path;
	INT32 shm_fd;
	ADDR region_start;
	ADDR region_size;
	BOOL is_code_cache;
	BOOL is_stack;
	INT32 code_cache_idx;
}SHM_ITEM;
INT32 shm_file_max_num = 0;
INT32 code_cache_idx = 0;
SHM_ITEM shm_array[SHM_MAX_NUM];

inline char *get_executable_name_from_path(char *path)
{
	char *name = strrchr(path, '/');
	ASSERT(name);
	name++;
	return name;
}

INT32 get_share_code_shm_fd(ADDR share_code_start, SIZE share_code_size, const char *share_code_path, INT32 mapfile_idx)
{
	//1. construct file name
	char shm_name[256];
	char *share_code_name = get_executable_name_from_path((char*)share_code_path);
	sprintf(shm_name, "%s.%d", share_code_name, mapfile_idx);
	//2. open shm_file
	remove(shm_name);
	INT32 fd = shm_open(shm_name, O_RDWR|O_CREAT, 0644);  
	PERROR(fd!=-1, "shm_open failed!");
	//3. truncate the shm_file
	INT32 ret = ftruncate(fd, share_code_size);
	PERROR(ret==0, "ftruncate failed!");
	//4. record 
	shm_array[shm_file_max_num].shm_name = strdup(shm_name);
	shm_array[shm_file_max_num].shm_fd = fd;
	shm_array[shm_file_max_num].code_path = strdup(share_code_path);
	shm_array[shm_file_max_num].region_start = share_code_start;
	shm_array[shm_file_max_num].region_size = share_code_size;
	shm_array[shm_file_max_num].is_code_cache = false;
	shm_array[shm_file_max_num].is_stack = false;
	shm_file_max_num++;
	return fd;
}

INT32 init_share_stack_shm(const char *main_file_name, ADDR stack_start, SIZE stack_size)
{
	// 1.construct file name
	char share_stack_name[256];
	sprintf(share_stack_name, "%s.share_stack", main_file_name);
	// 2.open shm file
	remove(share_stack_name);
	INT32 fd = shm_open(share_stack_name, O_RDWR|O_CREAT, 0644);
	PERROR(fd!=-1, "shm open failed!");
	// 3. truncate the shm file
	INT32 ret = ftruncate(fd, stack_size);
	PERROR(ret==0, "ftruncate failed!");
	// 4.record
	shm_array[shm_file_max_num].shm_name = strdup(share_stack_name);
	shm_array[shm_file_max_num].code_path = strdup(share_stack_name);
	shm_array[shm_file_max_num].shm_fd = fd;
	shm_array[shm_file_max_num].region_start = stack_start;
	shm_array[shm_file_max_num].region_size = stack_size;
	shm_array[shm_file_max_num].is_code_cache = false;
	shm_array[shm_file_max_num].code_cache_idx = -2;	
	shm_array[shm_file_max_num].is_stack = true;
	shm_file_max_num++;
	return fd;
}

static INT32 child_group_stack_idx = 0;
INT32 init_child_group_stack_shm(const char *main_file_name, SIZE stack_size)
{
	// 1.construct file name
	char share_stack_name[256];
	sprintf(share_stack_name, "%s.child_share_stack", main_file_name);
	// 2.open shm file
	remove(share_stack_name);
	INT32 fd = shm_open(share_stack_name, O_RDWR|O_CREAT, 0644);
	PERROR(fd!=-1, "shm open failed!");
	// 3. truncate the shm file
	INT32 ret = ftruncate(fd, stack_size);
	PERROR(ret==0, "ftruncate failed!");
	// 4.record
	shm_array[shm_file_max_num].shm_name = strdup(share_stack_name);
	shm_array[shm_file_max_num].code_path = strdup(share_stack_name);
	shm_array[shm_file_max_num].shm_fd = fd;
	shm_array[shm_file_max_num].region_start = 0;
	shm_array[shm_file_max_num].region_size = stack_size;
	shm_array[shm_file_max_num].is_code_cache = false;
	shm_array[shm_file_max_num].code_cache_idx = -3;	
	shm_array[shm_file_max_num].is_stack = true;
	child_group_stack_idx = shm_file_max_num;
	shm_file_max_num++;
	return fd;
}

void set_child_group_stack_start(ADDR stack_start)
{
	shm_array[child_group_stack_idx].region_start = stack_start;
}

INT32 code_cache_num = 0;
INT32 main_executable_cc_idx = -1;
INT32 so_cc_idx = -1;

INT32 init_code_cache_shm(const char *main_file_name, ADDR code_cache_start, SIZE code_cache_size)
{
	//1.construct file name
	char code_cache_name[256];
	sprintf(code_cache_name, "%s.code_cache.%d", main_file_name, code_cache_num);
	//2.open shm file
	remove(code_cache_name);
	INT32 fd = shm_open(code_cache_name, O_RDWR|O_CREAT, 0644);
	PERROR(fd!=-1, "shm open failed!");
	//3. truncate the shm file
	INT32 ret = ftruncate(fd, code_cache_size);
	PERROR(ret==0, "ftruncate failed!");
	//4.record
	shm_array[shm_file_max_num].shm_name = strdup(code_cache_name);
	shm_array[shm_file_max_num].code_path = strdup(code_cache_name);
	shm_array[shm_file_max_num].shm_fd = fd;
	shm_array[shm_file_max_num].region_start = code_cache_start;
	shm_array[shm_file_max_num].region_size = code_cache_size;
	shm_array[shm_file_max_num].is_code_cache = true;
	shm_array[shm_file_max_num].code_cache_idx = -1;
	shm_array[shm_file_max_num].is_stack = false;
		
	if(code_cache_num==0)
		main_executable_cc_idx = shm_file_max_num;
	else
		so_cc_idx = shm_file_max_num;
	
	shm_file_max_num++;	
	code_cache_num++;
	return fd;
}

void map_cc_to_code()
{
	ASSERT(main_executable_cc_idx!=-1 && so_cc_idx!=-1);
	
	INT32 idx;
	for(idx=0;idx<shm_file_max_num;idx++){
		if(!shm_array[idx].is_code_cache && !shm_array[idx].is_stack){
			if(idx==0){
				ASSERT((shm_array[idx].region_start+shm_array[idx].region_size - shm_array[main_executable_cc_idx].region_start)<=0x7fffffff);
				shm_array[idx].code_cache_idx = main_executable_cc_idx;
			}else{
				ASSERT((shm_array[idx].region_start+shm_array[idx].region_size - shm_array[so_cc_idx].region_start)<=0x7fffffff);
				shm_array[idx].code_cache_idx = so_cc_idx;
			}
		}
	}
}

void record_share_info(const char *process_name)
{
	//2.write log
	//2.1 construct log name
	char log_name[256];
	sprintf(log_name, "/tmp/%s.share.log", process_name);
	//2.2 create log file and record
	remove(log_name);
	//fprintf(stderr, "log name:%s\n", log_name);
	FILE *log_file = fopen(log_name, "w+");
	fprintf(log_file, "shm_num: %d\n", shm_file_max_num);
	INT32 idx;
	for(idx=0; idx<shm_file_max_num; idx++){
		fprintf(log_file, "%d %lx-%lx %s %s\n", shm_array[idx].code_cache_idx, shm_array[idx].region_start, \
			shm_array[idx].region_start+shm_array[idx].region_size, shm_array[idx].shm_name, shm_array[idx].code_path);
	}
	//3.close
	fclose(log_file);
}

void dump_shm_file()
{
	SC_INFO("=========dump shm file=========\n");
	INT32 idx;
	for(idx=0; idx<shm_file_max_num; idx++){
		SC_INFO("[%2d]%s %d\n", idx, shm_array[idx].shm_name, shm_array[idx].shm_fd);
	}
	SC_INFO("============end=============\n");
}

void shm_file_unlink()
{
	INT32 idx;
	for(idx=0; idx<shm_file_max_num; idx++){
		INT32 ret = shm_unlink(shm_array[shm_file_max_num].shm_name);
		ASSERT(ret==0);
		ret = remove(shm_array[shm_file_max_num].shm_name);
		ASSERT(ret==0);
	}
}
