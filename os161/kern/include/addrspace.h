#ifndef _ADDRSPACE_H_
#define _ADDRSPACE_H_

#include <vm.h>
#include "synch.h"
#include "opt-dumbvm.h"





/* 
 * Address space - data structure associated with the virtual memory
 * space of a process.
 *
 * You write this.
 */


/*
 * todo:
 * addr_region: pte
 * 
 
 
 
 */

struct coremapEntry* coremap;

struct addrspace_region {
    vaddr_t vstart;
    vaddr_t vend; //only needed for stack and heap
    size_t numofpages;
    paddr_t pstart;
    paddr_t pend; //only needed for stack
    //int perm; //0 is r/w, 1 is r/w/e
    struct pte* pagetable;
    
    vaddr_t vend_actual; // for heap

};

struct addrspace {
#if OPT_DUMBVM
    vaddr_t as_vbase1;
    paddr_t as_pbase1;
    size_t as_npages1;
    vaddr_t as_vbase2;
    paddr_t as_pbase2;
    size_t as_npages2;
    paddr_t as_stackpbase;
#else
    struct addrspace_region *text;
    struct addrspace_region *data;
    struct addrspace_region *heap;
    struct addrspace_region *stack;

#endif          
};


/*
 * Functions in addrspace.c:
 *
 *    as_create - create a new empty address space. You need to make 
 *                sure this gets called in all the right places. You
 *                may find you want to change the argument list. May
 *                return NULL on out-of-memory error.
 *
 *    as_copy   - create a new address space that is an exact copy of
 *                an old one. Probably calls as_create to get a new
 *                empty address space and fill it in, but that's up to
 *                you.
 *
 *    as_activate - make the specified address space the one currently
 *                "seen" by the processor. Argument might be NULL, 
 *		  meaning "no particular address space".
 *
 *    as_destroy - dispose of an address space. You may need to change
 *                the way this works if implementing user-level threads.
 *
 *    as_define_region - set up a region of memory within the address
 *                space.
 *
 *    as_prepare_load - this is called before actually loading from an
 *                executable into the address space.
 *
 *    as_complete_load - this is called when loading from an executable
 *                is complete.
 *
 *    as_define_stack - set up the stack region in the address space.
 *                (Normally called *after* as_complete_load().) Hands
 *                back the initial stack pointer for the new process.
 */

struct addrspace *as_create(void);
int as_define_region(struct addrspace *as,
        vaddr_t vaddr, size_t sz,
        int readable,
        int writeable,
        int executable);
int as_prepare_load(struct addrspace *as);

int as_copy(struct addrspace *src, struct addrspace **ret);
void as_activate(struct addrspace *);
void as_destroy(struct addrspace *);



int as_complete_load(struct addrspace *as);
int as_define_stack(struct addrspace *as, vaddr_t *initstackptr);

/*
 * Functions in loadelf.c
 *    load_elf - load an ELF user program executable into the current
 *               address space. Returns the entry point (initial PC)
 *               in the space pointed to by ENTRYPOINT.
 */

int load_elf(struct vnode *v, vaddr_t *entrypoint);


void free_coremap();
void free_addrspace(struct addrspace *as);
void free_addrspace_region(struct addrspace_region *region);
//paddr_t free_addrspace_region(struct addrspace_region *region, int *numofpages);
void print_coremap(); // print the current status of the coremap, to indicate if a page frame is allocated
void print_addrspace_region(struct addrspace_region* region);
void print_addrspace(struct addrspace* as);

// pagetable functions:
void pagetable_initialize(struct addrspace_region* region);
int access_pagetable(struct addrspace_region* region, vaddr_t faultaddress, int spl);
void free_pagetable(struct addrspace *as);


#endif /* _ADDRSPACE_H_ */




// TO DO:
/*
 * as_copy
 * 
 * CRASH?
 * 
 * malloc: sbrk
 * 
 * big program
 * 
 */