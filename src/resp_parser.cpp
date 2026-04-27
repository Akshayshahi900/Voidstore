#include "resp_parser.h"

bool parseOneCommand(std::string &buffer, std::vector<std::string> &args)
{
  args.clear();

  if (buffer.empty())
    return false;

  size_t pos = 0;

  // Skip bare CRLFs / LFs between commands
  while (pos < buffer.size() && (buffer[pos] == '\r' || buffer[pos] == '\n'))
    pos++;

  if (pos >= buffer.size())
    return false;

  // ── RESP array: *<count>\r\n ... ──────────────────────────────────────
  if (buffer[pos] == '*')
  {
    size_t line_end = buffer.find("\r\n", pos);
    if (line_end == std::string::npos)
      return false;

    int count = std::stoi(buffer.substr(pos + 1, line_end - pos - 1));
    pos = line_end + 2;

    for (int i = 0; i < count; i++)
    {
      if (pos >= buffer.size() || buffer[pos] != '$')
        return false;

      size_t len_end = buffer.find("\r\n", pos);
      if (len_end == std::string::npos)
        return false;

      size_t len = static_cast<size_t>(std::stoi(buffer.substr(pos + 1, len_end - pos - 1)));
      pos = len_end + 2;

      if (pos + len + 2 > buffer.size())
        return false; // partial frame

      args.push_back(buffer.substr(pos, len));
      pos += len + 2;
    }

    buffer.erase(0, pos);
    return true;
  }

  // ── Inline command: "CMD arg1 arg2\n" or "CMD arg1 arg2\r\n" ──────────
  // redis-cli --pipe sends stdin as-is, which uses \n line endings.
  size_t line_end = buffer.find('\n', pos);
  if (line_end == std::string::npos)
    return false; // incomplete line

  // Extract line, stripping trailing \r if present
  size_t line_len = line_end - pos;
  if (line_len > 0 && buffer[pos + line_len - 1] == '\r')
    line_len--;

  std::string line = buffer.substr(pos, line_len);
  buffer.erase(0, line_end + 1);

  // Split on spaces
  size_t p = 0;
  while (p < line.size())
  {
    while (p < line.size() && line[p] == ' ')
      p++; // skip spaces
    if (p >= line.size())
      break;
    size_t sp = line.find(' ', p);
    if (sp == std::string::npos)
    {
      args.push_back(line.substr(p));
      break;
    }
    args.push_back(line.substr(p, sp - p));
    p = sp + 1;
  }

  return !args.empty();
}
