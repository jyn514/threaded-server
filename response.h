#include <string>
#include <vector>
struct response {
  std::string status;
  std::string headers;
  char *body; // NOT a string, may not be null terminated
  int length;
  bool is_mmapped;
  bool persist_connection;
};
struct response handle_request(std::string&);
