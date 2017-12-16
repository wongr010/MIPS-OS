#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <thread.h>
#include <curthread.h>
#include <addrspace.h>
#include <vm.h>
#include <machine/spl.h>
#include <machine/tlb.h>

/*
 * Dumb MIPS-only "VM system" that is intended to only be just barely
 * enough to struggle off the ground. You should replace all of this
 * code while doing the VM assignment. In fact, starting in that
 * assignment, this file is not included in your kernel!
 */

/* under dumbvm, always have 48k of user stack */
#define DUMBVM_STACKPAGES    24
#define  MIN_STACK 0x6FFFFFF // stack cannot go below this address (it only grows down) ???
// expand stack region
#define STACKPAGES PAGETABLE_SIZE 
#define STACK_OD_VSTART (USERSTACK-PAGE_SIZE*STACKPAGES)

paddr_t pageFrameStart_pa;
paddr_t pageFrameEnd_pa;
int totalPageFrameNum;
int numCoremapEntry;
int beforeVM = 1;
int NPAGES; //changes value based on what process is running

paddr_t ROUND_DOWN(paddr_t address, int size) {
    return ((address) - ((address) % (size)));
}

paddr_t ROUND_UP(paddr_t address, int size) {
    return ((address) - ((address) % (size)) + (size));
}

// IMPORTANT: when print anything, make sure the interrupt is turned off!
// print the current status of the coremap, to indicate if a page frame is allocated
// allocated: 'X'
// not allocated: 

void print_coremap() {
    int i;
    for (i = 0; i < numCoremapEntry; i++) {
        // print coremap index
        if (i == 0) {
            kprintf("\n   %d    %d    %d    %d    %d    %d    %d    %d    %d    %d\n",
                    i, i + 1, i + 2, i + 3, i + 4, i + 5, i + 6, i + 7, i + 8, i + 9);
        } else if (i % 10 == 0) {
            kprintf("\n   %d   %d   %d   %d   %d   %d   %d   %d   %d   %d\n",
                    i, i + 1, i + 2, i + 3, i + 4, i + 5, i + 6, i + 7, i + 8, i + 9);
        }

        // print allocated status
        if (coremap[i].allocated == 0) {
            kprintf("     ");
        } else {
            kprintf("    %c", 'X');
        }
        if (i == numCoremapEntry - 1) {
            kprintf("\n");
        }
    }

}

void print_addrspace_region(struct addrspace_region* region) {
    kprintf("vstart: 0x%x\n", region->vstart);
    kprintf("vend: 0x%x\n", region->vend);
    kprintf("numofpages: %d\n", region->numofpages);
    kprintf("pstart: 0x%x\n", region->pstart);
    kprintf("pend: 0x%x\n", region->pend);


}

void print_addrspace(struct addrspace* as) {
    kprintf("-------- TEXT -------- \n");
    print_addrspace_region(as->text);
    kprintf("-------- DATA -------- \n");
    print_addrspace_region(as->data);
    kprintf("-------- HEAP -------- \n");
    print_addrspace_region(as->heap);
    kprintf("-------- STACK -------- \n");
    print_addrspace_region(as->stack);
    kprintf("\n");
}

// bootstrap coremap

void
vm_bootstrap(void) {
    // calculate how much space does ram take
    u_int32_t low, high;

    ram_getsize(&low, &high);
    //    kprintf("The lowest address is 0x%x, the highest is 0x%x\n", low, high);


    // calculate the coremap size
    int divisibleAddr_incl_cm = ROUND_DOWN((high - low), PAGE_SIZE);
    int nFrames_pre = divisibleAddr_incl_cm / PAGE_SIZE;
    int coremapSize = nFrames_pre * sizeof (struct coremapEntry);

    // calculate page frame related pa, size, etc.
    pageFrameStart_pa = ROUND_UP((low + coremapSize), PAGE_SIZE);
    int totalPageFrameSize = ROUND_DOWN((high - pageFrameStart_pa), PAGE_SIZE);
    totalPageFrameNum = totalPageFrameSize / PAGE_SIZE;
    pageFrameEnd_pa = pageFrameStart_pa + totalPageFrameSize;

    numCoremapEntry = totalPageFrameNum;
    paddr_t coremapStart_pa = low;
    paddr_t coremapEnd_pa = low + sizeof (struct coremapEntry) * numCoremapEntry;


    coremap = (struct coremapEntry *) PADDR_TO_KVADDR(coremapStart_pa); //
    // initialize coremap
    int i;
    for (i = 0; i < /*nFrames_pre*/numCoremapEntry; i++) {
        coremap[i].allocated = 0;
        coremap[i].paddr_base = pageFrameStart_pa + i*PAGE_SIZE;
    }



    //    kprintf("coremap_bootstrap-- coremap initialing with:\n");
    //    kprintf("number of frames available: %d\n", totalPageFrameNum);
    //    kprintf("pageFrameStart_pa: 0x%x       pageFrameEnd_pa: 0x%x\n", pageFrameStart_pa, coremap[numCoremapEntry-1].paddr_base);

    beforeVM = 0;
}

paddr_t
getppages(unsigned long npages) {
    int spl;
    paddr_t addr;

    spl = splhigh();

    if (beforeVM == 1) {
        splx(spl);
        return addr = ram_stealmem(npages);
    }


    //kprintf("--------------I'M GETTING PAGES!!---------------\n");


    int consecutiveCount = npages;
    int startPageFrameInd, endPageFrameInd;
    int i;
    for (i = 0; i < numCoremapEntry; i++) {
        if (consecutiveCount == 0) { // no page to allocate or the count is over
            endPageFrameInd = i - 1;
            startPageFrameInd = endPageFrameInd + 1 - npages;
            break;
        }
        if (coremap[i].allocated == 0) {
            consecutiveCount--;
        } else {
            if (consecutiveCount != 0) {
                //restart
                consecutiveCount = npages;
            }
        }
    }

    if (consecutiveCount != 0) {
        // cannot allocate memory for this process
    } else {
        // set the allocate flag to 1 for each page frame
        for (i = startPageFrameInd; i <= endPageFrameInd; i++) {
            coremap[i].allocated = 1;
        }
    }

    addr = coremap[startPageFrameInd].paddr_base;
   
       // kprintf("\nnpages is %d    ", npages);
       // kprintf("getppages: Vaddr is: 0x%x\n", PADDR_TO_KVADDR(addr));

    NPAGES = npages;

    splx(spl);
    return addr;
}

/* Allocate/free some kernel-space virtual pages */
vaddr_t
alloc_kpages(int npages) {
    paddr_t pa;

    pa = getppages(npages);
    if (pa == 0) {
        return 0;
    }

    return PADDR_TO_KVADDR(pa);
}

paddr_t alloc_upages(int npages) { //alloc physical pages
    paddr_t pa;
    pa = getppages(npages);
    if (pa == 0) {
        return 0;
    }


    return pa;
}

void
free_kpages(vaddr_t addr) {
    int i, spl;
    spl = splhigh();
    for (i = 0; i < numCoremapEntry; i++) {
        coremap[i].allocated = 0;
    }


    splx(spl);
}

void free_coremap() {
    int i, spl;
    spl = splhigh();
    for (i = 0; i < numCoremapEntry; i++) {
        coremap[i].allocated = 0;
    }


    splx(spl);

}

void free_addrspace_region(struct addrspace_region *region) {
    int spl;
    spl = splhigh();
    
    region->vstart = 0;
    region->vend = 0;
    region->numofpages = 0;
    region->pstart = 0;
    region->pend = 0;
    region->vend_actual = 0;
    
    // for stack and heap
    splx(spl);   
    vaddr_t vend_actual; // for stack
}

void free_addrspace(struct addrspace *as){
    int i, spl;
    spl = splhigh();
    
    free_addrspace_region(as->text);
    free_addrspace_region(as->data);
    free_addrspace_region(as->stack);
    free_addrspace_region(as->heap);
    
    free_pagetable(as);
    
    as->text = NULL;
    as->data = NULL;
    as->heap = NULL;
    as->stack = NULL;
    
    
    splx(spl);
    
}


void free_pagetable(struct addrspace *as) {
    int i, spl;
    spl = splhigh();

    // free stack pagetable
    for (i = 0; i < PAGETABLE_SIZE; i++) {
        if (as->stack->pagetable[i].allocated == 0) {
            break;
        }
        as->stack->pagetable[i].allocated = 0;
        as->stack->pagetable[i].vstart = 0;
        as->stack->pagetable[i].vend = 0;
        as->stack->pagetable[i].pstart = 0;
        as->stack->pagetable[i].pend = 0;
    }


    // free heap pagetable
    for (i = 0; i < PAGETABLE_SIZE; i++) {
        if (as->heap->pagetable[i].allocated == 0) {
            break;
        }
        as->heap->pagetable[i].allocated = 0;
        as->heap->pagetable[i].vstart = 0;
        as->heap->pagetable[i].vend = 0;
        as->heap->pagetable[i].pstart = 0;
        as->heap->pagetable[i].pend = 0;
    }
    
    for (i = 0; i < PAGETABLE_SIZE; i++) {
        if (as->heap->pagetable[i].allocated == 0) {
            break;
        }
        as->text->pagetable[i].allocated = 0;
        as->text->pagetable[i].vstart = 0;
        as->text->pagetable[i].vend = 0;
        as->text->pagetable[i].pstart = 0;
        as->text->pagetable[i].pend = 0;
    }
    
    for (i = 0; i < PAGETABLE_SIZE; i++) {
        if (as->heap->pagetable[i].allocated == 0) {
            break;
        }
        as->data->pagetable[i].allocated = 0;
        as->data->pagetable[i].vstart = 0;
        as->data->pagetable[i].vend = 0;
        as->data->pagetable[i].pstart = 0;
        as->data->pagetable[i].pend = 0;
    }
    
    
    as->text->pagetable = NULL;
    as->data->pagetable = NULL;
    as->heap->pagetable = NULL;
    as->stack->pagetable = NULL;


    splx(spl);

}

int access_pagetable(struct addrspace_region* region, vaddr_t faultaddress, int spl) {
    int i;
    int needNewPage = 0;
    struct pte* pagetable = region->pagetable;
    paddr_t paddr;
    
    // check if there is a page exists already
    for (i = 0; i < PAGETABLE_SIZE; i++) {
        if (!pagetable[i].allocated) {
            needNewPage = 1;
            break;
        }
       
        if (faultaddress >= pagetable[i].vstart && faultaddress < pagetable[i].vend) {
            // page found!
            paddr = (faultaddress - pagetable[i].vstart) + pagetable[i].pstart;

            if (write_into_TLB(faultaddress, paddr, spl) == 1) {
                return 1;
            }
            break;
        }
    }

    // not in pagetable, create a new page
    if (needNewPage) {
        vaddr_t newVstart = ROUND_DOWN(faultaddress, PAGE_SIZE);
        vaddr_t newVend = newVstart + PAGE_SIZE;
        paddr_t newPstart = getppages(1);
        paddr_t newPend = newPstart + PAGE_SIZE;

        // update this region's pagetable
        pagetable[i].allocated = 1;
        pagetable[i].vstart = newVstart;
        pagetable[i].vend = newVend;
        pagetable[i].pstart = newPstart;
        pagetable[i].pend = newPend;

        // assign corresponding paddr
        paddr = (faultaddress - pagetable[i].vstart) + pagetable[i].pstart;

        // writes into TLB
        if (write_into_TLB(faultaddress, paddr, spl) == 1) {
            return 1;
        }

    }

    return 0;

}

int write_into_TLB(vaddr_t faultaddress, paddr_t paddr, int spl) {
    int i;
    u_int32_t ehi, elo;
    for (i = 0; i < NUM_TLB; i++) { //NUM_TLB = 64
        TLB_Read(&ehi, &elo, i);
        if (elo & TLBLO_VALID) {
            continue;
        }
        ehi = faultaddress;
        elo = paddr | TLBLO_DIRTY | TLBLO_VALID;
        DEBUG(DB_VM, "dumbvm: 0x%x -> 0x%x\n", faultaddress, paddr);
        TLB_Write(ehi, elo, i);
        splx(spl);
        return 1;
    }
}

int
vm_fault(int faulttype, vaddr_t faultaddress) {
    int spl;
    spl = splhigh();

    faultaddress &= PAGE_FRAME;

    //kprintf("vm_fault: faultaddress is 0x%x\n", faultaddress);
    DEBUG(DB_VM, "dumbvm: fault: 0x%x\n", faultaddress);

    switch (faulttype) {
        case VM_FAULT_READONLY:
            /* We always create pages read-write, so we can't get this */
            panic("dumbvm: got VM_FAULT_READONLY\n");
        case VM_FAULT_READ: /* TLB miss on load */
            //kprintf("trying to read from tlb\n");
            break;
        case VM_FAULT_WRITE: /* TLB miss on store */
            break;
        default:
            splx(spl);
            return EINVAL;
    }


    struct addrspace *as;
    as = curthread->t_vmspace;
    if (as == NULL) {
        /*
         * No address space set up. This is probably a kernel
         * fault early in boot. Return EFAULT so as to panic
         * instead of getting into an infinite faulting loop.
         */
        return EFAULT;
    }



    // decide which defined region the faultaddress falls into, then determine its physical address
    paddr_t paddr;
    // faultaddress is user virtual address
    if (faultaddress >= as->text->vstart && faultaddress < as->text->vend) {
        // inside text region
        paddr = (faultaddress - as->text->vstart) + as->text->pstart;
        if (access_pagetable(as->text, faultaddress, spl) == 1) {
                // when there is a page in this region's pagetable already or when a new page is successfully created
                // access_pagetable == 0 when the pagetable is full
                  splx(spl);
                return 0;
            }
    } else if (faultaddress >= as->data->vstart && faultaddress < as->data->vend) {
        // inside data region
        paddr = (faultaddress - as->data->vstart) + as->data->pstart;
        
         if (access_pagetable(as->data, faultaddress, spl) == 1) {
                // when there is a page in this region's pagetable already or when a new page is successfully created
                // access_pagetable == 0 when the pagetable is full
                  splx(spl);
                return 0;
            }
    } else if (faultaddress >= as->stack->vstart && faultaddress < as->stack->vend) {
        // inside stack region
        paddr = (faultaddress - as->stack->vstart) + as->stack->pstart;
    } else if (faultaddress >= as->heap->vstart && faultaddress < as->heap->vend) {
        // inside heap region
        paddr = (faultaddress - as->heap->vstart) + as->heap->pstart;
    } else {

        // we need to check the pagetable
        if (faultaddress >= /*as->stack->vstart*/STACK_OD_VSTART&& faultaddress < as->stack->vend) {
            // inside stack OD region
            if (access_pagetable(as->stack, faultaddress, spl) == 1) {
                // when there is a page in this region's pagetable already or when a new page is successfully created
                // access_pagetable == 0 when the pagetable is full
                 splx(spl);
                return 0;
            }
        } else if(faultaddress >= as->heap->vstart && faultaddress < as->heap->vend_actual) { // created by user malloc
            if (access_pagetable(as->heap, faultaddress, spl) == 1) {
                // when there is a page in this region's pagetable already or when a new page is successfully created
                // access_pagetable == 0 when the pagetable is full
                  splx(spl);
                return 0;
            }
        }

      //  kprintf("vm_fault: faultaddress not in defined regions: 0x%x \n", faultaddress);

        splx(spl);
        return EFAULT;
    }


    if (write_into_TLB(faultaddress, paddr, spl) == 1) {
        return 0;
    }

 //   kprintf("dumbvm: Ran out of TLB entries - cannot handle page fault\n");
    splx(spl);
    return EFAULT;

}

struct addrspace *
as_create(void) {

//    kprintf("\nas_create: Current thread is %d\n", curthread->threadPID);
//
//    kprintf("before as_create: \n");

    //print_coremap();


    //    kprintf("as_create: size of addrspace: %d\n", sizeof (struct addrspace));


    struct addrspace *as = (struct addrspace *) kmalloc(sizeof (struct addrspace));
    if (as == NULL) {
        return NULL;
    }


    // initialize addrspace
    as->text = NULL;
    as->data = NULL;
    as->heap = NULL;
    as->stack = NULL;



    return as;
}

void
as_destroy(struct addrspace *as) {

    //kfree(as);
    free_coremap();
    free_addrspace(as);
}

void
as_activate(struct addrspace *as) {
    int i, spl;

    (void) as;

    spl = splhigh();

    for (i = 0; i < NUM_TLB; i++) {
        TLB_Write(TLBHI_INVALID(i), TLBLO_INVALID(), i);
    }

    splx(spl);
}

void pagetable_initialize(struct addrspace_region* region) {
    region->pagetable = kmalloc(sizeof (struct pte)*PAGETABLE_SIZE);
    int i;
    for (i = 0; i < PAGETABLE_SIZE; i++) {
        region->pagetable[i].allocated = 0;
        region->pagetable[i].vstart = 0;
        region->pagetable[i].vend = 0;
        region->pagetable[i].pstart = 0;
        region->pagetable[i].pend = 0;
    }
}

int
as_define_region(struct addrspace *as, vaddr_t vaddr, size_t sz,
        int readable, int writeable, int executable) {
    size_t npages;
    /* Align the region. First, the base... */
    sz += vaddr & ~(vaddr_t) PAGE_FRAME;
    vaddr &= PAGE_FRAME;

    /* ...and now the length. */
    sz = (sz + PAGE_SIZE - 1) & PAGE_FRAME;

    npages = sz / PAGE_SIZE; //number of pages for this region


    /* We don't use these - all pages are read-write */
    // dumbvm below
    (void) readable;
    (void) writeable;
    (void) executable;

//    kprintf("as_define_region: region info ----> vaddr: 0x%x    sz: %d (decimal), 0x%x (hex)    "
//            "readable: %d    writeable: %d   executable: %d\n", vaddr, sz, sz, readable, writeable, executable);

    // notice below none of the pstart or pend are defined, since no page allocated yet

    // define text region region
    if (as->text == NULL) {
        // set up text addrspace_region
        as->text = kmalloc(sizeof (struct addrspace_region));
        as->text->vstart = vaddr; // user virtual address
        as->text->vend = vaddr + sz;
        as->text->numofpages = npages;
        as->text->pstart = 0; // will be loaded later
        as->text->pend = 0;
        pagetable_initialize(as->text);
        return 0;
    }

    // define the rest of the regions: data, heap, and stack
    if (as->data == NULL) {
        // set up data addrspace_region
        as->data = kmalloc(sizeof (struct addrspace_region));
        as->data->vstart = vaddr; // user virtual address
        as->data->vend = vaddr + sz;
        as->data->numofpages = npages;
        as->data->pstart = 0; // will be loaded later
        as->data->pend = 0;
        pagetable_initialize(as->data);

        // set up heap sddrspace region
        as->heap = kmalloc(sizeof (struct addrspace_region));
        as->heap->vstart = ROUND_UP(as->data->vend, PAGE_SIZE); // user virtual address
        as->heap->vend = as->heap->vstart; // no heap initially
        as->heap->numofpages = 0; // will be loaded later
        as->heap->pstart = 0;
        as->heap->pend = 0;
        as->heap->vend_actual = as->heap->vstart;
        pagetable_initialize(as->heap);

        // set up stack sddrspace region        
        as->stack = kmalloc(sizeof (struct addrspace_region));
        as->stack->vstart = USERSTACK - DUMBVM_STACKPAGES*PAGE_SIZE; // user virtual address
        as->stack->vend = USERSTACK;
        as->stack->numofpages = DUMBVM_STACKPAGES;
        as->stack->pstart = 0; // will be loaded later
        as->stack->pend = 0;
        pagetable_initialize(as->stack);




        return 0;

    }


    /*
     * Support for more than two regions is not available.
     */
    kprintf("dumbvm: Warning: too many regions\n");
    return EUNIMP;
}

int
as_prepare_load(struct addrspace *as) {

    // first make sure no page frames allocated for each region
    assert(as->text->pstart == 0 && as->text->pend == 0);
    assert(as->data->pstart == 0 && as->data->pend == 0);
    assert(as->heap->pstart == 0 && as->heap->pend == 0);
    assert(as->stack->pstart == 0 && as->stack->pend == 0);


    // allocate physical page frames for the process, heap is excluded
    as->text->pstart = getppages(as->text->numofpages);
    if (as->text->pstart == 0) {
        return ENOMEM; // out of memory
    }
    as->text->pend = as->text->pstart + (as->text->numofpages) * PAGE_SIZE;

    as->data->pstart = getppages(as->data->numofpages);
    if (as->data->pstart == 0) {
        return ENOMEM; // out of memory
    }
    as->data->pend = as->data->pstart + (as->data->numofpages) * PAGE_SIZE;

    // for stack: first allocate 24 pages, will be modified later on based on user request
    as->stack->pstart = getppages(as->stack->numofpages);
    if (as->stack->pstart == 0) {
        return ENOMEM; // out of memory
    }
    as->stack->pend = as->stack->pstart + (as->stack->numofpages) * PAGE_SIZE;
    
    // does not allocate any page for heap

    return 0;
}

int
as_complete_load(struct addrspace *as) {
    (void) as;
    //kprintf("as_complete_load: \n");
   // print_coremap();
    return 0;
}

int
as_define_stack(struct addrspace *as, vaddr_t *stackptr) {
    // this function just assigns the value to stackptr
    // stackptr points to the user virtual address
    assert(as->stack->pstart != 0 && as->stack->pend != 0);
    *stackptr = USERSTACK; // which is USERSTACK

    return 0;
}

void copy_pagetables(struct addrspace_region *old, struct addrspace_region **new){
    int i;
    for (i=0; i<PAGETABLE_SIZE ; i++){
        (*new)->pagetable[i].allocated=old->pagetable[i].allocated;
        (*new)->pagetable[i].vstart=old->pagetable[i].vstart;
        (*new)->pagetable[i].vend=old->pagetable[i].vend;
        (*new)->pagetable[i].pstart=old->pagetable[i].pstart;
        (*new)->pagetable[i].pend=old->pagetable[i].pend;

    }
    
}

int num_empty_cm(){
    int i, p;
    for(i=0; i<numCoremapEntry; i++){
        if(coremap[i].allocated==0)
            p++;
    }
    return p;
}

int
as_copy(struct addrspace *old, struct addrspace **ret) {
    struct addrspace *new;
    //kprintf("COPY\n");
    new = as_create();
    if (new == NULL) {
        return ENOMEM;
    }
    new->data=kmalloc(sizeof(struct addrspace_region));
    new->data->vstart = old->data->vstart;
    new->data->vend = old->data->vend;
    new->data->pstart = old->data->pstart; //these should be written over anyways in as_prepare_load
    new->data->pend = old->data->pend;

    new->data->numofpages = old->data->numofpages;
    copy_pagetables(old->data, &(new->data));
    
    new->text=kmalloc(sizeof(struct addrspace_region));
    new->text->vstart = old->text->vstart;
    new->text->numofpages = old->text->numofpages;
    new->text->vend = old->text->vend;
    new->text->pstart = old->text->pstart; //these should be written over anyways in as_prepare_load
    new->text->pend = old->text->pend;
    copy_pagetables(old->text, &(new->text));
    
    new->stack=kmalloc(sizeof(struct addrspace_region));
    new->stack->vstart = old->stack->vstart;
    new->stack->numofpages = old->stack->numofpages;
    new->stack->vend = old->stack->vend;
    new->stack->pstart = old->stack->pstart; //these should be written over anyways in as_prepare_load
    new->stack->pend = old->stack->pend;
    copy_pagetables(old->stack, &(new->stack));
    
    new->heap=kmalloc(sizeof(struct addrspace_region));
    new->heap->vstart = old->heap->vstart;
    new->heap->numofpages = old->heap->numofpages;
    new->heap->vend = old->heap->vend;
    new->heap->pstart = old->heap->pstart; //these should be written over anyways in as_prepare_load
    new->heap->pend = old->heap->pend;
    new->heap->vend_actual = old->heap->vend_actual;
    copy_pagetables(old->heap, &(new->heap));

    if (as_prepare_load(new)) {
        as_destroy(new);
        return ENOMEM;
    }

    assert(new->data->pstart != 0);
    assert(new->text->pstart != 0);
    assert(new->stack->pstart != 0);

    memmove((void *) PADDR_TO_KVADDR(new->data->pstart),
            (const void *) PADDR_TO_KVADDR(old->data->pstart),
            old->data->numofpages * PAGE_SIZE);

    memmove((void *) PADDR_TO_KVADDR(new->text->pstart),
            (const void *) PADDR_TO_KVADDR(old->text->pstart),
            old->text->numofpages * PAGE_SIZE);

    //    memmove((void *) PADDR_TO_KVADDR(new->as_stackpbase),
    //            (const void *) PADDR_TO_KVADDR(old->as_stackpbase),
    //            DUMBVM_STACKPAGES * PAGE_SIZE);

    memmove((void *) PADDR_TO_KVADDR(new->heap->pstart), //copy the heap
            (const void *) PADDR_TO_KVADDR(old->heap->pstart),
            old->heap->numofpages * PAGE_SIZE);

    memmove((void *) PADDR_TO_KVADDR(new->stack->pend), //copy the heap
            (const void *) PADDR_TO_KVADDR(old->stack->pend),
            old->stack->numofpages * PAGE_SIZE);
    *ret = new;

    //kprintf("copy done!\n");


    return 0;
}

