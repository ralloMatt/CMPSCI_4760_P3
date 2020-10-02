#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <time.h>
#include <semaphore.h>

#define SHMKEY1	823566   // key for clock
#define SHMKEY2	823567   // key for shmMsg
#define SHMKEY3	823568   // key for semaphore

#define BUFF_SZ	sizeof ( int ) //size of memory used

int criticalSection(int (*shmClock)[2], int (*shmMsg)[2], int termSec, int termNano); 

int main (int argc, char *argv[]) {

	struct sembuf sb;

	//Access shared memory for clock and shmMsg
	int shmidClock = shmget ( SHMKEY1, BUFF_SZ*2, 0777 | IPC_CREAT ); //create memory
	int shmidShmMsg = shmget ( SHMKEY2, BUFF_SZ*2, 0777 | IPC_CREAT ); //create memory
	int semid = semget(SHMKEY3, 1, 0);
	
	if (shmidClock == -1 || shmidShmMsg == -1){ //checking for errors
		perror("Error in shmget\n");
		exit (1);
    }
	
	if(semid < 0){
		perror("Error in semaphore\n");
	}
	
	int (*shmClock)[2] = shmat(shmidClock, 0, 0);	//Get pointer to shared memory 
	int (*shmMsg)[2] = shmat(shmidShmMsg, 0, 0);	//Get pointer to shared memory

	if(*shmClock == (int *) -1 || *shmMsg == (int *) -1){ //check for errors
		perror("Error in shmat\n");
		exit(1);
	}
	

	time_t t;
	srand((int)time(&t) % getpid()); //needed so each user process random number isn't the same
	
	//Get termination deadline
	int duration = rand() % 999999 + 1; //generate random duration number
	int terminationSecondsTime = *shmClock[0];
	int terminationNanoSecondsTime = *shmClock[1] + duration; //add to nanoseconds
	if(terminationNanoSecondsTime >= 1000000000){
		terminationSecondsTime += 1; //add one to seconds
		terminationNanoSecondsTime -= 1000000000; //subtract nanoseconds
	}
	
	int check = 1;
	while(check == 1){
		sb.sem_op = -1;
		sb.sem_num = 0;
		sb.sem_flg = 0;
		semop(semid, &sb, 1); //put in semaphore
		
		//critical section
		check = criticalSection(shmClock, shmMsg, terminationSecondsTime, terminationNanoSecondsTime);
		
		sb.sem_op = 1;
		semop(semid, &sb, 1);
		
		if(check == 0)
			exit(0);
	}
	
	return 0;
}

int criticalSection(int (*shmClock)[2], int (*shmMsg)[2], int termSec, int termNano){
	
	//see if duration has passed
	if(*shmClock[0] >= termSec && *shmClock[1] >= termNano){ //means reached termination time
		if(*shmMsg[0] == 0 || shmMsg[1] == 0){ //means nothing is in shmMsg
			*shmMsg[0] = termSec; //set message clock to termination time for seconds
			*shmMsg[1] = termNano; //set message clock to termination time for nanoseconds
			return 0;
		}
	}
	
	return 1;
}