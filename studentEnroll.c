// Student Enrollment CS149 Team D.B.An

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <sys/time.h>

#define NUM_STUDENTS 75
#define NUM_STUDENT_PRIORITY 3
#define NUM_STUDENT_CLASS_TYPES 4

#define NUM_SECTIONS 3
#define SIZE_SECTION 20

#define ENROLLMENT_WINDOW 120
#define RAND_SEED 0
#define TIME_IMPATIENT 10

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
  pthread_mutex_t studentMutex;
  time_t arrivalTime;
};
struct Student allStudents[NUM_STUDENTS];

void printElapsedTime(){
  time_t now;
  time(&now);
  double elapsedTime = difftime(startTime,now);
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

    printf("Student #%i.GS added to GS queue\n",s->studentID);

    //gets semaphore value of queue and sets that as queue pos of student
    sem_getvalue(&gsSem,&(s->queuePos));
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

    printf("Student #%i.RS added to RS queue\n",s->studentID);

    sem_getvalue(&rsSem,&(s->queuePos));
    rsQueue[s->queuePos] = s->studentID;
    sem_post(&rsSem);

    pthread_mutex_unlock(&(s->studentMutex));
    pthread_mutex_unlock(&rsMutex);
  }
  else{
    pthread_mutex_lock(&eeMutex);
    pthread_mutex_lock(&(s->studentMutex));

    printf("Student #%i.EE added to EE queue\n",s->studentID);

    sem_getvalue(&eeSem,&(s->queuePos));
    eeQueue[s->queuePos] = s->studentID;
    sem_post(&eeSem);

    pthread_mutex_unlock(&(s->studentMutex));
    pthread_mutex_unlock(&eeMutex);
  }
}
void removeFromQueue(struct Student *s){
}
void createStudent(struct Student *s){
  sleep(rand()%ENROLLMENT_WINDOW);
  time(&(s->arrivalTime));
  addToQueue(s);
  sleep(TIME_IMPATIENT);
  removeFromQueue(s);
}
int addStudent(int sectionDesired, int studentID){
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
      if((class0Num+class1Num+class2Num)<(SIZE_SECTION*NUM_SECTIONS)){
        if(class0Num< class1Num && class0Num<class2Num && class0Num<20){
          class0[class0Num] = studentID;
          class0Num++;
          hasBeenAdded = 1;
        }
        else if(class1Num<class0Num && class1Num<class2Num && class1Num<20){
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
      pthread_mutex_lock(&class0Mutex);
      return hasBeenAdded;
    }
}
void gsQueueRun(){
  if(!timesUp){
    char[] addedOrDropped = ""; 
    //wait on gs Semaphore
    sem_wait(&gsSem);
    // Locking gs mutex to add student to class.
    pthread_mutex_lock(&gsMutex);
    int sectionDesired = allStudents[gsQueue[0]]->sectionDesired;
    int studentID = allStudents[gsQueue[0]]->studentID;
    
    //Waits Random time b/t 1 to 2 seconds
    sleep(rand()%2+1)
    //Tries to add to section
	int addedSuccessfully = addStudent(sectionDesired,studentID);
    if(addedSuccessfully) {
      addedOrDropped = "added";
      allStudents[gsQueue[0]]->enrolled = 1;
    } else {
      addedOrDropped = "dropped";
    }
    
    int i = 0;
    //removes Student
    for(i=1;i<NUM_STUDENTS;i++){
      if((allStudents[gsQueue[i]]->studentType)==0){
        allStudents[gsQueue[i]]->position
        gsQueue[i-1] = gsQueue[i];
      }
      else{
        gsQueue[i-1] = gsQueue[i];
        break;
      }
    }
    //Prints event, either success or drop
    printElapsedTime();                
    printf("Student: %i has been %s\n", allStudents[gsQueue[0]]->studentID, addedOrDropped);
    
    //Unlocks mutex.
    pthread_mutex_unlock(&gsMutex);
  }
}
void rsQueueRun(){
  
}
void eeQueueRun(){
}
//NOTE: look into why Queuestarts are needed?
void gsQueueStart(){
	print("Queue for Graduating Seniors Starts");
    do {
        gsQueueRun();
    } while (!timesUp);
    print("Queue for Graduating Seniors Ends");
}
void rsQueueStart(){
	print("Queue for Regular Seniors Starts");
    do {
        rsQueueRun();
    } while (!timesUp);
    print("Queue for Regular Seniors Ends");
}
void eeQueueStart(){
	print("Queue for Everyone Else Starts");
    do {
        eeQueueRun();
    } while (!timesUp);
    print("Queue for Everyone Else Ends");
}
// Timer signal handler.
void timerHandler(int signal)
{
    timesUp = 1;  // office hour is over  NOTE CHANGE THIS Right now I think timesup is not needed or currently incorrectly implemented.
}
int main(int argc, char *argv[])
{
  //Seed rng with RAND_SEED
  srand(RAND_SEED);

  //Initialize Semaphores
  sem_init(&eeSem,0,0);
  sem_init(&rsSem,0,0);
  sem_init(&gsSem,0,0);


  // Set the timer for for office hour duration.
  enrollmentTimer.it_value.tv_sec = ENROLLMENT_WINDOW;
  setitimer(ITIMER_REAL, &enrollmentTimer, NULL);
  
  // Set the timer signal handler.
  signal(SIGALRM, timerHandler);
  
  //Initialize Queue Threads
  int i;
  

  //Initialize Student Threads
  int a;
  for(a=0; a<NUM_STUDENTS; a++){
    allStudents[a].studentID = a;
    allStudents[a].studentType = rand() % NUM_STUDENT_PRIORITY;
    allStudents[a].sectionDesired = rand() % NUM_STUDENT_CLASS_TYPES;
    allStudents[a].queuePos = -1;
    allStudents[a].enrolled = 0;
    addToQueue(&allStudents[a]);
  }
  //Set startTime
  time(&startTime);
}