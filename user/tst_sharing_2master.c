// Test the creation of shared variables and using them
// Master program: create the shared variables, initialize them and run slaves
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
	uint32 pagealloc_start = ACTUAL_PAGE_ALLOC_START; //UHS + 32MB + 4KB
	uint32 *x, *y, *z ;
	int diff, expected;
	int freeFrames, usedDiskPages ;

	//x: Readonly
	freeFrames = sys_calculate_free_frames() ;
	usedDiskPages = sys_pf_calculate_allocated_pages();
	x = smalloc("x", 4, 0);
	if (x != (uint32*)pagealloc_start)
	{is_correct = 0; cprintf_colored(TEXT_TESTERR_CLR, "\nCreate(): Returned address is not correct. make sure that you align the allocation on 4KB boundary");}
	expected = 1+1 ; /*1page +1table*/
	diff = (freeFrames - sys_calculate_free_frames());
	if (!inRange(diff, expected, expected + 1 /*KH Block Alloc: 1 page for Share object*/ + 2 /*UH Block Alloc: max of 1 page & 1 table*/))
	{is_correct = 0; cprintf_colored(TEXT_TESTERR_CLR, "\nWrong allocation (current=%d, expected=[%d, %d]): make sure that you allocate the required space in the user environment and add its frames to frames_storage", freeFrames - sys_calculate_free_frames(), expected, expected +1 +2);}
	if( (sys_pf_calculate_allocated_pages() - usedDiskPages) !=  0)
	{is_correct = 0; cprintf_colored(TEXT_TESTERR_CLR, "Wrong page file allocation: ");}

	//y: Readonly
	freeFrames = sys_calculate_free_frames() ;
	usedDiskPages = sys_pf_calculate_allocated_pages();
	y = smalloc("y", 4, 0);
	if (y != (uint32*)(pagealloc_start + 1 * PAGE_SIZE))
	{is_correct = 0; cprintf_colored(TEXT_TESTERR_CLR, "\nCreate(): Returned address is not correct. make sure that you align the allocation on 4KB boundary");}
	expected = 1 ; /*1page*/
	diff = (freeFrames - sys_calculate_free_frames());
	if (!inRange(diff, expected, expected)) //no extra is expected since there'll be free blocks in Block Allo since last allocation
	{is_correct = 0; cprintf_colored(TEXT_TESTERR_CLR, "\nWrong allocation (current=%d, expected=%d): make sure that you allocate the required space in the user environment and add its frames to frames_storage", freeFrames - sys_calculate_free_frames(), expected);}
	if( (sys_pf_calculate_allocated_pages() - usedDiskPages) !=  0)
	{is_correct = 0; cprintf_colored(TEXT_TESTERR_CLR, "Wrong page file allocation: ");}

	//z: Writable
	freeFrames = sys_calculate_free_frames() ;
	usedDiskPages = sys_pf_calculate_allocated_pages();
	z = smalloc("z", 4, 1);
	if (z != (uint32*)(pagealloc_start + 2 * PAGE_SIZE)) {is_correct = 0; cprintf_colored(TEXT_TESTERR_CLR, "\nCreate(): Returned address is not correct. make sure that you align the allocation on 4KB boundary");}
	expected = 1 ; /*1page*/
	diff = (freeFrames - sys_calculate_free_frames());
	if (!inRange(diff, expected, expected)) //no extra is expected since there'll be free blocks in Block Allo since last allocation
	{is_correct = 0; cprintf_colored(TEXT_TESTERR_CLR, "\nWrong allocation (current=%d, expected=%d): make sure that you allocate the required space in the user environment and add its frames to frames_storage", freeFrames - sys_calculate_free_frames(), expected);}
	if( (sys_pf_calculate_allocated_pages() - usedDiskPages) !=  0)
	{is_correct = 0; cprintf_colored(TEXT_TESTERR_CLR, "Wrong page file allocation: ");}

	if (is_correct)	eval+=25;
	is_correct = 1;

	*x = 10 ;
	*y = 20 ;

	int id1, id2, id3;
	id1 = sys_create_env("shr2Slave1", (myEnv->page_WS_max_size),(myEnv->SecondListSize), (myEnv->percentage_of_WS_pages_to_be_removed));
	id2 = sys_create_env("shr2Slave1", (myEnv->page_WS_max_size), (myEnv->SecondListSize),(myEnv->percentage_of_WS_pages_to_be_removed));
	id3 = sys_create_env("shr2Slave1", (myEnv->page_WS_max_size), (myEnv->SecondListSize),(myEnv->percentage_of_WS_pages_to_be_removed));

	//to check that the slave environments completed successfully
	rsttst();

	sys_run_env(id1);
	sys_run_env(id2);
	sys_run_env(id3);

	//to ensure that the slave environments completed successfully
	while (gettst()!=3) ;// panic("test failed");


	if (*z != 30)
	{is_correct = 0; cprintf_colored(TEXT_TESTERR_CLR, "\nError!! Please check the creation (or the getting) of shared variables!!\n\n\n");}

	if (is_correct)	eval+=25;
	is_correct = 1;

	atomic_cprintf("\n%@Now, attempting to write a ReadOnly variable\n\n\n");

	id1 = sys_create_env("shr2Slave2", (myEnv->page_WS_max_size),(myEnv->SecondListSize), (myEnv->percentage_of_WS_pages_to_be_removed));

	sys_run_env(id1);

	//to ensure that the slave environment edits the z variable
	while (gettst() != 4) ;

	if (*z != 50)
	{is_correct = 0; cprintf_colored(TEXT_TESTERR_CLR, "\nError!! Please check the creation (or the getting) of shared variables!!\n\n\n");}

	if (is_correct)	eval+=25;
	is_correct = 1;

	//Signal slave2
	inctst();

	//to ensure that the slave environment attempt to edit the x variable
	while (gettst()!=6) ;// panic("test failed");

	if (*x != 10)
	{is_correct = 0; cprintf_colored(TEXT_TESTERR_CLR, "\nError!! Please check the creation (or the getting) of shared variables!!\n\n\n");}

	if (is_correct)	eval+=25;
	is_correct = 1;

	cprintf_colored(TEXT_light_green, "\n\n%~Test of Shared Variables [Create & Get] completed. Eval = %d%%\n\n", eval);
	return;
}
