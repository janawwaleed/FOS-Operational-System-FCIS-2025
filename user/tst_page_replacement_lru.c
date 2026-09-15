/* *********************************************************** */
/* MAKE SURE PAGE_WS_MAX_SIZE = 11 */
/* *********************************************************** */

#include <inc/lib.h>

char __arr__[PAGE_SIZE*12];
char* __ptr__ = (char* )0x0801000 ;
char* __ptr2__ = (char* )0x0804000 ;
uint32 expectedInitialVAs[11] = {
		0x800000, 0x801000, 0x802000,		//Code
		0x803000,0x804000,0x805000,0x806000,0x807000,0x808000,0x809000, 	//Data
		0xeebfd000, 	//Stack
} ;
uint32 expectedFinalVAs[11] = {
		0x809000, 0x800000, 0x80a000, 0x803000,0x801000,0x804000,0x80b000,0x80c000,0x827000,0x802000,	//Code & Data
		0xeebfd000, 	//Stack
} ;

void _main(void)
{

	//("STEP 0: checking Initial WS entries ...\n");
	bool found ;

#if USE_KHEAP
	{
		found = sys_check_WS_list(expectedInitialVAs, 11, 0x800000, 1);
		if (found != 1) panic("INITIAL PAGE WS entry checking failed! Review size of the WS!!\n*****IF CORRECT, CHECK THE ISSUE WITH THE STAFF*****");
	}
#else
	panic("make sure to enable the kernel heap: USE_KHEAP=1");
#endif

	//Writing (Modified)
	__arr__[PAGE_SIZE*10-1] = 'a' ;

	//Reading (Not Modified)
	char garbage1 = __arr__[PAGE_SIZE*11-1] ;
	char garbage2 = __arr__[PAGE_SIZE*12-1] ;
	char garbage4,garbage5;

	//Writing (Modified)
	int i ;
	for (i = 0 ; i < PAGE_SIZE*10 ; i+=PAGE_SIZE/2)
	{
		__arr__[i] = -1 ;
		/*2016: this BUGGY line is REMOVED el7! it overwrites the KERNEL CODE :( !!!*/
		//*__ptr__ = *__ptr2__ ;
		/*==========================================================================*/
		//always use pages at 0x801000 and 0x804000
		garbage4 = *__ptr__ ;
		garbage5 = *__ptr2__ ;
	}

	//===================


	cprintf_colored(TEXT_cyan, "%~\nChecking Content... \n");
	{
		if (garbage4 != *__ptr__) panic("test failed!");
		if (garbage5 != *__ptr2__) panic("test failed!");
	}
	cprintf_colored(TEXT_cyan, "%~\nChecking PAGE LRU algorithm... \n");
	{
		found = sys_check_WS_list(expectedFinalVAs, 11, 0, 0); //order is not important in LRU
		if (found != 1) panic("LRU alg. failed.. trace it by printing WS before and after page fault");
	}
	cprintf_colored(TEXT_cyan, "%~\nChecking Number of Disk Access... \n");
	{
		uint32 expectedNumOfFaults = 17;
		uint32 expectedNumOfWrites = 6;
		uint32 expectedNumOfReads = 17;
		if (myEnv->nPageIn != expectedNumOfReads || myEnv->nPageOut != expectedNumOfWrites || myEnv->pageFaultsCounter != expectedNumOfFaults)
			panic("LRU alg. failed.. unexpected number of disk access");
	}

	cprintf_colored(TEXT_light_green, "%~\nCongratulations!! test PAGE replacement [LRU Alg.] is completed successfully.\n");
	return;
}
