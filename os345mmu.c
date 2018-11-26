// os345mmu.c - LC-3 Memory Management Unit	03/05/2017
//
//		03/12/2015	added PAGE_GET_SIZE to accessPage()
//
// **************************************************************************
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
#include "os345.h"
#include "os345lc3.h"

// ***********************************************************************
// mmu variables

// LC-3 memory
unsigned short int memory[LC3_MAX_MEMORY];

// statistics
int memAccess;						// memory accesses
int memHits;						// memory hits
int memPageFaults;					// memory faults
int clockRPT;						// RPT clock
int clockUPT;						// UPT clock


int nextPage = 0;						// swap page size
int pageReads = 0;						// page reads
int pageWrites = 0;						// page writes
int getFrame(int);
int getAvailableFrame(void);
extern TCB tcb[];					// task control block
extern int curTask;					// current task #
int outerClock;
int innerClock;

int getFrameFromClock(int notme);

void swap_page(int idx);

int getFrame(int notme)
{
	int frame;
	frame = getAvailableFrame();
	if (frame >=0) return frame;

	// run clock
	//printf("\nWe're toast!!!!!!!!!!!!");
	//frame = getFrameFromClock(notme);

	return frame;
}
// **************************************************************************
// **************************************************************************
// LC3 Memory Management Unit
// Virtual Memory Process
// **************************************************************************
//           ___________________________________Frame defined
//          / __________________________________Dirty frame
//         / / _________________________________Referenced frame
//        / / / ________________________________Pinned in memory
//       / / / /     ___________________________
//      / / / /     /                 __________frame # (0-1023) (2^10)
//     / / / /     /                 / _________page defined
//    / / / /     /                 / /       __page # (0-4096) (2^12)
//   / / / /     /                 / /       /
//  / / / /     / 	             / /       /
// F D R P - - f f|f f f f f f f f|S - - - p p p p|p p p p p p p p

#define MMU_ENABLE	1
//getFrameFromClock()
//swapFrame()

int getFrameFromClock(int notme) {
	int myFrame;
	int max = 20;
	//outerClock = LC3_RPT;

	for (;max; outerClock += 2, innerClock = 0) {
		int i, rpte, upta, upte;

		if (outerClock >= LC3_RPT_END) {
			outerClock = LC3_RPT;//start at 0x2400
			max--;
		}
		rpte = memory[outerClock];

		if (DEFINED(rpte) && REFERENCED(rpte)) { //clear ref bit
			MEMWORD(outerClock) = rpte = CLEAR_REF(rpte);
		}
		else if (DEFINED(rpte)) {//if ref bit not set, lets get rid of this table and its associated user page tables
			upta = (FRAME(rpte) << 6);
			
			//size of frame is 64
			for (i = innerClock % 64; i < 64; i+=2, innerClock = i % 64) {
				upte = memory[upta + i];//user page table entry
				if (FRAME(upte) == notme || PINNED(upte)) {//if notme or in memory
					continue;
				}
				if ((DEFINED(upte) && REFERENCED(upte))) { //fine, clear refs and keep goin
					MEMWORD(outerClock) = rpte = SET_PINNED(rpte);
					MEMWORD(upta + i) = upte = CLEAR_REF(upte);
				}
				else if (DEFINED(upte)) {//not pinned, swap page to disk if dirty
					MEMWORD(outerClock) = rpte = SET_DIRTY(rpte);
					myFrame = FRAME(upte);
					swap_page(upta+i);
					innerClock += 2;
					return myFrame;
				}
			}
		}


		innerClock = 0;
		if (FRAME(rpte) != notme && !REFERENCED(rpte) && !PINNED(rpte) ) {
			myFrame = FRAME(rpte);
			swap_page(outerClock);
			outerClock += 2;
			return myFrame;
		}
		else {
			MEMWORD(outerClock) = rpte = CLEAR_PINNED(rpte);
		}
	}
	return -1;
}

void swap_page(int idx) {
	int e1 = memory[idx], e2 = memory[idx + 1];

	if (DIRTY(e1) && PAGED(e2)) {
		accessPage(SWAPPAGE(e2),FRAME(e1),PAGE_OLD_WRITE);
	}
	else if (!PAGED(e2)) {
		MEMWORD(idx+1) = e2 = SET_PAGED(nextPage);
		accessPage(nextPage, FRAME(e1), PAGE_NEW_WRITE);
	}
	memory[idx] = 0;
}

unsigned short int *getMemAdr(int va, int rwFlg)
{
	unsigned short int pa;
	int rpta, rpte, rpte2;
	int upta, upte, upte2;
	int rptFrame, uptFrame;
	memAccess += 2;
	// turn off virtual addressing for system RAM
	if (va < 0x3000) return &memory[va];
#if MMU_ENABLE
	rpta = tcb[curTask].RPT + RPTI(va);		// root page table address
	rpte = memory[rpta];					// FDRP__ffffffffff
	rpte2 = memory[rpta+1];					// S___pppppppppppp

	if (DEFINED(rpte))	{ // rpte defined
		memHits++;
	}					
	else{ // rpte undefined
		memPageFaults++;
		rptFrame = getFrame(-1);
		rpte = SET_DEFINED(rptFrame);
		if (PAGED(rpte2)) { //UPT frame paged out read from rpte2 into frame
			accessPage(SWAPPAGE(rpte2), rptFrame, PAGE_READ);
		}
		else { //define new frame and refernce from rpt
			memset(&memory[(rptFrame << 6)], 0, 128);
		}
	}					
	memory[rpta] = SET_REF(rpte);			// set rpt frame access bit
	memory[rpta + 1] = rpte2;
	upta = (FRAME(rpte)<<6) + UPTI(va);	// user page table address
	upte = memory[upta]; 					// FDRP__ffffffffff
	upte2 = memory[upta+1]; 				// S___pppppppppppp
	if (DEFINED(upte))	{ //if mem hit
		memHits++;
	}					// upte defined
	else//not mem hit			
	{ 
		memPageFaults++;
		uptFrame = getFrame(FRAME(memory[rpta]));
		memory[rpta] = rpte = SET_REF(SET_DIRTY(rpte));
		upte = SET_DEFINED(uptFrame);
		//do more
		if (PAGED(upte2)) {
			accessPage(SWAPPAGE(upte2), uptFrame, PAGE_READ);
		}
	}// upte undefined
	if (rwFlg) {
		upte = SET_DIRTY(upte);
	}

	memory[upta] = SET_REF(upte); 			// set upt frame access bit
	memory[upta + 1] = upte2;

	return &memory[(FRAME(upte)<<6) + FRAMEOFFSET(va)];
#else
	//pa = va;
	return &memory[va];
	
#endif
	
} // end getMemAdr


// **************************************************************************
// **************************************************************************
// set frames available from sf to ef
//    flg = 0 -> clear all others
//        = 1 -> just add bits
//
void setFrameTableBits(int flg, int sf, int ef)
{	int i, data;
	int adr = LC3_FBT-1;             // index to frame bit table
	int fmask = 0x0001;              // bit mask

	// 1024 frames in LC-3 memory
	for (i=0; i<LC3_FRAMES; i++)
	{	if (fmask & 0x0001)
		{  fmask = 0x8000;
			adr++;
			data = (flg)?MEMWORD(adr):0;
		}
		else fmask = fmask >> 1;
		// allocate frame if in range
		if ( (i >= sf) && (i < ef)) data = data | fmask;
		MEMWORD(adr) = data;
	}
	return;
} // end setFrameTableBits


// **************************************************************************
// get frame from frame bit table (else return -1)
int getAvailableFrame()
{
	int i, data;
	int adr = LC3_FBT - 1;				// index to frame bit table
	int fmask = 0x0001;					// bit mask

	for (i=0; i<LC3_FRAMES; i++)		// look thru all frames
	{	if (fmask & 0x0001)
		{  fmask = 0x8000;				// move to next work
			adr++;
			data = MEMWORD(adr);
		}
		else fmask = fmask >> 1;		// next frame
		// deallocate frame and return frame #
		if (data & fmask)
		{  MEMWORD(adr) = data & ~fmask;
			return i;
		}
	}
	return -1;
} // end getAvailableFrame



// **************************************************************************
// read/write to swap space
int accessPage(int pnum, int frame, int rwnFlg)
{
	
	static unsigned short int swapMemory[LC3_MAX_SWAP_MEMORY];

	if ((nextPage >= LC3_MAX_PAGE) || (pnum >= LC3_MAX_PAGE))
	{
		printf("\nVirtual Memory Space Exceeded!  (%d)", LC3_MAX_PAGE);
		exit(-4);
	}
	switch(rwnFlg)
	{
		case PAGE_INIT:                    		// init paging
			clockRPT = 0;						// clear RPT clock
			clockUPT = 0;						// clear UPT clock
			//memAccess = 0;						// memory accesses
			//memHits = 0;						// memory hits
			//memPageFaults = 0;					// memory faults
			//nextPage = 0;						// disk swap space size
			//pageReads = 0;						// disk page reads
			//pageWrites = 0;						// disk page writes
			return 0;

		case PAGE_GET_SIZE:                    	// return swap size
			return nextPage;

		case PAGE_GET_READS:                   	// return swap reads
			return pageReads;

		case PAGE_GET_WRITES:                    // return swap writes
			return pageWrites;

		case PAGE_GET_ADR:                    	// return page address
			return (int)(&swapMemory[pnum<<6]);

		case PAGE_NEW_WRITE:                   // new write (Drops thru to write old)
			pnum = nextPage++;

		case PAGE_OLD_WRITE:                   // write
			//printf("\n    (%d) Write frame %d (memory[%04x]) to page %d", p.PID, frame, frame<<6, pnum);
			memcpy(&swapMemory[pnum<<6], &memory[frame<<6], 1<<7);
			pageWrites++;
			return pnum;

		case PAGE_READ:                    	// read
			//printf("\n    (%d) Read page %d into frame %d (memory[%04x])", p.PID, pnum, frame, frame<<6);
			memcpy(&memory[frame<<6], &swapMemory[pnum<<6], 1<<7);
			pageReads++;
			return pnum;

		case PAGE_FREE:                   // free page
			printf("\nPAGE_FREE not implemented");
			break;
   }
   return pnum;
} // end accessPage
