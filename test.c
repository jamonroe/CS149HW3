/*
 * test.c
 *
 *  Created on: Mar 4, 2014
 *      Author: Jason
 */
#include <stdio.h>
#include <pthread.h>
#include <math.h>
#define SIZE 100
#define EMPTY 0
#define HIGH 1
#define MEDIUM 2
#define LOW 3

pthread_mutex_t mutex;

struct seller {
   char* name;
   int type;
   int sold;
};

struct seller seats[SIZE];

void init() {
   int i;
   for (i = 0; i < SIZE; i++) {
      seats[i].name = "--";
      seats[i].type = EMPTY;
      seats[i].sold = 0;
   }
}

void sellseat(struct seller* s, int i) {
   if (i >= 0 && i < SIZE) {
      (*s).sold = (*s).sold + 1;
      seats[i].name = (*s).name;
      seats[i].type = (*s).type;
      seats[i].sold = (*s).sold;
   }
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

int main() {
   init();
   struct seller sh0 = {"H0", HIGH, 0};
   struct seller sm1 = {"M1", MEDIUM, 0};
   struct seller sl1 = {"L1", LOW, 0};
   //which seller to test
   struct seller* s = &sh0;
   //test instructions
   int i = findseat((*s).type);
   while (i != -1) {
      sellseat(s, i);
      i = findseat((*s).type);
   }
   //print results
   printstadium();
   return 1;
}

