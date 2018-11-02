# Sliding Windows Jarkom


## Compiling

Change Directory to the project folder and use "make" command to correctly compile the program.

## Initiate File

Create A file with any name specified on the root folder of the program.

## Running The Program

Open 2 Command Prompt one serve as sender and other serve as receiver. On the Sender Command Prompt write the command ./sendfile <filename> <window_len> <buffer_size> <destination_ip> <destination_port>
and in the Receiver Command Prompt write the command
./recvfile <filename> <window_size> <buffer_size> <port>
After that the program will send the file specified in the sendfile command and output it in the file specified in the receiver command.


# Cara Kerja Sliding Windows

### Sender :
    1. First, Program will read the file specified in the sender command
    2. Program then will start thread to listen for ack
    3. Create Buffer lists, While read is not done then do :
        1. Read File and Put to buffer
        2. If buffer size == Max_Buffer Size then check if file had been fully read if yes then break from loop else     continue loop
        3. Else continue Loop
    4. Send buffers, While sending is not done do :
        1. Check windows ack mask then shift windows if possible
        2. Send frames that has not been sent or has timed out
        3. Move to next buffer if all frames in current buffer has been acked
    5. Finish send

### Receiver :
    1. Listen for transmission
    2. If transmission received stop listening and start receiving frames
    3. While receiving is not done yet do :
        1. If Frame contains error send NAK
        2. Else create ACK and sent to the sender port
        3. Then append the buffer received to output file.
        4. Set max sequence to sequence of frame with EOT
        5. Move to next buffer if all frames in current buffer has been received
    4. Start thread to keep sending requested ack to sender for 3 seconds


# Pembagian Tugas

13516008 - Muhammad Aufa Helfiandri
13516056 - Muhammad Rafly Alkhadafi
13516125 - Aldo Azali
