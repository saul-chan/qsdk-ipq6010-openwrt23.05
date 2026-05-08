// log.h
#ifndef LOG_H
#define LOG_H

#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>

#define LOG_ENABLE 1

// Define log level
#define LOG_LEVEL_DEBUG 0
#define LOG_LEVEL_INFO  1
#define LOG_LEVEL_WARN  2
#define LOG_LEVEL_ERROR 3

// set current log level
#define CURRENT_LOG_LEVEL LOG_LEVEL_INFO
#define LOG_TAG "Silentlogging"

#if LOG_ENABLE
    #define LOG_PRINT(level, fmt, ...) \
        do { \
            if (level >= CURRENT_LOG_LEVEL) { \
                struct timeval tv; \
                gettimeofday(&tv, NULL); \
                int hours = (tv.tv_sec / 3600) % 24; \
                int minutes = (tv.tv_sec / 60) % 60; \
                int seconds = tv.tv_sec % 60; \
                long milliseconds = tv.tv_usec / 1000; \
                printf("[%02d:%02d:%02d.%03ld] [%s] " fmt, hours, minutes, seconds, milliseconds, LOG_TAG, ##__VA_ARGS__); \
            } \
        } while (0)

    #define LOG_PRINT_D(fmt, ...) LOG_PRINT(LOG_LEVEL_DEBUG, fmt, ##__VA_ARGS__)
    #define LOG_PRINT_I(fmt, ...) LOG_PRINT(LOG_LEVEL_INFO, fmt, ##__VA_ARGS__)
    #define LOG_PRINT_W(fmt, ...) LOG_PRINT(LOG_LEVEL_WARN, fmt, ##__VA_ARGS__)
    #define LOG_PRINT_E(fmt, ...) LOG_PRINT(LOG_LEVEL_ERROR, fmt, ##__VA_ARGS__)
#else
    #define LOG_PRINT_D(fmt, ...)
    #define LOG_PRINT_I(fmt, ...)
    #define LOG_PRINT_W(fmt, ...)
    #define LOG_PRINT_E(fmt, ...)
#endif

#endif // LOG_H
