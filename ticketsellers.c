/**
*Author: Eric Castro
* 3/6/14
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <sys/time.h>

//Booleans
#define true 1
#define false 0

#define ID_BASE 100

#define HIGH_SELLERS 1
#define MEDIUM_SELLERS 3
#define LOW_SELLERS 6
#define SEAT_ROWS 0
#define SEAT_COLUMNS 0

#define HOURS_OF_OPERATION 10


int seats[10][10]; //Available seats
int seatsRemaining = 100; //Number of available seats
pthread_mutex_t seatsMutex; //Protects seats & seatsRemaining
pthread_mutex_t printMutex; //Protects printing
sem_t h_queue_sem; //Ticket sellers (high) wait on this semaphore
sem_t m_queue_sem; //Ticket sellers (medium) wait on this semaphore
sem_t l_queue_sem; //Ticket sellers (lower) wait on this semaphore

struct itimerval officeTimer; // Box office business hour timer
time_t startTime;

//Track ticket sales of each type
int h_purchase_count;
int m_purchase_count;
int l_purchase_count;

//Queues for each priority
int * h_queue;
int * m_queue;
int * l_queue;
int h_start = 0;
int m_start = 0;
int l_start = 0;
int h_end = 0;
int m_end = 0;
int l_end = 0;

int expected_customers;

int firstPrint = true;
int boxOfficeOpen = false;
int businessHours = true;

// Print a line for each event:
//   elapsed time
// Note: If showQueue == 0, no queue will be printed
void print(char *event, int showQueue)
{
	//Calculate time stamp
	time_t now;
    time(&now);
    double elapsed = difftime(now, startTime);
    int min = 0;
    int sec = (int) elapsed;

    if (sec >= 60) {
        min++;
        sec -= 60;
    }
    //End time stamp calculation

    // Acquire the mutex lock to protect the printing.
    pthread_mutex_lock(&printMutex);

    if (firstPrint) {
        printf("TIME | EVENT");
        int i = 0;
        for(i;i<20;i++)
        	printf(" ");
        printf(" | LINES\n");
        firstPrint = false;
    }

 	// Print elapsed time.
    printf("%1d:%02d | ", min, sec);

    printf("%s", event);

    //Prints the queue
    int * printQueue;
    char queue_name;
    int start;
    int end;
    switch(showQueue)
    {
    	case 1:
    		queue_name = 'H';
    		printQueue = h_queue;
    		start = h_start;
    		end = h_end;
    		break;
    	case 2:
    		queue_name = 'M';
    		printQueue = m_queue;
    		start = m_start;
    		end = m_end;
    		break;
    	case 3:
    	queue_name = 'L';
    		printQueue = l_queue;
    		start = l_start;
    		end = l_end;
    		break;
    	default:break;
    }
    if(showQueue)
    {
    	printf("              | ");
    	printf("%c: [ ", queue_name);
    	int index = start;
    	for(index;index<end;index++)
    		printf("%d ",printQueue[index]);

    	printf("]");
    }
    //End queue printing

    printf("\n");

    // Release the mutex lock.
    pthread_mutex_unlock(&printMutex);
}

// Seat lookup, returns array of size 2
// seatLoc[0] - Row
// seatLoc[0] - Column
// returns NULL if there are no seats remaining
int * findSeat(int priority)
{
	// Acquire the mutex lock to protect the printing.
    pthread_mutex_lock(&seatsMutex);
	if(!seatsRemaining)
		return NULL;

	int startPos;
	switch(priority)
	{
		case 1:startPos = 0;break;
		case 2:startPos = 5;break;
		case 3:startPos = 9;break;
	}

	int seatLoc[2];
	seatLoc[0] = -1;
	seatLoc[1] = -1;
	seatsRemaining--;
	// Release the mutex lock.
    pthread_mutex_unlock(&seatsMutex);

	return seatLoc;

}

void sellerHelpsCustomer()
{
	//Get a customer from the queue

	//Find them a seat

	//Get next customer (if available)
}

void customerArrives(int customer_id, int ticket_type)
{
	char event[80];
	//Get in queue base on priority
	switch(ticket_type){
		case 1:
			h_queue[h_end++] = customer_id;
			sprintf(event, "H%d arrives", customer_id, 1);
			break;
		case 2:
			m_queue[m_end++] = customer_id;
			sprintf(event, "M%d arrives", customer_id, 2);
			break;
		case 3:
			l_queue[l_end++] = customer_id;
			sprintf(event, "L%d arrives", customer_id, 3);
			break;
		default:
			sprintf(event, "Error: unknown ticket type %d", ticket_type);
			print(event,0);
			exit;
		break;
	}

	print(event, ticket_type);
}

//Ticket seller threads
void *highSeller(void *param)
{
	//First ticket seller opens the box office
	if(!boxOfficeOpen)
	{
		time(&startTime);
		print("Box office open", 0);
		boxOfficeOpen = true;
		// Set the timer for for office hour duration.
    	officeTimer.it_value.tv_sec = HOURS_OF_OPERATION;
    	setitimer(ITIMER_REAL, &officeTimer, NULL);
	}

	//Seller types: 1 - High Priority 
	//				2 - Medium Priority
	//				3 - Low Priority
	int sellerType = *((int *) param);

	do{
		sellerHelpsCustomer();
	}while(businessHours);

	//Ticket seller closes the box office
	if(boxOfficeOpen){
		print("Box office closed", 0);
		boxOfficeOpen = false;	
	}
	
}

void *mediumSeller(void *param)
{
	//First ticket seller opens the box office
	if(!boxOfficeOpen)
	{
		time(&startTime);
		print("Box office open", 0);
		boxOfficeOpen = true;
		// Set the timer for for office hour duration.
    	officeTimer.it_value.tv_sec = HOURS_OF_OPERATION;
    	setitimer(ITIMER_REAL, &officeTimer, NULL);
	}

	//Seller types: 1 - High Priority 
	//				2 - Medium Priority
	//				3 - Low Priority
	int sellerType = *((int *) param);

	do{
		sellerHelpsCustomer();
	}while(businessHours);

	//Ticket seller closes the box office
	if(boxOfficeOpen){
		print("Box office closed", 0);
		boxOfficeOpen = false;	
	}
	
}

void *lowSeller(void *param)
{
	//First ticket seller opens the box office
	if(!boxOfficeOpen)
	{
		time(&startTime);
		print("Box office open", 0);
		boxOfficeOpen = true;
		// Set the timer for for office hour duration.
    	officeTimer.it_value.tv_sec = HOURS_OF_OPERATION;
    	setitimer(ITIMER_REAL, &officeTimer, NULL);
	}

	//Seller types: 1 - High Priority 
	//				2 - Medium Priority
	//				3 - Low Priority
	int sellerType = *((int *) param);

	do{
		sellerHelpsCustomer();
	}while(businessHours);

	//Ticket seller closes the box office
	if(boxOfficeOpen){
		print("Box office closed", 0);
		boxOfficeOpen = false;	
	}
	
}

// Timer signal handler.
void timerHandler(int signal)
{
    businessHours = false;  // office hour is over
}

//Customer thread
void *customer(void *param)
{
	//Customer type (1-High, 2-Medium, 3-Low)
	int customer_id = *((int *) param);
	int ticket_type = rand()%3+1;

	int sleepTime = rand()%HOURS_OF_OPERATION;
	
	sleep(sleepTime);

	customerArrives(customer_id, ticket_type);
	return NULL;
}

int main( int argc, char* argv[] )
{
	if(argc <= 1)
	{
		printf("Usage: Enter the number of customers expected to arrive.\n");
		return 0;
	}
	
	printf("Setting up simulation :)\n");
	int n = atoi(argv[1]);//n customers
	int customerIds[n];
	expected_customers = n;

	//Set the queue sizes
	h_queue = (int*)malloc(expected_customers);
	m_queue = (int*)malloc(expected_customers);
	l_queue = (int*)malloc(expected_customers);

	// Initialize the mutexes and the semaphores.
    pthread_mutex_init(&seatsMutex, NULL);
    pthread_mutex_init(&printMutex, NULL);
    sem_init(&h_queue_sem, 0, 0);
    sem_init(&m_queue_sem, 0, 0);
    sem_init(&l_queue_sem, 0, 0);
	printf("SIMULATION READY >:)\n");
	printf("==========Jerrick - Ticket Sellers Simulation==========\n");
	
	//Create High ticket seller threads
	pthread_t highThreadId;
    pthread_attr_t highAttr;
    pthread_attr_init(&highAttr);
    pthread_create(&highThreadId, &highAttr, highSeller , 0);

	//Create Medium ticket seller threads
	int i;
	for(i=0;i<MEDIUM_SELLERS;i++)
	{
		pthread_t mediumThreadId;
        pthread_attr_t mediumAttr;
        pthread_attr_init(&mediumAttr);
        pthread_create(&mediumThreadId, &mediumAttr, mediumSeller, i+1);
	}

	//Create Low ticket seller threads
	for(i=0;i<LOW_SELLERS;i++)
	{
		pthread_t lowThreadId;
        pthread_attr_t lowAttr;
        pthread_attr_init(&lowAttr);
        pthread_create(&lowThreadId, &lowAttr, lowSeller, i+1);
	}

	//Create customer threads
	for(i=0;i < n; i++)
	{
		customerIds[i] = ID_BASE + i;
		pthread_t customerThreadId;
        pthread_attr_t customerAttr;
        pthread_attr_init(&customerAttr);
        pthread_create(&customerThreadId, &customerAttr, customer, &customerIds[i]);
	}

	// Set the timer signal handler.
    signal(SIGALRM, timerHandler);

	//Wait for threads to end
	pthread_join(highThreadId, NULL);
}