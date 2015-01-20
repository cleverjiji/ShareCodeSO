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
	INT32 shm_fd;
}SHM_ITEM;
INT32 shm_file_max_num = 0;
SHM_ITEM shm_array[SHM_MAX_NUM];

char *get_executable_name_from_path(char *path){
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
	INT32 fd = shm_open(shm_name, O_RDWR|O_CREAT, 0644);  
	PERROR(fd!=-1, "shm_open failed!");
	//3. truncate the shm_file
	INT32 ret = ftruncate(fd, share_code_size);
	PERROR(ret==0, "ftruncate failed!");
	//4. record 
	shm_array[shm_file_max_num].shm_name = strdup(shm_name);
	shm_array[shm_file_max_num].shm_fd = fd;
	shm_file_max_num++;
	return fd;
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
