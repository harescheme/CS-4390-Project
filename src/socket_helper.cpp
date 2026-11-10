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


socket_writer::socket_writer(int connfd) : connfd(connfd) {}

// Flushes the internal buffer after appending data using the << operator.
// Returns 0 on success, < 0 for error
int socket_writer::flush() {
    std::string data = buffer.str();
    ssize_t bytes_sent = send(connfd, data.c_str(), data.size(), 0);
    if (bytes_sent < 0) {
        std::cerr << "Error writing to socket" << std::endl;
        return bytes_sent;
    }

    // Clear the buffer after sending
    buffer.str("");
    buffer.clear();
    return 0;
}
