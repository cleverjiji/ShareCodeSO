#include <sys/mman.h>
#include "shfile.h"

#define JUMP_TABLE_SIZE 0x100000

void jump_table_init(const char *main_file_name)
{
	INT32 fd = init_jump_table_shm(main_file_name, JUMP_TABLE_SIZE);
	void *table_start = mmap(NULL, JUMP_TABLE_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
	set_jump_table_start((ADDR)table_start);
}
