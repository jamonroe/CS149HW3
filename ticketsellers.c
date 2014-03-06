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
int seatsRemaining; //Number of available seats
pthread_mutex_t seatsMutex; //Protects seats & seatsRemaining
pthread_mutex_t printMutex; //Protects printing
sem_t h_queue; //Ticket sellers (high) wait on this semaphore
sem_t m_queue; //Ticket sellers (medium) wait on this semaphore
sem_t l_queue; //Ticket sellers (lower) wait on this semaphore

struct itimerval officeTimer; // Box office business hour timer
time_t startTime;

int firstPrint = true;
int boxOfficeOpen = false;
int businessHours = true;

// Print a line for each event:
//   elapsed time
void print(char *event)
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
        printf("TIME | MEETING | WAITING     | EVENT\n");
        firstPrint = false;
    }

 	// Print elapsed time.
    printf("%1d:%02d | ", min, sec);

    printf("%s\n", event);

    // Release the mutex lock.
    pthread_mutex_unlock(&printMutex);
}

void sellerHelpsCustomer()
{

}

void customerArrives(int customer_id, int ticket_type)
{
	char event[80];
	sprintf(event, "Customer %d arrives. Priority: %d", customer_id, ticket_type);
	print(event);
}

//Ticket seller thread
void *ticketseller(void *param)
{
	//First ticket seller opens the box office
	if(!boxOfficeOpen)
	{
		time(&startTime);
		print("Box office open");
		boxOfficeOpen = true;
		// Set the timer for for office hour duration.
    	officeTimer.it_value.tv_sec = HOURS_OF_OPERATION;
    	setitimer(ITIMER_REAL, &officeTimer, NULL);
	}

	//Seller types: 1 - High Priority 
	//				2 - Medium Priority
	//				3 - Low Priority
	int sellerType = *((int *) param);

	int startPos;
	switch(sellerType)
	{
		case 1:startPos = 0;break;
		case 2:startPos = 5;break;
		case 3:startPos = 9;break;
	}

	do{
		sellerHelpsCustomer();
	}while(businessHours);

	print("Box office closed");
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
	
	int n = atoi(argv[1]);//n customers
	int customerIds[n];

	// Initialize the mutexes and the semaphores.
    pthread_mutex_init(&seatsMutex, NULL);
    pthread_mutex_init(&printMutex, NULL);
    sem_init(&h_queue, 0, 0);
    sem_init(&m_queue, 0, 0);
    sem_init(&l_queue, 0, 0);

	printf("==========Jerrick - Ticket Sellers Simulation==========\n");
	//Create High ticket seller threads

	//Create Medium ticket seller threads

	//Create Low ticket seller threads
	int professorId = 0;
	pthread_t professorThreadId;
    pthread_attr_t profAttr;
    pthread_attr_init(&profAttr);
    pthread_create(&professorThreadId, &profAttr, ticketseller, &professorId);

	//Create customer threads
	int i;
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

	//Wait for thread to end
	pthread_join(professorThreadId, NULL);
}