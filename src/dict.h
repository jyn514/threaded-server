#ifndef DEFS_H
#define DEFS_H

#include <stdio.h>
#include <stdbool.h>

typedef struct dict *DICT;

DICT dict_init(void);
// Returns whether key already exists
// NOTE: these are freed when you call dict_free (in Rust terms, they are owned value)
bool dict_put(DICT, char *key, char *val);
// Returns NULL if item not found
char *dict_get(DICT, const char *key);
void dict_free(DICT);

#endif
