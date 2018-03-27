//Author: Chance Ball
//Project 3 : Sleeping Tutors
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
int MAXSLEEP = 3; //Max amount of time a tutor can spend helping a student in one attempt.
int NUMHELPS; //Number of times a student can get help.
int NUMSEATS; //Number of seats at the CSMC.
int NUMSTUDENTS; //Number of students needing help at CSMC.
int NUMTUTORS; //Number of tutors at the CSMC.

//Variable Declarations
int helpCounter; //Times a student has been helped so far.
int numFinished; //How many students have finished getting help, and left CSMC.
int studentsWaiting; //How many students are waiting for assistance.

//Mutual Exclusion Variable
pthread_mutex_t mutex;

//Semaphore Variables
sem_t semStudents; //Student semaphore.
sem_t semTutors; //Tutor semaphore.

//Function Prototypes
void destroy(); //Found on lines 102-112
void *studentThread(void*); //Found on lines 114 - 167
void *tutorThread(void*); //FOund on lines 169 - 197

int main(int argc, char *argv[]) 
{
	if(argc != 5)
	{
		printf("Usage: ./csmc [#students] [#tutors] [#chairs] [#help]\n");
		exit(EXIT_FAILURE);
	}
	//Create Mutex lock.
	pthread_mutex_init(&mutex, NULL);

 	//Initialize semaphores for tutors.
	sem_init(&semTutors, 0, 0);
	//Initialize semaphores for students.
	sem_init(&semStudents, 0, 0);

	//Set the constant values.
	NUMSTUDENTS = atoi(argv[1]);
	NUMTUTORS   = atoi(argv[2]);
	NUMSEATS    = atoi(argv[3]);
	NUMHELPS    = atoi(argv[4]);
	
	//Initialize the threads for the students.
	pthread_t studentThreads[NUMSTUDENTS];
	//Initialize semaphores for tutors.
	pthread_t tutorThreads[NUMTUTORS];

	//Creates tutor threads.
	for(int i = 0; i < NUMTUTORS; i++)
	{
		pthread_create(&tutorThreads[i], NULL, tutorThread, (void*)(intptr_t) i);
	}
	
	//Create student threads.
	for (int j = 0; j < NUMSTUDENTS; j++) 
	{
		pthread_create(&studentThreads[j], NULL, studentThread, (void*)(intptr_t) j);
	}

	//Joins each student thread.
	for (int k = 0; k < NUMSTUDENTS; k++)
	{
		 pthread_join(studentThreads[k], NULL);
	}

	//Loops until all students are finished using CSMC, and cancels tutor thread.
	while(1)
	{
		if (numFinished == NUMSTUDENTS) 
		{
			pthread_cancel(tutorThreads[0]);
			break;
		}
	}
	printf("All students helped. Exiting.\n");

	//Deletes our semaphores and mutex.
	destroy();

	//Exited sucessfully.
	exit(EXIT_SUCCESS);
}

void destroy() //Destroys the mutex and both semaphores
{
	//Destroys our mutex lock we created.
	pthread_mutex_destroy(&mutex);

	//Destroys our student semaphore.
	sem_destroy(&semStudents);

	//Destroys our tutor semaphore.
	sem_destroy(&semTutors);
}

void *studentThread(void *index) //Keeps track of work done, times helped, etc. for each unique student at CSMC.
{
	//Index of specific student thread in the array of thread IDs.
	int studentIndex = (intptr_t)index;

	//A random number generated used for calculating time the student works before getting help.
	unsigned int waitRNG = (studentIndex + 1) * MAXSLEEP * NUMSTUDENTS * NUMHELPS * NUMSEATS;

	//Counter to track times a student gets helped.
	helpCounter = 0;

	//Loops until student has been helped necessary amount of times.
	while (helpCounter < NUMHELPS) 
	{
		//Student programs for random amount of time before getting stuck.
		int waitTime = (rand_r(&waitRNG) % MAXSLEEP) + 1;
		printf("Student %d has been working for %d seconds\n", studentIndex+1, waitTime);
		sleep(waitTime);

		//Locking critical section.
		pthread_mutex_lock(&mutex);
		if (studentsWaiting < NUMSEATS) 
		{
			//If there are open seats, the student takes a seat.
			studentsWaiting++;
			printf("Student %d takes a seat. Waiting students = %d\n", studentIndex+1, studentsWaiting);

			//Unlocking critical section.
			pthread_mutex_unlock(&mutex);

			//Available tutor thread unblocks to help student.
			sem_post(&semStudents);

			//Student thread waits until a tutor helps them.
			sem_wait(&semTutors);
			
			//Student is helped, increment times they've received help.
			helpCounter++;
		}
		else 
		{
			//Too many students looking for help.
			/*Unlock mutex because otherwise students will get stuck in chairs,
			  causing infinite looping from conflict.*/
			pthread_mutex_unlock(&mutex);
			printf("Student %d found no empty chair. Will try later\n", studentIndex+1);
			//Student just continues to work on their own.
		}
	}
	//Student has been helped for enough time, so they are done getting help meaning one less student at CSMC.
	printf("Student %d needs no more help. They are finished at CSMC.\n", studentIndex+1);
	numFinished++;
	pthread_exit(NULL);
}

void *tutorThread(void *index) 
{
	//Index of specific tutor thread in the array of thread IDs.
	int tutorIndex = (intptr_t)index;

	//A random number generated used for calculating time the tutor helps a student.
	unsigned int tutorWaitRNG = 5 * MAXSLEEP * NUMSTUDENTS * NUMHELPS * NUMSEATS;
	while(1) 
	{
		//Wait until a signal is received from student needing help.
		sem_wait(&semStudents);

		//Locking critical section.
		pthread_mutex_lock(&mutex);

		//Student helped, a seat opens up.
		studentsWaiting--;
		
		int tutorWaitTime = (rand_r(&tutorWaitRNG) % MAXSLEEP) + 1;
		printf("Tutor %d has been helping a student for %d seconds. Waiting students = %d\n", tutorIndex+1, tutorWaitTime, studentsWaiting);

		//Unlocking critical section.
		pthread_mutex_unlock(&mutex);

		//Helps student for a random amount of time, then sends a signal for the student to unblock.
		sem_post(&semTutors);
		sleep(tutorWaitTime);
	}
}
