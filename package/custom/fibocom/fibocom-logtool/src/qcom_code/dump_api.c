#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <byteswap.h>
#include <endian.h>
#include "dloaddef.h"
#include "dump_api.h"
#include "boot_sahara.h"
#ifdef CONFIG_QCOM_DUMP
#include "zlib.h"
#endif
#include <stdint.h>

#define MIN(a,b) (((a)>(b))? (b):(a))
uint8 sahara_packet_buffer[SAHARA_MAX_PACKET_SIZE_IN_BYTES];
uint8 sahara_packet_rcv_buffer[SAHARA_MAX_PACKET_SIZE_IN_BYTES];
uint32 g_u32_memory_table_addr = 0;
uint32 g_u32_memory_table_length = 0;
uint64 g_u64_memory_table_addr = 0;
uint64 g_u64_memory_table_length = 0;
static dload_debug_type g_dload_debug_info[NUM_REGIONS];
static dload_debug_type_64 g_dload_debug64_info[NUM_REGIONS];
char sahara_dump_path[SAHARA_PACKET_LOG_LENGTH] = {0};
int ram_dump_64bit = 0;

#if 0
static void cprintf_hex(const uint8 *buf, int buf_size)
{  
    int i;
    uint32 count = 0;
    cprintf("buf_size: %d\n", buf_size);

    for(i=0; i<buf_size; i++) {
        cprintf("%02X ", buf[i]);
        count++;
        if ((count%4) == 0) {
            cprintf(" ");
        }
        if ((count%32) == 0) {
            cprintf("\n");
        }
    }
    cprintf("\n");
}
#endif

#if 0
static void SetTermios(struct termios *p,word uBaudRate)
{
    cfsetispeed(p, uBaudRate);
    cfsetospeed(p, uBaudRate);
    p->c_cflag &= ~CSIZE;
    p->c_cflag |= CS8;

    p->c_cflag |= ~CS8;
    p->c_cflag &= ~CSTOPB;  //stop bit 1
    p->c_cflag &= ~PARENB;  //no check
    p->c_iflag = 0; //(INPCK | ISTRIP | IGNPAR);//IGNPAR | IUCLC | IXON | IGNCR;p->c_oflag |= 0;
    p->c_lflag = 0; //&= ~(ICANON | ECHO | ECHOE | ISIG);//ICANON;

    p->c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG); /*Input*/
    p->c_oflag &= ~OPOST; /*Output*/

    p->c_cc[VINTR] = 0;
    p->c_cc[VQUIT] = 0;
    p->c_cc[VERASE] = 0;
    p->c_cc[VKILL] = 0;
    p->c_cc[VEOF] = 0;
    p->c_cc[VTIME] = 1;
    p->c_cc[VMIN] = 0;
    p->c_cc[VSWTC] = 0;
    p->c_cc[VSTART] = 0;
    p->c_cc[VSTOP] = 0;
    p->c_cc[VSUSP] = 0;
    p->c_cc[VEOL] = 0;
    p->c_cc[VREPRINT] = 0;
    p->c_cc[VDISCARD] = 0;
    p->c_cc[VWERASE] = 0;
    p->c_cc[VLNEXT] = 0;
    p->c_cc[VEOL2] = 0;
}

int ttycom_init(char *ttypath)
{
    struct termios newio;
    printf("ttypath=%s\n",ttypath);
    int edl_port = open(ttypath,O_RDWR | O_NOCTTY /*| O_NDELAY |O_NONBLOCK*/ );
    if (edl_port == -1) {
        cprintf("open %s failed.", ttypath);
        return edl_port;
    }

    memset(&newio, 0, sizeof(struct termios));
    tcgetattr(edl_port,&newio);
    tcflush(edl_port, TCIOFLUSH);
    SetTermios(&newio,B115200);
    tcflush(edl_port, TCIFLUSH);
    tcsetattr(edl_port,TCSANOW,&newio);
    printf("open sucess.\n");

    return edl_port;
}

#endif

uint64 qlog_le64_fibo(uint64 v64)
{
    const uint64 is_bigendian = 1;
    uint64 tmp = v64;

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
int WriteToComPort(int fd_tty, void *data, const uint32 size)
{
    tcflush(fd_tty, TCOFLUSH);

    int write_len = write(fd_tty, data, size);
    if (write_len != size) {
        cprintf("[%s] failed.\n", __func__);
        return -1;
    }

    return write_len;
}

static int ReadComPort(int fd_tty, uint8 *data, const uint32 size)
{
    int fs_sel = -1;
    fd_set fs_read;
    struct timeval tv_timeout;

    tv_timeout.tv_sec = 5;
    tv_timeout.tv_usec = 0;
    FD_ZERO(&fs_read);
    FD_SET(fd_tty,&fs_read);

    fs_sel = select(fd_tty+1, &fs_read, NULL, NULL, &tv_timeout);
    if (fs_sel == 0) {
        cprintf("[%s] time out.\n", __func__);
        return 0;
    }

    if ((fs_sel > 0) && FD_ISSET(fd_tty, &fs_read)) {
        return read(fd_tty, data, size);
    }

    cprintf("[%s] failed.\n", __func__);
    return -1;
}

int sahara_send_packet
(
  int fd_tty,
  const uint8* packet_buffer,
  const uint32 length
)
{
    int sendbytes = WriteToComPort(fd_tty, (uint8 *)packet_buffer, length);
    //cprintf("OUT\n");
    //cprintf_hex(packet_buffer, sendbytes);

    return sendbytes;
}

int sahara_read_packet
(
    int fd_tty,
    uint8 *packet_buffer,
    size_t length
)
{
    int readbytes = ReadComPort(fd_tty, packet_buffer, length);
    //cprintf("IN\n");
    //cprintf_hex(packet_buffer, readbytes);

    return readbytes;
}

int sahara_read_memory_debug(    int fd_tty,
    uint8 *packet_buffer,
    size_t length)
{
    struct sahara_packet_memory_debug *packet_memory_debug =
                (struct sahara_packet_memory_debug*)packet_buffer;
    int readbytes = ReadComPort(fd_tty, packet_buffer, length);
    if (readbytes < packet_memory_debug->length ){
        readbytes += ReadComPort(fd_tty, packet_buffer+readbytes , packet_memory_debug->length - readbytes );
        if( readbytes == packet_memory_debug->length){
            return readbytes;
        }
    }
    return readbytes;
}

int sahara_no_cmd_id(int fd_tty)
{
    int ret = -1;
    char buf[16] = {0};
    tcflush(fd_tty, TCIOFLUSH);
    ret = sahara_send_packet(fd_tty, (uint8 *)buf, sizeof(buf));
    cprintf("[%s] ret=%d\n", __func__, ret);

    return ret;
}

int sahara_read_hello(int fd_tty)
{
    int i;
    struct sahara_packet_hello *packet_hello = (struct sahara_packet_hello*)sahara_packet_rcv_buffer;
    int cmd_len = sizeof(struct sahara_packet_hello);

    for (i=0; i<2; i++)
    {
        memset(sahara_packet_rcv_buffer, 0, sizeof(sahara_packet_rcv_buffer));
        int ret = sahara_read_packet(fd_tty, sahara_packet_rcv_buffer, cmd_len);
        cprintf("[%s] ret=%d, packet_hello->command=0x%02X, packet_hello->mode=0x%02X\n",__func__, ret, packet_hello->command, packet_hello->mode);
        if (ret == cmd_len && packet_hello->command == SAHARA_HELLO_ID) {
            return packet_hello->mode;
        }
    }

    return -1;
}

int sahara_send_hello_resp(int fd_tty, int mode)
{
    int ret = -1;
    struct sahara_packet_hello_resp *packet_hello_resp =
            (struct sahara_packet_hello_resp*)sahara_packet_buffer;
    int cmd_len = sizeof(struct sahara_packet_hello_resp);

    packet_hello_resp->command = myntohl(SAHARA_HELLO_RESP_ID);
    packet_hello_resp->length = myntohl(cmd_len);
    packet_hello_resp->version = myntohl(SAHARA_VERSION_MAJOR);
    packet_hello_resp->version_supported = myntohl(SAHARA_VERSION_MAJOR_SUPPORTED);
    packet_hello_resp->status = myntohl(SAHARA_STATUS_SUCCESS);
    packet_hello_resp->mode = myntohl(mode);
    packet_hello_resp->reserved0 = 0;
    packet_hello_resp->reserved1 = 0;
    packet_hello_resp->reserved2 = 0;
    packet_hello_resp->reserved3 = 0;
    packet_hello_resp->reserved4 = 0;
    packet_hello_resp->reserved5 = 0;

    tcflush(fd_tty, TCIOFLUSH);
    ret = sahara_send_packet(fd_tty, (uint8 *)packet_hello_resp, cmd_len);
    cprintf("[%s] ret=%d, cmd_len=%d, command=%u, length=%u, mode=%u\n", __func__, ret, cmd_len,
        myntohl(packet_hello_resp->command), myntohl(packet_hello_resp->length), myntohl(packet_hello_resp->mode));

    return ret;
}

int sahara_reset_target(int fd_tty)
{
    struct sahara_packet_reset *packet_reset = (struct sahara_packet_reset*)sahara_packet_buffer;
    int cmd_len = sizeof(struct sahara_packet_reset);

    packet_reset->command = myntohl(SAHARA_RESET_ID);
    packet_reset->length = myntohl(cmd_len);

    int ret = sahara_send_packet(fd_tty, (uint8 *)packet_reset, cmd_len);
    cprintf("[%s] ret=%d, cmd_len=%d, length=%u\n", __func__, ret, cmd_len, packet_reset->length);
    return ret;
}

int sahara_reset_target_resp(int fd_tty)
{
    struct sahara_packet_reset_resp *packet_done_resp = (struct sahara_packet_reset_resp*)sahara_packet_buffer;
    int cmd_len = sizeof(struct sahara_packet_reset_resp);

    int ret = sahara_read_packet(fd_tty, sahara_packet_rcv_buffer, cmd_len);
    cprintf("[%s] ret=%d, cmd_len=%d, length=%u, command=%u\n", __func__, ret, cmd_len,
                    myntohl(packet_done_resp->length), myntohl(packet_done_resp->command));

    if ((ret == cmd_len) && myntohl(packet_done_resp->length) == cmd_len
            && myntohl(packet_done_resp->command) == SAHARA_RESET_RESP_ID) {
        cprintf("[%s] successful.\n", __func__);
    }
    return 0;
}

int sahara_read_memory_debug_info(int fd_tty)
{
    int i;
    struct sahara_packet_memory_debug *packet_memory_debug =
                (struct sahara_packet_memory_debug*)sahara_packet_rcv_buffer;
    struct sahara_packet_memory_64bits_debug *packet_memory64_debug =
                (struct sahara_packet_memory_64bits_debug*)sahara_packet_rcv_buffer;
    int cmd_len = sizeof(struct sahara_packet_memory_debug);
    int cmd64_len = sizeof(struct sahara_packet_memory_64bits_debug);

    for (i=0; i<5; i++)
    {
        memset(sahara_packet_rcv_buffer, 0, sizeof(sahara_packet_rcv_buffer));
        int ret = sahara_read_memory_debug(fd_tty, sahara_packet_rcv_buffer, cmd_len);
        cprintf("[%s] ret=%d ",__func__, ret);
        cprintf("command=0x%02X, length=0x%02X, memory_table_addr=0x%08X, memory_table_length=0x%08X\n",
                    packet_memory_debug->command, packet_memory_debug->length,
                    packet_memory_debug->memory_table_addr, packet_memory_debug->memory_table_length);


        if (ret == cmd_len && packet_memory_debug->command == SAHARA_MEMORY_DEBUG_ID) {
            cprintf("command = SAHARA_MEMORY_DEBUG_ID\n");

            g_u32_memory_table_addr = packet_memory_debug->memory_table_addr;
            g_u32_memory_table_length = packet_memory_debug->memory_table_length;
            //g_u32_memory_table_length = 0x00000980;
            return 0;
        }
        else if (ret == cmd64_len && packet_memory_debug->command == SAHARA_MEMORY_DEBUG_64BITS_ID){
            cprintf("command = SAHARA_MEMORY_DEBUG_64BITS_ID\n");

            ram_dump_64bit = 1;
            g_u64_memory_table_addr = packet_memory64_debug->memory_table_addr;
            g_u64_memory_table_length = packet_memory64_debug->memory_table_length;
            return 0;
        }
        #if 0
        if(i==4)
        {
            if (sahara_reset_target(fd_tty) < 0) {
                return -1;
            }
        }
        #endif
    }
    return -1;
}

int sahara_memory_read_packet(int fd_tty)
{
    int ret = -1;
    struct sahara_packet_memory_read *packet_memory_read =
            (struct sahara_packet_memory_read*)sahara_packet_buffer;
    struct sahara_packet_memory_64bits_debug *packet_memory64_read =
            (struct sahara_packet_memory_64bits_debug*)sahara_packet_buffer;

    int cmd_len = 0;

    if(ram_dump_64bit == 1){
        cmd_len = sizeof(struct sahara_packet_memory_64bits_debug);
        packet_memory64_read->command = myntohl(SAHARA_MEMORY_READ_64BITS_ID);
        packet_memory64_read->length = myntohl(cmd_len);
        packet_memory64_read->memory_table_addr = qlog_le64_fibo(g_u64_memory_table_addr);
        packet_memory64_read->memory_table_length = qlog_le64_fibo(g_u64_memory_table_length);
    }
    else{
        cmd_len = sizeof(struct sahara_packet_memory_read);
        packet_memory_read->command = myntohl(SAHARA_MEMORY_READ_ID);
        packet_memory_read->length = myntohl(cmd_len);
        packet_memory_read->memory_addr = myntohl(g_u32_memory_table_addr);
        packet_memory_read->memory_length = myntohl(g_u32_memory_table_length);
    }

    tcflush(fd_tty, TCIOFLUSH);
    ret = sahara_send_packet(fd_tty, (uint8 *)packet_memory_read, packet_memory_read->length);

    return ret;
}

int sahara_memory64_read_packet_resp(int fd_tty){
    int i;
    int readbytes = -1;
    memset(g_dload_debug64_info, 0, sizeof(g_dload_debug64_info));

    cprintf("%s start.\n", __func__);
    for (i=0; i<2; i++)
    {
        memset(sahara_packet_rcv_buffer, 0, sizeof(sahara_packet_rcv_buffer));
        readbytes = sahara_read_packet(fd_tty, sahara_packet_rcv_buffer, g_u64_memory_table_length);
        if (readbytes > 0)
        {
            memcpy(g_dload_debug64_info, sahara_packet_rcv_buffer, readbytes);
            break;
        }
    }
    if (readbytes > 0) {
        char path[256] = {0};
        snprintf(path, sizeof(path), "%s/dump_info.txt", sahara_dump_path);
        FILE *fp = fopen(path, "a");
        if (fp) {
            fprintf(fp, "---------------------------------------------------------------------\n");
            fprintf(fp, "num    pref    base        length                region    filename\n");
            fprintf(fp, "---------------------------------------------------------------------\n");
        }
        cprintf("---------------------------------------------------------------------\n");
        cprintf("num    pref    base        length                region    filename\n");
        cprintf("---------------------------------------------------------------------\n");
        for (i=0; i<NUM_REGIONS; i++) {
            dload_debug_type_64 *dinfo = &g_dload_debug64_info[i];
            if (dinfo->save_pref) {
                cprintf("%3d %llu    %llu  %llu  %20s     %s\n", i, dinfo->save_pref,
                        dinfo->mem_base, dinfo->length, dinfo->desc, dinfo->filename);

                if (fp) {
                    fprintf(fp, "%3d %llu   %llu %llu  %20s     %s\n", i, dinfo->save_pref,
                        dinfo->mem_base, dinfo->length, dinfo->desc, dinfo->filename);
                }
            }
        }
        if (fp) {
            fclose(fp);
        }
        cprintf("---------------------------------------------------------------------\n");
    }
    return readbytes;
}

int sahara_memory_read_packet_resp(int fd_tty)
{
    int i;
    int readbytes = -1;

    memset(g_dload_debug_info, 0, sizeof(g_dload_debug_info));

    cprintf("%s start.\n", __func__);
    for (i=0; i<2; i++)
    {
        memset(sahara_packet_rcv_buffer, 0, sizeof(sahara_packet_rcv_buffer));
        readbytes = sahara_read_packet(fd_tty, sahara_packet_rcv_buffer, sizeof(g_dload_debug_info));
        if (readbytes > 0)
        {
            memcpy(g_dload_debug_info, sahara_packet_rcv_buffer, readbytes);
            break;
        }
    }

    if (readbytes > 0) {
        char path[256] = {0};
        snprintf(path, sizeof(path), "%s/dump_info.txt", sahara_dump_path);
        FILE *fp = fopen(path, "a");
        if (fp) {
            fprintf(fp, "---------------------------------------------------------------------\n");
            fprintf(fp, "num    pref    base        length                region    filename\n");
            fprintf(fp, "---------------------------------------------------------------------\n");
        }
        cprintf("---------------------------------------------------------------------\n");
        cprintf("num    pref    base        length                region    filename\n");
        cprintf("---------------------------------------------------------------------\n");
        for (i=0; i<NUM_REGIONS; i++) {
            dload_debug_type *dinfo = &g_dload_debug_info[i];
            if (dinfo->save_pref) {
                cprintf("%3d %4d    0x%08X %8d  %20s     %s\n", i, dinfo->save_pref,
                        dinfo->mem_base, dinfo->length, dinfo->desc, dinfo->filename);

                if (fp) {
                    fprintf(fp, "%3d %4d    0x%08X %8d  %20s     %s\n", i, dinfo->save_pref,
                        dinfo->mem_base, dinfo->length, dinfo->desc, dinfo->filename);
                }
            }
        }
        if (fp) {
            fclose(fp);
        }
        cprintf("---------------------------------------------------------------------\n");
    }
    return readbytes;
}


static int sahara_dump_mem_resp(int fd_tty, char *filename, int length, int append, char *name)
{
#ifdef CONFIG_QCOM_DUMP
    int ret = -1;
    uint8 readbuf[SAHARA_MAX_MEMORY_DATA_SIZE_IN_BYTES] = {0};
    char file_name[128] = {0};
    gzFile gzfile = NULL;
    int left_len = length;

    sprintf(file_name, "%s/%s.gz", sahara_dump_path, filename);

    //gzfile = gzopen(file_name, append? "ab6" : "wb6");

    gzfile = gzopen(file_name, append? "ab6" : "wb6");

    if (gzfile == NULL) {
        cprintf("[%s] open '%s' failed\n", __func__, file_name);
        goto END;
    }

    while (left_len > 0)
    {
        int toread = MIN(left_len, sizeof(readbuf));
        int readbytes = sahara_read_packet(fd_tty, readbuf, toread);
        if (readbytes < 0) {
            cprintf("[%s] [%s] total:%d, remaining:%d, toread:%d\n", __func__, name, length, left_len, toread);
            cprintf("[%s] sahara_read_packet failed.\n", __func__);
            goto END;
        }

        if (readbytes == 0) {
            cprintf("ERROR: [%s] [%s] total:%d, remaining:%d, toread:%d\n", __func__, name, length, left_len, toread);
            goto END;
        }

        if (gzwrite(gzfile, readbuf, readbytes) != readbytes)
        {
            cprintf("[%s] gzwrite failed.\n", __func__);
            goto END;
        }

        left_len -= readbytes;
    }
    ret = 0;

END:
    if (gzclose(gzfile) != Z_OK) {
        cprintf("[%s] gzclose failed\n", __func__);
    }
    return ret;
#else
    printf("Error:dump function should define CONFIG_QCOM_DUMP Macro\r\n");
    return -1;
#endif
}

#if 0
static int sahara_dump_mem_resp(int fd_tty, char *filename, int length, int append, char *name)
{
    int ret = -1;
    uint8 readbuf[SAHARA_MAX_MEMORY_DATA_SIZE_IN_BYTES] = {0};
    char file_name[64] = {0};
    //gzFile gzfile = NULL;
    int left_len = length;

    sprintf(file_name, "%s/%s", sahara_dump_path, filename);

    //gzfile = gzopen(file_name, append? "ab6" : "wb6");
    //gzfile = gzopen(file_name, append? "ab6" : "wb6");
    FILE *fp = fopen(file_name, "ab+");
    if (fp == NULL) {
        cprintf("[%s] open '%s' failed\n", __func__, file_name);
        goto END;
    }

    while (left_len > 0)
    {
        int toread = MIN(left_len, sizeof(readbuf));
        int readbytes = sahara_read_packet(fd_tty, readbuf, toread);
        if (readbytes < 0) {
            cprintf("[%s] [%s] total:%d, remaining:%d, toread:%d\n", __func__, name, length, left_len, toread);
            cprintf("[%s] sahara_read_packet failed.\n", __func__);
            goto END;
        }

        if (readbytes == 0) {
            cprintf("ERROR: [%s] [%s] total:%d, remaining:%d, toread:%d\n", __func__, name, length, left_len, toread);
            goto END;
        }

        if (fwrite(readbuf, sizeof(readbuf),readbytes,fp)!= readbytes)
        
        //gzwrite(gzfile, readbuf, readbytes) != readbytes)
        {
            cprintf("[%s] Fwrite failed.\n", __func__);
            goto END;
        }
        left_len -= readbytes;
    }
    ret = 0;

END:
    if (fclose(fp) != 0)
        //gzclose(gzfile) != Z_OK) 
    {
        cprintf("[%s] fclose failed\n", __func__);
    }
    return ret;
}
#endif

int sahara_dump_mem(int fd_tty)
{
    int i;
    int ret = -1;

    for (i=0; i<NUM_REGIONS; i++) {
        dload_debug_type *dinfo = &g_dload_debug_info[i];
        if (dinfo->save_pref) {
            cprintf("---------------------------------------------------------------------\n");
            cprintf("num  pref    base        length                region    filename\n");
            cprintf("---------------------------------------------------------------------\n");
            cprintf("%3d %4d    0x%08X %8d  %20s     %s\n", i, dinfo->save_pref,
                        dinfo->mem_base, dinfo->length, dinfo->desc, dinfo->filename);
            cprintf("---------------------------------------------------------------------\n");

            int left_len = dinfo->length;
            uint32 offset = 0;
            while (left_len > 0)
            {
                struct sahara_packet_memory_read *packet_memory_read =
                            (struct sahara_packet_memory_read*)sahara_packet_buffer;
                int cmd_len = sizeof(struct sahara_packet_memory_read);
                int toread = MIN(left_len, (packet_memory_read_memory_length));
                packet_memory_read->command = myntohl(SAHARA_MEMORY_READ_ID);
                packet_memory_read->length = myntohl(cmd_len);
                packet_memory_read->memory_addr = myntohl(dinfo->mem_base + offset);
                packet_memory_read->memory_length = myntohl(toread);

                cprintf("[%d][%s] toread:%d, left_len:%d\n", i, dinfo->filename, toread, left_len);
                ret = sahara_send_packet(fd_tty, (uint8 *)packet_memory_read, packet_memory_read->length);
                if (ret < 0) {
                    cprintf("[%s] sahara_send_packet ret=%d, cmd_len=%d, length=%d\n", __func__, ret, cmd_len, packet_memory_read->length);
                    return -1;
                }

                if (sahara_dump_mem_resp(fd_tty, dinfo->filename, toread, offset, dinfo->filename) < 0) {
                    return -1;
                }
                offset += toread;
                left_len -= toread;
            }
        }
    }

    return 0;
}

int sahara_dump64_mem(int fd_tty)
{
    int i;
    int ret = -1;

    for (i=0; i<NUM_REGIONS; i++) {
        dload_debug_type_64 *dinfo = &g_dload_debug64_info[i];
        if (dinfo->save_pref) {
            cprintf("---------------------------------------------------------------------\n");
            cprintf("num  pref    base        length                region    filename\n");
            cprintf("---------------------------------------------------------------------\n");
            cprintf("%3d %llu    %llu %llu  %20s     %s\n", i, dinfo->save_pref,
                        dinfo->mem_base, dinfo->length, dinfo->desc, dinfo->filename);
            cprintf("---------------------------------------------------------------------\n");

            int left_len = dinfo->length;
            uint32 offset = 0;
            while (left_len > 0)
            {
                struct sahara_packet_memory_64bits_read *packet_memory64_read =
                            (struct sahara_packet_memory_64bits_read*)sahara_packet_buffer;
                int cmd_len = sizeof(struct sahara_packet_memory_64bits_read);
                int toread = MIN(left_len, (packet_memory_read_memory_length));
                packet_memory64_read->command = myntohl(SAHARA_MEMORY_READ_64BITS_ID);
                packet_memory64_read->length = myntohl(cmd_len);
                packet_memory64_read->memory_addr = qlog_le64_fibo(dinfo->mem_base + offset);
                packet_memory64_read->memory_length = qlog_le64_fibo(toread);

                cprintf("[%d][%s] toread:%d, left_len:%d\n", i, dinfo->filename, toread, left_len);
                ret = sahara_send_packet(fd_tty, (uint8 *)packet_memory64_read, packet_memory64_read->length);
                if (ret < 0) {
                    cprintf("[%s] sahara_send_packet ret=%d, cmd_len=%d, length=%d\n", __func__, ret, cmd_len, packet_memory64_read->length);
                    return -1;
                }

                if (sahara_dump_mem_resp(fd_tty, dinfo->filename, toread, offset, dinfo->filename) < 0) {
                    return -1;
                }
                offset += toread;
                left_len -= toread;
            }
        }
    }

    return 0;
}


int dump_log_collect(int edl_port)
{
    int ret = -1;
    int mode = 0;
    struct timeval startTime;
    struct timeval endTime;
    memset(&startTime, 0, sizeof(struct timeval));
    memset(&endTime, 0, sizeof(struct timeval));
    gettimeofday(&startTime, NULL); //get start time

    if (sahara_no_cmd_id(edl_port) < 0) {
        goto END;
    }
    mode = sahara_read_hello(edl_port);
    if (mode < 0) {
        goto END;
    }
    if (sahara_send_hello_resp(edl_port, mode) < 0) {
        goto END;
    }
    if (sahara_read_memory_debug_info(edl_port) < 0) {
        goto END;
    }
    if (sahara_memory_read_packet(edl_port) < 0) {
        goto END;
    }
    if(ram_dump_64bit){
        if (sahara_memory64_read_packet_resp(edl_port) < 0) {
            goto END;
        }
        if (sahara_dump64_mem(edl_port)){
            goto END;
        }
    }
    else{
        if (sahara_memory_read_packet_resp(edl_port) < 0) {
            goto END;
        }
        if (sahara_dump_mem(edl_port) < 0) {
        goto END;
        }
    }

    if (sahara_reset_target(edl_port) < 0) {
        goto END;
    }
    if (sahara_reset_target_resp(edl_port) < 0) {
        goto END;
    }
    cprintf("-------------- [Finished] -----------------\n");
    ret = 0;
    gettimeofday(&endTime, NULL); //get end time
    show_used_time(&startTime, &endTime);
    return ret;
END:
    cprintf("-------------- [excepted] -----------------\n");
    ret = 0;
    /* if we don't save dump log successfully, can't reset the module MTC0732-2282 */
    //sahara_reset_target(edl_port);
    gettimeofday(&endTime, NULL); //get end time
    show_used_time(&startTime, &endTime);
    return ret;
}

static int mkdirs(char *path)
{
    int i;
    int len=strlen(path);
    char str[len+1];
    printf("path=%s\n",path);
    strncpy(str, path, sizeof(str));
    printf("str=%s\n",str);
    for(i=0; i<len; i++)
    {
        if (str[i]=='/' )
        {
            str[i] = '\0';
            if (access(str,0))
            {
                umask(0);
                if (mkdir(str, 0777))
                {
                    printf("mkdir '%s' failed.\n", str);
                    perror("mkdir"),exit(-1);
                }
            }
            str[i]='/';
        }
    }

    if (len>0 && access(str, 0))
    {
        umask(0); 
        if (mkdir(str, 0777)!=0)
        {
            printf("1mkdir '%s' failed.\n", str);
            perror("mkdir"),exit(-1);
        }
    }
    return 0;
}

int set_sahara_dump_path(char *file_path)
{
    printf("file_path=%s\n",file_path);
    if (file_path == NULL) {
        printf("sahara_dump_path1=%s\n",sahara_dump_path);
        sprintf(sahara_dump_path, "%s", ".");
        printf("sahara_dump_path2=%s\n",sahara_dump_path);
    } else { 
        //cprintf("sahara_dump_path11=%s\n",sahara_dump_path);
        sprintf(sahara_dump_path, "%s", file_path);
       // snprintf(sahara_dump_path, sizeof(sahara_dump_path), "%s",file_path);
        //sahara_dump_path=file_path;
        //cprintf("p=%s\n",p);
        printf("sahara_dump_path22=%s\n",sahara_dump_path);
    }

    printf("[%s] sahara_dump_path=%s\n", __func__, sahara_dump_path);

    return mkdirs(sahara_dump_path);
}

void show_used_time(struct timeval *startTime, struct timeval *endTime)
{
    struct timeval diff;

    //DEBUG_INFO("%s start.\n", __func__);

    if (startTime->tv_sec > endTime->tv_sec)
        return;

    if ((startTime->tv_sec == endTime->tv_sec) && (startTime->tv_usec > endTime->tv_usec))
        return;

    if (endTime->tv_usec >= startTime->tv_usec) {
        diff.tv_sec = endTime->tv_sec - startTime->tv_sec;
        diff.tv_usec = endTime->tv_usec - startTime->tv_usec;
    } else {
        endTime->tv_sec--;
        diff.tv_sec = endTime->tv_sec - startTime->tv_sec;
        diff.tv_usec = (1000*1000) + endTime->tv_usec-startTime->tv_usec;
    }
    cprintf("========================================\n");
    cprintf("Used Time: %d.%ds\n",
                (int)diff.tv_sec,
                (int)diff.tv_usec/1000);
    cprintf("========================================\n");
}

