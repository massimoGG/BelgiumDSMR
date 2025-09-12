#ifndef COMMON_H
#define COMMON_H

void setupLogs(void);
void printErrno(const char *prefix, const char *format, ...);

#define printError(...) _printLog(stderr, __VA_ARGS__)
#define printLog(...) _printLog(stdout, __VA_ARGS__)

#include <stdio.h>
void _printLog(FILE *f, const char *prefix, const char *format, ...);
#endif