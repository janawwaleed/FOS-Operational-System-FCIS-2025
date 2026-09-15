/*
 * channel.c
 *
 *  Created on: Sep 22, 2024
 *      Author: HP
 */
#include "channel.h"
#include <kern/proc/user_environment.h>
#include <kern/cpu/sched.h>
#include <inc/string.h>
#include <inc/disk.h>

//===============================
// 1) INITIALIZE THE CHANNEL:
//===============================
// initialize its lock & queue
void init_channel(struct Channel *chan, char *name)
{
	strcpy(chan->name, name);
	init_queue(&(chan->queue));
}

//===============================
// 2) SLEEP ON A GIVEN CHANNEL:
//===============================
// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
// Ref: xv6-x86 OS code
void sleep(struct Channel *chan, struct kspinlock* lk)
{	/*Othman [PROJECT'25.IM#5] KERNEL PROTECTION*/

	acquire_kspinlock(&(ProcessQueues.qlock));

	struct Env* block= get_cpu_proc();
	release_kspinlock(lk);
	block->env_status=ENV_BLOCKED;
	enqueue(&(chan->queue),block);
	sched();
	acquire_kspinlock(lk);

	release_kspinlock(&(ProcessQueues.qlock));

}

//==================================================
// 3) WAKEUP ONE BLOCKED PROCESS ON A GIVEN CHANNEL:
//==================================================
// Wake up ONE process sleeping on chan.
// The qlock must be held.
// Ref: xv6-x86 OS code
// chan MUST be of type "struct Env_Queue" to hold the blocked processes
void wakeup_one(struct Channel *chan)
{	/*Othman [PROJECT'25.IM#5] KERNEL PROTECTION*/
	//TODO: [PROJECT'25.IM#5] KERNEL PROTECTION: #2 CHANNEL - wakeup_one
	//Your code is here
	acquire_kspinlock(&(ProcessQueues.qlock));

	if(queue_size(&(chan->queue))>0){
		struct Env* block= dequeue(&(chan->queue));
		sched_insert_ready(block);
	}

	release_kspinlock(&(ProcessQueues.qlock));
	//Comment the following line
	//panic("wakeup_one() is not implemented yet...!!");
}

//====================================================
// 4) WAKEUP ALL BLOCKED PROCESSES ON A GIVEN CHANNEL:
//====================================================
// Wake up all processes sleeping on chan.
// The queues lock must be held.
// Ref: xv6-x86 OS code
// chan MUST be of type "struct Env_Queue" to hold the blocked processes

void wakeup_all(struct Channel *chan)
{	/*Othman [PROJECT'25.IM#5] KERNEL PROTECTION*/
	acquire_kspinlock(&(ProcessQueues.qlock));

	int size=queue_size(&(chan->queue));
	for(int i=0;i<size;i++){
		struct Env* block= dequeue(&(chan->queue));
		sched_insert_ready(block);
	}

	release_kspinlock(&(ProcessQueues.qlock));
}

