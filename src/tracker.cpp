#include <iostream>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

static void handle_request(int connfd) {
    
}

int main() {
    // TODO change to configuration file
    int port = 1234;
    std::string ip_addr = "0.0.0.0";

    // socket
    // SOCK_STREAM is TCP, SOCK_DGRAM is UDP
    int fd = socket(AF_INET, SOCK_STREAM, 0);

    // bind
    int opt = 1;
    // SO_REUSEADDR lets the program reuse IP:PORT after restart, should always be used
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    sockaddr_in addr {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (inet_aton(ip_addr.c_str(), &addr.sin_addr)) {
        std::cerr << "Invalid IP address given" << std::endl;
        return 1;
    }
    
    // bind
    if (bind(fd, (const sockaddr *) &addr, sizeof(addr))) {
        std::cerr << "bind() failed" << std::endl;
        return 1;
    }

    // listen
    if (listen(fd, SOMAXCONN)) {
        std::cerr << "listen() failed" << std::endl;
        return 1;
    }
    std::cerr << "Listening on port " << port << std::endl;

    while (true) {
        // accept
        sockaddr_in client_addr {};
        socklen_t addrlen = sizeof(client_addr);
        int connfd = accept(fd, (sockaddr *) &client_addr, &addrlen);
        if (connfd < 0) {
            std::cerr << "Error accepting connection" << std::endl;
            continue;
        }

        // child process
        if (fork() == 0) {
            close(fd);  //child does not need listener
            handle_request(connfd);
            close (connfd);
            exit(0);    // kill the process. child process all done with work
        }

        close(connfd);
    }
}