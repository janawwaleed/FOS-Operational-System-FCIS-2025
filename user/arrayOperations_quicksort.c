#include <inc/lib.h>
#include <user/arrayOperations.h>

void QuickSort(int *Elements, int NumOfElements);
void QSort(int *Elements,int NumOfElements, int startIndex, int finalIndex);

void _main(void)
{
	int32 envID = sys_getenvid();
	int32 parentenvID = sys_getparentenvid();

	int ret;
	/*[1] GET SEMAPHORES*/
#if USE_KERN_SEMAPHORE
#else
	struct semaphore ready = get_semaphore(parentenvID, "Ready");
	struct semaphore finished = get_semaphore(parentenvID, "Finished");
#endif

	/*[2] WAIT A READY SIGNAL FROM THE MASTER*/
#if USE_KERN_SEMAPHORE
	char waitCmd1[64] = "__KSem@0@Wait";
	sys_utilities(waitCmd1, 0);
#else
	wait_semaphore(ready);
#endif

	/*[3] GET SHARED VARs*/
	//Get the cons_mutex ownerID
	int* consMutexOwnerID = sget(parentenvID, "cons_mutex ownerID") ;
#if USE_KERN_SEMAPHORE
#else
	struct semaphore cons_mutex = get_semaphore(*consMutexOwnerID, "Console Mutex");
#endif

	//Get the shared array & its size
	int *numOfElements = NULL;
	int *sharedArray = NULL;
	sharedArray = sget(parentenvID,"arr") ;
	numOfElements = sget(parentenvID,"arrSize") ;

	/*[4] DO THE JOB*/
	//take a copy from the original array
	int *sortedArray;
	sortedArray = smalloc("quicksortedArr", sizeof(int) * *numOfElements, 0) ;
	int i ;
	for (i = 0 ; i < *numOfElements ; i++)
	{
		sortedArray[i] = sharedArray[i];
	}
	QuickSort(sortedArray, *numOfElements);

#if USE_KERN_SEMAPHORE
	char waitCmd2[64] = "__KSem@2@Wait";
	sys_utilities(waitCmd2, 0);
#else
	wait_semaphore(cons_mutex);
#endif
	{
		cprintf("Quick sort is Finished!!!!\n") ;
		cprintf("will notify the master now...\n");
		cprintf("Quick sort says GOOD BYE :)\n") ;
	}
#if USE_KERN_SEMAPHORE
	char signalCmd1[64] = "__KSem@2@Signal";
	sys_utilities(signalCmd1, 0);
#else
	signal_semaphore(cons_mutex);
#endif

	/*[5] DECLARE FINISHING*/
#if USE_KERN_SEMAPHORE
	char signalCmd2[64] = "__KSem@1@Signal";
	sys_utilities(signalCmd2, 0);
#else
	signal_semaphore(finished);
#endif
}

///Quick sort
void QuickSort(int *Elements, int NumOfElements)
{
	QSort(Elements, NumOfElements, 0, NumOfElements-1) ;
}


void QSort(int *Elements,int NumOfElements, int startIndex, int finalIndex)
{
	if (startIndex >= finalIndex) return;
	int pvtIndex = RANDU(startIndex, finalIndex) ;
	Swap(Elements, startIndex, pvtIndex);

	int i = startIndex+1, j = finalIndex;

	while (i <= j)
	{
		while (i <= finalIndex && Elements[startIndex] >= Elements[i]) i++;
		while (j > startIndex && Elements[startIndex] <= Elements[j]) j--;

		if (i <= j)
		{
			Swap(Elements, i, j);
		}
	}

	Swap( Elements, startIndex, j);

	QSort(Elements, NumOfElements, startIndex, j - 1);
	QSort(Elements, NumOfElements, i, finalIndex);

	//cprintf("qs,after sorting: start = %d, end = %d\n", startIndex, finalIndex);

}
