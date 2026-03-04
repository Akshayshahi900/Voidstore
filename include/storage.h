#pragma once
#include <unordered_map>
#include <string>
#include <ctime>

extern std::unordered_map<std::string, std::string> store;
extern std::unordered_map<std::string, std::time_t> expiry;
