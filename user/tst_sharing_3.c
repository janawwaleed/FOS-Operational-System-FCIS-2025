// Test the SPECIAL CASES during the creation & get of shared variables
#include <inc/lib.h>
#include <user/tst_malloc_helpers.h>

void
_main(void)
{
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

	cprintf_colored(TEXT_yellow, "%~************************************************\n");
	cprintf_colored(TEXT_yellow, "%~MAKE SURE to have a FRESH RUN for this test\n(i.e. don't run any program/test before it)\n");
	cprintf_colored(TEXT_yellow, "%~************************************************\n\n\n");

	int eval = 0;
	bool is_correct = 1;

	uint32 pagealloc_start = ACTUAL_PAGE_ALLOC_START; //UHS + 32MB + 4KB
	int freeFrames, usedDiskPages ;

	uint32 *x, *y, *z ;
	cprintf_colored(TEXT_cyan, "\n%~STEP A: checking creation of shared object that is already exists... [35%] \n\n");
	{
		int ret ;
		//int ret = sys_createSharedObject("x", PAGE_SIZE, 1, (void*)&x);
		x = smalloc("x", PAGE_SIZE, 1);
		freeFrames = sys_calculate_free_frames() ;
		usedDiskPages = sys_pf_calculate_allocated_pages();
		x = smalloc("x", PAGE_SIZE, 1);
		if (x != NULL) {is_correct = 0;
		cprintf_colored(TEXT_TESTERR_CLR, "%~Trying to create an already exists object and corresponding error is not returned!!");}
		if ((freeFrames - sys_calculate_free_frames()) !=  0)
		{is_correct = 0; cprintf_colored(TEXT_TESTERR_CLR, "%~Wrong allocation: make sure that you don't allocate any memory if the shared object exists");}
		if( (sys_pf_calculate_allocated_pages() - usedDiskPages) !=  0)
		{is_correct = 0; cprintf_colored(TEXT_TESTERR_CLR, "%~Wrong page file allocation: ");}
	}
	if (is_correct)	eval+=35;
	is_correct = 1;

	cprintf_colored(TEXT_cyan, "\n%~STEP B: checking getting shared object that is NOT exists... [35%]\n\n");
	{
		int ret ;
		x = sget(myEnv->env_id, "xx");
		freeFrames = sys_calculate_free_frames() ;
		usedDiskPages = sys_pf_calculate_allocated_pages();
		if (x != NULL)
		{is_correct = 0; cprintf_colored(TEXT_TESTERR_CLR, "%~Trying to get a NONE existing object and corresponding error is not returned!!");}
		if ((freeFrames - sys_calculate_free_frames()) !=  0)
		{is_correct = 0; cprintf_colored(TEXT_TESTERR_CLR, "%~Wrong get: make sure that you don't allocate any memory if the shared object not exists");}
		if( (sys_pf_calculate_allocated_pages() - usedDiskPages) !=  0)
		{is_correct = 0; cprintf_colored(TEXT_TESTERR_CLR, "%~Wrong page file allocation: ");}
	}
	if (is_correct)	eval+=35;
	is_correct = 1;

	cprintf_colored(TEXT_cyan, "\n%~STEP C: checking the creation of shared object that exceeds the SHARED area limit... [30%]\n\n");
	{
		freeFrames = sys_calculate_free_frames() ;
		usedDiskPages = sys_pf_calculate_allocated_pages();
		uint32 size = USER_HEAP_MAX - pagealloc_start - PAGE_SIZE + 1;
		y = smalloc("y", size, 1);
		if (y != NULL)
		{is_correct = 0; cprintf_colored(TEXT_TESTERR_CLR, "%~Trying to create a shared object that exceed the SHARED area limit and the corresponding error is not returned!!");}
		if ((freeFrames - sys_calculate_free_frames()) !=  0)
		{is_correct = 0; cprintf_colored(TEXT_TESTERR_CLR, "%~Wrong allocation: make sure that you don't allocate any memory if the shared object exceed the SHARED area limit");}
		if( (sys_pf_calculate_allocated_pages() - usedDiskPages) !=  0)
		{is_correct = 0; cprintf_colored(TEXT_TESTERR_CLR, "%~Wrong page file allocation: ");}
	}
	if (is_correct)	eval+=30;
	is_correct = 1;

	cprintf_colored(TEXT_light_green, "%~\nTest of Shared Variables [Create & Get: Special Cases] completed. Eval = %d%%\n\n", eval);

}
