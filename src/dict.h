#ifndef DEFS_H
#define DEFS_H

#include <stdio.h>
#include <stdbool.h>

// Change this to true for debugging output (hitting newline to continue)
#define DEBUG false

#define debug(s)      if (DEBUG) fprintf(stderr,s)
#define debug1(s,t)   if (DEBUG) fprintf(stderr,s,t)
#define debug2(s,t,u) if (DEBUG) fprintf(stderr,s,t,u)

typedef struct dict *DICT;
typedef void (*mapfunc)(const char *const key,
                        const char *const value, va_list args);
DICT dict_init(void);
// returns whether key exists
// note: these are freed when you call dict_free
bool dict_put(DICT, char *const key, char *const val);
char *dict_get(DICT, const char *const key);
void dict_free(DICT);
void dict_foreach(DICT, mapfunc, ...);

#endif
