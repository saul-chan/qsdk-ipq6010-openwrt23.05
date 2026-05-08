/*===========================================================================
 *  FILE:
 *  samsung_protocol.h
 *
 *==========================================================================================
 */

#ifndef SAMSUNG_PROTOCOL_H
#define SAMSUNG_PROTOCOL_H

#define DLOAD_DEBUG_STRLEN_BYTES 256
#define SAMSUNG_RAW_BUFFER_SIZE (16*1024)
typedef struct
{
  uint32_t save_pref;
  uint32_t  mem_base;
  uint32_t  length;
  char          desc[DLOAD_DEBUG_STRLEN_BYTES];
  char          filename[DLOAD_DEBUG_STRLEN_BYTES];
} dload_debug_type;

typedef struct
{
  uint64_t save_pref;  //force 8 bytes alignment
  uint64_t mem_base;
  uint64_t length;
  char desc[DLOAD_DEBUG_STRLEN_BYTES];
  char filename[DLOAD_DEBUG_STRLEN_BYTES];
} dload_debug_type_64bit;

typedef struct
{
  int mem_base;
  int length;
  char filename[DLOAD_DEBUG_STRLEN_BYTES];
} dload_type;

typedef struct {
    /* buffer for sahara rx */
    void* rx_buffer;

    /* buffer for sahara tx */
    void* tx_buffer;

    /* buffer for memory table */
    void* misc_buffer;

    size_t timed_data_size;

    int fd;

    int ram_dump_image;

    int max_ram_dump_retries;

    uint32_t max_ram_dump_read;
    unsigned int command;
    bool ram_dump_64bit;
} samsung_data_t;
#endif  /* SAHARA_PACKET_H */
