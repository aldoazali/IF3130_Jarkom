#include <sys/syscall.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <cstdlib>
#include <string.h>
#include <arpa/inet.h>
#include "init.hpp"

using namespace std;

#define TIMEOUT 1000
time_t waktu;

char* filename;
unsigned int windowsize;
unsigned int buffsize;
char* dest_ip;
int dest_port;

struct sockaddr_in serverAddr;
int sock, slen = sizeof(serverAddr);  //slen
SENDWINDOW window;
static Byte lastRChar; //

void error(std::string message) {
    perror(message.c_str());
    exit(EXIT_FAILURE);
}

void sendSegment(Byte segnum, Byte data, int sock, struct sockaddr_in serverAddr, int slen) {
    SEGMENT msg;
    msg.soh = C_SOH;
    msg.seqnum = segnum;
    msg.stx = C_STX;
    msg.data = data;
    msg.etx = C_ETX;
    msg.checksum = getChecksum(msg);

    if (sendto(sock, &msg, sizeof(SEGMENT), 0, (struct sockaddr*)&serverAddr, slen) < 0) {
        cout << "Error sendto Segment" << endl;
    }
}

void putBack(Byte data, SENDWINDOW* window) {
    unsigned int back = window->back;
    window->data[back] = data;
    window->ack[back] = false;
    window->startTime[back] = -1;
    window->back = (back + 1) % window->maxsize;
    window->count++;
}

void delHead(SENDWINDOW* window) {
    window->head = (window->head + 1) % window->maxsize;
    window->count--;
}

string createCRC(string bitStr) {
    static char result[8];
    char crc[7], invert;
    int  i;
   
    for (i=0; i<7; ++i){
        crc[i] = 0; 
    }                   
   
    for (i=0; i<bitStr.length(); i++) {
        invert = ('1' == bitStr[i]) ^ crc[6];
        crc[6] = crc[5] ^ invert;
        crc[5] = crc[4];
        crc[4] = crc[3] ^ invert;
        crc[3] = crc[2];
        crc[2] = crc[1] ^ invert;
        crc[1] = crc[0];
        crc[0] = invert;
    }
    for (i=0; i<7; ++i) {
        result[6-i] = crc[i] ? '1' : '0';
    }
    result[7] = 0;
    return result;
}

string getBitString(SEGMENT msg) {
    string bitStr = "";
    bitStr += bitset<8>(msg.soh).to_string();
    bitStr += bitset<8>(msg.seqnum).to_string();
    bitStr += bitset<8>(msg.stx).to_string();
    bitStr += bitset<8>(msg.data).to_string();
    bitStr += bitset<8>(msg.etx).to_string();
    return bitStr;
}

Byte getChecksum(SEGMENT msg) {
    string checkSumbit = createCRC(getBitString(msg));
    Byte checkSum = (Byte) (bitset<8>(checkSumbit).to_ulong());
    return checkSum;
}



void setting(int argc, char* argv[]) {
	if (argc != 5) {
        printf("Usage: ./sendfile <filename> <windowsize> <buffersize> "
        "<destination_ip> <destination_port>\n");
        exit(EXIT_FAILURE);
    }
    else{
		filename = argv[1];
		windowsize = atoi(argv[2]);
		buffsize = DEFAULT_BUFF_SIZE;
		dest_ip = argv[4];
		dest_port = atoi(argv[5]);	
		
		// UDP Socket
		if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
			error("Socket ERROR");
		} 	
		
		// Configure settings in address struct
		struct hostent* dest_host;
		long nHostAddress;
		dest_host = gethostbyname(dest_ip);
		if(!dest_host){
			error("Invalid host name");
		}
		memset(&serverAddr, 0, sizeof(serverAddr)); 
		serverAddr.sin_family = AF_INET;
		serverAddr.sin_port = htons(dest_port);
		//memset(serverAddr.sin_zero, '\0', sizeof(serverAddr.sin_zero));
		memcpy(&serverAddr.sin_addr, dest_host->h_addr_list[0], dest_host->h_length);
	}
}

int main(int argc, char* argv[]){
    setting(argc, argv);
	FILE *filesend;
	filesend = fopen(filename, "r");
	if(filesend == NULL){
		error("File doesn't exist !!");
	}
	// Main Algorithm
	bool endloop = false, endfile = false;
	Byte ch;
	int num = 0;
	while(!endloop){
		// Read message and put to window
		while(window.count <= window.maxsize / 2 && !endfile){
			if(lastRChar != XOFF){
				if(fscanf(filesend, "%c", &ch) == EOF){
					ch = endfile; // sequence endfile 
					endfile = true;
				}
				putBack(ch, &window);
			}
		}
		
		// Iterate through window and send frame which have not been ACK
		for(int i = window.head; i!=window.back; i = (i+1) % window.maxsize){
			if(!window.ack[i]){
				double timeDif = (double)(clock() - window.startTime[i])/CLOCKS_PER_SEC * 1000;
				if(timeDif > TIMEOUT || window.startTime[i]== -1 ){
					if(window.startTime[i] != -1) {
						printf("Time out frame number %d\n",i);
					}
					window.startTime[i] = clock();
					sendSegment(i, window.data[i], sock, serverAddr, slen);
					printf("Sent seqnum-%d: '%c'\n", i, window.data[i]);
				}
			} 
			else if(i == window.head && window.ack[i]){ // Slide Forward
				delHead(&window);
			}
		}
		if (endfile && window.head == window.back) {
			endloop = true;
		}
	}
	
	fclose(filesend);
	
	return 0;
}

