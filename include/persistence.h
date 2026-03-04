#pragma once
#include <string>
void loadDatabase();
void logSet(const std::string &key, const std::string &value, int ttl);
void logDel(const std::string &key);
