#include <string>
#include <vector>
struct response {
  std::string status;
  std::string headers;
  std::vector<char> *body; // handle arbitrary binary
};
struct response handle_request(std::string&);
