/*******************************************************************
 *  CopyRight(C) 2022-2026  Fibocom Wireless Inc
 *******************************************************************
 * FileName : qcom_main.c
 * DESCRIPTION : upgrade_tool for USB and PCIE of Fibocom modules
 * Author   : Frank.zhou
 * Date     : 2022.08.22
 *******************************************************************/
#include "md5sum.h"
#include "firehose_download.h"
#include "qcom_devices_list.h"
#include "usb2tcp.h"
#include "pcie_download.h"

fibo_usbdev_t *qcom_find_devices_in_table(int idvendor, int idproduct, int check_port_type)
{
    int i, size = 0;
    fibo_usbdev_t *pdev = NULL;

    // LogInfo("start\n");
    if (check_port_type) {
        size = sizeof(qcom_devices_table)/sizeof(qcom_devices_table[0]);
        for (i=0; i<size; i++)
        {
            pdev = &qcom_devices_table[i];
            if ((pdev->idVendor == idvendor) && (pdev->idProduct == idproduct)) {
                goto FIND;
            }
        }

        if (check_port_type == 1) {
            return NULL;
        }
    }

    size = sizeof(dl_qcom_devices_table)/sizeof(dl_qcom_devices_table[0]);
    for (i=0; i<size; i++)
    {
        pdev = &dl_qcom_devices_table[i];
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

#if 0
int at_switch_to_edl_mode(void *pdev)
{
    char *pcmd = "at+syscmd=\"sys_reboot edl\"\r\n";
    int ret = -1, rx_len = 0;

    pcmd = "at+gtdlmode=edl\r\n";
    if (fibo_usb_open(pdev, 0)) {
        LogInfo("failed.\n");
        return -1;
    }
    rx_len = write(pdev->ttyfd, pcmd, strlen(pcmd), 1, 1000);
    if (rx_len < 0) {
        LogInfo("failed.\n");
        goto END;
    }
    LogInfo("OK, please wait.\n");
    ret = 0;
END:
    fibo_usb_close(pdev);

    return ret;
}
#endif

// send the diag command to switch the Qualcomm device to EDL mode.
int qcom_switch_to_edl_mode(fibo_usbdev_t *pdev)
{
    char edl_cmd[] = {0x4b, 0x65, 0x01, 0x00, 0x54, 0x0f, 0x7e};
    char pbuf[MAX_PATH_LEN] = {0};
    int ret = -1, rx_len = 0;

    if (fibo_usb_open(pdev, 0)) {
        LogInfo("failed.\n");
        goto END;
    }

    do {
        rx_len = pdev->read(pdev, pbuf , sizeof(pbuf), 0, 3000);
    } while (rx_len > 0);

    LogInfo("send switch to edl mode cmd, please wait.\n");
    rx_len = pdev->write(pdev, edl_cmd, sizeof(edl_cmd), sizeof(edl_cmd), 3000);
    if (rx_len < 0) {
        LogInfo("failed.\n");
        goto END;
    }

    do {
        rx_len = pdev->read(pdev, pbuf , sizeof(pbuf), 0, 3000);
        if (rx_len == sizeof(edl_cmd) && memcmp(pbuf, edl_cmd, sizeof(edl_cmd)) == 0)
        {
            LogInfo("wait module reboot to edl mode\n");
            ret = 0;
            goto END;
        }
    } while (rx_len > 0);

END:
    fibo_usb_close(pdev);

    return ret;
}

static fibo_usbdev_t *CheckATPort(char *portname, char *syspath, int times)
{
    int i = 0;

    for (i=0; i<times; i++) {
        fibo_usbdev_t *pdev = fibo_get_fibocom_device(qcom_find_devices_in_table, portname, syspath, 1);
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
    LogInfo("CheckDLPort times is %d\n", times);
    for(i=0; i<times; i++) {
        fibo_usbdev_t *pdev = fibo_get_fibocom_device(qcom_find_devices_in_table, portname, syspath, 0);
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


//start: mdm9x07 and mdm9205 modules need -r 1 to automatically restore NV, frank.zhou 2022.01.22
#define ODM_EFS2_RESTORE_MODE    0x474E4F53
typedef struct oem_ops_stru {
    uint32_t efs_flag;
    uint32_t efs_count;
    uint32_t fw_flag;
    uint32_t fw_count;
} oem_ops_t;

static int mdm9x07_set_nv_restore_flag(char *path)
{
    char filename[MAX_PATH_LEN] = {0};
    char buffer[4096] = {0};
    oem_ops_t *oem_ops = (oem_ops_t *)buffer;

    oem_ops->efs_flag = ODM_EFS2_RESTORE_MODE;
    oem_ops->efs_count =5;//count must > TRIGGER_RESTORE_COUNT
    LogInfo("set efs flag OK.\n");

    //fota.bin
    snprintf(filename, sizeof(filename), "%s/fota.bin", path);
    if (!access(filename, F_OK)) {
        FILE *fp = fopen(filename, "wb+");
        if (fp == NULL) {
            LogInfo("open %s failed.\n", filename);
            return -1;
        }
        if (fwrite(buffer, 1, sizeof(buffer), fp) != sizeof(buffer)) {
            LogInfo("fwrite %s failed.\n", filename);
            fclose(fp);
            return -1;
        }
        fclose(fp);
    }

    //oeminfo.bin
    snprintf(filename, sizeof(filename), "%s/oeminfo.bin", path);
    if (!access(filename, F_OK)) {
        FILE *fp = fopen(filename, "wb+");
        if (fp == NULL) {
            LogInfo("open %s failed.\n", filename);
            return -1;
        }
        if (fwrite(buffer, 1, sizeof(buffer), fp) != sizeof(buffer)) {
            LogInfo("fwrite %s failed.\n", filename);
            fclose(fp);
            return -1;
        }
        fclose(fp);
        LogInfo("fwrite oeminfo OK.\n");
    }

    LogInfo("set nv restore to %s OK.\n", filename);

    return 0;
}
//end: mdm9x07 and mdm9205 modules need -r 1 to automatically restore NV, frank.zhou 2022.01.22

/* Modify for MBB0046-324 20230711 zhangboxing begin*/
#if 0
static void fibo_upgrade_result_save(int debug, const char *result)
{
    if (debug) {
        static char log_filename[128] = {0};
        FILE *fstream = NULL;

        if (result == NULL) {
            LogInfo("result is NULL.\n");
            return;
        }

        if (strstr(result, "UPGRADING")) {
            char cur_dir[MAX_PATH_LEN] = {0};

            getcwd(cur_dir, sizeof(cur_dir));
            sprintf(log_filename, "%s/log_filename.txt", cur_dir);
            fstream = fopen(log_filename, "w");
        } else {
            fstream = fopen(log_filename, "w+");
            if (fstream == NULL) {
                printf("open file %s failed!\n", log_filename);
                return;
            }
        }

        fputs(result, fstream);
        fclose(fstream);
    }
}
#endif
/* Modify for MBB0046-324 20230711 zhangboxing end*/

static void usage(const char *program_name)
{
    printf("------------------------------------------------------\n");
    printf("            <qcom upgrade help info>      \n");
    printf("Usage: %s [options...]\n", program_name);
    printf(" -f [firmware image dir]        for example: -f /XX/Maincode \n");
    printf(" -d [/dev/ttyUSBx]              Diagnose port, will auto-detect if not specified\n");
    printf(" -d [/sys/bus/usb/devices/xx]   When multiple modules exist on the board, use -s specify which module you want to upgrade\n");
    printf(" -l [log dir]                   Save the download log to file\n");
    printf(" -z [0/1]                       usb_need_zero_package, default is 0\n");
    printf(" -r [0 or 1] NV Restore, only for MDM9x07 and MDM9205 Models.\n");
    printf(" -e                             Erase All Before Download (This will Erase NV params, Generally not used)\n");
    printf(" -h  help info\n"); 
    printf("Applicable Models:\n"
            /* "        SDX55/65: FM150 FG150 FM160 FG160\n" */
            "        SDX12   : FM101 FG101\n"
            "        MDM9X07 : NL668 LC116 MC116\n"
            "        MDM9205 : MA510 MC109\n");
    printf("------------------------------------------------------\n");
}

int qcom_download_main(int argc, char **argv)
{
    int retval = -1, opt = -1, i = 0;
    int qusb_zlp_mode = 0; //default -z 0, FM150 and FM160 need to set -z 1.
    double old_time = 0, new_time = 0;
    char firmware_name[MAX_PATH_LEN] = {0};
    char xml_firmware_name[MAX_PATH_LEN*2] = {0};
    char portname[MAX_PATH_LEN] = {0}, syspath[MAX_PATH_LEN] = {0};
    char dl_log_filename[MAX_PATH_LEN] = {0}, usbmon_log_filename[MAX_PATH_LEN] = {0};
    int usb2tcp_server_mode = 0; //tcp server mode
    int usb2tcp_download_mode = 0; //tcp client mode
    fibo_usbdev_t *pdev = NULL;
    struct timeval tv;
    int q_erase_all_before_download = 0; //eraseall before download

    int mdm9x07_9250_nv_restore_flag = 0; //mdm 9x07 set nv restore flag
    int package_md5_check = 0; // 7z package md5 check

    LogInfo("start\n");
    umask(0);
    
    if (argc == 1) {
        LogInfo("argv[0]: %s\n", argv[0]);
    }
    optind = 1;//must set to 1
    while (-1 != (opt = getopt(argc, argv, "f:b:p:d:z:s:l:u:nehr:c:")))
    {
        switch (opt)
        {
            case 'b':
            case 'f':
                strcpy(firmware_name, optarg); break;
            case 'c':
                strcpy(xml_firmware_name, optarg); break;
            case 'l':
                snprintf(dl_log_filename, sizeof(dl_log_filename), "%s/fibo_download_%lu.log", optarg, time(NULL));
                break;
            case 'd':
                strncpy(portname, optarg, MAX_PATH_LEN);
                break;
            case 'p':
                strncpy(portname, optarg, MAX_PATH_LEN);
                if (!strcmp(portname, "9008"))
                {
                    usb2tcp_server_mode = 1;
                    LogInfo("usb2tcp_server_mode: %d\n", usb2tcp_server_mode);
                }
                else if (strstr(portname, ":9008"))
                {
                    usb2tcp_download_mode = 1;
                    memset(portname, 0, sizeof(portname));
                    LogInfo("usb2tcp_download_mode: %d\n", usb2tcp_download_mode);
                }
                break;
            case 'z':
                qusb_zlp_mode = !!atoi(optarg);
                break;
            case 'n':
                package_md5_check = 1;
                break;
            case 'e':
                q_erase_all_before_download = 1;
                LogInfo("q_erase_all_before_download: %d\n", q_erase_all_before_download);
                break;
            //start: mdm9x07 modules add -r 1 param to write nv restore flag into oeminfo.bin, frank.zhou 2022.01.22
            case 'r':
                mdm9x07_9250_nv_restore_flag = !!atoi(optarg);
                break;
            //end: mdm9x07 modules add -r 1 param to write nv restore flag into oeminfo.bin, frank.zhou 2022.01.22
            case 'u':
                strncpy(usbmon_log_filename, optarg, sizeof(usbmon_log_filename));
                break;
            case 'h':
                usage(argv[0]);
                return 0;
            default:
            break;
        }
    }

    if (usb2tcp_server_mode == 0)
    {
        if (firmware_name[0] == 0) {
            LogInfo("firmware_name is empty. required -f [package image dir]\n");
            usage(argv[0]);
            return -1;
        }

        if (dl_log_filename[0]) {
            g_dl_logfile_fp = fopen(dl_log_filename, "w+");
            if (g_dl_logfile_fp) {
                LogInfo("upgrade log will be save to %s\n", dl_log_filename);
            }
            else{
                LogInfo("upgrade log create failed! Check the path and operation permission!\n");
                goto END;
            }
        }

        //7z firmware package
        if(strstr(firmware_name, ".7z") != NULL)
        {
            char cmd[MAX_PATH_LEN*2] = {0};
            char *ptmp = NULL;

            if (access(firmware_name, R_OK)) {
                LogInfo("error, %s (%s)\n", firmware_name, strerror(errno));
                goto END;
            }

            // check the md5 value of the upgrade file
            if (package_md5_check && md5_check(firmware_name))
            {
                LogInfo("md5 check [%s] failed.\n", firmware_name);
                goto END;
            }

            sprintf(cmd,"7z x %s", firmware_name);
            system(cmd);

            //remove '.7z' in the firmware_name
            ptmp = strstr(firmware_name, ".7z");
            if (ptmp) {
                *ptmp = 0;
            }

            if (strstr(firmware_name, "/Maincode") == NULL) {
                strcat(firmware_name, "/Maincode");
            }
        }

        if (access(firmware_name, R_OK)) {
            LogInfo("error, %s (%s)\n", firmware_name, strerror(errno));
            goto END;
        }

        //start: mdm9x07 modules add -r 1 param to write nv restore flag into oeminfo.bin, frank.zhou 2022.01.22
        if (mdm9x07_9250_nv_restore_flag) {
            if (mdm9x07_set_nv_restore_flag(firmware_name)) {
                goto END;
            }
        }
        //end: mdm9x07 modules add -r 1 param to write nv restore flag into oeminfo.bin, frank.zhou 2022.01.22

        //fibo_upgrade_result_save(dl_log_filename[0], "UPGRADING"); //Modify for MBB0046-324 20230711 zhangboxing
        if (usb2tcp_download_mode)
        {
            pdev = fibo_usb2tcp_open();
            if (pdev == NULL) {
                LogInfo("error: pdev is NULL\n");
                goto END;
            }
            goto DL_STEP;
        }

        if (fibo_strStartsWith(portname, "/dev/mhi"))
        {
            pdev = pcie_open_dl(firmware_name, portname);
            if (pdev == NULL) {
                goto END;
            }
            goto __firehose_main;
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
            fibo_common_send_atcmd(pdev, "AT+GTDIAGEN=1\r\n");
            qcom_switch_to_edl_mode(pdev);
            sleep(1);
            if (CheckDLPort(portname, syspath, 500)) {
                break;
            }
            usleep(100*1000);
        }
    }

    if (!(pdev = CheckDLPort(portname, syspath, 30000))) {
        goto END;
    }

    if (fibo_usb_open(pdev, 1))
    {
        LogInfo("open usb node failed.\n");
        if (fibo_usb_open(pdev, 0))
        {
            LogInfo("open ttyUSB failed.\n");
            return -1;
        }
    }

DL_STEP:
    pdev->usb_need_zero_package = qusb_zlp_mode;
    if (usb2tcp_server_mode == 0) //normal download mode
    {
        gettimeofday(&tv, NULL);
        old_time = (double)tv.tv_sec + (double)tv.tv_usec / 1000000;
        retval = sahara_download_main(firmware_name, pdev, 1);
        if (retval) {
            goto END;
        }
__firehose_main:
        sleep(2);
        pdev->erase_all_before_download = q_erase_all_before_download;
        retval = firehose_download_main(firmware_name, pdev, mdm9x07_9250_nv_restore_flag, xml_firmware_name);
    }
    else { //usb2tcp mode
        retval = usb2tcp_server_main(pdev);
    }
END:
    fibo_usb_close(pdev);
    fibo_usb2tcp_close(pdev);
    if (usb2tcp_server_mode) { return retval; }

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
        //fibo_upgrade_result_save(dl_log_filename[0], "OK"); //Modify for MBB0046-324 20230711 zhangboxing
    } else {
        LogInfo("Upgrade module failed\n");
        //fibo_upgrade_result_save(dl_log_filename[0], "ERROR"); //Modify for MBB0046-324 20230711 zhangboxing
    }
    LogInfo("-----------------------\n");
    if (g_dl_logfile_fp) { fclose(g_dl_logfile_fp); g_dl_logfile_fp = NULL;}
    if (usbmon_log_filename[0]) { fibo_usbmon_log_stop(); }

    return retval;
}
