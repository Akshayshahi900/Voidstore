#pragma once
#include <vector>
#include <string>

std::vector<std::string> parseRESP(const std::string &input);
bool parseOneCommand(std::string &buffer, std::vector<std::string> &args);
