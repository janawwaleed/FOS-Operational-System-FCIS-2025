// Test the free of shared variables
#include <inc/lib.h>

void
_main(void)
{
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

	uint32 pagealloc_start = USER_HEAP_START + DYN_ALLOC_MAX_SIZE + PAGE_SIZE; //UHS + 32MB + 4KB
	uint32 *x, *y, *z ;
	int freeFrames, diff, expected;

	cprintf("************************************************\n");
	cprintf("MAKE SURE to have a FRESH RUN for this test\n(i.e. don't run any program/test before it)\n");
	cprintf("************************************************\n\n\n");

	int envID = sys_getenvid();

	int32 envIdSlave1, envIdSlave2, envIdSlaveB1, envIdSlaveB2;

	cprintf("STEP A: checking free of shared object using 2 environments... \n");
	{
		uint32 *x;
		envIdSlave1 = sys_create_env("ef_tshr5slave", (myEnv->page_WS_max_size),(myEnv->SecondListSize), 50);
		envIdSlave2 = sys_create_env("ef_tshr5slave", (myEnv->page_WS_max_size),(myEnv->SecondListSize), 50);

		int freeFrames = sys_calculate_free_frames() ;
		x = smalloc("x", PAGE_SIZE, 1);
		cprintf("Master env created x (1 page) \n");
		if (x != (uint32*)pagealloc_start) panic("Returned address is not correct. check the setting of it and/or the updating of the shared_mem_free_address");
		expected = 1+1 ; /*1page +1table*/
		diff = (freeFrames - sys_calculate_free_frames());
		if (diff < expected || diff > expected +1+1 /*extra 1 page & 1 table for sbrk (at max)*/)
			panic("Wrong allocation (current=%d, expected=%d): make sure that you allocate the required space in the user environment and add its frames to frames_storage", freeFrames - sys_calculate_free_frames(), expected);

		//to check that the slave environments completed successfully
		rsttst();

		sys_run_env(envIdSlave1);
		sys_run_env(envIdSlave2);

		cprintf("please be patient ...\n");
		env_sleep(3000);

		//to ensure that the slave environments completed successfully
		while (gettst()!=2) ;// panic("test failed");

		freeFrames = sys_calculate_free_frames() ;
		sfree(x);
		cprintf("Master env removed x (1 page) \n");
		int diff2 = (sys_calculate_free_frames() - freeFrames);
		expected = 1+1; /*1page+1table*/
		if (diff2 != expected) panic("Wrong free (diff=%d, expected=%d): revise your freeSharedObject logic\n", diff2, expected);
	}
	cprintf("Step A is finished!!\n\n\n");

	cprintf("STEP B: checking free of 2 shared objects ... \n");
	{
		uint32 *x, *z ;
		envIdSlaveB1 = sys_create_env("ef_tshr5slaveB1", (myEnv->page_WS_max_size),(myEnv->SecondListSize), 50);
		envIdSlaveB2 = sys_create_env("ef_tshr5slaveB2", (myEnv->page_WS_max_size), (myEnv->SecondListSize),50);

		z = smalloc("z", PAGE_SIZE+1, 1);
		cprintf("Master env created z (2 pages) \n");

		x = smalloc("x", PAGE_SIZE+1024, 1);
		cprintf("Master env created x (2 pages) \n");

		rsttst();

		sys_run_env(envIdSlaveB1);
		sys_run_env(envIdSlaveB2);

		//give slaves time to catch the shared object before removal
		{
			//			env_sleep(4000);
			while (gettst()!=2) ;
		}

		int freeFrames = sys_calculate_free_frames() ;

		sfree(z);
		cprintf("Master env removed z\n");

		sfree(x);
		cprintf("Master env removed x\n");

		inctst(); //finish the free's

		int diff = (sys_calculate_free_frames() - freeFrames);
		expected = 1 /*table*/;
		if (diff !=  expected) panic("Wrong free: frames removed not equal 1 !, correct frames to be removed are 1:\nfrom the env: 1 table\nframes_storage of z & x: should NOT cleared yet (still in use!)\n");

		inctst();	// finish checking

		//to ensure that the other environments completed successfully
		while (gettst()!=6) ;// panic("test failed");

		int* finish_children = smalloc("finish_children", sizeof(int), 1);
		*finish_children = 0;

		//To indicate that it create the finish_children & completed successfully
		cprintf("Master is completed.\n");
		inctst();

		if (sys_getparentenvid() > 0) {
			while(*finish_children != 1);
			cprintf("done\n");

			//DISABLE the interrupt to ensure the env_free is done as a whole without preemption
			//to avoid context switch (due to clock interrupt) while freeing the env to prevent:
			//	1. context switching to a wrong process specially in the part of temporarily switching the CPU process for freeing shared variables
			//	2. changing the # free frames
			char changeIntCmd[100] = "__changeInterruptStatus__";
			sys_utilities(changeIntCmd, 0);
			{
				sys_destroy_env(envIdSlave1);
				sys_destroy_env(envIdSlave2);
				sys_destroy_env(envIdSlaveB1);
				sys_destroy_env(envIdSlaveB2);
			}
			sys_utilities(changeIntCmd, 1);

			int *finishedCount = NULL;
			finishedCount = sget(sys_getparentenvid(), "finishedCount") ;

			//Critical section to protect the shared variable
			sys_lock_cons();
			{
				(*finishedCount)++ ;
			}
			sys_unlock_cons();
		}
	}


	return;
}
