#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <semaphore.h>
#include <errno.h>

#define SHMKEY1	823566   // key for clock
#define SHMKEY2	823567   // key for shmMsg
#define SHMKEY3	823568   // key for semaphore

#define BUFF_SZ	sizeof ( int ) //size of memory used

void terminate(int sig);//signal handler to terminate program, kill processes, and free memory

int shmidClock; //so I can free memory with signal handler function
int shmidShmMsg; //so I can free memory with signal handler function
int semid; //so I can clean semaphore
int childIds[100]; //used to keep track of children (100 is max number of processes)
int totalProcesses = 0; // total amount of processes

union semun{
	int value;
};

int main (int argc, char *argv[]) {

	printf("\nOSS C Program.\n\n");
	
	int option; //used to see if option is present
	int x = 5; //number of processes spawned (default 5)
	int z = 2; //seconds when master and its children will terminate (default 2)
	char *filename = "logfile.txt"; //log file to be used (default logfile.txt)
	
	while ((option = getopt (argc, argv, "hs:l:t:")) != -1){ //get option
		switch (option) { //check cases 
			case 'h':
				printf("How to run: './oss -option'\n");
				printf("\tOption: '-h' ~ Displays options and what they do.\n");
				printf("\tOption: '-s x' ~ Max number of user processes spawned.\n");
				printf("\tOption: '-l filename' ~ Log file for information to be written to. \n");
				printf("\tOption: '-t z' ~ Time in seconds when master and its children will terminate.\n");
				return 0;
				break;
			case 's':
				x = atoi(optarg);
				if(x == 0){ //in case user put characters instead of number or put zero
					printf("Setting process to default 5 because either characters were given or argument was 0.\n");
					x = 5; //just set to default
				}
				printf("You want %d processes spawned.\n", x);
				break;
			case 'l':
				filename = optarg; 
				printf("The file '%s' will be used.\n", filename);
				break;
			case 't':
				z = atoi(optarg);
				if(z == 0){ //in case user put characters instead of number or put zero
					printf("Setting seconds to default 2 because either characters were given or argument was 0.\n");
					z = 2; //just set to default
				}
				printf("Master and its children will terminate after %d seconds.\n", z);
				break;
			default: //invalid option
				return 0;
		}
	} //end while
	
	printf("\n"); //cosmetic purposes for if I print out options
	
	printf("%d processes will be spawned.\n", x);
	printf("The file '%s' will be written to.\n", filename);
	printf("Program will end after %d seconds.\n\n", z);
	

	//open file
	FILE *file = fopen(filename, "w"); //create new file to write to if file does not exist
	if(file == NULL){ //make sure file can be opened
		fprintf(stderr, "%s: ", argv[0]);
		perror("Error: \n");
		exit(1);
	}
	
	//SIGNAL HANDLERS
	
	//using signal handler "terminate" to kill processes, terminate program, and free memory
	signal(SIGALRM, terminate); //create signal handler
	signal(SIGINT, terminate); //terminate program if ctrl c is pressed
	alarm(z); //seconds in z when program should terminate
	
	//Semaphore
	union semun sem;
	sem.value = 1;
	
	//SHARED MEMORY
	
	//create shared memory for the clock and shmMsg
	shmidClock = shmget ( SHMKEY1, BUFF_SZ*2, 0777 | IPC_CREAT ); //create memory
	shmidShmMsg = shmget ( SHMKEY2, BUFF_SZ*2, 0777 | IPC_CREAT ); //create memory
	
	semid = semget(SHMKEY3, 1, 0777 | IPC_CREAT); //create semaphore

	if (shmidClock == -1 || shmidShmMsg == -1){ //checking for errors
		perror("Error in shmget\n");
		exit (1);
    }
	
	int (*shmClock)[2] = shmat(shmidClock, 0, 0);	//Get pointer to shared memory 
	int (*shmMsg)[2] = shmat(shmidShmMsg, 0, 0);	//Get pointer to shared memory

	if(*shmClock == (int *) -1 || *shmMsg == (int *) -1){ //check for errors
		perror("Error in shmat\n");
		exit(1);
	}
	
	//initialize clock and message
	*shmClock[0] = 0; //seconds for clock
	*shmClock[1] = 0; //nanoseconds for clock
	*shmMsg[0] = 0; //means 0 seconds
	*shmMsg[1] = 0; //means 0 nanoseconds
	
	semctl(semid, 0, SETVAL, 1, sem); //initialze semaphore
	if(errno){ //if semctl gives error
		fprintf(stderr, "%s\n", strerror(errno));
		exit(1);
	}

	//CREATING PROCESSES
	printf("Spawning processes.\n\n");
	
	pid_t child_pid; //child process ID
	int i;
	for(i = 0; i < x; i++){ //x is the number of processes
		child_pid = fork(); 
		if(child_pid == 0){ //in child
			execvp("./user", NULL);
			// If we get here, exec failed
			fprintf(stderr,"%s failed to exec worker!\n\n",argv[0]);
			exit(0); //exit child
		}
		
		//keep track of processes made
		totalProcesses++; 
		childIds[i] = child_pid;
	}
	
	while(*shmClock[0] < 2 && totalProcesses < 100){ 
	//while simulated clock is less than 2 seconds and processesare less than 100
		
		if(*shmMsg[0] != 0 || *shmMsg[1] != 0){ //means if we have a message
			//printf("Child %d is terminating at my time %d.%d becuase it reached %d.%d in user.\n", child_pid, *shmClock[0], *shmClock[1], *shmMsg[0], *shmMsg[1]);
			
			//write to file
			fprintf(file, "Child %d is terminating at my time %d.%d becuase it reached %d.%d in user.\n", child_pid, *shmClock[0], *shmClock[1], *shmMsg[0], *shmMsg[1]);
			
			wait();//wait for child to end
			
			//clear shmMsg
			*shmMsg[0] = 0; //means 0 seconds
			*shmMsg[1] = 0; //means 0 nanoseconds
			
			//fork off another child
			child_pid = fork(); 
			if(child_pid == 0){ //in child
				execvp("./user", NULL);
				// If we get here, exec failed
				fprintf(stderr,"%s failed to exec worker!\n\n",argv[0]);
				exit(0); //exit child
			}			
			
			childIds[totalProcesses] = child_pid;
			totalProcesses++;
		}
		
		
		//added 1000 each time 
		//because if I have too little 2 real seconds pass by and program ends
		//and if I have too much program ends because 2 simulated seconds pass by 
		*shmClock[1] += 1000;
		if(*shmClock[1] == 1000000000){ // 1 billion nanoseconds = 1 second
			*shmClock[0] += 1; //add one to seconds
			*shmClock[1] = 0; //set nanoseconds back to zero
		}
		
	}
	
	int sig = 0; //use sig to print out results in terminate() function
	
	if(*shmClock[0] >= 2){ //terminate if 2 simulated seconds pass by
		sig = -1;
	}
	
	if(totalProcesses == 100){ //terminate if 100 processes were made
		sig = -2;
	}
	
	printf("After processes Clock = %d.%d\n", *shmClock[0], *shmClock[1]);	

	fclose(file); //close file
	terminate(sig); //terminate the program, kill child processes, free shared memory

	return 0;
}

void terminate(int sig){ //signal handler
	if(sig == 14){ //SIGALRM is 14 when passed
		fprintf(stderr, "Time is up. Terminating Program!\n");
	}
	if(sig == 2){ //SIGINT is 2 when passed
		fprintf(stderr, "\nYou've pressed ctrl c. Terminating Program!\n");
	}
	if(sig == -1){ //means 2 simulated seconds passed by
		fprintf(stderr, "\nTwo simulated clock seconds passed. Terminating Program!\n");
	}
	if(sig == -2){ //means 2 simulated seconds passed by
		fprintf(stderr, "\n100 processes have been generated. Terminating Program!\n");
	}
	if(sig == 0){
		printf("Program has ended sucessfully. Terminating Program!\n");
	}
	
	int i;
	for(i = 0; i < totalProcesses; i++){
		kill(childIds[i], SIGKILL); //kill child process
	}
	
	semctl(semid, 0, IPC_RMID); //remove semaphore
	
	//free up any memory
	if(	shmctl(shmidClock, IPC_RMID, NULL) == -1) //deallocate shared memory and check for error
		perror("Error in shmctl\n");
	if(	shmctl(shmidShmMsg, IPC_RMID, NULL) == -1) //deallocate shared memory and check for error
		perror("Error in shmctl\n");
	exit(0); //terminate program
}