// os345.c - OS Kernel	09/12/2013
// ***********************************************************************
// **   DISCLAMER ** DISCLAMER ** DISCLAMER ** DISCLAMER ** DISCLAMER   **
// **                                                                   **
// ** The code given here is the basis for the BYU CS345 projects.      **
// ** It comes "as is" and "unwarranted."  As such, when you use part   **
// ** or all of the code, it becomes "yours" and you are responsible to **
// ** understand any algorithm or method presented.  Likewise, any      **
// ** errors or problems become your responsibility to fix.             **
// **                                                                   **
// ** NOTES:                                                            **
// ** -Comments beginning with "// ??" may require some implementation. **
// ** -Tab stops are set at every 3 spaces.                             **
// ** -The function API's in "OS345.h" should not be altered.           **
// **                                                                   **
// **   DISCLAMER ** DISCLAMER ** DISCLAMER ** DISCLAMER ** DISCLAMER   **
// ***********************************************************************

//#include "stdafx.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <setjmp.h>
#include <time.h>
#include <assert.h>

#include "os345.h"
#include "os345signals.h"
#include "os345config.h"
#include "os345lc3.h"
#include "os345fat.h"

// **********************************************************************
//	local prototypes
//
void pollInterrupts(void);
static int scheduler(void);
static int dispatcher(void);
void redoTime();

//static void keyboard_isr(void);
//static void timer_isr(void);

int sysKillTask(int taskId);
static int initOS(void);

// **********************************************************************
// **********************************************************************
// global semaphores

Semaphore* semaphoreList;			// linked list of active semaphores

Semaphore* keyboard;				// keyboard semaphore
Semaphore* charReady;				// character has been entered
Semaphore* inBufferReady;			// input buffer ready semaphore

Semaphore* tics1sec;				// 1 second semaphore
Semaphore* tics10thsec;				// 1/10 second semaphore
Semaphore* tics10sec;				//10 sec sem

// **********************************************************************
// **********************************************************************
// global system variables

TCB tcb[MAX_TASKS];					// task control block
Semaphore* taskSems[MAX_TASKS];		// task semaphore
jmp_buf k_context;					// context of kernel stack
jmp_buf reset_context;				// context of kernel stack
volatile void* temp;				// temp pointer used in dispatcher

int scheduler_mode;					// scheduler mode
int superMode;						// system mode
int curTask;						// current task #
long swapCount;						// number of re-schedule cycles
char inChar;						// last entered character
int charFlag;						// 0 => buffered input
int inBufIndx;						// input pointer into input buffer
char inBuffer[INBUF_SIZE+1];		// character input buffer
//Message messages[NUM_MESSAGES];		// process message buffers

int deltaTics;						// current clock()
int pollClock;						// current clock()
int lastPollClock;					// last pollClock
bool diskMounted;					// disk has been mounted

time_t oldTime1;					// old 1sec time
time_t oldTime10;					//old time 10 sec
clock_t myClkTime;
clock_t myOldClkTime;

//int* rq;							// ready priority queue
//extern PQ* q;


// **********************************************************************
// **********************************************************************
// OS startup
//
// 1. Init OS
// 2. Define reset longjmp vector
// 3. Define global system semaphores
// 4. Create CLI task
// 5. Enter scheduling/idle loop
//
int main(int argc, char* argv[])
{
	// save context for restart (a system reset would return here...)
	int resetCode = setjmp(reset_context);
	superMode = TRUE;						// supervisor mode
	//TODO when pulling out arguments malloc them into a variable
	switch (resetCode)
	{
		case POWER_DOWN_QUIT:				// quit
			powerDown(0);
			printf("\nGoodbye!!");
			return 0;

		case POWER_DOWN_RESTART:			// restart
			powerDown(resetCode);
			printf("\nRestarting system...\n");

		case POWER_UP:						// startup
			break;

		default:
			printf("\nShutting down due to error %d", resetCode);
			powerDown(resetCode);
			return resetCode;
	}

	// output header message
	printf("%s", STARTUP_MSG);

	// initalize OS
	if ( resetCode = initOS()) return resetCode;

	// create global/system semaphores here
	//?? vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv

	charReady = createSemaphore("charReady", BINARY, 0);
	inBufferReady = createSemaphore("inBufferReady", BINARY, 0);
	keyboard = createSemaphore("keyboard", BINARY, 1);
	tics1sec = createSemaphore("tics1sec", BINARY, 0);
	tics10sec = createSemaphore("tics10sec",COUNTING,0);
	tics10thsec = createSemaphore("tics10thsec", BINARY, 0);

	//?? ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	// schedule CLI task
	createTask("myShell",			// task name
					P1_shellTask,	// task
					MED_PRIORITY,	// task priority
					argc,			// task arg count
					argv);			// task argument pointers

	// HERE WE GO................

	// Scheduling loop
	// 1. Check for asynchronous events (character inputs, timers, etc.)
	// 2. Choose a ready task to schedule
	// 3. Dispatch task
	// 4. Loop (forever!)

	while(1)									// scheduling loop
	{
		// check for character / timer interrupts
		pollInterrupts();
		// schedule highest priority ready task
		signals();
		if ((curTask = scheduler()) < 0) continue;

		// dispatch curTask, quit OS if negative return
		if (dispatcher() < 0) break;
	}											// end of scheduling loop

	// exit os
	longjmp(reset_context, POWER_DOWN_QUIT);
	return 0;
} // end main



// **********************************************************************
// **********************************************************************
// scheduler
//
static int scheduler()
{
	int nextTask,tid = -1;
	// ?? Design and implement a scheduler that will select the next highest
	// ?? priority ready task to pass to the system dispatcher.

	// ?? WARNING: You must NEVER call swapTask() from within this function
	// ?? or any function that it calls.  This is because swapping is
	// ?? handled entirely in the swapTask function, which, in turn, may
	// ?? call this function.  (ie. You would create an infinite loop.)

	// ?? Implement a round-robin, preemptive, prioritized scheduler.

	// ?? This code is simply a round-robin scheduler and is just to get
	// ?? you thinking about scheduling.  You must implement code to handle
	// ?? priorities, clean up dead tasks, and handle semaphores appropriately.

	//if just using priority queue in round robin fashion
	if (!SCHEDULER_MODE) {
		if ((tid = deQ(q, -1)) >= 0) { ///DRIVER tasls not exiting, not in ready or regular queue
			enQ(q, tid, tcb[tid].priority);
		}
		else {
			tid = 0;
		}
		
	}
	//if doing fair algorithm
	else {
		for (int i = 0; i < q->size; i++)
		{
			int myTid = q->pq[i].tid;
			//if time remaining > 0, schedule it
			if (tcb[myTid].name && tcb[myTid].timeRemaining > 0) {
				tid = myTid;
				tcb[tid].timeRemaining--;
				break;
			}
		}
		//if we didnt find anything with a time remaining > 0
		//Time to redo fair algorithm
		if (tid < 0) {
			redoTime();
			tid = 0;
		}
	}
	// schedule next task
	nextTask = tid;

	if (tcb[nextTask].signal & mySIGSTOP) return -1;
	return nextTask;
} // end scheduler

void promoteChild(int *parent, int curTid) {
	//make this child's parent the shell
	tcb[curTid].parent = 0;
	//go through the TCB and replace the dead parent with the new parent for that group
	for (int i = 0; i < MAX_TASKS; i++)
	{
		//if the old parent was your parent, here is your new parent
		if ((*parent) == tcb[i].parent) {
			tcb[i].parent = curTid;
			tcb[i].name = "newParent";
		}
	}
	//the student is now the master
	(*parent) = curTid;
}

//Is dynamic
//Math: add ups users.Do users / groups * 10 (mult to get rid of decimal)
//	Then do that number / number in group, that is then how much each user in each group gets.Invluding the parent task, the parent is also given any remainder from that division
//
//	Parent needs to be given leftovers
void redoTime() {

	//TODO make fair schedular work with dead ancestors, check to see if parent ares dead(not in TCB), promote one of the children to parent. (so you can give the extra time to the parent)
	int groupArray[MAX_TASKS] = {0};
	int parentGiven[MAX_TASKS] = {0};
	int groups = 0,time,leftover, parent;
	int curTid;
	for (int i = 0; i < MAX_TASKS; i++)
	{
		curTid = q->pq[i].tid;
		if (curTid < 0) {break;}
		
		if (tcb[curTid].name && curTid != 0) { //if exists and is not the shell, and parent is not the shell
			parent = !tcb[curTid].parent ? curTid : tcb[curTid].parent;
			if (!tcb[parent].name) {//if parent is dead
				promoteChild(&parent,curTid);
			}
			//if we havent seen this group yet
			if (groupArray[parent] == 0) {
				groups++;
			}
			++(groupArray[parent]);
			
		}
	}
	if (groups == 0) { ++groups; }
	time = ((float)q->size / (float)groups) * 10;// time / groups;
	int grpTime = 0;
	leftover = time % groups;

	for (int i = 0; i < q->size; i++)
	{
		curTid = q->pq[i].tid;
		//get the parent tid of this task
		
		if (curTid != 0) {
			parent = !tcb[curTid].parent ? curTid : tcb[curTid].parent;
			//give this task the appropriate amount of time
			tcb[curTid].timeRemaining += grpTime = time / (groupArray[parent]);
			//give parent what is left over
			if (!parentGiven[parent]) {
				parentGiven[parent] = 1;
				tcb[parent].timeRemaining += time % (groupArray[parent]);
			}
		}
	}
	tcb[0].timeRemaining = leftover;
	
}

// **********************************************************************
// **********************************************************************
// dispatch curTask
//
static int dispatcher()
{
	int result;
	int cant_schedule = signals();
	if (cant_schedule) {
		return 0;
	}
	// schedule task
	switch(tcb[curTask].state)
	{
		case S_NEW:
		{
			// new task
			printf("\nNew Task[%d] %s", curTask, tcb[curTask].name);
			tcb[curTask].state = S_RUNNING;	// set task to run state

			// save kernel context for task SWAP's
			if (setjmp(k_context))
			{
				superMode = TRUE;					// supervisor mode
				break;								// context switch to next task
			}

			// move to new task stack (leave room for return value/address)
			temp = (int*)tcb[curTask].stack + (STACK_SIZE-8);
			SET_STACK(temp);
			superMode = FALSE;						// user mode

			// begin execution of new task, pass argc, argv
			result = (*tcb[curTask].task)(tcb[curTask].argc, tcb[curTask].argv);

			// task has completed
			if (result) printf("\nTask[%d] returned %d", curTask, result);
			else printf("\nTask[%d] returned %d", curTask, result);
			//if (curTask != 0) {
				tcb[curTask].state = S_EXIT;			// set task to exit state
			//}

			// return to kernal mode
			longjmp(k_context, 1);					// return to kernel
		}

		case S_READY:
		{
			tcb[curTask].state = S_RUNNING;			// set task to run
		}

		case S_RUNNING:
		{
			if (setjmp(k_context))
			{
				// SWAP executed in task
				superMode = TRUE;					// supervisor mode
				break;								// return from task
			}
			//signals will handle the clearing of signals and calling of handlers
			if (signals()) break;					//if we get signals
			longjmp(tcb[curTask].context, 3); 		// restore task context
		}

		case S_BLOCKED:
		{
			break;
		}

		case S_EXIT:
		{
			if (curTask == 0) return -1;			// if CLI, then quit scheduler
			// release resources and kill task
			///TODO error
			sysKillTask(curTask);					// kill current task
			break;
		}

		default:
		{
			printf("Unknown Task[%d] State", curTask);
			longjmp(reset_context, POWER_DOWN_ERROR);
		}
	}
	return 0;
} // end dispatcher



// **********************************************************************
// **********************************************************************
// Do a context switch to next task.

// 1. If scheduling task, return (setjmp returns non-zero value)
// 2. Else, save current task context (setjmp returns zero value)
// 3. Set current task state to READY
// 4. Enter kernel mode (longjmp to k_context)

void swapTask()
{
	assert("SWAP Error" && !superMode);		// assert user mode

	// increment swap cycle counter
	swapCount++;

	// either save current task context or schedule task (return)
	if (setjmp(tcb[curTask].context))
	{
		superMode = FALSE;					// user mode
		return;
	}

	// context switch - move task state to ready
	if (tcb[curTask].state == S_RUNNING) tcb[curTask].state = S_READY;

	// move to kernel mode (reschedule)
	longjmp(k_context, 2);
} // end swapTask



// **********************************************************************
// **********************************************************************
// system utility functions
// **********************************************************************
// **********************************************************************

// **********************************************************************
// **********************************************************************
// initialize operating system
static int initOS()
{
	int i;
	MY_DEBUG_MODE = 1;
	// make any system adjustments (for unblocking keyboard inputs)
	INIT_OS

	// reset system variables
	curTask = 0;						// current task #
	swapCount = 0;						// number of scheduler cycles
	scheduler_mode = 0;					// default scheduler
	inChar = 0;							// last entered character
	charFlag = 0;						// 0 => buffered input
	inBufIndx = 0;						// input pointer into input buffer
	semaphoreList = 0;					// linked list of active semaphores
	diskMounted = 0;					// disk has been mounted
	//maxTid = 0;
	fp = fopen("commands.txt","w");		//open command file, allowing you to overwrite the last session
	fclose(fp);

	// malloc ready queue
	//rq = (int*)malloc(MAX_TASKS * sizeof(int));
	q = (PQ*)malloc(sizeof(PQ));
	if (q == NULL) return -1;
	q->size = 0;
	q->isrq = 1;
	// capture current time
	lastPollClock = clock();			// last pollClock
	time(&oldTime1);
	time(&oldTime10);

	// init system tcb's
	for (i=0; i<MAX_TASKS; i++)
	{
		tcb[i].name = NULL;				// tcb
		taskSems[i] = NULL;				// task semaphore
	}

	// init tcb
	for (i=0; i<MAX_TASKS; i++)
	{
		tcb[i].name = NULL;
	}

	// initialize lc-3 memory
	initLC3Memory(LC3_MEM_FRAME, 0xF800>>6);

	// ?? initialize all execution queues

	return 0;
} // end initOS



// **********************************************************************
// **********************************************************************
// Causes the system to shut down. Use this for critical errors
void powerDown(int code)
{
	int i;
	printf("\nPowerDown Code %d", code);

	// release all system resources.
	printf("\nRecovering Task Resources...");

	// kill all tasks
	for (i = MAX_TASKS-1; i >= 0; i--)
		if(tcb[i].name) sysKillTask(i);

	// delete all semaphores
	while (semaphoreList)
		deleteSemaphore(&semaphoreList);

	// free ready queue
	free(q);

	// ?? release any other system resources
	// ?? deltaclock (project 3)

	RESTORE_OS
	return;
} // end powerDown

int enQ(PQ* q, TID tid, int priority) {
	int s = q->size;
	if (tid < 0) {
		return tid;
	}

	if (s < 128) {
		q->pq[q->size].priority = priority;
		q->pq[q->size].tid = tid;
		q->size++;
	}
	return tid;
}

void rollup(int i, PQ *q) {
	if (i < 0) {
		return;
	}
	for (size_t j = i; j < q->size; j++)
	{
		if (j == 127) {
			q->pq[j].priority = -1;
			q->pq[j].tid = -1;
		}
		q->pq[j] = q->pq[j + 1];
		q->pq[j] = q->pq[j + 1];
	}
	q->size--;
}

int deQ(PQ *q, int tid) {
	bool isneg = 0;
	int greatest = -1, idx = -1, id = -1;
	if (tid == -1) {
		isneg = 1;
	}
	for (int i = 0; i < q->size; i++) {
		if (isneg && q->pq[i].priority > greatest) {
			greatest = q->pq[i].priority;
			idx = i;
			id = q->pq[i].tid;
		}
		else if (q->pq[i].tid == tid) {
			idx = i;
			id = tid;
		}
	}
	//if we actually got something
	if (idx >= 0) {
		rollup(idx, q);
	}
	return id;
}