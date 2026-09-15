// Test the use of semaphores for critical section & dependency
// Master program: create the semaphores, run slaves and wait them to finish
#include <inc/lib.h>

void
_main(void)
{
	int envID = sys_getenvid();
	int semVal ;
	//Initialize the kernel semaphores
	char initCmd1[64] = "__KSem@0@Init";
	char initCmd2[64] = "__KSem@1@Init";
	semVal = 1;
	sys_utilities(initCmd1, (uint32)(&semVal));
	semVal = 0;
	sys_utilities(initCmd2, (uint32)(&semVal));

	//Run Slave Processes
	int id1, id2, id3;
	id1 = sys_create_env("ksem1Slave", (myEnv->page_WS_max_size),(myEnv->SecondListSize), (myEnv->percentage_of_WS_pages_to_be_removed));
	id2 = sys_create_env("ksem1Slave", (myEnv->page_WS_max_size), (myEnv->SecondListSize),(myEnv->percentage_of_WS_pages_to_be_removed));
	id3 = sys_create_env("ksem1Slave", (myEnv->page_WS_max_size), (myEnv->SecondListSize),(myEnv->percentage_of_WS_pages_to_be_removed));

	sys_run_env(id1);
	sys_run_env(id2);
	sys_run_env(id3);

	//Wait until all finished
	char waitCmd1[64] = "__KSem@1@Wait";
	sys_utilities(waitCmd1, 0);
	//cprintf("after 1st wait\n");
	char waitCmd2[64] = "__KSem@1@Wait";
	sys_utilities(waitCmd2, 0);
	//cprintf("after 2nd wait\n");
	char waitCmd3[64] = "__KSem@1@Wait";
	sys_utilities(waitCmd3, 0);
	//cprintf("after 3rd wait\n");

	//Check Sem Values
	int sem1val ;
	int sem2val ;
	char getCmd1[64] = "__KSem@0@Get";
	char getCmd2[64] = "__KSem@1@Get";

	sys_utilities(getCmd1, (uint32)(&sem1val));
	sys_utilities(getCmd2, (uint32)(&sem2val));

	if (sem2val == 0 && sem1val == 1)
		cprintf_colored(TEXT_light_green, "Congratulations!! Test of Semaphores [1] completed successfully!!\n\n\n");
	else
		panic("Error: wrong semaphore value... please review your semaphore code again! Expected = %d, %d, Actual = %d, %d", 1, 0, sem1val, sem2val);

	return;
}
