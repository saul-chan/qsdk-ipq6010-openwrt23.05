#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include "usb2tcp.h"
#include "zte_devices_list.h"
#include "zte_download.h"

fibo_usbdev_t *pdev = NULL;

fibo_usbdev_t *zte_find_devices_in_table(int idvendor, int idproduct, int check_port_type)
{
    int i, size = 0;
    fibo_usbdev_t *pdev = NULL;

    // LogInfo("start\n");
    if (check_port_type) {
        size = sizeof(zte_devices_table)/sizeof(zte_devices_table[0]);
        for (i=0; i<size; i++)
        {
            pdev = &zte_devices_table[i];

            if ((pdev->idVendor == idvendor) && (pdev->idProduct == idproduct)) {
                goto FIND;
            }
        }

        if (check_port_type == 1) {
            return NULL;
        }
    }

    size = sizeof(zte_dl_devices_table)/sizeof(zte_dl_devices_table[0]);
    for (i=0; i<size; i++)
    {
        pdev = &zte_dl_devices_table[i];
        if ((pdev->idVendor == idvendor) && (pdev->idProduct == idproduct)) {
            goto FIND;
        }
    }

    return NULL;
FIND:
    pdev->ttyfd = -1;
    pdev->usbdev = -1;
    pdev->tcp_client_fd = -1;
    pdev->pcie_fd = -1;
    return pdev;
}

BOOL SendData(BYTE *pbyWriteBuffer, DWORD dwWriteCount, DWORD dwSleepAfterAction, DWORD dwTimeoutCount)
{
    int result = 0;
    DWORD cur_size = 0;

    dwTimeoutCount = MAX(dwTimeoutCount, 10);
    while (cur_size < dwWriteCount) {
        DWORD writeBytes = MIN((dwWriteCount - cur_size), DOWNLOAD_PACKSIZE);
        result = pdev->write(pdev, (char *)pbyWriteBuffer + cur_size, writeBytes, writeBytes, dwTimeoutCount*1000);
        if (result <= 0) {
            LogInfo("write failed, result: %d\r\n", result);
            return FALSE;
        }
        // LogInfo("write result: %d", result);
        // DumpHex(pbyWriteBuffer, result);
        cur_size += writeBytes;
        usleep(5);
    }
    if (dwSleepAfterAction) {} //avoid warning

    return TRUE;
}

BOOL ReadData(BYTE *pbyReadBuffer, DWORD dwReadCount, DWORD dwSleepAfterAction, DWORD dwTimeoutCount)
{
    int result = 0;
    DWORD cur_size = 0;

    dwTimeoutCount = MAX(dwTimeoutCount, 10);
    LogInfo("dwReadCount: %d\r\n", dwReadCount);
    while (cur_size < dwReadCount) {
        result = pdev->read(pdev, (char *)pbyReadBuffer + cur_size, dwReadCount, dwReadCount, dwTimeoutCount*1000);
        if (result <= 0) {
            LogInfo("read failed, result: %d\r\n", result);
            return FALSE;
        }

        // LogInfo("read result: %d", result);
        // DumpHex(pbyReadBuffer+cur_size,result);
        cur_size += result;
    }
    if (dwSleepAfterAction) {} //avoid warning

    return TRUE;
}

static int fibo_send_atcmd(char *sendstr, char *mach_str)
{
    char readstr[1024] = {0};
    int ret = -1;

    if (sendstr == NULL) {
        LogInfo("sendstr is NULL.\r\n");
        goto END;
    }

    if (fibo_usb_open(pdev, 0) != 0) {
        LogInfo("failed.\r\n");
        goto END;
    }

    LogInfo("send: %s\r\n", sendstr);
    if (!SendData((BYTE *)sendstr, strlen(sendstr), 1, 1)) {
        goto END;
    }

    int readbyte = pdev->read(pdev, readstr, sizeof(readstr), sizeof(readstr), 1000);
    if (readbyte <= 0) {
        goto END;
    }

    if (mach_str && mach_str[0]) {
        LogInfo("readbyte: %d\r\n", readbyte);
        if (strstr(readstr, mach_str) == NULL) {
            LogInfo("mach_str: %s\r\n", mach_str);
            LogInfo("readstr: %s\r\n", readstr);
            LogInfo("readstr != mach_str\r\n");
            goto END;
        }
    } else {
        LogInfo("readstr: %s\r\n", readstr);
    }

    ret = 0;
END:
    fibo_usb_close(pdev);

    return ret;
}

static int CheckATPort(char *portname, char *syspath, int times)
{
    int i = 0;

    LogInfo("wait for at port\r\n");
    for (i=0; i<times; i++) {
        pdev = fibo_get_fibocom_device(zte_find_devices_in_table, portname, syspath, 1);
        if (pdev != NULL) {
            return 0;
        }
        usleep(5000);
    }

    return -1;
}

static int CheckDLPort(char *portname, char *syspath, int times)
{
    int i = 0;

    LogInfo("wait for dl port\r\n");
    for(i=0; i<times; i++) {
        pdev = fibo_get_fibocom_device(zte_find_devices_in_table, portname, syspath, 0);
        if (pdev != NULL) {
            return 0;
        }

        usleep(5000);
    }
    return -1;
}

static BOOL FileAnalyse(char *firmware_name)
{
    BOOL ret = FALSE;
    FILE *pf = NULL;
    char keycode[16] = {0};
    char standardkeycod[] = {0xdf, 0xf1, 0x13, 0x63, 0x1a, 0x98, 0xcd, 0xa8, 0xa5, 0x19, 0x7c, 0x1b, 0x1a, 0xba, 0x9f, 0xd7}; //IDENTIFYCODE

    LogInfo("firmware_name: %s\r\n", firmware_name);
    if (access(firmware_name, 0) != 0)
    {
        LogInfo("%s is not existed.\r\n", firmware_name);
        return FALSE;
    }

    pf = fopen(firmware_name, "rb");
    if (sizeof(keycode) != fread(keycode, 1, sizeof(keycode), pf)) {
        LogInfo("fread %s failed.\r\n", firmware_name);
        goto END;
    }

    DumpHex(keycode, sizeof(keycode));
    if (memcmp(keycode, standardkeycod, sizeof(keycode))) {
        LogInfo("check %s failed.\r\n", firmware_name);
        goto END;
    }

    ret = TRUE;
END:
    if (pf) {
        fclose(pf);
        pf = NULL;
    }

    return ret;
}

static void usage(char *arg)
{
    printf("------------------------------------------------------\n");
    printf("            <zte upgrade help info>      \n");
    printf("Usage: %s [options...]\n", arg);
    printf(" -f [bin file]         Upgrade bin file, for example: -f /xx/xx.bin \n");
    printf(" -d [/dev/ttyUSBx]              Diagnose port, will auto-detect if not specified\n");
    printf(" -d [/sys/bus/usb/devices/xx]   When multiple modules exist on the board, use -s specify which module to upgrade\n");
    printf(" -l [log dir]                   save log into a file(will create upgrade_tool_timestamp.log)\n");
    printf(" -z [0/1]                       usb_need_zero_package, default is 0\n");
    printf(" -h  help info\n"); 
    printf("Applicable Models: \n");
    printf("           ZTE V3E/T: L716/L718\n");
    printf("------------------------------------------------------\n");
}

int zte_download_main(int argc, char **argv)
{
    int retval = -1, opt = -1, i = 0;
    int qusb_zlp_mode = 0;
    double old_time = 0, new_time = 0;
    char firmware_name[MAX_PATH_LEN] = {0};
    char portname[MAX_PATH_LEN] = {0}, syspath[MAX_PATH_LEN] = {0};
    char dl_log_filename[MAX_PATH_LEN] = {0}, usbmon_log_filename[MAX_PATH_LEN] = {0};
    int usb2tcp_server_mode = 0; //tcp server mode
    int usb2tcp_download_mode = 0; //tcp client mode
    struct timeval tv;
    int wait_dl_only = 0;

    LogInfo("start\r\n");
    LogInfo("DOWNLOAD_PACKSIZE: %d\r\n", DOWNLOAD_PACKSIZE);

    umask(0);
    optind = 1;//must set to 1
    while ((opt = getopt(argc, argv, "f:b:d:l:ez:u:wh")) != -1)
    {
        switch (opt)
        {
            case 'b':
            case 'f':
                strcpy(firmware_name, optarg); break;
            case 'l':
                snprintf(dl_log_filename, sizeof(dl_log_filename), "%s/fibo_download_%lu.log", optarg, time(NULL));
                break;
            case 'd':
                strncpy(portname, optarg, MAX_PATH_LEN);
                if (!strcmp(portname, "9008"))
                {
                    usb2tcp_server_mode = 1;
                    LogInfo("usb2tcp_server_mode: %d\r\n", usb2tcp_server_mode);
                }
                else if (strstr(portname, ":9008"))
                {
                    usb2tcp_download_mode = 1;
                    memset(portname, 0, sizeof(portname));
                    LogInfo("usb2tcp_download_mode: %d\r\n", usb2tcp_download_mode);
                }
                break;
            case 'z':
                qusb_zlp_mode = !!atoi(optarg);
                break;
            case 'u':
                strncpy(usbmon_log_filename, optarg, sizeof(usbmon_log_filename));
                break;
            case 'w':
                wait_dl_only = 1;
                break;
            case 'h':
                usage(argv[0]);
                return 0;
            default:;
        }
    }

    if (dl_log_filename[0]) {
        g_dl_logfile_fp = fopen(dl_log_filename, "w+");
        if (g_dl_logfile_fp) {
            LogInfo("upgrade log will be save to %s\r\n", dl_log_filename);
        }
    }

    if (usb2tcp_server_mode == 0)
    {
        if (firmware_name[0] == 0) {
            usage(argv[0]);
            goto END;
        }

        if (!FileAnalyse(firmware_name))
        {
            LogInfo("FileAnalyse failed.\r\n");
            goto END;
        }

        if (usb2tcp_download_mode)
        {
            pdev = fibo_usb2tcp_open();
            if (pdev == NULL) {
                LogInfo("error: pdev is NULL\r\n");
                goto END;
            }
            goto DL_STEP;
        }

        if (fibo_strStartsWith(portname, USB_DIR_BASE)) {
            strcpy(syspath, portname);
            memset(portname, 0, sizeof(portname));
        }

        if (usbmon_log_filename[0]) {
            fibo_usbmon_log_start(usbmon_log_filename);
        }
    }

    if (wait_dl_only == 0 && !CheckATPort(portname, syspath, 2))
    {
        for(i=0; i<60; i++)
        {
            if (!fibo_send_atcmd("AT\r\n", "OK")) {
                LogInfo("Device start to reset, please wait.\r\n");
                fibo_send_atcmd("at+cfun=15\r\n", NULL);
                memset(syspath, 0, sizeof(syspath));
                memset(portname, 0, sizeof(portname));
                if (!CheckDLPort(portname, syspath, 1000)) {
                    break;
                }
            }
            usleep(10000);
        }
    }

    if (CheckDLPort(portname, syspath, 60000)) {
        return -1;
    }

    if (fibo_usb_open(pdev, 0) != 0) {
        LogInfo("failed.\r\n");
        goto END;
    }
DL_STEP:
    LogInfo("\n");
    pdev->usb_need_zero_package = qusb_zlp_mode;
    if (usb2tcp_server_mode == 0) {
        gettimeofday(&tv, NULL);
        old_time = (double)tv.tv_sec + (double)tv.tv_usec / 1000000;
        if (!DoDownloadBootForDL(firmware_name)) {
            goto END;
        }
        LogInfo("DoDownloadBootForDL OK\r\n");

        if (!PrimaryDoDownload(firmware_name)) {
            goto END;
        }
        LogInfo("PrimaryDoDownload OK\r\n");
        fibo_send_atcmd("reboot\r\n", NULL); //In the download mode, reboot
        retval = 0;
    }
    else { //usb2tcp mode
        retval = usb2tcp_server_main(pdev);
    }
END:
    fibo_usb_close(pdev);
    fibo_usb2tcp_close(pdev);
    if (usb2tcp_server_mode) { return retval; }
    if (g_dl_logfile_fp) { fclose(g_dl_logfile_fp); g_dl_logfile_fp = NULL;}
    if (usbmon_log_filename[0]) { fibo_usbmon_log_stop(); }

    LogInfo("-----------------------\r\n");
    if (retval == 0) {
        gettimeofday(&tv, NULL);
        new_time = (double)tv.tv_sec + (double)tv.tv_usec / 1000000;
        LogInfo("The total used time: %.3fs\r\n", new_time - old_time);
#if 0
        //start: After the download is successful, delete the download log.
        if (dl_log_filename[0] && !access(dl_log_filename, 0)) {
            if(!remove(dl_log_filename)) {
                LogInfo("Removed %s OK\r\n", dl_log_filename);
            }
        }
        //end: After the download is successful, delete the download log.
#endif
        LogInfo("Upgrade module successfully\r\n");
    } else {
        LogInfo("Upgrade module failed\r\n");
    }
    LogInfo("-----------------------\r\n");

    return retval;
}