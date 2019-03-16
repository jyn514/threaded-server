#include <iostream>
#include <sstream>
#include <vector>
#include <string>
#include <map>
#include <fstream>

#include "response.h"
#include "parse.h"

using std::map;
using std::string;

extern string current_dir;

static const string invalid_request =
"HTTP/1.1 400 Bad Request\r\n"
"Content-Type: text/html; charset=utf-8\r\n"
"<!doctype html>\r\n"
"<html><head>\r\n"
"<title>400 Bad Request</title>\r\n"
"</head><body>\r\n"
"<h1>Bad Request</h1>"
"<p>Your browser sent a request that this server could not understand.<br />\r\n"
"</p>\r\n"
"</body></html>\r\n",
                    empty_response =
"HTTP/1.1 204 No Content\r\n"
"Content-Type: text/html; charset=utf-8\r\n\r\n",
                    not_found =
"HTTP/1.1 404 Not Found\r\n\r\n",
                    internal_error =
"HTTP/1.1 500 Internal Server Error\r\n\r\n";


string get_file(const char *const filename) {
  std::ifstream file(filename, std::ios::binary | std::ios::ate);
  std::streamsize size = file.tellg();
  file.seekg(0, std::ios::beg);

  std::vector<char> buffer(size);
  if (file.read(buffer.data(), size)) {
    string result(buffer.begin(), buffer.end());
    return result;
  }
  perror("Could not read file");
  return internal_error;
}

string handle_url(const struct request_info& info,
                  const map<string,string>& headers) {
  char *path = realpath((current_dir + info.url).c_str(), NULL);
  if (path == NULL)
    return not_found;
  string result = get_file(path);
  free(path);
  return result;
}

string handle_request(string& request) {
  // cerr is thread-safe PER CALL, so multiple << operators may be reordered
  std::cerr << request;
  struct request_info line = process_request_line(request);
  std::stringstream log;
  log << "Parsed url as " << line.url << '\n';
  std::cerr << log.str();
  if (line.method == ERROR) return invalid_request;
  map<string, string> headers = process_headers(request);
  return handle_url(line, headers);
}
