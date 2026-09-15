// Test the use of semaphores for critical section & dependency
// Slave program: enter critical section, print it's ID, exit and signal the master program
#include <inc/lib.h>
extern volatile bool printStats ;

void
_main(void)
{
	int32 parentenvID = sys_getparentenvid();
	int id = sys_getenvindex();

	cprintf_colored(TEXT_light_blue, "%d: before the critical section\n", id);
	//wait_semaphore(cs1);
	char waitCmd[64] = "__KSem@0@Wait";
	sys_utilities(waitCmd, 0);
	{
		cprintf_colored(TEXT_light_cyan, "%d: inside the critical section\n", id) ;
		cprintf_colored(TEXT_light_cyan, "my ID is %d\n", id);
		int sem1val ;
		char getCmd[64] = "__KSem@0@Get";
		sys_utilities(getCmd, (uint32)(&sem1val));
		if (sem1val > 0)
			panic("Error: more than 1 process inside the CS... please review your semaphore code again...");
		env_sleep(RAND(1000, 5000)) ;
		cprintf_colored(TEXT_light_blue, "%d: leaving the critical section...\n", id);
	}
	char signalCmd1[64] = "__KSem@0@Signal";
	sys_utilities(signalCmd1, 0);
	//signal_semaphore(cs1);
	cprintf_colored(TEXT_light_blue, "%d: after the critical section\n", id);

	//signal_semaphore(depend1);
	char signalCmd2[64] = "__KSem@1@Signal";
	sys_utilities(signalCmd2, 0);

	cprintf_colored(TEXT_light_magenta, ">>> Slave %d is Finished\n", id);
	printStats = 0;

	return;
}
