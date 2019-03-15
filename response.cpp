#include <string>
#include <map>

#include "response.h"
#include "parse.h"

using std::map;
using std::string;

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
"Content-Type: text/html; charset=utf-8\r\n\r\n";

string handle_url(struct request_info info, map<string,string> headers) {
  return empty_response;
}

string handle_request(string& request) {
  struct request_info line = process_request_line(request);
  if (line.method == ERROR) return invalid_request;
  map<string, string> headers = process_headers(request);
  return handle_url(line, headers);
}
