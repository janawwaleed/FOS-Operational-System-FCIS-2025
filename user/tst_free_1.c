/* *********************************************************** */
/* MAKE SURE PAGE_WS_MAX_SIZE = 3000 */
/* *********************************************************** */
#include <inc/lib.h>
#include <user/tst_malloc_helpers.h>

void _main(void)
{
	/*********************** NOTE ****************************
	 * WE COMPARE THE DIFF IN FREE FRAMES BY "AT LEAST" RULE
	 * INSTEAD OF "EQUAL" RULE SINCE IT'S POSSIBLE THAT SOME
	 * PAGES ARE ALLOCATED IN DYNAMIC ALLOCATOR DUE TO sbrk()
	 * (e.g. DURING THE DYNAMIC CREATION OF WS ELEMENT in FH).
	 *********************************************************/

#if USE_KHEAP
	//cprintf("1\n");
	//Initial test to ensure it works on "PLACEMENT" not "REPLACEMENT"
	{
		if (LIST_SIZE(&(myEnv->page_WS_list)) >= myEnv->page_WS_max_size)
			panic("Please increase the WS size");
	}
	/*=================================================*/
#else
	panic("not handled!");
#endif
	//1. Alloc some spaces in PAGE allocator
	int correct = 1;
	int eval;
	cprintf_colored(TEXT_cyan,"\n1. Alloc some spaces in PAGE allocator\n");
	{
		eval = initial_page_allocations();
		if (eval != 100)
		{
			cprintf_colored(TEXT_TESTERR_CLR,"initial allocations are not correct!\nplease make sure the the kmalloc test is correct before testing the kfree\n");
			return ;
		}
	}
	eval = 0;
	uint32 pagealloc_end = ACTUAL_PAGE_ALLOC_START + totalRequestedSize ;


	correct = 1;
	//2. FREE Some
	cprintf_colored(TEXT_cyan,"%~\n2. Free some allocated spaces from PAGE ALLOCATOR [50%]\n");
	{
		//3 MB Hole
		correct = freeSpaceInPageAlloc(1, 1);
		if (correct) eval += 10;
		correct = 1;

		//2nd 4 MB Hole
		correct = freeSpaceInPageAlloc(3, 1);
		if (correct) eval += 10;
		correct = 1;

		//2nd 1 MB Hole
		correct = freeSpaceInPageAlloc(5, 1);
		if (correct) eval += 5;
		correct = 1;

		//2nd 2 MB Hole
		correct = freeSpaceInPageAlloc(7, 1);
		if (correct) eval += 5;
		correct = 1;

		//1st 3 KB Hole
		correct = freeSpaceInPageAlloc(9, 1);
		if (correct) eval += 5;
		correct = 1;

		//2nd 3 KB Hole
		correct = freeSpaceInPageAlloc(11, 1);
		if (correct) eval += 5;
		correct = 1;

		//5 KB Hole (should be merged with prev & next)
		correct = freeSpaceInPageAlloc(10, 1);
		if (correct) eval += 5;
		correct = 1;

		//LAST 9 KB Hole (break should be moved down to the begin of alloc#9)
		correct = freeSpaceInPageAlloc(12, 1);
		if (correct) eval += 5;
		correct = 1;
	}

	//3. Check the move-down of the BREAK
	correct = 1;
	cprintf_colored(TEXT_cyan,"%~\n3. Check the move-down of the BREAK [20%]\n");
	{
		uint32 expectedBreak = ACTUAL_PAGE_ALLOC_START + totalRequestedSize - 28*kilo;
		if(uheapPageAllocBreak != expectedBreak)
		{correct = 0; cprintf_colored(TEXT_TESTERR_CLR,"BREAK value is not correct! Expected = %x, Actual = %x\n", expectedBreak, uheapPageAllocBreak);}
	}
	if (correct) eval += 20;

	//4. Test accessing a freed area (processes should be killed by the validation of the fault handler)
	correct = 1;
	cprintf_colored(TEXT_cyan,"%~\n4. Test accessing a freed area (processes should be killed by the validation of the fault handler) [30%]\n");
	{
		rsttst();
		int ID1 = sys_create_env("tf1_slave1", (myEnv->page_WS_max_size), (myEnv->SecondListSize),(myEnv->percentage_of_WS_pages_to_be_removed));
		sys_run_env(ID1);

		//wait until the 1st slave finishes the allocation & freeing operations
		while (gettst() != 1) ;

		int ID2 = sys_create_env("tf1_slave2", (myEnv->page_WS_max_size), (myEnv->SecondListSize),(myEnv->percentage_of_WS_pages_to_be_removed));
		sys_run_env(ID2);

		//wait until the 2nd slave finishes the allocation & freeing operations
		while (gettst() != 2) ;

		//signal them to start accessing the freed area
		inctst();

		//sleep for a while to allow each slave to try access its freed location
		env_sleep(15000);

		if (gettst() > 3)
		{ correct = 0; cprintf_colored(TEXT_TESTERR_CLR,"Free: access to freed space is done while it's NOT expected to be!! (processes should be killed by the validation of the fault handler)\n");}
	}
	if (correct)
	{
		eval += 30;
	}
	cprintf_colored(TEXT_light_green, "%~\ntest free [1] [PAGE ALLOCATOR] completed. Eval = %d\n\n", eval);

	return;
}
