/* Copyright 2019 Joshua Nelson
 * This program is licensed under the BSD 3-Clause License.
 * See LICENSE.txt or https://opensource.org/licenses/BSD-3-Clause for details.
 *
 * Parser. Parses HTTP method and headers and removes them from the request.
 */
#define _POSIX_C_SOURCE 200809L

#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "parse.h"
#include "dict.h"

extern DICT mimetypes;

#define str_impl__(x) # x
#define str(x) str_impl__(x)

const char *get_mimetype(const char *const filename) {
  char *ext = strchr(filename, '.'), *type;
  if (ext != NULL && (type = dict_get(mimetypes, ++ext)))
    return type;
  return NULL;
}

DICT get_all_mimetypes(void) {
  DICT result = dict_init();
  FILE *mime_database;
  if ((mime_database = fopen("mime.types", "r")) == NULL
      && (mime_database = fopen("/etc/mime.types", "r")) == NULL)
      // empty map, we'll look files up with libmagic at runtime
      return result;
  char *line = NULL;
  size_t n = 0;
  while (getline(&line, &n, mime_database) > 0 && line != NULL) {
    if (line[0] != '#') {
        char *mimetype = malloc(MAX_MIMETYPE + 1), *ext = malloc(MAX_EXT + 1);
        if ((n = sscanf(line, "%" str(MAX_MIMETYPE) "s\t"
                     "%" str(MAX_EXT) "s\n", mimetype, ext)) == 2) {
            dict_put(result, ext, mimetype);
        } else {
            free(mimetype);
            free(ext);
        }
    }
    free(line);
    line = NULL;
    n = 0;
  }
  free(line);
  fclose(mime_database);
  return result;
}

int process_request_line(const char *const request, struct request_info *result) {
  char *method = malloc(MAX_METHOD + 1);
  int read, matched;
  result->version = malloc(MAX_VERSION + 1);
  result->url = malloc(MAX_URL + 1);
  matched = sscanf(request, "%" str(MAX_METHOD) "s %"
                   str(MAX_URL) "s %" str(MAX_VERSION) "s\r\n%n",
                   method, result->url, result->version, &read);

  if (matched < 2) {
    result->method = ERROR;
    free(method);
    return read;
  } else if (matched == 2) {  // no version sent
    result->version = "HTTP/1.0";
  }

  if (strcmp(method, "HEAD") == 0) {
    // do everything exactly the same as a GET, but main will not send the data
    // this catches access errors to files
    result->method = HEAD;
  } else if (strcmp(method, "GET") == 0) {
    result->method = GET;
  } else {
    result->method = NOT_RECOGNIZED;
  }

  free(method);
  return read;
}

int process_headers(const char *const request, DICT headers) {
  char header[MAX_HEADER+1], body[MAX_HEADER_BODY+1];
  int read, ret = 0;

  // for every line
  while ((sscanf(request, "%" str(MAX_HEADER) "s: %" str(MAX_HEADER_BODY)
                  "s\r\n%n", header, body, &read)) == 2) {
    dict_put(headers, header, body);
    ret += read;
  }
  return ret;
}
