#ifndef SOCKET_HELPER_H
#define SOCKET_HELPER_H

#include <iostream>
#include <sstream>
#include <vector>

#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h> 

#include "utils.hpp"

// Utilities to help with reading from sockets

constexpr int BUFFER_SIZE = 1024;

class socket_reader {
    int connfd;
    char buffer[BUFFER_SIZE];
    int buffer_bytes = 0;
    int buffer_index = 0;

public:
    socket_reader(int connfd);
    int read_line(std::string& line);
    int refresh_buffer();
};

class socket_writer {
    int connfd;
    std::stringstream buffer;

public:
    socket_writer(int connfd);

    template <typename T>
    socket_writer& operator<<(const T& data) {
        buffer << data;
        return *this;
    }
    int flush();
};

#endif