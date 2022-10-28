//for system calls, please refer to the MAN pages help in Linux 
//sample echo tensmit receive program over udp - CSS432 - Autumn 2022
// STEP 1: TFTP - sending less than 512 sized packet
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string>
#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <errno.h>          // for retrieving the error number.
#include <string.h>         // for strerror function.
#include <signal.h>         // for the signal handler registration.
#include <unistd.h>

using namespace std;

#define SERV_UDP_PORT   51971 // REPLACE WITH YOUR PORT NUMBER

const static unsigned short OP_RRQ = 1;
const static unsigned short OP_WRQ = 2;
const static unsigned short OP_DATA = 3;
const static unsigned short OP_ACK = 4;
const static unsigned short OP_ERROR = 5;
const static int DATA_OFFSET = 4;
char *progname;

/* Global variable to hold block number */
unsigned short blockNumber = 1;

/* Max length of data is 512 bytes, 2 bytes op code, 2 bytes block number. total 516 bytes. */
const static int MAX_BUFFER_SIZE = 516;

#define MAXLINE 512  // Actual data size

// Global variable to keep track of file to create (in case client sents a write request)
string writeToFile;

/* Size of maximum packet to received.                            */

#define MAXMESG 2048

/* The dg_echo function receives data from the already initialized */
/* socket sockfd and returns them to the sender.                   */

void dg_echo(int sockfd) {
/* struct sockaddr is a general purpose data structure that holds  */
/* information about a socket that can use a variety of protocols. */
/* Here, we use Internet family protocols and UDP datagram ports.  */
/* This structure receives the client's address whenever a         */
/* datagram arrives, so it needs no initialization.                */
	
	struct sockaddr pcli_addr;
	
/* Temporary variables, counters and buffers.                      */

	int    n, clilen;
	char buffer[MAX_BUFFER_SIZE];
	bzero(buffer, sizeof(buffer));

/* Main echo server loop. Note that it never terminates, as there  */
/* is no way for UDP to know when the data are finished.           */

	for ( ; ; ) {

/* Initialize the maximum size of the structure holding the        */
/* client's address.                                               */

		clilen = sizeof(struct sockaddr);

/* Receive data on socket sockfd, up to a maximum of MAXSIZE - 1   */
/* bytes, and store them in mesg. The sender's address is stored   */
/* in pcli_addr and the structure's size is stored in clilen.      */

// Wait till server has recieved the packet from client
// Recieves char array into buffer variable
  cout<<"looking to recieve a packet" << endl;
		n = recvfrom(sockfd, buffer, MAX_BUFFER_SIZE, 0, &pcli_addr, (unsigned int*) &clilen);
/* n holds now the number of received bytes, or a negative number  */
/* to show an error condition. Notice how we use progname to label */
/* the source of the error.                                        */
		if (n < 0) {
			printf("%s: recvfrom error\n",progname);
			exit(3);
		}
    cout << "got a packet from client" << endl;

		for ( int i = 0; i < 30; i++ ) {
        	printf("0x%X,", buffer[i]);
    	}
		cout << endl;
		blockNumber++;
		unsigned short *opCodePtr = (unsigned short*) buffer;
		unsigned short opCodeRcv = ntohs(*opCodePtr);
		// if conditionals checking OP code to decide how to process remaining buffer array
		if (opCodeRcv == OP_RRQ) { // Got read request

			// Analyze rrq packet
			// Check filename which is a string, end of string is marked with 0
			opCodePtr++;  // move to third byte
			char *a = (char*) opCodePtr; // Start getting char on third byte
			int i = 0;
			string filename = "";
			while (a[i] != 0) {
				filename = filename + a[i];
				i++;
			}

			// Send data block
			char data_buffer[MAX_BUFFER_SIZE];
			bzero(data_buffer, sizeof(data_buffer));
			unsigned short *opCodePtr = (unsigned short*) data_buffer;
			*opCodePtr = htons(OP_DATA);
			opCodePtr++;
			// Have block pointer point to same as op pointer; the 3rd byte of buffer
			unsigned short *blockNumPtr = opCodePtr;
			// Fill in the block byte (from 3rd to 4th byte) with block number
			*blockNumPtr = htons(blockNumber);
			// Removed below line, since block number is updated anyway when we recieve another packet
			//blockNumber++;
			char *fileData = data_buffer + DATA_OFFSET;
			cout << "Filename: " << filename << endl;
			std::ifstream in(filename);
			std::string contents((std::istreambuf_iterator<char>(in)), 
				std::istreambuf_iterator<char>());
			cout << "Contents of string contents: " << contents << endl;
			char file[contents.size()];
      		strcpy(file, contents.c_str());
			cout << "Contents of file: " << endl;
			for (int i = 0; i < sizeof(file); i++) {
				cout << file[i];
			}
			cout << endl;
			memcpy(fileData, file, strlen(file));

			for ( int i = 0; i < 30; i++ ) {
        		printf("0x%X,", data_buffer[i]);
    		}
			if (sendto(sockfd, data_buffer, sizeof(data_buffer), 0, &pcli_addr, clilen) != n) {
				printf("%s: sendto error\n",progname);
				exit(4);
			}

		}	else if (opCodeRcv == OP_ACK) {
			// Increment the pointer to the third byte for block number 
			opCodePtr++;
			// Create unsigned int pointer and assign the two bytes there to the unsigned int.
			unsigned short *blockNumPtr = opCodePtr;
			blockNumber = ntohs(*blockNumPtr) + 1; // update our block number

		} else if (opCodeRcv == OP_WRQ) { // Got a write request. Client is wants to send server a data packet
			// Check filename which is a string, end of string is marked with 0
			opCodePtr++;  // move to third byte
			char *a = (char*) opCodePtr; // Start getting char on third byte
			int i = 0;
			string filename = "";
			while (a[i] != 0) {
				filename = filename + a[i];
				i++;
			}
			writeToFile = filename; // Save name of file to write to
			char ackBuffer[4];
			bzero(ackBuffer, 4);			
			// Pointer thats pointing to the start of the buffer array
			unsigned short *opPtr = (unsigned short*) ackBuffer;
			// convert from network order
			*opPtr = htons(OP_ACK);
			// pointer of op is pointing to start of the buffer array. fill in with op code data.
			opPtr++; // increment by 1 (unsigned short = 2 bytes), so now pointing to 3rd byte.
			
			// Have block pointer point to same as op pointer; the 3rd byte of buffer
			unsigned short *blockPtr = opPtr;
			// Fill in the block byte (from 3rd to 4th byte) with block number
			*blockPtr = htons(blockNumber);
			for ( int i = 0; i < sizeof(ackBuffer); i++ ) {
        		printf("0x%X,", ackBuffer[i]);
    		}
			cout << endl;
			if (sendto(sockfd, ackBuffer, sizeof(ackBuffer), 0, &pcli_addr, clilen) != sizeof(ackBuffer)) {
				printf("%s: sendto error wrq\n",progname);
				exit(4);
			}

		}	else if (opCodeRcv == OP_DATA) {
			// Process rest of buffer array like done so above:
			// Increment the pointer to the third byte for block number 
			opCodePtr++;
			// Create unsigned int pointer and assign the two bytes there to the unsigned int.
			unsigned short *blockNumPtr = opCodePtr;
			blockNumber = ntohs(*blockNumPtr);
			// Remember to convert to host byte order.
			// create pointer char, pointing to 5th byte of buffer, copy the byte to a file on reciever side
			char *fileData = buffer + DATA_OFFSET;
			char file[MAXLINE];
			cout << "Size of file data: " << sizeof(fileData) << endl;
			cout << "Size of buffer: " << sizeof(buffer) << endl;
			cout << "Size being copied: " << (sizeof(buffer)/sizeof(buffer[0]) - DATA_OFFSET) << endl;
			bcopy(fileData, file, sizeof(buffer) - DATA_OFFSET);
			// Do  strncpy (fileData, file, strlen(file));  but in the reverse direction. Copying from the byte buffer to the file.
			ofstream output(writeToFile);
			// for (int i = 0; i < sizeof(fileData) + DATA_OFFSET; i++) {
			// 	output << file[i];
			// }
			int i = 0;
			while (file[i] != 0) {
				output << file[i];
				i++;
			}

			char ackBuffer[4];
			bzero(ackBuffer, 4);			
			// Pointer thats pointing to the start of the buffer array
			unsigned short *opPtr = (unsigned short*) ackBuffer;
			// convert from network order
			*opPtr = htons(OP_ACK);
			// pointer of op is pointing to start of the buffer array. fill in with op code data.
			opPtr++; // increment by 1 (unsigned short = 2 bytes), so now pointing to 3rd byte.
			
			// Have block pointer point to same as op pointer; the 3rd byte of buffer
			unsigned short *blockPtr = opPtr;
			// Fill in the block byte (from 3rd to 4th byte) with block number
			*blockPtr = htons(blockNumber);
			if (sendto(sockfd, ackBuffer, 4, 0, &pcli_addr, clilen) != sizeof(ackBuffer)) {
				printf("%s: sendto error\n",progname);
				exit(4);
			}
		}
	}
}

/* Main driver program. Initializes server's socket and calls the  */
/* dg_echo function that never terminates.                         */

int main(int argc, char *argv[]) {
	
/* General purpose socket structures are accessed using an         */
/* integer handle.                                                 */
	
	int                     sockfd;
	
/* The Internet specific address structure. We must cast this into */
/* a general purpose address structure when setting up the socket. */

	struct sockaddr_in      serv_addr;

/* argv[0] holds the program's name. We use this to label error    */
/* reports.                                                        */

	progname=argv[0];

/* Create a UDP socket (an Internet datagram socket). AF_INET      */
/* means Internet protocols and SOCK_DGRAM means UDP. 0 is an      */
/* unused flag byte. A negative value is returned on error.        */

	if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
		{
		 printf("%s: can't open datagram socket\n",progname);
		 exit(1); 
		}

/* Abnormal termination using the exit call may return a specific  */
/* integer error code to distinguish among different errors.       */
		
/* To use the socket created, we must assign to it a local IP      */
/* address and a UDP port number, so that the client can send data */
/* to it. To do this, we fisrt prepare a sockaddr structure.       */

/* The bzero function initializes the whole structure to zeroes.   */
	
	bzero((char *) &serv_addr, sizeof(serv_addr));
	
/* As sockaddr is a general purpose structure, we must declare     */
/* what type of address it holds.                                  */
	
	serv_addr.sin_family      = AF_INET;
	
/* If the server has multiple interfaces, it can accept calls from */
/* any of them. Instead of using one of the server's addresses,    */
/* we use INADDR_ANY to say that we will accept calls on any of    */
/* the server's addresses. Note that we have to convert the host   */
/* data representation to the network data representation.         */

	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

/* We must use a specific port for our server for the client to    */
/* send data to (a well-known port).                               */

	serv_addr.sin_port        = htons(SERV_UDP_PORT);

/* We initialize the socket pointed to by sockfd by binding to it  */
/* the address and port information from serv_addr. Note that we   */
/* must pass a general purpose structure rather than an Internet   */
/* specific one to the bind call and also pass its size. A         */
/* negative return value means an error occured.                   */

	if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
	       { 
		printf("%s: can't bind local address\n",progname);
		exit(2);
	       }

/* We can now start the echo server's main loop. We only pass the  */
/* local socket to dg_echo, as the client's data are included in   */
/* all received datagrams.                                         */

	dg_echo(sockfd);

/* The echo function in this example never terminates, so this     */
/* code should be unreachable.                                     */

}