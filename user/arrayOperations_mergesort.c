#include <inc/lib.h>
#include <user/arrayOperations.h>


void MSort(int* A, int p, int r);
void Merge(int* A, int p, int q, int r);

//int *Left;
//int *Right;

void _main(void)
{
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
	sharedArray = sget(parentenvID, "arr") ;
	numOfElements = sget(parentenvID, "arrSize") ;
	//PrintElements(sharedArray, *numOfElements);

	/*[4] DO THE JOB*/
	//take a copy from the original array
	int *sortedArray;

	sortedArray = smalloc("mergesortedArr", sizeof(int) * *numOfElements, 0) ;
	int i ;
	for (i = 0 ; i < *numOfElements ; i++)
	{
		sortedArray[i] = sharedArray[i];
	}
//	//Create two temps array for "left" & "right"
//	Left = smalloc("mergesortLeftArr", sizeof(int) * (*numOfElements), 1) ;
//	Right = smalloc("mergesortRightArr", sizeof(int) * (*numOfElements), 1) ;

	MSort(sortedArray, 1, *numOfElements);

#if USE_KERN_SEMAPHORE
	char waitCmd2[64] = "__KSem@2@Wait";
	sys_utilities(waitCmd2, 0);
#else
	wait_semaphore(cons_mutex);
#endif
	{
		cprintf("Merge sort is Finished!!!!\n") ;
		cprintf("will notify the master now...\n");
		cprintf("Merge sort says GOOD BYE :)\n") ;
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


void MSort(int* A, int p, int r)
{
	if (p >= r)
	{
		return;
	}

	int q = (p + r) / 2;

	MSort(A, p, q);
//	cprintf("LEFT is sorted: from %d to %d\n", p, q);

	MSort(A, q + 1, r);
//	cprintf("RIGHT is sorted: from %d to %d\n", q+1, r);

	Merge(A, p, q, r);
	//cprintf("[%d %d] + [%d %d] = [%d %d]\n", p, q, q+1, r, p, r);

}

void Merge(int* A, int p, int q, int r)
{
	int leftCapacity = q - p + 1;

	int rightCapacity = r - q;

	int leftIndex = 0;

	int rightIndex = 0;

	int* Left = malloc(sizeof(int) * leftCapacity);

	int* Right = malloc(sizeof(int) * rightCapacity);

	int i, j, k;
	for (i = 0; i < leftCapacity; i++)
	{
		Left[i] = A[p + i - 1];
	}
	for (j = 0; j < rightCapacity; j++)
	{
		Right[j] = A[q + j];
	}

	for ( k = p; k <= r; k++)
	{
		if (leftIndex < leftCapacity && rightIndex < rightCapacity)
		{
			if (Left[leftIndex] < Right[rightIndex] )
			{
				A[k - 1] = Left[leftIndex++];
			}
			else
			{
				A[k - 1] = Right[rightIndex++];
			}
		}
		else if (leftIndex < leftCapacity)
		{
			A[k - 1] = Left[leftIndex++];
		}
		else
		{
			A[k - 1] = Right[rightIndex++];
		}
	}

	free(Left);
	free(Right);

}

