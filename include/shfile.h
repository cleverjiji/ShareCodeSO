#ifndef _SHFILE_H_
#define _SHFILE_H_
#include "type.h"

extern INT32 get_share_code_shm_fd(ADDR share_code_start, SIZE share_code_size, const char *share_code_path, INT32 mapfile_idx);
extern INT32 init_code_cache_shm(const char *main_file_path, ADDR code_cache_start, SIZE code_cache_size);
extern INT32 init_share_stack_shm(const char *main_file_name, ADDR stack_start, SIZE stack_size);
void record_share_info(const char *process_name);
void map_cc_to_code();
extern void dump_shm_file();
extern void shm_file_unlink();
extern void set_child_group_stack_start(ADDR stack_start);
extern INT32 init_child_group_stack_shm(const char *main_file_name, SIZE stack_size);
extern INT32 init_jump_table_shm(const char *main_file_name, SIZE jump_table_size);
extern void set_jump_table_start(ADDR table_start);

#endif
