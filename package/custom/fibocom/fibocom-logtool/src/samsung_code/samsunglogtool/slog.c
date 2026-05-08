/******************************************************************************
  @file    slog.c
  @brief   log tool.

  ---------------------------------------------------------------------------
******************************************************************************/
#include "slog.h"
#include "log_control.h"

uint16_t slog_le16(uint16_t v16)
{
    uint16_t tmp = v16;
    const int is_bigendian = 1;

    if ( (*(char*)&is_bigendian) == 0 ) {
        uint8_t *s = (uint8_t *)(&v16);
        uint8_t *d = (uint8_t *)(&tmp);
        d[0] = s[1];
        d[1] = s[0];
    }
    return tmp;
}

uint32_t slog_le32(uint32_t v32)
{
    uint32_t tmp = v32;
    const int is_bigendian = 1;

    if ( (*(char*)&is_bigendian) == 0) {
        uint8_t *s = (uint8_t *)(&v32);
        uint8_t *d = (uint8_t *)(&tmp);
        d[0] = s[3];
        d[1] = s[2];
        d[2] = s[1];
        d[3] = s[0];
    }
    return tmp;
}

uint64_t slog_le64(uint64_t v64)
{
    const uint64_t is_bigendian = 1;
    uint64_t tmp = v64;

    if ((*(char*)&is_bigendian) == 0)
    {
        unsigned char *s = (unsigned char *)(&v64);
        unsigned char *d = (unsigned char *)(&tmp);
        d[0] = s[7];
        d[1] = s[6];
        d[2] = s[5];
        d[3] = s[4];
        d[4] = s[3];
        d[5] = s[2];
        d[6] = s[1];
        d[7] = s[0];
    }
    return tmp;
}
