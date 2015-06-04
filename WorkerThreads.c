// This program provides a layout for batch processing and coordiantion using the C's pthread constructs.
// The asymmetry of this high level program arcitecture stems from all worker threads starting at the same time, but requiring different amounts of time to complete.

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <pthread.h>

typedef enum { FALSE = 0, TRUE = 1 } Bool;
typedef Bool* BoolPtr;

// To introduce a high level of unpredictability, use many more worker threads than the number of cores inside of the CPU that you intend to use.
#define NUMBER_OF_WORKER_THREADS	30
#define NUMBER_OF_WORKING_ROUNDS 4

#define BOGUS 99
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Two sets of inter-thread communication variables are defined below.  Each "condition variable" requires an associated "mutex".
// It is the responsibility of the programmer to ensure that the global variables connected to a "mutex" are only modified under a lock condition.

// Set one.
pthread_mutex_t StartWorkMutex;
pthread_cond_t StartWorkCondition;

// The "StartWorkCondition" sends a boolean flag to request the worker threads to "WorkOn" or terminate, and a value indicating the current "Round".
Bool WorkOn = FALSE;
unsigned int Round = 0;

// Set two.
pthread_mutex_t CompleteMutex;
pthread_cond_t CompleteCondition;

// The worker threads need to let the main thread know which one has just sent the "CompleteCondition" signal, so it can coordinate the correct data set.
// The main thread needs to let the worker threads know when it is waiting for a signal and this is communicated using "MainThreadWaiting".
Bool MainThreadWaiting = FALSE;
unsigned int TheCompletedBatch = BOGUS;

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// This function represents an independent stream of computation that works on a batch of data.
// For now the work is simply busy-work using the "sqrt()" function on "double" floating point numbers.

void *WorkerThread(void *ThreadArgument){
	unsigned int X;
	double BusyWork;
	double BusyWorkTwo;
	long unsigned int ThisThreadNumber = (long unsigned int)ThreadArgument;
	
	// Enter a waiting state for the "StartWorkCondition".
	pthread_mutex_lock(&StartWorkMutex);
	pthread_cond_wait(&StartWorkCondition, &StartWorkMutex);
	while ( TRUE ) {
		// It is necessary to unlock the "StartWorkMutex" before thread termination, so that the other worker threads can also terminate.
		if ( WorkOn ) {
			printf("Thread-%lu: Begin Work On Batch %d\n", ThisThreadNumber, Round);
			pthread_mutex_unlock(&StartWorkMutex);
		}
		else {
			pthread_mutex_unlock(&StartWorkMutex);
			pthread_exit(NULL);
		}
		// This is the location where batch processing work will be carried out.  Right now it is busy-work.
		for ( X = 0; X < 45000000; X++ ) {
			BusyWork = (double)X + 50.75;
			BusyWork = sqrt(BusyWork);
			BusyWorkTwo = sqrt(BusyWork + (double)X);
		}
		// Get a lock on "CompleteMutex" and make sure that the main thread is waiting, then set "TheCompletedBatch" to "ThisThreadNumber".  Set "MainThreadWaiting" to "FALSE".
		// If the main thread is not waiting, continue trying to get a lock on "CompleteMutex" unitl "MainThreadWaiting" is "TRUE".
		while ( TRUE ) {
			pthread_mutex_lock(&CompleteMutex);
			if ( MainThreadWaiting == TRUE ) {
				// While this thread still has a lock on the "CompleteMutex", set "MainThreadWaiting" to "FALSE", so that the next thread to maintain a lock will be the main thread.
				MainThreadWaiting = FALSE;
				break;
			}
			pthread_mutex_unlock(&CompleteMutex);
		}
		TheCompletedBatch = ThisThreadNumber;
		// Lock the "StartWorkMutex" before we send out the "CompleteCondition" signal.
		// This way, we can enter a waiting state for the next round before the main thread broadcasts the "StartWorkCondition".
		pthread_mutex_lock(&StartWorkMutex);
		printf("Thread-%lu: Completed Batch %d\n", ThisThreadNumber, Round);
		pthread_cond_signal(&CompleteCondition);
		pthread_mutex_unlock(&CompleteMutex);
		// Wait for the Main thread to send us the next "StartWorkCondition" broadcast.
		// Be sure to unlock the corresponding mutex immediately so that the other worker threads can exit their waiting state as well.
		pthread_cond_wait(&StartWorkCondition, &StartWorkMutex);
	}
}

int main(){
	long unsigned int Identity;
	unsigned int X;
	unsigned int Y;
	
	// Initialize all of the thread related objects.
	pthread_t Threads[NUMBER_OF_WORKER_THREADS];
  pthread_attr_t attr;
	
	pthread_mutex_init(&CompleteMutex, NULL);
  pthread_cond_init (&CompleteCondition, NULL);
	
	pthread_mutex_init(&StartWorkMutex, NULL);
  pthread_cond_init (&StartWorkCondition, NULL);
	
  /* For portability, explicitly create threads in a joinable state */
  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
	
	// Create all of the worker threads here.
	for ( Identity = 0; Identity < NUMBER_OF_WORKER_THREADS; Identity++ ) pthread_create(&Threads[Identity], &attr, WorkerThread, (void *)Identity);
	
	// Allow time for the worker threads to start up and wait for the signal that is going to be sent out soon.
	printf("Wait for 1 second for the %d worker threads to enter a waiting state.\n\n", NUMBER_OF_WORKER_THREADS);
	sleep(1);
	
	// Broadcast the start work signal to all of the waiting worker threads, and recieve the complete signals one at a time.
	for ( X = 0; X < NUMBER_OF_WORKING_ROUNDS; X++ ) {
		// Lock the "StartWorkMutex" to set the global work coordination variables.
		pthread_mutex_lock(&StartWorkMutex);
		// Not a whole lot to coordinate in this framework demo.
		WorkOn = TRUE;
		Round += 1;
		printf("Main: Broadcast Signal To Start Batch |%d|\n", Round);
		// Lock the "CompleteMutex" so we can start waiting for completion before any of the worker threads finish their batch.
		pthread_mutex_lock(&CompleteMutex);
		pthread_cond_broadcast(&StartWorkCondition);
		pthread_mutex_unlock(&StartWorkMutex);
		
		for ( Y = 0; Y < NUMBER_OF_WORKER_THREADS; Y++ ) {
			// Before entering a waiting state, set "MainThreadWaiting" to "TRUE" while we still have a lock on the "CompleteMutex".
			// Worker threads will be waiting for this condition to be met before sending "CompleteCondition" signals.
			MainThreadWaiting = TRUE;
			pthread_cond_wait(&CompleteCondition, &CompleteMutex);
			printf("Main: Complete Signal Recieved From Thread-%d\n", TheCompletedBatch);
			// This is where partial work on the batch data coordination will happen.  All of the worker threads will have to finish before we can start the next batch.
		}
		pthread_mutex_unlock(&CompleteMutex);
	}
	
	
	pthread_mutex_lock(&StartWorkMutex);
	// Set the GAME OVER condition.
	WorkOn = FALSE;
	printf("Main: Broadcast The Termination Signal\n");
	pthread_cond_broadcast(&StartWorkCondition);
	pthread_mutex_unlock(&StartWorkMutex);

  // Wait for all threads to complete, and then join with them.
  for ( X = 0; X < NUMBER_OF_WORKER_THREADS; X++ ) {
		pthread_join(Threads[X], NULL);
		printf("Main: Thread[%d] Has Been Joined And Terminated.\n", X);
	}
	
	// Clean up and exit.
  pthread_attr_destroy(&attr);
  pthread_mutex_destroy(&CompleteMutex);
  pthread_cond_destroy(&CompleteCondition);
	pthread_mutex_destroy(&StartWorkMutex);
  pthread_cond_destroy(&StartWorkCondition);
  pthread_exit (NULL);
}