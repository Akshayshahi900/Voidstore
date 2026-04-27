#include "resp_parser.h"
#include <sstream>
#include <iostream>
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
bool parseOneCommand(std::string &buffer, std::vector<std::string> &args)
{
  std::cout << "RAW BUFFER:\n"
            << buffer << std::endl;
  if (buffer.empty())
    return false;

  size_t pos = 0;
  while (pos < buffer.size() && (buffer[pos] == '\r' || buffer[pos] == '\n' || buffer[pos] == ' '))
  {
    pos++;
  }
  if (pos >= buffer.size() || buffer[pos] != '*')
  {
    std::cout << "Invalid start char:" << buffer[pos] << std::endl;
    return false;
  }

  size_t line_end = buffer.find("\r\n", pos);
  if (line_end == std::string::npos)
    return false;

  int count = std::stoi(buffer.substr(pos + 1, line_end - pos - 1));
  pos = line_end + 2;

  for (int i = 0; i < count; i++)
  {
    // except "$<len>\r\n"
    if (pos >= buffer.size() || buffer[pos] != '$')
      return false;

    size_t len_end = buffer.find("\r\n", pos);
    if (len_end == std::string::npos)
      return false;

    int len = std::stoi(buffer.substr(pos + 1, len_end - pos - 1));

    pos = len_end + 2;
    std::cout << "Buffer size: " << buffer.size() << " pos: " << pos << " len: " << len << std::endl;
    // check if "<data\r\n"
    if (pos + len + 2 > buffer.size())
      return false;

    args.push_back(buffer.substr(pos, len));

    pos += len + 2; // skip data + \r\n
  }

  // remove the parsed command from buffer
  buffer.erase(0, pos);
  return true;
}
