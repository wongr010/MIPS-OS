#ifndef _VM_H_
#define _VM_H_

#include <machine/vm.h>

/*
 * VM system-related definitions.
 *
 * You'll probably want to add stuff here.
 */


/* Fault-type arguments to vm_fault() */
#define VM_FAULT_READ        0    /* A read was attempted */
#define VM_FAULT_WRITE       1    /* A write was attempted */
#define VM_FAULT_READONLY    2    /* A write to a readonly page was attempted*/
#define PAGETABLE_SIZE 512

struct vnode;

/* 
 * Address space - data structure associated with the virtual memory
 * space of a process.
 *
 * You write this.
 */

// declare pagetable and coremap

struct pte { // page table entry
    int allocated;
    vaddr_t vstart;
    vaddr_t vend;
    paddr_t pstart;
    paddr_t pend;
    
//    struct pte *next;
//    struct pte *prev;

//    int refcount; //permissions? is it a shared page or not

    //struct coremapEntry* coremapPtr;    
};




struct coremapEntry {
    //    struct pte *mypage;
    //    int tlb_index; //is the page and paddr in the tlb? -1 if not
    //    int kernpage; //is it a kernel page
    int allocated; //is allocated
//    int fixed; //if is a page containing coremap info, is fixed
    paddr_t paddr_base;
};

/* Initialization function */
void vm_bootstrap(void);

/* Fault handling function called by trap code */
int vm_fault(int faulttype, vaddr_t faultaddress);
int write_into_TLB(vaddr_t faultaddress, paddr_t paddr, int spl); 

/* Allocate/free kernel heap pages (called by kmalloc/kfree) */
vaddr_t alloc_kpages(int npages);
void free_kpages(vaddr_t addr);
int num_empty_cm();

//static 
paddr_t getppages(unsigned long npages);

#endif /* _VM_H_ */