#include <sys/syscall.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <cstdlib>
#include <string.h>
#include <arpa/inet.h>
#include "init.hpp"

using namespace std;

time_t waktu;
stringstream sendlog;

char* filename;
unsigned int windowsize;
unsigned int buffsize;
char* dest_ip;
int dest_port;

struct sockaddr_in serverAddr, clientAddr;
int sock, slen = sizeof(serverAddr);
SENDWINDOW window;
static Byte lastReceivedChar;

void error(std::string message) {
    perror(message.c_str());
    exit(EXIT_FAILURE);
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
		serverAddr.sin_family = AF_INET;
		serverAddr.sin_port = htons(dest_port);
		memset(serverAddr.sin_zero, '\0', sizeof(serverAddr.sin_zero));

		memset(&serverAddr, 0, sizeof(serverAddr)); 
		memset(&clientAddr, 0, sizeof(clientAddr)); 
		//memcpy(&recvAddr.sin_addr, dest_host->h_addr_list[0], dest_host->h_length);

	}
}

int main(int argc, char* argv[]){
    setting(argc, argv);
    	
	FILE *filesend;
	filesend = fopen(filename, "r");
	if(filesend == NULL){
		error("File doesn't exist !!");
	}
	// Sliding Window
	
	
	
	// Write Log
	fclose(filesend);
	filesend = fopen("logsend.txt", "w");
	string logsend = sendlog.str();
	replace(logsend.begin(), logsend.end(), '\n', ' ');
	replace(logsend.begin(), logsend.end(), '.', '\n');
	fprintf(filesend, "%s", logsend.c_str());
	fclose(filesend);
	
	
	return 0;
}

