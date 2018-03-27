//Author: Chance Ball
//Project 3 : Sleeping TAs
//Date: 3/27/18
//To compile: gcc csmc.c -pthread -std=c99 -lm -o csmc
//To execute: ./csmc [#students] [#tutors] [#chairs] [#help]

#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

//Constant Variables
int MAXSLEEP = 3;
int NUMHELPS;
int NUMSEATS;
int NUMSTUDENTS;
int NUMTAS;

//Variable Declarations
int helpCounter;
int numFinished;
int studentIndex;
int studentsWaiting;
int TAWaitTime;
int tutorIndex;
int waitTime;
//Mutual Exclusion Variable
pthread_mutex_t mutex;
//Semaphore Variables
sem_t semStudents;
sem_t semTAs;
//Random Number Variables
unsigned int TAWaitRNG;
unsigned int waitRNG;

//Function Prototypes
void destroy();
void *studentThread(void*);
void *TAThread(void*);

int main(int argc, char *argv[]) 
{
	if(argc != 5)
	{
		printf("Usage: ./csmc [#students] [#tutors] [#chairs] [#help]\n");
		exit(EXIT_FAILURE);
	}
	//Create Mutex lock
	pthread_mutex_init(&mutex, NULL);

 	//Initialize semaphores for students and TAs
	sem_init(&semTAs, 0, 0);
	sem_init(&semStudents, 0, 0);

	//Set the constant values 
	NUMSTUDENTS = atoi(argv[1]);
	NUMTAS = atoi(argv[2]);
	NUMSEATS = atoi(argv[3]);
	NUMHELPS = atoi(argv[4]);
	
	//Initialize the threads for the students and TAs
	pthread_t studentThreads[NUMSTUDENTS];
	pthread_t TAThreads[NUMTAS];

	//Creates TA threads
	for(int i = 0; i < NUMTAS; i++)
	{
		pthread_create(&TAThreads[i], NULL, TAThread, (void*)(intptr_t) i);
	}
	
	//Create student threads
	for (int j = 0; j < NUMSTUDENTS; j++) 
	{
		pthread_create(&studentThreads[j], NULL, studentThread, (void*)(intptr_t) j);
	}

	//Joins each student thread
	for (int k = 0; k < NUMSTUDENTS; k++)
	{
		 pthread_join(studentThreads[k], NULL);
	}

	//Loops until all students are finished using CSMC, and cancels TA thread
	while(1)
	{
		if (numFinished == NUMSTUDENTS) 
		{
			pthread_cancel(TAThreads[0]);
			break;
		}
	}
	printf("All students helped. Exiting.\n");

	//Deletes our semaphores and mutexes
	destroy();

	//Exited sucessfully
	exit(EXIT_SUCCESS);
}

void destroy()
{
	//Destroys our mutex lock we created
	pthread_mutex_destroy(&mutex);

	//Destroys our student semaphore
	sem_destroy(&semStudents);

	//Destroys our TA semaphore
	sem_destroy(&semTAs);
}

void *studentThread(void *index) 
{
	//Index of specific student thread in the array of thread IDs
	studentIndex = (intptr_t)index;

	//A random number generated used for calculating time the student works before getting help
	waitRNG = (studentIndex + 1) * MAXSLEEP * NUMSTUDENTS * NUMHELPS * NUMSEATS;

	//Counter to track times a student gets helped
	helpCounter = 0;

	//Loops until student has been helped necessary amount of times
	while (helpCounter < NUMHELPS) 
	{
		//Student programs for random amount of time before getting stuck
		waitTime = (rand_r(&waitRNG) % MAXSLEEP) + 1;
		printf("Student %d has been working for %d seconds\n", studentIndex+1, waitTime);
		sleep(waitTime);

		//Locking critical section
		pthread_mutex_lock(&mutex);
		if (studentsWaiting < NUMSEATS) 
		{
			//If there are open seats, the student takes a seat
			studentsWaiting++;
			printf("Student %d takes a seat. Waiting students = %d\n", studentIndex+1, studentsWaiting);

			//Unlocking critical section
			pthread_mutex_unlock(&mutex);

			//Available TA thread unblocks to help student
			sem_post(&semStudents);

			//Student thread waits until a TA helps them
			sem_wait(&semTAs);
			
			//Student is helped, increment
			helpCounter++;
		}
		else 
		{
			//Too many students looking for help.
			/*Unlock mutex because otherwise students will get stuck in chairs,
			  causing infinite looping from conflict.*/
			pthread_mutex_unlock(&mutex);
			printf("Student %d found no empty chair. Will try later\n", studentIndex+1);
			//Student then just continues to work on their own.
		}
	}
	/*Student has been helped for enough time, 
	so they are done getting help, 
	meaning one less student  at CSMC.*/
	printf("Student %d needs no more help. They are finished at CSMC.\n", studentIndex+1);
	numFinished++;
	pthread_exit(NULL);
}

void *TAThread(void *index) 
{
	//Index of specific student thread in the array of thread IDs.
	tutorIndex = (intptr_t)index;

	//A random number generated used for calculating time the TA helps a student.
	TAWaitRNG = 5 * MAXSLEEP * NUMSTUDENTS * NUMHELPS * NUMSEATS;
	while(1) 
	{
		//Wait until a signal is received from student needing help.
		sem_wait(&semStudents);

		//Locking critical section.
		pthread_mutex_lock(&mutex);

		//Student helped, a seat opens up.
		studentsWaiting--;
		
		TAWaitTime = (rand_r(&TAWaitRNG) % MAXSLEEP) + 1;
		printf("Tutor %d has been helping a student for %d seconds. Waiting students = %d\n", tutorIndex+1, TAWaitTime, studentsWaiting);

		//Unlocking critical section.
		pthread_mutex_unlock(&mutex);

		//Helps student for a random amount of time, then sends a signal for the student to unblock.
		sem_post(&semTAs);
		sleep(TAWaitTime);
	}
}


