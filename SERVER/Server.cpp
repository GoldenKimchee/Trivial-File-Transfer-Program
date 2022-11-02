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
#include <vector>

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

void dg_echo(int sockfd, int argc, char *argv[]) {
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

/* Initialize the maximum size of the structure holding the        */
/* client's address.                                               */

		clilen = sizeof(struct sockaddr);

/* Receive data on socket sockfd, up to a maximum of MAXSIZE - 1   */
/* bytes, and store them in mesg. The sender's address is stored   */
/* in pcli_addr and the structure's size is stored in clilen.      */

/* n holds the number of received bytes, or a negative number  */
/* to show an error condition. Notice how we use progname to label */
/* the source of the error.                                        */

// Server is requesting data packets
if (strcmp(argv[1], "-r") == 0) {
	// Server must first send the client a read request
	char rrqBuffer[MAX_BUFFER_SIZE];
	bzero(rrqBuffer, sizeof(rrqBuffer));
	unsigned short *opCodeSendPtr = (unsigned short*) rrqBuffer;
	*opCodeSendPtr = htons(OP_RRQ);
	opCodeSendPtr++;
	char *fileNamePtr = rrqBuffer + 2;
	memcpy(fileNamePtr, argv[2], strlen(argv[2]));
	if (sendto(sockfd, rrqBuffer, 4, 0, &pcli_addr, clilen) != sizeof(rrqBuffer)) {
		printf("%s: sendto error\n",progname);
		exit(4);
	}
	cout << "Sent the client a RRQ packet." << endl;

	// Continue to recieve all data packets 
	while (true) {
		// Following code assumes that the recieved packet is DATA type
		char buffer[MAX_BUFFER_SIZE];
		bzero(buffer, sizeof(buffer));
		if (int n < recvfrom(sockfd, buffer, MAX_BUFFER_SIZE, 0, &pcli_addr, (unsigned int*) &clilen)) {
			 printf("%s: recvfrom error getting data packet\n",progname);
			 exit(4);
		}
		cout << "Recieved a packet from client." << endl;
		//convert buffer to vector
		vector<char> bufferVector(buffer, buffer + sizeof(buffer) / sizeof(buffer[0]));
		//if data field is all 0
		if(bufferVector.end() == find(bufferVector.begin() + DATA_OFFSET, bufferVector.end(), false)) {
			//last packet, break
			cout << "Recieved last packet from client." << endl;
			break;
		}
		unsigned short *opCodePtrRcv = (unsigned short*) buffer;
		unsigned short opCodeRcv = ntohs(*opCodePtrRcv);
		if (opCodeRcv == OP_DATA) {
			opCodePtrRcv++;
			unsigned short *blockNumPtr = opCodePtrRcv;
			blockNumber = ntohs(*blockNumPtr);
			cout << "Received block #" << blockNumber << " of data." << endl;
			char *fileData = buffer + DATA_OFFSET;
			char file[MAXLINE];
			bcopy(fileData, file, sizeof(buffer) - DATA_OFFSET);
			ofstream output(argv[2]);
			int i = 0;
			while (file[i] != 0) {
				output << file[i];
				i++;
			}
		}

		// Send ACK packet for every DATA packet recieved
		char ackBuffer[4];
		bzero(ackBuffer, 4);			
		unsigned short *opPtr = (unsigned short*) ackBuffer;
		*opPtr = htons(OP_ACK);
		opPtr++; 
		unsigned short *blockPtr = opPtr;
		*blockPtr = htons(blockNumber);
		if (sendto(sockfd, ackBuffer, sizeof(ackBuffer), 0, &pcli_addr, clilen) != sizeof(ackBuffer)) {
			printf("%s: sendto error with ack packet\n",progname);
			exit(4);
		}
		cout << "Send ACK packet to client." << endl;
	}

	// Send last ACK packet to acknowledge last packet was recieved
	char ackBuffer[4];
	bzero(ackBuffer, 4);			
	unsigned short *opPtr = (unsigned short*) ackBuffer;
	*opPtr = htons(OP_ACK);
	opPtr++; 
	unsigned short *blockPtr = opPtr;
	*blockPtr = htons(blockNumber);
	if (sendto(sockfd, ackBuffer, sizeof(ackBuffer), 0, &pcli_addr, clilen) != sizeof(ackBuffer)) {
		printf("%s: sendto error with last ack packet\n",progname);
		exit(4);
		}
	}
	cout << endl;
	cout << "Done recieving all data packets." << endl;
	cout << "File should be fully downloaded." << endl;
	cout << endl;
	return;




		unsigned short *opCodePtr = (unsigned short*) buffer;
		unsigned short opCodeRcv = ntohs(*opCodePtr);
		// if conditionals checking OP code to decide how to process remaining buffer array
		if (opCodeRcv == OP_RRQ) { // Got read request
			cout<< "Recieved RRQ packet." << endl;
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
			std::ifstream in(filename);
			std::string contents((std::istreambuf_iterator<char>(in)), 
				std::istreambuf_iterator<char>());
			char file[contents.size()];
      strcpy(file, contents.c_str());
			memcpy(fileData, file, strlen(file));
			if (sendto(sockfd, data_buffer, sizeof(data_buffer), 0, &pcli_addr, clilen) != n) {
				printf("%s: sendto error\n",progname);
				exit(4);
			}
			cout<< "Server sent DATA packet of block #" << blockNumber << " to client." << endl;

		}	else if (opCodeRcv == OP_ACK) {

			cout<< "Recieved ACK packet." << endl;
			// Increment the pointer to the third byte for block number 
			opCodePtr++;
			// Create unsigned int pointer and assign the two bytes there to the unsigned int.
			unsigned short *blockNumPtr = opCodePtr;
			blockNumber = ntohs(*blockNumPtr) + 1; // update our block number

		} else if (opCodeRcv == OP_WRQ) { // Got a write request. Client is wants to send server a data packet
			
			cout<< "Recieved WRQ packet." << endl;
			opCodePtr++;  
			char *a = (char*) opCodePtr; 
			int i = 0;
			string filename = "";
			while (a[i] != 0) {
				filename = filename + a[i];
				i++;
			}
			writeToFile = filename;
			char ackBuffer[4];
			bzero(ackBuffer, 4);			
			unsigned short *opPtr = (unsigned short*) ackBuffer;
			*opPtr = htons(OP_ACK);
			opPtr++; 
			unsigned short *blockPtr = opPtr;
			*blockPtr = htons(blockNumber);
			if (sendto(sockfd, ackBuffer, sizeof(ackBuffer), 0, &pcli_addr, clilen) != sizeof(ackBuffer)) {
				printf("%s: sendto error wrq\n",progname);
				exit(4);
			}
			cout << "Server sent ACK packet of block #" << blockNumber << " to client." << endl;

		}	else if (opCodeRcv == OP_DATA) {
			cout<< "Recieved DATA packet." << endl;
			opCodePtr++;
			unsigned short *blockNumPtr = opCodePtr;
			blockNumber = ntohs(*blockNumPtr);
			char *fileData = buffer + DATA_OFFSET;
			char file[MAXLINE];
			bcopy(fileData, file, sizeof(buffer) - DATA_OFFSET);
			ofstream output(writeToFile);
			int i = 0;
			while (file[i] != 0) {
				output << file[i];
				i++;
			}
			cout << "Wrote DATA packet contents to file." << endl;

			char ackBuffer[4];
			bzero(ackBuffer, 4);			
			unsigned short *opPtr = (unsigned short*) ackBuffer;
			*opPtr = htons(OP_ACK);
			opPtr++; 
			
			unsigned short *blockPtr = opPtr;
			*blockPtr = htons(blockNumber);
			if (sendto(sockfd, ackBuffer, 4, 0, &pcli_addr, clilen) != sizeof(ackBuffer)) {
				printf("%s: sendto error\n",progname);
				exit(4);
			}
			cout << "Server sent ACK packet of block #" << blockNumber << " to client." << endl;
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

	dg_echo(sockfd, argc, argv);

	close(sockfd);
	exit(0);
}