/* Copyright 2019 Joshua Nelson
 * This program is licensed under the BSD 3-Clause License.
 * See LICENSE.txt or https://opensource.org/licenses/BSD-3-Clause for details.
 */
#ifndef PARSE_H
#define PARSE_H
#include "dict.h"

#define MAX_MIMETYPE 1000
#define MAX_EXT 100
#define MAX_METHOD 50
#define MAX_URL 8048
#define MAX_VERSION 25
#define MAX_HEADER 100
#define MAX_HEADER_BODY 4096

enum method {
  GET, HEAD, NOT_RECOGNIZED, ERROR
};

struct request_info {
  enum method method;
  char *url;
  char *version;
};

DICT get_all_mimetypes(void);
int process_request_line(const char*, struct request_info *);
int process_headers(const char*, DICT headers);
const char *get_mimetype(const char *);
#endif  // PARSE_H
