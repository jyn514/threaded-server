#include <fstream>
#include "lib/magic.h"
#include "parse.h"

using std::string;
using std::map;

map<string, string> get_all_mimetypes(void);

static const string line_delim = "\r\n";
static map<string, string> mimetypes = get_all_mimetypes();

string& get_mimetype(const char *const filename) {
  string name(filename), ext;
  size_t ext_start = name.rfind('.') + 1;
  ext = name.substr(ext_start, string::npos);
  auto type = mimetypes.find(ext);
  if (type != mimetypes.end()) return type->second;

  // do the expensive libmagic calls
  // initialize libmagic; it's not thread safe so we don't initialize in main
  magic_t cookie;
  if ((cookie = magic_open(MAGIC_SYMLINK |
          MAGIC_MIME_ENCODING | MAGIC_MIME_TYPE)) == NULL
      || magic_load(cookie, NULL) != 0) {
    puts(magic_error(cookie));
    exit(1);
  }

  string result = magic_file(cookie, filename);
  // assume this never changes while the server is running
  mimetypes[ext] = result;
  magic_close(cookie);
  return mimetypes[ext];
}

map<string, string> get_all_mimetypes(void) {
  map<string, string> result;
  std::ifstream mime_database("mime.types");
  if (!mime_database) // file not found or IO error
  mime_database.open("/etc/mime.types");
  if (!mime_database)
    // empty map, we'll look files up with libmagic at runtime
    return result;
  string line;
  while (std::getline(mime_database, line)) {
    size_t type_end, ext_start;
    if ((line.size() && line.at(0) == '#')
        || (type_end = line.find('\t')) == string::npos) continue;
    ext_start = type_end;

    // discard whitespace
    while (isspace(line.at(++ext_start)));
    result[line.substr(ext_start, line.size() - type_end - 1)] = line.substr(0, type_end);
  }
  return result;
}

struct request_info process_request_line(string& request) {
  size_t line_end = request.find(line_delim), method_end, url_end;
  struct request_info result = {GET, "", "HTTP/1.0"};

  if (line_end == string::npos) {
    result.method = ERROR;
    return result;
  }
  string request_line = request.substr(0, line_end);
  /* parse METHOD */
  method_end = request_line.find(' ');
  string method = request_line.substr(0, method_end);
  if (method == "HEAD") {
    // do everything exactly the same as a GET, but main will not send the data
    // this catches access errors to files
    result.method = HEAD;
  } else if (method != "GET") {
    result.method = NOT_RECOGNIZED;
    return result;
  }

  /* parse url */
  url_end = request_line.find(' ', method_end + 1);
  if (url_end == string::npos) {  // no http.version
    result.url = request_line.substr(method_end + 1);
  } else {
    result.url = request.substr(method_end + 1, url_end - method_end - 1);
    result.version = request_line.substr(url_end + 1, string::npos);
  }
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
    if (line == "") break; // end of headers
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
