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
#include <sys/ipc.h>
#include <sys/types.h>
#include <sys/wait.h>

#define BUFF_SZ (sizeof(int) *2)

int shmid;
int *sharedNum1;
int *sharedNum2;

/*static void alarmHandler(int signo){
	printf("seconds: %d milliseconds: %d\n", *sharedNum1, *sharedNum2);
	shmdt(sharedNum1);
	shmctl(shmid, IPC_RMID, NULL);
	kill(0, SIGTERM);
};*/
void timer(int sig){
	char end = 'n';
	printf("timer expired. would you like to terminate? Y/N\n");
	if(end == 'y' || end == 'Y'){
		shmctl(shmid, IPC_RMID, NULL);
		exit(1);
	}
}
int main(int argc, char *argv[]){
	
	//signal(SIGALRM, alarmHandler);
	//alarm(2);
	int num1 = 0;
	int num2 = 0;
	sharedNum1 = &num1;
	sharedNum2 = &num2;
	char arg1[10];
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
	int realTime = 5;
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
	} else if(timeFlag == 1){
		if(argc > 2){
			realTime = atoi(argv[2]);
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
	//shmget returns an identifier in shmid
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

	int counter = 1;
	int totalCounter = 0;
	//variable for for loop
	int i, status;
	int numChild = 20;
	snprintf(arg1, 10, "%d", totalSpawned);
	time_t start;
	start = time(NULL);
	while(start < realTime){
	//loop to spawn child processes
	if(totalSpawned < 100){
		for(i = 0; i < totalSpawned; i++){
			if((pid = fork()) == -1){
				perror("Failed to fork.\n");
				exit(1);
			}
			else if(pid == 0){
				execlp("./user", "./user", arg1, (char *) NULL);
			}
		}
	} else {
		do{
			if(counter == numChild){
				wait(&status);
				counter--;
			}
			if((pid = fork()) == -1){
				perror("Failed to fork.\n");
				exit(1);
			} else if(pid == 0){
				execlp("./user", "./user", arg1, (char *) NULL);
			}
			counter++;
			totalCounter++;
		} while(totalCounter < totalSpawned);
	}
		
		
	}
	//loop will run up to the amount specified or defaulted	
	while((wpid = wait(&status)) > 0);
	printf("total seconds: %d total milliseconds %d\n", *sharedNum1, *sharedNum2);
	//for(i = 0; i < totalSpawned; i++)
	//wait(NULL);

		
		
	
	//detach from shared memory
	shmdt(sharedNum1);
	//destroy shared memory
	shmctl(shmid, IPC_RMID, NULL);








return 0;
}
