// Sleep locks - Long-held locks for file system operations
#include "../include/types.h"
#include "../sync/spinlock.h"

#ifndef SLEEPLOCK_H
#define SLEEPLOCK_H

// Long-term locks for processes
// Sleep locks are used when a lock needs to be held for a long time.
// Unlike spinlocks, a process holding a sleep lock may sleep,
// which allows other processes to use the CPU.
struct sleeplock {
  uint locked;        // Is the lock held?
  struct spinlock lk; // spinlock protecting this sleep lock

  // For debugging:
  char *name; // Name of lock
  int pid;    // Process holding lock
};

#endif // SLEEPLOCK_H