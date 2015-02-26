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
sem_t class0Sem, class1Sem, class2Sem;

int gsQueue[NUM_STUDENTS];
int rsQueue[NUM_STUDENTS];
int eeQueue[NUM_STUDENTS];
pthread_mutex_t gsMutex, rsMutex, eeMutex;
sem_t gsSem, rsSem, eeSem;

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

void* addToQueue(struct Student *s){
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
void* removeFromQueue(struct Student *s){
}
void* createStudent(struct Student *s){
  sleep(rand()%ENROLLMENT_WINDOW);
  time(&(s->arrivalTime));
  addToQueue(s);
  sleep(TIME_IMPATIENT);
  removeFromQueue(s);
}
int main(int argc, char *argv[])
{
  //Seed rng with RAND_SEED
  srand(RAND_SEED);

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
}

