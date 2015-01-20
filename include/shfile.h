#ifndef _SHFILE_H_
#define _SHFILE_H_
#include "type.h"

extern INT32 get_share_code_shm_fd(ADDR share_code_start, SIZE share_code_size, const char *share_code_path, INT32 mapfile_idx);
extern INT32 init_code_cache_shm(const char *main_file_path, SIZE code_cache_size);
void record_share_info(ADDR code_cache_start, const char *process_name);
extern void dump_shm_file();
extern void shm_file_unlink();

#endif
