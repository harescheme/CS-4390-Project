#ifndef TRACKER_FILE_H
#define TRACKER_FILE_H

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <vector>

#include <arpa/inet.h>

#include "utils.hpp"

namespace fs = std::filesystem;

struct peer_tracker_file {
    struct in_addr ip_address;
    short port;
    size_t start_byte, end_byte;
    size_t timestamp;
};

struct tracker_file {
    std::string filename;
    size_t filesize;
    std::string description;
    std::string md5;
    std::vector<peer_tracker_file> peers;
};

bool read_tracker_file(const std::string& filename, tracker_file& tfile);
bool parse_peer_line(const std::string& line, peer_tracker_file& peer);

std::string tracker_file_to_string(const tracker_file& tfile);

bool write_tracker_file(const std::string& filename, const tracker_file& tfile);

std::vector<tracker_file> list_tracker_files();

#endif