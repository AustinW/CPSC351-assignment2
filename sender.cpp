/**
 * @File: sender.cpp
 * @Description:
 * 
 * This program shall implement the process that sends files to the receiver process. 
 * It shall perform the following sequence of steps: 
 *  
 * 1. The sender shall be invoked as ./sender file.txt where sender is the name of the 
 *    executable and file.txt is the name of the file to transfer. 
 * 2. The program shall then attach to the shared memory segment, and connect to the 
 *    message queue both previously set up by the receiver. 
 * 3. Read a predefined number of bytes from the specified file, and store these bytes in 
 *    the chunk of shared memory. 
 * 4. Send a message to the receiver (using a message queue). The message shall contain a 
 *    field called size indicating how many bytes were read from the file. 
 * 5. Wait on the message queue to receive a message from the receiver confirming success- 
 *    ful reception and saving of data to the file by the receiver. 
 * 6. Go back to step 3. Repeat until the whole file has been read. 
 * 7. When the end of the file is reached, send a message to the receiver with the size 
 *    field set to 0. This will signal to the receiver that the sender will send no more. 
 * 8. Close the file, detach shared memory, and exit. 

 * 
 * @authors: Ryan Ott, Ben Hoelzel, Austin White
 * @date: March 29th, 2014
 */

#include <sys/shm.h>
#include <sys/msg.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <iostream>
#include <fstream>
#include "msg.h"

// The size of the shared memory chunk
#define SHARED_MEMORY_CHUNK_SIZE 1000

// For testing purposes
#define MESSAGE_COUNT_FOR_TESTING 1000

using namespace std;

// The ids for the shared memory segment and the message queue
int shmid, msqid;

// The pointer to the shared memory
void* sharedMemPtr;

/**
 * Sets up the shared memory segment and message queue
 * @param shmid - the id of the allocated shared memory 
 * @param msqid - the id of the shared memory
 */

void init(int& shmid, int& msqid, void*& sharedMemPtr)
{
	
	//Create file
	ofstream fout;	
	fout.open("keyfile.txt");

	if ( ! fout.good()) {
		perror("(ofstream) File creation failed");
		exit(1);
	}

	// Build file bigger than allocated memory chunk. To tests messaging	
	for (int i =0; i < MESSAGE_COUNT_FOR_TESTING; i++){	
		fout << i << " Hello World!\n";
	}

	fout.close();
	
	// Get key
	key_t key = ftok("keyfile.txt", 'a');

	// Get the id of the shared memory segment
	if ( (shmid = shmget(key, SHARED_MEMORY_CHUNK_SIZE, 0644 | IPC_CREAT)) == -1 ) {
		perror("(shmget) Error getting the shared memory segment id");
		exit(1);
	}

	// Attach to the shared memory
	sharedMemPtr = shmat(shmid, (void *) 0, 0);
	
	if (sharedMemPtr == (char *)(-1)) {
		perror("(shmat) Error getting a pointer to shared memory");
		exit(1);
	}
	
	// Attach to the message queue
	if ( (msqid = msgget(key, 0644 | IPC_CREAT)) == -1) {
		perror("(msgget) Error getting message from queue");
		exit(1);
	}	
}

/**
 * Performs the cleanup functions
 * @param sharedMemPtr - the pointer to the shared memory
 * @param shmid - the id of the shared memory segment
 * @param msqid - the id of the message queue
 */
void cleanUp(const int& shmid, const int& msqid, void* sharedMemPtr)
{
	// Detach from shared memory
	if (shmdt(sharedMemPtr) == -1) {
		perror("(shmdt) Error detaching from shared memory");
		exit(1);
	}

	// Destroy shared memeory
	if (shmctl(shmid, IPC_RMID, NULL) == -1 ) {
		perror("(shmctl) Error destroying shared memory");
		exit(1);
	}

	// Destroy message queue
	if (msgctl(msqid, IPC_RMID, NULL) == -1) {
		perror("(msgctl) Error destroying message queue");
		exit(1);
	}
}

/**
 * The main send function
 * @param fileName - the name of the file
 */
void send(const char* fileName)
{
	// Open the file for reading
	FILE* fp = fopen(fileName, "r");

	// Was the file open?
	if ( ! fp)
	{
		perror("(fopen) Error opening file");
		exit(-1);
	}

	// A buffer to store message we will send to the receiver.
	message sndMsg;

	//Set message type to sender
	sndMsg.mtype = SENDER_DATA_TYPE;
	
	// A buffer to store message received from the receiver.
	message rcvMsg;
	
	/* Read the whole file */
	while (!feof(fp))
	{
		/* Read at most SHARED_MEMORY_CHUNK_SIZE from the file and store them in shared memory. 
		 * fread will return how many bytes it has actually read (since the last chunk may be less
		 * than SHARED_MEMORY_CHUNK_SIZE).
		 */

		if ( (sndMsg.size = fread(sharedMemPtr, sizeof(char), SHARED_MEMORY_CHUNK_SIZE, fp)) < 0) {
			perror("(fread) Error reading from shared memory");
			exit(-1);
		}
		
		
		/* Send message to queue alerting reciver message is ready */
		if (msgsnd(msqid, &sndMsg , sizeof(struct message) - sizeof(long), 0) == -1){
			perror("(msgsnd) Error sending message to alert receiver");
		}
			

		/* Wait until the receiver sends a message of type RECV_DONE_TYPE telling the sender 
		 * that it finished saving the memory chunk. 
		 */
		if ( msgrcv(msqid, &rcvMsg, sizeof(struct message) - sizeof(long), RECV_DONE_TYPE, 0) == -1 ) {
			perror("(msgrcv) Error receiving message from receiver");		
			exit(1);
		}
	}

	/* Send message to queue to tell reciver we have no more data to send, size = 0
	 * Set the message size in sndMsg to 0; siganls no more data
	 */
	sndMsg.size = 0;

	if (msgsnd(msqid, &sndMsg , sizeof(struct message) - sizeof(long) , 0) == -1) {
		perror("(msgsnd) Error sending a message");
	}

	// Close the file
	fclose(fp);
	
}

int main(int argc, char** argv)
{
	
	// Check the command line arguments
	if (argc < 2)
	{
		fprintf(stderr, "USAGE: %s <FILE NAME>\n", argv[0]);
		exit(-1);
	}
		
	// Connect to shared memory and the message queue
	init(shmid, msqid, sharedMemPtr);
	
	// Send the file
	send(argv[1]);

	// Cleanup
	cleanUp(shmid, msqid, sharedMemPtr);
		
	return 0;
}
