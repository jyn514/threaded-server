/* Copyright 2019 Joshua Nelson
 * This program is licensed under the BSD 3-Clause License.
 * See LICENSE.txt or https://opensource.org/licenses/BSD-3-Clause for details.
 */
#include <string>
#include <map>

enum method {
  GET, HEAD, NOT_RECOGNIZED, ERROR
};

struct request_info {
  enum method method;
  std::string url;
  std::string version;
};

struct request_info process_request_line(std::string&);
std::map<std::string, std::string> process_headers(std::string&);
std::string& get_mimetype(const char *const);
