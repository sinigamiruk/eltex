#ifndef RAW_SOCKETS_UTILS_H
#define RAW_SOCKETS_UTILS_H

#include <stdio.h>
#include <errno.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

#include <sys/types.h>

#ifndef RETRY
#define RETRY(expression)                           \
    (__extension__                                  \
     ({  int __result;                              \
         do __result = (long int) (expression);     \
         while (__result == -1L && errno == EINTR); \
         __result; }))
#endif

u_short checksum(u_short *addr, int len);
void die(int line, const char * format, ...);

#endif
