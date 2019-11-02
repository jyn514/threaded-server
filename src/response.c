/* Copyright 2019 Joshua Nelson
 * This program is licensed under the BSD 3-Clause License.
 * See LICENSE.txt or https://opensource.org/licenses/BSD-3-Clause for details.
 *
 * Responder. Given a request string,
 * performs all application logic to create a response.
 */
#define _POSIX_C_SOURCE 200809L

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <string.h>

#include <stdarg.h>
#include <stdlib.h>
#include <errno.h>

#include "response.h"
#include "parse.h"
#include "dict.h"
#include "str.h"

extern char current_dir[];

struct internal_response {
  enum response_code code;
  int length;
  bool is_mmapped;
  char *body;  // NOT a string, might not be null terminated
  struct str *logger, *headers;
};

static const char *index_page = "index.html",
                  *server_agent = "threaded_server/0.0.1",
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

static inline char *make_header_line(const enum response_code code) {
  const char *header = make_header(code);
  int len = strlen(header);
  char *buf = malloc(len + 12);
  snprintf(buf, len + 12, "HTTP/1.1 %s\r\n", header);
  return buf;
}

static inline char *add_date(const time_t t) {
  struct tm *current_time = localtime(&t);
  if (current_time == NULL) {
    perror("Failed to get current time");
    return NULL;
  }
  char *date = malloc(MAX_DATE);
  strftime(date, MAX_DATE, "%a, %d %b %Y %H:%M:%S GMT", current_time);
  return date;
}

static void get_file(const char *const filename,
                     struct internal_response *info) {
  int fd = open(filename, O_RDONLY);
  if (fd == -1) {
    // TODO: set up a semaphore and wait until we can open the file
    if (errno == EMFILE) {
      info->code = TRY_AGAIN;
      str_append(info->headers, "Retry-After: 1\r\n");
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
      if (info->body == MAP_FAILED) {
        printf("file: %s, fd: %d\n", filename, fd);
        perror("Could not mmap file");
        info->code = INTERNAL_ERROR;
        return;
      }
  }
  // this is a hack: the memory will stay mapped and
  // we don't have to worry about closing the file later.
  // see `man 2 munmap`
  close(fd);
  info->code = OK;
  info->is_mmapped = true;
  const char *mimetype = get_mimetype(filename);
  if (mimetype != NULL) {
      str_append(info->headers, "Content-Type: %s\r\n", mimetype);
  }
}

static void handle_url(struct request_info *info,
                const DICT headers,
                struct internal_response *result) {
  struct stat stat_info;
  int error;
  struct str *file = str_init();
  str_append(file, "%s%s", current_dir, info->url);
  printf("url: %s, file: %s\n", info->url, file->buf);

  if ((error = stat(file->buf, &stat_info)) == 0
      && S_ISDIR(stat_info.st_mode)) {

    // requested a subdirectory with no trailing slash
    if (file->buf[file->len - 1] != '/') str_append(file, "/");

    printf("append_worked? %d, index: %s, new file: %s\n",
            str_append(file, "%s", index_page), index_page, file->buf);
    error = stat(file->buf, &stat_info);
  }

  if (error) {
      if (errno == ENOENT) {
        result->code = NOT_FOUND;
      } else if (errno == EACCES) {
        result->code = FORBIDDEN;
      } else {
        perror("stat failed");
        result->code = INTERNAL_ERROR;
      }
      str_free(file);
      return;
  }

  result->length = stat_info.st_size;
  get_file(file->buf, result);
  str_free(file);
  if (info->method == HEAD && result->code == OK) {
    munmap(result->body, result->length);
    result->body = NULL;
    result->length = 0;
  }
  if (result->code == OK) {
    str_append(result->headers, "Content-Length: %ld\r\n", (long)stat_info.st_size);
    char *date = add_date(stat_info.st_mtime);
    if (date != NULL) {
        str_append(result->headers, "Last-Modified: %s\r\n", date);
        free(date);
    }
  }
}

struct response handle_request(char *orig_request) {
  struct request_info line;
  struct internal_response result;
  DICT headers = dict_init();
  char *request = orig_request + process_request_line(orig_request, &line);
  result.is_mmapped = false;
  result.headers = str_init(), result.logger = str_init();

  if (line.method == ERROR) {
    result.code = BAD_REQUEST;
  } else if (line.method == NOT_RECOGNIZED) {
    result.code = NOT_IMPLEMENTED;
  } else {
    process_headers(request, headers);
    handle_url(&line, headers, &result);
  }
  if (result.code != OK) {
    const char *error = "An error occured while processing your request",
               *header = make_header(result.code);
    str_append(result.headers, "Content-Type: text/html; charset=utf-8\r\n");
    int length = snprintf(NULL, 0, error_format, header, header, error);
    str_append(result.headers, "Content-Length: %d\r\n", length);
    if (line.method != HEAD) {
        result.length = length;
        result.body = malloc(length+1);
        snprintf(result.body, length+1, error_format, header, header, error);
    } else {
        result.body = NULL;
        result.length = 0;
    }
  }

  str_append(result.headers, "Server: %s\r\n", server_agent);

  char *date = add_date(time(NULL)),
       *user_agent = dict_get(headers, "User-Agent");

  *(request - 2) = '\0';  // only show header line of original request, minus newline
  str_append(result.logger, "[%s] \"%s\" %d %d \"%s\"",
            date == NULL ? "-" : date, orig_request, result.code,
            result.length, user_agent == NULL ? "-" : user_agent);
  puts(result.logger->buf);
  str_free(result.logger);
  dict_free(headers);

  if (date != NULL) {
      str_append(result.headers, "Date: %s\r\n", date);
      free(date);
  }
  str_append(result.headers, "\r\n");
  struct response ret = {
    make_header_line(result.code),
    result.headers->buf,
    result.body,
    result.length,
    result.is_mmapped,
    strcmp(line.version, "HTTP/1.0") != 0
  };
  free(line.version);
  free(line.url);
  free(result.headers);  // doesn't free the buf
  return ret;
}
