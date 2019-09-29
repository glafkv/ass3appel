#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <signal.h>
#include <fcntl.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>

//constants for the buffers used for each memory segment
#define INTBUFF_SZ (sizeof(int) * 2)
#define MSGBUFF_SZ (sizeof(char) * 100)
#define SEMBUFF_SZ (sizeof(int))

volatile sig_atomic_t signalCalled = 0;

//signal handler functions
static void sigHandler(int signo);
void cleanup(int *seconds, char *shmMsg, sem_t *sem, int *shmIntId, int *shmMsgId, int *semId);

//driver functions
void getOpt(int argc, char *argv[], FILE **output, int *option, int *x, int *z, char **filename);
void createKeys(key_t *key1, key_t *key2, key_t *key3);
void createSharedMemory(int *shmIntId, int *shmMsgId, int *semId, key_t key1, key_t key2, key_t key3);
void attachToSharedMemory(int **seconds, int **nano, sem_t **sem, char **shmMsg, int shmIntId, int shmMsgId, int semId);
void createExecArgs(char *iMemKey, char *sMemKey, char *semKey, int shmIntId, int shmMsgId, int semId);
void createInitialNumProcesses(int x, pid_t pid, char *iMemKey, char *sMemKey, char *semKey);
void ossCriticalSection(int **nano, int **seconds, sem_t *sem, char **shmMsg, char *childPid, char *childSeconds, char *childNano, FILE **output, int *x, int *totalNumProcesses, pid_t *pid, char *iMemKey, char *sMemKey, char *semKey, int *currentNumProcesses, int *status, int *shmIntId, int *shmMsgId, int *semId);


int main(int argc, char *argv[]){
	
	struct sigaction act;
	memset(&act, 0, sizeof(act));
	act.sa_handler = sigHandler;
	
	//pointers for shared memory
	int *seconds = 0;
	int *nano = 0;
	char *shmMsg = NULL;
	sem_t *sem;

	//variables to hold shared mem location IDs
	int shmIntId = 0;
	int shmMsgId = 0;
	int semId = 0;
	
	//initializing variables
	int totalNumProcesses = 0;
	int currentNumProcesses = 0;
	int x = 0;	
	int z = 0;
	int option = 0;
	int status;
	
	//files
	FILE *output;
	char *filename = NULL;

	//variables to hold the shared mem keys
	key_t key1;
	key_t key2;
	key_t key3;
	
	//variables to hold the process IDs
	pid_t pid = 0;
	pid_t wpid = 0;
	
	char iMemKey[11];
	char sMemKey[11];
	char semKey[11];
	
	char childPid[10];
	char childSeconds[10];
	char childNano[10];
	//signal(SIGALRM, alarmHandler);
	//alarm(2);
	//int num1 = 0;
	//int num2 = 0;
	//sharedNum1 = &num1;
	//sharedNum2 = &num2;
	//char arg1[10];
	//create shared key
	/*key_t key;
	key = ftok(".", 'A');
	printf("key in master: %d\n", key);
	
	pid_t childpid = 0;
	int i = 0, j = 0, done, shmid = 0, total = 0;
	int array[2];
	//pid_t wpid;	
	//assigning variables for getopt statement
	int option;
	char * logFile;
	//assigning to the default values in case they're not specified
	int totalSpawned = 5;
	logFile = "logFile.txt";
	int realTime = 5;
	//flags for the getopt statement
	int totalFlag = 0;
	int logFlag = 0;
	int timeFlag = 0;*/
	
	/*while((option = getopt(argc, argv, "hslt:")) != -1)
	{
		switch(option){
			
			case 'h':
				printf("\tI'm here to help\n");
				printf("**********************\n");
				printf("-h brings up the help message\n");
				printf("-s <x> : specify the number of child processes spawned. The default is 5.\n");
				printf("-l <filename> : specify the logfile you want to send information to.\n");	
				printf("-t <z> : specify the time in real seconds when the master will terminate itself and all it's children.\n");
				exit(0);
			case 's':
				x = atoi(optarg);
				break;
			case 'l':
				filename = optarg;
				strcpy(filename, ".txt");
				break;
			case 't':
				z = atoi(optarg);
				break;
			case '?':
				break;

			default:
				abort();
			}
	
	}*/
//	*output = fopen(*filename, "a");
	getOpt(argc, argv, &output, &option, &x, &z, &filename);	
	createKeys(&key1, &key2, &key3);
	createSharedMemory(&shmIntId, &shmMsgId, &semId, key1, key2, key3);
	attachToSharedMemory(&seconds, &nano, &sem, &shmMsg, shmIntId, shmMsgId, semId);
	createExecArgs(iMemKey, sMemKey, semKey, shmIntId, shmMsgId, semId);
	//signal functions
	sigaction(SIGINT, &act, 0);
	sigaction(SIGALRM, &act, 0);
	alarm(z);
	
	createInitialNumProcesses(x, pid, iMemKey, sMemKey, semKey);
	
	totalNumProcesses = x;
	currentNumProcesses = x;
	ossCriticalSection(&nano, &seconds, sem, &shmMsg, childPid, childSeconds, childNano, &output, &x, &totalNumProcesses, &pid, iMemKey, sMemKey, semKey, &currentNumProcesses, &status, &shmIntId, &shmMsgId, &semId);
	
	while((wpid = wait(&status)) > 0);
	
	shmdt(seconds);
	shmdt(shmMsg);
	shmdt(sem);
	sem_destroy(sem);
	
	shmctl(shmIntId, IPC_RMID, NULL);
	shmctl(shmMsgId, IPC_RMID, NULL);
	shmctl(semId, IPC_RMID, NULL);
	
	fclose(output);

	






return 0;
}
static void sigHandler(int signo){
	signalCalled = 1;
};
void cleanup(int *seconds, char *shmMsg, sem_t *sem, int *shmIntId, int *shmMsgId, int *semId){
	
	shmdt(seconds);
	shmdt(shmMsg);
	shmdt(sem);
	sem_destroy(sem);
	
	shmctl(*shmIntId, IPC_RMID, NULL);
	shmctl(*shmMsgId, IPC_RMID, NULL);
	shmctl(*semId, IPC_RMID, NULL);

	printf("terminating\n");
	kill(0,SIGTERM);
};
void getOpt(int argc, char *argv[], FILE **output, int *option, int *x, int *z, char **filename){
	
	*option = 0;
	*x = 5;
	*z = 2;
	*filename = (char *)malloc(sizeof(char) * 50);
	*filename = "log.txt";

	while((*option = getopt(argc, argv, "hs:l:t:")) != -1)
		switch(*option){
			case 'h':
				printf("help\n");
				exit(1);
			case 's':
				*x = atoi(optarg);
				break;
			case 'l':
				*filename = optarg;
				strcpy(*filename, ".txt");
				break;
			case 't':
				*z = atoi(optarg);
				break;
			case '?':
				break;
			default:
				break;
		}
	
	*output = fopen(*filename, "w");
};
void createKeys(key_t *key1, key_t *key2, key_t *key3){
	*key1 = ftok(".", 'I');
	*key2 = ftok(".", 'S');
	*key3 = ftok(".", '4');
};
void createSharedMemory(int *shmIntId, int *shmMsgId, int *semId, key_t key1, key_t key2, key_t key3){
	
	//creates shared memory segments using specific keys
	*shmIntId = shmget(key1, INTBUFF_SZ, 0777 | IPC_CREAT);
	if(*shmIntId == -1){
		fprintf(stderr, "error getting shmIntId %s\n", strerror(errno));
	}
	
	*shmMsgId = shmget(key2, MSGBUFF_SZ, 0777 | IPC_CREAT);
	if(*shmMsgId == -1){
		fprintf(stderr, "error getting shmMsgId %s\n", strerror(errno));
	}
	
	*semId = shmget(key3, SEMBUFF_SZ, 0777 | IPC_CREAT);
	if(*semId == -1){
		fprintf(stderr, "error getting semId %s\n", strerror(errno));
	}
};
void attachToSharedMemory(int **seconds, int **nano, sem_t **sem, char **shmMsg, int shmIntId, int shmMsgId, int semId){
	
	//attached each pointer to a location in shared memory
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
	
	//initialize semaphores and point to a location in shared memory
	sem_init(*sem, 1, 1);
};
void createExecArgs(char *iMemKey, char *sMemKey, char *semKey,int shmIntId, int shmMsgId, int semId){
	
	snprintf(iMemKey, 11, "%d", shmIntId);
	snprintf(sMemKey, 11, "%d", shmMsgId);
	snprintf(semKey, 11, "%d", semId);
};
void createInitialNumProcesses(int x, pid_t pid, char *iMemKey, char *sMemKey, char *semKey){
	
	int i;
	//loop to fork off the child process
	for(i = 0; i < x; i++){
		if((pid = fork()) == -1){
			perror("failed to fork\n");
			exit(1);
		}
		else if(pid == 0){
			execlp("./user", "./user", iMemKey, sMemKey, semKey, (char *)NULL);
		}
	}
};
void createMoreProcesses(int x, pid_t pid, int currentNumProcesses, char *iMemKey, char *sMemKey, char *semKey){
	
	int i = 0;
	for(i = currentNumProcesses; i < x; i++){
		if((pid = fork()) == -1){
			perror("failed to fork\n");
			exit(1);
		}
		else if(pid == 0){
			execlp("./user", "./user", iMemKey, sMemKey, semKey, (char *)NULL);
		}
	}
};
void ossCriticalSection(int **nano, int **seconds, sem_t *sem, char **shmMsg, char *childPid, char *childSeconds, char *childNano, FILE **output, int *x, int *totalNumProcesses, pid_t *pid, char *iMemKey, char *sMemKey, char *semKey, int *currentNumProcesses, int *status, int *shmIntId, int *shmMsgId, int *semId){
	
	do{
		if(signalCalled == 1){
			cleanup(*seconds, *shmMsg, sem, shmIntId, shmMsgId, semId);
		}
		//enter critical section and incrememnt the clock
		sem_wait(sem);
		*currentNumProcesses = *x;
		**nano += 500;
		if(**nano >= 1000000000){
			**seconds += 1;
			**nano = **nano - 1000000000;
		}
		else if(**nano == 1000000000){
			**seconds += 1;
			**nano = 0;
		}
		//check shared memory message to see if chiold process sent message that it is ready to be terminated
		if((*shmMsg[0] != '\0')){
			//if shared memory message is not empty, tokenize the info sent from child.
			strcpy(childPid, strtok(*shmMsg, " "));
			strcpy(childSeconds, strtok(NULL, " "));
			strcpy(childNano, strtok(NULL, " "));
			pid_t childProcess = atoi(childPid);
			//write to log file
			fprintf(*output, "OSS: Child %d is terminating at my time %d.%d because it reached %d.%d in user\n", childProcess, **seconds, **nano, atoi(childSeconds), atoi(childNano));
			*shmMsg[0] = '\0';
			//wait on the child process to terminate
			if(waitpid(childProcess, status, WUNTRACED) == -1){
				fprintf(stderr, "terminating child: %d\n", childProcess);
				kill(childProcess, SIGTERM);
			} 
			//decrement our num of processes running
			(*currentNumProcesses)--;
			if(*totalNumProcesses < 100){
				(*totalNumProcesses)++;
				createMoreProcesses(*x, *pid, *currentNumProcesses, iMemKey, sMemKey, semKey);
			}
			else if(*totalNumProcesses == 100){
				cleanup(*seconds, *shmMsg, sem, shmIntId, shmMsgId, semId);
			}
		}
		sem_post(sem);
	}while(**seconds < 2);
};
