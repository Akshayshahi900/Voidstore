#include "resp_parser.h"
#include <sstream>

std::vector<std::string> parseRESP(const std::string &input)
{
  std::vector<std::string> args;
  std::istringstream stream(input);
  std::string line;

  std::getline(stream, line);

  if (!line.empty() && line.back() == '\r')
    line.pop_back();

  if (line.empty() || line[0] != '*')
    return args;

  int argCount = std::stoi(line.substr(1));

  for (int i = 0; i < argCount; i++)
  {
    std::getline(stream, line);
    std::getline(stream, line);

    if (!line.empty() && line.back() == '\r')
      line.pop_back();

    args.push_back(line);
  }

  return args;
}
