// Test the creation of shared variables and using them
// Slave program1: Read the 2 shared variables, edit the 3rd one, and exit
#include <inc/lib.h>
#include <user/tst_malloc_helpers.h>

extern volatile bool printStats;
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

	uint32 pagealloc_start = ACTUAL_PAGE_ALLOC_START; //UHS + 32MB + 4KB

	uint32 *x,*y,*z, *expectedVA;
	int diff, expected;
	int freeFrames, usedDiskPages ;
	int32 parentenvID = sys_getparentenvid();
	//GET: z then y then x, opposite to creation order (x then y then z)
	//So, addresses here will be different from the OWNER addresses
	sys_lock_cons();
	{
		freeFrames = sys_calculate_free_frames() ;
		usedDiskPages = sys_pf_calculate_allocated_pages();
		z = sget(parentenvID,"z");
		expectedVA = (uint32*)(pagealloc_start + 0 * PAGE_SIZE);
		if (z != expectedVA) panic("Get(): Returned address is not correct. Expected = %x, Actual = %x\nMake sure that you align the allocation on 4KB boundary", expectedVA, z);
		expected = 1 ; /*1 table in UH*/
		diff = (freeFrames - sys_calculate_free_frames());
		if (!inRange(diff, expected, expected + 2 /*UH Block Alloc: max of 1 page & 1 table*/))
			panic("Wrong allocation (current=%d, expected=[%d, %d]): make sure that you allocate the required space in the user environment and add its frames to frames_storage", freeFrames - sys_calculate_free_frames(), expected, expected +2);
		if( (sys_pf_calculate_allocated_pages() - usedDiskPages) !=  0)
		{panic("Wrong page file allocation: ");}
	}
	sys_unlock_cons();

	sys_lock_cons();
	{
		freeFrames = sys_calculate_free_frames() ;
		usedDiskPages = sys_pf_calculate_allocated_pages();
		y = sget(parentenvID,"y");
		expectedVA = (uint32*)(pagealloc_start + 1 * PAGE_SIZE);
		if (y != expectedVA) panic("Get(): Returned address is not correct. Expected = %x, Actual = %x\nMake sure that you align the allocation on 4KB boundary", expectedVA, y);
		expected = 0 ;
		diff = (freeFrames - sys_calculate_free_frames());
		if (!inRange(diff, expected, expected)) //no extra is expected since there'll be free blocks in Block Allo since last allocation
			panic("Wrong allocation (current=%d, expected=%d): make sure that you allocate the required space in the user environment and add its frames to frames_storage", freeFrames - sys_calculate_free_frames(), expected);
		if( (sys_pf_calculate_allocated_pages() - usedDiskPages) !=  0)
		{panic("Wrong page file allocation: ");}
	}
	sys_unlock_cons();

	if (*y != 20) panic("Get(): Shared Variable is not created or got correctly") ;

	sys_lock_cons();
	{
		freeFrames = sys_calculate_free_frames() ;
		usedDiskPages = sys_pf_calculate_allocated_pages();
		x = sget(parentenvID,"x");
		expectedVA = (uint32*)(pagealloc_start + 2 * PAGE_SIZE);
		if (x != expectedVA) panic("Get(): Returned address is not correct. Expected = %x, Actual = %x\nMake sure that you align the allocation on 4KB boundary", expectedVA, x);
		expected = 0 ;
		diff = (freeFrames - sys_calculate_free_frames());
		if (!inRange(diff, expected, expected)) //no extra is expected since there'll be free blocks in Block Allo since last allocation
			panic("Wrong allocation (current=%d, expected=%d): make sure that you allocate the required space in the user environment and add its frames to frames_storage", freeFrames - sys_calculate_free_frames(), expected);
		if( (sys_pf_calculate_allocated_pages() - usedDiskPages) !=  0)
		{panic("Wrong page file allocation: ");}
	}
	sys_unlock_cons();

	if (*x != 10) panic("Get(): Shared Variable is not created or got correctly") ;

	*z = *x + *y ;
	if (*z != 30) panic("Get(): Shared Variable is not created or got correctly") ;

	//To indicate that it's completed successfully
	inctst();

	cprintf_colored(TEXT_green, "Slave1 completed.\n");
	printStats = 0;
	return;
}
