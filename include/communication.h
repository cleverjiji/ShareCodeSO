#ifndef _COMMUNICATION_H_
#define _COMMUNICATION_H_

#include "type.h"
#include <sys/types.h>
#include <ucontext.h>

typedef struct communication_info{
	volatile UINT64 jump_table_base;
	struct ucontext *origin_uc;
	volatile pid_t process_id;
	volatile UINT8 can_stop;//0: can not; 1:can
	volatile UINT8 flag;
}COMMUNICATION_INFO;

void init_communication();
void fini_communication();


#endif
