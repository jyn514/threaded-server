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
#include <stdbool.h>

#include "parse.h"
#include "dict.h"

extern DICT mimetypes;

#define str_impl__(x) # x
#define str(x) str_impl__(x)

const char *get_mimetype(const char *const filename) {
  char *ext = strrchr(filename, '.'), *type;
  if (ext != NULL && (type = dict_get(mimetypes, ++ext)))
    return type;
  return NULL;
}

// initialize a mime database into a DICT
DICT get_all_mimetypes(void) {
  DICT result = dict_init();
  FILE *mime_database;

  if ((mime_database = fopen("mime.types", "r")) == NULL
      && (mime_database = fopen("/etc/mime.types", "r")) == NULL)
      // empty database, we'll look files up with libmagic at runtime
      return result;

  char *line = NULL;
  size_t n = 0;
  while (getline(&line, &n, mime_database) > 0 && line != NULL) {
    char *mimetype, *ext;
    bool used_line = false;
    if (line[0] != '#' && (mimetype = strtok(line, " \t\r\n")) && (ext = strtok(NULL, " \t\r\n"))) {
        // there can be multiple extensions for a single mimetype
        // note: doesn't need to be thread-safe since only called at startup
        do {
            // don't replace existing extensions, dict_put will free the memory
            // and segfault
            if (dict_get(result, ext) == NULL) {
                dict_put(result, ext, mimetype);
                used_line = true;
            }
        } while ((ext = strtok(NULL, " \t\r\n")) != NULL);
    }
    if (!used_line) free(line);
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

int process_headers(const char *request, DICT headers) {
  char header[MAX_HEADER+1], body[MAX_HEADER_BODY+1];
  int read, ret = 0;

  // for every line
  while ((sscanf(request, "%" str(MAX_HEADER) "[^ \t\r\n:]: %"
                          str(MAX_HEADER_BODY) "s\r\n%n",
                 header, body, &read)) == 2) {
    dict_put(headers, strdup(header), strdup(body));
    ret += read;
    request += read;
  }
  return ret;
}
