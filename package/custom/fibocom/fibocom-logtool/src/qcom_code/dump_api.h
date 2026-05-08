#ifndef __DUMP_API_H
#define __DUMP_API_H

#if 0
#define cprintf(fmt, args...) printf(fmt, ## args)
#else
#define cprintf(fmt, args...) do { \
    char log_path[256]={0}; \
    snprintf(log_path, sizeof(log_path),"%s/dump_pull.log", sahara_dump_path); \
    FILE *fp = fopen(log_path, "a"); \
    if (fp) { \
        printf(fmt, ## args); \
        fprintf(fp, fmt, ## args); \
        fclose(fp); \
    } \
} while (0)
#endif

#define MAX_RETRY_TIMES 5
#define HEX_BUFFER_LEN (0x03F9)
#define FLASH_TX_LOAD_LEN 1024

#ifdef FIBO_CUSTOMER_GAOHONG
#define packet_memory_read_memory_length 8*1024
#else
#define packet_memory_read_memory_length 64*1024
#endif


#if (__BYTE_ORDER == __BIG_ENDIAN)
#define myntohl(x)  bswap_32 (x)
#define myntohs(x)  bswap_16 (x)
#define myhtonl(x)  bswap_32 (x)
#define myhtons(x)  bswap_16 (x)
#elif (__BYTE_ORDER == __LITTLE_ENDIAN)
#define myntohl(x)        (x)
#define myntohs(x)        (x)
#define myhtonl(x)        (x)
#define myhtons(x)        (x)
#else
#error "ERROR: '__BYTE_ORDER' is not defined"
#endif


#define SAHARA_MAX_PACKET_SIZE_IN_BYTES 0x4000
#define SAHARA_PACKET_LOG_LENGTH 0x64
#define SAHARA_MAX_MEMORY_DATA_SIZE_IN_BYTES 0x1000
// #define SAHARA_RAM_ZI_SIZE 0x20000
// #define SECTOR_SIZE_IN_BYTES 512

#define DLOAD_DEBUG_STRLEN_BYTES 20
#define NUM_REGIONS 50//zbx change for FM101/FM160
/*debug structure for regular 32 bits ram dump */
typedef struct
{
  char save_pref;
  unsigned int mem_base;
  unsigned int length;
  char desc[DLOAD_DEBUG_STRLEN_BYTES];
  char filename[DLOAD_DEBUG_STRLEN_BYTES];
} dload_debug_type;

typedef struct
{
  unsigned long long save_pref;  //force 8 bytes alignment
  unsigned long long mem_base;
  unsigned long long length;
  char desc[DLOAD_DEBUG_STRLEN_BYTES];
  char filename[DLOAD_DEBUG_STRLEN_BYTES];
} dload_debug_type_64;


extern char sahara_dump_path[SAHARA_PACKET_LOG_LENGTH];

int ttycom_init(char *ttypath);
int sahara_no_cmd_id(int fd_tty);
int sahara_read_hello(int fd_tty);
int sahara_send_hello_resp(int fd_tty, int mode);
int sahara_read_memory_debug_info(int fd_tty);
int sahara_memory_read_packet(int fd_tty);
int sahara_memory_read_packet_resp(int fd_tty);
int sahara_dump_mem(int fd_tty);
int dump_log_collect(int edl_port);
int set_sahara_dump_path(char *file_path);
void show_used_time(struct timeval *startTime, struct timeval *endTime);

#endif
