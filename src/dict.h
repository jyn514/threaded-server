#ifndef DEFS_H
#define DEFS_H

#include <stdio.h>
#include <stdbool.h>

typedef struct dict *DICT;
typedef void (*mapfunc)(const char *const key,
                        const char *const value, va_list args);
DICT dict_init(void);
// returns whether key exists
// NOTE: these are freed when you call dict_free (in Rust terms, they are owned value)
bool dict_put(DICT, char *key, char *val);
char *dict_get(DICT, const char *key);
void dict_free(DICT);
#endif
