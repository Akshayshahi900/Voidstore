#include "resp_parser.h"
#include <sstream>
#include <iostream>

bool parseOneCommand(std::string &buffer, std::vector<std::string> &args)
{

  args.clear();
  std::cout << "Buffer size: " << buffer.size() << std::endl;
  if (buffer.empty())
    return false;

  size_t pos = 0;
  // skip whitespace
  while (pos < buffer.size() && (buffer[pos] == '\r' || buffer[pos] == '\n' || buffer[pos] == ' '))
  {
    pos++;
  }
  // resync to next '*'
  if (pos >= buffer.size() || buffer[pos] != '*')
  {
    // size_t next = buffer.find('*');
    // if (next == std::string::npos)
    // {
    //   buffer.clear();
    //   return false;
    // }
    // if (next > 0)
    // {
    //   buffer.erase(0, next);
    // }
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
