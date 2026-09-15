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
uint32 expectedVAs1[11] = {
		0x806000,0x808000, 0xeebfd000,
		0x827000, 0x800000, 0x801000, 0x802000,	0x803000,
		0x80d000,0x80e000,0x80f000,
} ;
uint32 expectedVAs2[11] = {
		0x802000,
		0xeebfd000,
		0x803000, 0x809000, 0x80a000, 0x800000, 0x801000, 0x804000, 0x80b000,0x80c000,
		0x827000
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

	int freePages = sys_calculate_free_frames();
	int usedDiskPages = sys_pf_calculate_allocated_pages();

	uint32 va;
	char invPageCmd[20] = "__InvPage__";

	//Remove some pages from the WS
	cprintf_colored(TEXT_cyan, "%~\nRemove some pages from the WS... \n");
	{
		va = 0x805000;
		sys_utilities(invPageCmd, va);
		va = 0x807000;
		sys_utilities(invPageCmd, va);
		va = 0x809000;
		sys_utilities(invPageCmd, va);
	}
	//Writing (Modified)
	__arr__[PAGE_SIZE*10-1] = 'a' ;

	//Reading (Not Modified)
	char garbage1 = __arr__[PAGE_SIZE*11-1] ;
	char garbage2 = __arr__[PAGE_SIZE*12-1] ;
	char garbage4,garbage5;

	//Checking the WS after the 3 faults
	cprintf_colored(TEXT_cyan, "%~\nChecking PAGE CLOCK algorithm after freeing some pages [PLACEMENT]... \n");
	{
		found = sys_check_WS_list(expectedVAs1, 11, 0x806000, 1);
		if (found != 1) panic("CLOCK alg. failed.. trace it by printing WS before and after page fault");
	}

	//Remove some pages from the WS
	cprintf_colored(TEXT_cyan, "%~\nRemove some other pages from the WS including last WS element... \n");
	{
		va = 0x827000;
		sys_utilities(invPageCmd, va);
		va = 0x80F000;
		sys_utilities(invPageCmd, va);
		va = 0x806000;
		sys_utilities(invPageCmd, va);
		va = 0x808000;
		sys_utilities(invPageCmd, va);
		va = 0x800000;
		sys_utilities(invPageCmd, va);
		va = 0x801000;
		sys_utilities(invPageCmd, va);
	}
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

	//Checking the WS after the 10 faults
	cprintf_colored(TEXT_cyan, "%~\nChecking PAGE CLOCK algorithm after freeing other set of pages [REPLACEMENT]... \n");
	{
		found = sys_check_WS_list(expectedVAs2, 11, 0xeebfd000, 1);
		if (found != 1) panic("CLOCK alg. failed.. trace it by printing WS before and after page fault");
	}

	cprintf_colored(TEXT_cyan, "%~\nChecking Allocation in Mem & Page File... \n");
	{
		if( (sys_pf_calculate_allocated_pages() - usedDiskPages) !=  0) panic("Unexpected extra/less pages have been added to page file.. NOT Expected to add new pages to the page file");

		int freePagesAfter = (sys_calculate_free_frames() + sys_calculate_modified_frames());
		if( (freePages - freePagesAfter) != 0 )
			panic("Extra memory are wrongly allocated... It's REplacement: expected that no extra frames are allocated. Expected = %d, Actual = %d", 0, (freePages - freePagesAfter));
	}

	cprintf_colored(TEXT_light_green, "%~\nCongratulations!! test PAGE replacement [CLOCK Alg. #2] is completed successfully.\n");
	return;
}
