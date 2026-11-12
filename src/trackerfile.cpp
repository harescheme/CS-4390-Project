#include "trackerfile.hpp"

const std::set<std::string> valid_keys {"Filename", "Filesize", "Description", "MD5"};

fs::path get_tracker_file_path(const std::string& filename) {
    return tracker_dir / (filename + ".track");
}

// Reads a tracker file completely and populates the tracker_file struct. Returns true on success, false on failure.
bool read_tracker_file(const std::string& filename, tracker_file& tfile) {
    fs::path filepath = get_tracker_file_path(filename);
    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "Failed to open tracker file: " << filename << std::endl;
        return false;
    }

    std::map<std::string, std::string> metadata;
    std::string line;
    while (std::getline(file, line)) {
        if (line[0] == '#') {
            continue;   // skip comments
        }

        size_t colon_pos = line.find(':');
        if (colon_pos == std::string::npos) {
            std::cerr << "Invalid line in tracker file: " << line << std::endl;
            continue;
        }

        // Key-value pairs must be in valid_key, otherwise treated as peer
        std::string key = line.substr(0, colon_pos);
        std::string value = trim(line.substr(colon_pos + 1));
        if (valid_keys.find(key) == valid_keys.end()) {
            // Try to parse as a peer line
            peer_tracker_file peer;
            if (parse_peer_line(line, peer)) {
                tfile.peers.push_back(peer);
            }

            continue;
        }
        if (metadata.find(key) != metadata.end()) {
            std::cerr << "Duplicate key in tracker file: " << key << std::endl;
            continue;
        }

        metadata[key] = value;
    }

    for (const std::string& key : valid_keys) {
        if (metadata.find(key) == metadata.end()) {
            std::cerr << "Missing key in tracker file: " << key << std::endl;
            return false;
        }
    }

    tfile.filename = metadata["Filename"];

    size_t index = 0;
    tfile.filesize = std::stoul(metadata["Filesize"], &index);
    if (index != metadata["Filesize"].length()) {
        std::cerr << "Invalid filesize in tracker file: " << metadata["Filesize"] << std::endl;
        return false;
    }

    tfile.description = metadata["Description"];

    if (metadata["MD5"].length() != 32 || !std::all_of(metadata["MD5"].begin(), metadata["MD5"].end(), ::isxdigit)) {
        std::cerr << "Invalid MD5 hash in tracker file: " << metadata["MD5"] << std::endl;
        return false;
    }
    // TODO change md5 from strings to byte array
    tfile.md5 = metadata["MD5"];

    return true;
}

// Parses a peer line in the tracker file
bool parse_peer_line(const std::string& line, peer_tracker_file& peer) {
    std::vector<std::string> tokens = split_string(line, ':');
    if (tokens.size() != 5) {
        std::cerr << "Invalid peer line: " << line << std::endl;
        return false;
    }

    if (!validate_ip_address(tokens[0], peer.ip_address)) {
        std::cerr << "Invalid IP address in peer line: " << tokens[0] << std::endl;
        return false;
    }

    if (!validate_port(tokens[1], peer.port)) {
        std::cerr << "Invalid port in peer line: " << tokens[1] << std::endl;
        return false;
    }

    if (!validate_start_end_bytes(tokens[2], tokens[3], peer.start_byte, peer.end_byte)) {
        std::cerr << "Invalid start or end byte in peer line: " << tokens[2] << ", " << tokens[3] << std::endl;
        return false;
    }

    if (!validate_timestamp(tokens[4], peer.timestamp)) {
        std::cerr << "Invalid timestamp in peer line: " << tokens[4] << std::endl;
        return false;
    }

    return true;
}

// Converts a tracker file struct to a string representation
std::string tracker_file_to_string(const tracker_file& tfile) {
    std::stringstream ss;
    ss << "Filename: " << tfile.filename << '\n';
    ss << "Filesize: " << tfile.filesize << '\n';
    ss << "Description: " << tfile.description << '\n';
    ss << "MD5: " << tfile.md5 << '\n';

    for (const auto& peer : tfile.peers) {
        char ip_str[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &peer.ip_address, ip_str, sizeof(ip_str));
        ss << ip_str << ":" << peer.port << ":" << peer.start_byte
           << ":" << peer.end_byte << ":" << peer.timestamp << "\n";
    }

    return ss.str();
}

// Writes a tracker file struct to a file using its string representation
bool write_tracker_file(const std::string& filename, const tracker_file& tfile) {
    fs::path filepath = get_tracker_file_path(filename);
    std::ofstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "Failed to open tracker file for writing: " << filename << std::endl;
        return false;
    }

    file << tracker_file_to_string(tfile);

    return true;
}

// Lists all tracker files available on the server. Could consider caching in memory, but this project is simple enough.
std::vector<tracker_file> list_tracker_files() {
    std::vector<tracker_file> files;
    for (const auto& entry : fs::directory_iterator(tracker_dir)) {
        if (entry.is_regular_file() && entry.path().extension() == ".track") {
            tracker_file tfile;
            if (read_tracker_file(entry.path().stem().string(), tfile)) {
                files.push_back(tfile);
            }
        }
    }
    return files;
}


// Validation helper functions

bool validate_filename(const std::string& filename) {
    return !filename.empty() && filename.find_first_of("/\\") == std::string::npos;
}

bool validate_filesize(const std::string& filesize_str, size_t& filesize) {
    try {
        size_t index = 0;
        filesize = std::stoul(filesize_str, &index);
        return index == filesize_str.length();
    } catch (...) {
        return false;
    }
}

bool validate_md5(const std::string& md5) {
    return md5.length() == 32 && std::all_of(md5.begin(), md5.end(), ::isxdigit);
}

bool validate_ip_address(const std::string& ip_address_str, struct in_addr& ip_address) {
    return inet_pton(AF_INET, ip_address_str.c_str(), &ip_address) == 1;
}

bool validate_port(const std::string& port_str, short& port) {
    try {
        size_t index = 0;
        port = static_cast<short>(std::stoul(port_str, &index));
        return index == port_str.length() && port >= 0 && port <= 65535;
    } catch (...) {
        return false;
    }
}

bool validate_start_end_bytes(const std::string& start_str, const std::string& end_str, size_t& start_byte, size_t& end_byte) {
    if (!validate_filesize(start_str, start_byte)) {
        return false;
    }
    if (!validate_filesize(end_str, end_byte)) {
        return false;
    }
    return start_byte <= end_byte;
}

bool validate_timestamp(const std::string& timestamp_str, size_t& timestamp) {
    try {
        size_t index = 0;
        timestamp = std::stoul(timestamp_str, &index);
        return index == timestamp_str.length();
    } catch (...) {
        return false;
    }
}
