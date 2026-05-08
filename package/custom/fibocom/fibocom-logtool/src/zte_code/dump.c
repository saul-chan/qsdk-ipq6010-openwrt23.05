// linux系统参考代码如下，注意默认导出路径DEFAULT_RAMDUMP_FOLD配置需要修改。

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <getopt.h>
#include <stdarg.h>
#include <termios.h>
#include <stddef.h>
#include <dirent.h>
#include <unistd.h>
// #include<properties.h>
// #include<cutils/properties.h>
#define LOG_TAG "Modem_Ramdump"
// #include<cutils/log.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/select.h>
#include "zte_devices_list.h"
/******************************************************************************
 *                                    数据结构定义
 ******************************************************************************/
typedef struct
{
    char file_name[32];
    unsigned int file_size;
} file_info_t;

/******************************************************************************
 *                                    全局变量和宏定义
 ******************************************************************************/
typedef unsigned int UINT32;
#define MODEM_TRAP_PATH "/dev/ttyUSB0"
#define MODEM_TRAP_PATH2 "/dev/ttyUSB1"
// #define MODEM_RAMDUMP_PATH "/data/local/log/Ramdump"
#define RAMDUMP_DEFAULT_BAUD B115200
#define RAMDUMP_DEFAULT_DELAY 10000
static int g_modem_fd = -1;

/*Ramdump 指令定义*/
#define DUMPFILE_LINK_REQ       (UINT32)1     // 同步请求
#define DUMPFILE_LINK_RSP       (UINT32)2     // 同步请求应答，附带ramdump文件数目
#define DUMPFILE_FILE_REQ       (UINT32)3     // 请求指定编号文件信息
#define DUMPFILE_FILE_RSP       (UINT32)4     // 文件编号文件信息应答，附带传输文件名及大小
#define DUMPFILE_READ_REQ       (UINT32)5     // 请求读取指定编号文件内容
#define DUMPFILE_READ_RSP       (UINT32)6     // 文件内容读取应答，附带文件内容
#define DUMPFILE_END_REQ        (UINT32)7     // 传输结束
#define DUMPFILE_END_RSP        (UINT32)8     // 传输结束应答
#define DUMPFILE_CMD_FAIL       (UINT32)9     // 指令错去
#define DUMPFILE_NO_FAIL        (UINT32)10    // 文件编号错误
#define DUMPFILE_LENGTH_FAIL    (UINT32)11    // 文件位置大小错误
#define CMD_BUFFER_LEN          (UINT32)16    // 指令擦防毒
#define FILENAME_LEN            (UINT32)32    // 文件名长度
#define FILENAME_MAX_LEN        (UINT32)256   // 文件名最大长度
// #define DATA_BLOCK_SIZE      (0x40000)     // 数据缓冲大小
#define DATA_BLOCK_SIZE         (0x00800)     // 数据缓冲大小

// #define MIN(a, b) ((a) < (b) ? (a) : (b))
#define DEFAULT_RAMDUMP_FOLD "./ramdump_file/"

#define ZTE_LOG_PATH "./zte_file"
#define CPLOG_PATH "persist.service.ztelog.path"
// #define CPLOG_TIMESTAMP_DIR  "persist.service.ztelog.timedir"
#define __FUNCTION__ "ramdumpfuc"
int PROPERTY_VALUE_MAX = FILENAME_MAX_LEN;

static int g_current_file_index = 0;
/******************************************************************************
 *                                    函数原型:
 ******************************************************************************/

/******************************************************************************
 *                                    本地函数实现
 ******************************************************************************/
static void mdp_print_array(const char *prefix, const char *buf, int length)
{
#if 0
    int i =0;
    int len = MIN(length,16 );
    LogInfo("%s ", prefix );
    for (i=0; i< len; i++) {
        LogInfo("%02X ", buf[i]);
    }
    if (length > len)
        LogInfo("...");
    LogInfo("\n");
#endif
}
static int tty_write(int fd, const char *buf, int size)
{
    int ret = 0;
    int repeat_count = 0;
WRITE:
    ret = write(fd, buf, size);
    if (0 == ret)
    {
        LogInfo("%s error: %s\n", __FUNCTION__, strerror(errno));
        repeat_count += 1;
        if (3 > repeat_count)
        {
            sleep(1);
            goto WRITE;
        }
    }
    if (ret != size)
    {
        LogInfo("%s failed, size=%d, ret=%d\n", __FUNCTION__, size, ret);
        return -1;
    }
    return 0;
}

static int tty_read(int fd, char *buf, int size, unsigned int delay_ms)
{
    int ret = -1;
    int read_count = 0;
    fd_set fds;
    int repeat_count = 0;
    struct timeval tv;
    
    if (buf == NULL)
    {
        return -1;
    }
        
    tv.tv_sec = delay_ms / 1000;
    tv.tv_usec = (delay_ms % 1000) * 1000;
    FD_ZERO(&fds);
    FD_SET(fd, &fds);
READ:
    ret = select(fd + 1, &fds, NULL, NULL, &tv);
    if (ret > 0)
    {
        read_count = read(fd, buf, size);
        if (read_count <= 0)
        {
            LogInfo("%s read failed for ret=%d\n", __FUNCTION__, read_count);
            return -1;
        }
        return read_count;
    }
    else if (ret == 0)
    {
        LogInfo("%s select time out %dms\n", __FUNCTION__, delay_ms);
        LogInfo("%s timeout error: %s\n", __FUNCTION__, strerror(errno));
        if (3 > repeat_count)
        {
            repeat_count += 1;
            sleep(1);
            goto READ;
        }
    }
    else
    {
        LogInfo("%s select failed %s\n", __FUNCTION__, strerror(errno));
    }
    return -1;
}

static int mdp_send(const char *buf, int size)
{
    mdp_print_array(__FUNCTION__, buf, size);
    return tty_write(g_modem_fd, buf, size);
}

static int mdp_receive(char *buf, int size)
{
    int count = 0;
    int length = size;
    char *pbuffer = buf;
    while (length > 0)
    {
        count = tty_read(g_modem_fd, pbuffer, size, RAMDUMP_DEFAULT_DELAY);
        if (count < 0)
        {
            return -1;
        }
        pbuffer += count;
        length -= count;
    }
    mdp_print_array(__FUNCTION__, buf, size);
    return size;
}

static int mdp_send_command(unsigned int cmd, unsigned int argc, ...)
{
    char buffer[CMD_BUFFER_LEN] = {0};
    unsigned int i = 0;
    unsigned int arg = 0;
    UINT32 *pbuffer = (UINT32 *)buffer;
    *pbuffer = cmd;
    va_list ap;

    va_start(ap, argc);
    for (i = 0; i < argc; i++)
    {
        arg = va_arg(ap, unsigned int);
        *(++pbuffer) = arg;
    }
    va_end(ap);
    return mdp_send(buffer, CMD_BUFFER_LEN);
}
static int mdp_receive_ack(unsigned int ack)
{
    int ret = 0;
    unsigned int resp;
    char buffer[64] = {0};

    ret = mdp_receive((char *)&resp, sizeof(unsigned int));
    //    ret = mdp_receive(buffer, 64);
    if (ret > 0)
    {
        //        resp = *(unsigned int*)buffer;
        if (ack == resp)
        {
            return 0;
        }  
    }
    return -1;
}
static int init_devices(char *dev_path, speed_t speed)
{
    int fd = -1;
    struct termios tios;
    LogInfo("%s\n", __FUNCTION__);
    fd = open(dev_path, O_RDWR);
    if (fd < 0)
    {
        LogInfo("Can't open %s(%s)\n", dev_path, strerror(errno));
        return -1;
    }
    if (tcgetattr(fd, &tios) < 0)
    {
        LogInfo(" tcgetattr failed(%s)\n", strerror(errno));
        return -1;
    }
    tios.c_cflag = CS8 | CREAD | CRTSCTS | CLOCAL;
    tios.c_iflag = IGNPAR;
    tios.c_oflag = 0;
    tios.c_lflag = 0;
    tios.c_cc[VTIME] = 0;
    tios.c_cc[VMIN] = 1;

    cfsetispeed(&tios, RAMDUMP_DEFAULT_BAUD);
    cfsetospeed(&tios, RAMDUMP_DEFAULT_BAUD);
    tcflush(fd, TCIFLUSH);

    if (tcsetattr(fd, TCSANOW, &tios) < 0)
    {
        LogInfo(" tcgetattr failed(%s)\n", strerror(errno));
        return -1;
    }
    return fd;
}

static int create_fold(char *fold)
{
    char buffer[FILENAME_MAX_LEN] = {0};
    snprintf(buffer, FILENAME_MAX_LEN, "mkdir -p %s\n", fold);
    int ret = system(buffer);
    if (ret < 0)
    {
        return -1;
    }
        
    return 0;
}
static int create_file(char *fold, char *path)
{
    int fd = -1;
    DIR *pdir = NULL;
    char file_name[FILENAME_MAX_LEN] = {0};
    int ret = 0;
    if ((fold == NULL) || (*fold == '\0'))
    {
        fold = DEFAULT_RAMDUMP_FOLD;
    }
        
    if ((path == NULL) || (*path == '\0'))
    {
        LogInfo("%s path=NULL\n", __FUNCTION__);
        return -1;
    }
    if ((pdir = opendir(fold)) == NULL)
    {
        ret = create_fold(fold);
        if (ret < 0)
        {
            LogInfo("%s create fold %s failed (%s)", __FUNCTION__, fold, strerror(errno));
            return -1;
        }
    }
    if (pdir != NULL)
    {
        closedir(pdir);
    }
        
    snprintf(file_name, FILENAME_MAX_LEN, "%s/%s", fold, path);
    unlink(file_name);
    LogInfo("%s %s\n", __FUNCTION__, file_name);
    fd = open(file_name, O_CREAT | O_RDWR, 0777);
    if (fd < 0)
    {
        LogInfo("failed to create %s (%s)\n", path, strerror(errno));
    }
    return fd;
}
static int write_to_file(int fd, char *buffer, int size)
{
    int ret = 0;
    if ((fd < 0) || (buffer == NULL) || (size <= 0))
    {
        return -1;
    }
    ret = write(fd, buffer, size);
    if (ret < size)
    {
        LogInfo("write to file failed, ret=%d, size=%d\n", ret, size);
        return -1;
    }
    return 0;
}
static int dump_file(int index, char *fold)
{
    int ret = 0;
    char path[FILENAME_MAX_LEN] = {0};
    char cmd_buffer[CMD_BUFFER_LEN] = {0};
    file_info_t file_info = {{0}, 0};
    char data_buffer[DATA_BLOCK_SIZE] = {0};
    int fd = 0;
    int file_size, read_count, file_offset;

    if ((ret = mdp_send_command(DUMPFILE_FILE_REQ, 1, index)) < 0)
    {
        LogInfo("%s failed to send command DUMPFILE_FILE_REQ\n", __FUNCTION__);
        return -1;
    }
    if ((ret = mdp_receive_ack(DUMPFILE_FILE_RSP)) < 0)
    {
        LogInfo("%s failed to receive DUMPFILE_FILE_RSP\n", __FUNCTION__);
        return -1;
    }
    if ((ret = mdp_receive((char *)&file_info, sizeof(file_info))) < 0)
    {
        LogInfo("%s failed to get fileinfo\n", __FUNCTION__);
        return -1;
    }
    if ((fd = create_file(fold, file_info.file_name)) < 0)
    {
        LogInfo("failed to create file %s\n", file_info.file_name);
        return -1;
    }
    LogInfo("filename=%s\t size=%d\n", file_info.file_name, file_info.file_size);
    file_size = file_info.file_size;
    file_offset = read_count = 0;
    while (file_size > 0)
    {
        read_count = MIN(file_size, DATA_BLOCK_SIZE);
        if (mdp_send_command(DUMPFILE_READ_REQ, 3, index, file_offset, read_count) < 0)
        {
            LogInfo("%s failed to send DUMPFILE_READ_REQ\n", __FUNCTION__);
            ret = -1;
            goto exit;
        }
        if (mdp_receive_ack(DUMPFILE_READ_RSP) < 0)
        {
            LogInfo("%s failed to receive ack DUMPFILE_READ_RSP\n", __FUNCTION__);
            ret = -1;
            goto exit;
        }
        if (mdp_receive(data_buffer, read_count) < 0)
        {
            LogInfo("failed to read file data\n");
            ret = -1;
            goto exit;
        }
        if (write_to_file(fd, data_buffer, read_count) < 0)
        {
            LogInfo("failed to write file data\n");
            ret = -1;
            goto exit;
        }
        file_offset += read_count;
        file_size -= read_count;
    }
    ret = 0;
exit:
    close(fd);
    return ret;
    ;
}
static int do_modem_ramdump(char *tty, char *path)
{
    int ret = -1;
    int file_number = 0;
    int i = 0;

    g_modem_fd = init_devices(tty, RAMDUMP_DEFAULT_BAUD);
    if (g_modem_fd < 0)
    {
        LogInfo("failed to open %s\n", tty);
        return -1;
    }
    if ((ret = mdp_send_command(DUMPFILE_LINK_REQ, 0)) < 0)
    { // needed
        LogInfo("Send DUMPFILE_LINK_REQ failed\n");
        ret = -1;
        goto exit;
    }
    if ((ret = mdp_receive_ack(DUMPFILE_LINK_RSP)) < 0)
    {
        LogInfo("failed to receive DUMPFILE_LINK_RSP\n");
        ret = -1;
        goto exit;
    }
    ret = mdp_receive((char *)&file_number, sizeof(unsigned int));
    if (ret < 0)
    {
        LogInfo("failed to get filenumber\n");
        ret = -1;
        goto exit;
    }
    LogInfo("file_number = %d\n", file_number);
    for (i = 0; i < file_number; i++)
    {
        LogInfo("dump file index=%d ...\n", i);
        ret = dump_file(i, path);
        if (ret < 0)
        {
            LogInfo("dump file index=%d failed\n", i);
            ret = -1;
            goto exit;
        }
        LogInfo("dump file index=%d success\n", i);
    }
    if ((ret = mdp_send_command(DUMPFILE_END_REQ, 0)) < 0)
    {
        LogInfo("failed to send DUMPFILE_END_REQ\n");
        ret = -1;
        goto exit;
    }
    mdp_receive_ack(DUMPFILE_END_RSP);
    ret = 0;
exit:
    if (g_modem_fd > 0)
    {
        close(g_modem_fd);
    }
    return ret;
}

void broadcast_ramdump_result(int success)
{
#if 0
    char command[FILENAME_MAX_LEN];
    snprintf(command, FILENAME_MAX_LEN, "am broadcast -a zte.com.cn.intent_modemramdump_finished --ez extra_success %s", (success == 0 ? "true" : "false"));
    LogInfo("%s %s\n", __FUNCTION__, command);
    system(command);
#endif
}


static void compress_and_rm_fold(char *base_path, char *time_str)
{
    char cmd_buffer[512] = {0};
    char ramdump_path[FILENAME_MAX_LEN] = {0};
    int ret = 0;

    //1. delete dumplicate tar
    //2. tar with time_str
    //3. delete fold

    //{base_path}/{Ramdump}/
    strncat(ramdump_path, base_path, FILENAME_MAX_LEN);
    strncat(ramdump_path, "/", 1);
    strncat(ramdump_path, "Ramdump", 7);
    strncat(ramdump_path, "/", 1);
    
    // delete duplicate file which preffix is g_current_file_index;
    //cd {base_path}/{Ramdump}/
    //rm -rvf {g_current_file_index}.*.tgz
    snprintf(cmd_buffer, 512, "cd %s; busybox rm -rvf %d.*.tgz\n", ramdump_path, g_current_file_index);
    LogInfo("%s %s\n", __FUNCTION__, cmd_buffer);
    system(cmd_buffer);

    char path_name[FILENAME_MAX_LEN] = {0};
    char str[FILENAME_MAX_LEN] = {0};

    //{g_current_file_index}.{time_str}
    snprintf(str, FILENAME_MAX_LEN, "%d", g_current_file_index);
    strncat(path_name, str, 1);
    strncat(path_name, ".", 1);
    strncat(path_name, time_str, FILENAME_MAX_LEN);

    //cd {base_path}/{Ramdump}/
    //tar -zcf {path_name}.tgz {path_name}*
    //tar -zcf {g_current_file_index}.{time_str}.tgz {g_current_file_index}.{time_str}*
    LogInfo("%s %s %s\n", __FUNCTION__, ramdump_path, path_name);
    snprintf(cmd_buffer, 512, "cd %s; busybox tar -zcf %s.tgz %s*\n", ramdump_path, path_name, path_name);
    LogInfo("%s %s\n", __FUNCTION__, cmd_buffer);
    ret = system(cmd_buffer);
    if (ret != 0)
    {
        LogInfo("compress failed, delete the unfinished compressed file\n");
        snprintf(cmd_buffer, 512, "cd %s;busybox rm -rvf  %s.tgz \n", ramdump_path, path_name);
    }
    else
    {
        LogInfo("compress finished, delete the source fold\n");
        snprintf(cmd_buffer, 512, "cd %s; busybox rm -rvf %s\n", ramdump_path, path_name);
        // property_set("persist.service.ramdump.index", str); (后续打开） {index}

        char file_name[FILENAME_MAX_LEN];
        int fd = 0;
        snprintf(file_name, FILENAME_MAX_LEN, "%s/%s", ramdump_path, "index.txt");
        unlink(file_name);
        LogInfo("%s %s\n", __FUNCTION__, file_name);
        fd = open(file_name, O_CREAT | O_RDWR, 0777);
        if (fd < 0)
        {
            LogInfo("failed to create %s (%s)\n", ramdump_path, strerror(errno));
        }
        else
        {
            write(fd, str, 255);
        }
        close(fd);

        // property_get("persist.service.ramdump.index", str, "6");  OPEN LATER
        LogInfo("%s %s %s\n", __FUNCTION__, "persist.service.ramdump.index=", str); //{index}
    }
    LogInfo("%s %s\n", __FUNCTION__, cmd_buffer);
    system(cmd_buffer);
}

static int get_time_str(char *buf, size_t size)
{
    struct tm cur_tm;
    time_t now = time(NULL);
    if (NULL == buf || size <= 0)
    {
        return -1;
    }
        
    localtime_r(&now, &cur_tm);
    strftime(buf, size, "%Y_%m%d_%H%M%S", &cur_tm);
    LogInfo("%s %s\n", __FUNCTION__, buf);
    return 0;
}

static int get_ramdump_fold_name(char *ramdump_path, size_t size, char *base_path, char *time_str)
{
    // property_get(CPLOG_PATH, ramdump_path, ZTE_LOG_PATH);

    //{base_path}/{Ramdump}/
    strncat(ramdump_path, base_path, FILENAME_MAX_LEN);
    strncat(ramdump_path, "/", 1);
    strncat(ramdump_path, "Ramdump", 7);
    strncat(ramdump_path, "/", 1);

    char index[FILENAME_MAX_LEN] = {0};
    char str[FILENAME_MAX_LEN] = {0};

    // property_get("persist.service.ramdump.index", index, "0");
    g_current_file_index = atoi(index);
    LogInfo("%s %s %d\n", __FUNCTION__, "persist.service.ramdump.index", g_current_file_index);

    g_current_file_index += 1;
    if (g_current_file_index > 5)
    {
        g_current_file_index = 1;
    }

    snprintf(str, FILENAME_MAX_LEN, "%d", g_current_file_index);

    //{g_current_file_index}.{time_str}/
    strncat(ramdump_path, str, 1);
    strncat(ramdump_path, ".", 1);
    strncat(ramdump_path, time_str, FILENAME_MAX_LEN);
    strncat(ramdump_path, "/", 1);

    //{base_path}/{Ramdump}/{g_current_file_index}.{time_str}/
    LogInfo("%s %s\n", __FUNCTION__, ramdump_path);
    return 0;
}

int zte_dump_main(char *portname, char *s_logpath)
{
    int ret = -1;

    char *base_path = NULL;
    char dump_path[FILENAME_MAX_LEN] = {0};
    
    char time_str[64] = {0};
    // property_set("ctl.stop", "ztemodemlog");

    sleep(1);
    if (get_time_str(time_str, 64) < 0)
    {
        LogInfo("Can't get the time str\n");
        return -1;
    }

    // set default portname
    if ((portname == NULL) || (portname[0] == '\0'))
    {
        portname = MODEM_TRAP_PATH;
    }
    LogInfo("portname: %s\n", portname);

    // set default dump_path
    if ((s_logpath == NULL) || (s_logpath[0] == '\0'))
    {
        base_path = DEFAULT_RAMDUMP_FOLD;
    }
    else
    {
        base_path = s_logpath;
    }
    ret = get_ramdump_fold_name(dump_path, FILENAME_MAX_LEN, base_path, time_str);
    if (ret < 0)
    {
        LogInfo("Can't get the ramdump fold path\n");
        return -1;
    }
    LogInfo("base_path: %s, dump_path: %s\n", base_path, dump_path);

    //start dump
    LogInfo("try to get the ramdump data from %s\n", portname);
    ret = do_modem_ramdump(portname, dump_path);
    if (ret < 0)
    {
        LogInfo("get the ramdump data from %s err %d, try again\n", portname, ret);

        // try again
        ret = do_modem_ramdump(portname, dump_path);
        if (ret < 0)
        {
            LogInfo("get the ramdump data from %s err %d\n", portname, ret);
            broadcast_ramdump_result(ret);
            return ret;
        }
    }
    LogInfo("get the ramdump data from %s success\n", portname);
    compress_and_rm_fold(base_path, time_str);
    broadcast_ramdump_result(ret);

    return ret;
}
