// Student Enrollment CS149 Team D.B.An

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <sys/time.h>
#include <string.h>

#define NUM_STUDENTS 75
#define NUM_STUDENT_PRIORITY 3
#define NUM_STUDENT_CLASS_TYPES 4

#define NUM_SECTIONS 3
#define SIZE_SECTION 20

#define ENROLLMENT_WINDOW 60
#define RAND_SEED 0
#define TIME_IMPATIENT 10
#define ID_BASE = 101

int class0[SIZE_SECTION];
int class1[SIZE_SECTION];
int class2[SIZE_SECTION];
pthread_mutex_t class0Mutex,class1Mutex,class2Mutex, printMutex;
int class0Num = 0, class1Num = 0, class2Num = 0;

int gsQueue[NUM_STUDENTS];
int rsQueue[NUM_STUDENTS];
int eeQueue[NUM_STUDENTS];
pthread_mutex_t gsMutex, rsMutex, eeMutex;
sem_t gsSem, rsSem, eeSem;

int timesUp = 0;
time_t startTime;
struct itimerval enrollmentTimer;

struct Student{
	int studentID;
	int studentType;
	int sectionDesired;
	int queuePos;
	int enrolled;
	int waitTime;
	pthread_mutex_t studentMutex;
	time_t arrivalTime;
};
struct Student allStudents[NUM_STUDENTS];

void printElapsedTime(){
	time_t now;
	time(&now);
	double elapsedTime = difftime(now,startTime);
	int min = 0;
	int sec = (int)elapsedTime;
	while(sec >= 60) {
		min++;
		sec -= 60;
	}
	// Elapsed time.
	printf("%1d:%02d | ", min, sec);
}

void addToQueue(struct Student *s){
	if(s->studentType == 0){
		//locks student and queue Mutex
		pthread_mutex_lock(&gsMutex);
		pthread_mutex_lock(&(s->studentMutex));

		printElapsedTime();
		printf("Student #%i.GS added to GS queue\n",s->studentID);

		//gets semaphore value of queue and sets that as queue pos of student
		sem_getvalue(&gsSem,&(s->queuePos));
		printf("GS Queue Position:%i\n",s->queuePos);
		//adds to queue
		gsQueue[s->queuePos] = s->studentID;
		//increment semaphore
		sem_post(&gsSem);

		//unlock mutex
		pthread_mutex_unlock(&(s->studentMutex));
		pthread_mutex_unlock(&gsMutex);
	}
	else if(s-> studentType == 1){
		pthread_mutex_lock(&rsMutex);
		pthread_mutex_lock(&(s->studentMutex));

		printElapsedTime();
		printf("Student #%i.RS added to RS queue\n",s->studentID);

		sem_getvalue(&rsSem,&(s->queuePos));
		printf("RS Queue Position:%i\n",s->queuePos);
		rsQueue[s->queuePos] = s->studentID;
		sem_post(&rsSem);

		pthread_mutex_unlock(&(s->studentMutex));
		pthread_mutex_unlock(&rsMutex);
	}
	else{
		pthread_mutex_lock(&eeMutex);
		pthread_mutex_lock(&(s->studentMutex));

		printElapsedTime();
		printf("Student #%i.EE added to EE queue\n",s->studentID);

		sem_getvalue(&eeSem,&(s->queuePos));
		printf("EE Queue Position:%i\n",s->queuePos);
		eeQueue[s->queuePos] = s->studentID;
		sem_post(&eeSem);

		pthread_mutex_unlock(&(s->studentMutex));
		pthread_mutex_unlock(&eeMutex);
	}
}
void removeFromQueue(struct Student *s){
	int studentRemoved = 0;
	int queuePos = 0;
	if(s->studentType == 0){
		pthread_mutex_lock(&gsMutex);
		//Modify Student Data
		pthread_mutex_lock(&(s->studentMutex));
		if(s->enrolled != 1){
			studentRemoved = 1;
			queuePos = s->queuePos;
			s->queuePos = -1;
			s->enrolled = -1;
  printf("Student %i's impatient, trying to remove from queue\n",s->studentID);
		}
		pthread_mutex_unlock(&(s->studentMutex));
		
		//Move Rest of Students
		if(studentRemoved){		
			//removes Student
			int i;
			for(i = queuePos;i<NUM_STUDENTS-1;i++){
				if(gsQueue[i+1]!=-1 && (allStudents[gsQueue[i+1]].studentType)==0){
					pthread_mutex_lock(&(allStudents[gsQueue[i+1]].studentMutex));
					allStudents[gsQueue[i+1]].queuePos = i;
					gsQueue[i] = gsQueue[i+1];
					pthread_mutex_unlock(&(allStudents[gsQueue[i+1]].studentMutex));
				}
				else{
					gsQueue[i] = gsQueue[i+1];
					break;
				}
			}
		}
		pthread_mutex_unlock(&gsMutex);
	}
	else if(s->studentType == 1){
		pthread_mutex_lock(&rsMutex);
		pthread_mutex_lock(&(s->studentMutex));
		if(s->enrolled != 1){
			studentRemoved = 1;
			queuePos = s->queuePos;
			s->queuePos = -1;
			s->enrolled = -1;
		}
		pthread_mutex_unlock(&(s->studentMutex));
		
		//Move Rest of Students
		if(studentRemoved){		
			//removes Student
			int i;
			for(i = queuePos;i<NUM_STUDENTS-1;i++){
				if(rsQueue[i+1]!=-1 && (allStudents[rsQueue[i+1]].studentType)==0){
					pthread_mutex_lock(&(allStudents[rsQueue[i+1]].studentMutex));
					allStudents[rsQueue[i+1]].queuePos = i;
					rsQueue[i] = rsQueue[i+1];
					pthread_mutex_unlock(&(allStudents[rsQueue[i+1]].studentMutex));
				}
				else{
					rsQueue[i] = rsQueue[i+1];
					break;
				}
			}
		}
		pthread_mutex_unlock(&rsMutex);
	}
	else{
		pthread_mutex_lock(&eeMutex);
		pthread_mutex_lock(&(s->studentMutex));
		if(s->enrolled != 1){
			studentRemoved = 1;
			queuePos = s->queuePos;
			s->queuePos = -1;
			s->enrolled = -1;
		}
		pthread_mutex_unlock(&(s->studentMutex));
		
		//Move Rest of Students
		if(studentRemoved){		
			//removes Student
			int i;
			for(i = queuePos;i<NUM_STUDENTS-1;i++){
				if(eeQueue[i+1]!=-1 && (allStudents[eeQueue[i+1]].studentType)==0){
					pthread_mutex_lock(&(allStudents[eeQueue[i+1]].studentMutex));
					allStudents[eeQueue[i+1]].queuePos = i;
					eeQueue[i] = eeQueue[i+1];
					pthread_mutex_unlock(&(allStudents[eeQueue[i+1]].studentMutex));
				}
				else{
					eeQueue[i] = eeQueue[i+1];
					break;
				}
			}
		}
		pthread_mutex_unlock(&eeMutex);
	}
}
void *createStudent(void *param){
	struct Student *s = (struct Student*) param;
	sleep(rand()%ENROLLMENT_WINDOW);
	time(&(s->arrivalTime));
	addToQueue(s);
	sleep(TIME_IMPATIENT);
	removeFromQueue(s);
	return NULL;
}
int addStudent(int sectionDesired, int studentID){
  printf("Trying to add student:%i. Section Desired:%i\n",studentID, sectionDesired);
	int hasBeenAdded= 0;
	if(sectionDesired == 0){
		pthread_mutex_lock(&class0Mutex);
		if(class0Num < SIZE_SECTION){
			class0[class0Num] = studentID;
			class0Num++;
			pthread_mutex_unlock(&class0Mutex);
			hasBeenAdded = 1;
		}
	}
	else if(sectionDesired == 1){
		pthread_mutex_lock(&class1Mutex);
		if(class1Num < SIZE_SECTION){
			class1[class1Num] = studentID;
			class1Num++;
			pthread_mutex_unlock(&class1Mutex);
			hasBeenAdded = 1;
		}
	}
	else if(sectionDesired == 2){
		pthread_mutex_lock(&class2Mutex);
		if(class2Num < SIZE_SECTION){
			class2[class2Num] = studentID;
			class2Num++;
			pthread_mutex_unlock(&class2Mutex);
			hasBeenAdded = 1;
		}
	}
	else{
		pthread_mutex_lock(&class0Mutex);
		pthread_mutex_lock(&class1Mutex);
		pthread_mutex_lock(&class2Mutex);
		//Adds Student to class of smallest size less than 20
		printf("Total Number of students:%i; class0:%i; class1:%i; class2:%i\n",(class0Num+class1Num+class2Num),class0Num,class1Num,class2Num);
		if((class0Num+class1Num+class2Num)<(SIZE_SECTION*NUM_SECTIONS)){
			if(class0Num<= class1Num && class0Num<=class2Num && class0Num<20){
				class0[class0Num] = studentID;
				class0Num++;
				hasBeenAdded = 1;
			}
			else if(class1Num<=class0Num && class1Num<=class2Num && class1Num<20){
				class1[class1Num] = studentID;
				class1Num++;
				hasBeenAdded = 1;
			}
			else if(class2Num < 20){
				class2[class2Num] = studentID;
				class2Num++;
				hasBeenAdded = 1;
			}
		}
		pthread_mutex_unlock(&class2Mutex);
		pthread_mutex_unlock(&class1Mutex);
		pthread_mutex_unlock(&class0Mutex);
	}
	return hasBeenAdded;
}
void gsQueueRun(){
	if(!timesUp){
		//Waits Random time b/t 1 to 2 seconds
		sleep(rand()%2+1);
		char addedOrDropped[20]=""; 
		//wait on gs Semaphore
		sem_wait(&gsSem);
		
		// Locking gs mutex and student mutex to add student to class.
		pthread_mutex_lock(&gsMutex);
		pthread_mutex_lock(&(allStudents[gsQueue[0]].studentMutex));

		int sectionDesired = allStudents[gsQueue[0]].sectionDesired;
		int studentID = allStudents[gsQueue[0]].studentID;

		

		//Tries to add to section. Need to make sure student hasn't been impatient
		int addedSuccessfully = 0;
		if(allStudents[gsQueue[0]].enrolled != -1){
			addedSuccessfully = addStudent(sectionDesired,studentID);
		}
		if(addedSuccessfully) {
			strcat(addedOrDropped,"added");
			allStudents[gsQueue[0]].enrolled = 1;
		} else {
			strcat(addedOrDropped,"dropped");
			allStudents[gsQueue[0]].queuePos = -1;
			allStudents[gsQueue[0]].enrolled = -1;
		}
		//Prints event, either success or drop
		printElapsedTime();               
		printf("Student #%i.GS has been %s to class.\n", allStudents[gsQueue[0]].studentID, addedOrDropped);

		//assigns wait time to student
		time_t now;
		time(&now);
		
		double elapsedTime = difftime(now, allStudents[gsQueue[0]].arrivalTime);
		allStudents[gsQueue[0]].waitTime = (int) elapsedTime;
		//unlock student lock
		pthread_mutex_unlock(&(allStudents[gsQueue[0]].studentMutex));

		int i = 0;
		//removes Student
		for(i=1;i<NUM_STUDENTS;i++){
			if(gsQueue[i]!=-1 && (allStudents[gsQueue[i]].studentType)==0){
				pthread_mutex_lock(&(allStudents[gsQueue[i]].studentMutex));
				allStudents[gsQueue[i]].queuePos = i-1;
				gsQueue[i-1] = gsQueue[i];
				pthread_mutex_unlock(&(allStudents[gsQueue[i]].studentMutex));
			}
			else{
				gsQueue[i-1] = gsQueue[i];
				break;
			}
		}
		//Unlocks mutex.
		pthread_mutex_unlock(&gsMutex);
	}
}
void rsQueueRun(){  
	if(!timesUp){
		//Waits Random time b/t 2,3,4 seconds
		sleep(rand()%3+2);
		char addedOrDropped[20]=""; 
		//wait on gs Semaphore
		sem_wait(&rsSem);		

		// Locking gs mutex and student mutex to add student to class.
		pthread_mutex_lock(&rsMutex);
		pthread_mutex_lock(&(allStudents[rsQueue[0]].studentMutex));

		int sectionDesired = allStudents[rsQueue[0]].sectionDesired;
		int studentID = allStudents[rsQueue[0]].studentID;

		

		//Tries to add to section. Need to make sure student hasn't been impatient
		int addedSuccessfully = 0;
		if(allStudents[rsQueue[0]].enrolled != -1){
			addedSuccessfully = addStudent(sectionDesired,studentID);
		}
		if(addedSuccessfully) {
			strcat(addedOrDropped,"added");
			allStudents[rsQueue[0]].enrolled = 1;
		} else {
			strcat(addedOrDropped,"dropped");
			allStudents[rsQueue[0]].queuePos = -1;
			allStudents[rsQueue[0]].enrolled = -1;
		}
		//Prints event, either success or drop
		printElapsedTime();                
		printf("Student #%i.RS has been %s to class.\n", allStudents[rsQueue[0]].studentID, addedOrDropped);
		
		//assigns wait time to student
		time_t now;
		time(&now);
		double elapsedTime = difftime(now, allStudents[rsQueue[0]].arrivalTime);
		allStudents[rsQueue[0]].waitTime = (int) elapsedTime;
		//unlock student lock
		pthread_mutex_unlock(&(allStudents[rsQueue[0]].studentMutex));

		int i = 0;
		//removes Student
		for(i=1;i<NUM_STUDENTS;i++){
			if(rsQueue[i]!=-1 && (allStudents[rsQueue[i]].studentType)==0){
				pthread_mutex_lock(&(allStudents[rsQueue[i]].studentMutex));
				allStudents[rsQueue[i]].queuePos = i-1;
				rsQueue[i-1] = rsQueue[i];
				pthread_mutex_unlock(&(allStudents[rsQueue[i]].studentMutex));
			}
			else{
				rsQueue[i-1] = rsQueue[i];
				break;
			}
		}

		//Unlocks mutex.
		pthread_mutex_unlock(&rsMutex);
	}
}
void eeQueueRun(){
	if(!timesUp){
		//Waits Random time b/t 3,4,5,6 seconds
		sleep(rand()%4+3);
		char addedOrDropped[20]=""; 
		//wait on ee Semaphore
		sem_wait(&eeSem);
		
		// Locking ee mutex and student mutex to add student to class.
		pthread_mutex_lock(&eeMutex);
		pthread_mutex_lock(&(allStudents[eeQueue[0]].studentMutex));

		int sectionDesired = allStudents[eeQueue[0]].sectionDesired;
		int studentID = allStudents[eeQueue[0]].studentID;

		
		
		//Tries to add to section. Need to make sure student hasn't been impatient
		int addedSuccessfully = 0;
		if(allStudents[eeQueue[0]].enrolled != -1){
			addedSuccessfully = addStudent(sectionDesired,studentID);
		}
		if(addedSuccessfully) {
			strcat(addedOrDropped,"added");
			allStudents[eeQueue[0]].enrolled = 1;
		} else {
			strcat(addedOrDropped,"dropped");
			allStudents[eeQueue[0]].queuePos = -1;
			allStudents[eeQueue[0]].enrolled = -1;
		}
		//Prints event, either success or drop
		printElapsedTime();                
		printf("Student #%i.EE has been %s to class.\n", allStudents[eeQueue[0]].studentID, addedOrDropped);
		
		//assigns wait time to student
		time_t now;
		time(&now);
		double elapsedTime = difftime(now, allStudents[eeQueue[0]].arrivalTime);
		allStudents[eeQueue[0]].waitTime = (int) elapsedTime;
		//unlock student lock
		pthread_mutex_unlock(&(allStudents[eeQueue[0]].studentMutex));

		int i = 0;
		//removes Student
		for(i=1;i<NUM_STUDENTS;i++){
			if(eeQueue[i]!=-1 && (allStudents[eeQueue[i]].studentType)==0){
				pthread_mutex_lock(&(allStudents[eeQueue[i]].studentMutex));
				allStudents[eeQueue[i]].queuePos = i-1;
				eeQueue[i-1] = eeQueue[i];
				pthread_mutex_unlock(&(allStudents[eeQueue[i]].studentMutex));
			}
			else{
				eeQueue[i-1] = eeQueue[i];
				break;
			}
		}

		//Unlocks mutex.
		pthread_mutex_unlock(&eeMutex);
	}
}
void *gsQueueStart(void *param){
	printf("Queue for Graduating Seniors Starts\n");
	do {
		gsQueueRun();
	} while (!timesUp);
	printElapsedTime();
	printf("Queue for Graduating Seniors Ends\n");
	return NULL;
}
void *rsQueueStart(void *param){
	printf("Queue for Regular Seniors Starts\n");
	do {
		rsQueueRun();
	} while (!timesUp);
	printElapsedTime();
	printf("Queue for Regular Seniors Ends\n");
	return NULL;
}
void *eeQueueStart(void *param){
	printf("Queue for Everyone Else Starts\n");
	do {
		eeQueueRun();
	} while (!timesUp);
	printElapsedTime();
	printf("Queue for Everyone Else Ends\n");
	return NULL;
}
// Timer signal handler.
void timerHandler(int signal)
{
	timesUp = 1;
}
int main(int argc, char *argv[])
{
	//Seed rng with RAND_SEED
	srand(RAND_SEED);

	//Initialize Semaphores
	sem_init(&eeSem,0,0);
	sem_init(&rsSem,0,0);
	sem_init(&gsSem,0,0);

	//Initialize Queue Threads
	pthread_t gsThreadID, rsThreadID, eeThreadID;
	pthread_attr_t gsAttr, rsAttr, eeAttr;
	int gsID = 0;
	int rsID = 1;
	int eeID = 2;
	pthread_attr_init(&gsAttr);
	pthread_attr_init(&rsAttr);
	pthread_attr_init(&eeAttr);
	pthread_create(&gsThreadID, &gsAttr, gsQueueStart, &gsID);
	pthread_create(&rsThreadID, &rsAttr, rsQueueStart, &rsID);
	pthread_create(&eeThreadID, &eeAttr, eeQueueStart, &eeID);

	//Initialize Student Threads and make queues empty
	int a;
	for(a=0; a<NUM_STUDENTS; a++){
		//sets value to -1 to identify empty slots
		gsQueue[a] = -1;
		rsQueue[a] = -1;
		eeQueue[a] = -1;

		//randomizes student attributes
		allStudents[a].studentID = a;
		allStudents[a].studentType = rand() % NUM_STUDENT_PRIORITY;
		allStudents[a].sectionDesired = rand() % NUM_STUDENT_CLASS_TYPES;
		allStudents[a].queuePos = -1;
		allStudents[a].enrolled = 0;

		//addToQueue(&allStudents[a]); testing purposes
		//Create student thread
		pthread_t studentThreadId;
		pthread_attr_t studentAttr;
		pthread_attr_init(&studentAttr);
		pthread_create(&studentThreadId, &studentAttr, createStudent, &(allStudents[a]));
	}

	// Set & starts the timer for for enrollment duration.
	enrollmentTimer.it_value.tv_sec = ENROLLMENT_WINDOW;
	setitimer(ITIMER_REAL, &enrollmentTimer, NULL);

	// Set the timer signal handler.
	signal(SIGALRM, timerHandler);

	//Set startTime
	time(&startTime);

	//waits for Everyone Else Queue to finish.
	pthread_join(gsThreadID,NULL);
	pthread_join(rsThreadID,NULL);
	pthread_join(eeThreadID,NULL);
}
