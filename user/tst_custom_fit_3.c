/* *********************************************************** */
/* MAKE SURE PAGE_WS_MAX_SIZE = 3000 */
/* *********************************************************** */
#include <inc/lib.h>
#include <user/tst_malloc_helpers.h>

void _main(void)
{
	sys_set_uheap_strategy(UHP_PLACE_CUSTOMFIT);

	/*********************** NOTE ****************************
	 * WE COMPARE THE DIFF IN FREE FRAMES BY "AT LEAST" RULE
	 * INSTEAD OF "EQUAL" RULE SINCE IT'S POSSIBLE THAT SOME
	 * PAGES ARE ALLOCATED IN BLOCK ALLOCATOR OF KERNEL/USER
	 * (e.g. DURING THE CREATION OF SHARE OBJECT/FRAMES STORAGE).
	 *********************************************************/

	//cprintf_colored(TEXT_TESTERR_CLR, "%~1\n");
	//Initial test to ensure it works on "PLACEMENT" not "REPLACEMENT"
#if USE_KHEAP
	{
		if (LIST_SIZE(&(myEnv->page_WS_list)) >= myEnv->page_WS_max_size)
			panic("Please increase the WS size");
	}
#else
	panic("make sure to enable the kernel heap: USE_KHEAP=1");
#endif
	/*=================================================*/

	int eval = 0;
	bool is_correct = 1;

	int envID = sys_getenvid();

	uint32 pagealloc_start = ACTUAL_PAGE_ALLOC_START; //UHS + 32MB + 4KB

	void* ptr_allocations[20] = {0};
	int freeFrames, expected, expectedUpper, diff;
	int usedDiskPages;
	//[1] Allocate all
	cprintf_colored(TEXT_cyan, "\n%~[1] Allocate spaces of different sizes in PAGE ALLOCATOR [20%]\n");
	{
		//Allocate Shared 1 MB
		freeFrames = sys_calculate_free_frames() ;
		usedDiskPages = sys_pf_calculate_allocated_pages();
		ptr_allocations[0] = smalloc("x", 1*Mega, 1);
		if (ptr_allocations[0] != (uint32*)pagealloc_start)
		{is_correct = 0; cprintf_colored(TEXT_TESTERR_CLR, "%~Returned address is not correct. check the setting of it and/or the updating of the shared_mem_free_address");}
		expected = 256+1; /*256pages +1table*/
		expectedUpper = expected
						+2 /*KH Block Alloc: 1 for Share object, 1 for framesStorage*/
						+2 /*UH Block Alloc: max of 1 page & 1 table*/;
		diff = (freeFrames - sys_calculate_free_frames());
		if (!inRange(diff, expected, expectedUpper))
		{is_correct = 0; cprintf_colored(TEXT_TESTERR_CLR, "%~Wrong allocation (current=%d, expected=[%d, %d]): make sure that you allocate the required space in the user environment and add its frames to frames_storage", freeFrames - sys_calculate_free_frames(), expected, expectedUpper);}
		if( (sys_pf_calculate_allocated_pages() - usedDiskPages) !=  0) {is_correct = 0; cprintf_colored(TEXT_TESTERR_CLR, "%~Wrong page file allocation: ");}

		//Allocate 1 MB
		ptr_allocations[1] = malloc(1*Mega-kilo);
		if ((uint32) ptr_allocations[1] != (pagealloc_start + 1*Mega))
		{is_correct = 0; cprintf_colored(TEXT_TESTERR_CLR, "%~Wrong start address for the allocated space... ");}

		//Allocate 1 MB
		ptr_allocations[2] = malloc(1*Mega-kilo);
		if ((uint32) ptr_allocations[2] != (pagealloc_start + 2*Mega))
		{is_correct = 0; cprintf_colored(TEXT_TESTERR_CLR, "%~Wrong start address for the allocated space... ");}

		//Allocate 1 MB (New Table)
		ptr_allocations[3] = malloc(1*Mega-kilo);
		if ((uint32) ptr_allocations[3] != (pagealloc_start + 3*Mega) )
		{is_correct = 0; cprintf_colored(TEXT_TESTERR_CLR, "%~Wrong start address for the allocated space... ");}

		//Allocate 2 MB
		ptr_allocations[4] = malloc(2*Mega-kilo);
		if ((uint32) ptr_allocations[4] != (pagealloc_start + 4*Mega))
		{is_correct = 0; cprintf_colored(TEXT_TESTERR_CLR, "%~Wrong start address for the allocated space... ");}

		//Allocate Shared 2 MB (New Table)
		freeFrames = sys_calculate_free_frames() ;
		usedDiskPages = sys_pf_calculate_allocated_pages();
		ptr_allocations[5] = smalloc("y", 2*Mega, 1);
		if (ptr_allocations[5] != (uint32*)(pagealloc_start + 6*Mega))
		{is_correct = 0; cprintf_colored(TEXT_TESTERR_CLR, "%~Returned address is not correct. check the setting of it and/or the updating of the shared_mem_free_address");}
		expected = 512+1; /*512pages +1table*/
		expectedUpper = expected +1 /*KH Block Alloc: 1 for framesStorage*/;
		diff = (freeFrames - sys_calculate_free_frames());
		if (!inRange(diff, expected, expectedUpper))
		{is_correct = 0; cprintf_colored(TEXT_TESTERR_CLR, "%~Wrong allocation (current=%d, expected=[%d, %d]): make sure that you allocate the required space in the user environment and add its frames to frames_storage", freeFrames - sys_calculate_free_frames(), expected, expectedUpper);}
		if( (sys_pf_calculate_allocated_pages() - usedDiskPages) !=  0) {is_correct = 0; cprintf_colored(TEXT_TESTERR_CLR, "%~Wrong page file allocation: ");}

		//Allocate 3 MB
		ptr_allocations[6] = malloc(3*Mega-kilo);
		if ((uint32) ptr_allocations[6] != (pagealloc_start + 8*Mega))
		{is_correct = 0; cprintf_colored(TEXT_TESTERR_CLR, "%~Wrong start address for the allocated space... ");}

		//Allocate Shared 3 MB (New Table)
		freeFrames = sys_calculate_free_frames() ;
		usedDiskPages = sys_pf_calculate_allocated_pages();
		ptr_allocations[7] = smalloc("z", 3*Mega, 0);
		if (ptr_allocations[7] != (uint32*)(pagealloc_start + 11*Mega))
		{is_correct = 0; cprintf_colored(TEXT_TESTERR_CLR, "%~Returned address is not correct. check the setting of it and/or the updating of the shared_mem_free_address");}
		expected = 768+1; /*768pages +1table */
		expectedUpper = expected +1 /*+1page for framesStorage by Kernel Page Allocator since it exceed 2KB size*/;
		diff = (freeFrames - sys_calculate_free_frames());
		if (!inRange(diff, expected, expectedUpper))
		{is_correct = 0; cprintf_colored(TEXT_TESTERR_CLR, "%~Wrong allocation (current=%d, expected=[%d, %d]): make sure that you allocate the required space in the user environment and add its frames to frames_storage", freeFrames - sys_calculate_free_frames(), expected, expectedUpper);}
		if( (sys_pf_calculate_allocated_pages() - usedDiskPages) !=  0) {is_correct = 0; cprintf_colored(TEXT_TESTERR_CLR, "%~Wrong page file allocation: ");}
	}
	if (is_correct)	eval+=20;
	is_correct = 1;

	//[2] Free some to create holes
	cprintf_colored(TEXT_cyan, "\n%~[2] Free some to create holes\n");
	{
		//1 MB Hole
		free(ptr_allocations[1]);

		//2 MB Hole
		free(ptr_allocations[4]);

		//3 MB Hole
		free(ptr_allocations[6]);
	}

	//[3] Allocate again [test custom fit]
	cprintf_colored(TEXT_cyan, "\n%~[3] Allocate again [test custom fit] [40%]\n");
	{
		//Allocate Shared 512 KB - should be placed in 3rd hole [WORST FIT]
		is_correct = 1;
		{
			ptr_allocations[8] = smalloc("Cust1", 512*kilo - kilo, 1);
			if ((uint32) ptr_allocations[8] != (pagealloc_start + 8*Mega))
			{is_correct = 0; cprintf_colored(TEXT_TESTERR_CLR, "%~Wrong start address for the allocated space... ");}
		}
		if (is_correct)	eval+=10;
		is_correct = 1;

		//Get Shared 2 MB - should be placed in 2nd hole [EXACT FIT]
		is_correct = 1;
		{
			ptr_allocations[9] = sget(envID, "y");
			if ((uint32) ptr_allocations[9] != (pagealloc_start + 4*Mega))
			{is_correct = 0; cprintf_colored(TEXT_TESTERR_CLR, "%~Wrong start address for the allocated space... ");}
		}
		if (is_correct)	eval+=10;
		is_correct = 1;

		//Allocate Shared 2 MB + 512 KB - should be placed in remaining of 3rd hole [EXACT FIT]
		is_correct = 1;
		{
			ptr_allocations[10] = smalloc("Cust2", 2*Mega + 512*kilo, 1);
			if ((uint32) ptr_allocations[10] != (pagealloc_start + 8*Mega + 512*kilo))
			{is_correct = 0; cprintf_colored(TEXT_TESTERR_CLR, "%~Wrong start address for the allocated space... ");}
		}
		if (is_correct)	eval+=5;
		is_correct = 1;

		//Get Shared 512 KB - should be placed in 1st hole [WORST FIT]
		is_correct = 1;
		{
			ptr_allocations[11] = sget(envID, "Cust1");
			if ((uint32) ptr_allocations[11] != (pagealloc_start + 1*Mega))
			{is_correct = 0; cprintf_colored(TEXT_TESTERR_CLR, "%~Wrong start address for the allocated space... ");}
		}
		if (is_correct)	eval+=5;
		is_correct = 1;

		//Allocate Shared 4 MB - should be placed in end of all allocations [EXTEND]
		is_correct = 1;
		{
			ptr_allocations[12] = smalloc("Cust3", 4*Mega, 0);
			if ((uint32) ptr_allocations[12] != (pagealloc_start + 14*Mega))
			{is_correct = 0; cprintf_colored(TEXT_TESTERR_CLR, "%~Wrong start address for the allocated space... ");}
		}
		if (is_correct)	eval+=10;
		is_correct = 1;
	}

	//[4] Free contiguous allocations
	cprintf_colored(TEXT_cyan, "%~\n%~[4] Free contiguous allocations\n");
	{
		//1 MB Hole
		free(ptr_allocations[3]);

		//1 MB Hole appended to previous 512 KB hole and next 1 MB Hole => 2MB + 512KB Hole
		free(ptr_allocations[2]);
	}

	//[5] Allocate again [test custom fit]
	cprintf_colored(TEXT_cyan, "%~\n%~[5] Allocate again [test custom fit] [40%]\n");
	{
		//Allocate Shared 2 MB + 1 KB - should be placed in the contiguous hole (512 KB + 2 MB)
		is_correct = 1;
		{
			ptr_allocations[13] = smalloc("Cust4", 2*Mega + 1*kilo, 0);
			if ((uint32) ptr_allocations[13] != (pagealloc_start + 1*Mega + 512*kilo))
			{is_correct = 0; cprintf_colored(TEXT_TESTERR_CLR, "%~Wrong start address for the allocated space... ");}
		}
		if (is_correct)	eval+=15;
		is_correct = 1;

		//Get Shared of 1 MB [should be placed at the end of all allocations]
		is_correct = 1;
		{
			ptr_allocations[14] = sget(envID, "x");
			if ((uint32) ptr_allocations[14] != (pagealloc_start + 18*Mega))
			{is_correct = 0; cprintf_colored(TEXT_TESTERR_CLR, "%~Wrong start address for the allocated space... ");}
		}
		if (is_correct)	eval+=10;
		is_correct = 1;

		//Allocate shared of 508 KB [should be placed in the remaining part of the contiguous (512 KB + 2 MB) hole
		is_correct = 1;
		{
			ptr_allocations[15] = smalloc("Cust5", 508*kilo, 0);
			if ((uint32) ptr_allocations[15] != (pagealloc_start + 3*Mega + 512*kilo + 4*kilo))
			{is_correct = 0; cprintf_colored(TEXT_TESTERR_CLR, "%~Wrong start address for the allocated space... ");}
		}
		if (is_correct)	eval+=15;
		is_correct = 1;
	}

	cprintf_colored(TEXT_light_green, "%~\ntest Sharing CUSTOM FIT allocation (3) completed. Eval = %d%%\n\n", eval);

	return;
}
