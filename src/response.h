/* Copyright 2019 Joshua Nelson
 * This program is licensed under the BSD 3-Clause License.
 * See LICENSE.txt or https://opensource.org/licenses/BSD-3-Clause for details.
 */
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
