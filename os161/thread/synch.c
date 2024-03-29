/*
 * Synchronization primitives.
 * See synch.h for specifications of the functions.
 */

#include <types.h>
#include <lib.h>
#include <synch.h>
#include <thread.h>
#include <curthread.h>
#include <machine/spl.h>

////////////////////////////////////////////////////////////
//
// Semaphore.

struct semaphore *
sem_create(const char *namearg, int initial_count) {
    struct semaphore *sem;

    assert(initial_count >= 0);

    sem = kmalloc(sizeof (struct semaphore));
    if (sem == NULL) {
        return NULL;
    }

    sem->name = kstrdup(namearg);
    if (sem->name == NULL) {
        kfree(sem);
        return NULL;
    }

    sem->count = initial_count;

    sem->isOccupiedByMice = 0;
    sem->isOccupiedByCat = 0;

    return sem;
}

void
sem_destroy(struct semaphore *sem) {
    int spl;
    assert(sem != NULL);

    spl = splhigh();
    assert(thread_hassleepers(sem) == 0);
    splx(spl);

    /*
     * Note: while someone could theoretically start sleeping on
     * the semaphore after the above test but before we free it,
     * if they're going to do that, they can just as easily wait
     * a bit and start sleeping on the semaphore after it's been
     * freed. Consequently, there's not a whole lot of point in 
     * including the kfrees in the splhigh block, so we don't.
     */

    kfree(sem->name);
    //kfree(sem->isOccupiedByMice);
    //kfree(sem->isOccupiedByCat);
    kfree(sem);
}

void
P(struct semaphore *sem) {
    int spl;
    assert(sem != NULL);

    /*
     * May not block in an interrupt handler.
     *
     * For robustness, always check, even if we can actually
     * complete the P without blocking.
     */
    assert(in_interrupt == 0);

    spl = splhigh();
    while (sem->count == 0) {
        thread_sleep(sem);
    }

    assert(sem->count > 0);
    sem->count--;
    splx(spl);
}

void
V(struct semaphore *sem) {
    int spl;
    assert(sem != NULL);
    spl = splhigh();
    sem->count++;
    assert(sem->count > 0);
    thread_wakeup(sem);
    splx(spl);
}







void
Pcat(struct semaphore *curSem, struct semaphore *anotherSem) {
    int spl;
    assert(curSem != NULL);

    /*
     * May not block in an interrupt handler.
     *
     * For robustness, always check, even if we can actually
     * complete the P without blocking.
     */
    assert(in_interrupt == 0);

    spl = splhigh();

    while ((curSem->count == 0)|(anotherSem->isOccupiedByMice == 1)) {
        thread_sleep(curSem);
    }

    assert(curSem->count > 0);
    
    curSem->isOccupiedByCat = 1;
    curSem->count--;


    splx(spl);
}

void
Pmouse(struct semaphore *curSem, struct semaphore *anotherSem) {
    int spl;
    assert(curSem != NULL);

    /*
     * May not block in an interrupt handler.
     *
     * For robustness, always check, even if we can actually
     * complete the P without blocking.
     */
    assert(in_interrupt == 0);

    spl = splhigh();

    
    while ((curSem->count == 0) |(anotherSem->isOccupiedByCat == 1)) {
        thread_sleep(curSem);
    }
    

    assert(curSem->count > 0);
    curSem->isOccupiedByMice = 1;
    curSem->count--;

    splx(spl);
}

void
Vcat(struct semaphore *curSem, struct semaphore *anotherSem) {
    int spl;
    assert(curSem != NULL);
    spl = splhigh();
    curSem->count++;
    curSem->isOccupiedByCat = 0;
    assert(curSem->count > 0);
    thread_wakeup(curSem);
    splx(spl);
}

void
Vmouse(struct semaphore *curSem, struct semaphore *anotherSem) {
    int spl;
    assert(curSem != NULL);
    spl = splhigh();
    curSem->count++;
    curSem->isOccupiedByMice = 0;
    assert(curSem->count > 0);
    thread_wakeup(curSem);
    splx(spl);
}

////////////////////////////////////////////////////////////
//
// Lock.

struct lock *
lock_create(const char *name) {
    struct lock *lock;

    lock = kmalloc(sizeof (struct lock));
    if (lock == NULL) {
        return NULL;
    }

    lock->name = kstrdup(name);
    if (lock->name == NULL) {
        kfree(lock);
        return NULL;
    }
    lock->owner = NULL;
    // add stuff here as needed

    return lock;
}

void
lock_destroy(struct lock *lock) {
    assert(lock != NULL);

    // add stuff here as needed
    kfree(lock->owner);
    kfree(lock->name);
    kfree(lock);
}

void
lock_acquire(struct lock *lock) {
    // Write this
    int spl;
    assert(lock != NULL);
    spl = splhigh();
    while (lock->owner != NULL) thread_sleep(lock); //thread sleeping (waiting) on this lock

    lock->owner = curthread; //assign the lock to the current thread

    splx(spl);
}

void
lock_release(struct lock *lock) {
    assert(lock != NULL);
    int spl = splhigh();
    assert(curthread == lock->owner); //current thread holds this lock
    lock->owner = NULL;
    thread_wakeup(lock); //this function loops through the sleeping threads and wakes up one that is sleeping on this lock
    splx(spl);
    // Write this


}

int
lock_do_i_hold(struct lock *lock) {
    assert(lock != NULL);
    //int spl = splhigh();
    // Write this
    if (lock->owner == curthread) return 1;

    else return 0;


}

////////////////////////////////////////////////////////////
//
// CV

struct cv *
cv_create(const char *name) {
    struct cv *cv;

    cv = kmalloc(sizeof (struct cv));
    if (cv == NULL) {
        return NULL;
    }

    cv->name = kstrdup(name);
    if (cv->name == NULL) {
        kfree(cv);
        return NULL;
    }

    // add stuff here as needed

    return cv;
}

void
cv_destroy(struct cv *cv) {
    assert(cv != NULL);

    // add stuff here as needed

    kfree(cv->name);
    kfree(cv);
}

void
cv_wait(struct cv *cv, struct lock *lock) {
    assert(cv != NULL);
    assert(lock != NULL);
    int spl = splhigh();
    lock_release(lock);
    thread_sleep(cv); //sleep until condition variable satisfied

    lock_acquire(lock); //take the lock back
    splx(spl);
    // Write this

}

void
cv_signal(struct cv *cv, struct lock *lock) {
    assert(cv != NULL);
    assert(lock != NULL);
    int spl = splhigh();
    thread_onewakeup(cv);
    splx(spl);
}

void
cv_broadcast(struct cv *cv, struct lock *lock) {
    assert(cv != NULL);
    assert(lock != NULL);
    int spl = splhigh();
    thread_wakeup(cv);
    splx(spl);
}
