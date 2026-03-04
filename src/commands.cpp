#include "commands.h"
#include "storage.h"
#include "persistence.h"

#include <ctime>

std::string handleCommand(const std::vector<std::string> &args)
{
  if (args.empty())
    return "-ERR invalid command\r\n";

  std::string command = args[0];

  if (command == "SET")
  {
    if (args.size() < 3)
      return "-ERR wrong number of arguments\r\n";

    std::string key = args[1];
    std::string value = args[2];
    int ttl = -1;

    if (args.size() == 4)
      ttl = std::stoi(args[3]);

    if (ttl > 0)
      expiry[key] = time(NULL) + ttl;

    store[key] = value;

    logSet(key, value, ttl);

    return "+OK\r\n";
  }

  else if (command == "GET")
  {
    if (args.size() != 2)
      return "-ERR wrong number of arguments\r\n";

    std::string key = args[1];

    if (expiry.find(key) != expiry.end())
    {
      if (time(NULL) > expiry[key])
      {
        store.erase(key);
        expiry.erase(key);
        return "$-1\r\n";
      }
    }

    if (store.find(key) == store.end())
      return "$-1\r\n";

    std::string value = store[key];

    return "$" + std::to_string(value.size()) + "\r\n" + value + "\r\n";
  }

  else if (command == "DEL")
  {
    if (args.size() != 2)
      return "-ERR wrong number of arguments\r\n";

    std::string key = args[1];

    store.erase(key);
    expiry.erase(key);

    logDel(key);

    return ":1\r\n";
  }

  return "-ERR unknown command\r\n";
}
