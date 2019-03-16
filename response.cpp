#include <iostream>
#include <sstream>
#include <vector>
#include <string>
#include <map>
#include <fstream>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>

#include "response.h"
#include "parse.h"
#include "lib/magic.h"

using std::map;
using std::vector;
using std::string;

extern string current_dir;
extern magic_t cookie;

enum response_code {
  OK, BAD_REQUEST, NO_CONTENT, NOT_FOUND, INTERNAL_ERROR
};

struct internal_response {
  enum response_code code;
  map<string, string> headers;
  vector<char> *body;
};

static inline string make_date(time_t t);

static const char *index_page = "index.html",
                  *server_agent = "crappy_server/0.0.1",
                  *error_format = "<!doctype html>\r\n"
"<html><head>\r\n"
"<title>%s</title>\r\n"
"</head><body>\r\n"
"<h1>%s</h1>"
"<p>%s<br /></p>\r\n"
"</body></html>\r\n";

static inline const char *make_header(const enum response_code code) {
  switch(code) {
    case OK:
      return "200 OK";
    case BAD_REQUEST:
      return "400 Bad Request";
    case NO_CONTENT:
      return "204 No Content";
    case NOT_FOUND:
      return "404 Not Found";
    case INTERNAL_ERROR:
    default: // how did we get here?
      if (code != INTERNAL_ERROR)
        std::cerr << "Invalid response code\n";
      return "500 Internal Error";
  }
}

static inline string headers_to_string(const map<string, string>& headers) {
  string result;
  for (auto const &entry : headers) {
    result += entry.first + ": " + entry.second + "\r\n";
  }
  return result + "\r\n";
}

static inline const string make_header_line(const enum response_code code) {
  return string("HTTP/1.1 ") + make_header(code) + "\r\n";
}

static inline void add_default_headers(map<string,string>& headers) {
  headers["Date"] = make_date(time(NULL));
  headers["Server"] = server_agent;
}

static inline string make_date(const time_t t) {
  //  %a: 3, %d: 2, %b: 3, %Y: 4, %H: 2, %M: 2, %S: 2
  //  3 + 2 + 2 + 1 + 3 + 1 + 4 + 1 + 2 + 1 + 2 + 1 + 2 + 4 = 29
  //  lets double it, why not
  static const int len = 60;
  char date[len];
  struct tm *current_time = localtime(&t);
  if (current_time == NULL) {
    perror("Failed to get current time");
    return string();
  }
  strftime(date, len, "%a, %d %b %Y %H:%M:%S GMT", current_time);
  return string(date);
}

static void get_file(const char *const filename, struct internal_response& info) {
  std::ifstream file(filename, std::ios::binary | std::ios::ate);
  std::streamsize size = file.tellg();
  file.seekg(0, std::ios::beg);

  info.body = new vector<char>(size);
  if (file.read(&(*info.body)[0], size)) {
    info.code = OK;
    info.headers["Content-Length"] = std::to_string(size);
    info.headers["Content-Type"] = magic_file(cookie, filename);
    return;
  }
  perror("Could not read file");
  info.code = INTERNAL_ERROR;
}

static void handle_url(struct request_info& info,
                const map<string,string>& headers,
                struct internal_response& result) {
  struct stat stat_info;
  if (stat(info.url.c_str(), &stat_info) == 0 && S_ISDIR(stat_info.st_mode)) {
    if (info.url.back() != '/') info.url += '/';
    info.url += index_page;
  }
  char *path = realpath((current_dir + info.url).c_str(), NULL);
  if (path == NULL) {
    result.code = NOT_FOUND;
  // attempted path traversal attack
  } else if (current_dir.compare(0, string::npos, path, current_dir.size()) != 0) {
    std::stringstream log;
    log << "Preventing path traversal attack on " << path << '\n';
    std::cerr << log.str();
    free(path);
    result.code = NOT_FOUND;
  } else {
    get_file(path, result);
    result.headers["Last-Modified"] = make_date(stat_info.st_mtim.tv_sec);
    free(path);
  }
}

struct response handle_request(string& request) {
  // cerr is thread-safe PER CALL, so multiple << operators may be reordered
  std::cerr << request;
  struct request_info line = process_request_line(request);
  std::stringstream log;
  log << "Parsed url as " << line.url << '\n';
  std::cerr << log.str();

  struct internal_response result;
  if (line.method == ERROR) result.code = BAD_REQUEST;
  else {
      map<string, string> headers = process_headers(request);
      handle_url(line, headers, result);
  }
  if (result.code != OK) {
    const char *error = "An error occured while processing your request",
         *header = make_header(result.code);
    result.headers["Content-Type"] = "text/html; charset=utf-8";
    int length = snprintf(NULL, 0, error_format, header, header, error);
    // don't include null byte
    result.headers["Content-Length"] = std::to_string(length - 1);
    result.body = new vector<char>(length);
    snprintf(&(*result.body)[0], length, error_format, header, header, error);
  }
  add_default_headers(result.headers);
  return {
    make_header_line(result.code),
    headers_to_string(result.headers),
    result.body
  };
}
