//Sample program at client side for echo transmit-receive - CSS 432 - Autumn 2022
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <errno.h>          // for retrieving the error number.
#include <string.h>         // for strerror function.
#include <signal.h>         // for the signal handler registration.
#include <unistd.h>
#include <fstream>

#define SERV_UDP_PORT 51542 //REPLACE WITH YOUR PORT NUMBER
#define SERV_HOST_ADDR "10.158.82.33" //REPLACE WITH SERVER IP ADDRESS
const static unsigned short OP_RRQ = 1;
const static unsigned short OP_WRQ = 2;
const static unsigned short OP_DATA = 3;
const static unsigned short OP_ACK = 4;
const static unsigned short OP_ERROR = 5;
const static int DATA_OFFSET = 4;

/* A pointer to the name of this program for error reporting.      */

char *progname;

/* Size of maximum message to send.                                */

#define MAXLINE 512

/* Global variable to hold block number */
unsigned short blockNumber = 0;

/* Max length of data is 512 bytes, 2 bytes op code, 2 bytes block  */
/* number. total 516 bytes.											*/
const static int MAX_BUFFER_SIZE = 516;

main(argc, argv)
int     argc;
char    *argv[];
{
	int sockfd;
	
/* We need to set up two addresses, one for the client and one for */
/* the server.                                                     */
	
	struct sockaddr_in cli_addr, serv_addr;
	progname = argv[0];

/* Initialize first the server's data with the well-known numbers. */

	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	
/* The system needs a 32 bit integer as an Internet address, so we */
/* use inet_addr to convert the dotted decimal notation to it.     */

	serv_addr.sin_addr.s_addr = inet_addr(SERV_HOST_ADDR);
	serv_addr.sin_port = htons(SERV_UDP_PORT);

/* Create the socket for the client side.                          */
	
	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		printf("%s: can't open datagram socket\n",progname);
		exit(1);
	}

/* Initialize the structure holding the local address data to      */
/* bind to the socket.                                             */

	bzero((char *) &cli_addr, sizeof(cli_addr));
	cli_addr.sin_family = AF_INET;
	
/* Let the system choose one of its addresses for you. You can     */
/* use a fixed address as for the server.                          */
       
	cli_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	
/* The client can also choose any port for itself (it is not       */
/* well-known). Using 0 here lets the system allocate any free     */
/* port to our program.                                            */


	cli_addr.sin_port = htons(0);
	
/* The initialized address structure can be now associated with    */
/* the socket we created. Note that we use a different parameter   */
/* to exit() for each type of error, so that shell scripts calling */
/* this program can find out how it was terminated. The number is  */
/* up to you, the only convention is that 0 means success.         */

	if (bind(sockfd, (struct sockaddr *) &cli_addr, sizeof(cli_addr)) < 0) {
		printf("%s: can't bind local address\n",progname);
		exit(2);
	}
	
	/* The user has initialized a read request on the client side.	*/
	if (argv[1] == '-r') {

		/* Send out a read request by creating a read request       */
		/* as per TFTP protocol rfc1350 with opcode rrq and 		*/	
		/* filename. 												*/
		char rrqBuffer[MAX_BUFFER_SIZE];
		bzero(rrqBuffer, sizeof(rrqBuffer));
		unsigned short *opCodeSendPtr = (unsigned short*) rrqBuffer;
		*opCodeSendPtr = htons(OP_RRQ);
		opCodeSendPtr++;
		unsigned short *fileNamePtr = opCodeSendPtr;
		*fileNamePtr = htons(argv[2]);
		if (sendto(sockfd, fileNamePtr, sizeof(rrqBuffer), 0, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) != sizeof(rrqBuffer)) {
			printf("%s: sendto error on socket\n",progname);
			exit(3);
		}

		/* Recieve data block by creating buffer and parsing it 	*/
		/* as per TFTP protocol rfc1350 where a data block has an   */
		/* opcode (3), a block number, and data.   					*/
		char buffer[MAX_BUFFER_SIZE];
		bzero(buffer, sizeof(buffer));
		int n = recvfrom(sockfd, buffer, MAXLINE, 0, NULL, NULL);
		if (n < 0) {
			 printf("%s: recvfrom error\n",progname);
			 exit(4);
		}
		unsigned short *opCodePtrRcv = (unsigned short*) buffer;
		unsigned short opCodeRcv = ntohs(*opCodePtrRcv);
		if (opCodeRcv == OP_DATA) {
			opCodePtrRcv++;
			unsigned short *blockNumPtr = opCodePtrRcv
			blockNumber = ntohs(*blockNumPtr);
			char *fileData = buffer + DATA_OFFSET;
			char file[MAXLINE];
			memcpy(file, fileData, sizeof(buffer) - DATA_OFFSET);
			ofstream output(argv[2]);
			for (int i = 0; i < sizeof(buffer) - DATA_OFFSET; i++) {
				ofstream << file[i];
			}
		}
	}

	/* The user has initialized a read request on the client side.	*/
	else if (argv[1] == '-w') {

		/* Send out a read request by creating a read request       */
		/* as per TFTP protocol rfc1350 with opcode rrq and 		*/	
		/* filename. 												*/
		char wrqBuffer[MAX_BUFFER_SIZE];
		bzero(wrqBuffer, sizeof(wrqBuffer));
		unsigned short *opCodeSendPtr = (unsigned short*) wrqBuffer;
		*opCodeSendPtr = htons(OP_WRQ);
		opCodeSendPtr++;
		unsigned short *fileNamePtr = opCodeSendPtr;
		*fileNamePtr = htons(argv[2]);
		if (sendto(sockfd, fileNamePtr, sizeof(wrqBuffer), 0, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) != sizeof(wrqBuffer)) {
			printf("%s: sendto error on socket\n",progname);
			exit(3);
		}

		/* Recieve acknowledgment packet by creating buffer and 	*/
		/* parsing it as per TFTP protocol rfc1350 where an 		*/
		/* acknowledgement packet has an opcode (4) and a block 	*/
		/* number. 													*/
		char ackBuffer[MAX_BUFFER_SIZE];
		bzero(buffer, sizeof(buffer));
		int n = recvfrom(sockfd, buffer, MAXLINE, 0, NULL, NULL);
		if (n < 0) {
			printf("%s: recvfrom error\n",progname);
			exit(4);
		}
		unsigned short *opCodePtrRcv = (unsigned short*) buffer;
		unsigned short opCodeRcv = ntohs(*opCodePtrRcv);
		if (opCodeRcv == OP_ACK) {
			opCodePtrRcv++;
			unsigned short *blockNumPtr = opCodePtrRcv
			blockNumber = ntohs(*blockNumPtr);
		}

		/* Send out the data packet created from the file by 		*/
		/* creating a buffer and constructing a data packet as per	*/
		/* TFTP protocol rfc1350 where a data packet has an opcode  */
		/* (3), a block number, and data. 							*/
		char buffer[MAX_BUFFER_SIZE];
		bzero(buffer, sizeof(buffer));
		unsigned short *opCodePtr = (unsigned short*) buffer;
		*opCodePtr = htons(OP_DATA);
		opCodePtr++;
		unsigned short *blockNumPtr = opCodePtr;
		*blockNumPtr = htons(blockNumber);
		blockNumber++;
		char *fileData = buffer + DATA_OFFSET;
		std::ifstream in(argv[2]);
		std::string contents((std::istreambuf_iterator<char>(in)), 
    	std::istreambuf_iterator<char>());
		char file[] = contents.c_str();
		memcpy(fileData, file, strlen(file));
		if (sendto(sockfd, fileData, strlen(fileData), 0, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) != strlen(fileData)) {
			printf("%s: sendto error on socket\n",progname);
			exit(3);
		}
	}

/* We return here after the client sees the EOF and terminates.    */
/* We can now release the socket and exit normally.                */

	close(sockfd);
	exit(0);
}