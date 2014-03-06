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
#include <sys/time.h>
#include <math.h>

#define SIZE 100
#define SALE_DURATION 10

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
};

//A struct to store seat information
struct seat {
   char *name;
   int type;
   int sold;
};

//The array of seats
struct seat seats[SIZE];
int customerCount = 5;

//Used to initialize the seats to empty and sales to zero
void init() {
   int i;
   for (i = 0; i < SIZE; i++) {
      seats[i].name = "--";
      seats[i].type = EMPTY;
      seats[i].sold = 0;
   }
   sales[HIGH] = 0;
   sales[MEDIUM] = 0;
   sales[LOW] = 0;
   lost_sales = 0;
}

//Used to print events
void print(char *event) {
   
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

   //Elapsed time and event
   printf("%2d:%02d | %s\n", min, sec, event);
}

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

//Used to sell a seat
void sellseat(struct seller *s, int i) {
   if (i >= 0 && i < SIZE) {

      (*s).sold = (*s).sold + 1;
      seats[i].name = (*s).name;
      seats[i].type = (*s).type;
      seats[i].sold = (*s).sold;
      sales[seats[i].type]++;

      char event[80];
      sprintf(event, "Ticket Seller %s sells seat %d (sale #%d)", seats[i].name, i, seats[i].sold);
      
      //Acquire the mutex lock to protect printing
      pthread_mutex_lock(&printMutex);

      //Print the sale
      print(event);
      printf("\n");

      //Print the new seating chart
      printstadium();

      //Release the mutex lock for printing
      pthread_mutex_unlock(&printMutex);
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
   
   //Acquire the mutex lock to protect printing
   pthread_mutex_lock(&printMutex);

   print(event);

   //Release the mutex lock for printing
   pthread_mutex_unlock(&printMutex);
   
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
void sellerHelpsCustomer(struct seller *s) {
   
   if (!timesUp) {
      //Wait for a customer to arrive
      sem_wait(&(*s).waiting);

      char event[80];
      sprintf(event, "A customer is helped at ticket seller %s", (*s).name);
   
      //Acquire the mutex lock to protect printing
      pthread_mutex_lock(&printMutex);

      print(event);

      //Release the mutex lock for printing
      pthread_mutex_unlock(&printMutex);
   }
}

void *ticketSeller(void *param) {
   struct seller *s = (struct seller *) param;

   //Set up customer threads
   int i;
   //for (i = 0; i < customerCount; i++) {
      pthread_t customerThreadId;
      pthread_attr_t customerAttr;
      pthread_attr_init(&customerAttr);
      pthread_create(&customerThreadId, &customerAttr, customer, s);
   //}

   //Wait for the sale to start
   while (!hourStarted) { }
   //Help customers during the sale
   do {
      sellerHelpsCustomer(s);
   } while (!timesUp);
   
   return NULL;
}

// Timer signal handler.
void timerHandler(int signal)
{
    timesUp = 1;  // ticket office hour is over
}

int main() {
   init();

   //Initialize the mutexes
   pthread_mutex_init(&seatMutex, NULL);
   pthread_mutex_init(&printMutex, NULL);

   //Bind the timer signal
   signal(SIGALRM, timerHandler);

   //Construct the sellers (1 HIGH, 3 MEDIUM, 6 LOW)
   /* INCOMPLETE */
   struct seller s;
   s.name = "H0";
   s.type = HIGH;
   s.sold = 0;
   sem_init(&s.waiting, 0, 0);
   sem_init(&semTemp, 0, 0);

   //Create seller threads
   /* INCOMPLETE */
   pthread_t ticketSellerThreadId;
   pthread_attr_t ticketSellerAttr;
   pthread_attr_init(&ticketSellerAttr);
   pthread_create(&ticketSellerThreadId, &ticketSellerAttr, ticketSeller, &s);

   //Start the ticket selling
   time(&startTime);
   hourStarted = 1;
   saleTimer.it_value.tv_sec = SALE_DURATION;
   setitimer(ITIMER_REAL, &saleTimer, NULL);

   //Wait for the threads to finish
   /* INCOMPLETE */
   pthread_join(ticketSellerThreadId, NULL);

   return 1;
}

