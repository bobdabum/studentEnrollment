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

#define ENROLLMENT_WINDOW 120
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
int gsStart=0, rsStart=0,eeStart=0, gsSize = 0, rsSize = 0, eeSize = 0;
sem_t gsSem, rsSem, eeSem;

int timesUp = 0;
time_t startTime, endTime;
struct itimerval enrollmentTimer;

int remainingStudents[3];

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

		//adds to queue and records position
		gsQueue[gsStart+gsSize] = s->studentID;
		s->queuePos = gsStart+gsSize;
		
		//increment semaphore and queueSize
		gsSize++;
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
		
		rsQueue[rsStart+rsSize] = s->studentID;
		s->queuePos = rsStart+rsSize;
		
		rsSize++;
		sem_post(&rsSem);

		pthread_mutex_unlock(&(s->studentMutex));
		pthread_mutex_unlock(&rsMutex);
	}
	else{
		pthread_mutex_lock(&eeMutex);
		pthread_mutex_lock(&(s->studentMutex));

		printElapsedTime();
		printf("Student #%i.EE added to EE queue\n",s->studentID);

		eeQueue[eeStart+eeSize] = s->studentID;
		s->queuePos = eeStart+eeSize;
		
		eeSize++;
		sem_post(&eeSem);

		pthread_mutex_unlock(&(s->studentMutex));
		pthread_mutex_unlock(&eeMutex);
	}
}
void removeFromQueue(struct Student *s){
	if(!timesUp){		
		if(s->studentType == 0){
			pthread_mutex_lock(&gsMutex);
			//Modify Student Data
			pthread_mutex_lock(&(s->studentMutex));
			if(s->enrolled != 1 && s->queuePos==gsStart){				
				remainingStudents[0]--;
				s->queuePos = -1;
				s->enrolled = -2;
				
				//increments start of gs Queue decreases Size
				gsStart++; 
				gsSize--;
				
				//Prints information
				printElapsedTime();
				printf("Student #%i.GS has left due to impatience.\n",s->studentID);
				
				//assigns wait time to student
				s->waitTime = TIME_IMPATIENT;
			}
			pthread_mutex_unlock(&(s->studentMutex));
			pthread_mutex_unlock(&gsMutex);
		}
		else if(s->studentType == 1){
			pthread_mutex_lock(&rsMutex);
			pthread_mutex_lock(&(s->studentMutex));
			if(s->enrolled != 1 && s->queuePos==rsStart){				
				remainingStudents[1]--;
				s->queuePos = -1;
				s->enrolled = -2;
				//increments start of rs Queue decreases Size
				rsStart++;
				rsSize--;
				printElapsedTime();
				printf("Student #%i.RS has left due to impatience.\n",s->studentID);
				s->waitTime = TIME_IMPATIENT;
			}
			pthread_mutex_unlock(&(s->studentMutex));
			pthread_mutex_unlock(&rsMutex);
		}
		else{
			pthread_mutex_lock(&eeMutex);
			pthread_mutex_lock(&(s->studentMutex));
			if(s->enrolled != 1 && s->queuePos==eeStart){				
				remainingStudents[2]--;
				s->queuePos = -1;
				s->enrolled = -2;
				//increments start of ee Queue decreases Size
				eeStart++;
				eeSize--;
				printElapsedTime();
				printf("Student #%i.EE has left due to impatience.\n",s->studentID);
				s->waitTime = TIME_IMPATIENT;
			}
			pthread_mutex_unlock(&(s->studentMutex));
			pthread_mutex_unlock(&eeMutex);
		}
	}
}
void *createStudent(void *param){
	struct Student *s = (struct Student*) param;
	sleep(rand()%ENROLLMENT_WINDOW);
	time(&(s->arrivalTime));
	addToQueue(s);
	sleep(TIME_IMPATIENT);
	if(!timesUp){
		removeFromQueue(s);
	}
	return NULL;
}
int addStudent(int sectionDesired, int studentID){
	int hasBeenAdded= -1;
	if(sectionDesired == 0){
		pthread_mutex_lock(&class0Mutex);
		if(class0Num < SIZE_SECTION){
			class0[class0Num] = studentID;
			class0Num++;
			hasBeenAdded = 0;
		}
		pthread_mutex_unlock(&class0Mutex);
	}
	else if(sectionDesired == 1){
		pthread_mutex_lock(&class1Mutex);
		if(class1Num < SIZE_SECTION){
			class1[class1Num] = studentID;
			class1Num++;
			hasBeenAdded = 1;
		}
		pthread_mutex_unlock(&class1Mutex);
	}
	else if(sectionDesired == 2){
		pthread_mutex_lock(&class2Mutex);
		if(class2Num < SIZE_SECTION){
			class2[class2Num] = studentID;
			class2Num++;
			hasBeenAdded = 2;
		}
		pthread_mutex_unlock(&class2Mutex);
	}
	else{
		pthread_mutex_lock(&class0Mutex);
		pthread_mutex_lock(&class1Mutex);
		pthread_mutex_lock(&class2Mutex);
		//Adds Student to class of smallest size less than 20
		if((class0Num+class1Num+class2Num)<(SIZE_SECTION*NUM_SECTIONS)){
			if(class0Num<= class1Num && class0Num<=class2Num && class0Num<20){
				class0[class0Num] = studentID;
				class0Num++;
				hasBeenAdded = 0;
			}
			else if(class1Num<=class0Num && class1Num<=class2Num && class1Num<20){
				class1[class1Num] = studentID;
				class1Num++;
				hasBeenAdded = 1;
			}
			else if(class2Num < 20){
				class2[class2Num] = studentID;
				class2Num++;
				hasBeenAdded = 2;
			}
		}
		pthread_mutex_unlock(&class2Mutex);
		pthread_mutex_unlock(&class1Mutex);
		pthread_mutex_unlock(&class0Mutex);
	}
	return hasBeenAdded;
}
void gsQueueRun(){
	if(!timesUp && remainingStudents[0]>0){
		//wait on gs Semaphore
		sem_wait(&gsSem);
		
		// Locking gs mutex and record the start location
		pthread_mutex_lock(&gsMutex);
		int start = gsStart;
		printElapsedTime();
		if(allStudents[gsQueue[gsStart]].sectionDesired!=3){
			printf("Trying to add student #%i.GS to section %i.\n",allStudents[gsQueue[gsStart]].studentID, allStudents[gsQueue[gsStart]].sectionDesired);
		}
		else{
			printf("Trying to add student #%i.GS to any section.\n",allStudents[gsQueue[gsStart]].studentID);
		}
		pthread_mutex_unlock(&gsMutex);
		

		//Waits Random time b/t 1 to 2 seconds
		sleep(rand()%2+1);

		//relocks mutex and make sure that the start hasn't changed
		pthread_mutex_lock(&gsMutex);		
		if(gsStart == start && remainingStudents[0]>0){			
			remainingStudents[0]--;
			pthread_mutex_lock(&(allStudents[gsQueue[gsStart]].studentMutex));
			
			int sectionDesired = allStudents[gsQueue[gsStart]].sectionDesired;
			int studentID = allStudents[gsQueue[gsStart]].studentID;
			//Tries to add to section. Need to make sure student hasn't been impatient
			
			int addedSuccessfully = 0;
			if(allStudents[gsQueue[gsStart]].enrolled != -2){
				addedSuccessfully = addStudent(sectionDesired,studentID);
			}
			//Prints event, either success or drop
			if(addedSuccessfully>=0) {
				allStudents[gsQueue[gsStart]].enrolled = 1;
				printElapsedTime();                
				printf("Student #%i.GS has been added to class %i.\n", allStudents[gsQueue[gsStart]].studentID, addedSuccessfully);
			} else {
				allStudents[gsQueue[gsStart]].queuePos = -1;
				allStudents[gsQueue[gsStart]].enrolled = -1;
				printElapsedTime();                
				printf("Student #%i.GS has been removed from GS queue due to lack of space.\n", allStudents[gsQueue[gsStart]].studentID);
			}
			
			//assigns wait time to student
			time_t now;
			time(&now);			
			double elapsedTime = difftime(now, allStudents[gsQueue[gsStart]].arrivalTime);
			allStudents[gsQueue[gsStart]].waitTime = (int) elapsedTime;
			
			//unlock student lock
			pthread_mutex_unlock(&(allStudents[gsQueue[gsStart]].studentMutex));
			
			//removes student
			gsSize--;
			gsStart++;
		}
		//Unlocks mutex.
		pthread_mutex_unlock(&gsMutex);
	}
}
void rsQueueRun(){
	if(!timesUp && remainingStudents[1]>0){
		//wait on rs Semaphore
		sem_wait(&rsSem);
		
		// Locking gs mutex and record the start location
		pthread_mutex_lock(&rsMutex);
		int start = rsStart;
		printElapsedTime();
		if(allStudents[rsQueue[rsStart]].sectionDesired!=3){
			printf("Trying to add student #%i.RS to section %i.\n",allStudents[rsQueue[rsStart]].studentID, allStudents[rsQueue[rsStart]].sectionDesired);
		}
		else{
			printf("Trying to add student #%i.RS to any section.\n",allStudents[rsQueue[rsStart]].studentID);
		}
		pthread_mutex_unlock(&rsMutex);
		

		//Waits Random time b/t 2,3,4 seconds
		sleep(rand()%3+2);

		//relocks mutex and make sure that the start hasn't changed
		pthread_mutex_lock(&rsMutex);		
		if(rsStart == start && remainingStudents[1]>0){
			remainingStudents[1]--;
			pthread_mutex_lock(&(allStudents[rsQueue[rsStart]].studentMutex));
			
			int sectionDesired = allStudents[rsQueue[rsStart]].sectionDesired;
			int studentID = allStudents[rsQueue[rsStart]].studentID;
			//Tries to add to section. Need to make sure student hasn't been impatient
			
			int addedSuccessfully = 0;
			if(allStudents[rsQueue[rsStart]].enrolled != -2){
				addedSuccessfully = addStudent(sectionDesired,studentID);
			}
			//Prints event, either success or drop
			if(addedSuccessfully>=0) {
				allStudents[rsQueue[rsStart]].enrolled = 1;
				printElapsedTime();                
				printf("Student #%i.RS has been added to class %i.\n", allStudents[rsQueue[rsStart]].studentID, addedSuccessfully);
			} else {
				allStudents[rsQueue[rsStart]].queuePos = -1;
				allStudents[rsQueue[rsStart]].enrolled = -1;
				printElapsedTime();                
				printf("Student #%i.EE has been removed from EE queue due to lack of space.\n", allStudents[rsQueue[rsStart]].studentID);
			}
			
			//assigns wait time to student
			time_t now;
			time(&now);			
			double elapsedTime = difftime(now, allStudents[rsQueue[rsStart]].arrivalTime);
			allStudents[rsQueue[rsStart]].waitTime = (int) elapsedTime;
			
			//unlock student lock
			pthread_mutex_unlock(&(allStudents[rsQueue[rsStart]].studentMutex));
			
			//removes student
			rsSize--;
			rsStart++;
		}
		//Unlocks mutex.
		pthread_mutex_unlock(&rsMutex);
	}
}
void eeQueueRun(){
	if(!timesUp && remainingStudents[2]>0){
		//wait on ee Semaphore
		sem_wait(&eeSem);
		
		// Locking ee mutex and record the start location
		pthread_mutex_lock(&eeMutex);
		int start = eeStart;
		printElapsedTime();
		if(allStudents[eeQueue[eeStart]].sectionDesired!=3){
			printf("Trying to add student #%i.EE to section %i.\n",allStudents[eeQueue[eeStart]].studentID, allStudents[eeQueue[eeStart]].sectionDesired);
		}
		else{			
			printf("Trying to add student #%i.EE to any section.\n",allStudents[eeQueue[eeStart]].studentID);
		}
		pthread_mutex_unlock(&eeMutex);
		

		//Waits Random time b/t 3,4,5,6 seconds
		sleep(rand()%4+3);

		//relocks mutex and make sure that the start hasn't changed
		pthread_mutex_lock(&eeMutex);		
		if(eeStart == start && remainingStudents[2]>0){
			remainingStudents[2]--;
			pthread_mutex_lock(&(allStudents[eeQueue[eeStart]].studentMutex));
			
			int sectionDesired = allStudents[eeQueue[eeStart]].sectionDesired;
			int studentID = allStudents[eeQueue[eeStart]].studentID;
			//Tries to add to section. Need to make sure student hasn't been impatient
			
			int addedSuccessfully = 0;
			if(allStudents[eeQueue[eeStart]].enrolled != -2){
				addedSuccessfully = addStudent(sectionDesired,studentID);
			}
			//Prints event, either success or drop
			if(addedSuccessfully>=0) {
				allStudents[eeQueue[eeStart]].enrolled = 1;
				printElapsedTime();                
				printf("Student #%i.EE has been added to class %i.\n", allStudents[eeQueue[eeStart]].studentID, addedSuccessfully);
			} else {
				allStudents[eeQueue[eeStart]].queuePos = -1;
				allStudents[eeQueue[eeStart]].enrolled = -1;
				printElapsedTime();                
				printf("Student #%i.EE has been removed from EE queue due to lack of space.\n", allStudents[eeQueue[eeStart]].studentID);
			}
			
			//assigns wait time to student
			time_t now;
			time(&now);			
			double elapsedTime = difftime(now, allStudents[eeQueue[eeStart]].arrivalTime);
			allStudents[eeQueue[eeStart]].waitTime = (int) elapsedTime;
			
			//unlock student lock
			pthread_mutex_unlock(&(allStudents[eeQueue[eeStart]].studentMutex));
			
			//removes student
			eeSize--;
			eeStart++;
		}
		//Unlocks mutex.
		pthread_mutex_unlock(&eeMutex);
	}
}
void *gsQueueStart(void *param){
	printf("Queue for Graduating Seniors Starts.\n");
	do {
		gsQueueRun();
	} while (!timesUp);
	printElapsedTime();
	printf("Queue for Graduating Seniors Ends.\n");
	return NULL;
}
void *rsQueueStart(void *param){
	printf("Queue for Regular Seniors Starts.\n");
	do {
		rsQueueRun();
	} while (!timesUp);
	printElapsedTime();
	printf("Queue for Regular Seniors Ends.\n");
	return NULL;
}
void *eeQueueStart(void *param){
	printf("Queue for Everyone Else Starts.\n");
	do {
		eeQueueRun();
	} while (!timesUp);
	printElapsedTime();
	printf("Queue for Everyone Else Ends.\n");
	return NULL;
}
// Timer signal handler.
void timerHandler(int signal)
{
	printf("Time Over\n");
	timesUp = 1;
	sem_post(&gsSem);
	sem_post(&rsSem);
	sem_post(&eeSem);
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
		remainingStudents[allStudents[a].studentType]++;
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

	// Set & starts the timer for for office hour duration.
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
	time(&endTime);
	
	int numDropped = 0, numLeft = 0, numNever=0, numGS = 0, numRS = 0, numEE = 0;
	double waitTimeGS = 0,waitTimeRS = 0,waitTimeEE = 0;
	printf("\n--------------------\nSection 0 students:\n");
	for(a=0;a<class0Num;a++){
		printf("%i,",class0[a]);
	}
	printf("\n--------------------\nSection 1 students:\n");
	for(a=0;a<class1Num;a++){
		printf("%i,",class1[a]);
	}
	printf("\n--------------------\nSection 2 students:\n");
	for(a=0;a<class2Num;a++){
		printf("%i,",class2[a]);
	}
	
	printf("\n--------------------\nWait Times\n");
	for(a=0; a<NUM_STUDENTS;a++){
		if(allStudents[a].enrolled==0){
			numNever++;
			if(allStudents[a].studentType ==0){
				numGS++;
				waitTimeGS += difftime(endTime, allStudents[a].arrivalTime);
				printf("Student #%i.GS was never placed. Wait time:%02d\n",a,difftime(endTime, allStudents[a].arrivalTime));
			}
			else if(allStudents[a].studentType ==1){
				numRS++;
				waitTimeRS += difftime(endTime, allStudents[a].arrivalTime);
				printf("Student #%i.RS was never placed. Wait time:%02d\n",a,difftime(endTime, allStudents[a].arrivalTime));
			}
			else{
				numEE++;
				waitTimeEE += difftime(endTime, allStudents[a].arrivalTime);
				printf("Student #%i.EE was never placed. Wait time:%02d\n",a,difftime(endTime, allStudents[a].arrivalTime));
			}
		}
		if(allStudents[a].enrolled==-1){
			numDropped++;						
			if(allStudents[a].studentType ==0){
				numGS++;
				waitTimeGS += allStudents[a].waitTime;
				printf("Student #%i.GS could not be placed because class was full. Wait time:%is\n",a,allStudents[a].waitTime);
			}
			else if(allStudents[a].studentType ==1){
				numRS++;
				waitTimeRS += allStudents[a].waitTime;
				printf("Student #%i.RS could not be placed because class was full. Wait time:%is\n",a,allStudents[a].waitTime);
			}
			else{
				numEE++;
				waitTimeEE += allStudents[a].waitTime;
				printf("Student #%i.EE could not be placed because class was full. Wait time:%is\n",a,allStudents[a].waitTime);
			}
		}
		else if(allStudents[a].enrolled==-2){
			numLeft++;
			if(allStudents[a].studentType ==0){
				numGS++;
				waitTimeGS += TIME_IMPATIENT;
				printf("Student #%i.GS left impatiently. Wait time:%is\n",a,TIME_IMPATIENT);
			}
			else if(allStudents[a].studentType ==1){
				numRS++;
				waitTimeRS += allStudents[a].waitTime;
				printf("Student #%i.RS left impatiently. Wait time:%is\n",a,TIME_IMPATIENT);
			}
			else{
				numEE++;
				waitTimeEE += allStudents[a].waitTime;
				printf("Student #%i.EE left impatiently. Wait time:%is\n",a,TIME_IMPATIENT);
			}
		}
		else{
			if(allStudents[a].studentType ==0){
				numGS++;
				waitTimeGS += allStudents[a].waitTime;
				printf("Student #%i.GS placed. Wait time:%i\n",a,allStudents[a].waitTime);
			}
			else if(allStudents[a].studentType ==1){
				numRS++;
				waitTimeRS += allStudents[a].waitTime;
				printf("Student #%i.RS placed. Wait time:%i\n",a,allStudents[a].waitTime);
			}
			else{
				numEE++;
				waitTimeEE += allStudents[a].waitTime;
				printf("Student #%i.EE placed. Wait time:%i\n",a,allStudents[a].waitTime);
			}
		}
	}
		printf("\n--------------------\nAverage Wait Times\n");
		printf("GS Queue %.2fs; RS Queue %.2fs; EE Queue %.2fs;\n",waitTimeGS/numGS, waitTimeRS/numRS, waitTimeEE/numEE);
		printf("Final Stats\n");
		printf("Number Placed: %i.\nNumber not processed/failed to place: %i.\nNumber left impatiently: %i\n",class0Num+class1Num+class2Num, numDropped+numNever, numLeft);
}
