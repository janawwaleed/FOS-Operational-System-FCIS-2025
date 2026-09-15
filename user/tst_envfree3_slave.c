// Scenario that tests environment free run tef2 10 5
#include <inc/lib.h>

extern void destroy(void);

void _main(void)
{
	// Testing scenario 3: using dynamic allocation and free. Kill itself!
	// Testing removing the allocated pages (static & dynamic) in mem, WS, mapped page tables, env's directory and env's page file

	int freeFrames_before = sys_calculate_free_frames() ;
	int usedDiskPages_before = sys_pf_calculate_allocated_pages() ;
	cprintf("\n---# of free frames before running programs = %d\n", freeFrames_before);

	/*[4] CREATE AND RUN ProcessA & ProcessB*/
	//Create 3 processes
	int32 envIdProcessA = sys_create_env("sc_ms_leak_small", (myEnv->page_WS_max_size),(myEnv->SecondListSize), 50);
	int32 envIdProcessB = sys_create_env("sc_ms_noleak_small", (myEnv->page_WS_max_size),(myEnv->SecondListSize), 50);

	rsttst();

	//Run 2 processes
	sys_run_env(envIdProcessA);
	sys_run_env(envIdProcessB);

	//env_sleep(30000);

	//to ensure that the slave environments completed successfully
	while (gettst()!=2) ;// panic("test failed");

	cprintf("\n---# of free frames after running programs = %d\n", sys_calculate_free_frames());

	//Kill the 3 processes [including itself]
	//DISABLE the interrupt to ensure the env_free is done as a whole without preemption
	//to avoid context switch (due to clock interrupt) while freeing the env to prevent:
	//	1. context switching to a wrong process specially in the part of temporarily switching the CPU process for freeing shared variables
	//	2. changing the # free frames
	char changeIntCmd[100] = "__changeInterruptStatus__";
	sys_utilities(changeIntCmd, 0);
	{
		sys_destroy_env(envIdProcessA);
		sys_destroy_env(envIdProcessB);
		//KILL ITSELF
		destroy();
	}
	sys_utilities(changeIntCmd, 1);
}
