#include <inc/lib.h>
#include <user/arrayOperations.h>

//Functions Declarations
void ArrayStats(int *Elements, int NumOfElements, int64 *mean, int64 *var, int *min, int *max, int *med);
int KthElement(int *Elements, int NumOfElements, int k);
int QSort(int *Elements,int NumOfElements, int startIndex, int finalIndex, int kIndex);

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
	int64 mean;
	int64 var ;
	int min ;
	int max ;
	int med ;

	//take a copy from the original array
	int *tmpArray;
	tmpArray = smalloc("tmpArr", sizeof(int) * *numOfElements, 0) ;
	int i ;
	for (i = 0 ; i < *numOfElements ; i++)
	{
		tmpArray[i] = sharedArray[i];
	}

	ArrayStats(tmpArray ,*numOfElements, &mean, &var, &min, &max, &med);

#if USE_KERN_SEMAPHORE
	char waitCmd2[64] = "__KSem@2@Wait";
	sys_utilities(waitCmd2, 0);
#else
	wait_semaphore(cons_mutex);
#endif
	{
		cprintf("Stats Calculations are Finished!!!!\n") ;
		cprintf("will share the results & notify the master now...\n");
	}
#if USE_KERN_SEMAPHORE
	char signalCmd1[64] = "__KSem@2@Signal";
	sys_utilities(signalCmd1, 0);
#else
	signal_semaphore(cons_mutex);
#endif

	/*[3] SHARE THE RESULTS & DECLARE FINISHING*/
	int64 *shMean, *shVar;
	int *shMin, *shMax, *shMed;
	shMean = smalloc("mean", sizeof(int64), 0) ; *shMean = mean;
	shVar = smalloc("var", sizeof(int64), 0) ; *shVar = var;
	shMin = smalloc("min", sizeof(int), 0) ; *shMin = min;
	shMax = smalloc("max", sizeof(int), 0) ; *shMax = max;
	shMed = smalloc("med", sizeof(int), 0) ; *shMed = med;

#if USE_KERN_SEMAPHORE
	char waitCmd3[64] = "__KSem@2@Wait";
	sys_utilities(waitCmd3, 0);
#else
	wait_semaphore(cons_mutex);
#endif
	{
		cprintf("Stats app says GOOD BYE :)\n") ;
	}
#if USE_KERN_SEMAPHORE
	char signalCmd3[64] = "__KSem@2@Signal";
	sys_utilities(signalCmd3, 0);
#else
	signal_semaphore(cons_mutex);
#endif

#if USE_KERN_SEMAPHORE
	char signalCmd2[64] = "__KSem@1@Signal";
	sys_utilities(signalCmd2, 0);
#else
	signal_semaphore(finished);
#endif
}



///Kth Element
int KthElement(int *Elements, int NumOfElements, int k)
{
	return QSort(Elements, NumOfElements, 0, NumOfElements-1, k-1) ;
}


int QSort(int *Elements,int NumOfElements, int startIndex, int finalIndex, int kIndex)
{
	if (startIndex >= finalIndex) return Elements[finalIndex];

	int pvtIndex = RANDU(startIndex, finalIndex) ;
	Swap(Elements, startIndex, pvtIndex);

	int i = startIndex+1, j = finalIndex;

	while (i <= j)
	{
		while (i <= finalIndex && Elements[startIndex] >= Elements[i]) i++;
		while (j > startIndex && Elements[startIndex] < Elements[j]) j--;

		if (i <= j)
		{
			Swap(Elements, i, j);
		}
	}

	Swap( Elements, startIndex, j);

	if (kIndex == j)
		return Elements[kIndex] ;
	else if (kIndex < j)
		return QSort(Elements, NumOfElements, startIndex, j - 1, kIndex);
	else
		return QSort(Elements, NumOfElements, i, finalIndex, kIndex);
}

void ArrayStats(int *Elements, int NumOfElements, int64 *mean, int64 *var, int *min, int *max, int *med)
{
	int i ;
	*mean =0 ;
	*min = 0x7FFFFFFF ;
	*max = 0x80000000 ;
	for (i = 0 ; i < NumOfElements ; i++)
	{
		(*mean) += Elements[i];
		if (Elements[i] < (*min))
		{
			(*min) = Elements[i];
		}
		if (Elements[i] > (*max))
		{
			(*max) = Elements[i];
		}
	}

	(*med) = KthElement(Elements, NumOfElements, (NumOfElements+1)/2);

	(*mean) /= NumOfElements;
	(*var) = 0;
	for (i = 0 ; i < NumOfElements ; i++)
	{
		(*var) += (int64)((Elements[i] - (*mean))*(Elements[i] - (*mean)));
	}
	(*var) /= NumOfElements;
}
