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

//driver functions
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
	
	bool jobDone = false;
	
	pid_t pid = getpid();
	
	shmIntId = atoi(argv[1]);
	shmMsgId = atoi(argv[2]);
	semId = atoi(argv[3]);
	
	//attach to shared memory
	attachToSharedMemory(&seconds, &nano, &sem, &shmMsg, shmIntId, shmMsgId, semId);
	
	//critical section
	sem_wait(sem);
	getLocalTimes(&localSecs, &localNano, seconds, nano);
	sem_post(sem);
	
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

void attachToSharedMemory(int **seconds, int **nano, sem_t **sem, char **shmMsg, int shmIntId, int shmMsgId, int semId){
	
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
	sem_init(*sem, 1, 1);
};
void getLocalTimes(int *localSecs, int *localNano, int *seconds, int *nano){
	*localSecs = seconds[0];
	*localNano = nano[0];
};
int getRandNum(int *randNum, int *pid){
	srand((getpid()));
	*randNum = ((random() % (1000000 - 1)) - 1);
	return *randNum;
};
void getKillTime(int *localSecs, int *localNano, int *killSecs, int *killNano, int *randNum){
	
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


