#include "smr5300_download.h"
#include "smr5300_devices_list.h"
#include "usb2tcp.h"
extern char dl_filename[MAX_PATH_LEN*2];
extern int g_samsung_fastboot_type;
extern int samsung_open_tcp(fibo_usbdev_t *pdev, char *ifname, char *server_ip, int server_port, int timeout_s);
extern int samsung_close_tcp(fibo_usbdev_t *pdev);
extern int smr_dnw_fastboot_mode(fibo_usbdev_t *pdev);
enum SS_FB_DOWNLOAD_TYPE
{
    // PC <---> DEV
    FB_DOWNLOAD_TYPE_DIRECT = 0, //  connet to device directly ex. USB/PCIe

    // PC1 <---> PC2 <---> DEV
    FB_DOWNLOAD_TYPE_TCP_CLIENT = 5, // it`s a client on PC1,the images also on
    FB_DOWNLOAD_TYPE_TCP_SERVER      // it`s a server on PC2
};

fibo_usbdev_t *smr_find_devices_in_table(int idvendor, int idproduct, int check_port_type)
{
    int i, size = 0;
    fibo_usbdev_t *pdev = NULL;

    // LogInfo("start\n");
    if (check_port_type)
    {
        size = sizeof(smr_devices_table) / sizeof(smr_devices_table[0]);
        for (i = 0; i < size; i++)
        {
            pdev = &smr_devices_table[i];

            if ((pdev->idVendor == idvendor) && (pdev->idProduct == idproduct))
            {
                goto FIND;
            }
        }

        if (check_port_type == 1)
        {
            return NULL;
        }
    }

    size = sizeof(smr_dl_devices_table) / sizeof(smr_dl_devices_table[0]);
    for (i = 0; i < size; i++)
    {
        FILE *fd = NULL;
        pdev = &smr_dl_devices_table[i];
        if ((pdev->idVendor == idvendor) && (pdev->idProduct == idproduct))
        {
            goto FIND;
        }
        else if ((pdev->idVendor == 0xFFFF) && (pdev->idProduct == 0xFF00))
        {
            char sysCmd[1024] = {0};
            snprintf(sysCmd, sizeof(sysCmd), "ifconfig -a | grep mgif_raw > /dev/null && echo 1 || echo 0");
            if ((fd = popen(sysCmd, "r")) != NULL)
            {
                char tmp[256] = {0};
                fgets(tmp, 255, fd);
                pclose(fd);
                if (tmp[0] == '1')
                {
                    goto FIND;
                }
            }
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

static int smr_switch_to_fastboot_mode(fibo_usbdev_t *pdev)
{
    char *pcmd = NULL;

    pcmd = "at+reboot=0\r\n";
    if (fibo_usb_open(pdev, 0))
    {
        return -1;
    }

    if (pdev->write(pdev, pcmd, strlen(pcmd), 0, 3000) != (int)strlen(pcmd))
    {
        return -1;
    }

    LogInfo("%s\n", pcmd);
    LogInfo("OK\n");
    fibo_usb_close(pdev);

    return 0;
}

static fibo_usbdev_t *CheckATPort(char *portname, char *syspath, int times)
{
    int i = 0;

    for (i = 0; i < times; i++)
    {
        fibo_usbdev_t *pdev = fibo_get_fibocom_device(smr_find_devices_in_table, portname, syspath, 1);
        if (pdev != NULL)
        {
            LogInfo("find at port\n");
            return pdev;
        }
        else if (i == 0)
        {
            LogInfo("wait for at port\n");
        }
        usleep(5000);
    }

    return NULL;
}

static fibo_usbdev_t *CheckDLPort(char *portname, char *syspath, int times)
{
    int i = 0;

    for (i = 0; i < times; i++)
    {
        fibo_usbdev_t *pdev = fibo_get_fibocom_device(smr_find_devices_in_table, portname, syspath, 0);
        if (pdev != NULL)
        {
            LogInfo("find dl port\n");
            return pdev;
        }
        else if (i == 0)
        {
            LogInfo("wait for dl port, please power on or reboot the device\n");
        }
        usleep(5000);
    }
    return NULL;
}

static void usage(const char *arg)
{
    printf("------------------------------------------------------\n");
    printf("            <smr upgrade help info>      \n");
    printf("Usage: %s [options...]\n", arg);
    printf(" -f [Upgrade file]         Upgrade pac file, for example: -f 83600.1000.00.01.01.01 \n");
    printf(" -l [log dir]              save log into a file(will create fibo_download.log)\n");
    printf(" -e                        Erase All Before Download (This will Erase NV params, Generally not used)\n");
    printf(" -h  help info\n");
    printf("Applicable Models: \n");
    printf("        SMR : SMR5300\n");
    printf("------------------------------------------------------\n");
}

int samsung_download_main(int argc, char **argv)
{
    int retval = -1, i = 0;
    fibo_usbdev_t *pdev = NULL;
    char portname[MAX_PATH_LEN] = {0}, syspath[MAX_PATH_LEN] = {0};
    //bug MBB0678-911 guorui 20250102 start
    char ght_dl_filename[MAX_PATH_LEN*2] ={0};
    char firmware_dir[MAX_PATH_LEN] = {0};
    int opt = -1;
    LogInfo("start\n");
    optind = 1;
    while (-1 != (opt = getopt(argc, argv, "f:")))
    {
        switch (opt)
        {
        case 'f':
            strcpy(firmware_dir, optarg);
            memset(ght_dl_filename, 0, sizeof(ght_dl_filename));
            snprintf(ght_dl_filename, sizeof(ght_dl_filename), "%s/%s", firmware_dir, UPGRADE_UPDATE_NAME);
            break;
        default:;
        }
    }
    LogInfo("dl_filename is %s\n",ght_dl_filename);
    if(access(ght_dl_filename, F_OK) == 0)
    {
        LogInfo("dl_filename is %s\n",ght_dl_filename);
    }
    else
    {
        LogInfo("is not samsung pac\n");
        return retval;
    }
    //bug MBB0678-911 guorui 20250102 end
    if ((pdev = CheckATPort(portname, syspath, 2)))
    {

        if (smr_switch_to_fastboot_mode(pdev) != 0)
        {
            LogInfo("CAUSION: at cmd fail, exit !\n");
            retval = -1;
        }

        LogInfo("waiting for device goto fastboot after reboot at cmd.\n");
        for (i = 0; i < 60; i++)
        {
            pdev = CheckDLPort(portname, syspath, 1000);
            if (pdev != NULL)
            {
                if (pdev->main_function == NULL)
                {
                    printf("udev->main_function is NULL\n");
                    retval = -1;
                }
                printf("start to run [%s] main function\n", pdev->ModuleName);
                retval = pdev->main_function(argc, argv);
                break;
            }
            usleep(100 * 1000);
        }
        if (i == 60)
        {
            LogInfo("CAUSION: device can't goto fastboot after at cmd, exit \n");
            retval = -1;
        }
    }
    LogInfo("end\n");

    return retval;
}

int samsung_download_usb(int argc, char **argv)
{
    int retval = -1, opt = -1, i = 0;
    double old_time = 0, new_time = 0;
    char firmware_dir[MAX_PATH_LEN] = {0};
    char portname[MAX_PATH_LEN] = {0}, syspath[MAX_PATH_LEN] = {0};
    char dl_log_filename[MAX_PATH_LEN] = {0}, usbmon_log_filename[MAX_PATH_LEN] = {0};
    int usb2tcp_server_mode = 0;   // tcp server mode
    int usb2tcp_download_mode = 0; // tcp client mode
    fibo_usbdev_t *pdev = NULL;
    struct timeval tv;

    LogInfo("start\n");
    umask(0);
    optind = 1; // must set to 1
    while (-1 != (opt = getopt(argc, argv, "f:d:p:l:u:g:he")))
    {
        switch (opt)
        {
        case 'f':
            strcpy(firmware_dir, optarg);
            memset(dl_filename, 0, sizeof(dl_filename));
            snprintf(dl_filename, sizeof(dl_filename), "%s/%s", firmware_dir, UPGRADE_UPDATE_NAME);
            break;
        case 'l':
            snprintf(dl_log_filename, sizeof(dl_log_filename), "/%s/fibo_download_%lu.log", optarg, time(NULL));
            LogInfo("log filename : %s\n", dl_log_filename);
            break;
        case 'd':
            strncpy(portname, optarg, MAX_PATH_LEN);
            break;
        case 'p':
            if (strstr(optarg, ":9008"))
            {
                usb2tcp_download_mode = 1;
                LogInfo("usb2tcp_download_mode: %d\n", usb2tcp_download_mode);
            }
            else if (strstr(optarg, "9008"))
            {
                usb2tcp_server_mode = 1;
                LogInfo("usb2tcp_server_mode: %d\n", usb2tcp_server_mode);
            }
            break;
        case 'u':
            strncpy(usbmon_log_filename, optarg, sizeof(usbmon_log_filename));
            break;
        case 'e':
            LogInfo(" \n");
            memset(dl_filename, 0, sizeof(dl_filename));
            snprintf(dl_filename, sizeof(dl_filename), "%s/%s", firmware_dir, UPGRADE_ALL_NAME);
            LogInfo("all_erase filename : %s\n", dl_filename);
            break;
        case 'h':
            usage(argv[0]);
            return 0;
        default:;
        }
    }

    if (usb2tcp_server_mode == 0)
    {
        if (dl_log_filename[0])
        {
            g_dl_logfile_fp = fopen(dl_log_filename, "w+");
            if (g_dl_logfile_fp)
            {
                LogInfo("upgrade log will be save to %s\n", dl_log_filename);
            }
        }
        if (usb2tcp_download_mode)
        {
            pdev = fibo_usb2tcp_open();
            if (pdev == NULL)
            {
                LogInfo("error: pdev is NULL\n");
                goto END;
            }
            goto DL_STEP;
        }

        if (fibo_strStartsWith(portname, USB_DIR_BASE))
        {
            strcpy(syspath, portname);
            memset(portname, 0, sizeof(portname));
        }

        if (usbmon_log_filename[0])
        {
            fibo_usbmon_log_start(usbmon_log_filename);
        }
    }

    if ((pdev = CheckATPort(portname, syspath, 2)))
    {
        if (smr_switch_to_fastboot_mode(pdev) != 0)
        {
            LogInfo("CAUSION: at cmd fail, exit !\n");
            goto END;
        }
        LogInfo("waiting for device goto fastboot after reboot at cmd.\n");
        for (i = 0; i < 60; i++)
        {
            if (CheckDLPort(portname, syspath, 1000))
            {
                break;
            }
            usleep(100 * 1000);
        }
        if (i == 60)
        {
            LogInfo("CAUSION: device can't goto fastboot after at cmd, exit \n");
            goto END;
        }
    }
    LogInfo("CheckDLPort star \n");
    if (!(pdev = CheckDLPort(portname, syspath, 30000)))
    {
        goto END;
    }
    LogInfo("CheckDLPort end \n");

    if (fibo_usb_open(pdev, 0))
    {
        LogInfo("Upgrade module failed\n");
        return -1;
    }

DL_STEP:
    LogInfo("\n");
    pdev->usb_need_zero_package = 0; // qusb_zlp_mode;
    if (usb2tcp_server_mode == 0)    // normal download mode
    {
        gettimeofday(&tv, NULL);
        old_time = (double)tv.tv_sec + (double)tv.tv_usec / 1000000;
        sleep(2);
        retval = smr_dload(pdev, firmware_dir);
    }
    else
    { // usb2tcp download mode
        retval = usb2tcp_server_main(pdev);
    }

END:
    LogInfo("-----------------------\n");
    if (retval == 0)
    {
        gettimeofday(&tv, NULL);
        new_time = (double)tv.tv_sec + (double)tv.tv_usec / 1000000;
        LogInfo("The total used time: %.3f s\n", new_time - old_time);
        LogInfo("Upgrade module successfully\n");
    }
    else
    {
        LogInfo("Upgrade module failed\n");
    }
    LogInfo("-----------------------\n");

    fibo_usb_close(pdev);
    fibo_usb2tcp_close(pdev);
    if (usb2tcp_server_mode)
    {
        return retval;
    }
    if (g_dl_logfile_fp)
    {
        fclose(g_dl_logfile_fp);
        g_dl_logfile_fp = NULL;
    }
    if (usbmon_log_filename[0])
    {
        fibo_usbmon_log_stop();
    }
    return retval;
}

int smr_download_pcie(int argc, char **argv)
{
    int retval = -1, opt = -1;
    double old_time = 0, new_time = 0;
    char firmware_dir[MAX_PATH_LEN] = {0};
    char dl_log_filename[MAX_PATH_LEN] = {0};
    int fastboot_download_type = FB_DOWNLOAD_TYPE_DIRECT;
    fibo_usbdev_t tty_dev = {0};
    fibo_usbdev_t *pdev = NULL;
    fibo_usbdev_t *pdev_client = NULL;
    fibo_usbdev_t *pdev_pcie = NULL;
    struct timeval tv;
    char ifname[16] = "mgif_raw";        // mdi driver name on Ubuntu
    char server_ip[16] = "192.168.0.98"; // fixed IP of server in LK
    int server_port = 5554;              // fixed port of server in LK
    int wait_timeout = 0;                // second

    g_samsung_fastboot_type = SS_FASTBOOT_TYPE_NET;
    LogInfo("start\n");
    umask(0);
    optind = 1; // must set to 1
    while (-1 != (opt = getopt(argc, argv, "f:t:d:p:l:g:he")))
    {
        switch (opt)
        {
        case 'f':
            strcpy(firmware_dir, optarg);
            memset(dl_filename, 0, sizeof(dl_filename));
            snprintf(dl_filename, sizeof(dl_filename), "%s/%s", firmware_dir, UPGRADE_UPDATE_NAME);
            break;
        case 'l':
            snprintf(dl_log_filename, sizeof(dl_log_filename), "/%s/fibo_download_%lu.log", optarg, time(NULL));
            LogInfo("log filename : %s\n", dl_log_filename);
            break;
        case 'p':
            if (strstr(optarg, ":9008"))
            {
                fastboot_download_type = FB_DOWNLOAD_TYPE_TCP_CLIENT;
            }
            else if (strstr(optarg, "9008"))
            {
                fastboot_download_type = FB_DOWNLOAD_TYPE_TCP_SERVER;
            }
            break;
        case 'e':
            LogInfo(" \n");
            memset(dl_filename, 0, sizeof(dl_filename));
            snprintf(dl_filename, sizeof(dl_filename), "%s/%s", firmware_dir, UPGRADE_ALL_NAME);
            LogInfo("all_erase filename : %s\n", dl_filename);
            break;

        case 'h':
            usage(argv[0]);
            return 0;

        default:;
        }
    }

    switch (fastboot_download_type)
    {
    case FB_DOWNLOAD_TYPE_TCP_SERVER:
    case FB_DOWNLOAD_TYPE_DIRECT:
        // It will do access 'portname', if we are in LK fastboot mode,
        // the access will fail and will not send switch cmd
        strncpy(tty_dev.portname, "/dev/ttyMD_at0", sizeof(tty_dev.portname) - 1);
        tty_dev.ttyfd = -1;
        tty_dev.usbdev = -1;
        tty_dev.pcie_fd = -1;

        if (smr_switch_to_fastboot_mode(&tty_dev) == 0)
        {
            // need wait enter fastpcie
            wait_timeout = 30;
            LogInfo("waiting for device goto fastboot, wait_timeout=%d\n", wait_timeout);
            sleep(wait_timeout);
        }

        pdev_pcie = smr_find_devices_in_table(0xFFFF, 0xFF00, 2);
        if (!pdev_pcie)
        {
            LogInfo("get device fail!\n");
            goto END;
        }

        if (samsung_open_tcp(pdev_pcie, ifname, server_ip, server_port, wait_timeout))
        {
            LogInfo("tcp create fail!\n");
            goto END;
        }

        pdev = pdev_pcie;
        break;

    case FB_DOWNLOAD_TYPE_TCP_CLIENT:
        // main.c already do tcp_connect_module_host
        pdev_client = fibo_usb2tcp_open();
        if (!pdev_client)
        {
            LogInfo("error: pdev_client is NULL\n");
            goto END;
        }

        pdev = pdev_client;
        break;

    default:
        LogInfo("fastboot_download_type=%d not support!!\n", fastboot_download_type);
        goto END;

        break;
    }

    switch (fastboot_download_type)
    {
    case FB_DOWNLOAD_TYPE_TCP_SERVER:
        retval = usb2tcp_server_main(pdev);
        break;

    case FB_DOWNLOAD_TYPE_TCP_CLIENT:
    case FB_DOWNLOAD_TYPE_DIRECT:
        if (dl_log_filename[0])
        {
            g_dl_logfile_fp = fopen(dl_log_filename, "w+");
            if (g_dl_logfile_fp)
            {
                LogInfo("upgrade log will be save to %s\n", dl_log_filename);
            }
        }

        gettimeofday(&tv, NULL);
        old_time = (double)tv.tv_sec + (double)tv.tv_usec / 1000000;
        sleep(2);
        retval = smr_dload(pdev, firmware_dir);
        if (retval == 0)
        {
            gettimeofday(&tv, NULL);
            new_time = (double)tv.tv_sec + (double)tv.tv_usec / 1000000;
            LogInfo("The total used time: %.3f s\n", new_time - old_time);
            LogInfo("Upgrade module successfully\n");
        }
        else
        {
            LogInfo("Upgrade module failed\n");
        }

        break;

    default:
        break;
    }

END:
    // if (pdev_pcie) { samsung_close_tcp(pdev_pcie); }
    if (pdev_client)
    {
        fibo_usb2tcp_close(pdev_client);
    }
    if (g_dl_logfile_fp)
    {
        fclose(g_dl_logfile_fp);
    }

    return retval;
}