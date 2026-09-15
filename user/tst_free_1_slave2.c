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
	//	/*Dummy malloc to enforce the UHEAP initializations*/
	//	malloc(0);
	/*=================================================*/
#endif
	uint32 expectedVA = ACTUAL_PAGE_ALLOC_START; //UHS + 32MB + 4KB

	//malloc some spaces
	int i, freeFrames, usedDiskPages, expectedNumOfTables ;
	uint32 size = 0;
	char* ptr;
	int sums[20] = {0};
	totalRequestedSize = 0;

	int eval = 0;
	bool correct ;

	correct = 1;

	//ALLOCATE ONE SPACE
	{
		//2 MB
		{
			allocIndex = 0;
			expectedVA += ROUNDUP(size, PAGE_SIZE);
			size = 2*Mega - kilo;
			totalRequestedSize += ROUNDUP(size, PAGE_SIZE);
			expectedNumOfTables = 1;
			correct = allocSpaceInPageAlloc(allocIndex, size, 1, expectedNumOfTables);
			if ((uint32) ptr_allocations[allocIndex] != (expectedVA)) { correct = 0; cprintf_colored(TEXT_TESTERR_CLR,"%~%d.3 Wrong start address for the allocated space... Expected = %x, Actual = %x\n", allocIndex, expectedVA, ptr_allocations[allocIndex]); }
		}

	}

	//FREE IT
	{
		//Free 2 MB
		{
			correct = freeSpaceInPageAlloc(0, 1);
		}
	}
	if (!correct)
	{
		return;
	}

	inctst(); //to ensure that it reached here

	//wait until receiving a signal from the master
	while (gettst() != 3) ;

	//Test accessing a freed area (processes should be killed by the validation of the fault handler)
	{
		char* byteArr = (char *) ptr_allocations[allocIndex];
		byteArr[0] = maxByte ;
		inctst();
		panic("tst_free_1_slave2 failed: The env must be killed and shouldn't return here.");
	}

	return;
}
