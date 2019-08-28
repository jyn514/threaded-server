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
#include "log.h"

#define append(str, other) { {\
    int str_len__ = strlen(str), other_len__ = strlen(other); \
    char *tmp = realloc(str, str_len__ + other_len__ + 1); \
    if (tmp == NULL) perror("append: realloc failed"); \
    else { str = tmp; strncat(str, other, other_len__); } \
} }

extern char current_dir[];

struct internal_response {
  enum response_code code;
  int length;
  bool is_mmapped;
  char *headers;
  char *body;  // NOT a string, might not be null terminated
  struct log *logger;
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
      append(info->headers, "Retry-After: 1\r\n");
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
    const char *mimetype = get_mimetype(filename);
    if (mimetype != NULL) {
        char buf[MAX_MIMETYPE + 17];
        snprintf(buf, MAX_MIMETYPE + 17, "Content-Type: %s\r\n", mimetype);
        append(info->headers, buf);
    }
    return;
  }
  perror("Could not mmap file");
  info->code = INTERNAL_ERROR;
}

static void handle_url(struct request_info *info,
                const DICT headers,
                struct internal_response *result) {
  struct stat stat_info;
  int error;
  char *file = strdup(current_dir);
  append(file, info->url);

  if ((error = stat(file, &stat_info)) == 0
      && S_ISDIR(stat_info.st_mode)) {
    // requested a subdirectory with no trailing slash
    if (file[strlen(file)] != '/') append(file, "/");
    append(file, index_page);
    error = stat(file, &stat_info);
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
      free(file);
      return;
  }

  result->length = stat_info.st_size;
  get_file(file, result);
  free(file);
  if (info->method == HEAD && result->code == OK) {
    munmap(result->body, result->length);
    result->body = NULL;
    result->length = 0;
  }
  if (result->code == OK) {
    char header[100];
    char *date = add_date(stat_info.st_mtime);
    int len = snprintf(header, 100, "Content-Length: %ld\r\n", (long)stat_info.st_size);
    if (date != NULL) {
        snprintf(&header[len], 100 - len, "Last-Modified: %s\r\n", date);
        free(date);
    }
    append(result->headers, header);
  }
}

struct response handle_request(char *orig_request) {
  struct request_info line;
  struct internal_response result;
  DICT headers = dict_init();
  char *request = orig_request + process_request_line(orig_request, &line);
  result.is_mmapped = false;
  *(result.headers = malloc(1)) = '\0';
  result.logger = log_init();

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
    append(result.headers, "Content-Type: text/html; charset=utf-8\r\n");
    int length = snprintf(NULL, 0, error_format, header, header, error);
    char buf[50];
    snprintf(buf, 50, "Content-Length: %d\r\n", length);
    append(result.headers, buf);
    if (line.method != HEAD) {
        result.length = length;
        result.body = malloc(length+1);
        snprintf(result.body, length+1, error_format, header, header, error);
    } else {
        result.body = NULL;
        result.length = 0;
    }
  }

  int len = MAX_DATE + 21 + strlen(server_agent);
  char other_headers[len];
  int written = snprintf(other_headers, len - MAX_DATE - 8,
          "Server: %s\r\n", server_agent);

  char *date = add_date(time(NULL)),
       *user_agent = dict_get(headers, "User-Agent");

  *(request - 2) = '\0';  // only show header line of original request, minus newline
  log_write(result.logger, "[%s] \"%s\" %d %d \"%s\"",
            date == NULL ? "-" : date, orig_request, result.code,
            result.length, user_agent == NULL ? "-" : user_agent);
  log_flush(result.logger, stdout);
  log_free(result.logger);
  dict_free(headers);

  if (date != NULL) {
      snprintf(other_headers + written, len - written,
              "Date: %s\r\n\r\n", date);
      free(date);
  } else strncat(other_headers, "\r\n", 2);
  append(result.headers, other_headers);
  struct response ret = {
    make_header_line(result.code),
    result.headers,
    result.body,
    result.length,
    result.is_mmapped,
    strcmp(line.version, "HTTP/1.0") != 0
  };
  free(line.version);
  free(line.url);
  return ret;
}
