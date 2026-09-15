// Test the use of semaphores to allow multiprograms to enter the CS at same time
// Master program: take user input, create the semaphores, run slaves and wait them to finish
#include <inc/lib.h>

void
_main(void)
{
	int envID = sys_getenvid();
	char line[256] ;
	readline("Enter total number of customers: ", line) ;
	int totalNumOfCusts = strtol(line, NULL, 10);
	readline("Enter shop capacity: ", line) ;
	int shopCapacity = strtol(line, NULL, 10);

	int semVal ;
	//Initialize the kernel semaphores
	char initCmd1[64] = "__KSem@0@Init";
	char initCmd2[64] = "__KSem@1@Init";
	semVal = shopCapacity;
	sys_utilities(initCmd1, (uint32)(&semVal));
	semVal = 0;
	sys_utilities(initCmd2, (uint32)(&semVal));

	int i = 0 ;
	int id ;
	for (; i<totalNumOfCusts; i++)
	{
		id = sys_create_env("ksem2Slave", (myEnv->page_WS_max_size), (myEnv->SecondListSize),(myEnv->percentage_of_WS_pages_to_be_removed));
		if (id == E_ENV_CREATION_ERROR)
			panic("NO AVAILABLE ENVs... Please reduce the number of customers and try again...");
		sys_run_env(id) ;
	}

	//Wait until all finished
	for (i = 0 ; i<totalNumOfCusts; i++)
	{
		char waitCmd[64] = "__KSem@1@Wait";
		sys_utilities(waitCmd, 0);
	}

	//Check semaphore values
	int sem1val ;
	int sem2val ;
	char getCmd1[64] = "__KSem@0@Get";
	char getCmd2[64] = "__KSem@1@Get";

	sys_utilities(getCmd1, (uint32)(&sem1val));
	sys_utilities(getCmd2, (uint32)(&sem2val));

	//wait a while to allow the slaves to finish printing their closing messages
	env_sleep(10000);
	if (sem2val == 0 && sem1val == shopCapacity)
		cprintf_colored(TEXT_light_green,"\nCongratulations!! Test of Semaphores [2] completed successfully!!\n\n\n");
	else
		cprintf_colored(TEXT_TESTERR_CLR,"\nError: wrong semaphore value... please review your semaphore code again...\n");

	return;
}
