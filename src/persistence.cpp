#include "persistence.h"
#include "storage.h"

#include <fstream>
#include <sstream>
#include <ctime>

std::ofstream aof("db.aof", std::ios::app);

void loadDatabase()
{
  std::ifstream file("db.aof");
  std::string line;

  while (std::getline(file, line))
  {
    std::istringstream iss(line);
    std::string command;

    iss >> command;

    if (command == "SET")
    {
      std::string key, value;
      time_t abs_expiry;

      iss >> key >> value;

      store[key] = value;

      // The AOF stores the absolute expiry timestamp (not a relative TTL)
      if (iss >> abs_expiry)
      {
        if (abs_expiry > time(NULL))
          expiry[key] = abs_expiry; // still valid
        else
          store.erase(key); // already expired, skip it
      }
    }
    else if (command == "DEL")
    {
      std::string key;
      iss >> key;
      store.erase(key);
    }
  }
}

void logSet(const std::string &key, const std::string &value, int ttl)
{
  aof << "SET " << key << " " << value;
  // Write the absolute expiry timestamp so reloads don't reset the clock
  if (ttl > 0)
    aof << " " << (time(NULL) + ttl);
  aof << std::endl;
  aof.flush();
}

void logDel(const std::string &key)
{
  aof << "DEL " << key << std::endl;
  aof.flush();
}
