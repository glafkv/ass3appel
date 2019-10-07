#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <errno.h>
#include <stdbool.h>

//function prototypes
void attachToSharedMemory(int **seconds, int **nano, sem_t **sem, char **shmMsg, int shmIntId, int shmMsgId, int semId);
void getLocalTimes(int *localSecs, int *localNano, int *seconds, int *nano);
int getRandNum(int *randNum, int *pid);
void getKillTime(int *localSecs, int *localNano, int *killSecs, int *killNano, int *randNum);
void criticalSection(sem_t *sem, int *seconds, int *nano, int *killSecs, int *killNano, int *localSecs, int *localNano, pid_t *pid, char **shmMsg, bool *jobDone);

int main(int argc, char *argv[]){
	
	//shared mem pointers
	int *seconds = 0;
	int *nano = 0;
	char *shmMsg = NULL;
	sem_t *sem;
	
	int shmIntId = 0;
	int shmMsgId = 0;
	int semId = 0;
	int randNum = 0;
	
	//set the start time
	int localSecs = 0;
	int localNano = 0;
	
	//to hold the time the child process will terminate
	int killSecs = 0;
	int killNano = 0;
	
	//to see if we can break out of the critical section
	bool jobDone = false;
	
	//using a random number generator to make sure we don't keep getting the same number for each process
	pid_t pid = getpid();
	
	//grab the values passed to the program using execlp(). the argv[] are locations of the variables from the exec function
	shmIntId = atoi(argv[1]);
	shmMsgId = atoi(argv[2]);
	semId = atoi(argv[3]);
	
	//attach to shared memory
	attachToSharedMemory(&seconds, &nano, &sem, &shmMsg, shmIntId, shmMsgId, semId);
	
	//critical section
	sem_wait(sem);
	//child gets the timer values from the shared memory
	getLocalTimes(&localSecs, &localNano, seconds, nano);
	sem_post(sem);
	//functions to generate the termination time
	sem_wait(sem);
	getRandNum(&randNum, &pid);
	sem_post(sem);
	
	getKillTime(&localSecs, &localNano, &killSecs, &killNano, &randNum);
	criticalSection(sem, seconds, nano, &killSecs, &killNano, &localSecs, &localNano, &pid, &shmMsg, &jobDone);
	//unattach from and destroy shared memory segments
	shmdt(sem);
	sem_destroy(sem);
	shmdt(seconds);
	shmdt(shmMsg);

return 0;
}
//function to attach to shared memory
void attachToSharedMemory(int **seconds, int **nano, sem_t **sem, char **shmMsg, int shmIntId, int shmMsgId, int semId){
	//attaches each pointer to a location in shared memory
	if((*seconds = (int *)shmat(shmIntId, NULL, 0)) == (void *)-1){
		fprintf(stderr, "%s\n", strerror(errno));
	}
	*nano = *seconds + 1;
	
	if((*shmMsg = (char *)shmat(shmMsgId, NULL, 0)) == NULL){
		fprintf(stderr, "%s\n", strerror(errno));
	}
	if((*sem = (sem_t *)shmat(semId, NULL, 0)) == (void *)-1){
		fprintf(stderr, "%s\n", strerror(errno));
	}
	//initialize the semaphore and point it to a location in shared memory
	sem_init(*sem, 1, 1);
};
void getLocalTimes(int *localSecs, int *localNano, int *seconds, int *nano){
	//gets the seconds and nanos from the shared memory and stores in local variables. the local variables create the values for process termination within the shared memory clock
	*localSecs = seconds[0];
	*localNano = nano[0];
};
//get the pid with a random number
int getRandNum(int *randNum, int *pid){
	srand((getpid()));
	*randNum = ((random() % (1000000 - 1)) - 1);
	return *randNum;
};
void getKillTime(int *localSecs, int *localNano, int *killSecs, int *killNano, int *randNum){
	
	//set the process termination time using the local variables
	*killSecs = *localSecs;
	*killNano = *localNano + *randNum;
	
	if(*killNano > 1000000000){
		*killSecs += 1;
		*killNano = *killNano - 1000000000;
	} else if(*killNano == 1000000000){
		*killSecs += 1;
		*killNano = 0;
	}
	if((*killSecs == 2 && *killNano > 0) || (*killSecs > 2)){
		exit(1);
	}
};
void criticalSection(sem_t *sem, int *seconds, int *nano, int *killSecs, int *killNano, int *localSecs, int *localNano, pid_t *pid, char **shmMsg, bool *jobDone){
	//process will enter the critical section and compare its time with the time in the shared memory clock. if it is not the kill time, the child will signal the semaphore. then it will call the wait signal and check the shared memory clock again. if the kill timer is up, the the child will write a message over shared memory that the parent wil check. if shmMsg is empty, we will write to it, signal the semaphore and break out of the loop, otherwise it will wait until shmMsg is NULL, then write to it, and wait for the parent process to terminate it
	while(1){
		sem_wait(sem);
		if((*seconds > *killSecs) || (*seconds >= *killSecs && *nano >= *killNano)){
			if(*shmMsg[0] == '\0'){
				*jobDone = true;
				snprintf(*shmMsg, (sizeof(char) * 100), "%d %d %d", *pid, *killSecs, *killNano);
				if((strlen(*shmMsg) != '\0')){
				}
			}
		}
		if(*jobDone == true){
			sem_post(sem);
			break;
		}
		sem_post(sem);
	}
};


