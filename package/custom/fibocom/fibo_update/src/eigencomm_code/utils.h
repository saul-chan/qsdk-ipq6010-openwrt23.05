#pragma once

#include <unistd.h>
#include <sys/time.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdarg.h>
#include "sha256.h"


#define DLOG(format, args...) {if (1 == get_debug()) common_log(__FILE__, __LINE__, "DEBUG", format, ##args);}
#define BINDLOG(format, args...) {if (1 == get_debug()) printf(format, ##args);}
#define ILOG(format, args...) common_log(__FILE__, __LINE__, "INFO", format, ##args)
#define ELOG(format, args...) common_log(__FILE__, __LINE__, "ERROR", format, ##args)

extern uint32_t debug_log;

// malloc with NULL check
void *xmalloc(size_t size);

// free with NULL check
void xfree(void* p);

// realloc with NULL check
void *xrealloc(void *p, size_t size);

// Convert a string to upper case
char *uppercase(char *s);

// Convert a string to lower case
char *lowercase(char *s);

// Check whether str ends with suffix
bool endswith(const char *str, const char *suffix);

// hex to char
void hex2char(uint8_t *hex);

// hex to string
bool hex2str(uint8_t *outString, uint8_t *rawData, int32_t rawDataLen);

// string to hex
bool str2hex(uint8_t *pDest, uint8_t *pSrc, int32_t len);

// compare two char array by specified length
bool uchar_array_equal(uint8_t *s1, uint8_t *s2, int32_t len);

// get file size
uint32_t get_file_size(const char *path);

// sha256
void sha256_calc(uint8_t *data, uint32_t length, uint8_t *hashBuf);

// sleep fuction, unit: ms
void m_sleep(int32_t len);

// get timestamp
void getCurTimeStamp(char *curTime);

void common_log(char *pcFileName, uint64_t line, const char *level, char *pcFormat, ...);

void set_debug(uint32_t value);
static inline uint32_t get_debug(){return debug_log;}