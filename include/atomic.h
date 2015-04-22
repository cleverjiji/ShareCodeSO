#ifndef _ATOMIC_H_
#define _ATOMIC_H_
#include "type.h"

extern void spin_lock(spinlock_t *lock);
extern void spin_unlock(spinlock_t *lock);
extern INT32 spin_trylock(spinlock_t *lock);

#endif
