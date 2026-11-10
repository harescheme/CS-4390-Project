#include <iostream>
#include <thread>

#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include "socket_helper.hpp"

// Downloads a file given the tracker file.
void download_file(int connfd /*, some other parameters */) {
	// TODO implement, set parameters
}

int main(int argc, char *argv[]) {
   	// TODO change to configuration file
	// Address of tracker server
    int port = 3490;
    std::string ip_addr = "0.0.0.0";

    // socket
    // SOCK_STREAM is TCP, SOCK_DGRAM is UDP
    int fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd < 0) {
		std::cerr << "socket() failed" << std::endl;
		return 1;
	}

    // bind
    int opt = 1;
    // SO_REUSEADDR lets the program reuse IP:PORT after restart, should always be used
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    sockaddr_in addr {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (!inet_aton(ip_addr.c_str(), &addr.sin_addr)) {
        std::cerr << "Invalid IP address given" << std::endl;
        return 1;
    }
    
    // connect
    if (connect(fd, (const sockaddr *) &addr, sizeof(addr))) {
        std::cerr << "bind() failed" << std::endl;
        return 1;
    }


   /* If connected successfully*/

	if(!strcmp(argv[1],"list")){// if this is LIST command
		int list_req=htons(LIST);
		if((write(sockid,(char *)&list_req,sizeof(list_req))) < 0){//inform the server of the list request
			printf("Send_request  failure\n"); exit(0);
		}

        if((read(sockid,(char *)&msg,sizeof(msg)))< 0){// read what server has said
			printf("Read  failure\n"); exit(0);
		}

		close(sockid);
		printf("Connection closed\n");
	    exit(0);
	}//end close


}
