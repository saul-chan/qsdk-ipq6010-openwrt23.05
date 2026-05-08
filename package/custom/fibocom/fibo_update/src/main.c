/*******************************************************************
 *  CopyRight(C) 2022-2026  Fibocom Wireless Inc
 *******************************************************************
 * FileName : main.c
 * Author   : Frank.zhou
 * Date     : 2022.07.26
 * Used     : Fibocom_MultiPlatform_log_tool
 *******************************************************************/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "misc.h"
#include "usb2tcp.h"
#include "misc_usb.h"

#define TOOL_VERSION "Fibocom_Linux_Firmware_Upgrade_V1.4.0.0"

#define USB_DIR_BASE    "/sys/bus/usb/devices"

#define QUCM_DL 0x05c6
#define UNISOC_1_DL 0x4D00
#define UNISOC_2_DL 0x0A09
#define SMR_1_DL 0x18d1

static int g_idVendor = 0, g_idProduct = 0;

extern int zte_download_main(int argc, char **argv);
extern int qcom_download_main(int argc, char **argv);
extern int unisoc_download_main(int argc, char **argv);
extern int32_t eigencomm_download_main(int32_t argc, char **argv);
extern int samsung_download_main(int argc, char **argv);

extern fibo_usbdev_t *zte_find_devices_in_table(int idvendor, int idproduct, int check_port_type);
extern fibo_usbdev_t *qcom_find_devices_in_table(int idvendor, int idproduct, int check_port_type);
extern fibo_usbdev_t *unisoc_find_devices_in_table(int idvendor, int idproduct, int check_port_type);
extern fibo_usbdev_t *eigencomm_find_devices_in_table(int idvendor, int idproduct, int check_port_type);
extern fibo_usbdev_t *smr_find_devices_in_table(int idvendor, int idproduct, int check_port_type);

static fibo_usbdev_t *find_devices_in_table(int idvendor, int idproduct)
{
    fibo_usbdev_t *udev = NULL;

    udev = zte_find_devices_in_table(idvendor, idproduct, 2);
    if (udev != NULL) {
        return udev;
    }

    udev = qcom_find_devices_in_table(idvendor, idproduct, 2);
    if (udev != NULL) {
        return udev;
    }

    udev = unisoc_find_devices_in_table(idvendor, idproduct, 2);
    if (udev != NULL) {
        return udev;
    }

    udev = smr_find_devices_in_table(idvendor, idproduct, 2);
    if (udev != NULL) {
        return udev;
    }
#ifdef CONFIG_EIGENCOMM
    udev = eigencomm_find_devices_in_table(idvendor, idproduct, 2);
    if (udev != NULL) {
        return udev;
    }
#endif

    return NULL;
}

static int strStartsWith(const char *line, const char *src)
{
    for ( ; *line != '\0' && *src != '\0'; line++, src++) {
        if (*line != *src) {
            return 0;
        }
    }

    return *src == '\0';
}

static int fibo_get_usbsys_val(const char *sys_filename, int base)
{
    char buff[64] = {0};
    int ret_val = -1;

    int fd = open(sys_filename, O_RDONLY);
    if (fd < 0) {
        return -1;
    }

    if (read(fd, buff, sizeof(buff)) <= 0) {
        printf("[%s] read:%s failed\n", __func__, sys_filename);
    }
    else {
        ret_val = strtoul(buff, NULL, base);
    }
    close(fd);

    return ret_val;
}


static fibo_usbdev_t *fibo_dectect_fibocom_modules(char *portname)
{
    DIR *usbdir = NULL;
    struct dirent *dent = NULL;
    char sys_filename[MAX_PATH_LEN] = {0};
    int idVendor = 0, idProduct = 0;
    int bNumInterfaces = 0, bConfigurationValue = 0;
    fibo_usbdev_t *udev = NULL, *ret_udev = NULL;
    int modules_num = 0;
    char syspath[MAX_PATH_LEN] = {0};

    if (strStartsWith(portname, USB_DIR_BASE)) {
        strcpy(syspath, portname);
        portname[0] = 0;
    }

    usbdir = opendir(USB_DIR_BASE);
    if (usbdir == NULL) {
        return NULL;
    }

    while ((dent = readdir(usbdir)) != NULL)
    {
        if (!strstr(portname, ":9008")) {
            if (!strcmp(dent->d_name, ".") || !strcmp(dent->d_name, "..") || strStartsWith(dent->d_name, "usb")) {
                continue;
            }
            // printf("d_name: %s\n", dent->d_name);
            snprintf(sys_filename, sizeof(sys_filename), "%s/%s/idVendor", USB_DIR_BASE, dent->d_name);
            if ((idVendor = fibo_get_usbsys_val(sys_filename, 16)) <= 0) {
                continue;
            }

            snprintf(sys_filename, sizeof(sys_filename), "%s/%s/idProduct", USB_DIR_BASE, dent->d_name);
            if ((idProduct = fibo_get_usbsys_val(sys_filename, 16)) <= 0) {
                continue;
            }

            snprintf(sys_filename, sizeof(sys_filename), "%s/%s/bConfigurationValue", USB_DIR_BASE, dent->d_name);
            if ((bConfigurationValue = fibo_get_usbsys_val(sys_filename, 10)) <= 0) {
                continue;
            }

            snprintf(sys_filename, sizeof(sys_filename), "%s/%s/bNumInterfaces", USB_DIR_BASE, dent->d_name);
            if ((bNumInterfaces = fibo_get_usbsys_val(sys_filename, 10)) <= 0) {
                continue;
            }
        } else {
            idVendor = g_idVendor;
            idProduct = g_idProduct;
        }

        udev = find_devices_in_table(idVendor, idProduct);
        if (udev != NULL) {
            int i = 0;
            ret_udev = udev;
            printf("----------------------------------\n");
            printf("ModuleName: %s\n", udev->ModuleName);
            printf("idVendor: %04x\n", udev->idVendor);
            printf("idProduct: %04x\n", udev->idProduct);
            printf("bNumInterfaces: %d\n", bNumInterfaces);
            printf("bConfigurationValue: %d\n", bConfigurationValue);
            // snprintf(sys_filename, sizeof(sys_filename), "%s/%s/uevent", USB_DIR_BASE, dent->d_name);
            // fibo_get_busname_by_uevent(sys_filename, udev->busname);
            // printf("busname: %s\n", udev->busname);
            for (i = 0; i<bNumInterfaces; i++) {
                snprintf(udev->syspath, sizeof(udev->syspath), "%s/%s/%s:%d.%d", USB_DIR_BASE, dent->d_name, dent->d_name, bConfigurationValue, i);
                fibo_get_ttyport_by_syspath(udev);
                printf("portname: %s -- %s\n", udev->portname, udev->syspath);

                if ((syspath[0] && !strcmp(syspath, udev->syspath)) || (portname[0] &&!strcmp(portname, udev->portname))) {
                    udev->used_ifnum = i;
                    modules_num = 1;
                    goto END;
                }
                memset(udev->portname, 0, sizeof(udev->portname));
            }
            modules_num++;
            printf("----------------------------------\n");
            if (strstr(portname, ":9008")) {
                goto END;
            }
        }
        usleep(10000);
    }

END:
    if (usbdir) {
        closedir(usbdir);
        usbdir = NULL;
    }

    if (modules_num == 0) {
        // printf("Can not find Fibocom module.\n");
    }
    else
    if (modules_num == 1) {
        printf("%d module match.\n", modules_num);
        return ret_udev;
    } else if (!strcmp(ret_udev->ModuleName,"SMR5300 DL")) {
        printf("%d module match.\n", modules_num);
        return ret_udev;
    } else if (portname[0] == 0 && syspath[0] == 0) {
        printf("%d modules found\n", modules_num);
        printf("please set portname <-d /dev/XXX> or set syspath <-d /sys/bus/usb/devices/X-X/X-X:X.X>\n");
    }

    return NULL;
}

int main(int argc, char **argv)
{
    int i, opt = -1;
    char portname[MAX_PATH_LEN] = {0};
    char ipname[MAX_PATH_LEN] = {0};
    fibo_usbdev_t *udev = NULL;
    int module_type = -1;

    printf("\n===============================================\n");
    printf("%s\n", TOOL_VERSION);
    printf("===============================================\n");

    if (geteuid() != 0) {
        printf("%s must run by root user or use sudo\n", argv[0]);
        return -1;
    }

    //Bolian machine problem
    system("mount -t devtmpfs none /dev");

    optind = 1;//must set to 1
    while ((opt = getopt(argc, argv, "d:p:f:t:b:l:r:ez:u:whc:")) != -1)
    {
        switch (opt)
        {
            case 'd':
                strncpy(portname, optarg, MAX_PATH_LEN);
                break;
            case 'p':
                strncpy(ipname, optarg, MAX_PATH_LEN);
                break;
            case 't':
                if (!strcasecmp(optarg, "zte") || !strcasecmp(optarg, "1")) {
                    module_type = 1;
                }
                else if (!strcasecmp(optarg, "qcom") || !strcasecmp(optarg, "2")) {
                    module_type = 2;
                }
                else if (!strcasecmp(optarg, "unisoc") || !strcasecmp(optarg, "3")) {
                    module_type = 3;
                }
                else if (!strcasecmp(optarg, "eigencomm") || !strcasecmp(optarg, "4")) {
                    module_type = 4;
                }
                else if (!strcasecmp(optarg, "samsung") || !strcasecmp(optarg, "5")) {
                    module_type = 5;
                }
                else {
                    printf("-t param is invalid\n");
                }
                break;
            case 'h':
                zte_download_main(argc, argv);
                qcom_download_main(argc, argv);
                unisoc_download_main(argc, argv);
#ifdef CONFIG_EIGENCOMM
                eigencomm_download_main(argc, argv);
#endif
                return 0;
            default:;
        }
    }

    if (strstr(ipname, ":9008")) {
        if (tcp_connect_module_host(ipname, &g_idVendor, &g_idProduct)) {
            return -1;
        }
        if (g_idVendor == QUCM_DL){
            module_type=2;
        }
        else if(g_idProduct == UNISOC_1_DL || g_idProduct == UNISOC_2_DL){
            module_type=3;
        } else if(g_idVendor == SMR_1_DL ){
            module_type=5;
        }
    }

    switch (module_type)
    {
        case 1: return zte_download_main(argc, argv);
        case 2: return qcom_download_main(argc, argv);
        case 3: return unisoc_download_main(argc, argv);
#ifdef CONFIG_EIGENCOMM
        case 4: return eigencomm_download_main(argc, argv);
#endif
        case 5: return samsung_download_main(argc, argv);
        default:;
    }

    for(i=0; i<60; i++) {
        if(fibo_strStartsWith(portname, "/dev/mhi")){
            printf("Is QCOM PCIe modestart to run QCOM main function\n");
            return qcom_download_main(argc, argv);
        }
        udev = fibo_dectect_fibocom_modules(portname);
        if (udev != NULL) {
            
            if (udev->main_function == NULL) {
                printf("udev->main_function is NULL\n");
                return -1;
            }
            
            printf("start to run [%s] main function\n", udev->ModuleName);
            return udev->main_function(argc, argv);
        }
        if (i == 0) {
            printf("No fibocom modules found, wait for fibocom modules connected.\n");
        }
        usleep(500*1000);
    }

    return -1;
}
