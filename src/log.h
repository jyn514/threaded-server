#include <stdbool.h>

// This is basically a string class under the hood.

struct log *log_init(void);
// returns false if an error occured
// format is the same as for printf
bool log_write(struct log *, const char *format, ...);
// returns whether IO succeeded
bool log_flush(struct log *, FILE *stream);
void log_free(struct log*);
