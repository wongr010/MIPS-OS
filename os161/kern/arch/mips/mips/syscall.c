#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <machine/pcb.h>
#include <machine/spl.h>
#include <machine/trapframe.h>
#include <kern/callno.h>
#include <syscall.h>
#include <kern/unistd.h>
#include <thread.h>
#include <curthread.h>
#include <kern/limits.h>

#include <clock.h>
#include "vm.h"

#define DUMBVM_STACKPAGES    24
#define  MIN_STACK 0x6FFFFFF // stack cannot go below this address (it only grows down) ???
// expand stack region
#define STACKPAGES PAGETABLE_SIZE 
#define STACK_OD_VSTART (USERSTACK-PAGE_SIZE*STACKPAGES)
/*
 * System call handler.
 *
 * A pointer to the trapframe created during exception entry (in
 * exception.S) is passed in.
 *
 * The calling conventions for syscalls are as follows: Like ordinary
 * function calls, the first 4 32-bit arguments are passed in the 4
 * argument registers a0-a3. In addition, the system call number is
 * passed in the v0 register.
 *
 * On successful return, the return value is passed back in the v0
 * register, like an ordinary function call, and the a3 register is
 * also set to 0 to indicate success.
 *
 * On an error return, the error code is passed back in the v0
 * register, and the a3 register is set to 1 to indicate failure.
 * (Userlevel code takes care of storing the error code in errno and
 * returning the value -1 from the actual userlevel syscall function.
 * See src/lib/libc/syscalls.S and related files.)
 *
 * Upon syscall return the program counter stored in the trapframe
 * must be incremented by one instruction; otherwise the exception
 * return code will restart the "syscall" instruction and the system
 * call will repeat forever.
 *
 * Since none of the OS/161 system calls have more than 4 arguments,
 * there should be no need to fetch additional arguments from the
 * user-level stack.
 *
 * Watch out: if you make system calls that have 64-bit quantities as
 * arguments, they will get passed in pairs of registers, and not
 * necessarily in the way you expect. We recommend you don't do it.
 * (In fact, we recommend you don't use 64-bit quantities at all. See
 * arch/mips/include/types.h.)
 */

// notice: below is bad, fix it later
#define KERN_PTR ((void *)0x80000000) /* addr within kernel */
#define INVAL_PTR ((void *)0x40000000) /* addr not part of program */

void
mips_syscall(struct trapframe *tf) {
    int callno;
    int32_t retval;
    int err;

    assert(curspl == 0);

    callno = tf->tf_v0;
    time_t seconds;
    unsigned long nanoseconds;

    /*
     * Initialize retval to 0. Many of the system calls don't
     * really return a value, just 0 for success and -1 on
     * error. Since retval is the value returned on success,
     * initialize it to 0 by default; thus it's not necessary to
     * deal with it except for calls that return other values, 
     * like write.
     */

    retval = 0;

    switch (callno) { //the trapframe has different call numbers and depending on that, do something
        case SYS_reboot:
            err = sys_reboot(tf->tf_a0);
            break;
        case SYS_write:
            err = sys_write(tf->tf_a0, (void *) tf->tf_a1, tf->tf_a2);
            if (err != 0) retval = err;
            else retval = tf->tf_a2;
            break;
        case SYS_read:
            err = sys_read(tf->tf_a0, (void *) tf->tf_a1, tf->tf_a2);
            if (err != 0) retval = err;
            else retval = tf->tf_a2;
            break;
        case SYS_fork:
            err = sys_fork(tf, &retval);

            break;
        case SYS_getpid:

            err = sys_getpid(&retval);
            break;
        case SYS__exit:
            sys_exit(tf->tf_a0);
            break;
        case SYS_waitpid:
            //err=sys_waitpid(tf->tf_a0, tf->tf_a1, tf->tf_a2, &retval);

            err = sys_waitpid((pid_t) tf->tf_a0, (int*) tf-> tf_a1, tf->tf_a2, &retval);
            break;
        case SYS___time:

            err = sys__time((userptr_t) tf->tf_a0, (userptr_t) tf-> tf_a1, &retval); //should assign to err?


            break;
        case SYS_execv:
            //err = execv(tf->tf_a0, tf->tf_a1);
            err = sys_execv((char*) tf->tf_a0, (char **) tf->tf_a1, &retval);
            //err = sys_execv((const char*)tf->tf_a0, (char **)tf->tf_a1);
            break;
        case SYS_sbrk:
            err = sys_sbrk(tf->tf_a0, &retval);
            break;
        default:
            kprintf("Unknown syscall %d\n", callno);
            err = ENOSYS;
            break;
    }


    if (err) {
        /*
         * Return the error code. This gets converted at
         * userlevel to a return value of -1 and the error
         * code in errno.
         */
        tf->tf_v0 = err;
        tf->tf_a3 = 1; /* signal an error */
    } else {
        /* Success. */

        tf->tf_v0 = retval;
        tf->tf_a3 = 0; /* signal no error */
    }

    /*
     * Now, advance the program counter, to avoid restarting
     * the syscall over and over again.
     */

    tf->tf_epc += 4; //every instruction is 4 bytes

    /* Make sure the syscall code didn't forget to lower spl */
    assert(curspl == 0);
}

void
md_forkentry(struct trapframe *tf, unsigned long child_addrspace) {
    /*
     * This function is provided as a reminder. You need to write
     * both it and the code that calls it.
     *
     * Thus, you can trash it and do things another way if you prefer.
     */

    tf->tf_v0 = 0;
    tf->tf_a3 = 0;
    tf->tf_epc += 4; //increment instruction pointer
    curthread->t_vmspace = (struct addrspace*) child_addrspace;
    as_activate(curthread->t_vmspace);
    struct trapframe childtrap = *tf;
    kfree(tf);

    //lock_acquire(table[curthread->threadPID].exiting_parent); //Make sure the parent can't leave
    mips_usermode(&childtrap); //new thread goes into user mode


}

// sys_write(tf->tf_a0, (void *) tf->tf_a1, tf->tf_a2);

int sys_write(int fd, const void *buf, size_t nbytes) {

    if (fd != STDOUT_FILENO && fd != STDERR_FILENO) {
        //kprintf("EBADF error ");
        return EBADF;
    }

    char *array;
    array = kmalloc(sizeof (char) * nbytes);
    int result = copyin(buf, array, sizeof (char)*nbytes);
    if (result == EFAULT) return EFAULT;
    int i;
    for (i = 0; i < nbytes; i++) {
        putch(array[i]);
    }
    kfree(array);


    return 0;
}

int sys_read(int fd, void *buf, size_t buflen) {
    if (fd != STDIN_FILENO) return EBADF;

    char *array;
    array = kmalloc(sizeof (char) * buflen);

    int i = 0;
    for (i = 0; i < buflen; i++) {
        array[i] = getch();
    }
    int result = copyout(array, buf, sizeof (buf));
    if (result == EFAULT) return EFAULT;
    kfree(array);
    return 0;

}

pid_t sys_fork(struct trapframe *tf, int* retval) {
    //************ copy from parent's tf to child's tf

    int parent_id = curthread->threadPID;
    struct trapframe *child;
    struct thread *child_thread;
    child = kmalloc(sizeof (struct trapframe));
    if (child == NULL) {
        //kprintf("Could not allocate child tf \n");
        *retval = ENOMEM;
        return -1;
    }

    *child = *tf; //copy parent trapframe into child

    //************ copy from parent's as to child's as

    struct addrspace* parent_addrspace = curthread->t_vmspace;
    struct addrspace* child_addrspace; //address space for child




    int addresscopy = as_copy(parent_addrspace, &child_addrspace);
    if (addresscopy != 0) {
        //  kprintf("No addresscopy \n");
        *retval = addresscopy;
        return -1;
    }

    //************ create child's thread

    int forkreturn = thread_fork(curthread->t_name, (struct trapframe*) child, (unsigned long) child_addrspace, md_forkentry, &child_thread);

    if (forkreturn != 0) {
        //  kprintf("No forkreturn\n");
        *retval = forkreturn;
        return -1;
    }

    table[child_thread->threadPID].parentPID = parent_id;
    child_thread->threadparentPID = parent_id;

    *retval = child_thread->threadPID;
    //kprintf("\n%d forking from %d \n", parent_id, child_thread->threadPID);

    return 0;


}

pid_t
sys_getpid(int *retval) {
    *retval = curthread->threadPID;
    return 0;

}

//sys_waitpid(tf->tf_a0, tf->tf_a1, tf->tf_a2, &retval);

pid_t sys_waitpid(pid_t PID, int *status, int options, int *retval) {
    //status=error code
    //should restrict the parent forks going into this process to only call waitpid on children?

    if (options != 0) {

        return EINVAL;

    }

    if (status == NULL) {
        return EFAULT;
    }
    if (PID < 0 || PID >= TOTAL_PROCESSES) {

        return EINVAL;
    }

    if (PID == 0) {
        //  kprintf("\nHerre7\n");
        *status = 0;
        return EINVAL;
    }


    int *memcheck = kmalloc(sizeof (int));
    int err = copyin(status, memcheck, sizeof (int));
    if (err != 0) {
        kfree(memcheck);
        *retval = -1;
        return err;
    }

    if ((int) status % 4 != 0) {

        return EINVAL;
    }


    if (table[(int) PID].processthread == NULL) {
        //  kprintf("\nHerre2\n");
        return EINVAL;
    }

    if (table[(int) PID].waiting == 1) {

        return EINVAL;
    }
    if (PID == curthread->threadPID) { //waiting on itself

        return EINVAL;
    }

    if (PID == table[curthread->threadPID].parentPID) {

        return EINVAL;
    }


    if (table[PID].exited == 1) {
        *status = table[PID].exitcode;
        *retval = PID;
        return 0;
    }



    lock_acquire(table[PID].exitpermission); //this should always be able to be acquired
    cv_wait(table[PID].exitcv, table[PID].exitpermission);
    lock_release(table[PID].exitpermission);
    *status = table[PID].exitcode; //should exit code be stored globally?

    table[PID].waiting = 1;




    *retval = PID;




    return 0;

}

void sys_exit(int exitcode) {

    int exiting_pid = curthread->threadPID;
    lock_acquire(table[exiting_pid].exitpermission);

    cv_broadcast(table[exiting_pid].exitcv, table[exiting_pid].exitpermission);

    lock_release(table[exiting_pid].exitpermission);


    table[exiting_pid].exited = 1;


    table[exiting_pid].exitcode = exitcode;

    thread_exit();



    //waitPID... then set this table[exiting_pid] to not occupied
}

int sys_execv(const char *program, char **args, int* retval) {

    // check for badcall
    int result;

    if (program == NULL || args == NULL) {
        *retval = -1;
        return EFAULT; //One of the args is an invalid pointer.
    }

    int errLen = sizeof (int);
    int* errKPtr = kmalloc(errLen);

    // check program


    result = copyin(program, errKPtr, errLen);
    if (result) {
        *retval = -1;
        kfree(errKPtr);
        return result;
    }


    // check arg
    result = copyin(args, errKPtr, errLen);
    if (result) {
        *retval = -1;
        kfree(errKPtr);
        return result;
    }

    if (program == INVAL_PTR || args == INVAL_PTR) {
        *retval = -1;
        return EFAULT; //One of the args is an invalid pointer.
    } else if (program == KERN_PTR || args == KERN_PTR) {
        *retval = -1;
        return EFAULT; //One of the args is an invalid pointer.
    }


    if (strlen(program) == 0) {
        *retval = -1;
        return EINVAL;
    }


    // determine number of arguments
    int i = 0;
    while (args[i] != NULL) {
        i++;
    }


    int nArg = i;
    // bad style, fix later
    if (args[1] == INVAL_PTR || args[1] == KERN_PTR) {
        *retval = -1;
        return EFAULT; //One of the args is an invalid pointer.
    }
    if (args[1] == NULL) {
        *retval = -1;
        return EFAULT; //One of the args is an invalid pointer.
    }



    //******************* declare variables
    struct vnode *v;
    vaddr_t entrypoint, stackptr;

    int spl;
    spl = splhigh(); // disable interrupts 

    //******************* copy from user to kernel
    char* progNameKPtr = (char*) kmalloc(NAME_MAX);
    if (progNameKPtr == NULL) {
        splx(spl);
        *retval = -1;
        
        return ENOMEM; //Insufficient virtual memory is available.
    }
    size_t actLen;
    result = copyinstr((userptr_t) program, progNameKPtr, NAME_MAX, &actLen);
    if (result) {
        splx(spl);
        *retval = -1;
        return EFAULT; //One of the args is an invalid pointer.
    }

    // determine number of arguments
    i = 0;
    while (args[i] != NULL) {
        i++;
    }


    nArg = i;

    int nArg_w_Null = nArg + 1; // number of arguments with the null terminator
    // allocate kernel mem space for arguments
    char** argKPtr = (char**) kmalloc(nArg_w_Null * sizeof (char*));
    if (argKPtr == NULL) {
        
        *retval = -1;
        kfree(progNameKPtr);
        splx(spl);
        return ENOMEM; //Insufficient virtual memory is available.
    }

    // copy arguments to argKPtr kernel space
    for (i = 0; i <= nArg; i++) {
        if (i < nArg) {
            int argLen = (strlen(args[i])) + 1; // add 1 for terminator: '\0'
            // allocate mem for one argument string
            argKPtr[i] = (char*) kmalloc(argLen);

            if (argKPtr[i] == NULL) {
                int n;
                for (n = 0; n < i; n++) { // deallocate allocated argument strings
                    kfree(argKPtr[n]);
                }
                kfree(argKPtr);
                kfree(progNameKPtr);
                *retval = -1;
                splx(spl);
                return ENOMEM; //Insufficient virtual memory is available.
            }
            // copy to argKPtr
            result = copyinstr((userptr_t) args[i], argKPtr[i], argLen, &actLen);
            if (result) {
                *retval = -1;
                splx(spl);
                return result;
            }

        }
    }

    //******************* create new address space for the new process
    // open file
    result = vfs_open(progNameKPtr, O_RDONLY, &v);
    if (result) {
        *retval = -1;
        splx(spl);
        return result;
    }

    // create address space
    as_destroy(curthread->t_vmspace);

    curthread->t_vmspace = NULL;
    //assert(curthread->t_vmspace == NULL);

    curthread->t_vmspace = as_create();
    if (curthread->t_vmspace == NULL) {
        vfs_close(v);
        *retval = -1;
        splx(spl);
        return ENOMEM;
    }

    as_activate(curthread->t_vmspace);
    result = load_elf(v, &entrypoint);
    if (result) {
        kfree(progNameKPtr);

        int n;
        for (n = 0; n < nArg; n++) { // deallocate allocated argument strings
            kfree(argKPtr[n]);
        }

        kfree(argKPtr);
        vfs_close(v);
        *retval = -1;
        splx(spl);
        return result;
    }

    vfs_close(v);

    //******************* define stackptr for the new process
    result = as_define_stack(curthread->t_vmspace, &stackptr);
    if (result) {
        kfree(progNameKPtr);

        int n;
        for (n = 0; n < nArg; n++) { // deallocate allocated argument strings
            kfree(argKPtr[n]);
        }

        kfree(argKPtr);
        vfs_close(v);
        *retval = -1;
        splx(spl);
        return result;
    }
    //******************* copy arg from kernel to user stack

    int argPtrTable[nArg]; //for storing stackptr for each argument
    for (i = nArg - 1; i >= 0; i--) {
        int argLen = strlen(argKPtr[i]) + 1;
        //int argLen = strlen(argKPtr[i]);

        //decrease stackptr in multiples of 4
        int remainder = argLen % 4;
        int memLen;
        if (remainder) {
            memLen = argLen + (4 - remainder);
        } else {
            memLen = argLen;
        }
        stackptr = stackptr - memLen;

        //copy from kernel to user stack
        copyoutstr(argKPtr[i], (userptr_t) stackptr, argLen, &actLen);
        argPtrTable[i] = stackptr;

    }

    //******************* copy arg ptr from kernel to user stack

    for (i = nArg; i >= 0; i--) {
        stackptr -= 4;
        copyout(&argPtrTable[i], (userptr_t) stackptr, sizeof (argPtrTable[i]));
    }

    //******************* deallocate kernel mem
    kfree(progNameKPtr);

    int n;
    for (n = 0; n < nArg; n++) { // deallocate allocated argument strings
        kfree(argKPtr[n]);
    }

    kfree(argKPtr);
    splx(spl);

    /* Warp to user mode. */
    md_usermode(nArg/*argc*/, (userptr_t) stackptr/*userspace addr of stackptr*/,
            stackptr, entrypoint);


    /* md_usermode does not return */
    panic("md_usermode returned\n");
    *retval = -1;
    return EINVAL;


}

time_t sys__time(time_t *seconds, unsigned long *nanoseconds, int* retval) {


    time_t *sec = kmalloc(sizeof (seconds));
    unsigned long* nsec = kmalloc(sizeof (nanoseconds));
    gettime(sec, nsec);

    if (seconds != NULL) {
        int err = copyout(sec, seconds, sizeof (time_t));
        if (err) return EFAULT;
    }
    if (nanoseconds != NULL) {
        int err = copyout(nsec, nanoseconds, sizeof (unsigned long));
        if (err) return EFAULT;
    }

    *retval = *sec;
    //kprintf("after assignment");
    kfree(sec);
    kfree(nsec);


    return 0;


}

void* sys_sbrk(intptr_t amount, int* retval) { //malloc
    struct addrspace* as = curthread->t_vmspace; //current addrspace

    kprintf("sys_sbrk: amount is %d\n", amount);

//    if (amount < 0) {
//        return EINVAL;
//    }
    
    if(as->heap->vend_actual+amount<as->heap->vstart){
        return EINVAL;
    }
    
    
    if(as->heap->vend_actual+amount>STACK_OD_VSTART){
        *retval=-1;
        return ENOMEM; //heap grew into the stack!
    }
    
    if(amount>num_empty_cm()*PAGE_SIZE){
        *retval=-1;
        return ENOMEM;
    }
    

    *retval = as->heap->vend_actual;
    as->heap->vend_actual += amount;
    
    return 0;

}