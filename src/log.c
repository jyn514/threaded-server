#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "log.h"

#define INITIAL_SIZE 100
#define GROWTH_FACTOR 2

struct log {
    char *buf;
    // number used
    unsigned int len;
    // number available
    unsigned int capacity;
};

struct log *log_init(void) {
    struct log *logger = malloc(sizeof(struct log));
    logger->buf = malloc(INITIAL_SIZE);
    *logger->buf = '\0';
    logger->len = 0;
    logger->capacity = INITIAL_SIZE;
    return logger;
}

bool log_write(struct log *logger, char *format, ...) {
    va_list args;
    va_start(args, format);
    // for some reason clang-tidy thinks args is uninitialized (??)
    unsigned int len = vsnprintf(NULL, 0, format, args) + 1;  // NOLINT
    va_end(args);

    unsigned new_capacity = logger->capacity;
    while (new_capacity - logger->len <= len) {
        new_capacity *= GROWTH_FACTOR;
    }
    if (new_capacity != logger->capacity) {
        char *new_buf = realloc(logger->buf, new_capacity);
        if (new_buf == NULL) return false;
        logger->buf = new_buf;
        logger->capacity = new_capacity;
    }
    va_start(args, format);
    vsnprintf(logger->buf + logger->len, len, format, args);
    va_end(args);
    logger->len += len;
    return true;
}

bool log_flush(struct log *logger, FILE *stream) {
    bool ret = fputs(logger->buf, stream) != EOF;
    putc('\n', stream);
    return ret;
}

void log_free(struct log *logger) {
    free(logger->buf);
    free(logger);
}
