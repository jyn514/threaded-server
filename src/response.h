/* Copyright 2019 Joshua Nelson
 * This program is licensed under the BSD 3-Clause License.
 * See LICENSE.txt or https://opensource.org/licenses/BSD-3-Clause for details.
 */
#include <stdbool.h>
#ifndef RESPONSE_H
#define RESPONSE_H

#define MAX_DATE 60

struct response {
  char *status, *headers;
  char *body;  // NOT a string, may not be null terminated
  int length;
  bool is_mmapped;
  bool persist_connection;
};

enum response_code {
  OK = 200, NO_CONTENT = 204,
  BAD_REQUEST = 400, NOT_FOUND = 404, FORBIDDEN = 403,
  INTERNAL_ERROR = 500, NOT_IMPLEMENTED = 501, TRY_AGAIN = 503
};

struct response handle_request(char *);
#endif  // RESPONSE_H
