#include "kheap.h"
#include <inc/memlayout.h>
#include <inc/dynamic_allocator.h>
#include <kern/conc/sleeplock.h>
#include <kern/proc/user_environment.h>
#include <kern/mem/memory_manager.h>
#include "../conc/kspinlock.h"

/* tracking arrays (size derived from heap limits) */
#define KHEAP_PAGES_COUNT ((KERNEL_HEAP_MAX - KERNEL_HEAP_START) / PAGE_SIZE)
static uint32 kallocation_sizes[KHEAP_PAGES_COUNT]; // store allocation size in bytes, 0 => free

// Helper macro for index calculation
#define kva_to_index(va) (((uint32)(va) - KERNEL_HEAP_START) / PAGE_SIZE)

//Helper function to find a region using custom fit (EXACT then WORST)
static uint32 find_customfit(uint32 numPages) {

    uint32 first_exact_va = 0;
    uint32 worst_va = 0;
    uint32 worst_size = 0;

    uint32 curr_va = kheapPageAllocStart;
    while (curr_va < kheapPageAllocBreak) {
        uint32 index = kva_to_index(curr_va);

        if (kallocation_sizes[index] != 0) {
            uint32 usedSpace_size = kallocation_sizes[index];
            uint32 usedSpace_pages = ROUNDUP(usedSpace_size, PAGE_SIZE) / PAGE_SIZE;
            curr_va += usedSpace_pages * PAGE_SIZE;
            continue;
        }

        // Free Space, count size
        uint32 freeSpace_va = curr_va;
        uint32 freeSpace_size = 0;
        uint32 temp_va = curr_va;
        uint32 temp_index = index;
        while (temp_va < kheapPageAllocBreak && kallocation_sizes[temp_index] == 0) {
        	freeSpace_size++;
            temp_va += PAGE_SIZE;
            temp_index++;
        }

        // Check for exact fit
        if (freeSpace_size == numPages) {
            if (first_exact_va == 0) {
                first_exact_va = freeSpace_va;
            }
        }

        // Update worst fit if better
        if (freeSpace_size >= numPages && freeSpace_size > worst_size) {
            worst_size = freeSpace_size;
            worst_va = freeSpace_va;
        }

        // Move to next
        curr_va = temp_va;
    }

    // Prefer exact if found
    if (first_exact_va != 0) {
        //cprintf_colored(TEXT_light_magenta, " * find_customfit: returning exact fit 0x%x\n", first_exact_va);
        return first_exact_va;
    } else if (worst_size >= numPages) {
        //cprintf_colored(TEXT_light_magenta, " * find_customfit: returning worst fit 0x%x\n", worst_va);
        return worst_va;
    }
    //cprintf_colored(TEXT_light_red, " * find_customfit: no fit found, returning 0\n");
    return 0;
}

//==================================================================================//
//============================== GIVEN FUNCTIONS ===================================//
//==================================================================================//

//==============================================
// [1] INITIALIZE KERNEL HEAP:
//==============================================
//TODO: [PROJECT'25.GM#2] KERNEL HEAP - #0 kheap_init [GIVEN]
//Remember to initialize locks (if any)
void kheap_init()
{
//==================================================================================
//DON'T CHANGE THESE LINES==========================================================
//==================================================================================
{
	initialize_dynamic_allocator(KERNEL_HEAP_START, KERNEL_HEAP_START + DYN_ALLOC_MAX_SIZE);
	set_kheap_strategy(KHP_PLACE_CUSTOMFIT);
	kheapPageAllocStart = dynAllocEnd + PAGE_SIZE;
	kheapPageAllocBreak = kheapPageAllocStart;
}
//==================================================================================
//==================================================================================
}

//==============================================
// [2] GET A PAGE FROM THE KERNEL FOR DA:
//==============================================
int get_page(void* va){
	int ret = alloc_page(ptr_page_directory, ROUNDDOWN((uint32)va, PAGE_SIZE), PERM_WRITEABLE, 0);
	if (ret < 0)
		panic("get_page() in kern: failed to allocate page from the kernel");
	return 0;
}

//==============================================
// [3] RETURN A PAGE FROM THE DA TO KERNEL:
//==============================================
void return_page(void* va){
	unmap_frame(ptr_page_directory, ROUNDDOWN((uint32)va, PAGE_SIZE));
}

//==================================================================================//
//============================ REQUIRED FUNCTIONS ==================================//
//==================================================================================//
//===================================
// [1] ALLOCATE SPACE IN KERNEL HEAP:
//===================================
void *kmalloc(unsigned int size){
	/*Abdelaziz & Sama [PROJECT'25]*/
	//TODO: [PROJECT'25.GM#2] KERNEL HEAP - #1 kmalloc
	//Your code is here
	#if USE_KHEAP

	uint32 numPages = ROUNDUP(size, PAGE_SIZE) / PAGE_SIZE;
	uint32 neededBytes = numPages * PAGE_SIZE;

    if (size == 0 || (neededBytes > KERNEL_HEAP_MAX - kheapPageAllocBreak)) {
        return NULL;
    }

    if (size <= DYN_ALLOC_MAX_BLOCK_SIZE) {
        return alloc_block(size);
    }

    uint32 allocVA = find_customfit(numPages);

    if (allocVA == 0) {
        if (kheapPageAllocBreak + neededBytes > KERNEL_HEAP_MAX) {
            return NULL;
        }
        allocVA = kheapPageAllocBreak;
        kheapPageAllocBreak += neededBytes;
    }

    uint32 va = allocVA;
    for (uint32 i = 0; i < numPages; ++i, va += PAGE_SIZE) {
    	if(get_page((void*)va) != 0) {
    		kheapPageAllocBreak = allocVA;
    		return NULL;
    	}
    }

    kallocation_sizes[kva_to_index(allocVA)] = size;
	//cprintf_colored(TEXT_green, " * kmalloc: Page allocate success at VA=0x%x for size=%d\n", allocVA, size);
    return (void*)allocVA;

	#else
    	panic("KERNEL HEAP is OFF! kmalloc() needs USE_KHEAP to be 1");
	#endif
    //kpanic_into_prompt("kmalloc() is not implemented yet...!!");
    //TODO: [PROJECT'25.BONUS#3] FAST PAGE ALLOCATOR
}

//=================================
// [2] FREE SPACE FROM KERNEL HEAP:
//=================================
void kfree(void* virtual_address) {
	/*Jana [PROJECT'25]*/
	//TODO: [PROJECT'25.GM#2] KERNEL HEAP - #2 kfree
	//Your code is here
	#if USE_KHEAP

	uint32 mycurrentva = (uint32) virtual_address;

	if (virtual_address == NULL)
		return;

	if (mycurrentva < KERNEL_HEAP_START || mycurrentva >= KERNEL_HEAP_MAX) {
		return;
	}

	uint32 myindex = kva_to_index(mycurrentva);
	uint32 mysize = kallocation_sizes[myindex];

	//double free
	if (mysize == 0) {
		//cprintf_colored(TEXT_red, " * kfree: va already free VA=0x%x\n",mycurrentva);
		return;
	}

	if (mysize <= DYN_ALLOC_MAX_BLOCK_SIZE) {
		free_block(virtual_address);
		return;
	}

	uint32 numPages = ROUNDUP(mysize, PAGE_SIZE) / PAGE_SIZE;
	int i = 0;
	while (i < numPages) {

		uint32 current_page_va = mycurrentva + (i * PAGE_SIZE);
		return_page((void*)current_page_va);

		i++;
	}
	kallocation_sizes[myindex] = 0;

	// break
	while(kheapPageAllocBreak > kheapPageAllocStart){
		uint32 prevva = kheapPageAllocBreak - PAGE_SIZE;
		uint32 *ptr = NULL;

		get_page_table(ptr_page_directory,prevva,&ptr);

		if((ptr[PTX(prevva)] & PERM_PRESENT) != 0){
			break;
		}
		else{
			kheapPageAllocBreak -= PAGE_SIZE;
		}

	}

	#else
		panic("KERNEL HEAP is OFF! kfree() needs USE_KHEAP to be 1");
	#endif
	//Comment the following line
	//panic("kfree() is not implemented yet...!!");
}

//=================================
// [3] FIND VA OF GIVEN PA:
//=================================
unsigned int kheap_virtual_address(unsigned int physical_address){
	/*Sohila [PROJECT'25]*/
	//TODO: [PROJECT'25.GM#2] KERNEL HEAP - #3 kheap_virtual_address
	//Your code is here
	#if USE_KHEAP

	uint32 frame_no= physical_address / PAGE_SIZE;
	struct FrameInfo* ptr = &frames_info[frame_no];

	if (ptr->kheap_mapped_va==0)
		return (uint32) NULL;
	else{

		uint32 virt_add_base = ptr->kheap_mapped_va;
		uint32 offset= physical_address % PAGE_SIZE;
		uint32 virt_add= virt_add_base + offset;
		return virt_add;
	}

	#else
		panic("KERNEL HEAP is OFF! kheap_virtual_address() needs USE_KHEAP to be 1");
	#endif
	//Comment the following line
	//panic("kheap_virtual_address() is not implemented yet...!!");

	/*EFFICIENT IMPLEMENTATION ~O(1) IS REQUIRED */
}

//=================================
// [4] FIND PA OF GIVEN VA:
//=================================
unsigned int kheap_physical_address(unsigned int virtual_address){
	/*Sohila [PROJECT'25]*/
	//TODO: [PROJECT'25.GM#2] KERNEL HEAP - #4 kheap_physical_address
	//Your code is here
	#if USE_KHEAP

	uint32 passed_VA= virtual_address;
	uint32 *ptr_page_table= NULL;
	struct FrameInfo *ptr_frame_info = get_frame_info(ptr_page_directory ,passed_VA, &ptr_page_table);
	if (ptr_frame_info==0)
		return (uint32) NULL;
	else {
		uint32 physical_address_base = to_physical_address(ptr_frame_info);
		uint32 offset= passed_VA % PAGE_SIZE;
		return physical_address_base + offset;
	}

	#else
    	panic("KERNEL HEAP is OFF! kheap_physical_address() needs USE_KHEAP to be 1");
	#endif
	//Comment the following line
	//panic("kheap_physical_address() is not implemented yet...!!");
	/*EFFICIENT IMPLEMENTATION ~O(1) IS REQUIRED */
}

//=================================================================================//
//============================== BONUS FUNCTION ===================================//
//=================================================================================//
/* krealloc():
 *	Attempts to resize the allocated space at "virtual_address" to "new_size" bytes,
 *	possibly moving it in the heap.
 *	If successful, returns the new virtual_address, in which case the old virtual_address must no longer be accessed.
 *	On failure, returns a null pointer, and the old virtual_address remains valid.
 *
 *	A call with virtual_address = null is equivalent to kmalloc().
 *	A call with new_size = zero is equivalent to kfree().
 */

extern __inline__ uint32 get_block_size(void *va);

void* krealloc(void* virtual_address, uint32 new_size) {
	/*Abdelaziz [PROJECT'25]*/
	//TODO: [PROJECT'25.BONUS#2] KERNEL REALLOC - krealloc
	//Your code is here
	#if USE_KHEAP

    if (virtual_address == NULL){
        return kmalloc(new_size);
    }

    if (new_size == 0){
        kfree(virtual_address);
        return NULL;
    }

    uint32 va = (uint32)virtual_address;

    int old_is_block = (va >= KERNEL_HEAP_START && va < dynAllocEnd);
    int old_is_page  = (va >= kheapPageAllocStart && va < kheapPageAllocBreak);

    if (!old_is_block && !old_is_page) return NULL;

    uint32 old_size = 0;
    uint32 old_pages = 0;

    if (old_is_page){
        old_size = kallocation_sizes[kva_to_index(va)];
        if (old_size == 0) return NULL;
        old_pages = ROUNDUP(old_size, PAGE_SIZE) / PAGE_SIZE;
    }
    else { //block allocator
        old_size = get_block_size(virtual_address);
        if (old_size == 0) return NULL;
    }

    if (old_size == new_size) return virtual_address;

    int new_is_block = (new_size <= DYN_ALLOC_MAX_BLOCK_SIZE);

    //block => block
    if (old_is_block && new_is_block)
        return realloc_block(virtual_address, new_size);

    //block => page
    if (old_is_block && !new_is_block){
        void* new_ptr = kmalloc(new_size);
        if (!new_ptr) return NULL;
        memcpy(new_ptr, virtual_address, old_size);
        free_block(virtual_address);
        return new_ptr;
    }

    //page => block
    if (!old_is_block && new_is_block){
        void* new_ptr = alloc_block(new_size);
        if (!new_ptr) return NULL;
        memcpy(new_ptr, virtual_address, old_size);
        kfree(virtual_address);
        return new_ptr;
    }

    //page => page
    uint32 new_pages = ROUNDUP(new_size, PAGE_SIZE) / PAGE_SIZE;

    //shrink
    if (new_pages < old_pages){
        uint32 pages_to_free = old_pages - new_pages;
        uint32 start_va = va + new_pages * PAGE_SIZE;

        for (uint32 i = 0; i < pages_to_free; i++){
            return_page((void*)(start_va + i * PAGE_SIZE));
        }

        kallocation_sizes[kva_to_index(va)] = new_size;

        if (va + old_pages * PAGE_SIZE == kheapPageAllocBreak)
            kheapPageAllocBreak = va + new_pages * PAGE_SIZE;

        return virtual_address;
    }

    // grow in-place ?
    if (new_pages > old_pages)
    {
        uint32 extra = new_pages - old_pages;
        uint32 next_va = va + old_pages * PAGE_SIZE;
        int can_extend = 1;

        for (uint32 i = 0; i < extra; i++)
        {
            uint32 v = next_va + i * PAGE_SIZE;
            if (v >= KERNEL_HEAP_MAX ||
                v >= kheapPageAllocBreak ||
                kallocation_sizes[kva_to_index(v)] != 0)
            {
                can_extend = 0;
                break;
            }
        }

        if (can_extend)
        {
            for (uint32 i = 0; i < extra; i++)
            {
                if (get_page((void*)(next_va + i * PAGE_SIZE)) != 0)
                {
                    // rollback
                    for (uint32 j = 0; j < i; j++)
                        return_page((void*)(next_va + j * PAGE_SIZE));
                    return NULL;
                }
            }
            kallocation_sizes[kva_to_index(va)] = new_size;
            return virtual_address;
        }
    }

    // can't grow in-place => allocate new, copy data, free old
    void* new_ptr = kmalloc(new_size);
    if (!new_ptr) return NULL;

    memcpy(new_ptr, virtual_address, old_size);
    kfree(virtual_address);

    return new_ptr;
	#else
    	panic("KERNEL HEAP is OFF! krealloc() needs USE_KHEAP to be 1");
	#endif
    //Comment the following line
    //panic("krealloc() is not implemented yet...!!");
}
