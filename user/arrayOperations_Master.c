// Scenario that tests the usage of shared variables
#include <inc/lib.h>
#include <user/arrayOperations.h>

void InitializeAscending(int *Elements, int NumOfElements);
void InitializeDescending(int *Elements, int NumOfElements);
void InitializeSemiRandom(int *Elements, int NumOfElements);
uint32 CheckSorted(int *Elements, int NumOfElements);
void ArrayStats(int *Elements, int NumOfElements, int64 *mean, int64 *var);

void
_main(void)
{
	/*[1] CREATE SEMAPHORES*/
#if USE_KERN_SEMAPHORE
	//Initialize the kernel semaphores
	int semVal ;
	char initCmd1[64] = "__KSem@0@Init";
	char initCmd2[64] = "__KSem@1@Init";
	char initCmd3[64] = "__KSem@2@Init";
	semVal = 0; sys_utilities(initCmd1, (uint32)(&semVal));
	semVal = 0; sys_utilities(initCmd2, (uint32)(&semVal));
	semVal = 1; sys_utilities(initCmd3, (uint32)(&semVal));
#else
	struct semaphore ready = create_semaphore("Ready", 0);
	struct semaphore finished = create_semaphore("Finished", 0);
	struct semaphore cons_mutex = create_semaphore("Console Mutex", 1);
#endif
	/*[2] RUN THE SLAVES PROGRAMS*/
	int numOfSlaveProgs = 3 ;

	int32 envIdQuickSort = sys_create_env("slave_qs", (myEnv->page_WS_max_size),(myEnv->SecondListSize) ,(myEnv->percentage_of_WS_pages_to_be_removed));
	int32 envIdMergeSort = sys_create_env("slave_ms", (myEnv->page_WS_max_size),(myEnv->SecondListSize), (myEnv->percentage_of_WS_pages_to_be_removed));
	int32 envIdStats = sys_create_env("slave_stats", (myEnv->page_WS_max_size), (myEnv->SecondListSize),(myEnv->percentage_of_WS_pages_to_be_removed));

	if (envIdQuickSort == E_ENV_CREATION_ERROR || envIdMergeSort == E_ENV_CREATION_ERROR || envIdStats == E_ENV_CREATION_ERROR)
		panic("NO AVAILABLE ENVs...");

	sys_run_env(envIdQuickSort);
	sys_run_env(envIdMergeSort);
	sys_run_env(envIdStats);

	/*[3] CREATE SHARED VARIABLES*/
	//Share the cons_mutex owner ID
	int *mutexOwnerID = smalloc("cons_mutex ownerID", sizeof(int) , 0) ;
	*mutexOwnerID = myEnv->env_id ;

	int ret;
	char Chose;
	char Line[30];
	int NumOfElements;
	int *Elements = NULL;
	//lock the console
#if USE_KERN_SEMAPHORE
	char waitCmd1[64] = "__KSem@2@Wait";
	sys_utilities(waitCmd1, 0);
#else
	wait_semaphore(cons_mutex);
#endif
	{
		cprintf("\n");
		cprintf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
		cprintf("!!!   ARRAY OOERATIONS   !!!\n");
		cprintf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
		cprintf("\n");

		readline("Enter the number of elements: ", Line);

		//Create the shared array & its size
		int *arrSize = smalloc("arrSize", sizeof(int) , 0) ;
		*arrSize = strtol(Line, NULL, 10) ;
		NumOfElements = *arrSize;
		Elements = smalloc("arr", sizeof(int) * NumOfElements , 0) ;

		cprintf("Chose the initialization method:\n") ;
		cprintf("a) Ascending\n") ;
		cprintf("b) Descending\n") ;
		cprintf("c) Semi random\n");
		do
		{
			cprintf("Select: ") ;
			Chose = getchar() ;
			cputchar(Chose);
			cputchar('\n');
		} while (Chose != 'a' && Chose != 'b' && Chose != 'c');

	}
#if USE_KERN_SEMAPHORE
	char signalCmd1[64] = "__KSem@2@Signal";
	sys_utilities(signalCmd1, 0);
#else
	signal_semaphore(cons_mutex);
#endif
	//unlock the console

	int  i ;
	switch (Chose)
	{
	case 'a':
		InitializeAscending(Elements, NumOfElements);
		break ;
	case 'b':
		InitializeDescending(Elements, NumOfElements);
		break ;
	case 'c':
		InitializeSemiRandom(Elements, NumOfElements);
		break ;
	default:
		InitializeSemiRandom(Elements, NumOfElements);
	}

	/*[4] SIGNAL READY TO THE SLAVES*/
	for (int i = 0; i < numOfSlaveProgs; ++i)
	{
#if USE_KERN_SEMAPHORE
	char signalCmd2[64] = "__KSem@0@Signal";
	sys_utilities(signalCmd2, 0);
#else
	signal_semaphore(ready);
#endif
	}

	/*[5] WAIT TILL ALL SLAVES FINISHED*/
	for (int i = 0; i < numOfSlaveProgs; ++i)
	{
#if USE_KERN_SEMAPHORE
	char waitCmd2[64] = "__KSem@1@Wait";
	sys_utilities(waitCmd2, 0);
#else
	wait_semaphore(finished);
#endif
	}

	/*[6] GET THEIR RESULTS*/
	int *quicksortedArr = NULL;
	int *mergesortedArr = NULL;
	int64 *mean = NULL;
	int64 *var = NULL;
	int *min = NULL;
	int *max = NULL;
	int *med = NULL;
	quicksortedArr = sget(envIdQuickSort, "quicksortedArr") ;
	mergesortedArr = sget(envIdMergeSort, "mergesortedArr") ;
	mean = sget(envIdStats, "mean") ;
	var = sget(envIdStats,"var") ;
	min = sget(envIdStats,"min") ;
	max = sget(envIdStats,"max") ;
	med = sget(envIdStats,"med") ;

	/*[7] VALIDATE THE RESULTS*/
	uint32 sorted = CheckSorted(quicksortedArr, NumOfElements);
	if(sorted == 0) panic("The array is NOT quick-sorted correctly") ;
	sorted = CheckSorted(mergesortedArr, NumOfElements);
	if(sorted == 0) panic("The array is NOT merge-sorted correctly") ;
	int64 correctMean, correctVar ;
	ArrayStats(Elements, NumOfElements, &correctMean , &correctVar);
	int correctMin = quicksortedArr[0];
	int last = NumOfElements-1;
//	int middle = (NumOfElements-1)/2;
//	if (NumOfElements % 2 != 0)
//		middle--;
	int middle = (NumOfElements+1)/2 - 1; /*-1 to make it ZERO-Based*/

	int correctMax = quicksortedArr[last];
	int correctMed = quicksortedArr[middle];
#if USE_KERN_SEMAPHORE
	char waitCmd3[64] = "__KSem@2@Wait";
	sys_utilities(waitCmd3, 0);
#else
	wait_semaphore(cons_mutex);
#endif
	{
		//cprintf("Array is correctly sorted\n");
		cprintf("mean = %lld, var = %lld, min = %d, max = %d, med = %d\n", *mean, *var, *min, *max, *med);
		cprintf("mean = %lld, var = %lld, min = %d, max = %d, med = %d\n", correctMean, correctVar, correctMin, correctMax, correctMed);
	}
#if USE_KERN_SEMAPHORE
	char signalCmd3[64] = "__KSem@2@Signal";
	sys_utilities(signalCmd3, 0);
#else
	signal_semaphore(cons_mutex);
#endif

	if(*mean != correctMean || *var != correctVar|| *min != correctMin || *max != correctMax || *med != correctMed)
		panic("The array STATS are NOT calculated correctly") ;

	cprintf_colored(TEXT_light_green, "Congratulations!! Scenario of Using the Semaphores & Shared Variables completed successfully!!\n\n\n");

	return;
}


uint32 CheckSorted(int *Elements, int NumOfElements)
{
	uint32 Sorted = 1 ;
	int i ;
	for (i = 0 ; i < NumOfElements - 1; i++)
	{
		if (Elements[i] > Elements[i+1])
		{
			Sorted = 0 ;
			break;
		}
	}
	return Sorted ;
}

void InitializeAscending(int *Elements, int NumOfElements)
{
	int i ;
	for (i = 0 ; i < NumOfElements ; i++)
	{
		(Elements)[i] = i ;
	}
}

void InitializeDescending(int *Elements, int NumOfElements)
{
	int i ;
	for (i = 0 ; i < NumOfElements ; i++)
	{
		Elements[i] = NumOfElements - i - 1 ;
	}

}

void InitializeSemiRandom(int *Elements, int NumOfElements)
{
	int i ;
	int Repetition = NumOfElements / 3 ;
	for (i = 0 ; i < NumOfElements ; i++)
	{
		Elements[i] = i % Repetition ;
		//cprintf("Elements[%d] = %d\n",i, Elements[i]);
	}

}

void ArrayStats(int *Elements, int NumOfElements, int64 *mean, int64 *var)
{
	int i ;
	*mean =0 ;
	for (i = 0 ; i < NumOfElements ; i++)
	{
		*mean += Elements[i];
	}
	*mean /= NumOfElements;
	*var = 0;
	for (i = 0 ; i < NumOfElements ; i++)
	{
		*var += (int64) ((Elements[i] - *mean)*(Elements[i] - *mean));
//		if (i%1000 == 0)
//			cprintf("current #elements = %d, current var = %lld\n", i , *var);
	}
	*var /= NumOfElements;
}
