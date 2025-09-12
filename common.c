/**
 * common.c - Commonly used functions that can be used anywhere
 */
#include <stdio.h>  // For printf
#include <stdlib.h> // For exit
#include <stdarg.h>
#include <string.h>
#include <locale.h>
#include <errno.h>

/**
 * setupLogs sets up the locale of the user terminal
 */
void setupLogs(void)
{
    setlocale(LC_ALL, "");
}

/**
 * printErrno prints the given prefix and format followed by
 * the error number and errno string meaning
 */
void printErrno(const char *prefix, const char *format, ...)
{
    // Fetch errno value
    int errsv = errno;
    va_list va;
    va_start(va, format);
    fprintf(stderr, "%s:\t", prefix);
    vfprintf(stderr, format, va);
    fprintf(stderr, "\n\terrno %d: %s\n\n", errsv, strerror(errsv));
    va_end(va);
}
/**
 * printLog prints the given prefix and format using variable arguments
 */
void _printLog(FILE *f, const char *prefix, const char *format, ...)
{
    va_list va;
    va_start(va, format);
    fprintf(f, "%s:\t", prefix);
    vfprintf(f, format, va);
    fprintf(f, "\n");
    va_end(va);
}

/**
 * Matches string for figuring out OID
 */