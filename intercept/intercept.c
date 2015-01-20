#include "utility.h"
#include "type.h"
#include "memory.h"
#include "shfile.h"

void initialize() __attribute__((constructor));
void fini() __attribute__((destructor));

void initialize(){
	SC_INFO("so initialize!\n");
	share_code_segment();
	
}

void fini(){
	SC_INFO("so fini!\n");
}


