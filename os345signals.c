// os345signal.c - signals
// ***********************************************************************
// **   DISCLAMER ** DISCLAMER ** DISCLAMER ** DISCLAMER ** DISCLAMER   **
// **                                                                   **
// ** The code given here is the basis for the CS345 projects.          **
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
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <setjmp.h>
#include <assert.h>
#include "os345.h"
#include "os345signals.h"

extern TCB tcb[];							// task control block
extern int curTask;							// current task #

// ***********************************************************************
// ***********************************************************************
//	Call all pending task signal handlers
//
//	return 1 if task is NOT to be scheduled.
//
int signals(void)
{
	if (tcb[0].signal & mySIGCONT) {
		curTask = 0;
	}
	if (tcb[curTask].signal)
	{
		if (tcb[curTask].signal & mySIGINT){
			tcb[curTask].signal &= ~mySIGINT;
			(*tcb[curTask].sigIntHandler)();
		}
		if (tcb[curTask].signal & mySIGCONT) {
			tcb[curTask].signal &= ~mySIGCONT;
			tcb[curTask].signal &= ~mySIGSTOP;
			(*tcb[curTask].sigContHandler)();
		}
		if (tcb[curTask].signal & mySIGKILL) {
			tcb[curTask].signal &= ~mySIGKILL;
			(*tcb[curTask].sigKillHandler)();
		}
		if (tcb[curTask].signal & mySIGTERM) {
			tcb[curTask].signal &= ~mySIGTERM;
			(*tcb[curTask].sigTermHandler)();
			return 1;
		}
		if (tcb[curTask].signal & mySIGTSTP) {
			tcb[curTask].signal &= ~mySIGTSTP;
			(*tcb[curTask].sigTstpHandler)();
			return 1;
		}
	}
	return 0;
}


// **********************************************************************
// **********************************************************************
//	Register task signal handlers
//
int sigAction(void (*sigHandler)(void), int sig)
{
	switch (sig)
	{
		case mySIGINT:
		{
			tcb[curTask].sigIntHandler = sigHandler;		// mySIGINT handler
			return 0;
		}
		case mySIGCONT:
		{
			tcb[curTask].sigContHandler = sigHandler;		// cont handler
			return 0;
		}
		case mySIGKILL:
		{
			tcb[curTask].sigKillHandler = sigHandler;		// kill handler
			return 0;
		}
		case mySIGTERM:
		{
			tcb[curTask].sigTermHandler = sigHandler;		// term handler
			return 0;
		}
		case mySIGTSTP:
		{
			tcb[curTask].sigTstpHandler = sigHandler;		// Tstp handler
			return 0;
		}
	}
	return 1;
}


// **********************************************************************
//	sigSignal - send signal to task(s)
//
//	taskId = task (-1 = all tasks)
//	sig = signal
//
int sigSignal(int taskId, int sig)
{
	// check for task
	if ((taskId >= 0) && tcb[taskId].name)
	{
		tcb[taskId].signal |= sig;
		return 0;
	}
	else if (taskId == -1)
	{
		for (taskId=0; taskId<MAX_TASKS; taskId++)
		{
			sigSignal(taskId, sig);
			//Tell processes to delete their sems, will kill blocked processes
			semSignal(tcb[taskId].event); 
		}
		return 0;
	}
	// error
	return 1;
}


// **********************************************************************
// **********************************************************************
//	Default signal handlers
//
void defaultSigIntHandler(void)			// task mySIGINT handler
{
	printf("\ndefaultSigIntHandler");
	return;
}
void defaultsigContHandler(void)			// task mySIGCONT handler
{
	printf("\ndefaultSigContHandler");
	return;
}
void defaultsigKillHandler(void)			// task mySIGKILL handler
{
	printf("\ndefaultSigKillHandler");
	return;
}
void defaultsigTstpHandler(void)			// task mySIGTSTP handler
{
	printf("\ndefaultSigTstpHandler");
	return;
}
void defaultsigTermHandler(void)			// task mySIGTERM handler
{
	printf("\ndefaultSigTermHandler");
	return;
}


void createTaskSigHandlers(int tid)
{
	tcb[tid].signal = 0;
	if (tid)
	{
		// inherit parent signal handlers
		tcb[tid].sigIntHandler = tcb[curTask].sigIntHandler;			// mySIGINT handler
		tcb[tid].sigContHandler = tcb[curTask].sigContHandler;
		tcb[tid].sigKillHandler = tcb[curTask].sigKillHandler;
		tcb[tid].sigTstpHandler = tcb[curTask].sigTstpHandler;
		tcb[tid].sigTermHandler = tcb[curTask].sigTermHandler;
	}
	else
	{
		// otherwise use defaults
		tcb[tid].sigIntHandler = defaultSigIntHandler;			// task mySIGINT handler
		tcb[tid].sigContHandler = defaultsigContHandler;
		tcb[tid].sigKillHandler = defaultsigKillHandler;
		tcb[tid].sigTstpHandler = defaultsigTstpHandler;
		tcb[tid].sigTermHandler = defaultsigTermHandler;
	}
}
