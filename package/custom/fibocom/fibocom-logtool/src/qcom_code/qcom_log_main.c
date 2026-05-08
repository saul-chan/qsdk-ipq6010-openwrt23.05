/*******************************************************************
 *          CopyRight(C) 2022-2026  Fibocom Wireless Inc
 *******************************************************************
 * FileName : zte_trace_tool.c
 * Author   : Frank.zhou
 * Date     : 2022.05.25
 * Used     : Capture QCOM module's diag log
 *******************************************************************/
#include "qcom_devices_list.h"
#include "log_control.h"
#include <sys/statfs.h>
#include "dump_api.h"

#define TFTP                   1
#define DUMP_LOG_NEED_SPACE    512
#define BUF_SIZE               1024
#define MAXSZ                  128
#define DEVICES_SIZE           64
#define PCIE_DEVICES           7
#define PCIE_DUMP_DEVICES      6

#define RX_URB_SIZE                     (16*1024)
#define QCOM_QXDM_LOG_FILE_SIZE         (50 * 1024 * 1024)
#define QCOM_QXDM_LOG_FILE_MIN_SIZE     (1 * 1024 * 1024)
#define QCOM_QXDM_LOG_FILE_MAX_SIZE     (2000 * 1024 * 1024)
#define QCOM_QXDM_LOG_WRITE_SIZE        (64 * 1024)
#define QCOM_QXDM_LOG_READ_SIZE         (1 * 1024 * 1024)

#define PCIE_AT_PORT        "/dev/mhi_DUN"
#define PORT_NAME_PREFIX    "/dev/ttyUSB"
#define TFTP_F              "tftp:"

extern int qkfifo_idx(int fd);
extern size_t qkfifo_write(int idx, const void *buf, size_t size);
extern void qkfifo_free(int idx);
extern int qkfifo_alloc(int fd);
extern int tftp_write_request(const char *filename, long tsize);
extern int tftp_test_server(const char *serv_ip);
extern int sahara_catch_dump(int port_fd, const char *path_to_save_files, int do_reset);
extern const char *g_tftp_server_ip;

static int g_fd_port = -1;
static volatile int g_is_qxdm_logging = 0;
static volatile int g_exit_flag = 0;

char g_qcom_logdir[BUF_SIZE] = {0};
char g_str_sub_log_dir[MAXSZ] = {0};
char g_qxdm_log_filename[MAXSZ] = {0};

int g_qxdm_log_file_size = QCOM_QXDM_LOG_FILE_SIZE;
int g_log_file_maxNum = 0;

int s_socket_fd = -1;
int s_socket_port = -1;

typedef struct {
    char username[MAXSZ];
    char password[MAXSZ];
    char ftp_ipaddr[MAXSZ];
} ftp_info_t;
char g_ip_addr[BUF_SIZE] = {0};


volatile int ftp_put_flag = 0;
char g_ftp_put_filename[MAXSZ] = {0};

static bool s_use_adb = false;
static bool s_use_socket = false;
static bool s_nottyUSB = false;

static arguments_t qlog_args = {
    -1,
    -1,
    {-1, -1},
    NULL
};

static int qlog_init_filter_finished = 0;
//static int g_logfile_num = 0;
pthread_mutex_t mutex; /*resolve the bug g_mdm_req issue (mantis 63061), yanghaitao 2020.11.25 */

fibo_usbdev_t *qcom_find_devices_in_table(int idvendor, int idproduct)
{
    int i, size = 0;

    size = sizeof(qcom_devices_table)/sizeof(qcom_devices_table[0]);
    for (i=0; i<size; i++)
    {
        fibo_usbdev_t *udev = &qcom_devices_table[i];

        if ((udev->idVendor == idvendor) && (udev->idProduct == idproduct)) {
            return udev;
        }
    }

    return NULL;
}

/*zhangboxing IRP-20230200297 2023/03/13  begin*/
fibo_usbdev_t *qcom_find_pcie_devices()
{
    return &qcom_devices_table[PCIE_DEVICES];
}
/*zhangboxing IRP-20230200297 2023/03/13  end*/


fibo_usbdev_t *qcom_find_pciedump_devices()
{
    return &qcom_devices_table[PCIE_DUMP_DEVICES];
}

static int fibo_get_busname_by_uevent(const char *uevent, char *busname)
{
    FILE *fp = NULL;
    char line[FIBO_BUF_SIZE] = {0};
    int MAJOR = 0, MINOR = 0;
    char DEVTYPE[DEVICES_SIZE] = {0}, PRODUCT[DEVICES_SIZE] = {0};

    fp = fopen(uevent, "r");
    if (fp == NULL) {
        printf("fail to fopen %s, errno:%d(%s)\n", uevent, errno, strerror(errno));
        return -1;
    }

    // printf("----------------------------------------\n");
    // printf("%s\n", uevent);
    while (fgets(line, sizeof(line), fp))
    {
        if (line[strlen(line) - 1] == '\n' || line[strlen(line) - 1] == '\r') {
            line[strlen(line) - 1] = 0;
        }

        // printf("%s\n", line);
        if (strStartsWith(line, "MAJOR="))
        {
            MAJOR = atoi(&line[strlen("MAJOR=")]);
        }
        else if (strStartsWith(line, "MINOR="))
        {
            MINOR = atoi(&line[strlen("MINOR=")]);
        }
        //start: modify for kernel version 2.6, 2022.01.22
        else if (strStartsWith(line, "DEVICE="))
        {
            strncpy(busname, &line[strlen("DEVICE=")], FIBO_BUF_SIZE);
        }
        //end: modify for kernel version 2.6, 2022.01.22
        else if (strStartsWith(line, "DEVNAME="))
        {
            strncpy(busname, &line[strlen("DEVNAME=")], FIBO_BUF_SIZE);
        }
        else if (strStartsWith(line, "DEVTYPE="))
        {
            strncpy(DEVTYPE, &line[strlen("DEVTYPE=")], sizeof(DEVTYPE));
            DEVTYPE[DEVICES_SIZE-1] = '\0';
        }
        else if (strStartsWith(line, "PRODUCT="))
        {
            strncpy(PRODUCT, &line[strlen("PRODUCT=")], sizeof(PRODUCT));
            PRODUCT[DEVICES_SIZE-1] = '\0';
        }
    }
    fclose(fp);
    // printf("----------------------------------------\n");

    if (MAJOR != 189  || MINOR == 0 || busname[0] == 0
        || DEVTYPE[0] == 0 || PRODUCT[0] == 0
        || strStartsWith(DEVTYPE, "usb_device") == 0) {
        return -1;
    }

    return 0;
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

static fibo_usbdev_t *fibo_get_qcom_device(char *portname, char *syspath)
{
    DIR *usbdir = NULL;
    struct dirent *dent = NULL;
    char sys_filename[FIBO_BUF_SIZE] = {0};
    int idVendor = 0, idProduct = 0;
    int bNumInterfaces = 0, bConfigurationValue = 0;
    fibo_usbdev_t *udev = NULL, *ret_udev = NULL;
    unsigned modules_num = 0;

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

        udev = qcom_find_devices_in_table(idVendor, idProduct);
        if (udev != NULL) {
            printf("----------------------------------\n");
            printf("ModuleName: %s\n", udev->ModuleName);
            printf("idVendor: %04x\n", udev->idVendor);
            printf("idProduct: %04x\n", udev->idProduct);
            printf("bNumInterfaces: %d\n", bNumInterfaces);
            printf("bConfigurationValue: %d\n", bConfigurationValue);
            snprintf(sys_filename, sizeof(sys_filename), "%s/%s/uevent", USB_DIR_BASE, dent->d_name);
            fibo_get_busname_by_uevent(sys_filename, udev->busname);
            printf("busname: %s\n", udev->busname);
            if (syspath[0] || portname[0]) {
                for (udev->ifnum[0] = 0; udev->ifnum[0] < bNumInterfaces; udev->ifnum[0]++) {
                    snprintf(udev->syspath, sizeof(udev->syspath), "%s/%s/%s:%d.%d", USB_DIR_BASE, dent->d_name, dent->d_name, bConfigurationValue, udev->ifnum[0]);
                    memset(udev->portname, 0, sizeof(udev->portname));
                    fibo_get_ttyport_by_syspath(udev);
                    printf("portname: %s -- %s\n", udev->portname, udev->syspath);

                    if ((syspath[0] && !strcmp(syspath, udev->syspath)) || (portname[0] &&!strcmp(portname, udev->portname))) {
                        ret_udev = udev;
                        modules_num = 1;
                        printf("----------------------------------\n");
                        goto END;
                    }
                }
                // 2024-12-03, Shijiaxing, MTC0233-495, Begin. Haikang issue: support multiple device at the same time by specify portname.
                // use the given portname and last match udev.
                memcpy(udev->portname, portname, sizeof(udev->portname));
                // 2024-12-03, Shijiaxing, MTC0233-495, #nd. Haikang issue: support multiple device at the same time by specify portname.
            } else {
                snprintf(udev->syspath, sizeof(udev->syspath), "%s/%s/%s:%d.%d", USB_DIR_BASE, dent->d_name, dent->d_name, bConfigurationValue, udev->ifnum[0]);
                fibo_get_ttyport_by_syspath(udev);
                printf("portname: %s -- %s\n", udev->portname, udev->syspath);  
            }
            ret_udev = udev;
            modules_num++;
            printf("----------------------------------\n");
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
    else if (modules_num == 1) {
        printf("%u modules found.\n", modules_num);
        return ret_udev;
    }
    //2024-12-03, Shijiaxing, MTC0233-495, Begin. Haikang issue: support multiple device at the same time by specify portname.
    //choose the last match udev, later use the given portname.
    else if (ret_udev != NULL) {
        printf("%u modules found.\n", modules_num);
        return ret_udev;
    }
    else if (portname[0] == 0 && syspath[0] == 0) {
        printf("%u modules found\n", modules_num);
        printf("please set portname <-d /dev/XXX> or set syspath <-s /sys/bus/usb/devices/X-X/X-X:X.X>\n");
    }
    //2024-12-03, Shijiaxing, MTC0233-495, End. Haikang issue: support multiple device at the same time by specify portname.

    return NULL;
}

ssize_t log_poll_read(int fd,  void *pbuf, size_t size)
{
    unsigned t, timeout_msec = -1;
    ssize_t rc = 0;

    //printf("[%s], line: %d\n", __func__, __LINE__);

    while (g_is_qxdm_logging && timeout_msec > 0)
    {
        int ret = -1;
        struct pollfd pollfds[] = {{s_use_socket? s_socket_fd: fd, POLLIN, 0}};

        t = (timeout_msec < 200)? timeout_msec : 200;
        ret = poll(pollfds, 1, t);

        if (g_is_qxdm_logging == 0) {
            break;
        }

        if (ret == 0) {
            timeout_msec -= t;
            if (timeout_msec == 0) {
                rc = -1;
                errno = ETIMEDOUT;
                break;
            }
            continue;
        }
        else if (ret < 0) {
            printf("poll(handlefd) :%d, errno:%d(%s)\n", ret, errno, strerror(errno));
            break;
        }

        if (pollfds[0].revents & (POLLERR | POLLHUP | POLLNVAL)) {
            printf("poll fd:%d, revents: %04x\n", pollfds[0].fd, pollfds[0].revents);
            break;
        }

        if (pollfds[0].revents & (POLLIN))
        {
            if (s_use_socket) { //use_socket
                rc=recv(s_socket_fd, pbuf, size, 0);
                //printf("recv(s_socket_fd,pbuf, size,0): %d\n",rc);
            } else {
                rc = read(fd, pbuf, size);
            }
            break;
        }
    }

    return rc;
}

static size_t fibo_usbdev_write(arguments_t *args, const void *data, size_t size)
{
    struct usbdevfs_urb bulk;
    struct usbdevfs_urb *urb = &bulk;
    int n = 0;

    //printf("[%s], line: %d\n", __func__, __LINE__);

    memset(urb, 0, sizeof(struct usbdevfs_urb));
    urb->type = USBDEVFS_URB_TYPE_BULK;
    urb->endpoint = args->udev->bulk_ep_out[0];
    urb->status = -1;
    urb->buffer = (void *)data;
    urb->buffer_length = size;
    urb->usercontext = urb;
    urb->flags = 0;

    n = ioctl(args->usbdev, USBDEVFS_SUBMITURB, urb);
    if (n < 0) {
        printf("%s submit n: %d, errno:%d(%s)\n", __func__, n, errno, strerror(errno));
        return 0;
    }

    urb = NULL;
    n = ioctl(args->usbdev, USBDEVFS_REAPURB, &urb);
    if (n < 0) {
        printf("%s reap n: %d, errno:%d(%s)\n", __func__, n, errno, strerror(errno));
        return 0;
    }

    if (urb && urb->status == 0 && urb->actual_length) {
        // printf("urb->actual_length: %u\n", urb->actual_length);
        return urb->actual_length;
    }

    return 0;
}

size_t log_poll_write(int fd, const void *buf, size_t size)
{
    size_t wc = 0;
    ssize_t nbytes;
    uint32_t timeout_msec = 1000;

    //printf("[%s], line: %d\n", __func__, __LINE__);

    if (fd == qlog_args.ttyfd && fd == qlog_args.usb_sockets[0]) {
        return fibo_usbdev_write(&qlog_args, buf, size);
    }

    if (!s_use_socket)
    {
        nbytes = write(fd, (char *)buf+wc, size-wc);
        // printf("[%s] write nbytes %zd\n", __func__, nbytes);

        if (nbytes <= 0) {
            if (errno != EAGAIN) {
                printf("Fail to write fd: %d, errno:%d(%s)\n", fd, errno, strerror(errno));
                goto END;
            }
            nbytes = 0;
        }
        wc += nbytes;

        while (wc < size) {
            struct pollfd pollfds[] = {{fd, POLLOUT, 0}};
            int ret = poll(pollfds, 1, timeout_msec);
            if (ret <= 0) {
                printf("Fail to poll fd: %d, errno : %d (%s)\n", fd, errno, strerror(errno));
                break;
            }

            if (pollfds[0].revents & (POLLERR | POLLHUP | POLLNVAL)) {
                printf("Fail to poll fd: %d, revents: %04x\n", fd, pollfds[0].revents);
                break;
            }

            if (pollfds[0].revents & (POLLOUT)) {
                nbytes = write(fd, (char *)buf+wc, size-wc);
                //printf("[%s] write nbytes %d\n", __func__, nbytes);
                if (nbytes <= 0) {
                    printf("Fail to write fd: %d, errno:%d(%s)\n", fd, errno, strerror(errno));
                    break;
                }

                wc += nbytes;
            }
        }
    }
    else
    {
        nbytes=send(s_socket_fd, (char *)buf+wc, size-wc, 0);
        printf("[%s] send nbytes:%zd\n", __func__, nbytes);
        if (-1 == nbytes)
        {
            if (errno != EAGAIN) {
                printf("Fail to write fd: %d, errno:%d (%s)\n", fd, errno, strerror(errno));
                goto END;
            }
            nbytes = 0;
        }
        wc += nbytes;

        while (wc < size) {
            struct pollfd pollfds[] = {{s_socket_fd, POLLOUT, 0}};
            int ret = poll(pollfds, 1, timeout_msec);

            if (ret <= 0) {
                printf("Fail to poll fd: %d, errno: %d(%s)\n", fd, errno, strerror(errno));
                break;
            }

            if (pollfds[0].revents & (POLLERR | POLLHUP | POLLNVAL)) {
                printf("Fail to poll fd: %d, revents: %04x\n", fd, pollfds[0].revents);
                break;
            }

            if (pollfds[0].revents & (POLLOUT))
            {
                nbytes=send(s_socket_fd, (char *)buf+wc, size-wc,0);
                //printf("send(s_socket_fd, buf+wc, size-wc,0),nbytes :%d\n",nbytes);
                if (-1==nbytes)
                {
                    printf("Fail to write fd: %d, errno:%d(%s)\n", fd, errno, strerror(errno));
                    break;
                }
                wc += nbytes;
            }
        }
    }

END:
    if (wc != size) {
        printf("[%s] fd:%d, size:%zd, wc:%zd\n", __func__, fd, size, wc);
    }

    return wc;
}

static int serial_connect(const char *portname)
{
    struct termios newtio;

    if (NULL == portname)
    {
        printf("portname is NULL.\n");
        return -1;
    }

    if (g_fd_port >= 0)
    {
        printf("port[%s] opened already!\n", portname);
        return 0;
    }

    printf("port[%s] connecting...\n", portname);
    g_fd_port = open(portname, O_RDWR | O_NDELAY | O_NOCTTY);
    if(g_fd_port < 0)
    {
        printf("port[%s] open failed! errno:%d(%s)\n", portname, errno, strerror(errno));
        goto error_exit;
    }

    if (tcgetattr(g_fd_port, &newtio) != 0)
    {
        printf("port[%s] tcgetattr failed! errno:%d(%s)\n", portname, errno, strerror(errno));
        goto error_exit;
    }

    cfmakeraw(&newtio);
    /*set baudrate*/
    cfsetispeed(&newtio, B115200);
    cfsetospeed(&newtio, B115200);

    /*set char bit size*/
    newtio.c_cflag &= ~CSIZE;
    newtio.c_cflag |= CS8;

    /*set check sum*/
    newtio.c_cflag &= ~PARENB;

    /*set stop bit*/
    newtio.c_cflag &= ~CSTOPB;
    newtio.c_cflag |= CLOCAL |CREAD;
    newtio.c_cflag &= ~(PARENB | PARODD);

    newtio.c_iflag &= ~(INPCK | BRKINT |PARMRK | ISTRIP | INLCR | IGNCR | ICRNL);
    newtio.c_iflag |= IGNBRK;
    newtio.c_iflag &= ~(IXON|IXOFF|IXANY);

    newtio.c_oflag = 0;

    newtio.c_lflag = 0;

    /*set wait time*/
    newtio.c_cc[VMIN] = 0;
    newtio.c_cc[VTIME] = 0;

    if (tcsetattr(g_fd_port, TCSANOW, &newtio) !=0 )
    {
        printf("port[%s] tcsetattr failed! errno:%d(%s)\n", portname, errno, strerror(errno));
        goto error_exit;
    }

    tcflush(g_fd_port, TCIFLUSH);
    tcflush(g_fd_port, TCOFLUSH);

    printf("port[%s] connected!\n", portname);

    return 0;
error_exit:
    if (g_fd_port >= 0)
    {
        close(g_fd_port);
        g_fd_port = -1;
    }

    return -1;
}

static int usbfs_check_kernel_driver(int fd, int ifnum)
{
    struct usbdevfs_getdriver getdrv;

    getdrv.interface = ifnum;
    if (ioctl(fd, USBDEVFS_GETDRIVER, &getdrv) >= 0) {
        struct usbdevfs_ioctl operate;

        operate.data = NULL;
        operate.ifno = ifnum;
        operate.ioctl_code = USBDEVFS_DISCONNECT;

         printf("%s find interface %d has match the driver %s\n", __func__, ifnum, getdrv.driver);
        if (ioctl(fd, USBDEVFS_IOCTL, &operate) < 0) {
            printf("%s failed.\n", __func__);
            return -1;
        }

        printf("%s OK.\n", __func__);
    }
    else if (errno != ENODATA) {
        printf("%s ioctl USBDEVFS_GETDRIVER failed, errno:%d(%s)\n", __func__, errno, strerror(errno));
    }

    return 0;
}

static void *qlog_usbfs_read_thread(void *arg)
{
    struct usbdevfs_bulktransfer bulk;
    void *pbuf = NULL;
    arguments_t *args = (arguments_t *)arg;
    int n = 0;

    pbuf = malloc(RX_URB_SIZE);
    if (pbuf == NULL) {
        printf("%s malloc %d fail\n", __func__, RX_URB_SIZE);
        return NULL;
    }

    bulk.ep = args->udev->bulk_ep_in[0];
    bulk.len = RX_URB_SIZE;
    bulk.data = (void *)pbuf;
    bulk.timeout = 0;

    while (g_exit_flag == 0)
    {
        int nwrites = 0;
        int count = 0;

        n = ioctl(args->usbdev, USBDEVFS_BULK, &bulk);
        if (n < 0) {
            printf(" n = %d, errno:%d(%s)\n", n, errno, strerror(errno));
            break;
        }
        else if (n == 0) {
            //zero length packet
        }

        if (n > 0) {
            // printf("urb nreads: %d\n", n);
            while (count < n) {
                do {
                    nwrites = write(args->usb_sockets[1], pbuf, n);
                } while (nwrites == -1 && errno == EAGAIN);

                count += nwrites;
            }
        }
    }

    free(pbuf);

    return NULL;
}

static int fibo_usbdev_open(arguments_t *args)
{
    int ret = -1;
    char devname[FIBO_BUF_SIZE+EXTEND] = {0};
    char devdesc[FIBO_BUF_SIZE*2] = {0};
    size_t desc_length = 0, desc_index = 0;
    int bInterfaceNumber = 0;
    fibo_usbdev_t *udev = args->udev;

    //start: modify for kernel version 2.6, 2022.01.22
    if (strstr(udev->busname, "/proc/bus/usb")) {
        snprintf(devname, sizeof(devname), "%s", udev->busname);
    }
    else {
        snprintf(devname, sizeof(devname), "/dev/%s", udev->busname);
    }
    //end: modify for kernel version 2.6, 2022.01.22

    if (access(devname, F_OK) && errno == ENOENT) {
        printf("[%s] access %s failed, errno:%d(%s)\n", __func__, devname, errno, strerror(errno));
        return -1;
    }

    args->usbdev = open(devname, O_RDWR | O_NDELAY);
    if (args->usbdev < 0) {
        printf("usbfs open %s failed, errno:%d(%s)\n", devname, errno, strerror(errno));
        goto error;
    }

    printf("open %s usbdev: %d\n", devname, args->usbdev);
    desc_length = read(args->usbdev, devdesc, sizeof(devdesc));
    for (desc_index=0; desc_index < desc_length;)
    {
        struct usb_descriptor_header *h = (struct usb_descriptor_header *)(devdesc + desc_index);

        if (h->bLength == sizeof(struct usb_device_descriptor) && h->bDescriptorType == USB_DT_DEVICE)
        {
            // struct usb_device_descriptor *device = (struct usb_device_descriptor *)h;
            // printf("P: idVendor: %04x idProduct:%04x\n", device->idVendor, device->idProduct);
        }
        else if (h->bLength == sizeof(struct usb_config_descriptor) && h->bDescriptorType == USB_DT_CONFIG)
        {
            // struct usb_config_descriptor *config = (struct usb_config_descriptor *)h;
            // printf("C: bNumInterfaces: %d\n", config->bNumInterfaces);
        }
        else if (h->bLength == sizeof(struct usb_interface_descriptor) && h->bDescriptorType == USB_DT_INTERFACE)
        {
            struct usb_interface_descriptor *interface = (struct usb_interface_descriptor *)h;

            // printf("I: If#= %d Alt= %d #EPs= %d Cls=%02x Sub=%02x Prot=%02x\n",
                // interface->bInterfaceNumber, interface->bAlternateSetting, interface->bNumEndpoints,
                // interface->bInterfaceClass, interface->bInterfaceSubClass, interface->bInterfaceProtocol);
            bInterfaceNumber = interface->bInterfaceNumber;
        }
        else if (h->bLength == USB_DT_ENDPOINT_SIZE && h->bDescriptorType == USB_DT_ENDPOINT)
        {
            struct usb_endpoint_descriptor *endpoint = (struct usb_endpoint_descriptor *)h;

            // printf("bEndpointAddress:%02x, bmAttributes:%02x, wMaxPacketSize:%d\n",
                // endpoint->bEndpointAddress, endpoint->bmAttributes, endpoint->wMaxPacketSize);

            if (bInterfaceNumber == udev->ifnum[0])
            {
                if ((endpoint->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) == USB_ENDPOINT_XFER_BULK)
                {
                    if (endpoint->bEndpointAddress & USB_ENDPOINT_DIR_MASK) {
                        udev->bulk_ep_in[0] = endpoint->bEndpointAddress;
                    } else {
                        udev->bulk_ep_out[0] = endpoint->bEndpointAddress;
                    }
                    udev->wMaxPacketSize = endpoint->wMaxPacketSize;
                }
            }
        }
        desc_index += h->bLength;
    }
    printf("bulk_ep_in[0]: 0x%02x, bulk_ep_out[0]: 0x%02x\n",udev->bulk_ep_in[0], udev->bulk_ep_out[0]);
    printf("----------------------------------------\n");

    if (usbfs_check_kernel_driver(args->usbdev, udev->ifnum[0])) {
        goto error;
    }

    ret = ioctl(args->usbdev, USBDEVFS_CLAIMINTERFACE, &udev->ifnum[0]); // attach usbfs driver
    if (ret != 0)
    {
        printf("ioctl USBDEVFS_CLAIMINTERFACE failed, errno:%d(%s)\n", errno, strerror(errno));
        goto error;
    }

    socketpair(AF_LOCAL, SOCK_STREAM, 0, args->usb_sockets);
    g_fd_port = args->ttyfd = args->usb_sockets[0];

    {
        pthread_t thread_id;
        pthread_attr_t usb_thread_attr;

        pthread_attr_init(&usb_thread_attr);
        pthread_attr_setdetachstate(&usb_thread_attr, PTHREAD_CREATE_DETACHED);
        pthread_create(&thread_id, &usb_thread_attr, qlog_usbfs_read_thread, (void *)args);
    }

    ret = 0;
error:
    if (args->usbdev >= 0) {
        close(args->usbdev);
        args->usbdev = -1;
    }

    return ret;
}

int serial_autoconnect(const char *portname, const char *syspath)
{
    printf("%s start\n", __func__);

    if (g_fd_port >= 0)
    {
        printf("serial opened already\n");
        return 0;
    }

    if (!s_use_socket)
    {
        char portname_str[BUF_SIZE] = {0};

        memset(portname_str, 0, sizeof(portname_str));
        if (portname[0])
        {
            memcpy(portname_str, portname, strlen(portname));
            /*zhangboxing IRP-20230200297 2023/03/13  begin*/
            if(strcmp(portname,"/dev/mhi_DIAG")== 0)
            {
                if(serial_connect(portname_str) < 0)
                {
                    printf("failed to open pcie port <%s> !", portname_str);
                    return -1;
                }

                return 0;
            }
            /*zhangboxing IRP-20230200297 2023/03/13  end*/
        }
        else if (access("/dev/mhi_DIAG", F_OK) == 0)
        {
            sprintf(portname_str, "/dev/mhi_DIAG");
        }

        qlog_args.udev = fibo_get_qcom_device(portname_str, (char *)syspath);
        if (qlog_args.udev == NULL) {
            return -1;
        }

        if (qlog_args.udev->portname[0] == 0)
        {
            printf("catch log with usbfs!\n");
            if (fibo_usbdev_open(&qlog_args)) {
                return -1;
            }
        }
        else
        {
            printf("catch log with ttyUSB!\n");
            if (serial_connect(qlog_args.udev->portname) < 0) {
                return -1;
            }
        }
    }
    else
    {
        int port_index = 0;
        struct sockaddr_in server_addr;

        s_socket_fd=socket(AF_INET, SOCK_STREAM, 0);
        if (s_socket_fd==-1)
        {
            printf("socket error! errno:%d(%s)\n",errno, strerror(errno));
            return -1;
        }
        int length=sizeof(server_addr);
        server_addr.sin_family=AF_INET;  //PF_INET 地址族
        server_addr.sin_port= htons(s_socket_port); //端口号
        server_addr.sin_addr.s_addr=inet_addr(g_ip_addr);   //ip地址  绑定什么连接什么地址

        if (g_fd_port >= 0)
        {
            printf("serial opened already!\n");
            return 0;
        }

        for(port_index = 0; port_index < 50; port_index++)
        {
            g_fd_port=connect(s_socket_fd,(struct sockaddr *)&server_addr,length);
            if (-1==g_fd_port)
            {
                printf("connect s_socket_fd error! errno:%d(%s)\n",errno, strerror(errno));
            }
            else
            {
                printf("connect s_socket_fd sucessfully!!\n");
                break;
            }
        }

        if (-1 == g_fd_port)
        {
            printf("connect s_socket_fd error! errno: %d(%s)\n", errno, strerror(errno));
            return -1;
        }
        printf("connect s_socket_fd sucessfully!!\n");
    }

    return 0;
}

static int serial_disconnect()
{
    if (s_use_socket)
    {
        if (s_socket_fd >= 0)
        {
            close(s_socket_fd);
            s_socket_fd = -1;
        }
    }
    else
    {
        if (g_fd_port >= 0)
        {
            close(g_fd_port);
            g_fd_port = -1;
        }
    }

    return 0;
}

static int serial_write_hexstring(const char *str_data)
{
    char ch_high, ch_low;
    uint8_t *write_buf = NULL;
    ssize_t write_cnt = 0, wr_ret = 0;

    if (g_fd_port < 0)
    {
        printf("please open port first!\n");
        return 0;
    }

    if (!str_data || strlen(str_data) <= 0)
        goto error_exit;

    write_buf = (uint8_t *)malloc(strlen(str_data));
    if (!write_buf)
    {
        printf("malloc write_buf failed, errno:%d(%s)\n", errno, strerror(errno));
        goto error_exit;
    }

    while (*str_data != '\0' && (*str_data + 1) != '\0')
    {
        ch_high = tolower(*str_data);
        ch_low = tolower(*(str_data + 1));
        if ((('0' <= ch_high && '9' >= ch_high) || ('a' <= ch_high && 'f' >= ch_high)) &&
                        (('0' <= ch_low && '9' >= ch_low) || ('a' <= ch_low && 'f' >= ch_low)))
        {
            if ('0' <= ch_high && '9' >= ch_high)
            {
                write_buf[write_cnt] = (ch_high - '0') << 4;
            }
            else
            {
                write_buf[write_cnt] = (0x0a + ch_high - 'a') << 4;
            }

            if ('0' <= ch_low && '9' >= ch_low)
            {
                write_buf[write_cnt] |= (ch_low - '0');
            }
            else
            {
                write_buf[write_cnt] |= (0x0a + ch_low - 'a');
            }
            write_cnt++;
            str_data += 2;
        }
        else
            str_data++;
    }

    if (write_cnt <= 0) {
        goto error_exit;
    }

    /*just write data to device*/
    if (s_use_socket)
    {
        wr_ret=send(s_socket_fd, write_buf, write_cnt,0);
        if (-1==wr_ret)
        {
            printf("port[0x%x] write error! errno:%d(%s)\n", s_socket_fd, errno, strerror(errno));
            goto error_exit;
        }
    }
    else
    {
        wr_ret = write(g_fd_port, write_buf, write_cnt);
        if (wr_ret <= 0)
        {
            printf("port[0x%x] write error! errno:%d(%s)\n", g_fd_port, errno, strerror(errno));
            goto error_exit;
        }
    }
    if (write_buf)
        free(write_buf);

    return wr_ret;
error_exit:

    if (write_buf)
        free(write_buf);

    return 0;
}

static int serial_read(uint8_t *p_in_buffer, unsigned long in_buffer_size, unsigned long *p_bytes_read)
{
    int read_len = 0;
    fd_set fs_read;
    struct timeval tv_timeout;
    int fs_sel = -1;

    tv_timeout.tv_sec = 2;
    tv_timeout.tv_usec = 0;
    FD_ZERO(&fs_read);
    FD_SET(g_fd_port,&fs_read);

    if (g_fd_port < 0 )
    {
        printf("please open port first!\n");
        return 0;
    }

    if (!p_in_buffer || in_buffer_size <= 0) {
        return 0;
    }

    if (s_use_socket)
    {
        read_len=recv(s_socket_fd, p_in_buffer, in_buffer_size, 0);//4个参数
        if (-1==read_len)
        {
            printf("rec s_socket_fd failed. read_len: %d, errno:%d(%s)\n", read_len, errno, strerror(errno));
            read_len = 0;
        }
    }
    else
    {
        fs_sel = select(g_fd_port+1, &fs_read, NULL, NULL, &tv_timeout);
        if (fs_sel > 0)
        {
            read_len = read(g_fd_port, p_in_buffer, in_buffer_size);
            if (read_len < 0)
            {
                printf("read g_fd_port failed, read_len:%d, errno:%d(%s)\n", read_len, errno, strerror(errno));
                read_len = 0;
            }
        }
    }

    if (p_bytes_read) {
        *p_bytes_read = read_len;
    }

    return read_len;
}

typedef struct {
    qlog_ops_t *qcom_qlog_ops;
    int ttyfd;
    const char *log_dir;
    const char *cfg_name;
} parm_dev_t;

void *qlog_logfile_init_filter_thread(void *arg)
{
    parm_dev_t *param = (parm_dev_t *)arg;

    if (param && param->qcom_qlog_ops && param->qcom_qlog_ops->init_filter)
    {
        printf("[%s]: ttyfd:%d, cfg_name: %s\n",  __func__, param->ttyfd, param->cfg_name);
        param->qcom_qlog_ops->init_filter(param->ttyfd, param->log_dir, param->cfg_name);
    }

    printf("qlog_init_filter_finished\n");
    qlog_init_filter_finished = 1;

    return NULL;
}

static int qualcomm_serial_logstop()
{
    int ret = -1;
    
    if (g_is_qxdm_logging)
    {
        if (!serial_write_hexstring("60 00 12 6a  7e"))
            goto END;
        usleep(10*1000);
        if (!serial_write_hexstring("73 00 00 00  00 00 00 00    da 81 7e"))
            goto END;
        usleep(10*1000);
        if (!serial_write_hexstring("7d 5d 05 00  00 00 00 00    00 74 41 7e"))
            goto END;
        usleep(10*1000);
        if (!serial_write_hexstring("4b 04 0e 00  0d d3 7e"))
            goto END;
        usleep(10*1000);
        if (!serial_write_hexstring("4b 04 0e 00  0d d3 7e"))
            goto END;
        usleep(10*1000);

        printf("Log stopped, dir: %s\n", g_qcom_logdir);
    }

    ret = 0;
END:
    serial_disconnect();
    printf("[%s]\n", __func__);

    return ret;
}

void *thread_qcom_serial_read_log(void *arg)
{
    uint8_t *rx_buffer = NULL;
    int file_size, recved_bytes = 0, retlen;
    FILE *p_logfile = NULL;
    struct tm *tm;
    time_t t;
    arg =NULL;
    rx_buffer = (uint8_t *)malloc(QCOM_QXDM_LOG_READ_SIZE);
    if (rx_buffer == NULL) {
        printf("malloc rx_buffer error, errno:%d(%s)\n", errno, strerror(errno));
        return NULL;
    }

    printf("%s started!\n", __func__);

    while (g_is_qxdm_logging)
    {
        if (p_logfile)
        {
            /* close the file */
            int cur_pos = ftell(p_logfile);
            fseek(p_logfile, 0, SEEK_END);
            file_size = ftell(p_logfile);
            fseek(p_logfile, cur_pos, SEEK_SET);
            if (file_size > g_qxdm_log_file_size)
            {
                fflush(p_logfile);
                fclose(p_logfile);
                strcpy(g_ftp_put_filename, g_qxdm_log_filename);
                printf("g_ftp_put_filename %s\n", g_ftp_put_filename);
                ftp_put_flag = 1;
                p_logfile = NULL;
            }
        }
        else
        {
            char log_filename[BUF_SIZE+EXTEND] = {0};

            /* create log file */
            t = time(NULL);
            tm = localtime(&t);
            sprintf(g_qxdm_log_filename, "%02d%02d%02d%02d%02d%02d.qmdl2", tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday,tm->tm_hour, tm->tm_min, tm->tm_sec);
            sprintf(log_filename, "%s/%s", g_qcom_logdir, g_qxdm_log_filename);

            printf("log_filename: %s\n", log_filename);
            p_logfile = fopen(log_filename, "wb");
            if (p_logfile == NULL) {
                printf("create log file failed, errno:%d(%s)\n", errno, strerror(errno));
                break;
            }
        }

        /* read log to file */
        retlen = serial_read(rx_buffer + recved_bytes, QCOM_QXDM_LOG_READ_SIZE - recved_bytes, NULL);
        recved_bytes += retlen;
        if (recved_bytes >= QCOM_QXDM_LOG_WRITE_SIZE)
        {
            fwrite(rx_buffer, recved_bytes, 1, p_logfile);
            recved_bytes = 0;
        }

        if (g_exit_flag == 1)
        {
            break;
        }

        /* print current status */
        {
            static unsigned long start_tick = 0;
            static unsigned long reced_bytes = 0;
            unsigned long cur_tick = 0;
            unsigned long diff_tick = 0;

            reced_bytes += retlen;
            if (start_tick == 0) {
                start_tick = time(NULL);
            }
            else
            {
                cur_tick = time(NULL);
                diff_tick = cur_tick - start_tick;
                if (diff_tick >= 5)
                {
                    t = time(NULL);
                    tm = localtime(&t);
                    printf("[%02d%02d%02d%02d%02d%02d] recvd(%lu), tick(%lu), %lu B/s\n",tm->tm_year+1900, tm->tm_mon+1,
                        tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec, reced_bytes, diff_tick, reced_bytes / diff_tick);
                    start_tick = cur_tick;
                    reced_bytes = 0;
                }
            }
        }
    }

    if (p_logfile)
    {
        fflush(p_logfile);
        fclose(p_logfile);
    }
    if (rx_buffer) {
        free(rx_buffer);
        rx_buffer = NULL;
    }

    printf("[%s] log stopped!\n", __func__);

    return NULL;
}

#define straQxdmGrabLogType   "online"
int qualcomm_serial_logstart(const char *portname, const char *syspath, const char *log_dir, const char *cfg_name)
{
    int i, logfd = -1;
    pthread_t thread_id, thread_id1;
    pthread_attr_t thread_attr, attr;
    parm_dev_t thread_args;
    struct sched_param param;
    uint8_t *rbuf = NULL;
    ssize_t cur_single_logsie = 0;
    struct tm *tm;
    time_t t;

    g_is_qxdm_logging = 1;

    printf("[%s] start\n", __func__);

    if (serial_autoconnect(portname, syspath)) {
        goto error_exit;
    }

    if (!s_use_adb)
    {
        t = time(NULL);
        tm = localtime(&t);
        sprintf(g_str_sub_log_dir, "fibolog_%02d%02d%02d%02d%02d%02d",
            tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);
        sprintf(g_qcom_logdir, "%s/%s/", log_dir, g_str_sub_log_dir);
        printf("[%s]g_qcom_logdir:%s\n", __func__, g_qcom_logdir);

        for (i=1; i<BUF_SIZE && g_qcom_logdir[i] != 0; i++)
        {
            if (g_qcom_logdir[i] == '\\'  || g_qcom_logdir[i] == '/' )
            {
                char str_dir[BUF_SIZE] = {0};
                strcpy(str_dir, g_qcom_logdir);
                str_dir[i] = '\0';
                if (access(str_dir, 0)) {
                    mkdir(str_dir, 0777);
                    printf("[%s] mkdir:%s\n", __func__, str_dir);
                }
            }
        }

        rbuf = (uint8_t *)malloc(QCOM_QXDM_LOG_READ_SIZE);
        if (rbuf == NULL)
        {
            printf("malloc rbuf failed, errno:%d(%s)\n", errno, strerror(errno));
            return -1;
        }

        pthread_mutex_init(&mutex, NULL); /*resolve the bug g_mdm_req issue (mantis 63061), yanghaitao 2020.11.25 */
        thread_args.qcom_qlog_ops = &qcom_qlog_ops;
        if (s_nottyUSB)
        {
            thread_args.ttyfd = qlog_args.ttyfd;
        }
        else
        {
            thread_args.ttyfd = g_fd_port;
        }
        thread_args.cfg_name = cfg_name;
        thread_args.log_dir = log_dir;

        pthread_attr_init(&thread_attr);
        pthread_attr_getschedparam(&thread_attr, &param);
        param.sched_priority=99;
        pthread_attr_setschedparam(&thread_attr, &param);
        pthread_attr_setschedpolicy(&thread_attr, SCHED_RR);
        pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_DETACHED);
        pthread_create(&thread_id, &thread_attr, qlog_logfile_init_filter_thread, (void *)&thread_args);

        ssize_t rc = 0, wc = 0;
        while (g_is_qxdm_logging)
        {
            if (qlog_init_filter_finished)
            {
                qlog_init_filter_finished=0;
                pthread_attr_destroy(&thread_attr);
            }

            rc = log_poll_read(g_fd_port, rbuf, QCOM_QXDM_LOG_READ_SIZE);
            if (rc <= 0) {
                break;
            }

            /* print current status */
            {
                static unsigned long start_tick = 0;
                static unsigned long reced_bytes = 0;
                unsigned long cur_tick = 0;
                unsigned long diff_tick = 0;

                reced_bytes += rc;
                if (start_tick == 0) {
                    start_tick = time(NULL);
                }
                else
                {
                    cur_tick = time(NULL);
                    diff_tick = cur_tick - start_tick;
                    if (reced_bytes >= (16*1024*1024) || diff_tick >= 5)
                    {
                        t = time(NULL);
                        tm = localtime(&t);
                        printf("[%02d%02d%02d%02d%02d%02d] recvd(%lu), tick(%lu), %lu B/s\n",tm->tm_year+1900, tm->tm_mon+1,
                            tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec, reced_bytes, diff_tick, reced_bytes / diff_tick);
                        start_tick = cur_tick;
                        reced_bytes = 0;
                    }
                }
            }


            if (logfd == -1)
            {
                logfd = qcom_qlog_ops.logfile_create(g_qcom_logdir, "qmdl2", 0);
                printf("g_qcom_logdir: %s\n", g_qcom_logdir);

                if (logfd < 0)
                {
                    break;
                }
                if (qcom_qlog_ops.logfile_init)
                {
                    qcom_qlog_ops.logfile_init(logfd, 0);
                }
            }

            wc = qcom_qlog_ops.logfile_save(logfd, rbuf, rc);
            if (wc != rc)
            {
                printf("[%s] savelog failed %zd/%zd\n", __func__, wc, rc);
                g_is_qxdm_logging = 0;
                break;
            }

            cur_single_logsie += wc;
            if (cur_single_logsie >= g_qxdm_log_file_size)
            {
                cur_single_logsie = 0;
                qcom_qlog_ops.logfile_close(logfd);
                logfd = -1;
                strcpy(g_ftp_put_filename, g_qxdm_log_filename);
                printf("g_ftp_put_filename: %s\n", g_ftp_put_filename);
                ftp_put_flag = 1;
            }
        }

        pthread_mutex_destroy(&mutex);/*resolve the bug g_mdm_req issue (mantis 63061), yanghaitao 2020.11.25 */

        if (logfd >= 0) {
            qcom_qlog_ops.logfile_close(logfd);
            logfd = -1;
        }

        free(rbuf);
        rbuf = NULL;

        if (qcom_qlog_ops.clean_filter) {
            qcom_qlog_ops.clean_filter(g_fd_port);
        }
    }
    else
    {
        uint8_t Send_buf[BUF_SIZE] = {0};
        FB_TCP_HEAD hd;
        int hd_len = sizeof(FB_TCP_HEAD);

        memset(&hd,0,hd_len);
        hd.fb_flag_1 = 0xFF;
        hd.fb_flag_2 = 0xBB;
        hd.package_type = FB_TYPE_UPDATE_QXDM_CFG_FILE;//0x12;
        hd.package_flag = 0;
        hd.payload_lenth = strlen(straQxdmGrabLogType);

        memcpy(Send_buf, &hd, hd_len);
        memcpy(Send_buf+hd_len, straQxdmGrabLogType, strlen(straQxdmGrabLogType));

        if (-1 == send(s_socket_fd, Send_buf, hd_len + strlen(straQxdmGrabLogType), 0)) {
            printf("send s_socket_fd error! errno:%d(%s)\n", errno, strerror(errno));
            return 0;
        }
        else {
            printf("send successfully!!!FB_TYPE_UPDATE_QXDM_CFG_FILE\n");
        }

        FILE *fp = fopen(cfg_name, "rb");
        printf("[%s] cfg_name: %s", __func__, cfg_name);
        if (fp == NULL)
        {
            printf("use default config data in the qxdm_default_cfg.h\n");
            // send(s_socket_fd, (const char*)qxdm_default_cfg_buf, sizeof(qxdm_default_cfg_buf), 0);
            // printf("send cfg: %d\n", sizeof(qxdm_default_cfg_buf));
        } else {
            int nCount = 0;
            while ((nCount = fread(Send_buf, 1, BUF_SIZE, fp)) > 0)
            {
                send(s_socket_fd, (const char*)Send_buf, nCount, 0);
                printf("send cfg %d\n", nCount);
            }
        }
        // printf("send cfg file successfully!\n");
        sleep(5);

        close(s_socket_fd);
        s_socket_fd = -1;
        if (serial_autoconnect(portname, syspath)) {
            goto error_exit;
        }

        // create thread to log file.
        {
            struct tm *tm;
            time_t t;

            t = time(NULL);
            tm = localtime(&t);
            sprintf(g_str_sub_log_dir, "fibolog_%02d%02d%02d%02d%02d%02d",
                tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);
            sprintf(g_qcom_logdir, "%s/%s", log_dir, g_str_sub_log_dir);
            printf("[%s] g_qcom_logdir:%s \n", __func__, g_qcom_logdir);
            printf("[%s] g_str_sub_log_dir:%s \n", __func__, g_str_sub_log_dir);

            for (i=1; i<BUF_SIZE && g_qcom_logdir[i] != 0; i++)
            {
                if (g_qcom_logdir[i] == '\\' || g_qcom_logdir[i] == '/')
                {
                    char str_dir[BUF_SIZE] = {0};
                    strcpy(str_dir, g_qcom_logdir);
                    str_dir[i] = '\0';
                    if (access(str_dir, 0)) {
                        mkdir(str_dir, 0777);
                        printf("[%s] mkdir:%s\n", __func__, str_dir);
                    }
                }
            }
            printf("[%s] Log startted, cfg_name: %s, log dir:%s\n", __func__, cfg_name, g_qcom_logdir);
        }

        FB_TCP_HEAD hd1;
        int hd_len1 = sizeof(FB_TCP_HEAD);
        memset(&hd1,0,hd_len1);

        hd1.fb_flag_1 = 0xFF;
        hd1.fb_flag_2 = 0xBB;
        hd1.package_type = FB_TYPE_START_QXDM_GATHING;//0x0C;
        hd1.package_flag = 0;
        hd1.payload_lenth = strlen(straQxdmGrabLogType);

        memset(Send_buf, 0, sizeof(Send_buf));
        memcpy(Send_buf, &hd1, hd_len1);
        size_t j;
        for (j = 0; j < strlen(straQxdmGrabLogType); j++)
        {
            Send_buf[hd_len1 + j] = straQxdmGrabLogType[j];
        }

        if (-1 == send(s_socket_fd,(char*)Send_buf, hd_len1 + strlen(straQxdmGrabLogType),0))
        {
            printf("s_socket_fd send error! errno:%d(%s)\n", errno, strerror(errno));
            return 0;
        }
        else {
            printf("s_socket_fd send successfully!\n");
        }

        pthread_attr_init (&attr);
        pthread_attr_getschedparam(&attr, &param);
        param.sched_priority=99;
        pthread_attr_setschedparam(&attr, &param);
        pthread_attr_setschedpolicy(&attr, SCHED_RR);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

        if (pthread_create(&thread_id1, &attr, thread_qcom_serial_read_log, NULL))
        {
            printf("pthread_create log thread create failed. errno:%d(%s)\n", errno, strerror(errno));
            goto error_exit;
        }
        pthread_attr_destroy(&attr);
    }

    return 0;
error_exit:
    if (rbuf) {
        free(rbuf);
    }

    printf("[%s] failed.\n", __func__);

    return -1;
}

static void *thread_ftp_upload_logfile(void *arg)
{
    ftp_info_t *pftp_info = (ftp_info_t *) arg;
    char *ftp_main_param[4] = {"ftp_main", pftp_info->ftp_ipaddr, pftp_info->username, pftp_info->password};

    ftp_main(sizeof(ftp_main_param)/sizeof(ftp_main_param[0]), ftp_main_param);

    printf("%s\n", __func__);

    return NULL;
}

/*void OpenAtPort()
{
    int fd = -1;

    if (access(PCIE_AT_PORT, F_OK) != 0)
    {
        printf("%s is not found\n", PCIE_AT_PORT);
        return;
    }
    if (chmod(PCIE_AT_PORT, (S_IRWXU | S_IRWXG | S_IRWXO)) < 0)
    {
        printf("failed to change <%s> mode,error:%d(%s)\n", PCIE_AT_PORT, errno, strerror(errno));
    }
    fd = open(PCIE_AT_PORT, O_RDWR);
    if (fd < 0)
    {
        printf("open <%s> port failed!\n", PCIE_AT_PORT);
        return;
    }
    printf("open <%s> port success!\n", PCIE_AT_PORT);
    close(fd);
}*/

static void usage(char *arg)
{
    printf("========================================\n");
    printf("Usage:\n");
    printf("%s <-d [DiagPort]> <-s [logdir]> <-f [config_file]> <-m [single_log_filesize(MB)]> <-n [max_log_filenum]>\n", arg);
    printf("%s <-s [logdir]> <-f [config_file]> <-m [single_log_filesize(MB)]> -i [address] -u [username] -w [password] <-p [DiagPort]> <-n [max filenum]>\n", arg);
    // printf("%s <-s [logdir]> <-f [config_file]> <-m [single_log_filesize(MB)]> -port [port] -addr [IP] <-p [DiagPort]> <-n [max filenum]>\n", arg);
    // printf("%s <-s [logdir]> <-f [config_file]> <-m [single_log_filesize(MB)]> -port [port] -addr [127.0.0.1] -adb <-p [DiagPort]> <-n [max filenum]>\n", arg);
    //printf("%s -h (this info)\n", arg);
    printf("example: %s\n", arg);
    printf("example: %s -f qxdm_default.cfg\n", arg);
    printf("========================================\n");
}

static void dumpusage(char *arg)
{
    printf("========================================\n");
    printf("Usage:\n");
    printf("%s <-d [DiagPort]> <-s [logdir]>\n", arg);
    printf("example: %s\n", arg);
    printf("example: %s -d /dev/ttyUSB0 -s ./logfile\n", arg);
    printf("example: %s -d /dev/ttyUSB0 -i tftp:IP\n", arg);
    printf("========================================\n");
}

static void fibo_exit_function(int signo)
{
    printf("%s, sig %d\n", __func__, signo);
    log_storage_control(NULL, 0, 0);
    g_exit_flag = 1;
    sleep(1);
    g_is_qxdm_logging = 0;
    qualcomm_serial_logstop();
    sleep(1);
    signal(SIGINT, SIG_DFL); //Enable Ctrl+C to exit
}

int qcom_log_main(int argc, char **argv)
{
    int i;
    char portname[BUF_SIZE] = {0};
    char cfg_name[BUF_SIZE] = {0};
    char syspath[BUF_SIZE] = {0};
    char log_dir[BUF_SIZE] = {0};
    ftp_info_t ftp_info;

    bool set_port=false;
    bool set_addr=false;
    bool set_ftpipaddr=false;
    bool set_username=false;
    bool set_passwd=false;

    printf("%s start.\n", __func__);

    memset(&ftp_info, 0, sizeof(ftp_info));
    getcwd(log_dir,sizeof(log_dir));

    for (i = 1; i < argc ;)
    {
        if (!strcasecmp(argv[i], "-d") && (argc - i > 1))
        {
            strcpy(portname, argv[i + 1]);
            i += 2;
        }
        else if (!strcmp(argv[i], "-s") && (argc - i > 1))
        {
            strcpy(log_dir, argv[i + 1]);
            i += 2;
        }
        else if (!strcmp(argv[i], "-f") && (argc - i > 1))
        {
            strcpy(cfg_name, argv[i + 1]);
            i += 2;
        }
        else if (!strcmp(argv[i], "-m") && (argc - i > 1))
        {
            g_qxdm_log_file_size = atoi(argv[i + 1]) << 20;
            i += 2;
        }
        else if (!strcmp(argv[i], "-n") && (argc - i > 1))
        {
            g_log_file_maxNum = atoi(argv[i + 1]);
            i += 2;
        }
        else if (!strcmp(argv[i], "-i"))
        {
            snprintf(ftp_info.ftp_ipaddr, MAXSZ, "%s", argv[i+1]);
            i += 2;
            set_ftpipaddr = true;
            printf("set_ftpservipaddr successfully,is %s", ftp_info.ftp_ipaddr);
        }
        else if (!strcmp(argv[i], "-u"))
        {
            snprintf(ftp_info.username, MAXSZ, "%s", argv[i+1]);
            i += 2;
            set_username = true;
            printf("set_username successfully,is %s", ftp_info.username);
        }
        else if (!strcmp(argv[i], "-w"))
        {
            snprintf(ftp_info.password, MAXSZ, "%s", argv[i+1]);
            i += 2;
            set_passwd = true;
            printf("set_password successfully,is %s", ftp_info.password);
        }
        else if (!strcmp(argv[i], "-port") && (argc - i > 1))
        {
            s_socket_port = atoi(argv[i + 1]);
            i += 2;
            set_port = true;
            printf("set_port successfully, is %d", s_socket_port);
        }
        else if (!strcmp(argv[i], "-addr") && (argc - i > 1))
        {
            strncpy(g_ip_addr, argv[i + 1], strlen(argv[i + 1]));
            i += 2;
            set_addr = true;
            printf("set_addr successfully,is %s", g_ip_addr);
        }
        else if (!strcmp(argv[i], "-adb"))
        {
            i++;
            s_use_adb = true;
            printf("Will use adb to download log");
        }
        else if (!strcmp(argv[i], "-nottyUSB"))
        {
            printf("diaggrab log in Drive free mode");
            i++;
            s_nottyUSB = true;
        }
        else if (!strcmp(argv[i], "-t") && (argc - i > 1))
        {
            printf("platform is qcom\r\n");
            i += 2;
        }
        else if (!strcmp(argv[i], "-h"))
        {
            usage(argv[0]);
            i++;
            return 0;
        }
        else
        {
            usage(argv[0]);
            return 0;
        }
    }

    if (strStartsWith(portname, USB_DIR_BASE)) {
        strcpy(syspath, portname);
        memset(portname, 0, sizeof(portname));
    }

    // OpenAtPort();
    printf("log_dir: %s\n", log_dir);

    if (g_qxdm_log_file_size > QCOM_QXDM_LOG_FILE_MAX_SIZE || g_qxdm_log_file_size < QCOM_QXDM_LOG_FILE_MIN_SIZE)
    {
        g_qxdm_log_file_size = QCOM_QXDM_LOG_FILE_SIZE;
    }
    printf("g_qxdm_log_file_size:%d(MB)\n", g_qxdm_log_file_size >> 20);

    if (set_port && set_addr)
    {
        s_use_socket = true;
        printf("set_addr & set_port successfully,will download log from remote");
    }

    if ((!set_port && set_addr) || (set_port && !set_addr))
    {
        usage(argv[0]);
        goto error_exit;
    }

    if ((set_ftpipaddr && (!set_username || !set_passwd)) ||
       (set_username  && (!set_ftpipaddr || !set_passwd))||
       (set_passwd    && (!set_ftpipaddr || !set_username)))
    {
        usage(argv[0]);
        goto error_exit;
    }

    if (set_ftpipaddr && set_username && set_passwd)
    {
        pthread_t thread_id;
        pthread_attr_t attr;
        struct sched_param param;

        printf("set_ftpipaddr && set_username && set_passwd,will upload log to remote via FTP");
        pthread_attr_init(&attr);
        pthread_attr_getschedparam(&attr, &param);
        param.sched_priority=98;
        pthread_attr_setschedparam(&attr, &param);
        pthread_attr_setschedpolicy(&attr, SCHED_RR);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

        if (pthread_create(&thread_id, &attr, thread_ftp_upload_logfile, &ftp_info))
        {
            printf("pthread_create log test thread create failed, errno:%d(%s)", errno, strerror(errno));
            pthread_attr_destroy(&attr);
            goto error_exit;
        }
    }

    signal(SIGINT, fibo_exit_function);
    signal(SIGTERM, fibo_exit_function);

    printf("cfg_name: %s", cfg_name);
    if (qualcomm_serial_logstart(portname, syspath, log_dir, cfg_name) < 0)
    {
        goto error_exit;
    }

    while (g_is_qxdm_logging)
    {
        sleep(5);
    }

error_exit:
    qualcomm_serial_logstop();

    return 0;
}

size_t qlog_logfile_save(int logfd, const void *buf, size_t size)
{
    int idx = qkfifo_idx(logfd);

    if (idx != -1 )
        return qkfifo_write(idx, buf, size);

    return log_poll_write(logfd, buf, size);
}

int qlog_logfile_close(int logfd)
{
    qkfifo_free(qkfifo_idx(logfd));
    return close(logfd);
}

int qlog_logfile_create_fullname(int file_type, const char *fullname, long tftp_size, int is_dump)
{
    int fd = -1;

    if (!strncmp(fullname, "/dev/null", strlen("/dev/null"))) {
        fd = open("/dev/null", O_CREAT | O_RDWR | O_TRUNC, 0444);
    }
    else if (g_tftp_server_ip != NULL)
    {
        const char *filename = fullname;
        const char *p = strchr(filename, '/');
        while (p)
        {
            p++;
            filename = p;
            p = strchr(filename, '/');
        }

        fd = tftp_write_request(filename, tftp_size);
    }
    else
    {
        fd = open(fullname, O_CREAT | O_RDWR | O_TRUNC, 0444);
        if (!is_dump)
            qkfifo_alloc(fd);
    }


    return fd;
}

int qlog_avail_space_for_dump(const char *dir, long need_MB) {
    long free_space = 0;
    struct statfs stat;

    if (!statfs(dir, &stat)) {
        free_space = stat.f_bavail*(stat.f_bsize/512)/2; //KBytes
    }
    else {
        printf("statfs %s, errno : %d (%s)\n", dir, errno, strerror(errno));
    }

    free_space = (free_space/1024);
    if (free_space < need_MB) {
        printf("free space is %ldMBytes, need %ldMB\n", free_space, need_MB);
        return 0;
    }

    return 1;
}

int qcom_dumplog_main(int argc, char **argv)
{
    int i;
    int tftp = 0;
    char dump_dir[BUF_SIZE] = "dumplogfiles";
    char portname[BUF_SIZE] = "/dev/ttyUSB0";
    char tftp_ip[BUF_SIZE] = "";
    struct termios ios;

    for (i = 1; i < argc ;)
    {
        if (!strcmp(argv[i], "-s") && (argc - i > 1))
        {
            strcpy(dump_dir, argv[i + 1]);
            i += 2;
        }
        else if(!strcmp(argv[i], "-i") && (argc - i > 1))
        {
            tftp = TFTP;
            strcpy(tftp_ip, argv[i + 1]);
            if (!strncmp(tftp_ip, TFTP_F, strlen(TFTP_F)))
            {
                g_tftp_server_ip = tftp_ip+strlen(TFTP_F);
                if (tftp_test_server(g_tftp_server_ip))
                {
                    printf("save dump to tcp server %s\n", g_tftp_server_ip);
                }
                else
                {
                    exit(1);
                }
            }
            i += 2;
        }
        else if (!strcasecmp(argv[i], "-t") && (argc - i > 1))
        {
            printf("platform is qcom\r\n");
            i += 2;
        }
        else if (!strcasecmp(argv[i], "-d") && (argc - i > 1))
        {
            strcpy(portname, argv[i + 1]);
            i += 2;
        }
        else if (!strcmp(argv[i], "-h"))
        {
            dumpusage(argv[0]);
            i++;
            return 0;
        }
        else
        {
            dumpusage(argv[0]);
            return 0;
        }
    }

    g_fd_port = open(portname, O_RDWR | O_NDELAY | O_NOCTTY);

    if(g_fd_port < 0)
    {
        printf("Fail to open %s, errno : %d (%s)\n", portname, errno, strerror(errno));
    }
    else
    {
        printf("[%s] g_fd_port[%d]\n", __func__, g_fd_port);
        memset(&ios, 0, sizeof(ios));
        tcgetattr(g_fd_port, &ios);
        cfmakeraw(&ios);
        cfsetispeed(&ios, B115200);
        cfsetospeed(&ios, B115200);
        tcsetattr(g_fd_port, TCSANOW, &ios);
    }

    if(tftp == TFTP)
    {
        sahara_catch_dump(g_fd_port, dump_dir, 1);
    }
    else
    {
#ifdef CONFIG_QCOM_DUMP
        if(access(dump_dir, 0))
        {
            mkdir(dump_dir, 0777);
            printf("[%s] mkdir:%s\n", __func__, dump_dir);
        }

        strcpy(sahara_dump_path,dump_dir);

        if(!qlog_avail_space_for_dump(dump_dir, DUMP_LOG_NEED_SPACE))
        {
            printf("no enouth disk to save dump\n");
            printf("Using tftp mode: ./logtool -i tftp:IP\n");
            return -1;
        }
        else
        {
            dump_log_collect(g_fd_port);
        }
#else
        printf("The local grab function of dump log is disabled\n");
        printf("Need to change the Makefile:config_qcom_localdump = no ---> config_qcom_localdump = yes\n");
#endif
    }

    return 0;
}

