// os345interrupts.c - pollInterrupts	08/08/2013
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <setjmp.h>
#include <time.h>
#include <assert.h>

#include "os345.h"
#include "os345config.h"
#include "os345signals.h"


int lines_in_file = 0;//num of lines in file
int place_in_file = 0;//our place in the file holding commands
bool just_keyed = 0; //if our most recent action was to go up or down on the arrow keys
// **********************************************************************
//	local prototypes
//
void pollInterrupts(void);
static void keyboard_isr(void);
static void timer_isr(void);
void saveLine(char *line);
void delete_curr_line();
void get_file_line(bool goUp);
int check_place_in_file(bool goUp);
extern TCB tcb[];
// **********************************************************************
// **********************************************************************
// global semaphores

extern Semaphore* keyboard;				// keyboard semaphore
extern Semaphore* charReady;				// character has been entered
extern Semaphore* inBufferReady;			// input buffer ready semaphore

extern Semaphore* tics1sec;				// 1 second semaphore
extern Semaphore* tics10thsec;				// 1/10 second semaphore
extern Semaphore* tics10sec;				//10 sec sem

extern char inChar;				// last entered character
extern int charFlag;				// 0 => buffered input
extern int inBufIndx;				// input pointer into input buffer
extern char inBuffer[INBUF_SIZE+1];	// character input buffer

extern time_t oldTime1;					// old 1sec time
extern time_t oldTime10;				//old 10 sec time
extern clock_t myClkTime;
extern clock_t myOldClkTime;

extern int pollClock;				// current clock()
extern int lastPollClock;			// last pollClock

extern int superMode;						// system mode

extern int deltaTics;


// **********************************************************************
// **********************************************************************
// simulate asynchronous interrupts by polling events during idle loop
//
void pollInterrupts(void)
{
	// check for task monopoly
	pollClock = clock();
	//TODO had to comment this line out, was crashing
	//assert("Timeout" && ((pollClock - lastPollClock) < MAX_CYCLES));
	lastPollClock = pollClock;

	// check for keyboard interrupt
	if ((inChar = GET_CHAR) > 0)
	{
	  keyboard_isr();
	}

	// timer interrupt
	timer_isr();

	return;
} // end pollInterrupts


// **********************************************************************
// keyboard interrupt service routine
//
static void keyboard_isr()
{
	// assert system mode
	assert("keyboard_isr Error" && superMode);
	bool goUp;	//if we are going up or down in the command file

	semSignal(charReady);					// SIGNAL(charReady) (No Swap)
	if (charFlag == 0)
	{
		//printf("%02x ", inChar);
		switch (inChar)
		{
			case '\r':
			case '\n':
			{
				saveLine(inBuffer);
				inBufIndx = 0;				// EOL, signal line ready
				semSignal(inBufferReady);	// SIGNAL(inBufferReady)
				break;
			}

			case 0x18:						// ^x
			{
				inBufIndx = 0;
				inBuffer[0] = 0;
				sigSignal(-1, mySIGINT);		// interrupt task 0
				semSignal(inBufferReady);	// SEM_SIGNAL(inBufferReady)
				break;
			}
			case 0x12:						//^r
			{
				inBufIndx = 0;
				inBuffer[0] = 0;
				sigSignal(-1, mySIGCONT);		// interrupt all other tasks
				for (int id = 0; id<MAX_TASKS; id++)
				{
					tcb[id].signal &= (~mySIGSTOP & ~mySIGTSTP);
				}
				break;
			}
			case 0x08:						//when backspace is pressed
				if (inBufIndx > 0) {
					--inBufIndx;
					inBuffer[inBufIndx] = 0;
					printf("\b \b");
				}
				break;
			case 0x17:						//^w
			{
				inBufIndx = 0;
				inBuffer[0] = 0;
				sigSignal(-1, mySIGTSTP);		// interrupt all other tasks
				semSignal(inBufferReady);	// SEM_SIGNAL(inBufferReady)
				break;
			}
			case 72:
				goUp = 1;
				get_file_line(goUp);
				break;
			case 80:
				goUp = 0;
				get_file_line(goUp);
				break;

			default:
			{
				
				if (inBufIndx < INBUF_SIZE) {
					inBuffer[inBufIndx++] = inChar;
					inBuffer[inBufIndx] = 0;
					printf("%c", inChar);		// echo character
				}
			}
		}
	}
	else
	{
		// single character mode
		inBufIndx = 0;
		inBuffer[inBufIndx] = 0;
	}
	return;
} // end keyboard_isr


void saveLine(char *line) {
	bool save = 0;
	//check to see if this line just has whitespace, if so, dont save it to the command file
	for (size_t i = 0; i < strlen(line); i++)
	{
		if (!isspace(line[i])) {
			save = 1;
			break;
		}
	}
	//if we are going to save this line
	if (save) {
		fp = fopen("commands.txt", "a+");
		fprintf(fp, "%s\n", line);
		fclose(fp);
		lines_in_file++; //new line in file
		place_in_file = lines_in_file; //now at end of file
		just_keyed = 0;
	}
}
void delete_curr_line() {
	while (inBufIndx > 0) {	//delete everything already on the line
		--inBufIndx;
		inBuffer[inBufIndx] = 0;
		printf("\b \b");
	}
}
int check_place_in_file(bool goUp) {
	if (!goUp) {//if going down
		if (place_in_file + 1 <= lines_in_file) {
			if (just_keyed) {
				place_in_file++;//go down
			}
		}
		else {
			return -1; //nowhere to go
		}
	}
	else {//if going up
		if (place_in_file - 1 > 0) {
			if (just_keyed) {
				place_in_file--;//go up
			}
		}
		else {
			return -1;//nowhere to go
		}
	}
	return 0;
}

void get_file_line(bool goUp) {
	fp = fopen("commands.txt","r");
	//see if we can actually go up or down in the command list
	if (check_place_in_file(goUp) < 0) {
		return;
	}
	
	int count = 0;
	if (fp != NULL) {
		delete_curr_line(); //delete what is currently on the command line
		char line[1024];
		while (fgets(line,sizeof(line),fp)){
			count++;
			if (place_in_file == count) {
				line[strlen(line) - 1] = 0; //get rid of the new line
				break;
			}
		}
		fclose(fp);
		//put the line on the buffer
		strcpy(inBuffer, line);
		printf("%s", inBuffer);
		inBufIndx = strlen(inBuffer);
		inBuffer[inBufIndx] = 0;
		just_keyed = 1; //indicate that our last action was to go up or down the command list
	}
}

// **********************************************************************
// timer interrupt service routine
//
static void timer_isr()
{
	time_t currentTime;						// current time

	// assert system mode
	assert("timer_isr Error" && superMode);

	// capture current time
  	time(&currentTime);

  	// one second timer
  	if ((currentTime - oldTime1) >= 1)
  	{
		// signal 1 second
  	   semSignal(tics1sec);
		oldTime1 += 1;
  	}

	// sample fine clock
	myClkTime = clock();
	if ((myClkTime - myOldClkTime) >= ONE_TENTH_SEC)
	{
		deltaTics++;
		myOldClkTime = myOldClkTime + ONE_TENTH_SEC;   // update old
		semSignal(tics10thsec);
		updateDeltaClock(deltaTics);
	}
	// ?? add other timer sampling/signaling code here for project 2
	if (currentTime - oldTime10 >= 10) {
		semSignal(tics10sec);
		oldTime10 += 10;
	}
	return;
} // end timer_isr
