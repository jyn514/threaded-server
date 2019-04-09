/* Copyright 2019 Joshua Nelson
 * This program is licensed under the BSD 3-Clause License.
 * See LICENSE.txt or https://opensource.org/licenses/BSD-3-Clause for details.
 */
#include <stdbool.h>
#ifndef RESPONSE_H
#define RESPONSE_H
struct response {
  char *status, *headers;
  char *body;  // NOT a string, may not be null terminated
  int length;
  bool is_mmapped;
  bool persist_connection;
};
struct response handle_request(const char *);
#endif  // RESPONSE_H
