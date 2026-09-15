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
#define EXPECTED_REF_CNT 28
uint32 expectedRefStream[EXPECTED_REF_CNT] = {
		0x80d000, 0x800000, 0x80e000, 0xeebfd000, 0x80f000, 0x803000, 0x801000, 0x804000, 0x805000,
		0x806000, 0x807000, 0x808000, 0x800000, 0x803000, 0x801000, 0xeebfd000, 0x804000, 0x809000,
		0x80a000, 0x80b000, 0x80c000, 0x827000, 0x802000, 0x800000, 0x803000, 0xeebfd000, 0x801000,
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
		//*ptr = *ptr2 ;
		/*==========================================================================*/
		//always use pages at 0x801000 and 0x804000
		garbage4 = *__ptr__ ;
		garbage5 = *__ptr2__ ;
	}

	//===================

	cprintf_colored(TEXT_cyan, "%~\nChecking INITIAL WS... \n");
	{
		found = sys_check_WS_list(expectedInitialVAs, 11, 0x800000, 1);
		if (found != 1) panic("OPTIMAL alg. failed.. the initial working set is changed while it's not expected to");
	}
	cprintf_colored(TEXT_cyan, "%~\nChecking EXPECTED REFERENCE STREAM... \n");
	{
		char separator[2] = "@";
		char checkRefStreamCmd[100] = "__CheckRefStream@";
		char token[20] ;
		char cmdWithCnt[100] ;
		ltostr(EXPECTED_REF_CNT, token);
		strcconcat(checkRefStreamCmd, token, cmdWithCnt);
		strcconcat(cmdWithCnt, separator, cmdWithCnt);
		ltostr((uint32)&expectedRefStream, token);
		strcconcat(cmdWithCnt, token, cmdWithCnt);
		strcconcat(cmdWithCnt, separator, cmdWithCnt);

		atomic_cprintf("%~Ref Command = %s\n", cmdWithCnt);

		sys_utilities(cmdWithCnt, (uint32)&found);

		if (found != 1) panic("OPTIMAL alg. failed.. unexpected page reference stream!");
	}
	cprintf_colored(TEXT_cyan, "%~\nChecking Allocation in Mem & Page File... \n");
	{
		if( (sys_pf_calculate_allocated_pages() - usedDiskPages) !=  0) panic("Unexpected extra/less pages have been added to page file.. NOT Expected to add new pages to the page file");

		int freePagesAfter = (sys_calculate_free_frames() + sys_calculate_modified_frames());
		int expectedNumOfFrames = 7;
		if( (freePages - freePagesAfter) != expectedNumOfFrames)
			panic("Unexpected number of allocated frames in RAM. Expected = %d, Actual = %d", expectedNumOfFrames, (freePages - freePagesAfter));
	}

	cprintf_colored(TEXT_light_green, "%~\nCongratulations!! test PAGE replacement #1 [OPTIMAL Alg.] is completed successfully.\n");
	return;
}
