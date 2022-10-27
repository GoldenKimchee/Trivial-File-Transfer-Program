//
//  main.cpp
//  TFTP
//
//  Created by B Pan on 10/24/22.
//

#include <iostream>
#include <string.h>
using namespace std;
// max length of data is 512 bytes, 2 bytes op code, 2 bytes block number. total 516 bytes.
const static int MAX_BUFFER_SIZE = 516;
const static unsigned short OP_CODE_DATA = 3;
const static int DATA_OFFSET = 4;
int main(int argc, const char * argv[]) {
    // insert code here...
    std::cout << "Hello, World!\n";
    
    // sender
    // array packet that is sent. should comply to tftp
    // Fill this buffer in as TFTP protocol! OP code, block number, file, data, etc...
    char buffer[MAX_BUFFER_SIZE];
    bzero(buffer, sizeof(buffer));
    
    // unsigned short is 2 bytes
    unsigned short opCode = OP_CODE_DATA;
    
    cout<< "size of unsigned short: " << sizeof(opCode) << " bytes." << endl;
    
    // Pointer thats pointing to the start of the buffer array
    unsigned short *opCodePtr = (unsigned short*) buffer;
    // convert from network order
    *opCodePtr = htons(OP_CODE_DATA);
    // pointer of op is pointing to start of the buffer array. fill in with op code data.
    opCodePtr++; // increment by 1 (unsigned short = 2 bytes), so now pointing to 3rd byte.
    
    unsigned short blockNum = 1;
    // Have block pointer point to same as op pointer; the 3rd byte of buffer
    unsigned short *blockNumPtr = opCodePtr;
    // Fill in the block byte (from 3rd to 4th byte) with block number
    *blockNumPtr = htons(blockNum);
    
    // file pointer is pointing to the 5th byte. buffer start + 4 = 5th byte
    char *fileData = buffer + DATA_OFFSET;
    // This part should be coming from the file in my code.
    // Read file and create the file array from it.
    // Whatever is contained in the file, just copy it byte by byte into the file array.
    // char is one byte each
    char file[] = "This is a demo";
    // Copy file contents into where fileData pointer is at
    strncpy (fileData, file, strlen(file)); // bcopy , memcpy
    
    // print in hex (actual byte. each hex is a byte, each hex char is 4 bits)
    //print the first 30 bytes of the buffer
    for ( int i = 0; i < 30; i++ ) {
        printf("0x%X,", buffer[i]);
    }
    cout<<endl<< "*****Receiver side*****"<<endl;
    
    // Receiver ;
    // We know based on TFTP protocol that the next two bytes are OP code
    unsigned short *opCodePtrRcv = (unsigned short*) buffer;
    // Take the two bytes at opCodeRcv pointer and convert it back from network byte order to host byte order using ntohs
    unsigned short opCodeRcv = ntohs(*opCodePtrRcv);
    
    cout<< "received opcode is: " << opCodeRcv <<endl;

    // if conditionals checking OP code to decide how to process remaining buffer array
    
    // Process rest of buffer array like done so above:
    // Increment the pointer to the third byte for block number 
    // Create unsigned int pointer and assign the two bytes there to the unsigned int.
    // Remember to convert to host byte order.
    // create pointer char, pointing to 5th byte of buffer, copy the byte to a file on reciever side
    // Do  strncpy (fileData, file, strlen(file));  but in the reverse direction. Copying from the byte buffer to the file.

    // parse block num
    // parse file data
    
    
    cout<<endl;
    return 0;
}