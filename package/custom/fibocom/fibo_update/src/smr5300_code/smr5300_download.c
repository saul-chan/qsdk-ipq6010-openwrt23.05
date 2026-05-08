#include <stdio.h>
#include <string.h>
#include "smr5300_download.h"
#include "smr5300_pac.h"
#include <sys/utsname.h>
#include <stdbool.h>
#define smr_exec(_c)                                                                 \
    do                                                                               \
    {                                                                                \
        if (_c)                                                                      \
        {                                                                            \
            LogInfo("failed: %s (%s: %s: %d)\n", #_c, __FILE__, __func__, __LINE__); \
            return -1;                                                               \
        }                                                                            \
    } while (0)
#define REPLACE_SLOT "prefix"
static uint32_t frame_sz = 0;
static char backup_dir[64] = "/tmp";
char dl_filename[MAX_PATH_LEN*2] = {0};

int g_samsung_fastboot_type = SS_FASTBOOT_TYPE_USB;
char current_slot[2] = "b";

static int timeval_subtract(struct timeval *result, struct timeval *x, struct timeval *y)
{
    // Perform the carry for the later subtraction by updating y.
    if (x->tv_usec < y->tv_usec)
    {
        int nsec = (y->tv_usec - x->tv_usec) / 1000000 + 1;
        y->tv_usec -= 1000000 * nsec;
        y->tv_sec += nsec;
    }

    if (x->tv_usec - y->tv_usec > 1000000)
    {
        int nsec = (x->tv_usec - y->tv_usec) / 1000000;
        y->tv_usec += 1000000 * nsec;
        y->tv_sec -= nsec;
    }

    // Compute the time remaining to wait. tv_usec is certainly positive. */
    result->tv_sec = x->tv_sec - y->tv_sec;
    result->tv_usec = x->tv_usec - y->tv_usec;

    // Return 1 if result is negative.
    return x->tv_sec < y->tv_sec;
}

unsigned int g_record = 0;
int do_progressbar(unsigned int count, int max)
{
    const char bar = '=';
    unsigned int i;
    unsigned int tick = 5; // means 5%
    unsigned int percent = ((unsigned long long)count * 100) / max;
    unsigned int bar_count = percent / tick;

    if ((count > max) || (max <= 0))
    {
        LogInfo("count=%d max=%d is not correct!!\n", count, max);
        return -1;
    }

    if (percent == g_record)
    {
        return 0;
    }

    g_record = percent;

    printf("\r%d/%d [", count, max);
    for (i = 0; i < bar_count; i++)
    {
        printf("%c", bar);
    }

    printf("] %d%%", percent);

    if (percent == 100)
    {
        printf(" [done!]\n");
        g_record = 0;
    }

    return 0;
}

static int dl_read_data(file_ctx_t *ctx, char *buffer, int size)
{
    int ret;

    ret = ctx->pdev->read(ctx->pdev, buffer, size, 0, 6000);
    return ret;
}

static int dl_send_data(file_ctx_t *ctx, char *buffer, int size)
{
    int ret;

    ret = ctx->pdev->write(ctx->pdev, buffer, size, 0, 6000);
    return ret;
}

static int dl_send_cmd(file_ctx_t *ctx, char *pdl_cmd)
{
    int ret;

    ret = ctx->pdev->write(ctx->pdev, pdl_cmd, strlen(pdl_cmd), 0, 3000);
    return ret;
}

static int dl_read_cmd(file_ctx_t *ctx, unsigned timeout)
{
    int ret = 0;

    memset(ctx->file_rsp_data, 0, 256);
    ret = ctx->pdev->read(ctx->pdev, ctx->file_rsp_data, 256, 0, timeout ? timeout : 15000);
    return ret;
}

int dl_send_cmd_wait_ack(file_ctx_t *ctx, char *pdl_req, uint16_t pdl_cmd, unsigned timeout)
{
    int ret;
    struct timeval time_start, time_end, time_result;

    gettimeofday(&time_start, NULL);
    ret = dl_send_cmd(ctx, pdl_req);
    if (ret <= 0)
        return -1;

    ret = dl_read_cmd(ctx, timeout);
    if (ret == 0)
        return -1;
    gettimeofday(&time_end, NULL);

    timeval_subtract(&time_result, &time_end, &time_start);

    LogInfo("'%s' -->> '%s' (in %ld.%06ld seconds)\n", pdl_req, g_samsung_fastboot_type == SS_FASTBOOT_TYPE_NET ? (char *)ctx->file_rsp_data + 8 : (char *)ctx->file_rsp_data, time_result.tv_sec, time_result.tv_usec);
    return 0;
}

int smr_read_bin(file_ctx_t *ctx, size_t nSize, size_t file_size)
{
    FILE *fp = ctx->fp;
    int ret;
    int fix_size = 0;

    if ((ctx->cur_bin_offset + nSize) > file_size)
        nSize = file_size - ctx->cur_bin_offset;

    if (nSize == 0)
        return 0;

    if ((ctx->cur_bin_offset == 0) && (g_samsung_fastboot_type == SS_FASTBOOT_TYPE_NET))
    {
        sprintf((char *)ctx->file_req_data, "%08ld", file_size);
        ret = fread(ctx->file_req_data + 8, 1, nSize, fp);
        fix_size = 8;
    }
    else
    {
        ret = fread(ctx->file_req_data, 1, nSize, fp);
    }

    if (ret > 0)
    {
        ctx->cur_bin_offset += ret;
        ret += fix_size;
    }

    return ret;
}

static int smr_reboot(file_ctx_t *ctx, Scheme_t *File_scheme)
{
    char pdl_req[512] = {
        0,
    };
    char strtmp[256] = {
        0,
    };

    LogInfo("%-8s \n", "reboot");

    if (g_samsung_fastboot_type == SS_FASTBOOT_TYPE_NET)
    {
        sprintf(strtmp, "reboot");
        sprintf(pdl_req, "%08ld%s", strlen(strtmp), strtmp);
    }
    else
    {
        sprintf(pdl_req, "reboot");
    }
    smr_exec(dl_send_cmd_wait_ack(ctx, pdl_req, 0, 200000));

    return 0;
}

static int smr_getvar(file_ctx_t *ctx, Scheme_t *File_scheme)
{
    char pdl_req[256];
    char strtmp[256 - 8] = {
        0,
    };
    int offset = 0;

    LogInfo("%-8s \n", "getvar");

    if (g_samsung_fastboot_type == SS_FASTBOOT_TYPE_NET)
    {
        sprintf(strtmp, "getvar:%s", File_scheme->partition_name);
        sprintf(pdl_req, "%08ld%s", strlen(strtmp), strtmp);
        offset = 8;
    }
    else
    {
        sprintf(pdl_req, "getvar:%s", File_scheme->partition_name);
    }
    smr_exec(dl_send_cmd_wait_ack(ctx, pdl_req, 0, 200000));
    if (!strncmp((char *)ctx->file_rsp_data + offset, "OKAY", strlen("OKAY") - 1))
    {
        if (!strcmp((char *)ctx->file_rsp_data + offset + strlen("OKAY") - 1, "B"))
            strncpy(current_slot, "a", sizeof(current_slot));
    }
    return 0;
}

static int smr_setactive(file_ctx_t *ctx, Scheme_t *File_scheme)
{
    char pdl_req[256];
    char strtmp[256 - 8] = {
        0,
    };

    LogInfo("%-8s \n", "set_active");

    if (g_samsung_fastboot_type == SS_FASTBOOT_TYPE_NET)
    {
        sprintf(strtmp, "set_active:%s", current_slot);
        sprintf(pdl_req, "%08ld%s", strlen(strtmp), strtmp);
    }
    else
    {
        sprintf(pdl_req, "set_active:%s", current_slot);
    }
    smr_exec(dl_send_cmd_wait_ack(ctx, pdl_req, 0, 200000));
    return 0;
}

static int smr_oem_update_ap_nv(file_ctx_t *ctx, Scheme_t *File_scheme)
{
    char pdl_req[256];
    char strtmp[256 - 8] = {
        0,
    };

    LogInfo("%s \n", "oem update_ap_nv");
    if (g_samsung_fastboot_type == SS_FASTBOOT_TYPE_NET)
    {
        sprintf(strtmp, "oem update_ap_nv");
        sprintf(pdl_req, "%08ld%s", strlen(strtmp), strtmp);
    }
    else
    {
        sprintf(pdl_req, "oem update_ap_nv");
    }
    smr_exec(dl_send_cmd_wait_ack(ctx, pdl_req, 0, 200000));
    return 0;
}

static int smr_erase_partition(file_ctx_t *ctx, Scheme_t *File_scheme)
{
    char pdl_req[512] = {
        0,
    };
    char strtmp[256] = {
        0,
    };

    LogInfo("%-8s %s\n", "erase", File_scheme->partition_name);

    if (g_samsung_fastboot_type == SS_FASTBOOT_TYPE_NET)
    {
        sprintf(strtmp, "erase:%s", File_scheme->partition_name);
        sprintf(pdl_req, "%08ld%s", strlen(strtmp), strtmp);
    }
    else
    {
        sprintf(pdl_req, "erase:%s", File_scheme->partition_name);
    }
    smr_exec(dl_send_cmd_wait_ack(ctx, pdl_req, 0, 200000));

    return 0;
}

static int smr_fetch_partition(file_ctx_t *ctx, Scheme_t *File_scheme)
{
    char backup_file[256];
    char pdl_req[512] = {
        0,
    };
    char strtmp[256] = {
        0,
    };
    char *rsp;
    char *token;
    int size;
    int ret;
    int remain_size;
    int read_size;

    if (access(backup_dir, W_OK) && errno == ENOENT)
    {
        snprintf(backup_dir, sizeof(backup_dir), "data");
    }

    if (access(backup_dir, W_OK) && errno == ENOENT)
    {
        snprintf(backup_dir, sizeof(backup_dir), "cache");
    }

    snprintf(backup_file, sizeof(backup_file), "%s/smr_bak_%s", backup_dir, File_scheme->partition_name);
    LogInfo("%-8s %s -> '%s'\n", "fetch", File_scheme->partition_name, backup_file);

    if (g_samsung_fastboot_type == SS_FASTBOOT_TYPE_NET)
    {
        sprintf(strtmp, "getvar:partition-size:%s", File_scheme->partition_name);
        sprintf(pdl_req, "%08ld%s", strlen(strtmp), strtmp);
    }
    else
    {
        sprintf(pdl_req, "getvar:partition-size:%s", File_scheme->partition_name);
    }
    smr_exec(dl_send_cmd_wait_ack(ctx, pdl_req, 0, 1000));

    rsp = (char *)ctx->file_rsp_data;
    if (g_samsung_fastboot_type == SS_FASTBOOT_TYPE_NET)
    {
        rsp = (char *)ctx->file_rsp_data + 8;
    }

    if (strstr(rsp, "OKAY") == NULL)
    {
        LogInfo("cmd='%s' fail!!\n", pdl_req);
        return -1;
    }

    token = strtok((char *)rsp, "OKAY");
    size = strtol(token, 0, 16);
    LogInfo("paritition %s : size %s   -> '%x'\n", File_scheme->partition_name, token, size);

    if (g_samsung_fastboot_type == SS_FASTBOOT_TYPE_NET)
    {
        sprintf(strtmp, "fetch:%s:0:0x%08x", File_scheme->partition_name, size);
        sprintf(pdl_req, "%08ld%s", strlen(strtmp), strtmp);
    }
    else
    {
        sprintf(pdl_req, "fetch:%s:0:0x%08x", File_scheme->partition_name, size);
    }
    smr_exec(dl_send_cmd_wait_ack(ctx, pdl_req, 0, 1000));

    ctx->fp = fopen(backup_file, "wb");
    if (!ctx->fp)
    {
        LogInfo("fopen (%s) failed, errno: %d (%s)\n", backup_file, errno, strerror(errno));
        return -1;
    }

    LogInfo("Start recieve '%s'\n", backup_file);

    ctx->cur_bin_offset = 0;
    remain_size = size;
    while (remain_size > 0)
    {
        read_size = min(remain_size, frame_sz);
        ret = dl_read_data(ctx, (char *)ctx->file_req_data, read_size);
        if (fwrite(ctx->file_req_data, 1, ret, ctx->fp) != ret)
        {
            LogInfo("faill to save %d bytes\n", ret);
            fclose(ctx->fp);
            return -1;
        }

        remain_size -= ret;
        do_progressbar(size - remain_size, size);
    }

    fclose(ctx->fp);
    return 0;
}

void replace_prefix(char *str, const char *prefix, size_t n)
{
    char temp[100];
    sprintf(temp, "%s%s", prefix, str + n);
    strcpy(str, temp);
}

#define MAX_BUFFER_SIZE 128 * 1024 * 1024
static int smr_flash_partition(file_ctx_t *ctx, Scheme_t *File_scheme, char *package_dir)
{
    int ret;
    size_t remain_size;
    char pdl_req[512] = {
        0,
    };
    char strtmp[256] = {
        0,
    };
    struct stat st;
    int read_size;
    int offset;
    struct timeval time_start, time_end, time_result;

    gettimeofday(&time_start, NULL);

    if (File_scheme->file_path[0] == ' ')
    {
        snprintf(File_scheme->file_path, sizeof(File_scheme->file_path), "%s/smr_bak_%s", backup_dir, File_scheme->partition_name);
    } 
    else
    {   
        char tmp[1024] = {0};
        memset(tmp, 0, sizeof(tmp));
        snprintf(tmp, sizeof(tmp), "%s/%s",package_dir, File_scheme->file_path);
        memset(File_scheme->file_path, 0, sizeof(File_scheme->file_path));
        strncpy(File_scheme->file_path, tmp,sizeof(File_scheme->file_path));
    }

    if (!strncmp(File_scheme->partition_name, REPLACE_SLOT, strlen(REPLACE_SLOT) - 1))
    {
        replace_prefix(File_scheme->partition_name, current_slot, strlen(REPLACE_SLOT));
    }
    LogInfo("------->> smr_flash_partition %s size start ...\n",File_scheme->file_path);
    ret = stat(File_scheme->file_path, &st);
    if (ret != 0)
    {
        if (!strcmp(File_scheme->partition_name, "ap_nv_update"))
        {
            LogInfo("There's no  nv update data.");
            return 0;
        }

        perror("stat");
        return -1;
    }

    LogInfo("------->> Flash %s size %lx start ...\n", File_scheme->file_path, st.st_size);
    /* erase before write in image*/
    if (g_samsung_fastboot_type == SS_FASTBOOT_TYPE_NET)
    {
        sprintf(strtmp, "erase:%s", File_scheme->partition_name);
        sprintf(pdl_req, "%08ld%s", strlen(strtmp), strtmp);
    }
    else
    {
        sprintf(pdl_req, "erase:%s", File_scheme->partition_name);
    }
    smr_exec(dl_send_cmd_wait_ack(ctx, pdl_req, 0, 200000));

    ctx->fp = fopen(File_scheme->file_path, "rb");
    if (!ctx->fp)
    {
        LogInfo("fopen (%s) failed, errno: %d (%s)\n", File_scheme->file_path, errno, strerror(errno));
        return -1;
    }

    remain_size = st.st_size;
    offset = 0;
    while (remain_size > 0)
    {
        read_size = min(remain_size, MAX_BUFFER_SIZE);
        ctx->fp = ctx->fp;
        ctx->cur_bin_offset = 0;

        if (g_samsung_fastboot_type == SS_FASTBOOT_TYPE_NET)
        {
            sprintf(strtmp, "download:0x%08X", read_size);
            sprintf(pdl_req, "%08ld%s", strlen(strtmp), strtmp);
        }
        else
        {
            sprintf(pdl_req, "download:0x%08X", read_size);
        }
        smr_exec(dl_send_cmd_wait_ack(ctx, pdl_req, 0, 1000));

        while (ctx->cur_bin_offset < read_size)
        {
            ret = smr_read_bin(ctx, frame_sz, read_size);
            smr_exec(ret <= 0);
            dl_send_data(ctx, (char *)ctx->file_req_data, ret);
            do_progressbar(ctx->cur_bin_offset, remain_size);
        }

        if (!dl_read_cmd(ctx, 3000))
        {
            LogInfo("------->> !!! Flash %s fail no respoen !!!\n", File_scheme->file_path);
            return -1;
        }

        if (g_samsung_fastboot_type == SS_FASTBOOT_TYPE_NET)
        {
            sprintf(strtmp, "offsetflash:%s:0x%08x", File_scheme->partition_name, offset);
            sprintf(pdl_req, "%08ld%s", strlen(strtmp), strtmp);
        }
        else
        {
            sprintf(pdl_req, "offsetflash:%s:0x%08x", File_scheme->partition_name, offset);
        }
        smr_exec(dl_send_cmd_wait_ack(ctx, pdl_req, 0, 600000));

        remain_size -= read_size;
        offset += read_size;
    }
    fclose(ctx->fp);

    if (!strcmp(File_scheme->partition_name, "ap_nv_update"))
        smr_oem_update_ap_nv(ctx, File_scheme);

    gettimeofday(&time_end, NULL);

    timeval_subtract(&time_result, &time_end, &time_start);

    LogInfo("<<------- Flash %s size %x done in %ld.%06ld seconds\n", File_scheme->file_path, offset,
            time_result.tv_sec, time_result.tv_usec);

    return 0;
}

#define ITEM_NUMBER 64
Scheme_t smr_scheme[ITEM_NUMBER];
Edl_t edl_scheme[ITEM_NUMBER];

#define MAX_LINE_LENGTH 1024
#define MAX_WORD_LENGTH 128
#define DELIMITER ","

int parse_txt(void)
{
    FILE *file = fopen(dl_filename, "r");
    char line[MAX_LINE_LENGTH];
    int i = 0;
    int j = 0;

    memset(smr_scheme, 0, sizeof(Scheme_t) * ITEM_NUMBER);
    if (file == NULL)
    {
        LogInfo("Error opening file %s\n", dl_filename);
        return -1;
    }
    while (fgets(line, sizeof(line), file) != NULL)
    {
        char *token = strtok(line, DELIMITER);
        j = 0;
        while (token != NULL)
        {
            if (j == 0)
                strncpy(smr_scheme[i].FileID, token, sizeof(smr_scheme[i].FileID));
            if (j == 1)
                strncpy(smr_scheme[i].partition_name, token, sizeof(smr_scheme[i].partition_name));
            if (j == 2)
            {
                strncpy(smr_scheme[i].file_path, token, sizeof(smr_scheme[i].file_path));
            }
            token = strtok(NULL, DELIMITER);
            j++;
        }
        i++;
    }
    fclose(file);
    return 0;
}

static int dloader_smr(file_ctx_t *ctx, char *package_dir)
{
    int i;
    int ret = 0;
    ret = parse_txt();
    if (ret == 0)
    {
        for (i = 0; i < sizeof(smr_scheme) / sizeof(smr_scheme[0]); i++)
        {
            Scheme_t *x = &smr_scheme[i];
            frame_sz = FRAMESZ_DATA_SMR;
            if (!strcasecmp(x->FileID, "flash"))
            {
                smr_exec(smr_flash_partition(ctx, x, package_dir));
            }
            if (!strcasecmp(x->FileID, "fetch"))
            {
                smr_exec(smr_fetch_partition(ctx, x));
            }
            if (!strcasecmp(x->FileID, "erase"))
            {
                smr_exec(smr_erase_partition(ctx, x));
            }
            if (!strcasecmp(x->FileID, "reboot"))
            {
                smr_exec(smr_reboot(ctx, x));
            }

            if (!strcasecmp(x->FileID, "getvar"))
            {
                smr_exec(smr_getvar(ctx, x));
            }
            if (!strcasecmp(x->FileID, "set_active"))
            {
                smr_exec(smr_setactive(ctx, x));
            }
        }
    }
    return ret;
}

static file_ctx_t g_pac_ctx;
int smr_dload(fibo_usbdev_t *pdev, char *package_dir)
{
    file_ctx_t *ctx = &g_pac_ctx;
    int ret = -1;

    memset(ctx, 0, sizeof(file_ctx_t));
    ctx->pdev = pdev;

    if (strstr(pdev->ModuleName, "SMR5300 DL"))
    {
        ctx->platform = SMR5300;
    }
    else
    {
        LogInfo("%s is not supported\n", pdev->ModuleName);
        return ret;
    }
    LogInfo("ctx->platform:%d\n", ctx->platform);
    ret = dloader_smr(ctx, package_dir);
    return ret;
}
