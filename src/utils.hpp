#ifndef UTILS_H
#define UTILS_H

#include <chrono>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>
#include <openssl/md5.h>

std::vector<std::string> split_string(const std::string& str, char delimiter);

std::string trim(const std::string& str);

std::string md5(const std::string& str);

std::string strip_brackets(std::string s);

long long current_timestamp();

#endif