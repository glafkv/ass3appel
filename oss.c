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
	//if the logFlag is 1
	//if they didn't provide an argument, set the outfile to logFile.txt else the filename is what they indicated		
	
		
	if(totalFlag == 1){
		if(argc < 3){
			totalSpawned = 5;
			printf("%d \n", totalSpawned);
		} else {
			totalSpawned = atoi(argv[2]);
			printf("%d \n", totalSpawned);
		}
	} else if(logFlag == 1){
		if(argc < 3){
			logFile = "logFile.txt";
			FILE *ofPtr = fopen(logFile, "w");
			if(ofPtr == NULL){
				perror("oss.c: Error: ");
				exit(EXIT_FAILURE);
			}
			fclose(ofPtr);
		} else {
			logFile = argv[2];
			FILE *ofPtr = fopen(logFile, "w");
			if(ofPtr == NULL){
				perror("oss.c: Error: ");
				exit(EXIT_FAILURE);
			}
			fclose(ofPtr);
		}
	}
	
		

		











return 0;
}
