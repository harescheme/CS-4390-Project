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

extern const std::set<std::string> valid_keys;
const fs::path tracker_dir = "tracker_files";

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

fs::path get_tracker_file_path(const std::string& filename);

bool read_tracker_file(const std::string& filename, tracker_file& tfile);
bool parse_peer_line(const std::string& line, peer_tracker_file& peer);

std::string tracker_file_to_string(const tracker_file& tfile);

bool write_tracker_file(const std::string& filename, const tracker_file& tfile);

std::vector<tracker_file> list_tracker_files();


// Input validation helper functions
// description can be any string, so no validation needed

bool validate_filename(const std::string& filename);
bool validate_filesize(const std::string& filesize_str, size_t& filesize);
bool validate_md5(const std::string& md5);

bool validate_ip_address(const std::string& ip_address_str, struct in_addr& ip_address);
bool validate_port(const std::string& port_str, short& port);
bool validate_start_end_bytes(const std::string& start_str, const std::string& end_str, size_t& start_byte, size_t& end_byte);
bool validate_timestamp(const std::string& timestamp_str, size_t& timestamp);

#endif