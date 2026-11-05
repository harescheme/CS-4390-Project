#ifndef SOCKET_HELPER
#define SOCKET_HELPER

#include <iostream>
#include <sstream>
#include <vector>

#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h> 

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

std::vector<std::string> split_string(const std::string& str, char delimiter);

#endif