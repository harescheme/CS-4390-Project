#include <iostream>
#include <map>
#include <functional>
#include <thread>

#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include "socket_helper.hpp"
#include "trackerfile.hpp"

// Tracker Server-ONLY handlers

void create_tracker_handler(int connfd, const std::vector<std::string>& args) {
    // TODO implement create_tracker_handler
    if (args.size() < 7) {
        std::cerr << "Invalid number of arguments for createtracker" << std::endl;
        return;
    }

    // socket_writer writer(connfd);
    // tracker_file tfile;
    
}

void update_tracker_handler(int connfd, const std::vector<std::string>& args) {
    // TODO implement update_tracker_handler
}

void list_handler(int connfd, const std::vector<std::string>& args) {
    // TODO implement list_handler
    if (args.size() < 2 || args[1] != "LIST") {
        std::cerr << "Invalid request for LIST" << std::endl;
        return;
    }

    socket_writer writer(connfd);
    std::vector<tracker_file> tracker_files = list_tracker_files();
    
    writer << "REP LIST " << tracker_files.size() << '\n';
    int index = 1;
    for (const auto& tfile : tracker_files) {
        writer << index << ' ' << tfile.filename << ' ' << tfile.filesize << ' ' << tfile.md5 << '\n';
        index++;
    }

    writer << "REP LIST END\n";
    if (writer.flush()) {
        std::cerr << "Failed to send response" << std::endl;
    }
}

void get_handler(int connfd, const std::vector<std::string>& args) {
    // TODO implement get_handler
    if (args.size() < 2) {
        std::cerr << "Invalid number of arguments for GET" << std::endl;
        return;
    }

    socket_writer writer(connfd);
    writer << "REP GET BEGIN\n";
    tracker_file tfile;
    if (!read_tracker_file(args[1], tfile)) {
        std::cerr << "Failed to read tracker file: " << args[1] << std::endl;
        return;
    }

    std::string tfile_str = tracker_file_to_string(tfile);
    writer << tfile_str;
    writer << "REP GET END " << md5(tfile_str) << "\n";
    if (writer.flush()) {
        std::cerr << "Failed to send response" << std::endl;
    }
}

// Handlers for each request
static const std::map<std::string, std::function<void(int, const std::vector<std::string>&)>> handlers = {
    {"createtracker", create_tracker_handler},
    {"updatetracker", update_tracker_handler},
    {"REQ", list_handler},
    {"GET", get_handler}
};

static void handle_request(int connfd) {
    socket_reader reader(connfd);
    std::string line;
    reader.read_line(line);

    std::vector<std::string> args = split_string(line, ' ');
    if (args.empty()) {
        std::cerr << "Empty request received" << std::endl;
        return;
    }
    
    auto it = handlers.find(args[0]);
    if (it == handlers.end()) {
        std::cerr << "Unknown request: " << args[0] << std::endl;
        return;
    }

    it->second(connfd, args);
}

int main() {
    // TODO change to configuration file
    int port = 1234;
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
        // TODO switch to std::thread
        if (fork() == 0) {
            close(fd);  //child does not need listener
            handle_request(connfd);
            close (connfd);
            exit(0);    // kill the process. child process all done with work
        }

        close(connfd);
    }
}