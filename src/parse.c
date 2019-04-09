/* Copyright 2019 Joshua Nelson
 * This program is licensed under the BSD 3-Clause License.
 * See LICENSE.txt or https://opensource.org/licenses/BSD-3-Clause for details.
 *
 * Parser. Parses HTTP method and headers and removes them from the request.
 */
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <magic.h>

#include <parse.h>
#include <dict.h>

extern DICT mimetypes;

const char *get_mimetype(const char *const filename) {
  char *ext = strchr(filename, '.'), *type;
  if (ext != NULL && (type = dict_get(mimetypes, ++ext)))
    return type;

  // do the expensive libmagic calls
  // initialize libmagic; it's not thread safe so we don't initialize in main
  magic_t cookie;
  if ((cookie = magic_open(MAGIC_SYMLINK |
          MAGIC_MIME_ENCODING | MAGIC_MIME_TYPE)) == NULL
      || magic_load(cookie, NULL) != 0) {
    puts(magic_error(cookie));
    exit(1);
  }

  char *result = strdup(magic_file(cookie, filename));
  magic_close(cookie);
  // assume this is the same for all extensions
  if (ext != NULL) dict_put(mimetypes, strdup(ext), result);
  return result;
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
    char *mimetype, *ext;
    if (line[0] != '#' && (n = sscanf(line, "%ms\t%ms\n", &mimetype, &ext)) == 2) {
        dict_put(result, ext, mimetype);
    } else if (n == 1) {
        // clang-tidy mistakenly thinks mimetype is uninitialized
        // it's written to by sscanf above
        free(mimetype);  // NOLINT
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
  char *method;
  int read, matched;
  matched = sscanf(request, "%ms %ms %ms\r\n%n",
      &method, &result->url, &result->version, &read);

  if (matched < 2) {
    result->method = ERROR;
    if (matched == 1) free(method);
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
  char *header, *body;
  int read, ret = 0;

  // for every line
  while ((sscanf(request, "%ms: %ms\r\n%n",
                           &header, &body, &read)) == 2) {
    dict_put(headers, header, body);
    ret += read;
  }
  return ret;
}
