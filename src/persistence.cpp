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
      int ttl;

      iss >> key >> value;

      store[key] = value;

      if (iss >> ttl)
        expiry[key] = time(NULL) + ttl;
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
  if (ttl > 0)
    aof << " " << ttl;
  aof << std::endl;
  aof.flush();
}

void logDel(const std::string &key)
{
  aof << "DEL " << key << std::endl;
  aof.flush();
}
