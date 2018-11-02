all: recvfile sendfile

recvfile: src/utils.h src/utils.cpp src/recvfile.cpp
	g++ -std=c++11 -pthread src/utils.cpp src/recvfile.cpp -o recvfile

sendfile: src/utils.h src/utils.cpp src/sendfile.cpp
	g++ -std=c++11 -pthread src/utils.cpp src/sendfile.cpp -o sendfile

clean: recvfile sendfile
	rm -f recvfile sendfile