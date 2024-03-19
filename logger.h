/**
 * @file
 *
 * Helps facilitate debugging by providing basic logging functionality. Unlike
 * printf-style debugging, the log messages can be enabled/disabled by changing 
 * the value of LOGGER.
 */

#ifndef _LOGGER_H_
#define _LOGGER_H_

#include <stdio.h>
#include <unistd.h>

/**
 * If LOGGER is not set, it will be enabled by default.
 */
#ifndef LOGGER
#define LOGGER 1
#endif

/**
 * LOGGER_COLOR determines whether debug output is colorized. It is enabled by
 * default.
 */
#ifndef LOGGER_COLOR
#define LOGGER_COLOR 1
#endif

#if LOGGER_COLOR
#define LOGGER_COLOR_RED   "\033[0;31m"
#define LOGGER_COLOR_BLUE  "\033[1;34m"
#define LOGGER_COLOR_RESET "\033[0m"
#endif

/**
 * Prints an unformatted log message (single string).
 *
 * Example Usage:
 * LOGP("Hello world!");
 */
#define LOGP(str) \
    do { \
        if (LOGGER) { \
            if (LOGGER_COLOR) { \
                if (isatty(STDERR_FILENO)) { \
                    fprintf(stderr, "%s%s%s:%d:%s%s()%s: %s", \
                            LOGGER_COLOR_RED, __FILE__, LOGGER_COLOR_RESET, \
                            __LINE__, \
                            LOGGER_COLOR_BLUE, __func__, LOGGER_COLOR_RESET, \
                            str); \
                    break; \
                } \
                fprintf(stderr, "%s:%d:%s(): %s", \
                        __FILE__, __LINE__, __func__, str); \
            } \
        } \
    } while (0)

/**
 * Prints a formatted log message.
 *
 * Example Usage:
 * LOG("Hello %s, your lucky number is %d\n", "World", 42);
 */
#define LOG(fmt, ...) \
    do { \
        if (LOGGER) { \
            if (LOGGER_COLOR) { \
                if (isatty(STDERR_FILENO)) { \
                    fprintf(stderr, "%s%s%s:%d:%s%s()%s: " fmt, \
                            LOGGER_COLOR_RED, __FILE__, LOGGER_COLOR_RESET, \
                            __LINE__, \
                            LOGGER_COLOR_BLUE, __func__, LOGGER_COLOR_RESET, \
                            __VA_ARGS__); \
                    break; \
                } \
                fprintf(stderr, "%s:%d:%s(): " fmt, __FILE__, \
                        __LINE__, __func__, __VA_ARGS__); \
            } \
        } \
    } while (0)

#endif