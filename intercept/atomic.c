#include "type.h"																																					   
#include "utility.h"
	
inline INT32 testandset (INT32 *p){
	long int readval = 0;

	__asm__ __volatile__ (
		"lock; cmpxchgl %2, %0"
		: "+m" (*p), "+a" (readval)
		: "r" (1)
		: "cc");
	return readval;
}

void spin_lock(spinlock_t *lock){
	while (testandset(lock));
}

void spin_unlock(spinlock_t *lock){
	*lock = 0;
}

INT32 spin_trylock(spinlock_t *lock){
	return !testandset(lock);
}

