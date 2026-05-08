/*******************************************************************
 *  CopyRight(C) 2022-2026  Fibocom Wireless Inc
 *******************************************************************
 * FileName : misc_usb.c
 * DESCRIPTION : usb code
 * Author   : Frank.zhou
 * Date     : 2022.08.22
 *******************************************************************/
#include "misc_usb.h"

int fibo_common_send_atcmd(fibo_usbdev_t *pdev, char * at_cmd)
{
    int len, i;
    int fd;
    char buf[256];

    LogInfo("open at port %s\n",pdev->at_port);
    fd = open(pdev->at_port, O_RDWR | O_NOCTTY);
    if (fd < 0)
    {
        LogInfo("open");
        return -1;
    }

    fcntl(fd, F_SETFL, fcntl(fd,F_GETFL) | O_NONBLOCK);

    // 将“at”写入串口句柄中
    len = write(fd, at_cmd, strlen(at_cmd));
    LogInfo("write %d is %s\r\n", len, at_cmd);
    if (len <= 0)
    {
        LogInfo("write");
        close(fd);
        return -1;
    }

    LogInfo("write is ok\r\n");


    for(i=0; i< 2; i++)
    {
        memset(buf, 0, sizeof(buf));
        len = read(fd, buf, sizeof(buf));
        if (len < 0)
        {
            LogInfo("read fail\r\n");
        }else{
            LogInfo("NO %d read %d is %s\r\n", i, len, buf);
            break;
        }
        usleep(300000);
    }

    sleep(1);
    close(fd);
    return 0;
}

static int usbfs_bulk_write(fibo_usbdev_t *pdev, void *data, int len)
{
    struct usbdevfs_urb bulk;
    struct usbdevfs_urb *urb = NULL;
    int ret = -1;

    // LogInfo("start\n");
    memset(&bulk, 0, sizeof(bulk));
    bulk.type = USBDEVFS_URB_TYPE_BULK;
    bulk.endpoint = pdev->bulk_ep_out;
    bulk.status = -1;
    bulk.buffer = (void *)data;
    bulk.buffer_length = MIN(len, MAX_USBFS_BULK_OUT_SIZE);
    bulk.usercontext = &bulk;

    if (len <= MAX_USBFS_BULK_OUT_SIZE && pdev->usb_need_zero_package && !(bulk.buffer_length % pdev->wMaxPacketSize))
    {
        bulk.flags = USBDEVFS_URB_ZERO_PACKET;
        // LogInfo("< USBDEVFS_URB_ZERO_PACKET >\n");
    }

    do {
        ret = ioctl(pdev->usbdev, USBDEVFS_SUBMITURB, &bulk);
    } while ((ret < 0) && (errno == EINTR));

    if (ret != 0) {
        LogInfo("USBDEVFS_SUBMITURB failed, ret: %d, errno:%d(%s)\n", ret, errno, strerror(errno));
        return -1;
    }

    do {
        urb = NULL;
        ret = ioctl(pdev->usbdev, USBDEVFS_REAPURB, &urb);
    } while ((ret < 0) && (errno == EINTR));

    // if (ret != 0) {
        // LogInfo("USBDEVFS_REAPURB failed, ret: %d, errno:%d(%s)\n", ret, errno, strerror(errno));
    // }
    // else {
        // LogInfo("USBDEVFS_REAPURB OK\n");
    // }

    if (ret == 0 && urb && urb->status == 0 && urb->actual_length) {
        return urb->actual_length;
    }

    return -1;
}

int fibo_usb_write(const void *handle, void *pbuf, int max_size, int min_size, int timeout_msec)
{
    fibo_usbdev_t *pdev = (fibo_usbdev_t *)handle;
    int cur_size = 0, ret = 0;

    // LogInfo("start\n");
    if (pdev->usbdev < 0 && pdev->ttyfd < 0) {
        LogInfo("dev port is not opened\n");
        return -1;
    }

    min_size = MAX(min_size, 1);
    if (timeout_msec == 0)
        timeout_msec = 3000;

    while (cur_size < max_size)
    {
        if (pdev->usbdev >= 0)
        {
            ret = usbfs_bulk_write(pdev, (uint8_t *)pbuf+cur_size, max_size-cur_size);
        }
        else if (pdev->ttyfd >= 0)
        {
            if (poll_wait(pdev->ttyfd, POLLOUT, timeout_msec)) {
                LogInfo("poll_wait timeout\n");
                break;
            }
            ret = write(pdev->ttyfd, (uint8_t *)pbuf+cur_size, max_size-cur_size);
        }

        if (ret <= 0) {
            LogInfo("ret:%d, errno:%d(%s)\n", ret, errno, strerror(errno));
            break;
        }
        cur_size += ret;
    }

    if (cur_size == 0 || ret <= 0)
    {
        LogInfo("cur_size:%d, max_size:%d, min_size:%d\n", cur_size, max_size, min_size);
    }

    return cur_size;
}

static int usbfs_bulk_read(fibo_usbdev_t *pdev, void *pbuf, int len, int timeout_msec)
{
    int ret = -1;
    struct usbdevfs_bulktransfer bulk;

    // LogInfo("start\n");
    bulk.ep = pdev->bulk_ep_in;
    bulk.len = MIN(len, MAX_USBFS_BULK_IN_SIZE);
    bulk.data = pbuf;
    bulk.timeout = timeout_msec;

    do {
        ret = ioctl(pdev->usbdev, USBDEVFS_BULK, &bulk);
    } while ((ret < 0) && (errno == EINTR));

    // if (ret < 0)
    // {
        // LogInfo("bulk.len: %d, ret: %d, errno:%d(%s)\n", bulk.len, ret, errno, strerror(errno));
    // }

    return ret;
}

int fibo_usb_read(const void *handle, void *pbuf, int max_size, int min_size, int timeout_msec)
{
    fibo_usbdev_t *pdev = (fibo_usbdev_t *)handle;
    int cur_size = 0, ret = 0;

    // LogInfo("start\n");
    if (pdev->usbdev < 0 && pdev->ttyfd < 0) {
        LogInfo("dev port is not opened\n");
        return -1;
    }

    min_size = MAX(min_size, 1);
    if (timeout_msec == 0)
        timeout_msec = 3000;

    while (cur_size < min_size)
    {
        if (pdev->usbdev >= 0)
        {
            ret = usbfs_bulk_read(pdev, (uint8_t *)pbuf+cur_size, max_size-cur_size, timeout_msec);
        }
        else if (pdev->ttyfd >= 0)
        {
            if (poll_wait(pdev->ttyfd, POLLIN, timeout_msec)) {
                LogInfo("poll_wait timeout\n");
                break;
            }
            ret = read(pdev->ttyfd, (uint8_t *)pbuf+cur_size, max_size-cur_size);
        }

        if (ret <= 0) {
            LogInfo("ret:%d, errno:%d(%s)\n", ret, errno, strerror(errno));
            break;
        }
        cur_size += ret;
    }

    if (cur_size == 0 || ret <= 0)
    {
        LogInfo("cur_size:%d, max_size:%d, min_size:%d\n", cur_size, max_size, min_size);
    }

    return cur_size;
}


static int fibo_get_busname_by_uevent(const char *uevent, char *busname)
{
    FILE *fp = NULL;
    char line[MAX_PATH_LEN] = {0};
    int MAJOR = 0, MINOR = 0;
    char DEVTYPE[64] = {0}, PRODUCT[64] = {0};

    fp = fopen(uevent, "r");
    if (fp == NULL) {
        LogInfo("fopen %s failed, errno:%d(%s)\n", uevent, errno, strerror(errno));
        return -1;
    }

    // LogInfo("----------------------------------------\n");
    // LogInfo("%s\n", uevent);
    while (fgets(line, sizeof(line), fp))
    {
        if (line[strlen(line) - 1] == '\n' || line[strlen(line) - 1] == '\r') {
            line[strlen(line) - 1] = 0;
        }

        // LogInfo("%s\n", line);
        if (fibo_strStartsWith(line, "MAJOR="))
        {
            MAJOR = atoi(&line[strlen("MAJOR=")]);
        }
        else if (fibo_strStartsWith(line, "MINOR="))
        {
            MINOR = atoi(&line[strlen("MINOR=")]);
        }
        //start: modify for kernel version 2.6, 2022.01.22
        else if (fibo_strStartsWith(line, "DEVICE="))
        {
            strncpy(busname, &line[strlen("DEVICE=")], MAX_PATH_LEN);
        }
        //end: modify for kernel version 2.6, 2022.01.22
        else if (fibo_strStartsWith(line, "DEVNAME="))
        {
            strncpy(busname, &line[strlen("DEVNAME=")], MAX_PATH_LEN);
        }
        else if (fibo_strStartsWith(line, "DEVTYPE="))
        {
            strncpy(DEVTYPE, &line[strlen("DEVTYPE=")], sizeof(DEVTYPE));
        }
        else if (fibo_strStartsWith(line, "PRODUCT="))
        {
            strncpy(PRODUCT, &line[strlen("PRODUCT=")], sizeof(PRODUCT));
        }
    }
    fclose(fp);
    // LogInfo("----------------------------------------\n");

    if (MAJOR != 189  || MINOR == 0 || busname[0] == 0
        || DEVTYPE[0] == 0 || PRODUCT[0] == 0
        || fibo_strStartsWith(DEVTYPE, "usb_device") == 0) {
        return -1;
    }

    return 0;
}

int fibo_get_ttyport_by_syspath_at(fibo_usbdev_t *pdev)
{
    DIR *busdir = NULL;
    struct dirent *dent = NULL;
    char acm_dir_path[2*MAX_PATH_LEN + EXTEND];
    int flag_ttyacm=0;

    busdir = opendir(pdev->syspath);
    if (busdir == NULL) {
        LogInfo("open [%s] busdir failed\n", pdev->syspath);
        return -1;
    }

    while ((dent = readdir(busdir)) != NULL)
    {
        if (fibo_strStartsWith(dent->d_name, "tty")) {
            if(strcmp(dent->d_name, "tty"))
            {
                snprintf(pdev->at_port, sizeof(pdev->at_port), "/dev/%s", dent->d_name);
                closedir(busdir);
            }
            else
            {
                closedir(busdir);
                snprintf(acm_dir_path, sizeof(acm_dir_path), "%s/tty", pdev->syspath);
                flag_ttyacm = 1;
                break;
            }
            return 0;
        }
    }

    if(flag_ttyacm == 1)
    {
        busdir = opendir(acm_dir_path);
        if (busdir == NULL) {
            printf("%s: open [%s] acm dir failed\n", __func__, acm_dir_path);
            return -1;
        }

        while ((dent = readdir(busdir)) != NULL)
        {
            if (fibo_strStartsWith(dent->d_name, "ttyACM")) {
                snprintf(pdev->at_port, sizeof(pdev->at_port), "/dev/%s", dent->d_name);
                closedir(busdir);
                return 0;
            }
        }
    }
    closedir(busdir);

    return -1;
}


int fibo_get_ttyport_by_syspath(fibo_usbdev_t *pdev)
{
    DIR *busdir = NULL;
    struct dirent *dent = NULL;
    char acm_dir_path[2*MAX_PATH_LEN + EXTEND];
    int flag_ttyacm=0;

    busdir = opendir(pdev->syspath);
    if (busdir == NULL) {
        LogInfo("open [%s] busdir failed\n", pdev->syspath);
        return -1;
    }

    while ((dent = readdir(busdir)) != NULL)
    {
        if (fibo_strStartsWith(dent->d_name, "tty")) {
            if(strcmp(dent->d_name, "tty"))
            {
                snprintf(pdev->portname, sizeof(pdev->portname), "/dev/%s", dent->d_name);
                closedir(busdir);
            }
            else
            {
                closedir(busdir);
                snprintf(acm_dir_path, sizeof(acm_dir_path), "%s/tty", pdev->syspath);
                flag_ttyacm = 1;
                break;
            }
            return 0;
        }
    }

    if(flag_ttyacm == 1)
    {
        busdir = opendir(acm_dir_path);
        if (busdir == NULL) {
            printf("%s: open [%s] acm dir failed\n", __func__, acm_dir_path);
            return -1;
        }

        while ((dent = readdir(busdir)) != NULL)
        {
            if (fibo_strStartsWith(dent->d_name, "ttyACM")) {
                snprintf(pdev->portname, sizeof(pdev->portname), "/dev/%s", dent->d_name);
                closedir(busdir);
                return 0;
            }
        }
    }
    closedir(busdir);

    return -1;
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
        LogInfo("read:%s failed\n", sys_filename);
    }
    else {
        ret_val = strtoul(buff, NULL, base);
    }
    close(fd);

    return ret_val;
}

fibo_usbdev_t *fibo_get_fibocom_device(fibo_usbdev_t *(* find_devices_in_table)(int idvendor, int idproduct, int port_check_mode), char *portname, char *syspath, int port_check_mode)
{
    DIR *usbdir = NULL;
    struct dirent *dent = NULL;
    char sys_filename[MAX_PATH_LEN] = {0};
    int idVendor = 0, idProduct = 0;
    int bNumInterfaces = 0, bConfigurationValue = 0;
    fibo_usbdev_t *pdev = NULL, *ret_udev = NULL;
    uint32_t modules_num = 0;

    usbdir = opendir(USB_DIR_BASE);
    if (usbdir == NULL) {
        return NULL;
    }

    while ((dent = readdir(usbdir)) != NULL)
    {
        if (!strcmp(dent->d_name, ".") || !strcmp(dent->d_name, "..") || fibo_strStartsWith(dent->d_name, "usb")) {
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

        pdev = (* find_devices_in_table)(idVendor, idProduct, port_check_mode);
        if (pdev != NULL) {
            LogInfo("----------------------------------\n");
//          LogInfo("ModuleName: %s\n", pdev->ModuleName);  //sometimes many modules use one usb VID PID and the ModuleName will be wrong.
            LogInfo("idVendor: %04x\n", pdev->idVendor);
            LogInfo("idProduct: %04x\n", pdev->idProduct);
            LogInfo("bNumInterfaces: %d\n", bNumInterfaces);
            LogInfo("bConfigurationValue: %d\n", bConfigurationValue);
            snprintf(sys_filename, sizeof(sys_filename), "%s/%s/uevent", USB_DIR_BASE, dent->d_name);
            fibo_get_busname_by_uevent(sys_filename, pdev->busname);
            LogInfo("busname: %s\n", pdev->busname);

            LogInfo("portname0: %s -- %s\n", pdev->portname, pdev->syspath);

            if (syspath[0] || (portname[0] && strcmp(portname, "9008"))) {
                for (pdev->used_ifnum = 0; pdev->used_ifnum<bNumInterfaces; pdev->used_ifnum++) {
                    snprintf(pdev->syspath, sizeof(pdev->syspath), "%s/%s/%s:%d.%d", USB_DIR_BASE, dent->d_name, dent->d_name, bConfigurationValue, pdev->used_ifnum);
                    fibo_get_ttyport_by_syspath(pdev);
                    LogInfo("portname: %s -- %s\n", pdev->portname, pdev->syspath);

                    if ((syspath[0] && !strcmp(syspath, pdev->syspath)) || (portname[0] &&!strcmp(portname, pdev->portname))) {
                        ret_udev = pdev;
                        modules_num = 1;
                        goto END;
                    }
                    memset(pdev->portname, 0, sizeof(pdev->portname));
                }
            }
            else
            {
                if (pdev->at_interface_num != -1){
                    snprintf(pdev->syspath, sizeof(pdev->syspath), "%s/%s/%s:%d.%d", USB_DIR_BASE, dent->d_name, dent->d_name, bConfigurationValue, pdev->at_interface_num);
                    fibo_get_ttyport_by_syspath_at(pdev);
                    LogInfo("portname1: %s -- %s\n", pdev->at_port, pdev->syspath);
                    ret_udev = pdev;
                }

                snprintf(pdev->syspath, sizeof(pdev->syspath), "%s/%s/%s:%d.%d", USB_DIR_BASE, dent->d_name, dent->d_name, bConfigurationValue, pdev->used_ifnum);
                fibo_get_ttyport_by_syspath(pdev);
                LogInfo("portname2: %s -- %s\n", pdev->portname, pdev->syspath);
                ret_udev = pdev;
                modules_num++;
            }
            LogInfo("----------------------------------\n");
        }
        usleep(10000);
    }

END:
    if (usbdir) {
        closedir(usbdir);
        usbdir = NULL;
    }

    if (modules_num == 0) {
        //LogInfo("Can not find Fibocom module.\n");
    }
    else if (modules_num == 1) {
        LogInfo("%d module match, used_ifnum:%d\n", modules_num, ret_udev->used_ifnum);
        return ret_udev;
    }
    else if (portname[0] == 0 && syspath[0] == 0)
    {
        LogInfo("%d modules found\n", modules_num);
        LogInfo("please set portname <-d /dev/XXX> or set syspath <-s /sys/bus/usb/devices/X-X/X-X:X.X>\n");
    }

    return NULL;
}

static int usbfs_check_kernel_driver(int fd, int ifnum)
{
    struct usbdevfs_getdriver getdrv;

    //LogInfo("\n");
    getdrv.interface = ifnum;
    if (ioctl(fd, USBDEVFS_GETDRIVER, &getdrv) >= 0) {
        struct usbdevfs_ioctl operate;

        operate.data = NULL;
        operate.ifno = ifnum;
        operate.ioctl_code = USBDEVFS_DISCONNECT;

        LogInfo("find interface %d has match the driver %s\n", ifnum, getdrv.driver);
        if (ioctl(fd, USBDEVFS_IOCTL, &operate) < 0) {
            LogInfo("ioctl USBDEVFS_IOCTL failed.\n");
            return -1;
        }

        LogInfo("OK.\n");
    }
    else if (errno != ENODATA) {
        LogInfo("ioctl USBDEVFS_GETDRIVER failed, errno:%d(%s)\n", errno, strerror(errno));
    }

    return 0;
}

static int usbfs_bulk_open(fibo_usbdev_t *pdev)
{
    char devdesc[MAX_PATH_LEN*2] = {0};
    char devname[MAX_PATH_LEN*2] = {0};
    size_t desc_length = 0, len = 0;
    int bInterfaceNumber = 0;

    LogInfo("start\n");
    if (pdev == NULL) {
        LogInfo("pdev is NULL\n");
        return -1;
    }

    //start: modify for kernel version 2.6, 2022.01.22
    if (strstr(pdev->busname, "/proc/bus/usb")) {
        snprintf(devname, sizeof(devname), "%s", pdev->busname);
    }
    else {
        snprintf(devname, sizeof(devname), "/dev/%s", pdev->busname);
    }
    //end: modify for kernel version 2.6, 2022.01.22

    if (access(devname, F_OK | R_OK| W_OK)) {
        LogInfo("access %s failed, errno:%d(%s)\n", devname, errno, strerror(errno));
        return -1;
    }

    pdev->usbdev = open(devname, O_RDWR | O_NOCTTY);
    if (pdev->usbdev < 0) {
        LogInfo("open %s failed, errno:%d(%s)\n", devname, errno, strerror(errno));
        return -1;
    }
    LogInfo("[%s] OK.\n", devname);

    desc_length = read(pdev->usbdev, devdesc, sizeof(devdesc));
    for (len=0; len<desc_length;)
    {
        struct usb_descriptor_header *h = (struct usb_descriptor_header *)(devdesc + len);

        if (h->bLength == sizeof(struct usb_device_descriptor) && h->bDescriptorType == USB_DT_DEVICE)
        {
            // struct usb_device_descriptor *device = (struct usb_device_descriptor *)h;
            // LogInfo("P: idVendor: %04x idProduct:%04x\n", device->idVendor, device->idProduct);
        }
        else if (h->bLength == sizeof(struct usb_config_descriptor) && h->bDescriptorType == USB_DT_CONFIG)
        {
            // struct usb_config_descriptor *config = (struct usb_config_descriptor *)h;
            // LogInfo("C: bNumInterfaces: %d\n", config->bNumInterfaces);
        }
        else if (h->bLength == sizeof(struct usb_interface_descriptor) && h->bDescriptorType == USB_DT_INTERFACE)
        {
            struct usb_interface_descriptor *interface = (struct usb_interface_descriptor *)h;

            // LogInfo("I: If#= %d Alt= %d #EPs= %d Cls=%02x Sub=%02x Prot=%02x\n",
                // interface->bInterfaceNumber, interface->bAlternateSetting, interface->bNumEndpoints,
                // interface->bInterfaceClass, interface->bInterfaceSubClass, interface->bInterfaceProtocol);
            bInterfaceNumber = interface->bInterfaceNumber;
        }
        else if (h->bLength == USB_DT_ENDPOINT_SIZE && h->bDescriptorType == USB_DT_ENDPOINT)
        {
            struct usb_endpoint_descriptor *endpoint = (struct usb_endpoint_descriptor *)h;

            if (bInterfaceNumber == pdev->used_ifnum)
            {
                if ((endpoint->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) == USB_ENDPOINT_XFER_BULK)
                {
                    if (endpoint->bEndpointAddress & USB_ENDPOINT_DIR_MASK) {
                        pdev->bulk_ep_in = endpoint->bEndpointAddress;
                        LogInfo("bulk_ep_in:%02x, bmAttributes:%02x, wMaxPacketSize:%d\n", endpoint->bEndpointAddress, endpoint->bmAttributes, endpoint->wMaxPacketSize);
                    } else {
                        pdev->bulk_ep_out = endpoint->bEndpointAddress;
                        LogInfo("bulk_ep_out:%02x, bmAttributes:%02x, wMaxPacketSize:%d\n", endpoint->bEndpointAddress, endpoint->bmAttributes, endpoint->wMaxPacketSize);
                    }
                    pdev->wMaxPacketSize = endpoint->wMaxPacketSize;
                }
            }
        }
        len += h->bLength;
    }

    if (usbfs_check_kernel_driver(pdev->usbdev, pdev->used_ifnum)) {
        return -1;
    }

    if (ioctl(pdev->usbdev, USBDEVFS_CLAIMINTERFACE, &pdev->used_ifnum)) {
        LogInfo("ioctl USBDEVFS_CLAIMINTERFACE failed, errno:%d(%s)\n", errno, strerror(errno));
        return -1;
    }

    return 0;
}

int ttyusb_Open(fibo_usbdev_t *pdev)
{
    struct termios ios;
    char *port_name = pdev->portname;

    if (access(port_name, R_OK))
    {
        LogInfo("access %s failed, errno:%d (%s)\n", port_name, errno, strerror(errno));
        return -1;
    }

    pdev->ttyfd = open (port_name, O_RDWR | O_SYNC);
    if (pdev->ttyfd < 0)
    {
        LogInfo("open %s failed, errno:%d (%s)\n", port_name, errno, strerror(errno));
        return -1;
    }

    memset(&ios, 0, sizeof(ios));
    if (tcgetattr(pdev->ttyfd, &ios))
    {
        LogInfo("tcgetattr %s failed, errno:%d (%s)\n", port_name, errno, strerror(errno));
        goto error_end;
    }

    cfmakeraw (&ios);
    cfsetispeed(&ios, B115200);
    cfsetospeed(&ios, B115200);
    if (tcsetattr (pdev->ttyfd, TCSANOW, &ios))
    {
        LogInfo("tcsetattr %s failed, errno:%d (%s)\n", port_name, errno, strerror(errno));
        goto error_end;
    }

    fcntl(pdev->ttyfd, F_SETFL, fcntl(pdev->ttyfd,F_GETFL) | O_NONBLOCK);

    LogInfo("open %s OK\n", port_name);
    return 0;
error_end:
    if (pdev->ttyfd >= 0) {
        close(pdev->ttyfd);
        pdev->ttyfd = -1;
    }
    return -1;
}

int fibo_usb_open(fibo_usbdev_t *pdev, int usbfs_only)
{
    LogInfo("start\n");

    if (pdev == NULL) {
        LogInfo("error: pdev is NULL\n");
        return -1;
    }

    if (usbfs_only) {
        if (pdev->syspath[0] != 0) {
            if (usbfs_bulk_open(pdev)) {
                return -1;
            }
        }
    } else {
        //ttyUSB first
        if (pdev->portname[0] != 0) {       
            if (ttyusb_Open(pdev)) {
                return -1;
            }
        }
        else if (pdev->syspath[0] != 0) {
            if (usbfs_bulk_open(pdev)) {
                return -1;
            }
        } else {
            LogInfo("portname and syspath are all empty.\n");
            return -1;
        }
    }

    pdev->write = fibo_usb_write;
    pdev->read = fibo_usb_read;

    return 0;
}

int fibo_usb_close(fibo_usbdev_t *pdev)
{
    if (pdev == NULL) {
        return 0;
    }

    if (pdev->ttyfd >= 0) {
        close(pdev->ttyfd);
        pdev->ttyfd = -1;
    }

    if (pdev->usbdev >= 0) {
        ioctl(pdev->usbdev, USBDEVFS_RELEASEINTERFACE, &pdev->used_ifnum);
        close(pdev->usbdev);
        pdev->usbdev = -1;
    }

    if (pdev->pcie_fd >= 0) {
        close(pdev->pcie_fd);
        pdev->pcie_fd = -1;
    }

    return 0;
}


