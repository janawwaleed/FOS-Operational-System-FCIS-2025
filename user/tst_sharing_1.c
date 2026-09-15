// Test the creation of shared variables (create_shared_memory)
#include <inc/lib.h>
#include <user/tst_malloc_helpers.h>

void
_main(void)
{
	/*********************** NOTE ****************************
	 * WE COMPARE THE DIFF IN FREE FRAMES BY "AT LEAST" RULE
	 * INSTEAD OF "EQUAL" RULE SINCE IT'S POSSIBLE THAT SOME
	 * PAGES ARE ALLOCATED IN KERNEL BLOCK ALLOCATOR OR USER
	 * BLOCK ALLOCATOR DUE TO DIFFERENT MANAGEMENT OF USER HEAP
	 *********************************************************/

	cprintf_colored(TEXT_yellow, "%~************************************************\n");
	cprintf_colored(TEXT_yellow, "%~MAKE SURE to have a FRESH RUN for this test\n(i.e. don't run any program/test before it)\n");
	cprintf_colored(TEXT_yellow, "%~************************************************\n\n\n");

	/*=================================================*/
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

	uint32 *x, *y, *z ;
	uint32 expected ;
	uint32 pagealloc_start = ACTUAL_PAGE_ALLOC_START; //UHS + 32MB + 4KB
	int freeFrames, usedDiskPages ;

	cprintf_colored(TEXT_cyan, "\n%~STEP A: checking the creation of shared variables... [60%]\n");
	{
		is_correct = 1;
		freeFrames = sys_calculate_free_frames() ;
		usedDiskPages = sys_pf_calculate_allocated_pages();
		x = smalloc("x", PAGE_SIZE, 1);
		if (x != (uint32*)pagealloc_start)
		{is_correct = 0; cprintf_colored(TEXT_TESTERR_CLR, "%~1 Returned address is not correct. check the setting of it and/or the updating of the shared_mem_free_address");}
		expected = 1+1 ; /*1page +1table*/
		int diff = (freeFrames - sys_calculate_free_frames());
		if (!inRange(diff, expected, expected + 1 /*KH Block Alloc: 1 page for Share object*/ + 2 /*UH Block Alloc: max of 1 page & 1 table*/))
		{is_correct = 0; cprintf_colored(TEXT_TESTERR_CLR, "%~1 Wrong allocation (actual=%d, expected=[%d, %d]): make sure that you allocate the required space in the user environment and add its frames to frames_storage", freeFrames - sys_calculate_free_frames(), expected, expected +1 +2);}
		if( (sys_pf_calculate_allocated_pages() - usedDiskPages) !=  0)
		{is_correct = 0; cprintf_colored(TEXT_TESTERR_CLR, "%~1 Wrong page file allocation: ");}
		if (is_correct) eval += 20 ;

		is_correct = 1;
		freeFrames = sys_calculate_free_frames() ;
		usedDiskPages = sys_pf_calculate_allocated_pages();
		z = smalloc("z", PAGE_SIZE + 4, 1);
		if (z != (uint32*)(pagealloc_start + 1 * PAGE_SIZE))
		{is_correct = 0; cprintf_colored(TEXT_TESTERR_CLR, "%~2 Returned address is not correct. check the setting of it and/or the updating of the shared_mem_free_address");}
		expected = 2 ; /*2 pages*/
		diff = (freeFrames - sys_calculate_free_frames());
		if (!inRange(diff, expected, expected)) //no extra is expected since there'll be free blocks in Block Allo since last allocation
		{is_correct = 0; cprintf_colored(TEXT_TESTERR_CLR, "%~2 Wrong allocation (current=%d, expected=%d): make sure that you allocate the required space in the user environment and add its frames to frames_storage", freeFrames - sys_calculate_free_frames(), expected);}
		if( (sys_pf_calculate_allocated_pages() - usedDiskPages) !=  0)
		{is_correct = 0; cprintf_colored(TEXT_TESTERR_CLR, "%~2 Wrong page file allocation: ");}
		if (is_correct) eval += 20 ;

		is_correct = 1;
		freeFrames = sys_calculate_free_frames() ;
		usedDiskPages = sys_pf_calculate_allocated_pages();
		y = smalloc("y", 4, 1);
		if (y != (uint32*)(pagealloc_start + 3 * PAGE_SIZE))
		{is_correct = 0; cprintf_colored(TEXT_TESTERR_CLR, "%~3 Returned address is not correct. check the setting of it and/or the updating of the shared_mem_free_address");}
		expected = 1 ; /*1 page*/
		diff = (freeFrames - sys_calculate_free_frames());
		if (!inRange(diff, expected, expected)) //no extra is expected since there'll be free blocks in Block Allo since last allocation
		{is_correct = 0; cprintf_colored(TEXT_TESTERR_CLR, "%~3 Wrong allocation (current=%d, expected=%d): make sure that you allocate the required space in the user environment and add its frames to frames_storage", freeFrames - sys_calculate_free_frames(), expected);}
		if( (sys_pf_calculate_allocated_pages() - usedDiskPages) !=  0)
		{is_correct = 0; cprintf_colored(TEXT_TESTERR_CLR, "%~3 Wrong page file allocation: ");}
		if (is_correct) eval += 20 ;
	}

	is_correct = 1;
	cprintf_colored(TEXT_cyan, "\n%~STEP B: checking reading & writing... [40%]\n");
	{
		int i=0;
		for(;i<PAGE_SIZE/4;i++)
		{
			x[i] = -1;
			y[i] = -1;
		}

		i=0;
		for(;i<2*PAGE_SIZE/4;i++)
		{
			z[i] = -1;
		}

		if( x[0] !=  -1)  					{is_correct = 0; cprintf_colored(TEXT_TESTERR_CLR, "%~Reading/Writing of shared object is failed");}
		if( x[PAGE_SIZE/4 - 1] !=  -1)  	{is_correct = 0; cprintf_colored(TEXT_TESTERR_CLR, "%~Reading/Writing of shared object is failed");}

		if( y[0] !=  -1)  					{is_correct = 0; cprintf_colored(TEXT_TESTERR_CLR, "%~Reading/Writing of shared object is failed");}
		if( y[PAGE_SIZE/4 - 1] !=  -1)  	{is_correct = 0; cprintf_colored(TEXT_TESTERR_CLR, "%~Reading/Writing of shared object is failed");}

		if( z[0] !=  -1)  					{is_correct = 0; cprintf_colored(TEXT_TESTERR_CLR, "%~Reading/Writing of shared object is failed");}
		if( z[2*PAGE_SIZE/4 - 1] !=  -1)  	{is_correct = 0; cprintf_colored(TEXT_TESTERR_CLR, "%~Reading/Writing of shared object is failed");}
	}
	if (is_correct)
		eval += 40 ;
	cprintf_colored(TEXT_light_green, "%~\n%~Test of Shared Variables [Create] [1] completed. Eval = %d%%\n\n", eval);

	return;
}
