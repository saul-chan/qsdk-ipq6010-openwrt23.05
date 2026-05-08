/******************************************************************************
  @file    qlog.c
  @brief   mdm log tool.

  DESCRIPTION
  qlog for USB and PCIE of Fibocom wireless cellular modules.
  INITIALIZATION AND SEQUENCING REQUIREMENTS
  None.
  ---------------------------------------------------------------------------
******************************************************************************/
#include "qlog.h"
#include "log_control.h"

/*
80-na157-61_yb_diagnostic_system_user_guide.pdf
For non-HDLC encoded config
When .cfg2 is used, which is non-HDLC encoded, the format of the file is as follows:
Field       Length (in bits)    Description
-------     ----------------    -----------
Start           8               This is the start of packet, 0x7E
Version         8               Version
Payload Length  16              Payload length
Payload Variable This is the actual data
Packet End  8 Ending character "0x7E"

1d 1c 3b 7e // 0x1D == Time Stamp Request
00 78 f0 7e // 0x00 == Version Request
7c 93 49 7e // 0x7C == Extended Build ID Request
1c 95 2a 7e // 0x1C == DIAG Version Request
0c 14 3a 7e // 0x0c == Status Request
63 e5 a1 7e // 0x6C == Phone State Request

4.7.5 Reading the CFG file
Open the CFG file in a hex editor.
For HDLC-encoded config
Field Length (in bits) Description
------- ---------------- -----------
Information Variable ICD Packet or Message
Frame Check   6 CRC-CCITT standard 16-bit CRC
Ending Flag   8 Ending character "0x7E"

4b 0f 1a 00 00 bb 60 7e // 0x4B 0x0F 0x001A == Call Manager Subsystem Sys Select (80-V1294-7)
4B 09 00 00 62 B6 7E // 0x4B 0x09 0x0000 == UMTS Subsystem Version Request (80-V2708-1
4b 08 00 00 be ec 7e // 0x4B 0x08 0x0000 == GSM Subsystem Version Request (80-V5295-1)
4b 08 01 00 66 f5 7e // 0x4B 0x0F 0x0001 == GSM Subsystem Status Request (80-V5295-1)
4b 04 00 00 1d 49 7e // 0x4B 0x04 0x0000 == WCDMA Subsystem Version Request (80-V2708-1)
4b 04 0f 00 d5 ca 7e // 0x4B 0x04 0x000F == WCDMA Subsystem Additional Status Request (80-V2708-1)

80-v1294-100_c_mask-related_commands_for_diagnostic_monitoring.pdf
2.6 Extended Message Configuration (125 / 0x7D)
2.4 Logging Configuration (115 / 0x73)
2.2 Event Report Control (96 / 0x60)
2.8 Event Set Mask (130 / 0x82)

C:\ProgramData\QUALCOMM\QXDM\Config\Qualcomm DMC Library\Primary\Default.cfg
*/
#include "qxdm_default_cfg.h"

const uint8_t *g_qcom_req;
static int qcom_need_parse_data = 0;
extern int g_log_file_maxNum;
uint16_t qlog_le16(uint16_t v16)
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

uint32_t qlog_le32(uint32_t v32)
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

uint64_t qlog_le64(uint64_t v64)
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
ssize_t qcom_send_cmd(int fd, const uint8_t *buf, size_t size)
{
    size_t wc = 0;

    //printf("[%s], line: %d\n", __func__, __LINE__);

    while (wc < size) {
        size_t flag = wc;
        const uint8_t *cur = buf + wc;
        unsigned short len = cur[2] + (((unsigned short)cur[3]) << 8) + 5;

        if (cur[0] == 0x7e && cur[1] == 0x01 && (wc + len) <= size && cur[len - 1] == 0x7e) {
            flag += (len - 1);
        }
        else {
            if (flag == 0 && buf[flag] == 0x7E)
                flag++;

            while (buf[flag] != 0x7E && flag < size)
                flag++;
        }

        if (buf[flag] == 0x7E || flag == size) {
#if 0
            size_t nbytes = 0;
            for (nbytes = 0; nbytes < (flag - wc + 1); nbytes++) {
                printf("0x%02X,", buf[wc + nbytes]);
            }
            printf("\n");
#endif
            g_qcom_req = &buf[wc];
            log_poll_write(fd, buf + wc, flag - wc + 1);
            //printf("buf[wc]:%02X, qcom_need_parse_data:%d\n",buf[wc],qcom_need_parse_data);
            //printf("g_qcom_req: %s\n", g_qcom_req);

            if (qcom_need_parse_data) {
                int rx_wait = 1000;

                while (rx_wait-- > 0) {
                    if (g_qcom_req == NULL)
                        break;
                    usleep(2*1000);
                }

                //printf("g_qcom_req:%s\n", g_qcom_req);
                if (g_qcom_req != NULL) {
                    /*start: resolve the bug g_mdm_req issue (mantis 63061), yanghaitao 2020.11.25 */
                    pthread_mutex_lock(&mutex); 
                    g_qcom_req = NULL;
                    pthread_mutex_unlock(&mutex);
                    /*end resolve the bug g_mdm_req issue (mantis 63061), yanghaitao 2020.11.25 */
                    printf("timeout g_qcom_req=%02x\n", buf[wc]);
                }
            }
        }
        else {
            printf("%s unknow qcom cmd\n", __func__);
        }

        wc = flag + 1;
    }

    return size;
}

#define READ_BUF_SIZE   (16*1024)
    
int qcom_init_filter(int fd, const char *log_dir, const char *cfg_name)
{
    uint8_t *rbuf = NULL;
    size_t cfg_size = 0;

    //printf("[%s], line: %d\n", __func__, __LINE__);

#if 1
    if (cfg_name && !strcmp(cfg_name, "dump")) {
        const uint8_t qcom_enter_dump1[] = {
            0x4b, 0x12, 0x18, 0x02, 0x01, 0x00, 0xd2, 0x7e
        };
        const uint8_t qcom_enter_dump2[] = {
            0x7e, 0x01, 0x04, 0x00, 0x4b, 0x25, 0x03, 0x00, 0x7e
        };
        printf("send mdm dump command\n");
        qcom_send_cmd(fd, qcom_enter_dump1, sizeof(qcom_enter_dump1));
        usleep(100*1000);
        qcom_send_cmd(fd, qcom_enter_dump2, sizeof(qcom_enter_dump2));
        return 0;
    }
#endif

    qcom_need_parse_data = 1;
    qcom_create_qshrink4_file(fd, log_dir);

    if (cfg_name) {
        int cfgfd = open(cfg_name, O_RDONLY);
        if (cfgfd >= 0) {
            rbuf = (uint8_t *)malloc(READ_BUF_SIZE);
            if (rbuf == NULL) {
                printf("malloc rbuf failed, errno:%d(%s)\n", errno, strerror(errno));
                qcom_need_parse_data = 0;
                return -1;
            }
            
            cfg_size = read(cfgfd, rbuf, READ_BUF_SIZE);
            close(cfgfd);
        }
    }
    
    if (cfg_size > 0) {
        qcom_send_cmd(fd, rbuf, cfg_size);
    }
    else {
        printf("use default config data in the qxdm_default_cfg.h\n");
        qcom_send_cmd(fd, (uint8_t *)qxdm_default_cfg_buf, sizeof(qxdm_default_cfg_buf));
    }
  
    free(rbuf);

    qcom_need_parse_data = 0;
    return 0;
}

int qcom_clean_filter(int fd)
{
    printf("[%s], line: %d, fd %d\n", __func__, __LINE__, fd);
    qcom_send_empty_mask();
    return 0;
}

static int qcom_miss_qmdlv2_logfd = -1;
int qcom_logfile_init(int logfd, unsigned logfile_seq)
{
    int ret = -1;

    printf("[%s], line: %d, logfile %d\n", __func__, __LINE__, logfile_seq);

    if (!qmdl2_v2_mode)
        return 0;

    if (qlog_le32(qshrink4_data.header_length) == 0) {
        qcom_miss_qmdlv2_logfd = logfd;
        return 0;
    }

    qcom_miss_qmdlv2_logfd = -1;
    ret = write(logfd, &qshrink4_data, qlog_le32(qshrink4_data.header_length));
    /* printf("%s write %d\n", __func__, ret); */

    return ret;
}

size_t qcom_logfile_save(int logfd, const void *buf, size_t size)
{
    int ret = -1;

    //printf("[%s], line: %d\n", __func__, __LINE__);

    if (qcom_need_parse_data) {
        qcom_parse_data_for_command_rsp(buf, size);
    }

    if (qmdl2_v2_mode) {
        if (qlog_le32(qshrink4_data.header_length) == 0) {
            return size;
        }
        else if (qcom_miss_qmdlv2_logfd == logfd) {
            qcom_miss_qmdlv2_logfd = -1;
            ret = write(logfd, &qshrink4_data, qlog_le32(qshrink4_data.header_length));
            /* printf("%s write %d\n", __func__, ret); */
        }
    }
    
    ret = write(logfd, buf, size);

    return ret;
}

static int qlog_create_logfile(const char *log_dir, const char *logfile_suffix, unsigned logfile_seq)
{
    int logfd = -1;
    char filename[NAME_BUF_SIZE] = {0};
    struct tm *tm;
    time_t t;

    printf("[%s], line: %d, logfile_suffix %s, logfile_seq %d\n", __func__, __LINE__, logfile_suffix, logfile_seq);

    t = time(NULL);
    tm = localtime(&t);
    sprintf(g_qxdm_log_filename, "%02d%02d%02d%02d%02d%02d.qmdl2", tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday,
                    tm->tm_hour, tm->tm_min, tm->tm_sec);
    sprintf(filename, "%s%s", log_dir, g_qxdm_log_filename);
    logfd = open(filename, O_CREAT | O_WRONLY | O_TRUNC, 0777);
    if (logfd < 0)
    {
        printf("[%s] %s failed. errno:%d(%s)\n", __func__, filename, errno, strerror(errno));
        return -1;
    }
    log_storage_control(filename, g_log_file_maxNum, 1);
    printf("[%s] %s OK.\n", __func__, filename);

    return logfd;
}

qlog_ops_t qcom_qlog_ops = 
{
    .init_filter = qcom_init_filter,
    .clean_filter = qcom_clean_filter,
    .logfile_init = qcom_logfile_init,
    .logfile_create = qlog_create_logfile,
    .logfile_save = qcom_logfile_save,
    .logfile_close = close,
};

void file_log_message(char *log_dir, const char *str_fmt,...)
{
    char filename[512] = {0};
    FILE *fp = NULL;
    
    snprintf(filename, sizeof(filename), "%s/%s", log_dir, LOG_FILE_NAME);
    
    fp = fopen(filename, "a+t");
    if (fp)
    {
        char log_trace[2048] = {0};
        va_list ap;
        struct tm *tm;
        time_t t;

        va_start(ap, str_fmt);
        vsnprintf(log_trace, sizeof(log_trace), str_fmt, ap);
        va_end(ap);

        t = time(NULL);
        tm = localtime(&t);
        fprintf(fp, "[%02d/%02d/%02d/ %02d:%02d:%02d]: %s\n", tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday,
                        tm->tm_hour, tm->tm_min, tm->tm_sec, log_trace);
        fclose(fp);
    }
}