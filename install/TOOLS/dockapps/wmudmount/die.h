#ifndef DIE_H
#define DIE_H

#include <string.h>
#include <errno.h>

#define DEBUG_DEBUG 4
#define DEBUG_INFO  3
#define DEBUG_WARN  2
#define DEBUG_ERROR 1

extern int warn_level;

void warn(int level, const char *fmt, ...) __attribute__ ((format (printf, 2, 3)));
void die(const char *fmt, ...) __attribute__ ((noreturn, format (printf, 1, 2)));

#endif
