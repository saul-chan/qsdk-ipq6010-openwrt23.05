#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>
#include <time.h>
#include "utils.h"

uint32_t debug_log = 0;

void *xmalloc(size_t size)
{
    if (size == 0) return NULL;
    void *p = malloc(size);
    memset(p, 0, size);
    if (!p) abort();
    return p;
}

void xfree(void* p)
{
    if (p == NULL)
        return;
    free(p);
    p = NULL;
}

void *xrealloc(void *p, size_t size)
{
    if ((size == 0) && (p == NULL)) return NULL;
    p = realloc(p, size);
    if (!p) abort();
    return p;
}

char *uppercase(char *s)
{
    while(*s) {
        *s = (char)toupper((int32_t)*s);
        s++;
    }
    return s;
}

char *lowercase(char *s)
{
    while(*s) {
        *s = (char)tolower((int32_t)*s);
        s++;
    }
    return s;
}

bool endswith(const char *str, const char *suffix) {
    size_t str_len = strlen(str);
    size_t suffix_len = strlen(suffix);
    return str_len > suffix_len && !strcmp(str + (str_len - suffix_len), suffix);
}

// hex to char
void hex2char(uint8_t *hex)
{
    if(((*hex) >= 0)  && ((*hex) <=9))
        (*hex) += 0x30;
    else if(((*hex) >= 10) && ((*hex) <= 15))
        (*hex) += 0x37;
    else
        (*hex) = 0xff;
}

// hex to string
bool hex2str(uint8_t *outString, uint8_t *rawData, int32_t rawDataLen)
{
    bool result = false;
    uint8_t *pOut = outString;
    uint8_t temp[3];
    int32_t i;
    if(outString == NULL || rawData == NULL)
    {
        return result;
    }
    for(i=0; i<rawDataLen;i++)
    {
        temp[0] = ((rawData[i]>>4) & 0x0F);
        temp[1] = (rawData[i] & 0x0F);
        hex2char(&temp[0]);
        hex2char(&temp[1]);
        *(pOut++) = temp[0];
        *(pOut++) = temp[1];
    }

    result = true;
    return result;
}

bool str2hex(uint8_t *pDest, uint8_t *pSrc, int32_t len)
{
    bool result = false;
    uint8_t hc,lc,hb,lb;
    int32_t i;
    if(pSrc == NULL || pDest == NULL)
        return result;

    for(i=0; i<len; i++)
    {
        hc = pSrc[2*i];
        lc = pSrc[2*i+1];
        hb = toupper(hc) - 0x30;
        if(hb > 9)
            hb -= 7;
        lb = toupper(lc) - 0x30;
        if(lb > 9)
            lb -= 7;

        pDest[i] = hb*16 + lb;
    }

    result = true;
    return result;
}

bool uchar_array_equal(uint8_t *s1, uint8_t *s2, int32_t len)
{
    int32_t i;
    if ((sizeof(s1) < len) || (sizeof(s2) < len))
        return false;
    for (i=0; i<len; i++)
    {
        if (s1[i] != s2[i])
            return false;
    }
    return true;
}

uint32_t get_file_size(const char *path)
{
    FILE *fp;
    struct stat file_stat;
    DLOG("get file size is %s\r\n ",path);
    fp = fopen(path, "rb");
    if (fp != NULL)
    {
        if (0 == fstat(fileno(fp), &file_stat))
        {
            fclose(fp);
            DLOG("st_size is %d\r\n ",file_stat.st_size);
            return file_stat.st_size;
        }
    }

    return 0;

}

void sha256_calc(uint8_t *data, uint32_t length, uint8_t *hashBuf)
{
    SHA256_CTX ctx;
    if ((data == NULL) || (length == 0) || (hashBuf == NULL))
        return;
    sha256_init(&ctx);
    sha256_update(&ctx, data, length);
    sha256_final(&ctx, hashBuf);
}

void m_sleep(int32_t len)
{
    usleep(len*1000);
}

void getCurTimeStamp(char *curTime)
{
    struct timeval tv;
    struct tm *tm;
    gettimeofday(&tv, NULL);
    tm = localtime(&tv.tv_sec);
    sprintf(curTime, "%d-%d-%d %02d:%02d:%02d:%03d",tm->tm_year+1900, tm->tm_mon, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec, (int)tv.tv_usec/1000);
}

void common_log(char *pcFileName, uint64_t line, const char *level, char *pcFormat, ...)
{
    char curTime[256];
    va_list stArgs;

    memset(curTime, 0, sizeof(curTime));
    getCurTimeStamp(curTime);

    printf("[%s][%s][%s-%lu]:", curTime, level, pcFileName, line);
    va_start(stArgs, pcFormat);
    vprintf(pcFormat, stArgs);
    va_end(stArgs);
    printf("\n");
    fflush(stdout);
}

void set_debug(uint32_t value)
{
    debug_log = value;
}
