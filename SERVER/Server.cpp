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
#include <algorithm>
#include <filesystem>

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
unsigned short blockNumber = 0;

/* Global variable to hold number of consecutive timeouts */
int totalTimeouts = 0;

/* Global variable to hold number of seconds till timeout */
unsigned int timeout = 2;

/* Max length of data is 512 bytes, 2 bytes op code, 2 bytes block number. total 516 bytes. */
const static int MAX_BUFFER_SIZE = 516;

#define MAXLINE 512  // Actual data size

// Global variable to keep track of file to create (in case client sents a write request)
string writeToFile;

/* Size of maximum packet to received.                            */

#define MAXMESG 2048

// Handler for the SIGALRM signal. Increments corresponding global variables.
void handler(int signum) {
	// Terminate the connection after 10 consecutive timeouts
	if (totalTimeouts == 10) {
		cout << "10 consecutive timeouts. Terminate program." << endl;
		exit(7);
	}
	totalTimeouts += 1;  // Increment number of timeouts. Cannot have more than 10.
	cout << "Timeout occured." << endl;
}

int register_handler() {
	int rt_value = 0;
	// Register the handler function
	rt_value = (int) signal(SIGALRM, handle_timeout);
	if (rt_value == (int) SIG_ERR) {
		printf("Can't register funcction handler.\n");
		printf("signal() error: %s.\n", strerror(errno));
		return -1;
	}
	// Disable the restart of system call on signal. 
	// Otherwise OS will be stuck in the system call.
	rt_value = siginterrupt(SIGARLM, 1);
	if (rt_value == -1) {
		printf("invalid sign number.\n");
		return -1;
	}
	return 0;
}

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
// No timeout for the first packet to be recieved
		n = recvfrom(sockfd, buffer, MAX_BUFFER_SIZE, 0, &pcli_addr, (unsigned int*) &clilen);
/* n holds now the number of received bytes, or a negative number  */
/* to show an error condition. Notice how we use progname to label */
/* the source of the error.                                        */
		if (n < 0) {
			printf("%s: recvfrom error\n",progname);
			exit(3);
		}
		unsigned short *opCodePtr = (unsigned short*) buffer;
		unsigned short opCodeRcv = ntohs(*opCodePtr);
		// if conditionals checking OP code to decide how to process remaining buffer array

		if (opCodeRcv == OP_RRQ) { // Got read request
			cout<< "Recieved RRQ packet." << endl;
			blockNumber = 1;
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
			// File that the client wants to read does not exist
			if (!filesystem::exists(filename)) {
				cout << "Client wants to read a file that does not exist." << endl;

				// Build error packet with error message
				char buffer[MAX_BUFFER_SIZE];
				bzero(buffer, sizeof(buffer));
				unsigned short *opCodePtr = (unsigned short*) buffer;
				*opCodePtr = htons(OP_ERROR);
				opCodePtr++;
				unsigned short *blockNumPtr = opCodePtr;
				// Error code for File not found is 1
				*blockNumPtr = htons(1);
				char *fileData = buffer + DATA_OFFSET;
				string errorMsg = "File was not found in the server.";
				char msg[512];
				strcpy(msg, errorMsg.c_str());
				bcopy(msg, fileData, sizeof(msg));
				if (sendto(sockfd, buffer, sizeof(buffer), 0, &pcli_addr, clilen) != sizeof(buffer)) {			
					printf("%s: sendto error on socket\n",progname);
					exit(5);
				}

				cout << "Sent a error packet to client." << endl;
				break;
			}
			// No permission to access the requested file
			// In the case that the file on server is not readable
			else if (access(filename.c_str(), R_OK) != 0) {
				cout << filename << " does not have read permissions." << endl;

				// Build error packet with error message
				char buffer[MAX_BUFFER_SIZE];
				bzero(buffer, sizeof(buffer));
				unsigned short *opCodePtr = (unsigned short*) buffer;
				*opCodePtr = htons(OP_ERROR);
				opCodePtr++;
				unsigned short *blockNumPtr = opCodePtr;
				// Error code for Access Violation is 2
				*blockNumPtr = htons(2);
				char *fileData = buffer + DATA_OFFSET;
				string errorMsg = "File does not have read permissions.";
				// Starting at 5th byte of packet, fill in bytes of error message string
				char msg[512];
				strcpy(msg, errorMsg.c_str());
				bcopy(msg, fileData, sizeof(msg));
				
				if (sendto(sockfd, buffer, sizeof(buffer), 0, &pcli_addr, clilen) != sizeof(buffer)) {
					printf("%s: sendto error on socket\n",progname);
					exit(5);
				}

				cout << "Sent a error packet to client." << endl;
				break;
			}
			// Send data block to client
			std::ifstream in(filename);
			vector<char> contents((istreambuf_iterator<char>(in)), (istreambuf_iterator<char>()));
			for (int i = 0; i < contents.size(); i += 512) {
				char buffer[MAX_BUFFER_SIZE];
				bzero(buffer, sizeof(buffer));
				unsigned short *opCodePtr = (unsigned short*) buffer;
				*opCodePtr = htons(OP_DATA);
				opCodePtr++;
				unsigned short *blockNumPtr = opCodePtr;
				*blockNumPtr = htons(blockNumber);
				char *fileData = buffer + DATA_OFFSET;
				char file[512];

				vector<char> newContents(contents.begin() + i, contents.begin() + i + 512);
				copy(newContents.begin(), newContents.end(), file);

				bcopy(file, fileData, sizeof(file));
				if (sendto(sockfd, buffer, sizeof(buffer), 0, &pcli_addr, clilen) != sizeof(buffer)) {
					printf("%s: sendto error on socket\n",progname);
					exit(3);
				}
				cout << "Sent block #" << blockNumber << " of data" << endl;

				// Recieve ACK packet from client
				char ackBuffer[MAX_BUFFER_SIZE];
				bzero(ackBuffer, sizeof(ackBuffer));
				// Set a timer now.
				// If timed out at 3 seconds (timeout) we recieve SIGALRM.
				printf("Setting a timeout alarm\n");
				alarm(timeout);
				while (recvfrom(sockfd, ackBuffer, MAX_BUFFER_SIZE, 0, &pcli_addr, (unsigned int*) &clilen) < 0) {
					printf("%s: recvfrom error\n",progname);
					if (errno == EINTR) {  // There was a timeout
						printf("Timeout has triggered!\n");
						// Retransmit packet
						printf("Resending packet.\n");
						if (sendto(sockfd, buffer, sizeof(buffer), 0, &pcli_addr, clilen) != sizeof(buffer)) {
							printf("%s: sendto error on socket\n",progname);
							exit(3);
						}
						// Set a timer now.
						// If timed out at 3 seconds (timeout) we recieve SIGALRM.
						printf("Setting a timeout alarm\n");
						alarm(timeout);
					} else {
						exit(4);
					}
				}
				// Recieved from client. Reset timer 
				printf("Recieved data from server. Clear timeout alarm.\n")
				alarm(0);

				unsigned short *opCodePtrRcv = (unsigned short*) ackBuffer;
				unsigned short opCodeRcv = ntohs(*opCodePtrRcv);
				if (opCodeRcv == OP_ACK) {
					opCodePtrRcv++;
					unsigned short *blockNumPtr = opCodePtrRcv;
					blockNumber = ntohs(*blockNumPtr);
					cout << "Recieved Ack #" << blockNumber << endl;
					blockNumber++;
				}	
			}

		// Send last, zeroed out DATA packet
		char buffer[MAX_BUFFER_SIZE];
		bzero(buffer, sizeof(buffer));
		unsigned short *opCodePtr = (unsigned short*) buffer;
		*opCodePtr = htons(OP_DATA);
		opCodePtr++;
		unsigned short *blockNumPtr = opCodePtr;
		*blockNumPtr = htons(blockNumber);
		if (sendto(sockfd, buffer, sizeof(buffer), 0, &pcli_addr, clilen) != sizeof(buffer)) {
			printf("%s: sendto error wrq\n",progname);
			exit(4);
		}

		// Recieve last ACK packet
		char rrqAckBuffer[MAX_BUFFER_SIZE];
		bzero(rrqAckBuffer, sizeof(rrqAckBuffer));

		// Set a timer now.
		// If timed out at 3 seconds (timeout) we recieve SIGALRM.
		printf("Setting a timeout alarm\n");
		alarm(timeout);
		while (recvfrom(sockfd, rrqAckBuffer, MAX_BUFFER_SIZE, 0, &pcli_addr, (unsigned int*) &clilen) < 0) {
			printf("%s: recvfrom error\n",progname);
			if (errno == EINTR) {  // There was a timeout
				printf("Timeout has triggered!\n");
				// Retransmit packet
				printf("Resending packet.\n");
				if (sendto(sockfd, buffer, sizeof(buffer), 0, &pcli_addr, clilen) != sizeof(buffer)) {
					printf("%s: sendto error wrq\n",progname);
					exit(4);
				}
				// Set a timer now.
				// If timed out at 3 seconds (timeout) we recieve SIGALRM.
				printf("Setting a timeout alarm\n");
				alarm(timeout);
			} else {
				exit(4);
			}
		}
		// Recieved from client. Reset timer 
		printf("Recieved data from server. Clear timeout alarm.\n")
		alarm(0);

		unsigned short *rrqOpCodePtrRcv = (unsigned short*) rrqAckBuffer;
		unsigned short rrqOpCodeRcv = ntohs(*rrqOpCodePtrRcv);
		if (rrqOpCodeRcv == OP_ACK) {
			rrqOpCodePtrRcv++;
			unsigned short *blockNumPtr = rrqOpCodePtrRcv;
			blockNumber = ntohs(*blockNumPtr);
			blockNumber++;
			cout << "Recieved Ack #" << blockNumber << endl;
		}	

		cout << "Server is done processing read request for client." << endl;
		blockNumber = 0;
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

			// File already exists (overwrite warning)
			ifstream fileCheck(filename.c_str());
			if (fileCheck.good()) {
				cout << "Client wants to write to a file that already exists." << endl;

				// Build error packet with error message
				char buffer[MAX_BUFFER_SIZE];
				bzero(buffer, sizeof(buffer));
				unsigned short *opCodePtr = (unsigned short*) buffer;
				*opCodePtr = htons(OP_ERROR);
				opCodePtr++;
				unsigned short *blockNumPtr = opCodePtr;
				// Error code for File exists is 6
				*blockNumPtr = htons(6);
				char *fileData = buffer + DATA_OFFSET;
				string errorMsg = "File already exists in the server. File was not overwritten";
				char msg[512];
				strcpy(msg, errorMsg.c_str());
				bcopy(msg, fileData, sizeof(msg));
				
				if (sendto(sockfd, buffer, sizeof(buffer), 0, &pcli_addr, clilen) != sizeof(buffer)) {
					printf("%s: sendto error on socket\n",progname);
					exit(5);
				}

				cout << "Sent a error packet to client." << endl;
				break;
			}
			fileCheck.close();
			// Send ACK packet to client
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

		// Continue to recieve DATA packets and send corresponding ACK packets to client
		ofstream output(filename);
		while (true) {
		// Recieve DATA packet
		char buffer[MAX_BUFFER_SIZE];
		bzero(buffer, sizeof(buffer));

		// Set a timer now.
		// If timed out at 3 seconds (timeout) we recieve SIGALRM.
		printf("Setting a timeout alarm\n");
		alarm(timeout);
		while (recvfrom(sockfd, buffer, MAX_BUFFER_SIZE, 0, &pcli_addr, (unsigned int*) &clilen) < 0) {
			printf("%s: recvfrom error\n",progname);
			if (errno == EINTR) {  // There was a timeout
				printf("Timeout has triggered!\n");
				// Wait for another timeout cycle. Set a timer now.
				// If timed out at (timeout) seconds we recieve SIGALRM.
				printf("Setting a timeout alarm\n");
				alarm(timeout);
			} else {
				exit(4);
			}
		}
		// Recieved from client. Reset timer 
		printf("Recieved data from server. Clear timeout alarm.\n")
		alarm(0);
		
		//convert buffer to vector
		vector<char> bufferVector(buffer, buffer + sizeof(buffer) / sizeof(buffer[0]));
		//if data field is all 0
		vector<char> newBuffer(bufferVector.begin() + DATA_OFFSET, bufferVector.end());
		bool is_clear = all_of(newBuffer.cbegin(), newBuffer.cend(), [](unsigned char c) {return c == 0; });
		if(is_clear) {
			//last packet, break
			cout << "Recieved last packet" << endl;
			break;
		}

		unsigned short *opCodePtrRcv = (unsigned short*) buffer;
		unsigned short opCodeRcv = ntohs(*opCodePtrRcv);
		if (opCodeRcv == OP_DATA) {
			opCodePtrRcv++;
			unsigned short *blockNumPtr = opCodePtrRcv;
			blockNumber = ntohs(*blockNumPtr);
			cout << "Received block #" << blockNumber << " of data" << endl;
			char *fileData = buffer + DATA_OFFSET;
			char file[MAXLINE];
			bcopy(fileData, file, sizeof(buffer) - DATA_OFFSET);
			int i = 0;
			while (file[i] != 0) {
				output << file[i];
				i++;
			}
		}

		// Send client an ACK packet
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
	}

	// Send last ACK packet after recieving last (all-zero) DATA packet
	char finalAckBuffer[4];
	bzero(finalAckBuffer, 4);			
	unsigned short *finalOpPtr = (unsigned short*) finalAckBuffer;
	*finalOpPtr = htons(OP_ACK);
	finalOpPtr++; 
	unsigned short *finalBlockPtr = finalOpPtr;
	*finalBlockPtr = htons(blockNumber);
	if (sendto(sockfd, finalAckBuffer, sizeof(finalAckBuffer), 0, &pcli_addr, clilen) != sizeof(finalAckBuffer)) {
		printf("%s: sendto error wrq\n",progname);
		exit(4);
	}
	cout << "Server is done processing write request for client." << endl;
	blockNumber = 0;
		
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

// Specify port number at runtime from command line
	if (argc == 3) {
		serv_addr.sin_port = htons(stoi(argv[2]));
	} else {
		serv_addr.sin_port        = htons(SERV_UDP_PORT);
	}

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

cout << "Register the timeout handler." << endl;
if (register_handler() != 0) {
	printf("Failed to register timeout...\n");
}
/* We can now start the echo server's main loop. We only pass the  */
/* local socket to dg_echo, as the client's data are included in   */
/* all received datagrams.                                         */

	dg_echo(sockfd);

/* The echo function in this example never terminates, so this     */
/* code should be unreachable.                                     */

}