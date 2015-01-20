#ifndef _SHFILE_H_
#define _SHFILE_H_
#include "type.h"

extern INT32 get_share_code_shm_fd(ADDR share_code_start, SIZE share_code_size, const char *share_code_path, INT32 mapfile_idx);
extern void dump_shm_file();
extern void shm_file_unlink();

#endif
