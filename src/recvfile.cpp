#include <iostream>
#include <stdio.h>
#include <thread>
#include <sys/socket.h>
#include <netdb.h>
#include "utils.h"

using namespace std;

int socket_fd;
struct sockaddr_in serverAddress, clientAddress;

int main(int argc, char * argv[]) {
    int port;
    int windowSize;
    int maxBufferSize;
    char *fname;

    if (argc == 5) {
        fname = argv[1];
        windowSize = (int) atoi(argv[2]);
        maxBufferSize = MAX_DATA_SIZE * (int) atoi(argv[3]);
        port = atoi(argv[4]);
    } else {
        cerr << "usage: ./recvfile <filename> <window_size> <buffer_size> <port>" << endl;
        return 1;
    }

    memset(&serverAddress, 0, sizeof(serverAddress)); 
    memset(&clientAddress, 0, sizeof(clientAddress)); 

    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port = htons(port);

    if ((socket_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        cerr << "socket creation failed" << endl;
        return 1;
    }

    if (::bind(socket_fd, (const struct sockaddr *)&serverAddress, 
            sizeof(serverAddress)) < 0) { 
        cerr << "socket binding failed" << endl;
        return 1;
    }

    cout << "Waiting for transmission on port " << port << endl << endl;

    FILE *file = fopen(fname, "wb");
    char buffer[maxBufferSize];
    int bufferSize;
    char frame[MAX_FRAME_SIZE];
    char data[MAX_DATA_SIZE];
    char ack[ACK_SIZE];
    int frameSize;
    int dataSize;
    int lfr, laf;
    int receivedSeqNumber;
    bool eot;
    bool isFrameError;

    /* Receive frames until End of Transmission */
    bool isDoneReceived = false;
    int bufferNum = 0;
    while (!isDoneReceived) {
        bufferSize = maxBufferSize;
        memset(buffer, 0, bufferSize);
    
        int receivedSeqCount = (int) maxBufferSize / MAX_DATA_SIZE;
        bool windowReceived[windowSize];
        for (int i = 0; i < windowSize; i++) {
            windowReceived[i] = false;
        }
        lfr = -1;
        laf = lfr + windowSize;
        
        /* Receive current buffer with sliding window */
        while (true) {
            socklen_t clientAddressSize;
            frameSize = recvfrom(socket_fd, (char *) frame, MAX_FRAME_SIZE, 
                    MSG_WAITALL, (struct sockaddr *) &clientAddress, 
                    &clientAddressSize);
            isFrameError = readFrame(&receivedSeqNumber, data, &dataSize, &eot, frame);

            if (isFrameError) {
                cout << endl << "->x Frame received contains errors";
                cout << endl << "<- Sending NAK";
            } else {
                cout << endl << "-> Frame " << receivedSeqNumber << " received";
                cout << endl << "<- Sending ACK";
            }
            createACK(receivedSeqNumber, ack, isFrameError);
            sendto(socket_fd, ack, ACK_SIZE, 0, 
                    (const struct sockaddr *) &clientAddress, clientAddressSize);

            if (receivedSeqNumber <= laf) {
                if (!isFrameError) {
                    int bufferShift = receivedSeqNumber * MAX_DATA_SIZE;

                    if (receivedSeqNumber == lfr + 1) {
                        memcpy(buffer + bufferShift, data, dataSize);

                        int shift = 1;
                        for (int i = 1; i < windowSize; i++) {
                            if (!windowReceived[i]) break;
                            shift += 1;
                        }
                        for (int i = 0; i < windowSize - shift; i++) {
                            windowReceived[i] = windowReceived[i + shift];
                        }
                        for (int i = windowSize - shift; i < windowSize; i++) {
                            windowReceived[i] = false;
                        }
                        lfr += shift;
                        laf = lfr + windowSize;
                    } else if (receivedSeqNumber > lfr + 1) {
                        if (!windowReceived[receivedSeqNumber - (lfr + 1)]) {
                            memcpy(buffer + bufferShift, data, dataSize);
                            windowReceived[receivedSeqNumber - (lfr + 1)] = true;
                        }
                    }

                    /* Set max sequence to sequence of frame with EOT */ 
                    if (eot) {
                        bufferSize = bufferShift + dataSize;
                        receivedSeqCount = receivedSeqNumber + 1;
                        isDoneReceived = true;
                    }
                } else {
                    cout << endl << endl << "[x] Frame received contains errors" << endl;
                }
            }
            
            /* Move to the next buffer if all frames in current buffer has been received */
            if (lfr >= receivedSeqCount - 1) break;
        }

        cout << "\n" << "[RECEIVED " << (unsigned long long) bufferNum * (unsigned long long) 
                maxBufferSize + (unsigned long long) bufferSize << " BYTES]" << flush;
        fwrite(buffer, 1, bufferSize, file);
        bufferNum += 1;
    }

    fclose(file);

    cout << "\nData Received" << endl;
    return 0;
}

void sendACK() {
    char frame[MAX_FRAME_SIZE];
    char data[MAX_DATA_SIZE];
    char ack[ACK_SIZE];
    int frameSize;
    int dataSize;
    socklen_t clientAddressSize;
    
    int receivedSeqNumber;
    bool isFrameError;
    bool eot;

    /* Listen for frames and send ack */
    while (true) {
        frameSize = recvfrom(socket_fd, (char *)frame, MAX_FRAME_SIZE, 
                MSG_WAITALL, (struct sockaddr *) &clientAddress, 
                &clientAddressSize);
        isFrameError = readFrame(&receivedSeqNumber, data, &dataSize, &eot, frame);

        createACK(receivedSeqNumber, ack, isFrameError);
        sendto(socket_fd, ack, ACK_SIZE, 0, 
                (const struct sockaddr *) &clientAddress, clientAddressSize);
    }
}