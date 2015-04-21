#ifndef _COMMUNICATION_H_
#define _COMMUNICATION_H_

#include "type.h"
#include <sys/types.h>
#include <ucontext.h>

typedef struct communication_info{
	volatile UINT64 origin_rbp;
	struct ucontext *origin_uc;
	volatile pid_t process_id;
	volatile UINT8 flag;
}COMMUNICATION_INFO;

void init_communication();
void fini_communication();


#endif
