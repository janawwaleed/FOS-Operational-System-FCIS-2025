#ifndef FOS_INC_DYNBLK_MANAGE_H
#define FOS_INC_DYNBLK_MANAGE_H
#include <inc/queue.h>
#include <inc/types.h>
#include <inc/environment_definitions.h>

/*DATA*/
//[1] Constants
#define LOG2_MIN_SIZE (3)								//3 Bits
#define LOG2_MAX_SIZE (11)								//11 Bits
#define DYN_ALLOC_MAX_SIZE (32<<20) 					//32 MB
#define DYN_ALLOC_MIN_BLOCK_SIZE (1<<LOG2_MIN_SIZE)		//8 BYTE
#define DYN_ALLOC_MAX_BLOCK_SIZE (1<<LOG2_MAX_SIZE) 	//2 KB

//[2] Data Structures
struct BlockElement
{
	LIST_ENTRY(BlockElement) prev_next_info;	/* linked list links */
};
LIST_HEAD(BlockElement_List, BlockElement);
struct BlockElement_List freeBlockLists[LOG2_MAX_SIZE - LOG2_MIN_SIZE + 1] ;

struct PageInfoElement
{
	LIST_ENTRY(PageInfoElement) prev_next_info;	/* linked list links */
	uint16 block_size;
	uint16 num_of_free_blocks;
};
LIST_HEAD(PageInfoElement_List, PageInfoElement);
struct PageInfoElement_List freePagesList ;
struct PageInfoElement pageBlockInfoArr[DYN_ALLOC_MAX_SIZE/PAGE_SIZE];

//[3] Limits (to be set in initialize_dynamic_allocator())
uint32 dynAllocStart;
uint32 dynAllocEnd;

/*FUNCTIONS*/
//=============================================================================
/*2025*/ //GIVEN FUNCTIONS
uint32 to_page_va(struct PageInfoElement* ptrPageInfo);

//KERNEL: implemented inside kern/mem/kheap.c
//USER: implemented inside kern/mem/uheap.c
int get_page(void* va);		//get a page from the Kernel Page Allocator for DA (i.e. Allocate it)
void return_page(void* va);	//return a page from the DA to Kernel Page Allocator (i.e. Free It)
//=============================================================================

/*2025*/ //REQUIRED FUNCTIONS
void initialize_dynamic_allocator(uint32 daStart, uint32 daEnd);
void *alloc_block(uint32 size);
void free_block(void* va);
__inline__ uint32 get_block_size(void *va);

/*2025*/ //BONUS FUNCTIONS
void *realloc_block(void* va, uint32 new_size);

#endif
