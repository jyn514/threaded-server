/* Copyright 2019 Joshua Nelson
 * This program is licensed under the BSD 3-Clause License.
 * See LICENSE.txt or https://opensource.org/licenses/BSD-3-Clause for details.
 *
 * Responder. Given a request string,
 * performs all application logic to create a response.
 */
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <bsd/string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <errno.h>

#include "response.h"
#include "parse.h"
#include "dict.h"
#include "utils.h"

extern char *current_dir;

enum response_code {
  OK, TRY_AGAIN, BAD_REQUEST, NO_CONTENT, NOT_FOUND, INTERNAL_ERROR,
  NOT_IMPLEMENTED, FORBIDDEN
};

struct internal_response {
  enum response_code code;
  DICT headers;
  int length;
  bool is_mmapped;
  char *body;  // NOT a string, might not be null terminated
};

static inline char *make_date(time_t t);

static const char *index_page = "index.html",
                  *server_agent = "crappy_server/0.0.1",
                  *error_format = "<!doctype html>\r\n"
"<html><head>\r\n"
"<title>%s</title>\r\n"
"</head><body>\r\n"
"<h1>%s</h1>"
"<p>%s<br /></p>\r\n"
"</body></html>\r\n";

static inline const char *make_header(const enum response_code code) {
  switch (code) {
    case OK:
      return "200 OK";
    case BAD_REQUEST:
      return "400 Bad Request";
    case NO_CONTENT:
      return "204 No Content";
    case FORBIDDEN:
      return "403 Forbidden";
    case NOT_FOUND:
      return "404 Not Found";
    case TRY_AGAIN:
      return "503 Service Unavailable";
    case INTERNAL_ERROR:
      return "500 Internal Error";
    case NOT_IMPLEMENTED:
      return "501 Not Implemented";
  }
}

static void update_result(const char *const key, const char *const val, va_list args) {
  char **result = va_arg(args, char**);

  int header_len = snprintf(NULL, 0, "%s: %s\r\n", key, val) + 1;
  char buf[header_len];
  snprintf(buf, header_len, "%s: %s\r\n", key, val);

  if (*result == NULL) {
    *result = memcpy(malloc(header_len), buf, header_len);
  } else {
    int result_len = strlen(*result);
    *result = realloc(*result, result_len + header_len);
    //strcat(&(*result)[result_len], buf);
    strlcat(*result, buf, result_len + header_len);
  }
}

static inline char *headers_to_string(DICT headers) {
  char *result = NULL;
  dict_foreach(headers, update_result, &result);
  int len = strlen(result);
  result = realloc(result, len + 3);
  strlcat(result, "\r\n", len + 3);
  return result;
}

static inline char *make_header_line(const enum response_code code) {
  const char *header = make_header(code);
  int len = strlen(header);
  char *buf = malloc(len + 11);
  snprintf(buf, len + 11, "HTTP/1.1 %s\r\n", header);
  return buf;
}

static inline void add_default_headers(DICT headers) {
  dict_put(headers, "Date", make_date(time(NULL)));
  dict_put(headers, "Server", strdup(server_agent));
}

static inline char *make_date(const time_t t) {
  //  %a: 3, %d: 2, %b: 3, %Y: 4, %H: 2, %M: 2, %S: 2
  //  3 + 2 + 2 + 1 + 3 + 1 + 4 + 1 + 2 + 1 + 2 + 1 + 2 + 4 = 29
  //  lets double it, why not
#define LEN 60
  char *date = malloc(LEN);
  struct tm *current_time = localtime(&t);
  if (current_time == NULL) {
    perror("Failed to get current time");
    free(date);
    return "";
  }
  strftime(date, LEN, "%a, %d %b %Y %H:%M:%S GMT", current_time);
  return date;
#undef LEN
}

static void get_file(const char *const filename,
                     struct internal_response *info) {
  int fd = open(filename, O_RDONLY);
  if (fd == -1) {
    // TODO: set up a semaphore and wait until we can open the file
    if (errno == EMFILE) {
      info->code = TRY_AGAIN;
      dict_put(info->headers, "Retry-After", "1");
      return;
    } else if (errno == EACCES) {
      info->code = FORBIDDEN;
      return;
    }
    info->code = INTERNAL_ERROR;
    perror("Could not open file");
    return;
  }
  if (info->length == 0) {
      info->body = NULL;
  } else {
      info->body = (char*)mmap(NULL, info->length, PROT_READ, MAP_SHARED, fd, 0);
  }
  if (info->body != MAP_FAILED) {
    // this is a hack: the memory will stay mapped and
    // we don't have to worry about closing the file later.
    // see `man 2 munmap`
    close(fd);
    info->code = OK;
    info->is_mmapped = true;
    info->headers = dict_init();
    dict_put(info->headers, "Content-Type", strdup(get_mimetype(filename)));
    return;
  }
  perror("Could not mmap file");
  info->code = INTERNAL_ERROR;
}

static void handle_url(struct request_info *info,
                const DICT headers,
                struct internal_response *result) {
  struct stat stat_info;
  int error,
      dir_len = strlen(current_dir),
      file_len = strlen(info->url) + dir_len,
      index_len = file_len + strlen(index_page);
  char *file = strdup(current_dir);
  file = realloc(file, file_len + 1);
  strlcat(file, info->url, file_len + 1);

  if ((error = stat(file, &stat_info)) == 0
      && S_ISDIR(stat_info.st_mode)) {
    // requested a subdirectory with no trailing slash
    if (file[file_len] != '/') {
        file = realloc(file, ++file_len + 1);
        file[file_len] = '/';
    }
    file = realloc(file, index_len + 1);
    strlcat(file, index_page, index_len + 1);
    error = stat(file, &stat_info);
  }

  if (errno == ENOENT) {
    result->code = NOT_FOUND;
    free(file);
    return;
  } else if (errno == EACCES) {
    result->code = FORBIDDEN;
  } else if (error) {
    perror("stat failed");
  }

  result->length = stat_info.st_size;
  get_file(file, result);
  free(file);
  if (info->method == HEAD && result->code == OK) {
    munmap(result->body, result->length);
    result->body = NULL;
    result->length = 0;
  }
  dict_put(result->headers, "Content-Length", ltoa(stat_info.st_size));
  dict_put(result->headers, "Last-Modified", make_date(stat_info.st_mtim.tv_sec));
}

struct response handle_request(const char *request) {
  struct request_info line;
  struct internal_response result;
  request += process_request_line(request, &line);
  result.is_mmapped = false;
  result.headers = dict_init();

  if (line.method == ERROR) {
    result.code = BAD_REQUEST;
  } else if (line.method == NOT_RECOGNIZED) {
    result.code = NOT_IMPLEMENTED;
  } else {
    DICT headers = dict_init();
    request += process_headers(request, headers);
    handle_url(&line, headers, &result);
    dict_free(headers);
  }
  if (result.code != OK) {
    const char *error = "An error occured while processing your request",
               *header = make_header(result.code);
    dict_put(result.headers, "Content-Type", "text/html; charset=utf-8");
    int length = snprintf(NULL, 0, error_format, header, header, error);
    dict_put(result.headers, "Content-Length", itoa(length));
    if (line.method != HEAD) {
        result.length = length;
        result.body = malloc(length+1);
        snprintf(result.body, length+1, error_format, header, header, error);
    } else {
        result.body = NULL;
        result.length = 0;
    }
  }
  add_default_headers(result.headers);
  struct response ret = {
    make_header_line(result.code),
    headers_to_string(result.headers),
    result.body,
    result.length,
    result.is_mmapped,
    strcmp(line.version, "HTTP/1.0") == 0
  };
  return ret;
}
