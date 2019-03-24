#include <string>
#include <map>

enum method {
  GET, ERROR
};

struct request_info {
  enum method method;
  std::string url;
  std::string version;
};

struct request_info process_request_line(std::string&);
std::map<std::string, std::string> process_headers(std::string&);
std::string& get_mimetype(const char *const);
