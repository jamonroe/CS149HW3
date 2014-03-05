#include "procon.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>

#define TRUE 1

buffer_item buffer[BUFFER_SIZE];
pthread_mutex_t mutex;
sem_t emptySlots;
sem_t filledSlots;

int in = 0, out = 0;

void *producer(void *param);
void *consumer(void *param);

int insert_item(buffer_item item)
{
    // Wait on the "emptySlots" semaphore.
    sem_wait(&emptySlots);

    // Acquire the mutex lock to protect the buffer.
    pthread_mutex_lock(&mutex);
    
    // Critical region: Insert an item into the buffer.
    buffer[in] = item;
    in = (in+1)%BUFFER_SIZE;

    // Release the mutex lock.
    pthread_mutex_unlock(&mutex);
    
    // Signal the "filledSlots" semaphore.
    sem_post(&filledSlots);  // signal

    return 0;
}

int remove_item(buffer_item *item)
{
    // Wait on the "filledSlots" semaphore.
    sem_wait(&filledSlots);

    // Acquire the mutex lock to protect the buffer.
    pthread_mutex_lock(&mutex);
    
    // Critical region: Remove an item from the buffer.
    *item = buffer[out];
    out = (out+1)%BUFFER_SIZE;

    // Release the mutex lock. 
    pthread_mutex_unlock(&mutex);
    
    // Signal the "emptySlots" semaphore.
    sem_post(&emptySlots);  // signal

    return 0;
}

int main(int argc, char *argv[])
{
    int sleepTime, producerThreadCount, consumerThreadCount;
    int i, j;

    if (argc != 4) {
        fprintf(stderr, 
         "Usage: <run time secs.> <# producer threads> <# consumer threads>\n");
        return -1;
    }

    sleepTime = atoi(argv[1]);
    producerThreadCount = atoi(argv[2]);
    consumerThreadCount = atoi(argv[3]);
    
    int producerIds[producerThreadCount];
    int consumerIds[consumerThreadCount];

    // Initialize the the locks.
    printf("         Mutex init: %d\n", pthread_mutex_init(&mutex, NULL));
    printf("Empty semaphor init: %d\n", sem_init(&emptySlots, 0, BUFFER_SIZE));
    printf(" Full semaphor init: %d\n", sem_init(&filledSlots, 0, 0));
    printf("\n");
    
    srand(time(0));

    // Create the producer threads.
    for (i = 0; i < producerThreadCount; i++) {
        producerIds[i] = i+1;
        pthread_t tid;
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_create(&tid, &attr, producer, &producerIds[i]);
    }

    // Create the consumer threads.
    for (j = 0; j < consumerThreadCount; j++) {
        consumerIds[j] = j+1;
        pthread_t tid;
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_create(&tid, &attr, consumer, &consumerIds[j]);
    }

    // Sleep for user-specified time while the
    // producer and consumer threads run.
    sleep(sleepTime);
    return 0;
}

void *producer(void *param)
{
    int id = *((int *) param);
    int i = 0;
    buffer_item item = 0;

    for (;;) {
        sleep(rand()%5);
        item = 100*id + i++;

        if (insert_item(item)) {
            fprintf(stderr, "Production error");
        }
        else {
            printf("Producer %d produced %d \n", id, item);
        }
    }
}

void *consumer(void *param)
{
    int id = *((int *) param);
    buffer_item item;
    int r;

    for (;;) {
        sleep(rand()%2);

        if (remove_item(&item)) {
            fprintf(stderr, "Consumption error");
        }
        else {
            printf("%40s %d consumed %d \n", "Consumer", id, item);
        }
    }
}
