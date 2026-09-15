// Test the use of semaphores to allow multiprograms to enter the CS at same time
// Slave program: enter the shop, leave it and signal the master program

#include <inc/lib.h>
extern volatile bool printStats;

void
_main(void)
{
	printStats = 0;
	int id = sys_getenvindex();

	int32 parentenvID = sys_getparentenvid();
	cprintf_colored(TEXT_light_blue, "Cust %d: outside the shop\n", id);

	//wait_semaphore(shopCapacitySem);
	char waitCmd[64] = "__KSem@0@Wait";
	sys_utilities(waitCmd, 0);
	{
		cprintf_colored(TEXT_light_cyan,"Cust %d: inside the shop\n", id) ;
		env_sleep(1000) ;
	}
	char signalCmd1[64] = "__KSem@0@Signal";
	sys_utilities(signalCmd1, 0);
	//signal_semaphore(shopCapacitySem);

	cprintf_colored(TEXT_light_blue, "Cust %d: exit the shop\n", id);

	char signalCmd2[64] = "__KSem@1@Signal";
	sys_utilities(signalCmd2, 0);
	//signal_semaphore(dependSem);

	cprintf_colored(TEXT_light_magenta, ">>> Cust %d is Finished\n", id);
	printStats = 0;

	return;
}
