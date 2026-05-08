#include "unisoc_download.h"
#include "unisoc_devices_list.h"
#include "usb2tcp.h"

fibo_usbdev_t *unisoc_find_devices_in_table(int idvendor, int idproduct, int check_port_type)
{
    int i, size = 0;
    fibo_usbdev_t *pdev = NULL;

    // LogInfo("start\n");
    if (check_port_type) {
        size = sizeof(unisoc_devices_table)/sizeof(unisoc_devices_table[0]);
        for (i=0; i<size; i++)
        {
            pdev = &unisoc_devices_table[i];

            if ((pdev->idVendor == idvendor) && (pdev->idProduct == idproduct)) {
                goto FIND;
            }
        }

        if (check_port_type == 1) {
            return NULL;
        }
    }

    size = sizeof(unisoc_dl_devices_table)/sizeof(unisoc_dl_devices_table[0]);
    for (i=0; i<size; i++)
    {
        pdev = &unisoc_dl_devices_table[i];
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

static int unisoc_switch_to_edl_mode(fibo_usbdev_t *pdev)
{
    char *pcmd = NULL;

    if (strstr(pdev->ModuleName, "UDX710")) {
        pcmd = "at+gtdlmode=autodloader\r\n";
    }
    else {
        pcmd = "at+spref=\"autodloader\"\r\n";
    }

    if (fibo_usb_open(pdev, 0)) {
        return -1;
    }

    if (pdev->write(pdev, pcmd, strlen(pcmd), 0, 3000) != (int)strlen(pcmd)) {
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

    for (i=0; i<times; i++) {
        fibo_usbdev_t *pdev = fibo_get_fibocom_device(unisoc_find_devices_in_table, portname, syspath, 1);
        if (pdev != NULL) {
            LogInfo("find at port\n");
            return pdev;
        }
        else if (i == 0) {
            LogInfo("wait for at port\n");
        }
        usleep(5000);
    }

    return NULL;
}

static fibo_usbdev_t *CheckDLPort(char *portname, char *syspath, int times)
{
    int i = 0;

    for(i=0; i<times; i++) {
        fibo_usbdev_t *pdev = fibo_get_fibocom_device(unisoc_find_devices_in_table, portname, syspath, 0);
        if (pdev != NULL) {
            LogInfo("find dl port\n");
            return pdev;
        }
        else if (i == 0) {
            LogInfo("wait for dl port, please power on or reboot the device\n");
        }
        usleep(5000);
    }
    return NULL;
}

static int fibo_send_sciu2s_msg(fibo_usbdev_t *pdev)
{
    //only udx710 dl mode need to send msg to open usb
    if (strstr(pdev->ModuleName, "UDX710 DL"))
    {
        struct usbdevfs_ctrltransfer control = {
            .bRequestType = 0x21,
            .bRequest = 0x22,
            .wValue = 0x01,
        };

        if (pdev->usbdev < 0) {
            LogInfo("ERROR, need usbdev to send sciu2s msg.\n");
            return -1;
        }

        if (0 != ioctl(pdev->usbdev, USBDEVFS_CONTROL, &control)) {
            LogInfo("ioctl USBDEVFS_CONTROL failed, errno:%d(%s)\n", errno, strerror(errno));
            return -1;
        }

        LogInfo("OK\n");
    }

    return 0;
}


static void usage(const char *arg)
{
    printf("------------------------------------------------------\n");
    printf("            <unisoc upgrade help info>      \n");
    printf("Usage: %s [options...]\n", arg);
    printf(" -f [pac file]         Upgrade pac file, for example: -f /xx/xx.pac \n");
    printf(" -d [/dev/ttyUSBx]              Diagnose port, will auto-detect if not specified\n");
    printf(" -d [/sys/bus/usb/devices/xx]   When multiple modules exist on the board, use -s specify which module to upgrade\n");
    printf(" -l [log dir]                   save log into a file(will create fibo_download.log)\n");
    printf(" -z [0/1]                       usb_need_zero_package, default is 0\n");
    printf(" -e                             Erase All Before Download (This will Erase NV params, Generally not used)\n");
    printf(" -h  help info\n");
    printf("Applicable Models: \n");
    printf("        UNISOC UDX710: FG650,FG652,FG621\n");
    printf("        UNISOC 8910: L610\n");
    printf("        UNISOC 8850: MC661 \n");
    printf("------------------------------------------------------\n");
}

int unisoc_download_main(int argc, char **argv)
{
    int retval = -1, opt = -1, i = 0;
    int qusb_zlp_mode = 1; //default -z 1
    double old_time = 0, new_time = 0;
    char firmware_name[MAX_PATH_LEN] = {0};
    char portname[MAX_PATH_LEN] = {0}, syspath[MAX_PATH_LEN] = {0};
    char dl_log_filename[MAX_PATH_LEN] = {0}, usbmon_log_filename[MAX_PATH_LEN] = {0};
    int usb2tcp_server_mode = 0; //tcp server mode
    int usb2tcp_download_mode = 0; //tcp client mode
    fibo_usbdev_t *pdev = NULL;
    struct timeval tv;
    int q_erase_all_before_download = 0; //eraseall before download

    LogInfo("start\n");
    umask(0);
    optind = 1;//must set to 1
    while (-1 != (opt = getopt(argc, argv, "f:d:p:l:ez:u:h")))
    {
        switch (opt)
        {
            case 'f':
                strcpy(firmware_name, optarg); break;
            case 'l':
                snprintf(dl_log_filename, sizeof(dl_log_filename), "%s/fibo_download_%lu.log", optarg, time(NULL));
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
            case 'e':
                q_erase_all_before_download = 1;
                LogInfo("q_erase_all_before_download: %d\n", q_erase_all_before_download);
                break;
            case 'z':
                qusb_zlp_mode = !!atoi(optarg);
                break;
            case 'u':
                strncpy(usbmon_log_filename, optarg, sizeof(usbmon_log_filename));
                break;
            case 'h':
                usage(argv[0]);
                return 0;
            default:;
        }
    }

    if (usb2tcp_server_mode == 0)
    {
        if (dl_log_filename[0]) {
            g_dl_logfile_fp = fopen(dl_log_filename, "w+");
            if (g_dl_logfile_fp) {
                LogInfo("upgrade log will be save to %s\n", dl_log_filename);
            }
        }

        if (firmware_name[0] == 0) {
            LogInfo("firmware_name is empty. required -f [pac file]\n");
            usage(argv[0]);
            goto END;
        }

        LogInfo("firmware_name: %s\n", firmware_name);
        if (access(firmware_name, R_OK)) {
            LogInfo("error, %s (%s)\n", firmware_name, strerror(errno));
            goto END;
        }

        if (usb2tcp_download_mode)
        {
            pdev = fibo_usb2tcp_open();
            if (pdev == NULL) {
                LogInfo("error: pdev is NULL\n");
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

    if ((pdev = CheckATPort(portname, syspath, 2)))
    {
        for(i=0; i<60; i++)
        {
            unisoc_switch_to_edl_mode(pdev);
            sleep(1);
            if (CheckDLPort(portname, syspath, 1000)) {
                break;
            }
            usleep(100*1000);
        }
    }

    if (!(pdev = CheckDLPort(portname, syspath, 30000))) {
        goto END;
    }

    if (fibo_usb_open(pdev, 1)) {
        return -1;
    }

    if (fibo_send_sciu2s_msg(pdev)) {
        return -1;
    }

DL_STEP:
    LogInfo("\n");
    pdev->usb_need_zero_package = qusb_zlp_mode;
    if (usb2tcp_server_mode == 0) //normal download mode
    {
        gettimeofday(&tv, NULL);
        old_time = (double)tv.tv_sec + (double)tv.tv_usec / 1000000;
        sleep(2);
        retval = unisoc_dloader_main(pdev, firmware_name);
    }
    else { //usb2tcp download mode
        retval = usb2tcp_server_main(pdev);
    }
END:
    fibo_usb_close(pdev);
    fibo_usb2tcp_close(pdev);
    if (usb2tcp_server_mode) { return retval; }
    if (g_dl_logfile_fp) { fclose(g_dl_logfile_fp); g_dl_logfile_fp = NULL;}
    if (usbmon_log_filename[0]) { fibo_usbmon_log_stop(); }

    LogInfo("-----------------------\n");
    if (retval == 0) {
        gettimeofday(&tv, NULL);
        new_time = (double)tv.tv_sec + (double)tv.tv_usec / 1000000;
        LogInfo("The total used time: %.3f s\n", new_time - old_time);
#if 0
        //start: After the download is successful, delete the download log.
        if (dl_log_filename[0] && !access(dl_log_filename, 0)) {
            if(!remove(dl_log_filename)) {
                LogInfo("Removed %s OK\n", dl_log_filename);
            }
        }
        //end: After the download is successful, delete the download log.
#endif
        LogInfo("Upgrade module successfully\n");
    } else {
        LogInfo("Upgrade module failed\n");
    }
    LogInfo("-----------------------\n");

    return retval;
}
