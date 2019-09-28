#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include <signal.h>


int main(int argc, char *argv[]){
	//assigning variables for getopt statement
	int option;
	int totalSpawned = 5;
	int totalFlag = 0;
	int logFlag = 0;
	int timeFlag = 0;
	char * logFile;
	logFile = "logFile.txt";
	int realTime = 0;
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
				//totalSpawned = atoi(argv[2]);
				break;
			case 'l':
				logFlag = 1;
				//logFile = argv[2];
				//printf("%s \n", logFile);
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
	
			
	//if the user doesn't provide an argument then the default is chosen
	
	//if the user specifies the amount, set it to totalSpawned	
	if(totalFlag == 1){
		if(argc > 2){
			totalSpawned = atoi(argv[2]);	
			printf("%d %s\n", totalSpawned, logFile);
		}
	} //if the user specifies the file name, set it to logFile 
	else if(logFlag == 1){
		if(argc > 2){
			logFile = argv[2];
			printf("%d %s\n",totalSpawned, logFile);
		}
	}
	//create the file and make sure it's a good file
	FILE *ofPtr = fopen(logFile, "w");
	if(ofPtr == NULL){
		perror("oss.c: Error: ");
		exit(EXIT_FAILURE);
	}
	fclose(ofPtr);
	//variable for for loop
	int i;
	//loop to spawn child processes
	for(i = 0; i < totalSpawned; i++){
		
		if(fork() == 0){
			printf("[son] pid %d from [parent] pid %d\n", getpid(), getppid());
			exit(0);
		}
		
	}	
	for(i = 0; i < totalSpawned; i++)
	wait(NULL);

		
		
	









return 0;
}
