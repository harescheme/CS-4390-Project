#include "socket_helper.hpp"

// Constructor
socket_reader::socket_reader(int connfd) : connfd(connfd) {
    std::fill(buffer, buffer + BUFFER_SIZE, 0);
}

// Reads one line from the socket. Returns 0 on success, < 0 for error
int socket_reader::read_line(std::string& line) {
    while (true) {
        while (buffer_index < buffer_bytes) {
            line += buffer[buffer_index++];
            if (line.back() == '\n') {
                return 0;
            }
        }
        int bytes_read = refresh_buffer();
        if (bytes_read < 0) {
            return bytes_read;
        }
    }
}

// Refreshes the internal buffer. Returns the number of bytes read, or < 0 for error
int socket_reader::refresh_buffer() {
    buffer_bytes = recv(connfd, buffer, BUFFER_SIZE - 1, 0);
    if (buffer_bytes < 0) {
        std::cerr << "Error reading from socket" << std::endl;
        return buffer_bytes;
    }
    buffer_index = 0;
    return buffer_bytes;
}

// Self explanatory, splits a string by a delimiter
std::vector<std::string> split_string(const std::string& str, char delimiter) {
    std::vector<std::string> tokens;
    std::stringstream ss(str);
    std::string token;
    while (std::getline(ss, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}