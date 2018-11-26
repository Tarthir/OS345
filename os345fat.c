// os345fat.c - file management system	2017-06-28
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
//		11/19/2011	moved getNextDirEntry to P6
//
// ***********************************************************************
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <setjmp.h>
#include <assert.h>
#include "os345.h"
#include "os345fat.h"

// ***********************************************************************
// ***********************************************************************
//	functions to implement in Project 6
//
int fmsCloseFile(int);
int fmsDefineFile(char*, int);
int fmsDeleteFile(char*);
int fmsOpenFile(char*, int);
int fmsReadFile(int, char*, int);
int fmsSeekFile(int, int);
int fmsWriteFile(int, char*, int);

// ***********************************************************************
// ***********************************************************************
//	Support functions available in os345p6.c
//
extern int fmsGetDirEntry(char* fileName, DirEntry* dirEntry);
extern int fmsGetNextDirEntry(int *dirNum, char* mask, DirEntry* dirEntry, int dir);

extern int fmsMount(char* fileName, void* ramDisk);

extern void setFatEntry(int FATindex, unsigned short FAT12ClusEntryVal, unsigned char* FAT);
extern unsigned short getFatEntry(int FATindex, unsigned char* FATtable);

extern int fmsMask(char* mask, char* name, char* ext);
extern void setDirTimeDate(DirEntry* dir);
extern int isValidFileName(char* fileName);
extern void printDirectoryEntry(DirEntry*);
extern void fmsError(int);

extern int fmsReadSector(void* buffer, int sectorNumber);
extern int fmsWriteSector(void* buffer, int sectorNumber);

// ***********************************************************************
// ***********************************************************************
// fms variables
//
// RAM disk
unsigned char RAMDisk[SECTORS_PER_DISK * BYTES_PER_SECTOR];

// File Allocation Tables (FAT1 & FAT2)
#define END_OF_FAT NUM_FAT_SECTORS * BYTES_PER_SECTOR
unsigned char FAT1[NUM_FAT_SECTORS * BYTES_PER_SECTOR];
unsigned char FAT2[NUM_FAT_SECTORS * BYTES_PER_SECTOR];

char dirPath[128];							// current directory path
FCB OFTable[NFILES];						// open file table

extern bool diskMounted;					// disk has been mounted
extern TCB tcb[];							// task control block
extern int curTask;							// current task #


// ***********************************************************************
// ***********************************************************************
// This function closes the open file specified by fileDescriptor.
// The fileDescriptor was returned by fmsOpenFile and is an index into the open file table.
//	Return 0 for success, otherwise, return the error number.
//
int fmsCloseFile(int fileDescriptor)
{
	// ?? add code here
	if (OFTable[fileDescriptor].name[0] == 0) {
		return ERR63;//the file is not there
	}
	OFTable[fileDescriptor].name[0] = 0;// get rid of name
	
	return 0;
} // end fmsCloseFile



// ***********************************************************************
// ***********************************************************************
// If attribute=DIRECTORY, this function creates a new directory
// file directoryName in the current directory.
// The directory entries "." and ".." are also defined.
// It is an error to try and create a directory that already exists.
//
// else, this function creates a new file fileName in the current directory.
// It is an error to try and create a file that already exists.
// The start cluster field should be initialized to cluster 0.  In FAT-12,
// files of size 0 should point to cluster 0 (otherwise chkdsk should report an error).
// Remember to change the start cluster field from 0 to a free cluster when writing to the
// file.
//
// Return 0 for success, otherwise, return the error number.
//
int fmsDefineFile(char* fileName, int attribute)
{
	// ?? add code here
	printf("\nfmsDefineFile Not Implemented");


	return ERR72;
} // end fmsDefineFile



// ***********************************************************************
// ***********************************************************************
// This function deletes the file fileName from the current director.
// The file name should be marked with an "E5" as the first character and the chained
// clusters in FAT 1 reallocated (cleared to 0).
// Return 0 for success; otherwise, return the error number.
//
int fmsDeleteFile(char* fileName)
{
	// ?? add code here
	printf("\nfmsDeleteFile Not Implemented");

	return ERR61;
} // end fmsDeleteFile



// ***********************************************************************
// ***********************************************************************
// This function opens the file fileName for access as specified by rwMode.
// It is an error to try to open a file that does not exist.
// The open mode rwMode is defined as follows:
//    0 - Read access only.
//       The file pointer is initialized to the beginning of the file.
//       Writing to this file is not allowed.
//    1 - Write access only.
//       The file pointer is initialized to the beginning of the file.
//       Reading from this file is not allowed.
//    2 - Append access.
//       The file pointer is moved to the end of the file.
//       Reading from this file is not allowed.
//    3 - Read/Write access.
//       The file pointer is initialized to the beginning of the file.
//       Both read and writing to the file is allowed.
// A maximum of 32 files may be open at any one time.
// If successful, return a file descriptor that is used in calling subsequent file
// handling functions; otherwise, return the error number.
//


//H20.C is at cluster 1380, so its at sector 1411
//Look in directory, directory will point to first cluster
//A directory with 16 entries would not have 0x00 end of directory byte
//eod byte in first spot

//Call fmsGetNextDirEntry with the filename as the mask, will return the directory you are looking for
//can also be used to rename a directory/file
//The pattern found in it can also be used to close a file/write it back out to disk
#define NAMELEN 11
int fmsOpenFile(char* fileName, int rwMode)
{
	FCB *fcb;
	// ?? add code here
	DirEntry *entry = (DirEntry*)malloc(sizeof(DirEntry));
	int dirNum = 0;
	if (fmsGetNextDirEntry(&dirNum, fileName, entry, CDIR) < 0) {
		return ERR61; // did not find it
	}
	
	for (int i = 0; i < NFILES; i++)
	{
		if (OFTable[i].name[0] == 0) {
			fcb = (FCB*)malloc(sizeof(FCB));
			fcb->pid = curTask;
			//copy into FCB and put in OFTABLE
			memcpy(fcb->name,entry->name, strlen(entry->name));
			memcpy(fcb->extension, entry->extension, strlen(entry->extension));
			fcb->attributes = entry->attributes;
			fcb->startCluster = entry->startCluster;
			fcb->fileSize = entry->fileSize;
			fcb->fileIndex = 0;
			fcb->currentCluster = 0;
			fcb->directoryCluster = CDIR;
			fcb->mode = rwMode;
			fcb->flags = 0;
			fcb->startCluster = entry->startCluster;
			free(entry);
			OFTable[i] = *fcb;
			free(fcb);
			return i;
		}
	}
	return ERR65;
} // end fmsOpenFile



// ***********************************************************************
// ***********************************************************************
// This function reads nBytes bytes from the open file specified by fileDescriptor into
// memory pointed to by buffer.
// The fileDescriptor was returned by fmsOpenFile and is an index into the open file table.
// After each read, the file pointer is advanced.
// Return the number of bytes successfully read (if > 0) or return an error number.
// (If you are already at the end of the file, return EOF error.  ie. you should never
// return a 0.)
//
int fmsReadFile(int fileDescriptor, char* buffer, int nBytes)
{
	// ?? add code here
	int error, nextCluster;
	FCB *fcb;
	int numBytesRead = 0;
	unsigned int bytesLeft, bufferIndex;
	fcb = &OFTable[fileDescriptor];
	if (fcb->name[0] == 0) {
		return ERR63;
	}
	if ((fcb->mode == 1) || (fcb->mode == 2)) {
		return ERR85;
	}
	while (nBytes > 0) {
		if (fcb->fileSize == fcb->fileIndex) {//if at end of file
			//we will give all the bytes wanted, if called again we give EOF
			return (numBytesRead ? numBytesRead : ERR66);
		}
		bufferIndex = fcb->fileIndex % BYTES_PER_SECTOR;
		if ((bufferIndex == 0) && (fcb->fileIndex || !fcb->currentCluster)) {
			if (fcb->currentCluster == 0) {//if we havnt read anything yet
				if (fcb->startCluster == 0) {
					return ERR66;
				}
				nextCluster = fcb->startCluster;
				fcb->fileIndex = 0;

			}
			else {
				nextCluster = getFatEntry(fcb->currentCluster,FAT1);
				if (nextCluster == FAT_EOC) {
					return numBytesRead;
				}
			}
			if (fcb->flags & BUFFER_ALTERED) { //if dirty bit is set
				if ((error = fmsWriteSector(fcb->buffer, C_2_S(fcb->currentCluster)))) {
					return error;
				}
				fcb->flags &= ~BUFFER_ALTERED;
			}
			if ((error = fmsReadSector(fcb->buffer,C_2_S(nextCluster)))) {
				return error;
			}
			fcb->currentCluster = nextCluster;
		}
		bytesLeft = BYTES_PER_SECTOR - bufferIndex;
		if (bytesLeft > nBytes) {
			bytesLeft = nBytes;
		}
		if (bytesLeft > (fcb->fileSize - fcb->fileIndex)) {
			bytesLeft = fcb->fileSize - fcb->fileIndex;
		}

		memcpy(buffer,&fcb->buffer[bufferIndex],bytesLeft);
		fcb->fileIndex += bytesLeft;
		numBytesRead += bytesLeft;
		buffer += bytesLeft;
		nBytes -= bytesLeft;
	}
	
	return numBytesRead;
} // end fmsReadFile



// ***********************************************************************
// ***********************************************************************
// This function changes the current file pointer of the open file specified by
// fileDescriptor to the new file position specified by index.
// The fileDescriptor was returned by fmsOpenFile and is an index into the open file table.
// The file position may not be positioned beyond the end of the file.
// Return the new position in the file if successful; otherwise, return the error number.
//
int fmsSeekFile(int fileDescriptor, int index)
{
	// ?? add code here
	printf("\nfmsSeekFile Not Implemented");

	return ERR63;
} // end fmsSeekFile



// ***********************************************************************
// ***********************************************************************
// This function writes nBytes bytes to the open file specified by fileDescriptor from
// memory pointed to by buffer.
// The fileDescriptor was returned by fmsOpenFile and is an index into the open file table.
// Writing is always "overwriting" not "inserting" in the file and always writes forward
// from the current file pointer position.
// Return the number of bytes successfully written; otherwise, return the error number.
//
int fmsWriteFile(int fileDescriptor, char* buffer, int nBytes)
{
	// ?? add code here
	int error, nextCluster;
	FCB *fcb;
	int numBytesWritten = 0;
	unsigned int bytesLeft, bufferIndex;
	fcb = &OFTable[fileDescriptor];
	if (fcb->name[0] == 0) {
		return ERR63;
	}
	if ((fcb->mode == 0)) { //if read mode
		return ERR85;
	}
	while (nBytes > 0) {
		if (fcb->mode == 2) { //if append
			fcb->fileIndex = fcb->fileSize;//go to end
		}

		if (fcb->fileIndex == BYTES_PER_SECTOR) {//if at end of file
			return (numBytesWritten ? numBytesWritten : ERR66);
		}
		
		bufferIndex = fcb->fileIndex % BYTES_PER_SECTOR;
		if ((bufferIndex >= 0) && (fcb->fileIndex || !fcb->currentCluster)) {
			if (fcb->currentCluster == 0) {//if we havnt read anything yet
				if (fcb->startCluster == 0) {
					return ERR66;
				}
				nextCluster = fcb->startCluster;
				fcb->fileIndex = 0;

			}
			else {
				nextCluster = getFatEntry(fcb->currentCluster, FAT1);
				if (nextCluster == FAT_EOC) {
					return numBytesWritten;
				}
			}
			//if (fcb->flags & BUFFER_ALTERED) {
				if ((error = fmsWriteSector(buffer, C_2_S(fcb->currentCluster)))) {
					return error;
			}
				fcb->flags &= BUFFER_ALTERED; //set dirty bit
			//}
			fcb->currentCluster = nextCluster;
		}
		bytesLeft = BYTES_PER_SECTOR - bufferIndex;
		/*if (bytesLeft > nBytes) {
			bytesLeft = nBytes;
		}
		if (bytesLeft > (fcb->fileSize - fcb->fileIndex)) {
			bytesLeft = fcb->fileSize - fcb->fileIndex;
		}*/

		//memcpy(buffer, &fcb->buffer[bufferIndex], bytesLeft);
		fcb->fileIndex += bytesLeft;
		numBytesWritten += bytesLeft;
		buffer += bytesLeft;
		nBytes -= bytesLeft;
	}

	return numBytesWritten;
} // end fmsWriteFile
