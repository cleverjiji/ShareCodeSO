#include "string.h"
#include <sys/mman.h>
#include <unistd.h>
#include "type.h"
#include "utility.h"
#include "shfile.h"
#include "libc_sc.h"
#include "communication.h"
#include <sys/types.h>

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

char process_path[2048];
char process_name[1024];

void get_executable_path_and_name()
{
	char *path_end;
	INT32 ret = readlink("/proc/self/exe", process_path, 2048); 
	ASSERT(ret>0);

	path_end = strrchr(process_path,  '/');
	ASSERT(path_end);
	path_end++;
	strcpy(process_name, path_end);
	return ;
}

inline BOOL need_share_judge(const MapsFileItem *item_ptr)
{
	if(is_executable(item_ptr) &&  !strstr(item_ptr->pathname, "libsc.so") && !strstr(item_ptr->pathname, "[vdso]")
		&& !strstr(item_ptr->pathname, "[vsyscall]")){
		return true;
	}else
		return false;
}

INT32 heap_idx = -1;
INT32 stack_idx = -1;
SIZE so_shared_size = 0;

void read_proc_dynamic_link_info()
{
	;
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

		//find heap and stack
		if(strstr(currentRow->pathname, "[heap]"))
			heap_idx = mapsRowNum;
		else if(strstr(currentRow->pathname, "[stack]"))
			stack_idx = mapsRowNum;
		
		//calculate the row number
		mapsRowNum++;
        	ASSERT(mapsRowNum < mapsRowMaxNum);
		//calculate the max size
		if(need_share_judge(currentRow)){
			size_t currentRowSize = currentRow->end - currentRow->start;
			if(currentRowSize>max_size)
				max_size = currentRowSize;
			//calculate so code size
			if(mapsRowNum!=1)
				so_shared_size+=currentRowSize;
			//judge the segment need be shared or not
			currentRow->needShared = true;
		}else
			currentRow->needShared = false;
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

void allocate_code_cache()
{
	//debug
	void *ret = mmap((void*)0x10000, 0x1000, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_FIXED|MAP_ANON, -1, 0);
	PERROR(ret!=MAP_FAILED, "mmap failed!\n");
	
	//main executable code cache
	ADDR start = 0x11000;
	ADDR size = mapsArray[0].start - start;
	INT32 code_cache_fd = init_code_cache_shm(process_name, start, size);
	void * code_cache_start = mmap((void*)start, size, PROT_READ|PROT_EXEC, MAP_SHARED|MAP_FIXED, code_cache_fd, 0);
	PERROR(code_cache_start!=MAP_FAILED, "mmap failed!");
	//so code cache
	start = mapsArray[heap_idx+1].start - so_shared_size*2;
	size = so_shared_size*2;
	code_cache_fd = init_code_cache_shm(process_name, start, size);
	code_cache_start = mmap((void*)start, size, PROT_READ|PROT_EXEC, MAP_SHARED|MAP_FIXED, code_cache_fd, 0);
	PERROR(code_cache_start!=MAP_FAILED, "mmap failed!");
}

#define SHARE_STACK_MULTIPULE 8
extern COMMUNICATION_INFO *main_info;
void *temp_stack = NULL;
ADDR temp_stack_rsp = 0;
ADDR origin_stack_rsp = 0;
void *share_stack_start = NULL;
SIZE share_stack_size = 0;
void *default_stack_start = NULL;
SIZE default_stack_size = 0;
void *buf = NULL;
INT32 share_stack_fd = -1;
void *ret = NULL;
extern pid_t sc_gettid();

void share_stack()
{
	//sleep(10);
	// 1.allocate temp stack which is used for share origin stack
	temp_stack = mmap(NULL, 0x1000, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANON, -1, 0);
	PERROR(temp_stack!=MAP_FAILED, "mmap failed!");
	temp_stack_rsp = (ADDR)temp_stack + 0x1000;
	// 1.2 calculate default stack info
	default_stack_start = (void*)mapsArray[stack_idx].start;
	default_stack_size = mapsArray[stack_idx].end - mapsArray[stack_idx].start;
	// 1.3 enlarge default stack
	share_stack_size = default_stack_size*SHARE_STACK_MULTIPULE;
	share_stack_start = (void*)(mapsArray[stack_idx].end - share_stack_size);
	ASSERT((ADDR)share_stack_start>=(mapsArray[stack_idx-1].end));
	//SC_INFO("0x%lx-0x%lx\n", (ADDR)default_stack_start, (ADDR)default_stack_start+default_stack_size);
	//SC_ERR("0x%lx-0x%lx\n", (ADDR)share_stack_start, share_stack_size+(ADDR)share_stack_start);
	// 2.store rsp and switch stack
	__asm__ __volatile__ (
		"movq %%rsp, %[stack]\n\t"
		"movq %[new_stack], %%rsp\n\t"
		:[stack]"+m"(origin_stack_rsp), [new_stack]"+m"(temp_stack_rsp)
		::"cc"
	);
	
	// 3.share origin stack
	// 3.1 allocate a buffer and record origin stack content
	buf = mmap (NULL, default_stack_size, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
	PERROR(buf!=MAP_FAILED, "mmap failed!\n");	
	memcpy(buf, (const void*)default_stack_start, default_stack_size);	
	// 3.2 share enlarged stack
	share_stack_fd = init_share_stack_shm(process_name, (ADDR)share_stack_start, share_stack_size);
	ret = mmap(share_stack_start, share_stack_size, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_FIXED, share_stack_fd, 0);
	PERROR(ret!=MAP_FAILED, "mmap failed!");
	// 3.3 recover default stack data
	memcpy(default_stack_start, (const void*)buf, default_stack_size);
	// 3.4 munmap buffer
	munmap(buf, default_stack_size);
	
	// 4.restore rsp
	__asm__ __volatile__(
		"movq %[stack], %%rsp\n\t"
		:[stack]"+m"(origin_stack_rsp)
		::"cc"
	);
	// 5.free temp stack
	munmap(temp_stack, 0x1000);
	// 6.init communication flag
	main_info = (COMMUNICATION_INFO*)share_stack_start;
	main_info->origin_rbp = 0;
	main_info->origin_uc = 0;
	main_info->process_id = sc_gettid();
	main_info->flag = 0;
	return ;
}

extern void init_child_stack(void);
pid_t main_tid = 0;

void share_code_segment()
{
	main_tid = sc_gettid();
	//1.get executable name and path
	get_executable_path_and_name();
	//2.read proc maps to find need shared segments
	size_t max_len = read_proc_maps();
	//dump_proc_maps();
	//3.mmap max_len size memory to store the temporay code
	void *buf = mmap (NULL, max_len, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
	PERROR(buf!=MAP_FAILED, "mmap failed!\n");
	//4.init shm file for shared code segments 
	allocate_shm_file_for_share_code();
	//load libc_sc
	libc_sc_load();
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
			INT32 ret = libsc_mprotect((void*)original_start, original_size, PROT_READ|PROT_WRITE|PROT_EXEC);
			PERROR(ret==0, "mprotect failed!\n");
			//backup the code segment
			libsc_memcpy(buf, (const void*)original_start, original_size);
			//remap the code segment to a share file
			ret = libsc_munmap((void *)original_start, original_size);
			PERROR(ret==0, "munmap failed!\n");
			void * map_start = libsc_mmap((void*)original_start, original_size, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_FIXED, shm_fd, 0);
			PERROR(map_start!=MAP_FAILED, "mmap failed!");
			//recover the backup data
			libsc_memcpy((void*)original_start, buf, original_size);
			//change to original prot
			ret = libsc_mprotect((void*)original_start, original_size, original_prot);
			PERROR(ret==0, "mprotect failed!\n");
			
		}
	}
	//unload libc_sc
	libc_sc_unload();
	//6.munmap buf
	munmap(buf, max_len);
	//7.code_cache init
	allocate_code_cache();
	//8.map cc
	map_cc_to_code();
	//9.share stack
	share_stack();
	// 10.share child stack
	init_child_stack();
	//10.record share info
	record_share_info(process_name);
}
