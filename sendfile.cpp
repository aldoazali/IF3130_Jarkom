#include <iostream>
#include <thread>
#include <mutex>

#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>

#include "helpers.h"

#define TIMEOUT 1000

using namespace std;

int sock;
struct sockaddr_in serverAddr, clientAddr;

int window_size;
bool *get_ack;
time_stamp *send_time;
int lar, lfs;

time_stamp TMIN = current_time();
mutex window_mutex;

void error(std::string message) {
    perror(message.c_str());
    exit(EXIT_FAILURE);
}

void listen_ack() {
    char ack[ACK_SIZE];
    int ack_size;
    int ack_seqnum;
    bool ack_error;
    bool ack_neg;

    /* Listen for ack from reciever */
    while (true) {
        socklen_t server_size;
        ack_size = recvfrom(sock, (char *)ack, ACK_SIZE, 
                MSG_WAITALL, (struct sockaddr *) &serverAddr, 
                &server_size);
        ack_error = readACK(&ack_seqnum, &ack_neg, ack);

        window_mutex.lock();

        if (!ack_error && ack_seqnum > lar && ack_seqnum <= lfs) {
            if (!ack_neg) {
                get_ack[ack_seqnum - (lar + 1)] = true;
                cout << endl << "-> ACK with sequence number " << ack_seqnum << " is just received";
            } else {
                send_time[ack_seqnum - (lar + 1)] = TMIN;
            }
        }

        window_mutex.unlock();
    }
}

int main(int argc, char *argv[]) {
    char *dest_ip;
    int dest_port;
    int max_buffer_size;
    struct hostent *dest_hostname;
    char *fname;

    if (argc == 6) {
        fname = argv[1];
        window_size = atoi(argv[2]);
        max_buffer_size = MAX_DATA_SIZE * (int) atoi(argv[3]);
        dest_ip = argv[4];
        dest_port = atoi(argv[5]);
    } else {
        cerr << "usage: ./sendfile <filename> <window_size> <buffer_size> <destination_ip> <destination_port>" << endl;
        exit(EXIT_FAILURE);
    }

    /* Get hostnet from server hostname or IP address */ 
    dest_hostname = gethostbyname(dest_ip); 
    if (!dest_hostname) {
		error("Could not resolve host name");
    }

    memset(&serverAddr, 0, sizeof(serverAddr)); 
    memset(&clientAddr, 0, sizeof(clientAddr)); 

    /* Fill server address data structure */
    serverAddr.sin_family = AF_INET;
    bcopy(dest_hostname->h_addr, (char *)&serverAddr.sin_addr, 
            dest_hostname->h_length); 
    serverAddr.sin_port = htons(dest_port);

    // Fill client address data structure 
    clientAddr.sin_family = AF_INET;
    clientAddr.sin_addr.s_addr = INADDR_ANY; 
    clientAddr.sin_port = htons(0);

    // Create socket file descriptor 
    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        error("Create Socket failed");
    }

    // Bind socket to client address
    if (::bind(sock, (const struct sockaddr *)&clientAddr, sizeof(clientAddr)) < 0) { 
        error("Binding Socket failed");
    }
    
    if (access(fname, F_OK) == -1) {
        error("File not found");
    }

    /* Open file to send */
    FILE *fp = fopen(fname, "rb");
    char buffer[max_buffer_size];
    int buffer_size;

    /* Start thread to listen for ack */
    thread recv_thread(listen_ack);

    char frame[MAX_FRAME_SIZE];
    char data[MAX_DATA_SIZE];
    int frame_size;
    int data_size;

    /* Send file */
    bool read_done = false;
    int buffer_num = 0;
    while (!read_done) {

        /* Read part of file to buffer */
        cout << endl << "[!] Read file and put to buffer";
        buffer_size = fread(buffer, 1, max_buffer_size, fp);
        if (buffer_size == max_buffer_size) {
            char temp[1];
            int next_buffer_size = fread(temp, 1, 1, fp);
            if (next_buffer_size == 0) read_done = true;
            int error = fseek(fp, -1, SEEK_CUR);
        } else if (buffer_size < max_buffer_size) {
            read_done = true;
        }
        
        window_mutex.lock();

        /* Initialize sliding window variables */
        int seq_count = buffer_size / MAX_DATA_SIZE + ((buffer_size % MAX_DATA_SIZE == 0) ? 0 : 1);
        int seq_num;
        send_time = new time_stamp[window_size];
        get_ack = new bool[window_size];
        bool window_sent_mask[window_size];
        for (int i = 0; i < window_size; i++) {
            get_ack[i] = false;
            window_sent_mask[i] = false;
        }
        lar = -1;
        lfs = lar + window_size;

        window_mutex.unlock();
        
        /* Send current buffer with sliding window */
        bool send_done = false;
        while (!send_done) {

            window_mutex.lock();

            /* Check window ack mask, shift window if possible */
            if (get_ack[0]) {
                int shift = 1;
                for (int i = 1; i < window_size; i++) {
                    if (!get_ack[i]) break;
                    shift += 1;
                }
                for (int i = 0; i < window_size - shift; i++) {
                    window_sent_mask[i] = window_sent_mask[i + shift];
                    get_ack[i] = get_ack[i + shift];
                    send_time[i] = send_time[i + shift];
                }
                for (int i = window_size - shift; i < window_size; i++) {
                    window_sent_mask[i] = false;
                    get_ack[i] = false;
                }
                lar += shift;
                lfs = lar + window_size;
            }

            window_mutex.unlock();

            /* Send frames that has not been sent or has timed out */
            for (int i = 0; i < window_size; i ++) {
                seq_num = lar + i + 1;

                if (seq_num < seq_count) {
                    window_mutex.lock();

                    if (!window_sent_mask[i] || (!get_ack[i] && (elapsed_time(current_time(), send_time[i]) > TIMEOUT))) {
                        if (window_sent_mask[i] && !get_ack[i] && (elapsed_time(current_time(), send_time[i]) > TIMEOUT)) {
                            cout << endl << "[x] Frame ACK " << seq_num << " has timed out";
                        }
                        int buffer_shift = seq_num * MAX_DATA_SIZE;
                        data_size = (buffer_size - buffer_shift < MAX_DATA_SIZE) ? (buffer_size - buffer_shift) : MAX_DATA_SIZE;
                        memcpy(data, buffer + buffer_shift, data_size);
                        
                        bool eot = (seq_num == seq_count - 1) && (read_done);
                        frame_size = createFrame(seq_num, frame, data, data_size, eot);

                        sendto(sock, frame, frame_size, 0, 
                                (const struct sockaddr *) &serverAddr, sizeof(serverAddr));
                        window_sent_mask[i] = true;
                        send_time[i] = current_time();
                        cout << endl << "<- Sending Frame " << seq_num;
                    }

                    window_mutex.unlock();
                }
            }

            /* Move to next buffer if all frames in current buffer has been acked */
            if (lar >= seq_count - 1) send_done = true;
        }

        cout << endl << endl << "[SENT " << (unsigned long long) buffer_num * (unsigned long long) 
                max_buffer_size + (unsigned long long) buffer_size << " BYTES]" << endl;
        buffer_num += 1;
        if (read_done) break;
    }
    
    fclose(fp);
    delete [] get_ack;
    delete [] send_time;
    recv_thread.detach();

    cout << "\nData Sent" << endl;
    return 0;
}
