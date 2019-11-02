#include <stdbool.h>

// a dynamically allocated, resizable string class
// individual elements of `buf` can be modified freely,
// but to grow `buf` you must use `str_append`.
struct str {
    char *buf;
    // number used (including null terminator)
    unsigned int len;
    // number available
    unsigned int capacity;
};

struct str *str_init(void);
// returns false if an error occured
// format is the same as for printf
bool str_append(struct str *, const char *format, ...);
void str_free(struct str *);
