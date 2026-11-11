#ifndef CONFIG_H
#define CONFIG_H

#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "utils.hpp"

struct peer_config {
    // clientThreadConfig.cfg
    int tracker_port;
    std::string tracker_ip;
    int update_interval;

    // serverThreadConfig.cfg
    int listen_port;
    std::string shared_dir;
};

// Reads all non-empty lines of a config file. Returns false if it cannot be opened
inline bool read_config_lines(const std::string &path, std::vector<std::string> &lines) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "Failed to open config file: " << path << std::endl;
        return false;
    }

    std::string line;
    while (std::getline(file, line)) {
        line = trim(line);
        if (!line.empty()) {
            lines.push_back(line);
        }
    }
    return true;
}

// Loads both config files into cfg. Returns false (after printing why) on any problem
inline bool load_peer_config(const std::string &client_path, const std::string &server_path, peer_config &cfg) {
    std::vector<std::string> client_lines, server_lines;
    if (!read_config_lines(client_path, client_lines) || !read_config_lines(server_path, server_lines)) {
        return false;
    }
    if (client_lines.size() < 3) {
        std::cerr << client_path << " needs: tracker port, tracker IP, update interval" << std::endl;
        return false;
    }
    if (server_lines.size() < 2) {
        std::cerr << server_path << " needs: listen port, shared folder" << std::endl;
        return false;
    }

    try {
        cfg.tracker_port = std::stoi(client_lines[0]);
        cfg.tracker_ip = client_lines[1];
        cfg.update_interval = std::stoi(client_lines.back());
        cfg.listen_port = std::stoi(server_lines[0]);
    } catch (...) {
        std::cerr << "Invalid number in config files" << std::endl;
        return false;
    }
    cfg.shared_dir = server_lines.back();
    return true;
}

#endif
