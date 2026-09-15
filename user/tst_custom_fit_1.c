/* *********************************************************** */
/* MAKE SURE PAGE_WS_MAX_SIZE = 3000 */
/* *********************************************************** */
#include <inc/lib.h>
#include <user/tst_malloc_helpers.h>

void _main(void)
{
#if USE_KHEAP

	sys_set_uheap_strategy(UHP_PLACE_CUSTOMFIT);

	/*********************** NOTE ****************************
	 * WE COMPARE THE DIFF IN FREE FRAMES BY "AT LEAST" RULE
	 * INSTEAD OF "EQUAL" RULE SINCE IT'S POSSIBLE THAT SOME
	 * PAGES ARE ALLOCATED IN BLOCK ALLOCATOR OF KERNEL/USER
	 * (e.g. DURING THE DYNAMIC CREATION OF WS ELEMENT in FH).
	 *********************************************************/

	//cprintf("1\n");
	//Initial test to ensure it works on "PLACEMENT" not "REPLACEMENT"
	{
		if (LIST_SIZE(&(myEnv->page_WS_list)) >= myEnv->page_WS_max_size)
			panic("Please increase the WS size");
	}
	/*=================================================*/
	int correct = 1;
	int eval;
	int expectedNumOfTables;

	//1. Alloc some spaces in PAGE allocator
	cprintf_colored(TEXT_cyan,"%~\n1. Alloc some spaces in PAGE allocator\n");
	{
		eval = initial_page_allocations();
		if (eval != 100)
		{
			cprintf_colored(TEXT_TESTERR_CLR,"initial allocations are not correct!\nplease make sure the the kmalloc test is correct before testing the kfree\n");
			return ;
		}
	}
	eval = 0;

	//2. Free some allocations to create initial holes
	cprintf_colored(TEXT_cyan,"%~\n2. Free some allocations to create initial holes [5%]\n");
	correct = 1;
	{
		//3 MB Hole
		correct = freeSpaceInPageAlloc(1, 1);

		//2nd 4 MB Hole
		correct = freeSpaceInPageAlloc(3, 1);

		//2nd 1 MB Hole
		correct = freeSpaceInPageAlloc(5, 1);

		//2nd 2 MB Hole
		correct = freeSpaceInPageAlloc(7, 1);
	}
	if (correct) eval += 5;
	correct = 1;

	//3. Check content of un-freed spaces
	uint32 sums[MAX_NUM_OF_ALLOCS] = {0};
	cprintf_colored(TEXT_cyan,"%~\n3. Check content of un-freed spaces [5%]\n");
	{
		for (int i = 0; i < allocIndex; ++i)
		{
			//skip the freed spaces
			if (i == 1 || i == 3 || i == 5 || i == 7)
				continue;
			char* ptr = (char*)ptr_allocations[i];
			sums[i] += ptr[0] ;
			sums[i] += ptr[lastIndices[i]] ;
			if (sums[i] != (maxByte + maxByte))
			{ correct = 0; cprintf_colored(TEXT_TESTERR_CLR,"invalid content in alloc#%d. Expected = %d, Actual = %d\n", i, 2*maxByte, sums[i]); }
		}
	}
	if (correct) eval += 5;
	correct = 1;

	//4. Check BREAK
	correct = 1;
	uint32 expectedBreak = 0;
	cprintf_colored(TEXT_cyan,"%~\n4. Check BREAK [5%]\n");
	{
		expectedBreak = ACTUAL_PAGE_ALLOC_START + totalRequestedSize;
		if(uheapPageAllocBreak != expectedBreak)
		{correct = 0; cprintf_colored(TEXT_TESTERR_CLR,"BREAK value is not correct! Expected = %x, Actual = %x\n", expectedBreak, uheapPageAllocBreak);}
	}
	if (correct) eval += 5;
	correct = 1;

	//5. Allocate after kfree [Test CUSTOM FIT]
	uint32 allocIndex,expectedVA, size = 0;
	cprintf_colored(TEXT_cyan,"%~\n5. Allocate after free [Test CUSTOM FIT] [30%]\n");
	{
		//1 MB [EXACT FIT in 1MB Hole (alloc#5)]
		allocIndex = 14;
		size = 1*Mega - kilo;
		expectedNumOfTables = 0;
		correct = allocSpaceInPageAlloc(allocIndex, size, 1, expectedNumOfTables);
		expectedVA = (uint32)ptr_allocations[5] ; //Address of 1MB Hole
		if ((uint32) ptr_allocations[allocIndex] != (expectedVA)) { correct = 0; cprintf_colored(TEXT_TESTERR_CLR,"%d.3 Wrong start address for the allocated space... Expected = %x, Actual = %x\n", allocIndex, expectedVA, ptr_allocations[allocIndex]); }

		//1MB + 4KB [WORST FIT in 4MB Hole (alloc#3)]
		allocIndex = 15;
		size = 1*Mega + 4*kilo;
		expectedNumOfTables = 0;
		correct = allocSpaceInPageAlloc(allocIndex, size, 1, expectedNumOfTables);
		expectedVA = (uint32)ptr_allocations[3] ; //Address of 4MB Hole
		if ((uint32) ptr_allocations[allocIndex] != (expectedVA)) { correct = 0; cprintf_colored(TEXT_TESTERR_CLR,"%d.3 Wrong start address for the allocated space... Expected = %x, Actual = %x\n", allocIndex, expectedVA, ptr_allocations[allocIndex]); }

		//3MB - 4KB [EXACT FIT in remaining area of 4MB Hole (alloc#3)]
		allocIndex = 16;
		size = 3*Mega - 4*kilo;
		expectedNumOfTables = 0;
		correct = allocSpaceInPageAlloc(allocIndex, size, 1, expectedNumOfTables);
		expectedVA = (uint32)ptr_allocations[3] + 1*Mega + 4*kilo; //1MB.4KB after the Start Address of 4MB Hole
		if ((uint32) ptr_allocations[allocIndex] != (expectedVA)) { correct = 0; cprintf_colored(TEXT_TESTERR_CLR,"%d.3 Wrong start address for the allocated space... Expected = %x, Actual = %x\n", allocIndex, expectedVA, ptr_allocations[allocIndex]); }

		//1.5 MB [WORST FIT in 3MB Hole (alloc#1)]
		allocIndex = 17;
		size = 1*Mega + Mega/2;
		expectedNumOfTables = 0;
		correct = allocSpaceInPageAlloc(allocIndex, size, 1, expectedNumOfTables);
		expectedVA = (uint32)ptr_allocations[1] ; //Address of 3MB Hole
		if ((uint32) ptr_allocations[allocIndex] != (expectedVA)) { correct = 0; cprintf_colored(TEXT_TESTERR_CLR,"%d.3 Wrong start address for the allocated space... Expected = %x, Actual = %x\n", allocIndex, expectedVA, ptr_allocations[allocIndex]); }

		//2.5 MB [EXTEND THE BREAK]
		allocIndex = 18;
		size = 2*Mega + Mega/2;
		expectedNumOfTables = 0;
		correct = allocSpaceInPageAlloc(allocIndex, size, 1, expectedNumOfTables);
		expectedVA = expectedBreak ;
		expectedBreak += ROUNDUP(size, PAGE_SIZE);
		if ((uint32) ptr_allocations[allocIndex] != (expectedVA))
		{ correct = 0; cprintf_colored(TEXT_TESTERR_CLR,"%d.3 Wrong start address for the allocated space... Expected = %x, Actual = %x\n", allocIndex, expectedVA, ptr_allocations[allocIndex]); }
		if(uheapPageAllocBreak != expectedBreak)
		{correct = 0; cprintf_colored(TEXT_TESTERR_CLR,"BREAK value is not correct! Expected = %x, Actual = %x\n", expectedBreak, uheapPageAllocBreak);}

		//Insufficient space
		allocIndex = 19;
		expectedVA = 0;
		int freeFrames = (int)sys_calculate_free_frames() ;
		int usedDiskFrames = (int)sys_pf_calculate_allocated_pages() ;
		uint32 restOfUHeap = (USER_HEAP_MAX - ACTUAL_PAGE_ALLOC_START) - expectedBreak ;
		ptr_allocations[allocIndex] = malloc(restOfUHeap+1);
		if (ptr_allocations[allocIndex] != NULL)
		{ correct = 0; cprintf_colored(TEXT_TESTERR_CLR,"%d.1 Allocating insufficient space: should return NULL\n", allocIndex); }
		if (((int)sys_pf_calculate_allocated_pages() - usedDiskFrames) != 0)
		{ correct = 0; cprintf_colored(TEXT_TESTERR_CLR,"%d.2 Page file is changed while it's not expected to. (pages are wrongly allocated/de-allocated in PageFile)\n", allocIndex); }
		if ((freeFrames - (int)sys_calculate_free_frames()) != 0)
		{ correct = 0; cprintf_colored(TEXT_TESTERR_CLR,"%d.3 Wrong allocation: pages are not loaded successfully into memory\n", allocIndex); }
	}
	if (correct) eval+=30;
	correct = 1;

	//6. Check content of newly allocated spaces
	cprintf_colored(TEXT_cyan,"%~\n6. Check content of newly allocated spaces [10%]\n");
	{
		for (int i = 14; i < allocIndex; ++i)
		{
			char* ptr = (char*)ptr_allocations[i];
			sums[i] += ptr[0] ;
			sums[i] += ptr[lastIndices[i]] ;
			if (sums[i] != (maxByte + maxByte))
			{ correct = 0; cprintf_colored(TEXT_TESTERR_CLR,"invalid content in alloc#%d. Expected = %d, Actual = %d\n", i, 2*maxByte, sums[i]); }
		}
	}
	if (correct) eval += 10;
	correct = 1;

	//7. Free some allocations to create MERGED holes
	correct = 1;
	cprintf_colored(TEXT_cyan,"%~\n7. Free some allocations to create MERGED holes [5%]\n");
	{
		//Free new 3MB allocation inside the 4MB Hole
		correct = freeSpaceInPageAlloc(16,1);

		//Free new 1MB allocation at the beginning of the 4MB Hole (should be MERGED with next 3MB) => 4MB HOLE
		correct = freeSpaceInPageAlloc(15,1);

		//Free new 1MB allocation at the beginning of the 3MB Hole (should be MERGED with next 1.5MB) => 3MB HOLE
		correct = freeSpaceInPageAlloc(17,1);

		//Free new 1MB allocation at the 1MB Hole (NO MERGED)
		correct = freeSpaceInPageAlloc(14,1);

		//Free original 3rd 1MB allocation (should be MERGED with next 2MB hole and the prev 1MB hole) => 4MB HOLE
		correct = freeSpaceInPageAlloc(6,1);

		//Free original last 2MB allocation (should be MERGED with the prev 4MB created hole) => 6MB HOLE
		correct = freeSpaceInPageAlloc(8,1);
	}
	if (correct) eval += 5;
	correct = 1;

	//8. Allocate after kfree [Test CUSTOM FIT in MERGED FREE SPACES]
	cprintf_colored(TEXT_cyan,"%~\n8. Allocate after free [Test CUSTOM FIT in MERGED FREE SPACES] [40%]\n");
	{
		//3 MB [EXACT FIT in 3MB Hole]
		allocIndex = 20;
		size = 3*Mega - kilo;
		expectedNumOfTables = 0;
		correct = allocSpaceInPageAlloc(allocIndex, size, 1, expectedNumOfTables);
		expectedVA = (uint32)ptr_allocations[1] ; //Address of 3MB Hole
		if ((uint32) ptr_allocations[allocIndex] != (expectedVA)) { correct = 0; cprintf_colored(TEXT_TESTERR_CLR,"%d.3 Wrong start address for the allocated space... Expected = %x, Actual = %x\n", allocIndex, expectedVA, ptr_allocations[allocIndex]); }

		//3 MB [WORST FIT in 6MB Hole]
		allocIndex = 21;
		size = 3*Mega - kilo;
		expectedNumOfTables = 0;
		correct = allocSpaceInPageAlloc(allocIndex, size, 1, expectedNumOfTables);
		expectedVA = (uint32)ptr_allocations[5] ; //Address of 6MB Hole
		if ((uint32) ptr_allocations[allocIndex] != (expectedVA)) { correct = 0; cprintf_colored(TEXT_TESTERR_CLR,"%d.3 Wrong start address for the allocated space... Expected = %x, Actual = %x\n", allocIndex, expectedVA, ptr_allocations[allocIndex]); }

		//3MB - 4KB [WORST FIT in 4MB Hole]
		allocIndex = 22;
		size = 3*Mega - 4*kilo;
		expectedNumOfTables = 0;
		correct = allocSpaceInPageAlloc(allocIndex, size, 1, expectedNumOfTables);
		expectedVA = (uint32)ptr_allocations[3] ; //Address of 4MB Hole
		if ((uint32) ptr_allocations[allocIndex] != (expectedVA)) { correct = 0; cprintf_colored(TEXT_TESTERR_CLR,"%d.3 Wrong start address for the allocated space... Expected = %x, Actual = %x\n", allocIndex, expectedVA, ptr_allocations[allocIndex]); }

		//3 MB [EXACT FIT in remaining of 6MB Hole]
		allocIndex = 23;
		size = 3*Mega;
		expectedNumOfTables = 0;
		correct = allocSpaceInPageAlloc(allocIndex, size, 1, expectedNumOfTables);
		expectedVA = (uint32)ptr_allocations[5] + 3*Mega ; //3MB after the start address of 6MB Hole
		if ((uint32) ptr_allocations[allocIndex] != (expectedVA)) { correct = 0; cprintf_colored(TEXT_TESTERR_CLR,"%d.3 Wrong start address for the allocated space... Expected = %x, Actual = %x\n", allocIndex, expectedVA, ptr_allocations[allocIndex]); }
	}
	if (correct) eval += 40;
	correct = 1;


	cprintf_colored(TEXT_light_green, "%~\ntest CUSTOM FIT (1) [PAGE ALLOCATOR] completed. Eval = %d\n\n", eval);

	return;
#endif
}
