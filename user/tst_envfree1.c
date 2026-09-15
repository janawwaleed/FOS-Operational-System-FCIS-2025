// Scenario that tests environment free run tef1 5 3
#include <inc/lib.h>

void _main(void)
{
	rsttst();
	// Testing scenario 1: without using dynamic allocation/de-allocation, shared variables and semaphores
	// Testing removing the allocated pages in mem, WS, mapped page tables, env's directory and env's page file

	char getksbrkCmd[100] = "__getKernelSBreak__";
	uint32 ksbrk_before ;
	sys_utilities(getksbrkCmd, (uint32)&ksbrk_before);

	int freeFrames_before = sys_calculate_free_frames() ;
	int usedDiskPages_before = sys_pf_calculate_allocated_pages() ;
	cprintf("\n---# of free frames before running programs = %d\n", freeFrames_before);

	/*[4] CREATE AND RUN ProcessA & ProcessB*/
	//Create 3 processes

	int32 envIdProcessA = sys_create_env("sc_fib_recursive", (myEnv->page_WS_max_size),(myEnv->SecondListSize), 50);
	int32 envIdProcessB = sys_create_env("sc_fact_recursive", (myEnv->page_WS_max_size)*4,(myEnv->SecondListSize), 50);
	int32 envIdProcessC = sys_create_env("sc_fos_add",(myEnv->page_WS_max_size),(myEnv->SecondListSize), 50);

	//Run 3 processes
	sys_run_env(envIdProcessA);
	sys_run_env(envIdProcessB);
	sys_run_env(envIdProcessC);

	//env_sleep(6000);
	while (gettst() != 3) ;

	cprintf("\n---# of free frames after running programs = %d\n", sys_calculate_free_frames());

	uint32 ksbrk_after ;
	sys_utilities(getksbrkCmd, (uint32)&ksbrk_after);

	//Kill the 3 processes
	//DISABLE the interrupt to ensure the env_free is done as a whole without preemption
	//to avoid context switch (due to clock interrupt) while freeing the env to prevent:
	//	1. context switching to a wrong process specially in the part of temporarily switching the CPU process for freeing shared variables
	//	2. changing the # free frames
	char changeIntCmd[100] = "__changeInterruptStatus__";
	sys_utilities(changeIntCmd, 0);
	{
		sys_destroy_env(envIdProcessA);
		sys_destroy_env(envIdProcessB);
		sys_destroy_env(envIdProcessC);
	}
	sys_utilities(changeIntCmd, 1);

	//Checking the number of frames after killing the created environments
	int freeFrames_after = sys_calculate_free_frames() ;
	int usedDiskPages_after = sys_pf_calculate_allocated_pages() ;

	cprintf("\n---# of free frames after KILLING programs = %d\n", sys_calculate_free_frames());

	int expected = (ROUNDUP((uint32)ksbrk_after, PAGE_SIZE) - ROUNDUP((uint32)ksbrk_before, PAGE_SIZE)) / PAGE_SIZE;
	if ((freeFrames_before - freeFrames_after) != expected) {
		cprintf("\n---# of free frames after closing running programs not as before running = %d\ndifference = %d, expected = %d\n",
				freeFrames_after, freeFrames_after - freeFrames_before, expected);
		panic("env_free() does not work correctly... check it again.");
	}

	cprintf("\n---# of free frames after closing running programs returned back as expected\n");

	cprintf("\n\nCongratulations!! test scenario 1 for envfree completed successfully.\n");
	return;
}
