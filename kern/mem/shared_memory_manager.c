#include <inc/memlayout.h>
#include "shared_memory_manager.h"

#include <inc/mmu.h>
#include <inc/error.h>
#include <inc/string.h>
#include <inc/assert.h>
#include <inc/queue.h>
#include <inc/environment_definitions.h>

#include <kern/proc/user_environment.h>
#include <kern/trap/syscall.h>
#include "kheap.h"
#include "memory_manager.h"

//==================================================================================//
//============================== GIVEN FUNCTIONS ===================================//
//==================================================================================//

//===========================
// [1] INITIALIZE SHARES:
//===========================
//Initialize the list and the corresponding lock
void sharing_init()
{
#if USE_KHEAP
	LIST_INIT(&AllShares.shares_list) ;
	init_kspinlock(&AllShares.shareslock, "shares lock");
	//init_sleeplock(&AllShares.sharessleeplock, "shares sleep lock");
#else
	panic("not handled when KERN HEAP is disabled");
#endif
}

//=========================
// [2] Find Share Object:
//=========================
//Search for the given shared object in the "shares_list"
//Return:
//	a) if found: ptr to Share object
//	b) else: NULL
struct Share* find_share(int32 ownerID, char* name)
{
#if USE_KHEAP
	struct Share * ret = NULL;
	bool wasHeld = holding_kspinlock(&(AllShares.shareslock));
	if (!wasHeld)
	{
		acquire_kspinlock(&(AllShares.shareslock));
	}
	{
		struct Share * shr ;
		LIST_FOREACH(shr, &(AllShares.shares_list))
		{
			//cprintf("shared var name = %s compared with %s\n", name, shr->name);
			if(shr->ownerID == ownerID && strcmp(name, shr->name)==0)
			{
				//cprintf("%s found\n", name);
				ret = shr;
				break;
			}
		}
	}
	if (!wasHeld)
	{
		release_kspinlock(&(AllShares.shareslock));
	}
	return ret;
#else
	panic("not handled when KERN HEAP is disabled");
#endif
}

//==============================
// [3] Get Size of Share Object:
//==============================
int size_of_shared_object(int32 ownerID, char* shareName)
{
	// This function should return the size of the given shared object
	// RETURN:
	//	a) If found, return size of shared object
	//	b) Else, return E_SHARED_MEM_NOT_EXISTS
	//
	struct Share* ptr_share = find_share(ownerID, shareName);
	if (ptr_share == NULL)
		return E_SHARED_MEM_NOT_EXISTS;
	else
		return ptr_share->size;

	return 0;
}
//===========================================================


//==================================================================================//
//============================ REQUIRED FUNCTIONS ==================================//
//==================================================================================//

inline struct FrameInfo** AllocFramesStorage(int S_framesNum)
{
	#if USE_KHEAP

	if(S_framesNum <= 0) return NULL;

	int S_sizeOfFrames = S_framesNum * sizeof(struct FrameInfo*);
	struct FrameInfo** S_frames_storage = (struct FrameInfo**)kmalloc(S_sizeOfFrames);

	if (S_frames_storage == NULL) return NULL;

	memset(S_frames_storage, 0, S_sizeOfFrames);
	return S_frames_storage;

	#else
		panic("KERNEL HEAP is OFF! create_frames_storage() needs USE_KHEAP to be 1");
	#endif
}


//=====================================
// [1] Alloc & Initialize Share Object:
//=====================================
//Allocates a new shared object and initialize its member
//It dynamically creates the "framesStorage"
//Return: allocatedObject (pointer to struct Share) passed by reference
struct Share* alloc_share(int32 ownerID, char* shareName, uint32 size, uint8 isWritable)
{	/*Sohila [PROJECT'25.IM#3] SHARED MEMORY*/
	//TODO: [PROJECT'25.IM#3] SHARED MEMORY - #1 alloc_share
	//Your code is here
	#if USE_KHEAP

	struct Share* S_newShared = (struct Share *) kmalloc(sizeof(struct Share));
	if (S_newShared == NULL)
		return NULL;

	S_newShared->ownerID = ownerID;

	strncpy(S_newShared->name, shareName, sizeof(S_newShared->name) - 1);
	S_newShared->size = size;
	S_newShared->isWritable = isWritable;
	S_newShared->references = 1;
	//have to assign the ID with the VA and removing the msb
	S_newShared->ID = ((int32) S_newShared) & 0x0FFFFFFF;

	uint32 numOfallocframes = ROUNDUP(size, PAGE_SIZE) / PAGE_SIZE;

	S_newShared->framesStorage = AllocFramesStorage(numOfallocframes);
	if (S_newShared->framesStorage == NULL) {
		kfree(S_newShared);
		return NULL;
	}

	return S_newShared;

	#else
    	panic("KERNEL HEAP is OFF! alloc_share() needs USE_KHEAP to be 1");
	#endif
	//Comment the following line
	//panic("alloc_share() is not implemented yet...!!");
}


//=========================
// [4] Create Share Object:
//=========================
int create_shared_object(int32 ownerID, char* shareName, uint32 size, uint8 isWritable, void* virtual_address)
{	/*Sohila [PROJECT'25.IM#3] SHARED MEMORY*/
	//TODO: [PROJECT'25.IM#3] SHARED MEMORY - #3 create_shared_object
	//Your code is here
	#if USE_KHEAP

	struct Env* myenv = get_cpu_proc(); //The calling environment
	acquire_kspinlock(&(AllShares.shareslock));

	//shared object already exists b el name wel id da fymnf3sh a3mlo create tane
	if (find_share(ownerID, shareName) != NULL) {
		release_kspinlock(&(AllShares.shareslock));
	    return E_SHARED_MEM_EXISTS;
	}

	struct Share* s = alloc_share(ownerID, shareName, size, isWritable);
	if (s == NULL) {
		release_kspinlock(&(AllShares.shareslock));
	    ////////souuuuuuuuuuu//////
	    kfree(s);
	    return E_NO_SHARE;
	}

	uint32 pageNum = ROUNDUP(size, PAGE_SIZE) / PAGE_SIZE;
	uint32 page_address = (uint32) virtual_address;

	for (int i = 0; i < pageNum; i++) {
		struct FrameInfo* f;
	    //int ret = allocate_frame(&f);
	    if (allocate_frame(&f) != 0) {
	    	release_kspinlock(&(AllShares.shareslock));
	        return E_NO_SHARE;
	    }
	    map_frame(myenv->env_page_directory, f, (uint32) page_address, PERM_USER | PERM_PRESENT | PERM_WRITEABLE);
	    s->framesStorage[i] = f;
	    page_address += PAGE_SIZE;
	}
	LIST_INSERT_TAIL(&(AllShares.shares_list), s);
	release_kspinlock(&(AllShares.shareslock));
	return s->ID;

	#else
    	panic("KERNEL HEAP is OFF! create_shared_object() needs USE_KHEAP to be 1");
	#endif
	//Comment the following line
	//panic("create_shared_object() is not implemented yet...!!");
	// This function should create the shared object at the given virtual address with the given size
	// and return the ShareObjectID
	// RETURN:
	//	a) ID of the shared object (its VA after masking out its msb) if success
	//	b) E_SHARED_MEM_EXISTS if the shared object already exists
	//	c) E_NO_SHARE if failed to create a shared object
}


//======================
// [5] Get Share Object:
//======================
int get_shared_object(int32 ownerID, char* shareName, void* virtual_address)
{	/*Sohila [PROJECT'25.IM#3] SHARED MEMORY*/
	//TODO: [PROJECT'25.IM#3] SHARED MEMORY - #5 get_shared_object
	//Your code is here
	#if USE_KHEAP

	struct Env* myenv = get_cpu_proc(); //The calling environment
	acquire_kspinlock(&(AllShares.shareslock));
	struct Share* finding_shrObj = find_share(ownerID, shareName);

	if (finding_shrObj == NULL) {
		release_kspinlock(&(AllShares.shareslock));
		return E_SHARED_MEM_NOT_EXISTS;
	}

	int Numframe = ROUNDUP(finding_shrObj->size, PAGE_SIZE) / PAGE_SIZE;

	for (uint32 i = 0; i < Numframe; i++) {
		uint32 page_address = (uint32) virtual_address + (i * PAGE_SIZE);
	    uint32 writableCheck = PERM_USER;
	    if (finding_shrObj->isWritable) {
	    	writableCheck = PERM_USER | PERM_WRITEABLE;
	    }
	    map_frame(myenv->env_page_directory, finding_shrObj->framesStorage[i], page_address, writableCheck);
	}

	finding_shrObj->references++;
	release_kspinlock(&(AllShares.shareslock));

	return finding_shrObj->ID;

	#else
		panic("KERNEL HEAP is OFF! get_shared_object() needs USE_KHEAP to be 1");
	#endif
	//Comment the following line
	//panic("get_shared_object() is not implemented yet...!!");
	// 	This function should share the required object in the heap of the current environment
	//	starting from the given virtual_address with the specified permissions of the object: read_only/writable
	// 	and return the ShareObjectID
	// RETURN:
	//	a) ID of the shared object (its VA after masking out its msb) if success
	//	b) E_SHARED_MEM_NOT_EXISTS if the shared object is not exists

}

//==================================================================================//
//============================== BONUS FUNCTIONS ===================================//
//==================================================================================//
//=========================
// [1] Delete Share Object:
//=========================
//delete the given shared object from the "shares_list"
//it should free its framesStorage and the share object itself
void free_share(struct Share* ptrShare)
{	/*Abdelaziz [PROJECT'25]*/
	//TODO: [PROJECT'25.BONUS#5] EXIT #2 - free_share
	//Your code is here
	#if USE_KHEAP


    acquire_kspinlock(&(AllShares.shareslock));
    LIST_REMOVE(&(AllShares.shares_list), ptrShare);
    release_kspinlock(&(AllShares.shareslock));

    if (ptrShare->references == 1) {
        for (int i = 0; ptrShare->framesStorage[i] != NULL; i++) {
            free_frame(ptrShare->framesStorage[i]);
        }
    }

    kfree(ptrShare->framesStorage);
    kfree(ptrShare);

	#else
    	panic("KERNEL HEAP is OFF! free_share() needs USE_KHEAP to be 1");
	#endif
	//Comment the following line
	//panic("free_share() is not implemented yet...!!");
}


//=========================
// [2] Free Share Object:
//=========================
int delete_shared_object(int32 sharedObjectID, void *startVA)
{
	//TODO: [PROJECT'25.BONUS#5] EXIT #2 - delete_shared_object
	//Your code is here
	//Comment the following line
	panic("delete_shared_object() is not implemented yet...!!");

	struct Env* myenv = get_cpu_proc(); //The calling environment

	// This function should free (delete) the shared object from the User Heap of the current environment
	// If this is the last shared env, then the "frames_store" should be cleared and the shared object should be deleted
	// RETURN:
	//	a) 0 if success
	//	b) E_SHARED_MEM_NOT_EXISTS if the shared object is not exists

	// Steps:
	//	1) Get the shared object from the "shares" array (use get_share_object_ID())
	//	2) Unmap it from the current environment "myenv"
	//	3) If one or more table becomes empty, remove it
	//	4) Update references
	//	5) If this is the last share, delete the share object (use free_share())
	//	6) Flush the cache "tlbflush()"

}
