/* Copyright 2019 Joshua Nelson
 * This program is licensed under the BSD 3-Clause License.
 * See LICENSE.txt or https://opensource.org/licenses/BSD-3-Clause for details.
 *
 * Parser. Parses HTTP method and headers and removes them from the request.
 */
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "lib/magic.h"
#include "parse.h"
#include "dict.h"
#include "utils.h"

static const char *line_delim = "\r\n";
extern DICT mimetypes;

const char *get_mimetype(const char *const filename) {
  int ext_start = strchr(filename, '.') - filename;
  const char *ext = &filename[ext_start],
        *type = dict_get(mimetypes, ext);
  if (type != NULL) return type;

  // do the expensive libmagic calls
  // initialize libmagic; it's not thread safe so we don't initialize in main
  magic_t cookie;
  if ((cookie = magic_open(MAGIC_SYMLINK |
          MAGIC_MIME_ENCODING | MAGIC_MIME_TYPE)) == NULL
      || magic_load(cookie, NULL) != 0) {
    puts(magic_error(cookie));
    exit(1);
  }

  const char *result = magic_file(cookie, filename);
  // assume this never changes while the server is running
  dict_put(mimetypes, strdup(ext), strdup(result));
  magic_close(cookie);
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
  while (getline(&line, 0, mime_database) && line != NULL) {
    char *type_end, *ext_start;
    if (line[0] == '#'
        || (type_end = strchr(line, '\t')) == NULL) continue;
    ext_start = type_end;

    // discard whitespace
    while (isspace(*++ext_start)) {}
    dict_put(result, substr(ext_start, strlen(ext_start) - (type_end-line) - 1),
        substr(line, type_end - line));
    free(line);
  }
  return result;
}

struct request_info process_request_line(const char **request) {
  char *line_end = strstr(*request, line_delim), *method_end, *url_end;
  struct request_info result = {GET, "", "HTTP/1.0"};

  if (line_end == NULL) {
    result.method = ERROR;
    return result;
  }
  char *request_line = substr(*request, line_end - *request);
  /* parse METHOD */
  if ((method_end = strchr(request_line, ' ')) == NULL) {
    result.method = ERROR;
    return result;
  }
  char *method = substr(request_line, method_end - request_line);
  if (strcmp(method, "HEAD") == 0) {
    // do everything exactly the same as a GET, but main will not send the data
    // this catches access errors to files
    result.method = HEAD;
  } else if (strcmp(method, "GET") != 0) {
    result.method = NOT_RECOGNIZED;
    return result;
  }

  /* parse url */
  url_end = strchr(method_end + 1, ' ');
  if (url_end == NULL) {  // no http.version
    result.url = substr(request_line, method_end - request_line + 1);
  } else {
    result.url = substr(method_end + 1, url_end-method_end + 1);
    result.version = url_end + 1;
  }
  *request = line_end + 2;
  return result;
}

DICT process_headers(const char **request) {
  DICT headers = dict_init();
  char *start = *request, *name_end, *stop;
  char *line, *header, *body;

  // for every line
  while ((stop = strstr(start, line_delim)) != NULL) {
    line = substr(start, stop - start);
    //line = request.substr(start, stop - start);
    if (strcmp(line, "") == 0) break;  // end of headers
    // find where header name stops
    name_end = strchr(line, ':');
    header = substr(line, name_end - line);

    // discard colon and whitespace
    while (isspace(*++name_end)) {}
    body = substr(name_end, stop - start);

    // replaces existing entries
    dict_put(headers, header, body);
    start = stop + 2;  // skip over \r\n
  }
  *request = stop;
  return headers;
}
