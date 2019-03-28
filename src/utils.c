#include <stdlib.h>
#include <stdio.h>
#include <string.h>

char *itoa(int i) {
    char *ret = malloc(sizeof(int) + 1);
    snprintf(ret, sizeof(int) + 1, "%d", i);
    return ret;
}

char *ltoa(long l) {
    char *ret = malloc(sizeof(long) + 1);
    snprintf(ret, sizeof(long) + 1, "%ld", l);
    return ret;
}

char *substr(const char *const str, const int len) {
    char *ret = malloc(sizeof(char)*len + 1);
    memcpy(ret, str, len);
    ret[len] = '\0';
    return ret;
}
