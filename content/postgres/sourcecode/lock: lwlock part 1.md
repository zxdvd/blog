<!---
tags: postgres, sourcecode, lock, lwlock
-->

There are many processes in a postgres cluster. Lwlock is a very important interprocess lock.

A simple example at autovacuum.c:

```c
    LWLockAcquire(AutovacuumLock, LW_EXCLUSIVE);
    autovac_balance_cost();
    LWLockRelease(AutovacuumLock);
```

There may have multiple vacuum workers, so to change shared variables you need a lock to 
protect it. And `AutovacuumLock` is the lock for autovacuum, `LWLockAcquire` and `LWLockRelease`
 are related functions. They behavior like pthread mutex.

### data structure
Let's see the related definitions:

```c
// src/include/storage/lwlock.h
typedef enum LWLockMode
{
	LW_EXCLUSIVE,
	LW_SHARED,
	LW_WAIT_UNTIL_FREE			/* A special mode used in PGPROC->lwWaitMode,
								 * when waiting for lock to become free. Not
								 * to be used as LWLockAcquire argument */
} LWLockMode
```

There are 3 kinds of lock mode, but only two could be used: LW_EXCLUSIVE and LW_SHARED. 
LW_WAIT_UNTIL_FREE is used internally.

```c
// src/include/storage/lwlock.h
typedef struct LWLock
{
	uint16		tranche;		/* tranche ID */
	pg_atomic_uint32 state;		/* state of exclusive/nonexclusive lockers */
	proclist_head waiters;		/* list of waiting PGPROCs */
} LWLock
```

This is lock struct with debug related info omitted. Postgres will initialize all needed locks.
Each have id and name. `tranche` is the id. `state` indicates whether it's a RO (shared) lock 
or a RW (exclusive) lock. `waiters` is a list of processes that waiting on this lock.

```c
// src/backend/storage/lmgr/lwlock.c
#define LW_VAL_EXCLUSIVE			((uint32) 1 << 24)
#define LW_VAL_SHARED				1
```

So if `state & LW_VAL_EXCLUSIVE != 0`, it indicates it's a RW lock. However, for RO lock, it's 
different. A RO lock can be locked multiple times, the `state` will add `LW_VAL_SHARED` each 
time locked. Since it shares a single uint32 with RW lock, it must not be large then `1 << 24`.

```c
// src/backend/storage/lmgr/lwlock.c
#define LW_FLAG_HAS_WAITERS			((uint32) 1 << 30)
#define LW_FLAG_RELEASE_OK			((uint32) 1 << 29)
#define LW_FLAG_LOCKED				((uint32) 1 << 28)
```
The `LW_FLAG_HAS_WAITERS` flag in `state` indicates that whether the wait queue is empty or not.
Multiple processes may use same lock. These two are used to lock the lock itself. They are also 
stored at the `state`.


### LWLockAttemptLock
Main code of the `LWLockAttemptLock` function, the `LWLockAcquire` relies on it. It try to get the 
lock if available. If it is locked, it returns `true` which means that the caller need to wait the 
lock.

```C
static bool LWLockAttemptLock(LWLock *lock, LWLockMode mode) {
	uint32		old_state;

	AssertArg(mode == LW_EXCLUSIVE || mode == LW_SHARED);

	old_state = pg_atomic_read_u32(&lock->state);

    // though old_state may be lockable, other may get the during the gap of read and CAS update
    // so need a while(true) to loop, the loop is very fast
	while (true)
	{
		uint32		desired_state;
		bool		lock_free;         // false means it is locked

		desired_state = old_state;

		if (mode == LW_EXCLUSIVE)
		{
            // for RW mode, lock_free means no RW or RO lock, thus 0~24 bit are all 0
			lock_free = (old_state & LW_LOCK_MASK) == 0;
			if (lock_free)
				desired_state += LW_VAL_EXCLUSIVE;
		}
		else
		{
            // for RO lock, it only conflicts with the RW bit, so only check one bit
            // and each time, inc the shared counter by 1
			lock_free = (old_state & LW_VAL_EXCLUSIVE) == 0;
			if (lock_free)
				desired_state += LW_VAL_SHARED;
		}

		/*
		 * Attempt to swap in the state we are expecting. If we didn't see
		 * lock to be free, that's just the old value. If we saw it as free,
		 * we'll attempt to mark it acquired. The reason that we always swap
		 * in the value is that this doubles as a memory barrier. We could try
		 * to be smarter and only swap in values if we saw the lock as free,
		 * but benchmark haven't shown it as beneficial so far.
		 *
		 * Retry if the value changed since we last looked at it.
		 */
         // update state atomically
         // attention, old_state is also refreshed here, so no need to get old_state at begin of the while
		if (pg_atomic_compare_exchange_u32(&lock->state,
										   &old_state, desired_state))
		{
			if (lock_free)
			{
				/* Great! Got the lock. */
				return false;
			}
			else
				return true;	/* somebody else has the lock */
		}
	}
    // should never get here, otherwise you got a bug
	pg_unreachable();
}
```


### LWLockAcquire
`LWLockAcquire` try to get the lock. If it's not available, it will sleep and try again.
The code and comment are easy to read. Though the comment has explained why it needs to 
do the second `LWLockAttemptLock`, I'll give it more details. 

Following may happen ocasionally:

```
        process A                          process B
        have the lock with empty queue
        have the lock                      try to acquire lock, failed since A has it
        release the lock
        released (wakeup nobody since queue is empty)
                                           add self to wait queue since failed
                                           sleep (wait until others wakeup it)
```
At above scenario, process B sleeps and waits the lock but actually the lock is free to 
acquire. To avoid this, a second `LWLockAttemptLock` is needed. It will get lock if 
available. Otherwise, other process holds the lock and it will wakeup queued processes.

```c
bool LWLockAcquire(LWLock *lock, LWLockMode mode) {
	PGPROC	   *proc = MyProc;
	bool		result = true;
	int			extraWaits = 0;

	AssertArg(mode == LW_SHARED || mode == LW_EXCLUSIVE);

	/* Ensure we will have room to remember the lock */
	if (num_held_lwlocks >= MAX_SIMUL_LWLOCKS)
		elog(ERROR, "too many LWLocks taken");

	for (;;)
	{
		bool		mustwait;

		/*
		 * Try to grab the lock the first time, we're not in the waitqueue
		 * yet/anymore.
		 */
		mustwait = LWLockAttemptLock(lock, mode);

		if (!mustwait)
		{
			break;				/* got the lock */
		}

		/*
		 * Ok, at this point we couldn't grab the lock on the first try. We
		 * cannot simply queue ourselves to the end of the list and wait to be
		 * woken up because by now the lock could long have been released.
		 * Instead add us to the queue and try to grab the lock again. If we
		 * succeed we need to revert the queuing and be happy, otherwise we
		 * recheck the lock. If we still couldn't grab it, we know that the
		 * other locker will see our queue entries when releasing since they
		 * existed before we checked for the lock.
		 */

		/* add to the queue */
		LWLockQueueSelf(lock, mode);

		/* we're now guaranteed to be woken up if necessary */
		mustwait = LWLockAttemptLock(lock, mode);

		/* ok, grabbed the lock the second time round, need to undo queueing */
		if (!mustwait)
		{
			LWLockDequeueSelf(lock);
			break;
		}

		/*
		 * Wait until awakened.
		 *
		 * Since we share the process wait semaphore with the regular lock
		 * manager and ProcWaitForSignal, and we may need to acquire an LWLock
		 * while one of those is pending, it is possible that we get awakened
		 * for a reason other than being signaled by LWLockRelease. If so,
		 * loop back and wait again.  Once we've gotten the LWLock,
		 * re-increment the sema by the number of additional signals received,
		 * so that the lock manager or signal manager will see the received
		 * signal when it next waits.
		 */

		LWLockReportWaitStart(lock);

		for (;;)
		{
			PGSemaphoreLock(proc->sem);
			if (!proc->lwWaiting)
				break;
			extraWaits++;
		}

		/* Retrying, allow LWLockRelease to release waiters again. */
		pg_atomic_fetch_or_u32(&lock->state, LW_FLAG_RELEASE_OK);

		LWLockReportWaitEnd();

		/* Now loop back and try to acquire lock again. */
		result = false;
	}


	/* Add lock to list of locks held by this backend */
	held_lwlocks[num_held_lwlocks].lock = lock;
	held_lwlocks[num_held_lwlocks++].mode = mode;

	/*
	 * Fix the process wait semaphore's count for any absorbed wakeups.
	 */
	while (extraWaits-- > 0)
		PGSemaphoreUnlock(proc->sem);

	return result;
}
```

### LWLockRelease

```c
void LWLockRelease(LWLock *lock) {
	LWLockMode	mode;
	uint32		oldstate;
	bool		check_waiters;
	int			i;

	/*
	 * Remove lock from list of locks held.  Usually, but not always, it will
	 * be the latest-acquired lock; so search array backwards.
	 */
	for (i = num_held_lwlocks; --i >= 0;)
		if (lock == held_lwlocks[i].lock)
			break;

	if (i < 0)
		elog(ERROR, "lock %s is not held", T_NAME(lock));

	mode = held_lwlocks[i].mode;

	num_held_lwlocks--;
	for (; i < num_held_lwlocks; i++)
		held_lwlocks[i] = held_lwlocks[i + 1];


	/*
	 * Release my hold on lock, after that it can immediately be acquired by
	 * others, even if we still have to wakeup other waiters.
	 */
	if (mode == LW_EXCLUSIVE)
		oldstate = pg_atomic_sub_fetch_u32(&lock->state, LW_VAL_EXCLUSIVE);
	else
		oldstate = pg_atomic_sub_fetch_u32(&lock->state, LW_VAL_SHARED);

	/* nobody else can have that kind of lock */
	Assert(!(oldstate & LW_VAL_EXCLUSIVE));


	/*
	 * We're still waiting for backends to get scheduled, don't wake them up
	 * again.
	 */
	if ((oldstate & (LW_FLAG_HAS_WAITERS | LW_FLAG_RELEASE_OK)) ==
		(LW_FLAG_HAS_WAITERS | LW_FLAG_RELEASE_OK) &&
		(oldstate & LW_LOCK_MASK) == 0)
		check_waiters = true;
	else
		check_waiters = false;

	/*
	 * As waking up waiters requires the spinlock to be acquired, only do so
	 * if necessary.
	 */
	if (check_waiters)
	{
		/* XXX: remove before commit? */
		LOG_LWDEBUG("LWLockRelease", lock, "releasing waiters");
		LWLockWakeup(lock);
	}
}
```
