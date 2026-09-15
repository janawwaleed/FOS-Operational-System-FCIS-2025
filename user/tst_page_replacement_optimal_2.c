/* *********************************************************** */
/* MAKE SURE PAGE_WS_MAX_SIZE = 3000 */
/* *********************************************************** */

#include <inc/lib.h>

char* __ptr__ = (char* )0x0801000 ;
char* __ptr2__ = (char* )0x0804000 ;
#define EXPECTED_REF_CNT 12
uint32 expectedRefStream[EXPECTED_REF_CNT] = {
		0xeebf1000, 0xeebfb000, 0xeebfc000, 0xeebf2000, 0xeebf3000, 0xeebf4000,
		0xeebf5000, 0xeebf6000, 0xeebf7000, 0xeebf8000, 0xeebf9000, 0xeebfa000
} ;
void _main(void)
{
	char __arr__[PAGE_SIZE*12];

	//("STEP 0: checking Initial WS entries ...\n");
	bool found ;

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

	cprintf_colored(TEXT_cyan, "%~\nChecking Content... \n");
	{
		if (garbage4 != *__ptr__) panic("test failed!");
		if (garbage5 != *__ptr2__) panic("test failed!");
		if(__arr__[PAGE_SIZE*10-1] != 'a') panic("test failed!");
		for (i = 0 ; i < PAGE_SIZE*10 ; i+=PAGE_SIZE/2)
		{
			if(__arr__[i] != -1) panic("test failed!");
		}
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

		//if (found != 1) panic("OPTIMAL alg. failed.. unexpected page reference stream!");
	}
	cprintf_colored(TEXT_cyan, "%~\nChecking Allocation in Mem & Page File... \n");
	{
		if( (sys_pf_calculate_allocated_pages() - usedDiskPages) !=  0) panic("Unexpected extra/less pages have been added to page file.. NOT Expected to add new pages to the page file");

		int freePagesAfter = (sys_calculate_free_frames() + sys_calculate_modified_frames());
		int expectedNumOfFrames = 11;
		if( (freePages - freePagesAfter) != expectedNumOfFrames)
			panic("Unexpected number of allocated frames in RAM. Expected = %d, Actual = %d", expectedNumOfFrames, (freePages - freePagesAfter));
	}

	cprintf_colored(TEXT_light_green, "%~\nCongratulations!! test PAGE replacement #2 [OPTIMAL Alg.] is completed successfully.\n");
	return;
}
