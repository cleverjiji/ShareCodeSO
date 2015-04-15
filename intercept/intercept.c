#include "utility.h"
#include "type.h"
#include "memory.h"
#include "shfile.h"
#include "communication.h"
#include <unistd.h>
void initialize() __attribute__((constructor));
void fini() __attribute__((destructor));

void initialize()
{
	share_code_segment();
	init_communication();
}

void fini()
{
	;
}


