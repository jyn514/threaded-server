#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "str.h"

// this must not be 0
#define INITIAL_SIZE 100
// this must be greater than 1
#define GROWTH_FACTOR 2

struct str *str_init(void) {
    struct str *s = malloc(sizeof(struct str));
    s->buf = malloc(INITIAL_SIZE);
    *s->buf = '\0';
    s->len = 0;
    s->capacity = INITIAL_SIZE;
    return s;
}

bool str_append(struct str *s, const char *format, ...) {
    unsigned int len;
    va_list args;
    va_start(args, format);
    {
        // for some reason clang-tidy thinks args is uninitialized (??)
        // https://stackoverflow.com/questions/58672959
        int slen = vsnprintf(NULL, 0, format, args); // NOLINT
        va_end(args);
        if (slen < 0) return false;
        len = slen + 1;
    }

    unsigned new_capacity = s->capacity;
    while (new_capacity - s->len <= len) {
        new_capacity *= GROWTH_FACTOR;
    }
    if (new_capacity != s->capacity) {
        char *new_buf = realloc(s->buf, new_capacity);
        if (new_buf == NULL) return false;
        s->buf = new_buf;
        s->capacity = new_capacity;
    }
    va_start(args, format);
    vsnprintf(s->buf + s->len, len, format, args);
    va_end(args);
    s->len += len;
    return true;
}

void str_free(struct str *s) {
    free(s->buf);
    free(s);
}
