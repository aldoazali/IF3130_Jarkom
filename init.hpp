#ifndef _INIT_HPP_
#define _INIT_HPP_

#include <algorithm>
#include <arpa/inet.h>
#include <bitset>
//#include <chrono>
#include <cstring>
#include <ctime>
#include <iostream>
#include <netinet/in.h>
#include <sstream>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>  
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

/* ASCII Constants */
#define C_SOH 1
#define C_STX 2
#define C_ETX 3
#define C_ENQ 5
#define C_ACK 6
#define C_NAK 21
#define C_EOF -1
#define DEFAULT_BUFF_SIZE 256 

/* XON/XOFF protocol */
#define XON (0x11)
#define XOFF (0x13)



using namespace std;

typedef char Byte;

typedef struct QTYPE
{
	Byte *data;
	unsigned int head;
	unsigned int back;
	unsigned int count;
	unsigned int maxsize;
} QTYPE;

typedef struct SEGMENT {
	Byte data;
	Byte soh;
	Byte stx;
	Byte etx;
	Byte checksum;
	unsigned int seqnum;
} SEGMENT;

typedef struct ACK {
	Byte ack;
	Byte advWinsize;
	Byte checksum;
	unsigned int nextSeqnum;
} ACK;

typedef struct SENDWINDOW {
	Byte *data;
	bool *ack;
	clock_t *startTime;
	unsigned int count;
	unsigned int back;
	unsigned int head;
	unsigned int maxsize;
} SENDWINDOW;

//Send message
void sendSegment(Byte seqnum, Byte data, int sock, struct sockaddr_in receiverAddr, int slen);

//Add data to back of window
void putBack(Byte data, SENDWINDOW* window);

//Remove head data from window
void delHead(SENDWINDOW* window);

//Get string of CRC from bitstring
string createCRC(string bitStr);

//Generate BitString from message
string getBitString(SEGMENT msg);

//Generate checksum from message to be sent
Byte getChecksum(SEGMENT msg);

void* receiveResponse(void*);
void* receiveMessage(void*);
#endif
