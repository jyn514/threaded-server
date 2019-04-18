#include <stdbool.h>

struct log;

struct log *log_init(void);
// returns false if we ran out of memory
// len is optional
// 0 if unused, length of message (including NULL terminator) otherwise
bool log_write(struct log *, char *format, ...);
// returns whether IO succeeded
bool log_flush(struct log *, FILE *stream);
void log_free(struct log*);
