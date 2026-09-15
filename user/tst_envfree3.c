// Scenario that tests the usage of shared variables
#include <inc/lib.h>

void _main(void)
{
	// Testing scenario 3: using dynamic allocation and free. Kill itself!
	// Testing removing the allocated pages (static & dynamic) in mem, WS, mapped page tables, env's directory and env's page file

	char getksbrkCmd[100] = "__getKernelSBreak__";
	uint32 ksbrk_before ;
	sys_utilities(getksbrkCmd, (uint32)&ksbrk_before);

	int freeFrames_before = sys_calculate_free_frames() ;
	int usedDiskPages_before = sys_pf_calculate_allocated_pages() ;
	cprintf("\n---# of free frames before running programs = %d\n", freeFrames_before);

	int32 envIdProcess = sys_create_env("tef3_slave", myEnv->page_WS_max_size,(myEnv->SecondListSize), 50);

	sys_run_env(envIdProcess);

	char getProcStateCmd[100] = "__getProcState@";
	int procState = 0;
	do
	{
		char id[20] ;
		ltostr(envIdProcess, id);
		char getProcStateWithIDCmd[100] ;
		strcconcat(getProcStateCmd, id, getProcStateWithIDCmd);

		sys_utilities(getProcStateWithIDCmd, (uint32)&procState) ;
		//cprintf("status of env %d = %d\n", envIdProcess, procState);
	}
	while (procState != E_BAD_ENV) ;

	uint32 ksbrk_after ;
	sys_utilities(getksbrkCmd, (uint32)&ksbrk_after);

	//Checking the number of frames after killing the created environments
	int freeFrames_after = sys_calculate_free_frames() ;
	int usedDiskPages_after = sys_pf_calculate_allocated_pages() ;

	int expected = (ROUNDUP((uint32)ksbrk_after, PAGE_SIZE) - ROUNDUP((uint32)ksbrk_before, PAGE_SIZE)) / PAGE_SIZE;
	cprintf("expected = %d\n",expected);
	if ((freeFrames_before - freeFrames_after) != expected) {
		cprintf("\n---# of free frames after closing running programs not as before running = %d\ndifference = %d, expected = %d\n",
				freeFrames_after, freeFrames_after - freeFrames_before, expected);
		panic("env_free() does not work correctly... check it again.");
	}

	cprintf("\n---# of free frames after closing running programs returned back as expected = %d + kbreak pages (%d)\n", freeFrames_after, expected);

	cprintf("\n\nCongratulations!! test scenario 3 for envfree completed successfully.\n");
	return;
}
