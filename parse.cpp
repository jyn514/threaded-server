#include "parse.h"

using std::string;
using std::map;

static const string line_delim = "\r\n";

struct request_info process_request_line(string& request) {
  size_t line_end = request.find(line_delim), space_end;
  struct request_info result = {GET, ""};

  if (line_end == string::npos) {
    struct request_info error = {ERROR, ""};
    return error;
  }
  string request_line = request.substr(0, line_end);
  space_end = request_line.find(' ');
  if (request_line.substr(0, space_end) != "GET") {
    struct request_info error = {ERROR, "Unrecognized method"};
    return error;
  }

  result.url = request.substr(space_end + 1, line_end - space_end - 1);
  request.erase(0, line_end + 2);
  return result;
}

map<string, string> process_headers(string& request) {
  map<string, string> headers;
  static const string header_delim = line_delim + line_delim;
  size_t start = 0, name_end, stop;
  string line, header, body;

  // for every line
  while ((stop = request.find(line_delim, start)) != std::string::npos) {
    // substr takes LENGTH as second argument, not ending position
    line = request.substr(start, stop - start);
    // find where header name stops
    name_end = line.find(':');
    header = request.substr(start, name_end);

    // discard colon and whitespace
    while (isspace(line.at(++name_end)));
    body = line.substr(name_end, stop);

    // replaces existing entries
    headers[header] = body;
    start = stop + 2;  // skip over \r\n
  }
  request.erase(0, stop);
  return headers;
}
