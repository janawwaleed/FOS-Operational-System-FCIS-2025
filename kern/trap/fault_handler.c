/*
 * fault_handler.c
 *
 *  Created on: Oct 12, 2022
 *      Author: HP
 */

#include "trap.h"
#include <kern/proc/user_environment.h>
#include <kern/cpu/sched.h>
#include <kern/cpu/cpu.h>
#include <kern/disk/pagefile_manager.h>
#include <kern/mem/memory_manager.h>
#include <kern/mem/kheap.h>

//2014 Test Free(): Set it to bypass the PAGE FAULT on an instruction with this length and continue executing the next one
// 0 means don't bypass the PAGE FAULT
uint8 bypassInstrLength = 0;

//===============================
// REPLACEMENT STRATEGIES
//===============================
//2020
void setPageReplacmentAlgorithmLRU(int LRU_TYPE)
{
	assert(LRU_TYPE == PG_REP_LRU_TIME_APPROX || LRU_TYPE == PG_REP_LRU_LISTS_APPROX);
	_PageRepAlgoType = LRU_TYPE ;
}
void setPageReplacmentAlgorithmCLOCK(){_PageRepAlgoType = PG_REP_CLOCK;}
void setPageReplacmentAlgorithmFIFO(){_PageRepAlgoType = PG_REP_FIFO;}
void setPageReplacmentAlgorithmModifiedCLOCK(){_PageRepAlgoType = PG_REP_MODIFIEDCLOCK;}
/*2018*/ void setPageReplacmentAlgorithmDynamicLocal(){_PageRepAlgoType = PG_REP_DYNAMIC_LOCAL;}
/*2021*/ void setPageReplacmentAlgorithmNchanceCLOCK(int PageWSMaxSweeps){_PageRepAlgoType = PG_REP_NchanceCLOCK;  page_WS_max_sweeps = PageWSMaxSweeps;}
/*2024*/ void setFASTNchanceCLOCK(bool fast){ FASTNchanceCLOCK = fast; };
/*2025*/ void setPageReplacmentAlgorithmOPTIMAL(){ _PageRepAlgoType = PG_REP_OPTIMAL; };

//2020
uint32 isPageReplacmentAlgorithmLRU(int LRU_TYPE){return _PageRepAlgoType == LRU_TYPE ? 1 : 0;}
uint32 isPageReplacmentAlgorithmCLOCK(){if(_PageRepAlgoType == PG_REP_CLOCK) return 1; return 0;}
uint32 isPageReplacmentAlgorithmFIFO(){if(_PageRepAlgoType == PG_REP_FIFO) return 1; return 0;}
uint32 isPageReplacmentAlgorithmModifiedCLOCK(){if(_PageRepAlgoType == PG_REP_MODIFIEDCLOCK) return 1; return 0;}
/*2018*/ uint32 isPageReplacmentAlgorithmDynamicLocal(){if(_PageRepAlgoType == PG_REP_DYNAMIC_LOCAL) return 1; return 0;}
/*2021*/ uint32 isPageReplacmentAlgorithmNchanceCLOCK(){if(_PageRepAlgoType == PG_REP_NchanceCLOCK) return 1; return 0;}
/*2021*/ uint32 isPageReplacmentAlgorithmOPTIMAL(){if(_PageRepAlgoType == PG_REP_OPTIMAL) return 1; return 0;}

//===============================
// PAGE BUFFERING
//===============================
void enableModifiedBuffer(uint32 enableIt){_EnableModifiedBuffer = enableIt;}
uint8 isModifiedBufferEnabled(){  return _EnableModifiedBuffer ; }

void enableBuffering(uint32 enableIt){_EnableBuffering = enableIt;}
uint8 isBufferingEnabled(){  return _EnableBuffering ; }

void setModifiedBufferLength(uint32 length) { _ModifiedBufferLength = length;}
uint32 getModifiedBufferLength() { return _ModifiedBufferLength;}

//===============================
// FAULT HANDLERS
//===============================

//==================
// [0] INIT HANDLER:
//==================
void fault_handler_init()
{
	//setPageReplacmentAlgorithmLRU(PG_REP_LRU_TIME_APPROX);
	//setPageReplacmentAlgorithmOPTIMAL();
	setPageReplacmentAlgorithmCLOCK();
	//setPageReplacmentAlgorithmModifiedCLOCK();
	enableBuffering(0);
	enableModifiedBuffer(0) ;
	setModifiedBufferLength(1000);
}
//==================
// [1] MAIN HANDLER:
//==================
/*2022*/
uint32 last_eip = 0;
uint32 before_last_eip = 0;
uint32 last_fault_va = 0;
uint32 before_last_fault_va = 0;
int8 num_repeated_fault  = 0;
extern uint32 sys_calculate_free_frames() ;

struct Env* last_faulted_env = NULL;

/*Abdelaziz [PROJECT'25.IM#1] FAULT HANDLER II*/
struct WS_List page_active_WS_list;
static int optimal_initialized = 0;

void fault_handler(struct Trapframe *tf)
{
	/******************************************************/
	// Read processor's CR2 register to find the faulting address
	uint32 fault_va = rcr2();
	//cprintf("************Faulted VA = %x************\n", fault_va);
	//	print_trapframe(tf);
	/******************************************************/

	//If same fault va for 3 times, then panic
	//UPDATE: 3 FAULTS MUST come from the same environment (or the kernel)
	struct Env* cur_env = get_cpu_proc();
	if (last_fault_va == fault_va && last_faulted_env == cur_env)
	{
		num_repeated_fault++ ;
		if (num_repeated_fault == 3)
		{
			print_trapframe(tf);
			panic("Failed to handle fault! fault @ at va = %x from eip = %x causes va (%x) to be faulted for 3 successive times\n", before_last_fault_va, before_last_eip, fault_va);
		}
	}
	else
	{
		before_last_fault_va = last_fault_va;
		before_last_eip = last_eip;
		num_repeated_fault = 0;
	}
	last_eip = (uint32)tf->tf_eip;
	last_fault_va = fault_va ;
	last_faulted_env = cur_env;
	/******************************************************/
	//2017: Check stack overflow for Kernel
	int userTrap = 0;
	if ((tf->tf_cs & 3) == 3) {
		userTrap = 1;
	}
	if (!userTrap)
	{
		struct cpu* c = mycpu();
		//cprintf("trap from KERNEL\n");
		if (cur_env && fault_va >= (uint32)cur_env->kstack && fault_va < (uint32)cur_env->kstack + PAGE_SIZE)
			panic("User Kernel Stack: overflow exception!");
		else if (fault_va >= (uint32)c->stack && fault_va < (uint32)c->stack + PAGE_SIZE)
			panic("Sched Kernel Stack of CPU #%d: overflow exception!", c - CPUS);
#if USE_KHEAP
		if (fault_va >= KERNEL_HEAP_MAX)
			panic("Kernel: heap overflow exception!");
#endif
	}
	//2017: Check stack underflow for User
	else
	{
		//cprintf("trap from USER\n");
		if (fault_va >= USTACKTOP && fault_va < USER_TOP)
			panic("User: stack underflow exception!");
	}

	//get a pointer to the environment that caused the fault at runtime
	//cprintf("curenv = %x\n", curenv);
	struct Env* faulted_env = cur_env;
	if (faulted_env == NULL)
	{
		cprintf("\nFaulted VA = %x\n", fault_va);
		print_trapframe(tf);
		panic("faulted env == NULL!");
	}
	//check the faulted address, is it a table or not ?
	//If the directory entry of the faulted address is NOT PRESENT then
	if ( (faulted_env->env_page_directory[PDX(fault_va)] & PERM_PRESENT) != PERM_PRESENT)
	{
		faulted_env->tableFaultsCounter ++ ;
		table_fault_handler(faulted_env, fault_va);
	}
	else
	{
		if (userTrap)
		{
			/*============================================================================================*/
			/*Abdelaziz & Sama [PROJECT'25]*/
			//TODO: [PROJECT'25.GM#3] FAULT HANDLER I - #2 Check for invalid pointers
			//(e.g. pointing to unmarked user heap page, kernel or wrong access rights),
			//your code is here
			int perms = pt_get_page_permissions(faulted_env->env_page_directory, fault_va);

			if (fault_va >= USTACKTOP){
				env_exit();
			}

			if ((perms & PERM_UHPAGE) == 0 && (fault_va >= USER_HEAP_START && fault_va < USER_HEAP_MAX)){
				env_exit();
			}

			if ((perms & PERM_PRESENT) && (perms & PERM_WRITEABLE) == 0){
				env_exit();
			}


			/*============================================================================================*/
		}

		/*2022: Check if fault due to Access Rights */
		int perms = pt_get_page_permissions(faulted_env->env_page_directory, fault_va);
		if (perms & PERM_PRESENT)
			panic("Page @va=%x is exist! page fault due to violation of ACCESS RIGHTS\n", fault_va) ;
		/*============================================================================================*/


		// we have normal page fault =============================================================
		faulted_env->pageFaultsCounter ++ ;

//				cprintf("[%08s] user PAGE fault va %08x\n", faulted_env->prog_name, fault_va);
//				cprintf("\nPage working set BEFORE fault handler...\n");
//				//env_page_ws_print(faulted_env);
		//int ffb = sys_calculate_free_frames();

		if(isBufferingEnabled())
		{
			__page_fault_handler_with_buffering(faulted_env, fault_va);
		}
		else
		{
			page_fault_handler(faulted_env, fault_va);
		}

		//		cprintf("\nPage working set AFTER fault handler...\n");
		//		//env_page_ws_print(faulted_env);
		//		int ffa = sys_calculate_free_frames();
		//		cprintf("fault handling @%x: difference in free frames (after - before = %d)\n", fault_va, ffa - ffb);
	}

	/*************************************************************/
	//Refresh the TLB cache
	tlbflush();
	/*************************************************************/
}


//=========================
// [2] TABLE FAULT HANDLER:
//=========================
void table_fault_handler(struct Env * curenv, uint32 fault_va)
{
	//panic("table_fault_handler() is not implemented yet...!!");
	//Check if it's a stack page
	uint32* ptr_table;
#if USE_KHEAP
	{
		ptr_table = create_page_table(curenv->env_page_directory, (uint32)fault_va);
	}
#else
	{
		__static_cpt(curenv->env_page_directory, (uint32)fault_va, &ptr_table);
	}
#endif
}

//=========================
// [3] PAGE FAULT HANDLER:
//=========================
/* Calculate the number of page faults according to the OPTIMAL replacement strategy
 * Given:
 * 	1. Initial Working Set List (that the process started with)
 * 	2. Max Working Set Size
 * 	3. Page References List (contains the stream of referenced VAs till the process finished)
 *
 * 	IMPORTANT: This function SHOULD NOT change any of the given lists
 */
int get_optimal_num_faults(struct WS_List *initWorkingSet, int maxWSSize, struct PageRef_List *pageReferences)
{ 	/*Abdelaziz [PROJECT'25.IM#1] FAULT HANDLER II*/
	//TODO: [PROJECT'25.IM#1] FAULT HANDLER II - #2 get_optimal_num_faults
	//Your code is here
	#if USE_KHEAP

	int falut_count = 0;

	//copy initial WS
	uint32 ws_copy[maxWSSize];
	int ws_count = 0;

	struct WorkingSetElement *ptr;
	LIST_FOREACH_SAFE(ptr, initWorkingSet, WorkingSetElement){
	    if (ws_count < maxWSSize){
	        ws_copy[ws_count++] = ptr->virtual_address;
	    }
	}

	//copy reference stream (VA's only)
	uint32 *ref_stream_copy = kmalloc(sizeof(uint32) * LIST_SIZE(pageReferences));
	if (!ref_stream_copy) panic("kmalloc failed for ref_stream");

	int ref_count = 0;
	struct PageRefElement *ref;
	LIST_FOREACH(ref, pageReferences){
	    ref_stream_copy[ref_count++] = ref->virtual_address;
	}

	for(int i = 0; i < LIST_SIZE(pageReferences); i++){
		int found = 0;
		for (int ii = 0; ii < ws_count; ii++){
			//cprintf_colored(TEXTBG_cyan,"Addresses: %x : %x\n", ws_copy[ii], ref_stream_copy[i]);
		    if (ws_copy[ii] == ref_stream_copy[i]){
		    	found = 1;
		    	break;
		    }
		}
		if(!found){
		if(ws_count == maxWSSize){
		    int ws_Maxsteps = 0;
		    int ws_Maxsteps_index = 0;
		    for(int k = 0; k< ws_count; k++)
		    {
		    	int found_future = 0;
		    	int ws_steps = 0;

		    	for (int j = i+1; j < LIST_SIZE(pageReferences); j++) {
		    	    if (ws_copy[k] == ref_stream_copy[j]) {
		    	        found_future = 1;
		    	        break;
		    	    }
		    	    ws_steps++;
		    	}

		    	int distance;

		    	if (!found_future)
		    	    distance = 1000000000;
		    	else
		    	    distance = ws_steps;

		    	if (distance > ws_Maxsteps) {
		    	    ws_Maxsteps = distance;
		    	    ws_Maxsteps_index = k;
		    	}
		    }
		    ws_copy[ws_Maxsteps_index] = ref_stream_copy[i];
		}
		else{
		    ws_copy[ws_count++] = ref_stream_copy[i];
		}
		falut_count++;
	}
	}

	//cprintf_colored(TEXTBG_green,"FALUTS: %d\n", falut_count);
	return falut_count;

	#else
    	panic("KERNEL HEAP is OFF! get_optimal_num_faults() needs USE_KHEAP to be 1");
	#endif
	//Comment the following line
	//panic("get_optimal_num_faults() is not implemented yet...!!");
}

int is_page_in_ws(struct WS_List *ws_list, uint32 page_va){  /*Abdelaziz [PROJECT'25.IM#1] FAULT HANDLER II*/
    struct WorkingSetElement *curr;
    LIST_FOREACH_SAFE(curr, ws_list, WorkingSetElement)
    {
        if (curr->virtual_address == page_va)
        	return 1;
    }
	return 0;
}

void page_fault_handler(struct Env * faulted_env, uint32 fault_va)
{
#if USE_KHEAP
	if (isPageReplacmentAlgorithmOPTIMAL())
	{	/*Abdelaziz [PROJECT'25.IM#1] FAULT HANDLER II*/
		//TODO: [PROJECT'25.IM#1] FAULT HANDLER II - #1 Optimal Reference Stream
		//Your code is here
    	/*	1. Keep track of the Active WS
		 *	2. If faulted page not in memory, read it from disk
		 *		Else, just set its present bit
		 *	3. If the faulted page in the Active WS, do nothing
		 *		Else, if Active WS is FULL, reset present & delete all its pages
		 *	4. Add the faulted page to the Active WS
		 *	5. Add faulted page to the end of the reference stream list
		 */

		if (!optimal_initialized) //Active WS
		{
		    optimal_initialized = 1;
		    LIST_INIT(&page_active_WS_list);

		    struct WorkingSetElement *elem;
		    LIST_FOREACH_SAFE(elem, &(faulted_env->page_WS_list), WorkingSetElement)
		    {
		    	struct WorkingSetElement *copy = env_page_ws_list_create_element(faulted_env, elem->virtual_address);
		    	LIST_INSERT_TAIL(&page_active_WS_list, copy);
		    }
		}

		unsigned int page_va = ROUNDDOWN(fault_va, PAGE_SIZE);
		int perms = pt_get_page_permissions(faulted_env->env_page_directory, fault_va);

		if ((perms & PERM_PRESENT) == 0){ //if not present in mem

			uint32 *pt = NULL;
			struct FrameInfo *fi = get_frame_info(faulted_env->env_page_directory, page_va, &pt);

			if(fi == 0){ //in hard
				struct FrameInfo *f = NULL;
				int newFrame = allocate_frame(&f);
				int frameMap = map_frame(faulted_env->env_page_directory, f, fault_va, PERM_USER | PERM_PRESENT | PERM_WRITEABLE);
				int pf_errorCode = pf_read_env_page(faulted_env, (void*)fault_va);
			}
			else{ //in mem but not present
		        pt_set_page_permissions(faulted_env->env_page_directory, page_va, PERM_PRESENT, 0);
			}

		}

		if(is_page_in_ws(&(page_active_WS_list), page_va) == 0){

			if(LIST_SIZE(&(page_active_WS_list)) == faulted_env->page_WS_max_size){ //Active WS is FULL
			    struct WorkingSetElement *curr, *next;
			    for (curr = LIST_FIRST(&(page_active_WS_list)); curr != NULL; curr = next)
			    {
			        next = LIST_NEXT(curr);
			        pt_set_page_permissions(faulted_env->env_page_directory, curr->virtual_address, 0, PERM_PRESENT);
			        LIST_REMOVE(&(page_active_WS_list), curr);
			    }
			}

			struct WorkingSetElement *ws_elem = env_page_ws_list_create_element(faulted_env, fault_va);
			LIST_INSERT_TAIL(&(page_active_WS_list), ws_elem);


			struct PageRefElement *ref = kmalloc(sizeof(struct PageRefElement));
			if (!ref) panic("no memory for reference stream");
			ref->virtual_address = page_va;
			LIST_INSERT_TAIL(&(faulted_env->referenceStreamList), ref);
		}

		/*cprintf("REF STREAM: ");
		struct PageRefElement *ref;
		LIST_FOREACH(ref, &(faulted_env->referenceStreamList))
		{
		    cprintf("0x%x ", ROUNDDOWN(ref->virtual_address, PAGE_SIZE));
		}
		cprintf("\n");
		env_page_ws_print(faulted_env);*/

		//Comment the following line
		//panic("page_fault_handler().REPLACEMENT is not implemented yet...!!");
	}
	else
	{
		struct WorkingSetElement *victimWSElement = NULL;
		uint32 wsSize = LIST_SIZE(&(faulted_env->page_WS_list));
		uint32 page_va = ROUNDDOWN(fault_va, PAGE_SIZE);
		if(wsSize < (faulted_env->page_WS_max_size))
		{	/*Abdelaziz & Sama [PROJECT'25]*/
			//TODO: [PROJECT'25.GM#3] FAULT HANDLER I - #3 placement
			//Your code is here

			struct FrameInfo *f = NULL;
			int newFrame = allocate_frame(&f);
			int frameMap = map_frame(faulted_env->env_page_directory, f, fault_va, PERM_USER| PERM_PRESENT | PERM_WRITEABLE);

			int pf_errorCode = pf_read_env_page(faulted_env, (void*)fault_va);
			if(pf_errorCode == E_PAGE_NOT_EXIST_IN_PF){
				if((fault_va >= USER_HEAP_START && fault_va < USER_HEAP_MAX) ||
						(fault_va >= USTACKBOTTOM && fault_va < USTACKTOP)){
					//OK
				}
				else {
					unmap_frame(faulted_env->env_page_directory, fault_va);
					free_frame(f);
					env_exit();
				}
			}


			struct WorkingSetElement *ws_elem = env_page_ws_list_create_element(faulted_env, fault_va);
			LIST_INSERT_TAIL(&(faulted_env->page_WS_list), ws_elem);

			if(LIST_SIZE(&(faulted_env->page_WS_list)) == faulted_env->page_WS_max_size){
				faulted_env->page_last_WS_element = LIST_FIRST(&(faulted_env->page_WS_list));
			}
			else
			{
				faulted_env->page_last_WS_element = NULL;
			}

			//Comment the following line
			//panic("page_fault_handler().PLACEMENT is not implemented yet...!!");
		}
		else
		{
			if (isPageReplacmentAlgorithmCLOCK())
			{	/*Abdelaziz [PROJECT'25.IM#1] FAULT HANDLER II*/
				//TODO: [PROJECT'25.IM#1] FAULT HANDLER II - #3 Clock Replacement
				//Your code is here
				if(is_page_in_ws(&(page_active_WS_list), page_va) == 0){
					int found = 0;
					while(!found){
						int perm = pt_get_page_permissions(faulted_env->env_page_directory, faulted_env->page_last_WS_element->virtual_address);
						if(perm & PERM_USED){
							pt_set_page_permissions(faulted_env->env_page_directory, faulted_env->page_last_WS_element->virtual_address, 0, PERM_USED);

							if(faulted_env->page_last_WS_element == LIST_LAST(&(faulted_env->page_WS_list)))
								faulted_env->page_last_WS_element = LIST_FIRST(&(faulted_env->page_WS_list));
							else
								faulted_env->page_last_WS_element = faulted_env->page_last_WS_element->prev_next_info.le_next;
						}
						else{
							victimWSElement = faulted_env->page_last_WS_element;
							int perms = pt_get_page_permissions(faulted_env->env_page_directory, victimWSElement->virtual_address);
							if(perms & PERM_MODIFIED){
								uint32 *pt = NULL;
								struct FrameInfo *fi = get_frame_info(faulted_env->env_page_directory, victimWSElement->virtual_address, &pt);
								pf_update_env_page(faulted_env, victimWSElement->virtual_address, fi);
							}

							unmap_frame(faulted_env->env_page_directory, victimWSElement->virtual_address);
							if (LIST_FIRST(&(faulted_env->page_WS_list)) == victimWSElement)
							{
							    LIST_REMOVE(&(faulted_env->page_WS_list), victimWSElement);
							}
							else
							{
							    struct WorkingSetElement *curr = LIST_FIRST(&(faulted_env->page_WS_list));

							    while (curr != victimWSElement)
							    {
							        LIST_REMOVE(&(faulted_env->page_WS_list), curr);
							        LIST_INSERT_TAIL(&(faulted_env->page_WS_list), curr);
							        curr = LIST_FIRST(&(faulted_env->page_WS_list));
							    }

							    LIST_REMOVE(&(faulted_env->page_WS_list), victimWSElement);
							}
							kfree(victimWSElement);

						    page_fault_handler(faulted_env, fault_va);
							found = 1;
						}

						//env_page_ws_print(faulted_env); //Print
					}
				}
				else{
			        pt_set_page_permissions(faulted_env->env_page_directory, faulted_env->page_last_WS_element->virtual_address, PERM_USED, 0);
				}
				//Comment the following line
				//panic("page_fault_handler().REPLACEMENT is not implemented yet...!!");
			}
			else if (isPageReplacmentAlgorithmLRU(PG_REP_LRU_TIME_APPROX))
			{	/*Sama [PROJECT'25.IM#6] FAULT HANDLER II*/
				//TODO: [PROJECT'25.IM#6] FAULT HANDLER II - #2 LRU Aging Replacement
				//Your code is here

				unsigned int S_minTimeStamp=UINT_MAX;
				struct WorkingSetElement* S_victim=NULL;
				LIST_FOREACH_SAFE(S_victim, &(faulted_env-> page_WS_list), WorkingSetElement){
					if(S_victim->time_stamp<S_minTimeStamp){
						S_minTimeStamp=S_victim->time_stamp;
						victimWSElement=S_victim;
					}
				}

				int perms = pt_get_page_permissions(faulted_env->env_page_directory, victimWSElement->virtual_address);
				if(perms & PERM_MODIFIED){
					uint32 *pt = NULL;
					struct FrameInfo *fi = get_frame_info(faulted_env->env_page_directory, victimWSElement->virtual_address, &pt);
					pf_update_env_page(faulted_env, victimWSElement->virtual_address, fi);
				}

				unmap_frame(faulted_env->env_page_directory, victimWSElement->virtual_address);
				LIST_REMOVE(&(faulted_env->page_WS_list), victimWSElement);
				kfree(victimWSElement);

				struct FrameInfo *f = NULL;
				int newFrame = allocate_frame(&f);
				int frameMap = map_frame(faulted_env->env_page_directory, f, fault_va, PERM_USER | PERM_PRESENT | PERM_WRITEABLE | PERM_USED);
				int pf_errorCode = pf_read_env_page(faulted_env, (void*)fault_va);
				if (pf_errorCode == E_PAGE_NOT_EXIST_IN_PF) {
					if((fault_va >= USER_HEAP_START && fault_va < USER_HEAP_MAX) ||
					   (fault_va >= USTACKBOTTOM && fault_va < USTACKTOP)){
						   //OK
					   }

					else {
					 	unmap_frame(faulted_env->env_page_directory, fault_va);
						free_frame(f);
					    env_exit();
					}
				}

				struct WorkingSetElement *S_ws_elem = env_page_ws_list_create_element(faulted_env, fault_va);
				LIST_INSERT_TAIL(&(faulted_env->page_WS_list), S_ws_elem);

				//Comment the following line
				//panic("page_fault_handler().REPLACEMENT is not implemented yet...!!");
			}
			else if (isPageReplacmentAlgorithmModifiedCLOCK())
			{	/*Sama [PROJECT'25.IM#6] FAULT HANDLER II*/
				//TODO: [PROJECT'25.IM#6] FAULT HANDLER II - #3 Modified Clock Replacement
				//Your code is here

				struct WS_List *S_ws_list = &faulted_env->page_WS_list;
				int ws_size = faulted_env->page_WS_max_size;

				if (is_page_in_ws(&(page_active_WS_list), page_va) == 0) {
					//go back and forth between try1 and try2 until a victimis found!!!!!
					while (victimWSElement == NULL)
					{
						//Try 1: (search for a not used, not modified)
						{
							struct WorkingSetElement *S_victim = faulted_env->page_last_WS_element;
							for (int checked = 0; checked< ws_size; checked++) {
								int perms = pt_get_page_permissions(faulted_env->env_page_directory, S_victim->virtual_address);
								//cprintf_colored(TEXT_yellow, "try 1:\n");
								//env_page_ws_print(faulted_env);

								if (((perms & PERM_USED) == 0) && ((perms & PERM_MODIFIED) == 0)) {
									victimWSElement = S_victim;
									//cprintf_colored(TEXT_yellow, "try1: victim found VA=%x\n", S_victim->virtual_address);
									break;
								}

								S_victim = LIST_NEXT(S_victim);
								if (!S_victim)
									S_victim = LIST_FIRST(S_ws_list);

								faulted_env->page_last_WS_element = S_victim;
							}
						}

						if (victimWSElement)
							break;

						//Try 2: (normal clock)
						{
							struct WorkingSetElement *S_victim = faulted_env->page_last_WS_element;
							for (int checked = 0; checked < ws_size; checked++) {
								int perms = pt_get_page_permissions(faulted_env->env_page_directory,S_victim->virtual_address);

								//cprintf_colored(TEXT_yellow, "try 2:\n");
								//env_page_ws_print(faulted_env);
								if (perms & PERM_USED) {
									pt_set_page_permissions(faulted_env->env_page_directory, S_victim->virtual_address, 0, PERM_USED);
								}

								else {
									victimWSElement = S_victim;
									//cprintf_colored(TEXT_yellow, "try2: victim found VA=%x\n", S_victim->virtual_address);
									break;
								}

								S_victim = LIST_NEXT(S_victim);
								if (!S_victim)
									S_victim = LIST_FIRST(S_ws_list);

								faulted_env->page_last_WS_element = S_victim;
							}
						}

					} // while ends only when victimWSElement!=NULL


					//update clock pointer to element after victim (next search starts there)
					struct WorkingSetElement *next = LIST_NEXT(victimWSElement);
					if (!next)
						next = LIST_FIRST(S_ws_list);
					faulted_env->page_last_WS_element = next;

					int perm = pt_get_page_permissions(faulted_env->env_page_directory, victimWSElement->virtual_address);
					if (perm & PERM_MODIFIED) {
						uint32 *pt = NULL;
						struct FrameInfo *fi = get_frame_info(faulted_env->env_page_directory, victimWSElement->virtual_address, &pt);
						pf_update_env_page(faulted_env, victimWSElement->virtual_address, fi);
					}

					unmap_frame(faulted_env->env_page_directory, victimWSElement->virtual_address);

					if (LIST_FIRST(S_ws_list) == victimWSElement) {
						LIST_REMOVE(S_ws_list, victimWSElement);
					}

					else {
						struct WorkingSetElement *curr = LIST_FIRST(S_ws_list);
						while (curr != victimWSElement) {
							LIST_REMOVE(S_ws_list, curr);
							LIST_INSERT_TAIL(S_ws_list, curr);
							curr = LIST_FIRST(S_ws_list);
						}
						LIST_REMOVE(S_ws_list, victimWSElement);
					}

					kfree(victimWSElement);
					page_fault_handler(faulted_env, fault_va);

					//cprintf_colored(TEXT_yellow, "After replacement:\n");
					//env_page_ws_print(faulted_env);
				}
				else {
					pt_set_page_permissions(faulted_env->env_page_directory,faulted_env->page_last_WS_element->virtual_address,PERM_USED, 0);
				}


				//Comment the following line
				//panic("page_fault_handler().REPLACEMENT is not implemented yet...!!");
			}
		}
	}
#endif
}


void __page_fault_handler_with_buffering(struct Env * curenv, uint32 fault_va)
{
	panic("this function is not required...!!");
}



