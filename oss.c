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

//function prototypes
void getOpt(int argc, char *argv[], FILE **output, int *option, int *x, int *z, char **filename);
void createKeys(key_t *key1, key_t *key2, key_t *key3);
void createSharedMemory(int *shmIntId, int *shmMsgId, int *semId, key_t key1, key_t key2, key_t key3);
void attachToSharedMemory(int **seconds, int **nano, sem_t **sem, char **shmMsg, int shmIntId, int shmMsgId, int semId);
void createExecArgs(char *iMemKey, char *sMemKey, char *semKey, int shmIntId, int shmMsgId, int semId);
void createInitialNumProcesses(int x, pid_t pid, char *iMemKey, char *sMemKey, char *semKey);
void ossCriticalSection(int **nano, int **seconds, sem_t *sem, char **shmMsg, char *childPid, char *childSeconds, char *childNano, FILE **output, int *x, int *totalNumProcesses, pid_t *pid, char *iMemKey, char *sMemKey, char *semKey, int *currentNumProcesses, int *status, int *shmIntId, int *shmMsgId, int *semId);


int main(int argc, char *argv[]){
	//signal handler
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
	
	//file pointer and filename buffer
	FILE *output;
	char *filename = NULL;

	//variables to hold the shared mem keys
	key_t key1;
	key_t key2;
	key_t key3;
	
	//variables to hold the process IDs
	pid_t pid = 0;
	pid_t wpid = 0;
	
	//buffers to store the values getting passed to execlp()
	char iMemKey[11];
	char sMemKey[11];
	char semKey[11];
	
	//buffers to tokenize shared memory string from user
	char childPid[10];
	char childSeconds[10];
	char childNano[10];	
	
	//function call to get the command line arguments				
	getOpt(argc, argv, &output, &option, &x, &z, &filename);	
	//function call to create the shared keys
	createKeys(&key1, &key2, &key3);
	//function call to create the shared memory segments
	createSharedMemory(&shmIntId, &shmMsgId, &semId, key1, key2, key3);
	//function call to attach to the shared memory segments
	attachToSharedMemory(&seconds, &nano, &sem, &shmMsg, shmIntId, shmMsgId, semId);
	//function create exec arguments
	createExecArgs(iMemKey, sMemKey, semKey, shmIntId, shmMsgId, semId);
	//signal functions
	sigaction(SIGINT, &act, 0);
	sigaction(SIGALRM, &act, 0);
	alarm(z);
	
	//create the initial amount of children
	createInitialNumProcesses(x, pid, iMemKey, sMemKey, semKey);
	
	//setting the total number of processes that will be made to x which was given by user. set the current number of processes to x as well
	totalNumProcesses = x;
	currentNumProcesses = x;
	//function for the critical section
	ossCriticalSection(&nano, &seconds, sem, &shmMsg, childPid, childSeconds, childNano, &output, &x, &totalNumProcesses, &pid, iMemKey, sMemKey, semKey, &currentNumProcesses, &status, &shmIntId, &shmMsgId, &semId);
	
	//make sure all the children terminate before clearing the shared memory
	while((wpid = wait(&status)) > 0);
	
	//detach the pointers from their shared memory segments
	shmdt(seconds);
	shmdt(shmMsg);
	shmdt(sem);
	sem_destroy(sem);
	//destroy the shared memory segments
	shmctl(shmIntId, IPC_RMID, NULL);
	shmctl(shmMsgId, IPC_RMID, NULL);
	shmctl(semId, IPC_RMID, NULL);
	//close the file	
	fclose(output);

return 0;
}
static void sigHandler(int signo){
	signalCalled = 1;
};
//call this function to destroy all the shared memory
void cleanup(int *seconds, char *shmMsg, sem_t *sem, int *shmIntId, int *shmMsgId, int *semId){
	//detach the pointers from their shared memory segments	
	shmdt(seconds);
	shmdt(shmMsg);
	shmdt(sem);
	sem_destroy(sem);
	//destroy the shared memory segments
	shmctl(*shmIntId, IPC_RMID, NULL);
	shmctl(*shmMsgId, IPC_RMID, NULL);
	shmctl(*semId, IPC_RMID, NULL);

	kill(0,SIGTERM);
};
void getOpt(int argc, char *argv[], FILE **output, int *option, int *x, int *z, char **filename){
	//declare variables for the getopt statement. make sure to declare the default variables
	*option = 0;
	*x = 5;
	*z = 2;
	*filename = (char *)malloc(sizeof(char) * 50);
	*filename = "log.txt";

	while((*option = getopt(argc, argv, "hs:l:t:")) != -1)
		switch(*option){
			case 'h':
				printf("Help Menu\n");
				printf("-h : brings up the help menu.\n");
				printf("-s <x> : choose the maximum number of child processes spawned. The default is 5. \n");
				printf("-l <filename> : choose the name of the log file. The default is 'log.txt'\n");
				printf("-t <z> : choose the time in real seconds when the master will termindate itself and all children. The default is 2.\n");
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
	//open the file to write to it
	*output = fopen(*filename, "w");
};
//function to create the shared keys
void createKeys(key_t *key1, key_t *key2, key_t *key3){
	*key1 = ftok(".", 'I');
	*key2 = ftok(".", 'S');
	*key3 = ftok(".", '4');
};
//create the shared memory
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
//function to attach to the shared memory
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
	//the value of '1' in arg2 means the semaphore will be shared betwen processes. the value of '1' in arg3 means the semaphore will hold the value 1
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
		//if the pid is successful, the child will exec into a unique process
		else if(pid == 0){
			execlp("./user", "./user", iMemKey, sMemKey, semKey, (char *)NULL);
		}
	}
};
void createMoreProcesses(int x, pid_t pid, int currentNumProcesses, char *iMemKey, char *sMemKey, char *semKey){
	
	int i = 0;
	//loop to fork off child processes
	for(i = currentNumProcesses; i < x; i++){
		if((pid = fork()) == -1){
			perror("failed to fork\n");
			exit(1);
		}
		//if the pid is successful, the child will exec into a unique process
		else if(pid == 0){
			execlp("./user", "./user", iMemKey, sMemKey, semKey, (char *)NULL);
		}
	}
};
//this is the critical section
void ossCriticalSection(int **nano, int **seconds, sem_t *sem, char **shmMsg, char *childPid, char *childSeconds, char *childNano, FILE **output, int *x, int *totalNumProcesses, pid_t *pid, char *iMemKey, char *sMemKey, char *semKey, int *currentNumProcesses, int *status, int *shmIntId, int *shmMsgId, int *semId){
	
	do{
		//check if the signal has been called
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
			//reset the shared memory buffer to empty
			*shmMsg[0] = '\0';
			//wait on the child process to terminate
			if(waitpid(childProcess, status, WUNTRACED) == -1){
				fprintf(stderr, "terminating child: %d\n", childProcess);
				kill(childProcess, SIGTERM);
			} 
			//decrement our num of processes running
			(*currentNumProcesses)--;
			//fork/exec the next process until we hit 100
			if(*totalNumProcesses < 100){
				(*totalNumProcesses)++;
				createMoreProcesses(*x, *pid, *currentNumProcesses, iMemKey, sMemKey, semKey);
			}
			//if we've reached 100, clean up
			else if(*totalNumProcesses == 100){
				cleanup(*seconds, *shmMsg, sem, shmIntId, shmMsgId, semId);
			}
		}
		sem_post(sem);
	//if the program keeps running, increment the semaphore and go back through the loop
	}while(**seconds < 2);

	 
};
