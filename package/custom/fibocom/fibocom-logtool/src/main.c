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
#include "misc_usb.h"

int socket_port = -1;
char tty_port[20] = {0};
int logtype = -1;
platform_enum g_platform=NONE;

#define FIBO_BUF_SIZE    512

#define TOOL_VERSION "Fibocom_MultiPlatform_logtool_V1.5.0.0"

fibo_usbdev_t *zte_find_devices_in_table(int idvendor, int idproduct);
fibo_usbdev_t *qcom_find_devices_in_table(int idvendor, int idproduct);
fibo_usbdev_t *uinisoc_find_devices_in_table(int idvendor, int idproduct);
fibo_usbdev_t *unisoc_x_find_devices_in_table(int idvendor, int idproduct);
fibo_usbdev_t *qcom_find_pcie_devices();
fibo_usbdev_t *udx710_find_pcie_devices();
fibo_usbdev_t *qcom_find_pciedump_devices();
fibo_usbdev_t *sl8563_x_find_devices_in_table(int idvendor, int idproduct);
fibo_usbdev_t *eigencomm_find_devices_in_table(int idvendor, int idproduct);
fibo_usbdev_t *samsung_find_devices_in_table(int idvendor, int idproduct);
fibo_usbdev_t *samsung_find_pcie_log_devices();


int hostproxy();

#define MAX_DEVICES 100
fibo_usbdev_t new_add_devices_table[MAX_DEVICES];
extern int unisoc_log_main(int argc, char** argv);
extern int zte_log_main(int argc, char **argv);
extern int unisoc_x_log_main(int argc, char **argv);
extern int eigencomm_log_main(int argc,char *argv[]);
extern int qcom_log_main(int argc, char **argv);
extern int qcom_dumplog_main(int argc, char **argv);
extern int unisoc_x_log_main(int argc, char** argv);
extern int samsung_log_main(int argc, char **argv);

void read_config_and_initialize_devices(const char *filename, fibo_usbdev_t *devices, int num_devices)
{

    char line[256];
    int device_index = 0;
    FILE *file = fopen(filename, "r");

    if (!file)
    {
        perror("Error opening file");
        return;
    }

    while (fgets(line, sizeof(line), file))
    {
        // Trim newline character if present
        line[strcspn(line, "\r\n")] = 0;

        // Check for start of a new device section
        if (line[0] == '[')
        {
            // Read subsequent lines for device configuration
            while (fgets(line, sizeof(line), file))
            {
                line[strcspn(line, "\r\n")] = 0; // Trim newline
                // End of device section reached
                if (line[0] == '[')
                   device_index++;

                // Parse key-value pairs
                char *key = strtok(line, "=");
                char *value = strtok(NULL, "=");

                if (key && value)
                {
                    // Assign values based on key
                    if (strcmp(key, "ModuleName") == 0)
                    {
                        strncpy(devices[device_index].ModuleName, value, FIBO_BUF_SIZE);
                    }
                    else if (strcmp(key, "idVendor") == 0)
                    {
                        devices[device_index].idVendor = strtol(value, NULL, 0);
                    }
                    else if (strcmp(key, "idProduct") == 0)
                    {
                        devices[device_index].idProduct = strtol(value, NULL, 0);
                    }
                    else if (strcmp(key, "ifnum") == 0)
                    {
                        devices[device_index].ifnum[0] = strtol(strtok(value, ","), NULL, 0);
                        devices[device_index].ifnum[1] = strtol(strtok(NULL, ","), NULL, 0);
                    }
                    else if (strcmp(key, "log_main_function") == 0)
                    {
                        // Assuming this is the function name to map to unisoc_log_main
                        #ifdef CONFIG_UNISOC
                        devices[device_index].log_main_function = unisoc_log_main;
                        #elif CONFIG_QCOM
                        devices[device_index].log_main_function = qcom_log_main;
                        #elif CONFIG_ZTE
                        devices[device_index].log_main_function = zte_log_main;
                        #elif CONFIG_EIGENCOMM
                        devices[device_index].log_main_function = eigencomm_log_main;
                        #elif CONFIG_QCOM_DUMP
                        devices[device_index].log_main_function = qcom_dumplog_main;
                        #elif CONFIG_UNISOC_X
                        devices[device_index].log_main_function = unisoc_x_log_main;
                        #elif CONFIG_SL8563_X
                        devices[device_index].log_main_function = sl8563_x_log_main;
                        #elif CONFIG_SAMSUNG
                        devices[device_index].log_main_function = samsung_log_main;
                        #else
                        devices[device_index].log_main_function = NULL;
                        #endif
                    }
                    else if (strcmp(key, "usbdev") == 0)
                    {
                        devices[device_index].usbdev = strtol(value, NULL, 0);
                    }
                    else if (strcmp(key, "ttyfd") == 0)
                    {
                        devices[device_index].ttyfd[0] = strtol(strtok(value, ","), NULL, 0);
                        devices[device_index].ttyfd[1] = strtol(strtok(NULL, ","), NULL, 0);
                    }
                    else if (strcmp(key, "bulk_ep_in") == 0)
                    {
                        devices[device_index].bulk_ep_in[0] = strtol(strtok(value, ","), NULL, 0);
                        devices[device_index].bulk_ep_in[1] = strtol(strtok(NULL, ","), NULL, 0);
                    }
                    else if (strcmp(key, "bulk_ep_out") == 0)
                    {
                        devices[device_index].bulk_ep_out[0] = strtol(strtok(value, ","), NULL, 0);
                        devices[device_index].bulk_ep_out[1] = strtol(strtok(NULL, ","), NULL, 0);
                    }
                    else if (strcmp(key, "wMaxPacketSize") == 0)
                    {
                        devices[device_index].wMaxPacketSize = strtol(value, NULL, 0);
                    }
                    else if (strcmp(key, "usb_need_zero_package") == 0)
                    {
                        devices[device_index].usb_need_zero_package = strtol(value, NULL, 0);
                    }
                    else if (strcmp(key, "write") == 0)
                    {
                        // Assuming write=NULL means no function assigned
                        devices[device_index].write = NULL;
                    }
                    else if (strcmp(key, "read") == 0)
                    {
                        // Assuming read=NULL means no function assigned
                        devices[device_index].read = NULL;
                    }
                }
            }

            // Move to the next device in the array
            device_index++;
        }
    }

    fclose(file);
}

static void fibo_add_new_table()
{

        int num_devices = 0;
        int i = 0;
        read_config_and_initialize_devices("devices.ini", new_add_devices_table, MAX_DEVICES);

        printf("Parsed %d devices from config.ini:\n", num_devices);
        for (i = 0; i < num_devices; ++i) {
            printf("Device %d:\n", i + 1);
            printf("  ModuleName: %s\n", new_add_devices_table[i].ModuleName);
            printf("  idVendor: %x\n", new_add_devices_table[i].idVendor);
            printf("  idProduct: %x\n", new_add_devices_table[i].idProduct);
            printf("  ifnum: {%d, %d}\n", new_add_devices_table[i].ifnum[0], new_add_devices_table[i].ifnum[1]);
            printf("  log_main_function: %p\n", new_add_devices_table[i].log_main_function);
            printf("  usbdev: %d\n", new_add_devices_table[i].usbdev);
            printf("  ttyfd: {%d, %d}\n", new_add_devices_table[i].ttyfd[0], new_add_devices_table[i].ttyfd[1]);
            printf("  bulk_ep_in: {%d, %d}\n", new_add_devices_table[i].bulk_ep_in[0], new_add_devices_table[i].bulk_ep_in[1]);
            printf("  bulk_ep_out: {%d, %d}\n", new_add_devices_table[i].bulk_ep_out[0], new_add_devices_table[i].bulk_ep_out[1]);
            printf("  wMaxPacketSize: %d\n", new_add_devices_table[i].wMaxPacketSize);
            printf("  usb_need_zero_package: %d\n", new_add_devices_table[i].usb_need_zero_package);
            printf("\n");
        }
}

fibo_usbdev_t *fibo_new_find_devices_in_table(int idvendor, int idproduct)
{
    int i, size = 0;

    size = sizeof(new_add_devices_table)/sizeof(new_add_devices_table[0]);
    for (i=0; i<size; i++)
    {
        fibo_usbdev_t *udev = &new_add_devices_table[i];

        if ((udev->idVendor == idvendor) && (udev->idProduct == idproduct)) {
            return udev;
        }
    }
    return NULL;
}

static fibo_usbdev_t *find_devices_in_table(int idvendor, int idproduct)
{
    fibo_usbdev_t *udev = NULL;

#ifdef CONFIG_ZTE
    if((g_platform == NONE) || (g_platform == V3E) || (g_platform == V3T))
    {
        udev = zte_find_devices_in_table(idvendor, idproduct);
        if (udev != NULL) {
            return udev;
        }
    }
#endif

#ifdef CONFIG_QCOM
    if((g_platform == NONE) || (g_platform == QCOM))
    {
        udev = qcom_find_devices_in_table(idvendor, idproduct);
        if (udev != NULL) {
            return udev;
        }
    }
#endif

#ifdef CONFIG_UNISOC_X
    if((g_platform == NONE) || (g_platform == UNISOC_X))
    {
        udev = unisoc_x_find_devices_in_table(idvendor, idproduct);
        if (udev != NULL) {
            return udev;
        }
    }
#endif

#ifdef CONFIG_UNISOC
    if((g_platform == NONE) || (g_platform == UNISOC))
    {
        udev = uinisoc_find_devices_in_table(idvendor, idproduct);
        if (udev != NULL) {
            return udev;
        }
    }
#endif

#ifdef CONFIG_EIGENCOMM
    if((g_platform == NONE) || (g_platform == EIGENCOMM))
    {
        udev = eigencomm_find_devices_in_table(idvendor, idproduct);
        if (udev != NULL) {
            return udev;
        }
    }
#endif

#ifdef CONFIG_SL8563_X
    if((g_platform == NONE) || (g_platform == SL8563))
    {
        udev = sl8563_x_find_devices_in_table(idvendor, idproduct);
        if (udev != NULL) {
            return udev;
        }
    }
#endif

#ifdef CONFIG_SAMSUNG
		if((g_platform == NONE) || (g_platform == SAMSUNG))
		{
			udev = samsung_find_devices_in_table(idvendor, idproduct);
			if (udev != NULL) {
				return udev;
			}
		}
#endif

    if(g_platform == NONE)
    {
        udev = fibo_new_find_devices_in_table(idvendor, idproduct);
        if (udev != NULL) {
            return udev;
        }
    }

    return NULL;
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
    char sys_filename[FIBO_BUF_SIZE] = {0};
    int idVendor = 0, idProduct = 0;
    int bNumInterfaces = 0, bConfigurationValue = 0;
    fibo_usbdev_t *udev = NULL, *ret_udev = NULL;
    unsigned modules_num = 0;
    char syspath[FIBO_BUF_SIZE] = {0};

    if (strStartsWith(portname, USB_DIR_BASE)) {
        strcpy(syspath, portname);
        portname[0] = 0; 
    }

#ifdef CONFIG_QCOM
    /*zhangboxing IRP-20230200297 2023/03/13  begin*/
    if(strcmp(portname,"/dev/mhi_DIAG")== 0)
    {
        udev = qcom_find_pcie_devices();
        return udev;
    }
    /*zhangboxing IRP-20230200297 2023/03/13  end*/
    if(strcmp(portname,"/dev/mhi_SAHARA")== 0)
    {
        udev = qcom_find_pciedump_devices();
        return udev;
    }
#endif

#ifdef CONFIG_UNISOC
    if(strcmp(portname,"/dev/sdiag_nr")== 0)
    {
        udev = udx710_find_pcie_devices();
        return udev;
    }

    if(strcmp(portname,"/dev/slog_nr")== 0)
    {
        udev = udx710_find_pcie_devices();
        return udev;
    }
#endif

#ifdef CONFIG_SAMSUNG
		if(strcmp(portname,"/dev/ttyMD_ap0")== 0)
		{
			udev = samsung_find_pcie_log_devices();
			return udev;
		}
		if(strcmp(portname,"/dev/ttyMD_dm0")== 0)
		{
			udev = samsung_find_pcie_log_devices();
			return udev;
		}

		if (strstr(portname,"tcp")) {
            		udev = find_devices_in_table(0xFFFF, 0xFF00);
            		return udev;
        	}
#endif


    usbdir = opendir(USB_DIR_BASE);
    if (usbdir == NULL) {
        return NULL;
    }

    while ((dent = readdir(usbdir)) != NULL)
    {
        if (strcmp(dent->d_name, ".") == 0 || strcmp(dent->d_name, "..") == 0) {
            continue;
        }

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

        udev = find_devices_in_table(idVendor, idProduct);
        if (udev != NULL) {
            int i = 0;
            printf("----------------------------------\n");
            printf("ModuleName: %s\n", udev->ModuleName);
            printf("idVendor: %04x\n", udev->idVendor);
            printf("idProduct: %04x\n", udev->idProduct);
            printf("bNumInterfaces: %d\n", bNumInterfaces);
            printf("bConfigurationValue: %d\n", bConfigurationValue);
            snprintf(sys_filename, sizeof(sys_filename), "%s/%s/uevent", USB_DIR_BASE, dent->d_name);
            // fibo_get_busname_by_uevent(sys_filename, udev->busname);
            // printf("busname: %s\n", udev->busname);
            for (i = 0; i<bNumInterfaces; i++) {
                snprintf(udev->syspath, sizeof(udev->syspath), "%s/%s/%s:%d.%d", USB_DIR_BASE, dent->d_name, dent->d_name, bConfigurationValue, i);
                memset(udev->portname, 0, sizeof(udev->portname));
                fibo_get_ttyport_by_syspath(udev);
                printf("portname: %s -- %s\n", udev->portname, udev->syspath);
                printf("%s , %s; %s , %s\r\n", syspath, udev->syspath, portname, udev->portname);

                if ((syspath[0] && !strcmp(syspath, udev->syspath)) || (portname[0] &&!strcmp(portname, udev->portname))) {
                    udev->ifnum[0] = i;
                    ret_udev = udev;
                    modules_num = 1;
                    printf("find the ifnum is %d\r\n", udev->ifnum[0]);
                    printf("----------------------------------\n");
                    goto END;
                }
                memset(udev->portname, 0, sizeof(udev->portname));
            }
            // 2024-12-03, Shijiaxing, MTC0233-495, Begin. Haikang issue: support multiple device at the same time by specify portname.
            // use the given portname and last match udev.
            memcpy(udev->portname, portname, sizeof(udev->portname));
            ret_udev = udev;
            modules_num++;
            printf("----------------------------------\n");
            // 2024-12-03, Shijiaxing, MTC0233-495, #nd. Haikang issue: support multiple device at the same time by specify portname.
        }
        usleep(10000);
    }

END:
    if (usbdir) {
        closedir(usbdir);
        usbdir = NULL;
    }

    if (modules_num == 0) {
        printf("Can not find Fibocom module.\n");
    }
    //2024-12-03, Shijiaxing, MTC0233-495, Begin. Haikang issue: support multiple device at the same time by specify portname.
    //choose the last match udev, later use the given portname.
    else if (ret_udev != NULL) {
        printf("%d module match.\n", modules_num);
        return ret_udev;
    }
    else if (portname[0] == 0 && syspath[0] == 0) {
        printf("%d modules found\n", modules_num);
        printf("please set portname <-d /dev/XXX> or set syspath <-d /sys/bus/usb/devices/X-X/X-X:X.X>\n");
    }
    // 2024-12-03, Shijiaxing, MTC0233-495, End. Haikang issue: support multiple device at the same time by specify portname.

    return NULL;
}

int main(int argc, char **argv)
{
    int opt = -1;
    char portname[FIBO_BUF_SIZE] = {0};
    fibo_usbdev_t *udev = NULL;

    printf("TOOL_VERSION:%s\n", TOOL_VERSION);

    umask(0);
    optind = 1;//must set to 1
    while ((opt = getopt(argc, argv, "d:p:P:M:s:a:m:n:i:u:w:f:t:hvc:e:g:")) != -1)/* Modify for MBB0201-84 20240525 willa.liu */
    {
        switch (opt)
        {
            case 'd':
                strcpy(portname, optarg);
                strcpy(tty_port,portname);
                break;
            case 'p':
                strcpy(portname, optarg);
                strcpy(tty_port,portname);
                break;
            case 'e':
                strcpy(portname, optarg);
                strcpy(tty_port,portname);
                break;
            case 'P':
                socket_port = atoi(optarg);
                break;
            case 'a':
                logtype = 2;
                break;
            case 't':
                g_platform = atoi(optarg);
                break;
            default:;
        }
    }

    if(socket_port != -1)
    {
        //hostproxy
        if((socket_port < 9000 || socket_port > 9999) && strlen(tty_port)!= 0)
        {
            //hostproxy
            hostproxy();
        }
        else if(socket_port >= 9000 && socket_port <= 9999)
        {
            printf("UDX710 Port proxy mode\n");
        }
        else
        {
            printf("The port is not specified or the proxy port is incorrectly specified\n");
            printf("Example:./logtool -P 8888 -d /dev/ttyUSB0\n");
            return 0;
        }
    }
    fibo_add_new_table();

    udev = fibo_dectect_fibocom_modules(portname);
    if (udev == NULL) {
        return -1;
    }

    return udev->log_main_function(argc, argv);
}
