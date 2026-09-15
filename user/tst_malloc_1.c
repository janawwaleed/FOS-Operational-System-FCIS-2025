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
	 * PAGES ARE ALLOCATED IN BLOCK ALLOCATOR
	 * (e.g. DURING THE DYNAMIC CREATION OF WS ELEMENT in FH
	 * or DURING THE MANAGEMENT OF THE PAGE ALLOCATOR ITSELF).
	 *********************************************************/

	//cprintf("1\n");
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
	bool correct ;

	correct = 1;
	//Create some areas in PAGE allocators
	cprintf_colored(TEXT_cyan,"%~\n1 Create some allocations\n");
	{
		eval = initial_page_allocations();
		eval = eval * 70 / 100; //rescale
	}

	//2. Check BREAK
	correct = 1;
	cprintf_colored(TEXT_cyan,"%~\n2. Check Page Allocator BREAK [10%]\n");
	{
		uint32 allocSizes = 0;
		for (int i = 0; i < allocIndex; ++i)
		{
			allocSizes += ROUNDUP(requestedSizes[i], PAGE_SIZE);
		}
		uint32 expectedVA = ACTUAL_PAGE_ALLOC_START + allocSizes;
		if(uheapPageAllocBreak != expectedVA) {correct = 0; cprintf_colored(TEXT_TESTERR_CLR,"BREAK value is not correct! Expected = %x, Actual = %x\n", expectedVA, uheapPageAllocBreak);}
	}
	if (correct) eval += 10;
	correct = 1;

	//3. Check Content
	uint32 sums[MAX_NUM_OF_ALLOCS] = {0};
	cprintf_colored(TEXT_cyan,"%~\n3. Check Content [20%]\n");
	{
		for (int i = 0; i < allocIndex; ++i)
		{
			char* ptr = (char*)ptr_allocations[i];
			sums[i] += ptr[0] ;
			sums[i] += ptr[lastIndices[i]] ;
			if (sums[i] != (maxByte + maxByte))
			{ correct = 0; cprintf_colored(TEXT_TESTERR_CLR,"invalid content in allocation#%d. Expected = %d, Actual = %d\n", i, maxByte + maxByte, sums[i]); }
		}
	}
	if (correct) eval += 20;
	correct = 1;

	cprintf_colored(TEXT_light_green, "%~\nTest malloc (1) [PAGE ALLOCATOR] completed. Eval = %d\n", eval);

	return;
}
