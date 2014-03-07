/*
 * test.c
 *
 *  Created on: Mar 4, 2014
 *      Author: Jason
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <string.h>
#include <sys/time.h>
#include <math.h>

#define SIZE 100
#define SALE_DURATION 60

#define TRUE 1
#define FALSE 0

#define EMPTY 0
#define HIGH 1
#define MEDIUM 2
#define LOW 3

//The mutex for accessing seats and editing stats
pthread_mutex_t seatMutex;
//The mutex for printing
pthread_mutex_t printMutex;

sem_t semTemp;

//The timing variables
struct itimerval saleTimer;
time_t startTime;
int timesUp = 0;
int hourStarted = 0;

//Stats
int sales[4];
int lost_sales;

//A struct to simulate a ticket seller
struct seller {
   char *name;
   int type;
   int sold;
   sem_t waiting;
   pthread_t threadid;
};

//A struct to store seat information
struct seat {
   char *name;
   int type;
   int sold;
};

//The array of seats
struct seat seats[SIZE];
struct seller sellers[10];
int customerCount;

void printseat(int i) {
   if (seats[i].type == EMPTY) {
      printf("----");
   } else {
      printf("%2s%02d", seats[i].name, seats[i].sold);
   }
}

void printstadium() {
   int i;
   for (i = 0; i < SIZE; i++) {
      printseat(i);
      printf(" ");
      if ((i+1) % 10 == 0) {
         printf("\n");
      }
   }
}

//Used to print events
void print(char *event, int printSeating) {
   
   //Calculate the timestamp
   time_t now;
   time(&now);
   double elapsed = difftime(now, startTime);
   int min = 0;
   int sec = (int) elapsed;

   //Convert the seconds to minutes and seconds
   while (sec >= 60) {
      min++;
      sec -= 60;
   }

   //Acquire the mutex lock to protect printing
   pthread_mutex_lock(&printMutex);

   //Elapsed time and event
   printf("%2d:%02d | %s\n", min, sec, event);

   if (printSeating) {
      printf("\n");
      printstadium();
      printf("\n");
   }

   //Release the mutex lock for printing
   pthread_mutex_unlock(&printMutex);
}

//Used to sell a seat
void sellseat(struct seller *s, int i) {
   if (i >= 0 && i < SIZE) {
      (*s).sold = (*s).sold + 1;
      seats[i].name = (*s).name;
      seats[i].type = (*s).type;
      seats[i].sold = (*s).sold;
      sales[seats[i].type]++;
   }
}

int findseat(int type) {
   int i;
   switch(type) {
   case HIGH:
      i = 0;
      while (i < SIZE) {
         if (seats[i].type == EMPTY) {
            return i;
         }
         i++;
      }
   case MEDIUM:
      i = 50;
      int n = 0;
      while (i < SIZE && i >= 0) {
         if (seats[i].type == EMPTY) {
            return i;
         }
         i++;
         if (i % 10 == 0) {
            n++;
            i = 50 + 10 * ceil(n/2) * pow(-1, n);
         }
      }
   case LOW:
      i = 90;
      while(i >= 0) {
         if (seats[i].type == EMPTY) {
            return i;
         }
         i++;
         if (i % 10 == 0) {
            i = i - 20;
         }
      }
   }
   return -1;
}

void customerArrives(struct seller *s) {
   char event[80];
   sprintf(event, "A customer arrives at ticket seller %s", (*s).name);
   
   print(event, FALSE);

   //Notify the ticket seller that there is a customer
   sem_post(&(*s).waiting);
}

void *customer(void *param) {
   struct seller *s = ((struct seller *) param);
   
   //Wait for the hour to start
   while (!hourStarted) { }
   
   //Customers will arrive at random times during the sale
   sleep(rand()%SALE_DURATION);
   customerArrives(s);

   return NULL;
}

//A ticket seller services a customer
int sellerHelpsCustomer(struct seller *s) {
   
   //Wait for a customer to arrive
   sem_wait(&(*s).waiting);

   //When time is up, the customers leave the queue
   if (timesUp) {

      int lost;
      sem_getvalue(&(*s).waiting, &lost);
      if (lost == 0) {
         return 0;
      }

      //Acquire the mutex lock to protect lost_sales
      pthread_mutex_lock(&seatMutex);

      lost_sales = lost_sales + lost;

      //Release the mutex lock for seating
      pthread_mutex_unlock(&seatMutex);

      char event[80];
      sprintf(event, "%d customer(s) leave(s) ticket seller %s", lost, (*s).name);

      print(event, FALSE);
      return 0;
   }

   char event[80];

   //Acquire the mutex lock to protect seating
   pthread_mutex_lock(&seatMutex);

   int seat = findseat((*s).type);
   if (seat == -1) {
      //Release the mutex lock for seating
      pthread_mutex_unlock(&seatMutex);

      sprintf(event, "A customer leaves ticket seller %s since no seats are available", (*s).name);
      print(event, FALSE);
      lost_sales++;
      return 1;
   }
   
   sellseat(s, seat);
   
   //Release the mutex lock for seating
   pthread_mutex_unlock(&seatMutex);

   sprintf(event, "Ticket seller %s begins the sale of seat #%d", (*s).name, seat);
   print(event, TRUE);

   int sleeptime = rand()%((*s).type + 1) + pow(2, (*s).type - 1);
   sleep(sleeptime);
   
   sprintf(event, "After %d minutes the ticket seller %s finishes the sale", sleeptime, (*s).name);
   print(event, FALSE);
   return 1;
}

void *ticketSeller(void *param) {
   struct seller *s = (struct seller *) param;

   //Set up customer threads
   int i;
   for (i = 0; i < customerCount; i++) {
      pthread_t customerThreadId;
      pthread_attr_t customerAttr;
      pthread_attr_init(&customerAttr);
      pthread_create(&customerThreadId, &customerAttr, customer, s);
   }

   //Wait for the sale to start
   while (!hourStarted) { }
   int flag;
   //Help customers during the sale
   do {
      flag = sellerHelpsCustomer(s);
   } while (flag);
}
   
// Timer signal handler.
void timerHandler(int signal)
{
   timesUp = 1;  // ticket office hour is over
   print("Ticket sales end", FALSE);
   //Wake up ticket sellers
   int i;
   for (i = 0; i < 10; i++) {
      sem_post(&sellers[i].waiting);
   }
}

//Used to initialize the seats to empty and sales to zero
void initSeats() {
   int i;
   for (i = 0; i < SIZE; i++) {
      seats[i].type = EMPTY;
      seats[i].sold = 0;
   }
   sales[HIGH] = 0;
   sales[MEDIUM] = 0;
   sales[LOW] = 0;
   lost_sales = 0;
}

//Used to initialize the sellers
void initSellers() {
   int i;
   char name[3];
   //HIGH PRICE TICKET SELLER
      sellers[0].name = "H0";
      sellers[0].type = HIGH;
      sellers[0].sold = 0;
      sem_init(&sellers[0].waiting, 0, 0);

      //Create seller threads
      pthread_attr_t ticketSellerAttr;
      pthread_attr_init(&ticketSellerAttr);
      pthread_create(&sellers[0].threadid, &ticketSellerAttr, ticketSeller, &sellers[0]);

   //MEDIUM PRICE TICKET SELLERS
   for (i = 1; i < 4; i++) {
      sprintf(name, "M%1d", i);
      sellers[i].name = (char *) malloc(strlen(name)*sizeof(char));
      strcpy(sellers[i].name, name);
      sellers[i].type = MEDIUM;
      sellers[i].sold = 0;
      sem_init(&sellers[i].waiting, 0, 0);

      //Create seller threads
      pthread_attr_t ticketSellerAttr;
      pthread_attr_init(&ticketSellerAttr);
      pthread_create(&sellers[i].threadid, &ticketSellerAttr, ticketSeller, &sellers[i]);
   }

   //LOW PRICE TICKET SELLERS
   for (i = 4; i < 10; i++) {
      sprintf(name, "L%1d", i-3);
      sellers[i].name = (char *) malloc(strlen(name)*sizeof(char));
      strcpy(sellers[i].name, name);
      sellers[i].type = LOW;
      sellers[i].sold = 0;
      sem_init(&sellers[i].waiting, 0, 0);

      //Create seller threads
      pthread_attr_t ticketSellerAttr;
      pthread_attr_init(&ticketSellerAttr);
      pthread_create(&sellers[i].threadid, &ticketSellerAttr, ticketSeller, &sellers[i]);
   }
}

int main(int argc, char *argv[]) {
   if (argc != 2) {
      fprintf(stderr, 
         "Usage: Indicate the number of customers for each ticket seller (5, 10, or 15)\n");
      return -1;
   }

   customerCount = atoi(argv[1]);
   if (customerCount != 5 && customerCount != 10 && customerCount != 15) {
      fprintf(stderr,
         "Usage: Indicate the number of customers for each ticket seller (5, 10, or 15)\n");
      return -1;
   }
   srand(time(0));

   //Initialize seats and sellers
   initSeats();
   initSellers();

   //Initialize the mutexes
   pthread_mutex_init(&seatMutex, NULL);
   pthread_mutex_init(&printMutex, NULL);

   //Bind the timer signal
   signal(SIGALRM, timerHandler);

   //Start the ticket selling
   printf("Simulation start\n");
   time(&startTime);
   hourStarted = 1;
   saleTimer.it_value.tv_sec = SALE_DURATION;
   setitimer(ITIMER_REAL, &saleTimer, NULL);

   //Wait for the threads to finish
   int i;
   for (i = 0; i < 10; i++) {
      pthread_join(sellers[i].threadid, NULL);
   }

   print("Simulation Complete", TRUE);

   printf("Sales:\nH: %d\nM: %d\nL: %d\nLost: %d\n", sales[HIGH], sales[MEDIUM], sales[LOW], lost_sales);
   return 1;
}

