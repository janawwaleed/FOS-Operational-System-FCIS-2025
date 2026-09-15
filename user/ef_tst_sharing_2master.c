// Test the creation of shared variables and using them
// Master program: create the shared variables, initialize them and run slaves
#include <inc/lib.h>

void
_main(void)
{
//	//Initial test to ensure it works on "PLACEMENT" not "REPLACEMENT"
//#if USE_KHEAP
//	{
//		if (LIST_SIZE(&(myEnv->page_WS_list)) >= myEnv->page_WS_max_size)
//			panic("Please increase the WS size");
//	}
//#else
//	panic("make sure to enable the kernel heap: USE_KHEAP=1");
//#endif
//	/*=================================================*/

	uint32 pagealloc_start = USER_HEAP_START + DYN_ALLOC_MAX_SIZE + PAGE_SIZE; //UHS + 32MB + 4KB
	uint32 *x, *y, *z ;
	int diff, expected;

	//x: Readonly
	int freeFrames = sys_calculate_free_frames() ;
	x = smalloc("x", 4, 0);
	if (x != (uint32*)pagealloc_start) {panic("Create(): Returned address is not correct. make sure that you align the allocation on 4KB boundary");}
	expected = 1+1 ; /*1page +1table*/
	diff = (freeFrames - sys_calculate_free_frames());
	if (diff < expected || diff > expected +1+1 /*extra 1 page & 1 table for sbrk (at max)*/) {panic("Wrong allocation (current=%d, expected=%d): make sure that you allocate the required space in the user environment and add its frames to frames_storage", freeFrames - sys_calculate_free_frames(), expected);}

	//y: Readonly
	freeFrames = sys_calculate_free_frames() ;
	y = smalloc("y", 4, 0);
	if (y != (uint32*)(pagealloc_start + 1 * PAGE_SIZE)) {panic("Create(): Returned address is not correct. make sure that you align the allocation on 4KB boundary");}
	expected = 1 ; /*1page*/
	diff = (freeFrames - sys_calculate_free_frames());
	if (diff < expected || diff > expected +1+1 /*extra 1 page & 1 table for sbrk (at max)*/) {panic("Wrong allocation (current=%d, expected=%d): make sure that you allocate the required space in the user environment and add its frames to frames_storage", freeFrames - sys_calculate_free_frames(), expected);}

	//z: Writable
	freeFrames = sys_calculate_free_frames() ;
	z = smalloc("z", 4, 1);
	if (z != (uint32*)(pagealloc_start + 2 * PAGE_SIZE)) {panic("Create(): Returned address is not correct. make sure that you align the allocation on 4KB boundary");}
	expected = 1 ; /*1page*/
	diff = (freeFrames - sys_calculate_free_frames());
	if (diff < expected || diff > expected +1+1 /*extra 1 page & 1 table for sbrk (at max)*/) {panic("Wrong allocation (current=%d, expected=%d): make sure that you allocate the required space in the user environment and add its frames to frames_storage", freeFrames - sys_calculate_free_frames(), expected);}

	*x = 10 ;
	*y = 20 ;

	int id1, id2, id3;
	id1 = sys_create_env("ef_shr2Slave1", (myEnv->page_WS_max_size),(myEnv->SecondListSize), 50);
	id2 = sys_create_env("ef_shr2Slave1", (myEnv->page_WS_max_size),(myEnv->SecondListSize), 50);
	id3 = sys_create_env("ef_shr2Slave1", (myEnv->page_WS_max_size),(myEnv->SecondListSize), 50);

	//to check that the slave environments completed successfully
	rsttst();

	int* finish_children = smalloc("finish_children", sizeof(int), 1);

	sys_run_env(id1);
	sys_run_env(id2);
	sys_run_env(id3);

	env_sleep(15000) ;

	//to ensure that the slave environments completed successfully
	while (gettst()!=3) ; //panic("test failed");


	if (*z != 30)
		panic("Error!! Please check the creation (or the getting) of shared 2variables!!\n\n\n");
	else
		cprintf("test sharing 2 [Create & Get] is finished. Now, it'll destroy its children...\n\n");


	if (sys_getparentenvid() > 0) {
		//DISABLE the interrupt to ensure the env_free is done as a whole without preemption
		//to avoid context switch (due to clock interrupt) while freeing the env to prevent:
		//	1. context switching to a wrong process specially in the part of temporarily switching the CPU process for freeing shared variables
		//	2. changing the # free frames

		char changeIntCmd[100] = "__changeInterruptStatus__";
		sys_utilities(changeIntCmd, 0);
		{
			sys_destroy_env(id1);
			cprintf("[1] *****************************>>>>>>>>>>>>>>>>>>>>>\n");
			sys_destroy_env(id2);
			cprintf("[2] *****************************>>>>>>>>>>>>>>>>>>>>>\n");
			sys_destroy_env(id3);
			cprintf("[3] *****************************>>>>>>>>>>>>>>>>>>>>>\n");
		}
		sys_utilities(changeIntCmd, 1);

		int *finishedCount = NULL;
		finishedCount = sget(sys_getparentenvid(), "finishedCount") ;
		sys_lock_cons();
		{
			(*finishedCount)++ ;
		}
		sys_unlock_cons();
	}
	return;
}


