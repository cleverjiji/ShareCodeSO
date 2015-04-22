#include "utility.h"
#include "type.h"
#include "memory.h"
#include "shfile.h"
#include "wrapper.h"
#include "communication.h"
#include <unistd.h>

void initialize() __attribute__((constructor));
void fini() __attribute__((destructor));

void initialize()
{
	init_libc_wrapper();
	init_pthread_wrapper();

	share_code_segment();
	init_communication();
}

void fini()
{
	fini_communication();
}


