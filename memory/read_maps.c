#include "string.h"
#include <sys/mman.h>
#include <unistd.h>
#include "type.h"
#include "utility.h"
#include "shfile.h"

typedef struct mapsRow{
	ADDR start;
	ADDR end;
	ADDR offset;
	char perms[8];
	char dev[8];
	INT32 inode;
	char pathname[128];
	BOOL needShared;
	INT32 shm_fd;//used for shm file
}MapsFileItem;

#define mapsRowMaxNum 100
MapsFileItem mapsArray[mapsRowMaxNum] = {{0}};
INT32 mapsRowNum = 0;

inline BOOL is_readable(const MapsFileItem *item_ptr)
{
	if(strstr(item_ptr->perms, "r"))
		return true;
	else
		return false;
}

inline BOOL is_writeable(const MapsFileItem *item_ptr)
{
	if(strstr(item_ptr->perms, "w"))
		return true;
	else
		return false;
}

inline BOOL is_executable(const MapsFileItem *item_ptr)
{
	if(strstr(item_ptr->perms, "x"))
		return true;
	else
		return false;
}

INT32 calculate_mmap_prot(MapsFileItem *item_ptr)
{                                                                                                                      
	 INT32 ret = PROT_NONE;
	 if(is_readable(item_ptr))
		 ret |= PROT_READ;
	 if(is_writeable(item_ptr))
		 ret |= PROT_WRITE;
	 if(is_executable(item_ptr))
		 ret |= PROT_EXEC;
	 return ret;
}

#define PATH_MAX 2048
#define NAME_MAX 1024
char process_path[PATH_MAX];
char process_name[NAME_MAX];

void get_executable_path_and_name()
{
	char *path_end;
	INT32 ret = readlink("/proc/self/exe", process_path, PATH_MAX); 
	ASSERT(ret>0);

	path_end = strrchr(process_path,  '/');
	ASSERT(path_end);
	path_end++;
	strcpy(process_name, path_end);
	return ;
}

inline BOOL need_share_judge(const MapsFileItem *item_ptr)
{
	if(is_executable(item_ptr) && strstr(item_ptr->pathname, process_name)){
		return true;
	}else
		return false;
}

size_t read_proc_maps()
{
	//1.open maps file
	FILE *maps_file = fopen("/proc/self/maps", "r"); 
	ASSERT(maps_file!=NULL);
	//2.read maps file, line by line
	size_t max_size = 0;
	mapsRowNum = 0;
	char row_buffer[256];
	fgets(row_buffer, 256, maps_file);
	while(!feof(maps_file)){
		MapsFileItem *currentRow = mapsArray+mapsRowNum;
		currentRow->needShared = false;
		//read one row
		currentRow->pathname[0] = '\0';
		sscanf(row_buffer, "%lx-%lx %s %lx %s %d %s", &(currentRow->start), &(currentRow->end), \
			currentRow->perms, &(currentRow->offset), currentRow->dev, &(currentRow->inode), currentRow->pathname);
		//calculate the row number
		mapsRowNum++;
        	ASSERT(mapsRowNum < mapsRowMaxNum);
		//calculate the max size
		size_t currentRowSize = currentRow->end - currentRow->start;
		if(currentRowSize>max_size)
			max_size = currentRowSize;
		//judge the segment need be shared or not
		currentRow->needShared= need_share_judge(currentRow);
		//read next row
		fgets(row_buffer, 256, maps_file);
	}

	fclose(maps_file);
	return max_size;
}

void dump_proc_maps()
{
	INT32 idx;
	SC_INFO("-----------------------dump Proc Maps-----------------------\n");
	for(idx=0; idx<mapsRowNum; idx++){
		MapsFileItem *currentRow = mapsArray+idx;
		if (currentRow->needShared){
			SC_INFO("[%2d] %lx-%lx %s %lx %s %d %s\n", idx, currentRow->start, currentRow->end, 
				currentRow->perms, currentRow->offset, currentRow->dev, currentRow->inode, currentRow->pathname);
		}else
			SC_PRINT("[%2d] %lx-%lx %s %lx %s %d %s\n", idx, currentRow->start, currentRow->end, 
			currentRow->perms, currentRow->offset, currentRow->dev, currentRow->inode, currentRow->pathname);
	}
	SC_INFO("----------------------------END-----------------------------\n");
}

void allocate_shm_file_for_share_code()
{
	INT32 idx;
	for(idx=0; idx<mapsRowNum; idx++){
		MapsFileItem *currentRow = mapsArray+idx;
		if(currentRow->needShared)
			currentRow->shm_fd = get_share_code_shm_fd(currentRow->start, currentRow->end - currentRow->start, currentRow->pathname, idx);
	}
}

void share_code_segment()
{
	//1.get exetable name and path
	get_executable_path_and_name();
	//2.read proc maps to find need shared segments
	size_t max_len = read_proc_maps();
	//dump_proc_maps();
	//3.mmap max_len size memory to store the temporay code
	void *buf = mmap (NULL, max_len, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
	PERROR(buf!=MAP_FAILED, "mmap failed!\n");
	//4.init shm file for shared code segments 
	allocate_shm_file_for_share_code();
	//5.share segments
	INT32 idx;
	for(idx=0; idx<mapsRowNum; idx++){
		MapsFileItem *currentRow = mapsArray+idx;
		if(currentRow->needShared){//share this segment
			ADDR original_start = currentRow->start;
			SIZE original_size = currentRow->end - original_start;
			INT32 original_prot = calculate_mmap_prot(currentRow);
			INT32 shm_fd = currentRow->shm_fd;
			ASSERT(shm_fd!=0);
			//change prot to readable
			INT32 ret = mprotect((void*)original_start, original_size, PROT_READ|PROT_WRITE);
			PERROR(ret==0, "mprotect failed!\n");
			//backup the code segment
			memcpy(buf, (const void*)original_start, original_size);
			//remap the code segment to a share file
			ret = munmap((void *)original_start, original_size);
			PERROR(ret==0, "munmap failed!\n");
			void * map_start = mmap((void*)original_start, original_size, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_FIXED, shm_fd, 0);
			ASSERT(map_start!=MAP_FAILED);
			//recover the backup data
			memcpy((void*)original_start, buf, original_size);
			//change to original prot
			ret = mprotect((void*)original_start, original_size, original_prot);
			PERROR(ret==0, "mprotect failed!\n");
		}
	}
	//4.munmap buf
	munmap(buf, max_len);
}
