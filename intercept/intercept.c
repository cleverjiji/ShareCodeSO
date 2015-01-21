#include "utility.h"
#include "type.h"
#include "memory.h"
#include "shfile.h"

void initialize() __attribute__((constructor));
void fini() __attribute__((destructor));

void initialize()
{
	share_code_segment();
}

void fini()
{
	;
}


