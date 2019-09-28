#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include <signal.h>
#include <errno.h>

#define BUFF_SZ (sizeof(int) *2)
int shmid;
int *sharedNum1;
int *sharedNum2;

static void alarmHandler(int signo){
	printf("seconds: %d milliseconds: %d\n", *sharedNum1, *sharedNum2);
	shmdt(sharedNum1);
	shmctl(shmid, IPC_RMID, NULL);
	kill(0, SIGTERM);
};
int main(int argc, char *argv[]){
	
	signal(SIGALRM, alarmHandler);
	alarm(2);
	int num1 = 0;
	int num2 = 0;
	sharedNum1 = &num1;
	sharedNum2 = &num2;
	
	//create shared key
	key_t key;
	key = ftok(".", 'A');
	printf("key in master: %d\n", key);

	pid_t pid = 0;
	pid_t wpid;	
	//assigning variables for getopt statement
	int option;
	char * logFile;
	//assigning to the default values in case they're not specified
	int totalSpawned = 5;
	logFile = "logFile.txt";
	int realTime = 0;
	//flags for the getopt statement
	int totalFlag = 0;
	int logFlag = 0;
	int timeFlag = 0;
	
	while((option = getopt(argc, argv, "hslt:")) != -1)
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
				totalFlag = 1;
				break;
			case 'l':
				logFlag = 1;
				break;
			case 't':
				timeFlag = 1;
				break;
			case '?':
				break;

			default:
				abort();
			}
	}
	
	
	//if the user specifies the amount, set it to totalSpawned	
	if(totalFlag == 1){
		if(argc > 2){
			totalSpawned = atoi(argv[2]);	
		}
	} //if the user specifies the file name, set it to logFile 
	else if(logFlag == 1){
		if(argc > 2){
			logFile = argv[2];
		}
	}
	//create the file and make sure it's a good file
	FILE *ofPtr = fopen(logFile, "w");
	if(ofPtr == NULL){
		perror("oss.c: Error: ");
		exit(EXIT_FAILURE);
	}
	fclose(ofPtr);
	
	//create the shared memory
	shmid = shmget(key, BUFF_SZ, 0777 | IPC_CREAT);
	printf("shmid from master: %d\n", shmid);
	if(shmid == -1){
		perror("shmget\n");
		exit(1);
	}
	//create the shared integers
	sharedNum1 = (int *) shmat(shmid, NULL, 0);
	printf("value at shareNum1 %p\n", sharedNum1);
	sharedNum2 = sharedNum1 + 1;


	//variable for for loop
	int i;
	//loop to spawn child processes
	for(i = 0; i < totalSpawned; i++){
		
		if(fork() == 0){
			printf("[son] pid %d from [parent] pid %d\n", getpid(), getppid());
			exit(0);
		}
		
	}
	//loop will run up to the amount specified or defaulted	
	for(i = 0; i < totalSpawned; i++)
	wait(NULL);

		
		
	









return 0;
}
