//#include <types.h>
//#include <kern/errno.h>
//#include <lib.h>
//#include <thread.h>
//#include <curthread.h>
//#include <addrspace.h>
//#include <vm.h>
//#include <machine/spl.h>
//#include <machine/tlb.h>
//#include <synch.h>
//
///*
// * Dumb MIPS-only "VM system" that is intended to only be just barely
// * enough to struggle off the ground. You should replace all of this
// * code while doing the VM assignment. In fact, starting in that
// * assignment, this file is not included in your kernel!
// */
//
//
///* under dumbvm, always have 48k of user stack */
//#define DUMBVM_STACKPAGES    12
//
//
//paddr_t ROUND_DOWN(paddr_t address, int size) {
//    return ((address) - ((address) % (size)));
//}
//
//
//paddr_t ROUND_UP(paddr_t address, int size) {
//    return ((address) - ((address) % (size)) + (size));
//    
//}
//
//struct coremapEntry *coremap; //coremap is array of coremapEntry
//
//struct lock *coremap_lock;
//int total_page_num; //number of entries in coremap
//
//paddr_t baseaddr; //the physical address of the first page after the coremap
//
//
////struct addrspace *
////as_create(void) {
////    struct addrspace *as = kmalloc(sizeof (struct addrspace)); // where it allocates to? to RAM?
////    if (as == NULL) {
////        return NULL;
////    }
////}
//
//
//void
//vm_bootstrap(void)
//{
//	/* Do nothing. */
//}
//
//static
//paddr_t
//getppages(unsigned long npages)
//{
//        int i, startindex, pcount=npages;
//        paddr_t start_addr;
//    lock_acquire(coremap_lock); //about to chang the coremap, get exclusive access
//    
//
//    for (i=0; i<total_page_num; i++){
//        if(coremap[i].allocated==0) {
//            //entry not occupied atm
//            if (pcount==npages){
//                startindex=i; //this is the first empty slot in a block, save the startin index
//            }
//            pcount--;
//            if(pcount==0){
//                break;
//            }
//        }
//        else{
//            pcount=npages;
//        }
//    }
//    
//    if (i==total_page_num){
//        lock_release(coremap_lock); //not enough space
//        return 0;
//    }
//    
//    for (i=startindex; i<startindex+npages; i++){
//        coremap[i].allocated=1; //these entries occupied now
//        
//    }
//    
//    start_addr=coremap[startindex].paddr;
//    
//    lock_release(coremap_lock);
//    
//    return start_addr;
//    
//    
//
//}
//
///* Allocate/free some kernel-space virtual pages */
//
//int get_free_page(){
//    int i;
//    for (i=0; i<total_page_num; i++){
//        
//    }
//}
//vaddr_t 
//alloc_kpages(int npages)
//{
//	paddr_t pa;
//	pa = getppages(npages);
//	if (pa==0) {
//		return 0;
//	}
//	return PADDR_TO_KVADDR(pa);
//}
//
//void 
//free_kpages(vaddr_t addr)
//{
//	/* nothing */
//
//	(void)addr;
//}
//
//int
//vm_fault(int faulttype, vaddr_t faultaddress)
//{
//	vaddr_t vbase1, vtop1, vbase2, vtop2, stackbase, stacktop;
//	paddr_t paddr;
//	int i;
//	u_int32_t ehi, elo;
//	struct addrspace *as;
//	int spl;
//
//	spl = splhigh();
//
//	faultaddress &= PAGE_FRAME;
//
//	DEBUG(DB_VM, "dumbvm: fault: 0x%x\n", faultaddress);
//
//	switch (faulttype) {
//	    case VM_FAULT_READONLY:
//		/* We always create pages read-write, so we can't get this */
//		panic("dumbvm: got VM_FAULT_READONLY\n");
//	    case VM_FAULT_READ:
//	    case VM_FAULT_WRITE:
//		break;
//	    default:
//		splx(spl);
//		return EINVAL;
//	}
//
//	as = curthread->t_vmspace;
//	if (as == NULL) {
//		/*
//		 * No address space set up. This is probably a kernel
//		 * fault early in boot. Return EFAULT so as to panic
//		 * instead of getting into an infinite faulting loop.
//		 */
//		return EFAULT;
//	}
//
//	/* Assert that the address space has been set up properly. */
//	assert(as->as_vbase1 != 0);
//	assert(as->as_pbase1 != 0);
//	assert(as->as_npages1 != 0);
//	assert(as->as_vbase2 != 0);
//	assert(as->as_pbase2 != 0);
//	assert(as->as_npages2 != 0);
//	assert(as->as_stackpbase != 0);
//	assert((as->as_vbase1 & PAGE_FRAME) == as->as_vbase1);
//	assert((as->as_pbase1 & PAGE_FRAME) == as->as_pbase1);
//	assert((as->as_vbase2 & PAGE_FRAME) == as->as_vbase2);
//	assert((as->as_pbase2 & PAGE_FRAME) == as->as_pbase2);
//	assert((as->as_stackpbase & PAGE_FRAME) == as->as_stackpbase);
//
//	vbase1 = as->as_vbase1;
//	vtop1 = vbase1 + as->as_npages1 * PAGE_SIZE;
//	vbase2 = as->as_vbase2;
//	vtop2 = vbase2 + as->as_npages2 * PAGE_SIZE;
//	stackbase = USERSTACK - DUMBVM_STACKPAGES * PAGE_SIZE;
//	stacktop = USERSTACK;
//
//	if (faultaddress >= vbase1 && faultaddress < vtop1) {
//		paddr = (faultaddress - vbase1) + as->as_pbase1;
//	}
//	else if (faultaddress >= vbase2 && faultaddress < vtop2) {
//		paddr = (faultaddress - vbase2) + as->as_pbase2;
//	}
//	else if (faultaddress >= stackbase && faultaddress < stacktop) {
//		paddr = (faultaddress - stackbase) + as->as_stackpbase;
//	}
//	else {
//		splx(spl);
//		return EFAULT;
//	}
//
//	/* make sure it's page-aligned */
//	assert((paddr & PAGE_FRAME)==paddr);
//
//	for (i=0; i<NUM_TLB; i++) {
//		TLB_Read(&ehi, &elo, i);    
//		if (elo & TLBLO_VALID) {
//			continue;
//		}
//		ehi = faultaddress;
//		elo = paddr | TLBLO_DIRTY | TLBLO_VALID;
//		DEBUG(DB_VM, "dumbvm: 0x%x -> 0x%x\n", faultaddress, paddr);
//		TLB_Write(ehi, elo, i);
//		splx(spl);
//		return 0;
//	}
//
//	kprintf("dumbvm: Ran out of TLB entries - cannot handle page fault\n");
//	splx(spl);
//	return EFAULT;
//}
//
////old version of as_create
//
//
//void coremap_bootstrap() {
//    kprintf("in the cm bootstrap\n");
//    // calculate how much space does ram take
//    u_int32_t low, high; //high is lastpaddr right?
//   // u_int32_t availableRam;
//
//    ram_getsize(&low, &high);
//    kprintf("The lowest address is %d, the highest is %d\n", low, high);
//  
//    
//    // calculate number of frames
//   // u_int32_t nFrames;
//    int divisibleAddr = ROUND_DOWN(high, PAGE_SIZE);
//    int nFrames = divisibleAddr/PAGE_SIZE;
//    
//    
//    
//    /***********************/
//    // calculate the number of page table entries for each frame
//    int nPageEntry = PAGE_SIZE / sizeof (struct pagetableEntry);
// 
//    
//    total_page_num = (int) (ROUND_DOWN((high-low), PAGE_SIZE))/ PAGE_SIZE; /* calculate the number of pages from 0 to lastpaddr by calling macro */
//    
//    // ??? save space for context switch
//
//    // segments???
//    // how many process per kernel is using the address space
//    
//
//    kprintf("coremap_bootstrap: coremap initialing with:\n");
//    kprintf("available number of PTEs per frame: %d\n", nPageEntry);
////    kprintf("number of frames occupied by pagetable: %d\n", pframes);  ???
//    kprintf("number of frames available: %d\n", nFrames);
//    kprintf("number of pages we have: %d \n", total_page_num);
//
//    coremap = (struct coremapEntry *) PADDR_TO_KVADDR(low);
//    //^ make the coremap start at the first possible address. how to allocate coremap when your kmalloc needs coremap to work lol
//    unsigned coremap_size;
//    coremap_size=total_page_num*sizeof(struct coremapEntry); //coremap tells you all the leftover space that's available
//    
//    
//    coremap_lock=lock_create("coremap_access");
//    
//    baseaddr=low+coremap_size;
//    paddr_t lowaddr_copy=low;
//    int num_cm_entries=((int)ROUND_DOWN(high-baseaddr, PAGE_SIZE))/PAGE_SIZE;
//    int i;
//    for (i=0; i<total_page_num; i++){
//        coremap[i].paddr=lowaddr_copy;
//        coremap[i].mypage=NULL;
//        coremap[i].tlb_index=-1; //currently not in the TLB
//        coremap[i].kernpage=0;
//       
//        lowaddr_copy+=PAGE_SIZE;
//        if (i<num_cm_entries){
//            coremap[i].fixed=1; //cannot change this entry
//            coremap[i].allocated=1; //don't use this
//        }
//        else {
//            coremap[i].fixed=0;
//             coremap[i].allocated=0;
//        }
//    }
//    
//
//
//}
//
//int get_coremap_entry(paddr_t paddr){
//    int i;
//
//    
//    i=(int)ROUND_DOWN(paddr-baseaddr, PAGE_SIZE)/PAGE_SIZE-1;
//    return i;
//    
//}
//
//unsigned tlb_replace(){ //when we encounter tlb fault, we go to the page table, retrieve a page table entry, and evict a cached page table entry from the tlb
//    //not sure how to choose which entry to evict yet. Need replacement algorithm
//}
//
//int open_tlb_index(){
//    //function to remove something from the TLB and return the index of the empty spot
//}
//
//void tlb_handlefault(vaddr_t vadd, paddr_t padd){ //calls tlb_replace to get an available spot, then stores requested pte in tlb
//    int spl, tlb_index, cm_index;
//    spl=splhigh();
//    
//    tlb_index=TLB_Probe((u_int32_t) vadd, 0); //probe checks if this vaddr already exists in TLB
//    
//    if(tlb_index<0){
//        tlb_index=open_tlb_index();
//    }
//    
//    cm_index=get_coremap_entry(padd);
//    coremap[cm_index].tlb_index=tlb_index;
//    
//    u_int32_t hi=vadd&TLBHI_VPAGE; //get the page number using bitmask
//    u_int32_t lo=padd&TLBLO_PPAGE; //may need to bitwise OR with TLB Valid??
//    
//    TLB_Write(hi, lo, tlb_index);
//    
//    splx(spl);
//   
//    
//    
//}