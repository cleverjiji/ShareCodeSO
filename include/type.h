#ifndef _TYPE_H_
#define _TYPE_H_

#include <stdbool.h>
#include <stdlib.h>

typedef bool BOOL;
typedef char INT8;
typedef unsigned char UINT8;
typedef short INT16;
typedef unsigned short UINT16;
typedef int INT32;
typedef unsigned int UINT32;
typedef long long INT64;
typedef unsigned long long UINT64;
typedef unsigned long ADDR;
typedef size_t SIZE;
typedef int spinlock_t;

#define CHILD_STACK_SIZE 0x801000

#endif
