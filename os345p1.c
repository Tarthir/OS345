// os345p1.c - Command Line Processor 01/20/2017
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <setjmp.h>
#include <assert.h>
#include <math.h>
#include "os345.h"
#include "os345signals.h"

// The 'reset_context' comes from 'main' in os345.c.  Proper shut-down
// procedure is to long jump to the 'reset_context' passing in the
// power down code from 'os345.h' that indicates the desired behaviour.

extern jmp_buf reset_context;
extern TCB tcb[];
extern int curTask;

// ***********************************************************************
// project 1 global variables
//
extern long swapCount;					// number of scheduler cycles
extern char inBuffer[];					// character input buffer
extern Semaphore* inBufferReady;		// input buffer ready semaphore
extern bool diskMounted;				// disk has been mounted
extern char dirPath[];					// directory path
Command** commands;						// shell commands


// ***********************************************************************
// project 1 prototypes
Command** P1_init(void);
Command* newCommand(char*, char*, int (*func)(int, char**), char*);

void mySigIntHandler()
{
	sigSignal(-1,mySIGTERM);
}
void mySigContHandler()
{
	printf("Continuing task: %s\n",tcb->name);
}
void mySigKillHandler()
{
	printf("Hellomynameisinigomontoyayoukilledmyfatherpreparetodie");
}
void mySigTermHandler()
{
	killTask(curTask);
}
void mySigTstpHandler()
{
	sigSignal(-1,mySIGSTOP);
}


// ***********************************************************************
// myShell - command line interpreter
//
// Project 1 - implement a Shell (CLI) that:
//
// 1. Prompts the user for a command line.
// 2. WAIT's until a user line has been entered.
// 3. Parses the global char array inBuffer.
// 4. Creates new argc, argv variables using malloc.
// 5. Searches a command list for valid OS commands.
// 6. If found, perform a function variable call passing argc/argv variables.
// 7. Supports background execution of non-intrinsic commands.
//
int P1_shellTask(int argc, char* argv[])
{
	int i = 0, found,background;
	int newArgc;							// # of arguments
	char** newArgv;							// pointers to arguments

	// initialize shell commands
	commands = P1_init();					// init shell commands

	sigAction(mySigIntHandler, mySIGINT);
	sigAction(mySigContHandler,mySIGCONT);
	sigAction(mySigKillHandler,mySIGKILL);
	//sigAction(mySigStopHandler, mySIGSTOP);
	sigAction(mySigTstpHandler, mySIGTSTP);
	sigAction(mySigTermHandler, mySIGTERM);
	while (1)
	{
		background = 0;
		// output prompt
		if (diskMounted) printf("\n%s>>", dirPath);
		else printf("\n%ld>>", swapCount);

		SEM_WAIT(inBufferReady);			// wait for input buffer semaphore
		if (!inBuffer[0]) continue;		// ignore blank lines
		// printf("%s", inBuffer);

		SWAP										// do context switch

		// ?? vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
		// ?? parse command line into argc, argv[] variables
		// ?? must use malloc for argv storage!
		{
			static char *sp, **myArgv;
			char* tok;
			// init arguments
			//newArgc = 1;
			newArgc = 0;
			myArgv = (char**)malloc(sizeof(char*)*MAX_ARGS);
			//myArgv[0] = (char *)malloc(sizeof(char) * 20);
			sp = inBuffer;				// point to input string
			//tok = strtok(sp, " ");
			//strcpy(myArgv[0], sp);
			//sp = myArgv[0];
			if (sp[strlen(sp)-1] =='&') {
				background = 1;
				sp[strlen(sp) - 1] = 0;
			}

			for (i=1; i<MAX_ARGS; i++) myArgv[i] = 0;
			bool qoute = 0,start = 0,done = 0;
			while (!done) {
				char ip[256] = "";
				for (size_t j = start; j < strlen(sp); j++)
				{
					//Right now if you only put on " then its grabbing EVERYTHING after it
					if (sp[j] == '"') {
						if (qoute) {
							qoute = 0;
						}
						else {
							qoute = 1;
						}
					}
					
					if (sp[j] == ' ') {
						if (!qoute) {
							start = j+1;
							break;
						}
					}
					if (sp[j] != '"') {
						int len = strlen(ip);
						ip[len] = sp[j];
						ip[len + 1] = 0;
					}
					if (j == strlen(sp)-1) {
						done = 1;
					}
				}
				int len = strlen(ip);
				if (len > 0) {
					myArgv[newArgc] = (char *)malloc(sizeof(char) *len + 1);
					strcpy(myArgv[newArgc++], ip);
				}
				else {
					done = !done;
				}
			}
			newArgv = myArgv;
			//for (int i = 0; i < newArgc; i++) printf("\n%s", newArgv[i]);
		}
		// ?? ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
		//make commands lowercase
		/*if (newArgv[0] == NULL) {
			free(newArgv);
			return 0;
		}*/
			///TODO we will get an error here if we send in just spaces
		for (size_t i = 0; newArgv[0][i]; i++)
		{
			newArgv[0][i] = tolower(newArgv[0][i]);
		}
		//TODO buf: args dfas "Fds fdsa" aborts for some reason
		//for (int i = 0; i < newArgc; i++) printf("\n%s", newArgv[i]);
		SWAP
		for (found = i = 0; i < NUM_COMMANDS; i++)
		{
			if (!strcmp(newArgv[0], commands[i]->command) ||
				 !strcmp(newArgv[0], commands[i]->shortcut))
			{
				if (background) {
					int tid = createTask(&(*commands[i]->description), &(*commands[i]->func),tcb[0].priority,newArgc,newArgv);
					if (tid < 0) {
						printf("\n%s", "TCB is full");
					}
				}
				else {
					// command found, make implicit call thru function pointer
					int retValue = (*commands[i]->func)(newArgc, newArgv);
					if (retValue) printf("\nCommand Error %d", retValue);
				}
				found = TRUE;
				break;
			}
		}
		SWAP
		if (!found)	printf("\nInvalid command!");

		// ?? free up any malloc'd argv parameters
		for (i=0; i<INBUF_SIZE; i++) inBuffer[i] = 0;
		for (size_t i = 0; i < newArgc; i++)
		{
			free(newArgv[i]);
		}
		free(newArgv);
		SWAP
	}
	return 0;						// terminate task
} // end P1_shellTask


// ***********************************************************************
// ***********************************************************************
// P1 Project
//
#define NUM_ALIVE	3

int P1AliveTask(int argc, char* argv[])
{
	while (1)
	{
		int i;
		printf("\n(%d) ", curTask);
		for (i = 0; i < argc; i++) printf("%s%s", argv[i], (i < argc) ? " " : "");
		for (i = 0; i < 100000; i++) swapTask();
	}
	return 0;						// terminate task
} // end P1AliveTask

int P1_project1(int argc, char* argv[])
{
	int p1_mode = 0;
	// check if just changing P1 mode
	if (argc > 1)
	{
		p1_mode = atoi(argv[1]);
	}
	switch(p1_mode)
	{
		default:
		case 0:
		{
			int j;
			for (j = 0; j < 4; j++)
			{
				int i;
				printf("\nI'm Alive %d,%d", curTask, j);
				for (i = 0; i < 100000; i++) swapTask();
			}
			break;
		}

		case 1:
		{
			int i;
			char buffer[16];
			for (i = 0; i < NUM_ALIVE; i++)
			{
				sprintf_s(buffer, "I'm Alive %d", i);
				createTask(buffer,			// task name
						P1AliveTask,		// task
						LOW_PRIORITY,		// task priority
						argc,				// task argc
						argv);				// task argument pointers
			}
		}
		break;
	}
	return 0;
} // end P1_project1



// ***********************************************************************
// ***********************************************************************
// quit command
//
int P1_quit(int argc, char* argv[])
{
	int i;

	// free P1 commands
	for (i = 0; i < NUM_COMMANDS; i++)
	{
		free(commands[i]->command);
		free(commands[i]->shortcut);
		free(commands[i]->description);
	}
	free(commands);

	// powerdown OS345
	longjmp(reset_context, POWER_DOWN_QUIT);
	return 0;
} // end P1_quit



// **************************************************************************
// **************************************************************************
// lc3 command
//
int P1_lc3(int argc, char* argv[])
{
	strcpy (argv[0], "0");
	return lc3Task(argc, argv);
} // end P1_lc3



// ***********************************************************************
// ***********************************************************************
// help command
//
int P1_help(int argc, char* argv[])
{
	int i;

	// list commands
	for (i = 0; i < NUM_COMMANDS; i++)
	{
		SWAP										// do context switch
		if (strstr(commands[i]->description, ":")) printf("\n");
		printf("\n%4s: %s", commands[i]->shortcut, commands[i]->description);
	}

	return 0;
} // end P1_help

  // ***********************************************************************
  // ***********************************************************************
  // add command
  //
int P1_add(int argc, char* argv[]) {
	double answer = 0;
	for (int i = 1; i < argc; i++) {
		//if its a binary number
		if (argv[i][0] == 'b' && strlen(argv[i]) > 1) {
			double num = 0;
			for (size_t j = 1; j < strlen(argv[i]); j++)
			{
				if (argv[i][j] == '1') {
					num = num * 2 + 1;
				}
				else if( argv[i][j] == '0'){
					num *= 2;
				}
			}
			answer += num;
		}
		//if its octal
		else if (argv[i][0] == 'o' &&  strlen(argv[i]) > 1) {
			double num = 0;
			double exp = strlen(argv[i]) - 2;
			for (size_t j = 1; j < strlen(argv[i]); j++)
			{
				char c = argv[i][j];
				double tmp = c - '0';
				num += (tmp) *pow(8,exp);
				exp--;
			}
			answer += num;
		}
		//if its hex or decimal
		else {
			answer += atof(argv[i]);
		}
	}
	printf("\nAnswer: %f", answer);
	return 0;
}
// ***********************************************************************
// ***********************************************************************
// args command
//
int P1_args(int argc, char* argv[]) {
	printf("\nParsed Arguments:");
		for (size_t i = 0; i < argc; i++)
		{
			printf("|%s| ",argv[i]);
		}
	return 0;
}


// ***********************************************************************
// ***********************************************************************
// initialize shell commands
//
Command* newCommand(char* command, char* shortcut, int (*func)(int, char**), char* description)
{
	Command* cmd = (Command*)malloc(sizeof(Command));

	// get long command
	cmd->command = (char*)malloc(strlen(command) + 1);
	strcpy(cmd->command, command);

	// get shortcut command
	cmd->shortcut = (char*)malloc(strlen(shortcut) + 1);
	strcpy(cmd->shortcut, shortcut);

	// get function pointer
	cmd->func = func;

	// get description
	cmd->description = (char*)malloc(strlen(description) + 1);
	strcpy(cmd->description, description);

	return cmd;
} // end newCommand


Command** P1_init()
{
	int i  = 0;
	Command** commands = (Command**)malloc(sizeof(Command*) * NUM_COMMANDS);

	// system
	commands[i++] = newCommand("quit", "q", P1_quit, "Quit");
	commands[i++] = newCommand("kill", "kt", P2_killTask, "Kill task");
	commands[i++] = newCommand("reset", "rs", P2_reset, "Reset system");

	// P1: Shell
	commands[i++] = newCommand("project1", "p1", P1_project1, "P1: Shell");
	commands[i++] = newCommand("help", "he", P1_help, "OS345 Help");
	commands[i++] = newCommand("lc3", "lc3", P1_lc3, "Execute LC3 program");
	commands[i++] = newCommand("add", "add", P1_add, "Add two numbers");
	commands[i++] = newCommand("args", "args", P1_args, "list all parameters on the command line, numbers or strings");

	// P2: Tasking
	commands[i++] = newCommand("project2", "p2", P2_project2, "P2: Tasking");
	commands[i++] = newCommand("semaphores", "sem", P2_listSems, "List semaphores");
	commands[i++] = newCommand("tasks", "lt", P2_listTasks, "List tasks");
	commands[i++] = newCommand("signal1", "s1", P2_signal1, "Signal sem1 semaphore");
	commands[i++] = newCommand("signal2", "s2", P2_signal2, "Signal sem2 semaphore");

	// P3: Jurassic Park
	commands[i++] = newCommand("project3", "p3", P3_project3, "P3: Jurassic Park");
	commands[i++] = newCommand("deltaclock", "dc", P3_dc, "List deltaclock entries");
	commands[i++] = newCommand("testdeltaclock", "tdc", P3_tdc, "Test List deltaclock entries");

	// P4: Virtual Memory
	commands[i++] = newCommand("project4", "p4", P4_project4, "P4: Virtual Memory");
	commands[i++] = newCommand("frametable", "dft", P4_dumpFrameTable, "Dump bit frame table");
	commands[i++] = newCommand("initmemory", "im", P4_initMemory, "Initialize virtual memory");
	commands[i++] = newCommand("touch", "vma", P4_vmaccess, "Access LC-3 memory location");
	commands[i++] = newCommand("stats", "vms", P4_virtualMemStats, "Output virtual memory stats");
	commands[i++] = newCommand("crawler", "cra", P4_crawler, "Execute crawler.hex");
	commands[i++] = newCommand("memtest", "mem", P4_memtest, "Execute memtest.hex");

	commands[i++] = newCommand("frame", "dfm", P4_dumpFrame, "Dump LC-3 memory frame");
	commands[i++] = newCommand("memory", "dm", P4_dumpLC3Mem, "Dump LC-3 memory");
	commands[i++] = newCommand("page", "dp", P4_dumpPageMemory, "Dump swap page");
	commands[i++] = newCommand("virtual", "dvm", P4_dumpVirtualMem, "Dump virtual memory page");
	commands[i++] = newCommand("root", "rpt", P4_rootPageTable, "Display root page table");
	commands[i++] = newCommand("user", "upt", P4_userPageTable, "Display user page table");

	// P5: Scheduling
	commands[i++] = newCommand("project5", "p5", P5_project5, "P5: Scheduling");

	// P6: FAT
	commands[i++] = newCommand("project6", "p6", P6_project6, "P6: FAT");
	commands[i++] = newCommand("change", "cd", P6_cd, "Change directory");
	commands[i++] = newCommand("copy", "cf", P6_copy, "Copy file");
	commands[i++] = newCommand("define", "df", P6_define, "Define file");
	commands[i++] = newCommand("delete", "dl", P6_del, "Delete file");
	commands[i++] = newCommand("directory", "dir", P6_dir, "List current directory");
	commands[i++] = newCommand("mount", "md", P6_mount, "Mount disk");
	commands[i++] = newCommand("mkdir", "mk", P6_mkdir, "Create directory");
	commands[i++] = newCommand("run", "run", P6_run, "Execute LC-3 program");
	commands[i++] = newCommand("space", "sp", P6_space, "Space on disk");
	commands[i++] = newCommand("type", "ty", P6_type, "Type file");
	commands[i++] = newCommand("unmount", "um", P6_unmount, "Unmount disk");

	commands[i++] = newCommand("fat", "ft", P6_dfat, "Display fat table");
	commands[i++] = newCommand("fileslots", "fs", P6_fileSlots, "Display current open slots");
	commands[i++] = newCommand("sector", "ds", P6_dumpSector, "Display disk sector");
	commands[i++] = newCommand("chkdsk", "ck", P6_chkdsk, "Check disk");
	commands[i++] = newCommand("final", "ft", P6_finalTest, "Execute file test");

	commands[i++] = newCommand("open", "op", P6_open, "Open file test");
	commands[i++] = newCommand("read", "rd", P6_read, "Read file test");
	commands[i++] = newCommand("write", "wr", P6_write, "Write file test");
	commands[i++] = newCommand("seek", "sk", P6_seek, "Seek file test");
	commands[i++] = newCommand("close", "cl", P6_close, "Close file test");

	assert(i == NUM_COMMANDS);

	return commands;

} // end P1_init
