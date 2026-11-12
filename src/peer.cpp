#include <iostream>
#include <fstream>
#include <filesystem>
#include <thread>

#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include "socket_helper.hpp"
#include "config.hpp"

namespace fs = std::filesystem;

static peer_config config;
static const fs::path cache_dir = "cache";
constexpr size_t MAX_CHUNK_SIZE = 1024;

// Downloads a file given the tracker file.
void download_file(int connfd /*, some other parameters */) {
    // TODO implement, set parameters
}
// -- server --

static void send_peer_reply(int connfd, const std::string &message) {
    socket_writer writer(connfd);
    writer << "<" + message + ">" << '\n';
    if (writer.flush()) {
        std::cerr << "Failed to send response" << std::endl;
    }
}

void peer_get_handler(int connfd, const std::vector<std::string> &args) {
    if (args.size() != 4) {
        std::cerr << "Invalid number of arguments for peer GET" << std::endl;
        send_peer_reply(connfd, "GET fail");
        return;
    }

    size_t start_byte, end_byte;
    try {
        start_byte = std::stoul(args[2]);
        end_byte = std::stoul(args[3]);
    } catch (...) {
        send_peer_reply(connfd, "GET fail");
        return;
    }
    if (end_byte <= start_byte) {
        send_peer_reply(connfd, "GET fail");
        return;
    }

    if (end_byte - start_byte > MAX_CHUNK_SIZE) {
        std::cerr << "Rejected GET for " << (end_byte - start_byte) << " bytes (limit " << MAX_CHUNK_SIZE << ")" <<
                std::endl;
        send_peer_reply(connfd, "GET invalid");
        return;
    }

    fs::path filepath = fs::path(config.shared_dir) / fs::path(args[1]).filename();
    std::error_code ec;
    size_t filesize = fs::file_size(filepath, ec);
    std::ifstream file(filepath, std::ios::binary);
    if (ec || !file.is_open() || end_byte > filesize) {
        send_peer_reply(connfd, "GET ferr");
        return;
    }

    std::string data(end_byte - start_byte, '\0');
    file.seekg(start_byte);
    file.read(&data[0], data.size());
    if (static_cast<size_t>(file.gcount()) != data.size()) {
        send_peer_reply(connfd, "GET fail");
        return;
    }

    socket_writer writer(connfd);
    writer << "<GET ok " << data.size() << ">\n" << data;
    if (writer.flush()) {
        std::cerr << "Failed to send response" << std::endl;
        return;
    }
    std::cout << "Served " << args[1] << " bytes " << start_byte << "-" << end_byte << std::endl;
}

static void handle_peer_request(int connfd) {
    socket_reader reader(connfd);
    std::string line;
    if (reader.read_line(line) < 0) {
        std::cerr << "Failed to read peer request" << std::endl;
        return;
    }

    std::vector<std::string> args = split_string(strip_brackets(line), ' ');
    if (args.empty()) {
        std::cerr << "Empty request received" << std::endl;
        return;
    }

    if (args[0] == "GET") {
        peer_get_handler(connfd, args);
    } else {
        std::cerr << "Unknown request: " << args[0] << std::endl;
    }
}

// Listens for other peers; each connection is handled by its own thread
static void peer_server_loop() {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        std::cerr << "socket() failed" << std::endl;
        return;
    }

    int opt = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(config.listen_port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(fd, (const sockaddr *) &addr, sizeof(addr))) {
        std::cerr << "bind() failed on port " << config.listen_port << std::endl;
        return;
    }
    if (listen(fd, SOMAXCONN)) {
        std::cerr << "listen() failed" << std::endl;
        return;
    }
    std::cout << "Peer server listening on port " << config.listen_port << std::endl;

    while (true) {
        int connfd = accept(fd, nullptr, nullptr);
        if (connfd < 0) {
            std::cerr << "Error accepting connection" << std::endl;
            continue;
        }

        std::thread([connfd]() {
            handle_peer_request(connfd);
            close(connfd);
        }).detach();
    }
}

// -- client --

// Connects to the tracker and sends one request line. Returns the socket, or -1 on error
static int send_to_tracker(const std::string &request) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        std::cerr << "socket() failed" << std::endl;
        return -1;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(config.tracker_port);
    if (!inet_aton(config.tracker_ip.c_str(), &addr.sin_addr)) {
        std::cerr << "Invalid IP address given" << std::endl;
        close(fd);
        return -1;
    }

    if (connect(fd, (const sockaddr *) &addr, sizeof(addr))) {
        std::cerr << "connect() to tracker " << config.tracker_ip << ":" << config.tracker_port << " failed" <<
                std::endl;
        close(fd);
        return -1;
    }

    socket_writer writer(fd);
    writer << "<" + request + ">" << '\n';
    if (writer.flush()) {
        std::cerr << "Failed to send request" << std::endl;
        close(fd);
        return -1;
    }
    return fd;
}
static int send_to_peers(const std::string &request, const std::string &serverIP,const std::string &serverPort) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        std::cerr << "socket() failed" << std::endl;
        return -1;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(serverPort); // change this
    if (!inet_aton(serverIP, &addr.sin_addr)) {
        std::cerr << "Invalid IP address given" << std::endl;
        close(fd);
        return -1;
    }

    if (connect(fd, (const sockaddr *) &addr, sizeof(addr))) {
        std::cerr << "connect() to peer " << serverIP << ":" << serverPort << " failed" <<
                std::endl;
        close(fd);
        return -1;
    }

    socket_writer writer(fd);
    writer << "<" + request + ">" << '\n';
    if (writer.flush()) {
        std::cerr << "Failed to send request" << std::endl;
        close(fd);
        return -1;
    }
    return fd;
}

// commands

// createtracker / updatetracker
void simple_command(const std::string &line, const std::vector<std::string> &args) {
    int fd = send_to_tracker(line);
    if (fd < 0) return;

    socket_reader reader(fd);
    std::string reply;
    if (reader.read_line(reply) < 0) {
        std::cerr << "No reply from tracker" << std::endl;
    } else {
        std::cout << strip_brackets(reply) << std::endl;
    }
    close(fd);
}

// REQ LIST
void list_command(const std::string &line, const std::vector<std::string> &args) {
    if (args.size() < 2 || args[1] != "LIST") {
        std::cerr << "Usage: REQ LIST" << std::endl;
        return;
    }
    int fd = send_to_tracker(line);
    if (fd < 0) return;

    socket_reader reader(fd);
    while (true) {
        std::string reply;
        if (reader.read_line(reply) < 0) {
            std::cerr << "Incomplete reply from tracker" << std::endl;
            break;
        }
        reply = strip_brackets(reply);
        std::cout << reply << std::endl;
        if (reply == "REP LIST END") break;
    }
    close(fd);
}

// GET filename.track
void get_command(const std::string &line, const std::vector<std::string> &args) {
    if (args.size() < 2) {
        std::cerr << "Usage: GET filename.track" << std::endl;
        return;
    }
    int fd = send_to_tracker(line);
    if (fd < 0) return;

    socket_reader reader(fd);
    std::string reply;
    if (reader.read_line(reply) < 0 || strip_brackets(reply) != "REP GET BEGIN") {
        std::cerr << "GET failed: tracker did not send the file (" << strip_brackets(reply) << ")" << std::endl;
        close(fd);
        return;
    }

    const std::string end_prefix = "REP GET END ";
    std::string content, expected_md5;
    bool complete = false;
    while (true) {
        std::string next;
        if (reader.read_line(next) < 0) break;

        std::string stripped = strip_brackets(next);
        if (stripped.compare(0, end_prefix.size(), end_prefix) == 0) {
            expected_md5 = trim(stripped.substr(end_prefix.size()));
            complete = true;
            break;
        }
        content += next;
    }
    close(fd);

    if (!complete) {
        std::cerr << "GET failed: incomplete reply from tracker" << std::endl;
        return;
    }
    if (md5(content) != expected_md5) {
        std::cerr << "GET failed: MD5 mismatch, tracker file discarded" << std::endl;
        return;
    }

    fs::path cache_path = cache_dir / fs::path(args[1]).filename();
    std::ofstream out(cache_path);
    if (!out.is_open() || !(out << content)) {
        std::cerr << "Failed to save tracker file to " << cache_path << std::endl;
        return;
    }
    std::cout << "Tracker file " << args[1] << " received, MD5 OK, saved to " << cache_path << std::endl;
}

int main() {
    if (!load_peer_config("clientThreadConfig.cfg", "serverThreadConfig.cfg", config)) {
        return 1;
    }
    fs::create_directories(config.shared_dir);
    fs::create_directories(cache_dir);

    std::thread(peer_server_loop).detach();

    std::cout << "Commands: REQ LIST | GET <file>.track | createtracker ... | updatetracker ... | quit" << std::endl;
    std::string line;
    while (std::cout << "> " && std::getline(std::cin, line)) {
        line = trim(line);
        if (line.empty()) continue;
        if (line == "quit") break;

        std::vector<std::string> args = split_string(line, ' ');
        std::string cmd = args[0];

        if (cmd == "createtracker" || cmd == "updatetracker") {
            std::cout << "Sent to tracker: " << line << std::endl;
            simple_command(line, args);
        } else if (cmd == "REQ") {
            std::cout << "Sent to tracker: " << line << std::endl;
            list_command(line, args);
        } else if (cmd == "GET") {
            std::cout << "Sent to tracker: " << line << std::endl;
            get_command(line, args);
        } else {
            std::cerr << "Unknown command: " << cmd << std::endl;
        }
    }
    return 0;
}
