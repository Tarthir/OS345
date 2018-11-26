// os345p3.c - Jurassic Park
// ***********************************************************************
// **   DISCLAMER ** DISCLAMER ** DISCLAMER ** DISCLAMER ** DISCLAMER   **
// **                                                                   **
// ** The code given here is the basis for the CS345 projects.          **
// ** It comes "as is" and "unwarranted."  As such, when you use part   **
// ** or all of the code, it becomes "yours" and you are responsible to **
// ** understand any alrithm or method presented.  Likewise, any      **
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
#include "os345park.h"
#include "os345p3.h"

// ***********************************************************************
// project 3 variables

// Jurassic Park
extern JPARK myPark;
extern Semaphore* parkMutex;						// protect park access
extern Semaphore* fillSeat[NUM_CARS];			// (signal) seat ready to fill
extern Semaphore* seatFilled[NUM_CARS];		// (wait) passenger seated
extern Semaphore* rideOver[NUM_CARS];			// (signal) ride over
extern TCB tcb[];
extern int deltaTics;
Delta* deltaClock[MAX_DELTAS];
int deltaCount;
int my_driverID;

//counting
Semaphore *tickets;
Semaphore *roomPark;
Semaphore *roomGiftShop;
Semaphore *roomMuseum;
Semaphore *wakeupDriver;
//binary
Semaphore *needTicket;
Semaphore *getTicket;
Semaphore *buyTicket;
Semaphore *needDriver;
Semaphore *deltaclockChange;
Semaphore *deltaclockMutex;
Semaphore *getPassenger;
Semaphore *seatTaken;
Semaphore *needDriverMutex;
Semaphore *passengerSeated;
Semaphore *driverDoneSig;
Semaphore *needpassengerWait;
Semaphore *passengerReady;
Semaphore *passengerAcquired;
Semaphore *mySem;// handles hand off of passengers to cars
Semaphore *driverReady;
Semaphore *gotADriver;
Semaphore *getTicketMutex;
Semaphore *driverAndPassMutex;
Semaphore *needDriverSemaphore;
Semaphore *driverReadyMutex;

Semaphore *needCashier;
Semaphore *getCashier;
Semaphore *buyGifts;
Semaphore *doMaintenance;
Semaphore *getCashierMutex;
Semaphore *boughtGifts;

int timeTaskID;
// ***********************************************************************
// project 3 functions and tasks
void CL3_project3(int, char**);
void CL3_dc(int, char**);
void carTask(int argc, char*argv[]);
int driverTask(int argc, char*argv[]);
int dcMonitorTask(int argc, char* argv[]);
void printDeltaClock(void);
int timeTask(int argc, char* argv[]);
int visitorTask(int argc, char*argv[]);
int P3_dc(int argc, char* argv[]);
Semaphore *getPassengerWait();
Semaphore * getDriverWait(int carID);
int updateDriverPos(int driverID, int carPos);
int driverTask(int argc, char*argv[]);
void passOffDriver(driverDoneSig, driverID);
void addToPark();
void randomize_sem(int max, Semaphore *sem);
void getInTicketLine();
void waitForTicket(Semaphore *visitorWait);
void waitForMuseum(Semaphore *visitorWait);
void InMuseumLine();
void InTourLine();
void waitForTour(Semaphore *visitorWait);
void ToGiftLine();
void waitForGiftShop(Semaphore *visitorWait);
void leavePark();

int insertDeltaClock(int time, Semaphore * sem);

// ***********************************************************************
// ***********************************************************************
// project3 command
int P3_project3(int argc, char* argv[])
{
	char buf[32];
	char* newArgv[2];
	char idbuf[32];

	// start park
	sprintf(buf, "jurassicPark");
	newArgv[0] = buf;
	createTask( buf,				// task name
		jurassicTask,				// task
		MED_PRIORITY,				// task priority
		1,								// task count
		newArgv);					// task argument

	// wait for park to get initialized...
	while (!parkMutex) SWAP;
	printf("\nStart Jurassic Park...");
	//counting
	tickets = createSemaphore("tickets",COUNTING,MAX_TICKETS);
	wakeupDriver = createSemaphore("wakeupDriver", COUNTING,NUM_DRIVERS);
	roomGiftShop = createSemaphore("roomGiftShop", COUNTING, MAX_IN_GIFTSHOP);
	roomMuseum = createSemaphore("roomMuseum", COUNTING, MAX_IN_MUSEUM);
	roomPark = createSemaphore("roomPark", COUNTING, MAX_IN_PARK);
	
	//start at 1
	deltaclockChange = createSemaphore("deltaclockChange", BINARY, 1);
	deltaclockMutex = createSemaphore("deltaclockMutex", BINARY, 1);
	getTicketMutex = createSemaphore("getTicketMutex", BINARY, 1);
	getCashierMutex = createSemaphore("getCashierMutex", BINARY, 1);
	needDriverMutex = createSemaphore("needDriverMutex",BINARY,1);
	driverAndPassMutex = createSemaphore("resourceMutex",BINARY,1);
	doMaintenance = createSemaphore("doMaintenance", BINARY, 1);//one driver will do maintence when doing nothing else
	//start at 0
	needCashier = createSemaphore("needCashier", BINARY, 0);
	getTicket = createSemaphore("getTicket", BINARY, 0);
	needTicket = createSemaphore("needTicket", BINARY, 0);
	buyTicket = createSemaphore("buyTicket", BINARY, 0);
	needDriver = createSemaphore("needDriver", BINARY, 0);
	getPassenger = createSemaphore("getPassenger", BINARY, 0);
	seatTaken = createSemaphore("seatTaken", BINARY, 0);
	passengerSeated = createSemaphore("passengerSeated", BINARY, 0);
	driverDoneSig = createSemaphore("driverDoneSig", BINARY, 0);
	needpassengerWait = createSemaphore("needpassengerWait", BINARY, 0);
	passengerReady = createSemaphore("passengerReady", BINARY, 0);
	passengerAcquired = createSemaphore("passengerAcquired", BINARY, 0);
	driverReady = createSemaphore("driverReady", BINARY, 0);
	gotADriver = createSemaphore("gotADriver", BINARY, 0);
	needDriverSemaphore = createSemaphore("needDriverSemaphore", BINARY, 0);
	driverReadyMutex = createSemaphore("driverReadyMutex", BINARY, 0);
	getCashier = createSemaphore("getCashier", BINARY, 0);
	buyGifts = createSemaphore("buyGifts", BINARY, 0);
	boughtGifts = createSemaphore("boughtGifts", BINARY, 0);
	
	//?? create car, driver, and visitor tasks here
	for (size_t i = 0; i < NUM_CARS; i++)
	{
		sprintf(buf, "carid%d", i);	SWAP;
		sprintf(idbuf,"%d",i);	SWAP;
		newArgv[0] = buf;
		newArgv[1] = idbuf;
		createTask(buf, carTask, MED_PRIORITY, 2, newArgv);	SWAP;
	}
	for (size_t i = 0; i < NUM_DRIVERS; i++)
	{
		sprintf(buf, "driver%d", i);	SWAP;
		sprintf(idbuf, "%d", i);	SWAP;
		newArgv[0] = buf;
		newArgv[1] = idbuf;
		createTask(buf, driverTask, MED_PRIORITY, 2, newArgv);	SWAP;
	}
	for (size_t i = 0; i < NUM_VISITORS; i++)
	{
		sprintf(buf, "visitor%d", i);	SWAP;
		sprintf(idbuf, "%d", i);	SWAP;
		newArgv[0] = buf;
		newArgv[1] = idbuf;
		createTask(buf, visitorTask, MED_PRIORITY, 2, newArgv);	SWAP;
	}
	return 0;
} // end project3

int visitorTask(int argc, char*argv[]) {
	char buf[32];
	int visitorID = atoi(argv[1]);
	sprintf(buf,"%swait",argv[0]);
	Semaphore *visitorWait = createSemaphore(buf,BINARY,0); SWAP;
	{
		//get to park
		randomize_sem(100, visitorWait); SWAP;
		//get in line
		//printf("\naddToPark:%d", visitorID);
		addToPark(); SWAP;
		//wait to get in
		SEM_WAIT(roomPark)SWAP;
		//get in ticket line
		//printf("\ngetInTicketLine:%d", visitorID);
		getInTicketLine(); SWAP;
		//wait for ticket
		//printf("\ngetTicket:%d", visitorID);
		waitForTicket(visitorWait); SWAP;
		//printf("\ng0tTicket:%d", visitorID);
		//get in museum line
		//printf("\ngetInMuseumLine:%d", visitorID);
		InMuseumLine(); SWAP;
		//wait for museum
		//printf("\nDoMuseumIfRoom:%d", visitorID);
		waitForMuseum(visitorWait); SWAP;
		//printf("\nMuseumDone:%d", visitorID);
		//go to tour
		//printf("\nget in Tour line:%d", visitorID);
		InTourLine(); SWAP;
		//wait for tour
		//printf("\nDo Tour:%d", visitorID);
		waitForTour(visitorWait); SWAP;
		//printf("\nTour Over:%d", visitorID);
		//got to gift shop line
		//printf("\nget in gift line:%d", visitorID);
		ToGiftLine(); SWAP
		//printf("\nget in gift shop:%d", visitorID);
		//wait for gift shop
		waitForGiftShop(visitorWait); SWAP;
		//printf("\nleave gift shop:%d", visitorID);
		//leave park
		leavePark(); SWAP;
		//printf("\nBYE!:%d", visitorID);
		SEM_SIGNAL(roomPark)SWAP;
	}
	return 0;
}

void addToPark() {
	SEM_WAIT(parkMutex); SWAP;
	myPark.numOutsidePark++; SWAP;
	//printf("\naddToPark SignalparkMutex");
	SEM_SIGNAL(parkMutex); SWAP;
}
void getInTicketLine() {
	SEM_WAIT(parkMutex); SWAP;
	myPark.numOutsidePark--; SWAP;
	myPark.numInTicketLine++; SWAP;
	myPark.numInPark++; SWAP;
	//printf("\ngetInTicketLine SignalparkMutex");
	SEM_SIGNAL(parkMutex); SWAP;
}
void waitForTicket(Semaphore *visitorWait) {
	randomize_sem(30, visitorWait); SWAP;//time waiting in line for ticket
	SEM_WAIT(getTicketMutex); SWAP;
	{
		//printf("\nwaitForTicket SignalneedTicket");
		SEM_SIGNAL(needTicket); SWAP; //say you need a ticket
		//printf("\nwaitForTicket SignalwakeupDriver");
		SEM_SIGNAL(wakeupDriver); SWAP; //get a driver to sell you one
		SEM_WAIT(getTicket); SWAP;
		//printf("\nwaitForTicket SignalbuyTicket");
		SEM_SIGNAL(buyTicket); SWAP; //buy it
		SEM_WAIT(parkMutex)  SWAP;
		{
			myPark.numTicketsAvailable--; SWAP;
		}
		//printf("\nwaitForTicket SignalparkMutex");
		SEM_SIGNAL(parkMutex); SWAP;
	}
	//printf("\nwaitForTicket SignalgetTicketMutex");
	SEM_SIGNAL(getTicketMutex); SWAP;
}

void InMuseumLine() {
	SEM_WAIT(parkMutex); SWAP;
	{
		myPark.numInTicketLine--; SWAP;
		myPark.numInMuseumLine++; SWAP;
	}
	//printf("\ngoInMuseumLine SignalparkMutex");
	SEM_SIGNAL(parkMutex); SWAP;
}

void waitForMuseum(Semaphore *visitorWait) {
	randomize_sem(30, visitorWait); SWAP;//time spent in line for museum
	SEM_WAIT(roomMuseum); SWAP;
	{
		SEM_WAIT(parkMutex)SWAP;
		{
			myPark.numInMuseumLine--; SWAP;
			myPark.numInMuseum++; SWAP;
		}
		//printf("\nwaitForMuseum SignalparkMutex");
		SEM_SIGNAL(parkMutex); SWAP;
		randomize_sem(30,visitorWait);//time spent in museum
	}
	//printf("\nwaitForMuseum SignalroomMuseum");
	SEM_SIGNAL(roomMuseum); SWAP;
}

void InTourLine() {
	SEM_WAIT(parkMutex); SWAP;
	{
		myPark.numInMuseum--; SWAP;
		myPark.numInCarLine++; SWAP;
	}
	//printf("\ngoInTourLine SignalparkMutex");
	SEM_SIGNAL(parkMutex); SWAP;
}

//lets get in a car!
void waitForTour(Semaphore *visitorWait) {
	randomize_sem(30, visitorWait); SWAP;//randomize time in tour line
	SEM_WAIT(getPassenger); SWAP; //once a car has an open spot take it ///getting STUCK HERE
	SEM_WAIT(parkMutex); SWAP;
	{
		myPark.numInCars++; SWAP;
		myPark.numInCarLine--; SWAP;
		myPark.numTicketsAvailable++; SWAP;
		//printf("\nwaitForTour SignalticketsVisitor");
		SEM_SIGNAL(tickets); SWAP; //signal that you are done with your tickets
	}
	//printf("\nwaitForTour parkMutex%s", visitorWait->name);
	SEM_SIGNAL(parkMutex); SWAP;

	SEM_WAIT(driverAndPassMutex); SWAP;
	{
		//printf("\nwaitForTour seatTaken%s",visitorWait->name);
		SEM_SIGNAL(seatTaken);	SWAP;
		SEM_WAIT(needpassengerWait);		SWAP;
		mySem = visitorWait;			SWAP;
		//printf("\nwaitForTour passengerReady%s", visitorWait->name);
		SEM_SIGNAL(passengerReady);  SWAP;// got the passenger
		SEM_WAIT(passengerAcquired); SWAP;
		
	}
	//printf("\nwaitForTour driverAndPassMutex%s", visitorWait->name);
	SEM_SIGNAL(driverAndPassMutex); SWAP;

	SEM_WAIT(visitorWait); SWAP;//wait until tour is over

}

void ToGiftLine() {
	SEM_WAIT(parkMutex); SWAP;
	{
		myPark.numInCars--; SWAP;
		myPark.numInGiftLine++; SWAP;
	}
	//printf("\ngoToGiftLine SignalparkMutex");
	SEM_SIGNAL(parkMutex); SWAP;
}

void waitForGiftShop(Semaphore *visitorWait) {
	randomize_sem(30, visitorWait); SWAP;//randomize time in line
	SEM_WAIT(roomGiftShop); SWAP;
	{
		
		SEM_WAIT(parkMutex); SWAP;
		{
			myPark.numInGiftLine--; SWAP;
			myPark.numInGiftShop++; SWAP;
		}
		//printf("\nwaitForGiftShop SignalparkMutex");
		SEM_SIGNAL(parkMutex); SWAP;
		randomize_sem(30, visitorWait); SWAP;//randomize time in giftshop
	}
	
	SEM_WAIT(getCashierMutex); SWAP;
	{
		//printf("\nneedCashier%s",visitorWait->name);
		//let drivers know we need a cashier
		SEM_SIGNAL(needCashier); SWAP;
		SEM_SIGNAL(wakeupDriver); SWAP;
		//wait until we can buy
		
		SEM_WAIT(getCashier)SWAP;
		//printf("\ngotCashier%s", visitorWait->name);
		SEM_SIGNAL(buyGifts); SWAP;
		
		SEM_WAIT(boughtGifts); SWAP;
		//printf("\ngotGifts%s", visitorWait->name);
	}
	SEM_SIGNAL(getCashierMutex); SWAP;

	
	//printf("\nVistor in gift shop:%s",visitorWait->name);
	SEM_SIGNAL(roomGiftShop); SWAP;
}
void leavePark() {
	SEM_WAIT(parkMutex); SWAP;
	{
		myPark.numInGiftShop--; SWAP;
		myPark.numInPark--; SWAP;
		myPark.numExitedPark++; SWAP;
	}
	//printf("\nleavePark SignalparkMutex");
	SEM_SIGNAL(parkMutex); SWAP;
}

int driverTask(int argc, char*argv[]) {
	char buf[14];		
	Semaphore *driverDoneSig;
	int driverID = atoi(argv[1]);						SWAP;
	printf(buf, "Starting driverTask %d", driverID);	SWAP;
	sprintf(buf, "driverDoneSig%d", driverID);				SWAP;
	driverDoneSig = createSemaphore(buf, BINARY, 0);		SWAP;
	/*if (driverID == 0) {
		insertDeltaClock(10, doMaintenance);
	}*/
	while (1) {
		SEM_WAIT(wakeupDriver);							SWAP;
		//printf("\nDriver:%d woke up", driverID);
		if (SEM_TRYLOCK(needDriver)) {					SWAP;
			//printf("\nDriver needed to drive:%d", driverID);
			printf("\nDriver ready:%d", driverID);
			passOffDriver(driverDoneSig,driverID);			SWAP;
			SEM_SIGNAL(driverReady); 					SWAP;

			SEM_WAIT(driverDoneSig);						SWAP;
			printf("\nDriver finished:%s", driverDoneSig->name);
			updateDriverPos(driverID,0);				SWAP;
		}
		else if(SEM_TRYLOCK(needTicket)) {				SWAP;
			//printf("\nDriver needed to sell:%d", driverID);
			//not in a car
			updateDriverPos(driverID, -1);				SWAP;
			//wait until ticket available (counting)
			//printf("\nDriver waitinf for available tickets:%d", driverID);
			SEM_WAIT(tickets);							SWAP;
			//create Ticket
			//printf("\nDriver getting available ticket:%d", driverID);
			SEM_SIGNAL(getTicket);						SWAP;
			//printf("\nDriver selling ticket:%d", driverID);
			SEM_WAIT(buyTicket);						SWAP;
			updateDriverPos(driverID, 0);				SWAP;
		}
		else if (SEM_TRYLOCK(needCashier)) {			SWAP;//do we need a cashier?
			//printf("\ncashier needed%s", driverDoneSig->name);
			SEM_SIGNAL(getCashier);						SWAP;//we got a cashier
			if (updateDriverPos(driverID, 6) == 0) {	SWAP;
				//printf("\nBe Cashier:%d", driverID);	SWAP;
				SEM_WAIT(buyGifts);						SWAP;//wait for gifts
				updateDriverPos(driverID, 0);			SWAP;
				//printf("\ncashier rang up gifts%s", driverDoneSig->name);
				SEM_SIGNAL(boughtGifts);				SWAP;
				//printf("\nDone Being Cashier:%d", driverID); 
			}
		}
		else if (SEM_TRYLOCK(doMaintenance)) {			SWAP;
			
			if (updateDriverPos(driverID, 5) == 0) {	SWAP;
				
				//printf("\ndoMaintenance:%d", driverID); SWAP;
				randomize_sem(30, driverDoneSig);			SWAP;

				updateDriverPos(driverID, 0);			SWAP;
				//printf("\nMaintenance Done:%d", driverID); SWAP;
				insertDeltaClock(30, doMaintenance);	SWAP; //queue up maintainence
			}
			
		}
		else if (myPark.numExitedPark == NUM_VISITORS) {
			printf("\nDriver%s is done for today, going home.",driverDoneSig->name); SWAP;
			break;
		}
	}
	return 0;
}

void carTask(int argc, char*argv[]) {
	//char buf[32];						   
	int carID = atoi(argv[1]);			   SWAP;
	Semaphore *passengerWait[NUM_SEATS];
	Semaphore *driverDoneSig;

	printf("\nStarting carTask%d",carID);  SWAP;
	while (1) {
		//for every seat
		for (size_t i = 0; i < NUM_SEATS; i++)
		{
			SEM_WAIT(fillSeat[carID]);	   SWAP;
			//printf("\ncarTask SignalgetPassenger");
			SEM_SIGNAL(getPassenger);      SWAP;//say you need a passenger
			SEM_WAIT(seatTaken);           SWAP;//wait until passenger has taken that seat
			//save passenger ride over sem
			passengerWait[i] = getPassengerWait(); SWAP;//grab the passenger
			//printf("\ncarTask seatFilled with:%s",passengerWait[i]->name);
			SEM_SIGNAL(seatFilled[carID]);        SWAP;
			//printf("\nFilled seat %d for car %d", i+1,carID);
		}
		//get driver
		//printf("\nSeats Filled:Getting Driver for car %d", carID);
		SEM_WAIT(needDriverMutex); SWAP;
		{
			//signal that we need the driver
			//printf("\nCar%d needs driver", carID);
			SEM_SIGNAL(needDriver);		SWAP;
			//wakeup the driver
			//printf("\nCar%d wakes up driver", carID);
			SEM_SIGNAL(wakeupDriver);  SWAP;
			//save drive rride over sem
			driverDoneSig = getDriverWait(carID); SWAP;
		}
		//got driver
		//printf("\ngSignaling driver mutex%d", carID);
		//printf("\nGot Driver %s for car %d", driverDoneSig->name, carID);
		SEM_SIGNAL(needDriverMutex); SWAP;

		SEM_WAIT(rideOver[carID]);		   SWAP;
		//printf("\nCar %d and driver %s done with tour:%d", carID,driverDoneSig->name);
		SEM_SIGNAL(driverDoneSig)			   SWAP;
		for (size_t i = 0; i < NUM_SEATS; i++)
		{

			SEM_SIGNAL(passengerWait[i]);  SWAP;
			printf("\nFreeing passenger for car %d", i+1); SWAP;
		}
		if (myPark.numExitedPark == NUM_VISITORS) {
			SWAP;
			break;
		}
	}
}
void randomize_sem(int max, Semaphore *sem) {
	//printf("\ninserting clock%s,%d", sem->name, sem->taskNum);
	if (!(sem->state < -100)) {
		insertDeltaClock(rand() % max, sem); SWAP;
	
		SEM_WAIT(sem); SWAP;
	}
}

//Gets the passenger from the visitor task for the car task
Semaphore *getPassengerWait() {
	Semaphore * wSem;
	//printf("\ngetPassengerWait SignalneedpassengerWait");
	SEM_SIGNAL(needpassengerWait);	SWAP;
	SEM_WAIT(passengerReady);		SWAP;
	wSem = mySem;					SWAP;
	//printf("\ngetPassengerWait SignalpassengerAcquired");
	SEM_SIGNAL(passengerAcquired);  SWAP;// got the passenger
	return wSem;

}

//gets the driver to drive the car for tour in the car task
Semaphore * getDriverWait(int carID) {
	Semaphore * wSem;
	//printf("\ngetDriverWait SignalneedDriver");
	SEM_SIGNAL(needDriverSemaphore);				    SWAP;
	SEM_WAIT(driverReadyMutex);				    SWAP;
	wSem = mySem;						    SWAP;
	//my_driverID is set by the driver task
	updateDriverPos(my_driverID,carID + 1); SWAP;
	//printf("\ngetDriverWait SignalgotADriver");
	SEM_SIGNAL(gotADriver);			    SWAP;// got the passenger
	return wSem;
}

int updateDriverPos(int driverID,int carPos) {
	//printf("\nDriver %d going to pos:%d", driverID,carPos);
	if (!(parkMutex->state < -100)) {//if doesnt exist
		SEM_WAIT(parkMutex);				SWAP;
		myPark.drivers[driverID] = carPos;  SWAP;
		//printf("\nDriver,parkMutex%d", driverID);
		SEM_SIGNAL(parkMutex);				SWAP;
		return 0;
	}
	return -1;//cant access jurassic park
}
//get a driver for driver task, mySem becomes driverDoneSig semaphore which will be passed to the carTask
void passOffDriver(Semaphore *sem, int driverID) {

	SEM_WAIT(driverAndPassMutex);	SWAP;
	SEM_WAIT(needDriverSemaphore);			SWAP;
	mySem = sem;				SWAP;
	my_driverID = driverID;			SWAP;
	//printf("\ndriverReady%d", driverID);
	SEM_SIGNAL(driverReadyMutex);		SWAP;
	SEM_WAIT(gotADriver);		SWAP;
	//printf("\ndriverAndPassMutex%d", driverID);
	SEM_SIGNAL(driverAndPassMutex); SWAP;
}
// ***********************************************************************
// ***********************************************************************
// ***********************************************************************
// ***********************************************************************
// ***********************************************************************
// ***********************************************************************
// delta clock command
int P3_dc(int argc, char* argv[])
{
	printDeltaClock();
	return 0;
} // end CL3_dc


// ***********************************************************************
// display all pending events in the delta clock list
void printDeltaClock(void)
{
	int argc = 0;
	char* argv[] = { NULL };
	//P3_tdc(argc,argv);
	printf("\nDelta Clock");
	// ?? Implement a routine to display the current delta clock contents
	//printf("\nTo Be Implemented!");
	int i;
	for (i= deltaCount - 1; i > -1; i--)
	{
		printf("\n%4d%4d  %-20s",deltaCount - i, deltaClock[i]->time, deltaClock[i]->sem->name);
	}
	return;
}


// ***********************************************************************
// test delta clock
int P3_tdc(int argc, char* argv[])
{

	//dcchange sem
	deltaclockChange = createSemaphore("deltaclockChange", BINARY, 1);
	deltaclockMutex = createSemaphore("deltaclockMutex", BINARY, 1);
	deltaCount = 0;
	//dcmutex sem
	timeTaskID = -1;
	printf("\nDC Test created");
	createTask( "DC Test",			// task name
		dcMonitorTask,		// task
		HIGH_PRIORITY,					// task priority
		argc,					// task arguments
		argv);					
	printf("\nTime task created");
	timeTaskID = createTask( "Time",		// task name
		timeTask,	// task
		HIGH_PRIORITY,			// task priority
		argc,			// task arguments
		argv);			
	return 0;
} // end P3_tdc

//create Delta object
int insertDeltaClock(int time, Semaphore * sem) {
	if (deltaCount >= MAX_DELTAS) {
		return -1;
	}
	SEM_WAIT(deltaclockMutex);
	Delta *delta = (Delta*)malloc(sizeof(Delta));	SWAP;
	delta->sem = sem;								SWAP;
	delta->time = time;								SWAP;

	/*if (deltaCount > 0) {
		printf("\nSignal%s", deltaClock[deltaCount - 1]->sem->name); ///ERROR from deleting sem with it still in queue?
	}*/
	for (int i = deltaCount; i > -1; i--) {
		if (i == 0 || delta->time < deltaClock[i - 1]->time) 
		{
			deltaClock[i] = delta; SWAP; //for tasks just being put in
			if (i > 0) {
				deltaClock[i - 1]->time -= delta->time; SWAP;
			}
			break;
		}
		delta->time -= deltaClock[i - 1]->time; SWAP;
		deltaClock[i] = deltaClock[i - 1]; SWAP;
	}
	//deltaClock[deltaCount - 1]->sem;
	deltaCount += 1;SWAP;
	//printf("\nSignal%s", deltaClock[deltaCount - 1]->sem->name);
	SEM_SIGNAL(deltaclockMutex);
	return 0;

}

int updateDeltaClock(int tics) {
	if (!deltaclockChange) {
		return 0;
	}
	if (deltaCount == 0) {
		deltaTics = 0;
		return 0;
	}
	if (deltaClock[deltaCount - 1]->sem->taskNum < 0) { 
		free(deltaClock[deltaCount - 1]);
		deltaClock[deltaCount - 1] = NULL;
		deltaCount--;
	}
	/*if (deltaCount > 0) {
		printf("\nSignal%s", deltaClock[deltaCount - 1]->sem->name);
	}*/
	if (deltaclockMutex->state && deltaCount > 0) {
		int my_tics = tics - deltaClock[deltaCount - 1]->time;
		deltaClock[deltaCount - 1]->time -= tics; //how many tics have passed

		while (deltaCount > 0 && deltaClock[deltaCount - 1]->time <= 0) {
			//printf("\nSignal%s", deltaClock[deltaCount - 1]->sem->name);
			SEM_SIGNAL(deltaClock[deltaCount - 1]->sem);
			if (!strcmp(deltaClock[deltaCount - 1]->sem->name, "doMaintenance")) {
				SEM_SIGNAL(wakeupDriver);
			}
			free(deltaClock[deltaCount - 1]);
			deltaClock[deltaCount - 1] = NULL;
			deltaCount--;
			// to next pos if there is one
			if (my_tics > 0 && deltaCount > 0) {
				int tic = my_tics - deltaClock[deltaCount - 1]->time;
				deltaClock[deltaCount - 1]->time -= my_tics;
				my_tics = tic;
			}
			//printf("\nSignal deltaclockChange");
			SEM_SIGNAL(deltaclockChange);
		}
		deltaTics = 0;
	}
	return 0;

}

// ***********************************************************************
// monitor the delta clock task. Done
int dcMonitorTask(int argc, char* argv[])
{
	int i, flg;
	char buf[32];
	// create some test times for event[0-9]
	Semaphore* event[10];
	int ttime[10] = {
		90, 300, 50, 170, 340, 300, 50, 300, 40, 110	};

	for (i=0; i<10; i++)
	{
		sprintf(buf, "event[%d]", i);
		event[i] = createSemaphore(buf, BINARY, 0);
		insertDeltaClock(ttime[i], event[i]);
	}
	printDeltaClock();
	while (deltaCount > 0)
	{
		SEM_WAIT(deltaclockChange)
		flg = 0;
		for (i=0; i<10; i++)
		{
			if (event[i]->state ==1)			{
					printf("\n  event[%d] signaled", i);
					event[i]->state = 0;
					flg = 1;
				}
		}
		if (flg) printDeltaClock();
	}
	printf("\nNo more events in Delta Clock");

	// kill deltaclockMonitorTask
	tcb[timeTaskID].state = S_EXIT;
	return 0;
} // end deltaclockMonitorTask


extern Semaphore* tics1sec;

// ********************************************************************************************
// display time every tics1sec
int timeTask(int argc, char* argv[])
{
	char svtime[64];						// ascii current time
	while (1)
	{
		SEM_WAIT(tics1sec)
		printf("\nTime = %s", myTime(svtime));
	}
	return 0;
} // end timeTask


