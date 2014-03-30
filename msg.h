/**
 * @File: msg.h
 * @Description:
 * 
 * This header file used by both the sender and the receiver.
 * It contains the struct of the message relayed through message queues.
 * 
 * @authors: Ryan Ott, Ben Hoelzel, Austin White
 * @date: March 29th, 2014
 */

// The information type
#define SENDER_DATA_TYPE 1

// The done message type
#define RECV_DONE_TYPE 2

/**
 * The message structure
 * @property long mtype
 * @property int size
 */
struct message
{
	// The message type
	long mtype;
	
	// How many bytes in the message
	int size;
	
	/**
	 * Prints the structure
	 * @param fp - the file stream to print to
	 */
	void print(FILE* fp)
	{
		fprintf(fp, "%ld %d", mtype, size);
		
	}

	// Testing function to print to stdout
	void printer()
	{
		printf("%d\n", size);
	}
};
