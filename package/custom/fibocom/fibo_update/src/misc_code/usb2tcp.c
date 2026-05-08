/*******************************************************************
 *  CopyRight(C) 2022-2026  Fibocom Wireless Inc
 *******************************************************************
 * FileName : usb2tcp.c
 * DESCRIPTION : upgrade_tool for USB and PCIE of Fibocom modules
 * Author   : Frank.zhou
 * Date     : 2022.08.22
 *******************************************************************/
#include "usb2tcp.h"
#include <endian.h> //__BYTE_ORDER

static int fd_sockets[2] = {-1, -1};


#if __BYTE_ORDER != __LITTLE_ENDIAN
#define cpu_to_le32(X) (uint32_t)((((X)&0xFF000000) >> 24) | (((X)&0x00FF0000) >> 8) | (((X)&0x0000FF00) << 8) | (((X)&0x000000FF) << 24))
#else
#define cpu_to_le32(X) (X)
#endif
#define le32_to_cpu(X) cpu_to_le32(X)

//start: tcp client function
#if 1
fibo_usbdev_t s_usb2tcp_dev =
{
    .ModuleName = "",
    .idVendor   = 0,
    .idProduct  = 0,
    .used_ifnum  = 0,
    .ttyfd = -1,
    .usbdev = -1,
    .tcp_client_fd = -1,
    .pcie_fd = -1,
};

tcp_tlv_modules tlv_usb;
int tcp_connect_module_host(const char *ip_portname, int *p_idVendor, int *p_idProduct)
{
    int ret = -1, i = 0;
    char *ip_str = NULL;
    char *tcp_port = NULL;
    struct sockaddr_in sockaddr;

    LogInfo("start\n");
    if (ip_portname == NULL) {
        LogInfo("ip_portname is NULL\n");
        return -1;
    }

    LogInfo("ip_portname: %s\n", ip_portname);
    ip_str = strdup(ip_portname);
    tcp_port = strchr(ip_str, ':');
    *tcp_port++ = '\0';
    if (atoi(tcp_port) <= 0 || atoi(tcp_port) >= 65536) {
        goto END;
    }

    LogInfo("tcp_port: %d\n", atoi(tcp_port));
    s_usb2tcp_dev.tcp_client_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (s_usb2tcp_dev.tcp_client_fd < 0) {
        LogInfo("socket failed, errno: %d(%s)\n", errno, strerror(errno));
        goto END;
    }

    memset(&sockaddr, 0, sizeof(sockaddr));
    sockaddr.sin_family = AF_INET;
    sockaddr.sin_addr.s_addr = inet_addr(ip_str);
    sockaddr.sin_port = htons(atoi(tcp_port));
    LogInfo("sin_port: %d\n", sockaddr.sin_port);

    for (i=0; i<100; i++) {
        ret = connect(s_usb2tcp_dev.tcp_client_fd, (struct sockaddr *)&sockaddr, sizeof(sockaddr));
        // LogInfo("ret:%d\n", ret);
        if (ret == 0) {
            break;
        }
        LogInfo("no connect, errno: %d(%s)\n", errno, strerror(errno));
        LogInfo("retry\n");
        sleep(1);
    }

    if (ret < 0) {
        LogInfo("connect failed, errno: %d(%s)\n", errno, strerror(errno));
        goto END;
    }

    LogInfo("s_usb2tcp_dev.tcp_client_fd: %d\n", s_usb2tcp_dev.tcp_client_fd);
    memset(&tlv_usb, 0, sizeof(tlv_usb));
    ret = read(s_usb2tcp_dev.tcp_client_fd, &tlv_usb, sizeof(tlv_usb));
    if (ret != sizeof(tlv_usb)) {
        LogInfo("read failed, ret:%d, errno: %d(%s)\n", ret, errno, strerror(errno));
        goto END;
    }
    *p_idVendor = tlv_usb.idVendor;
    *p_idProduct = tlv_usb.idProduct;
    s_usb2tcp_dev.ModuleName = tlv_usb.ModuleName;

    ret = fcntl(s_usb2tcp_dev.tcp_client_fd, F_GETFL);
    if (ret == -1) {
        LogInfo("fcntl F_GETFL failed, errno: %d(%s)\n", errno, strerror(errno));
        goto END;
    }

//#MBB0152-227 modify by fuxuanqi 2024/12/2 begin
    if(strstr(s_usb2tcp_dev.ModuleName, "UDX710"))
    {
        ret = fcntl(s_usb2tcp_dev.tcp_client_fd, F_SETFL, ret);
    }
    else
    {
        ret = fcntl(s_usb2tcp_dev.tcp_client_fd, F_SETFL, ret | O_NONBLOCK);
    }
//#MBB0152-227 modify by fuxuanqi 2024/12/2 end

    if (ret == -1) {
        LogInfo("fcntl F_SETFL failed, errno: %d(%s)\n", errno, strerror(errno));
        goto END;
    }

    LogInfo("================================================\n");
    LogInfo("type: 0x%08x\n", tlv_usb.type);
    LogInfo("ModuleName:%s\n", s_usb2tcp_dev.ModuleName);
    LogInfo("idVendor:0x%04x, idProduct:0x%04x\n",tlv_usb.idVendor, tlv_usb.idProduct);
    LogInfo("================================================\n");
    ret = 0;
END:
    if (ip_str) {
        free(ip_str);
        ip_str = NULL;
    }

    if (ret) {
        if (s_usb2tcp_dev.tcp_client_fd >=0) {
            close(s_usb2tcp_dev.tcp_client_fd);
            s_usb2tcp_dev.tcp_client_fd = -1;
        }
    }

    return ret;
}

static int tcp_write_once(int fd, void *pbuf, int size, int timeout_msec)
{
    tcp_tlv tlv = {FIBO_USB2TPC_MASK, size};
    int cur_size = 0, retval = 0;

    // LogInfo("start\n");
    retval = write(fd, &tlv, sizeof(tlv));
    if (retval != sizeof(tlv))
    {
        LogInfo("write:%d, errno: %d(%s)\n", size, errno, strerror(errno));
        return 0;
    }

    while (cur_size < size)
    {
        if (poll_wait(fd, POLLOUT, timeout_msec)){
            LogInfo("poll_wait timeout\n");
            break;
        }

        retval = write(fd, (uint8_t *)pbuf+cur_size, size-cur_size);
        if (retval <= 0)
        {
            LogInfo("write failed, retval:%d, errno: %d(%s)\n", retval, errno, strerror(errno));
            break;
        }
        cur_size += size;
    }

    if (cur_size < size)
    {
        LogInfo("cur_size:%d, size:%d\n", cur_size, size);
    }

    return cur_size;
}

static int usb2tcp_client_write(const void *handle, void *pbuf, int max_size, int min_size, int timeout_msec)
{
    fibo_usbdev_t *pdev = (fibo_usbdev_t *)handle;
    int cur_size = 0, retval = 0;

    // LogInfo("start\n");
    if (pdev->tcp_client_fd < 0) {
        LogInfo("tcp_socket_fdt is not opened\n");
        return -1;
    }

    if (timeout_msec == 0)
        timeout_msec = 3000;

    while (cur_size < max_size)
    {
        if (poll_wait(pdev->tcp_client_fd, POLLOUT, timeout_msec)) {
            LogInfo("poll_wait timeout\n");
            break;
        }

        retval = tcp_write_once(pdev->tcp_client_fd, (uint8_t *)pbuf+cur_size, max_size-cur_size, timeout_msec);
        if (retval <= 0) {
            break;
        }
        cur_size += retval;
    }

    if (cur_size == 0 || retval <= 0)
    {
        LogInfo("cur_size:%d, max_size:%d, min_size:%d\n", cur_size, max_size, min_size);
    }

    return cur_size;
}


static int tcp_read_once(int fd, void *pbuf, int size, int timeout_msec)
{
    static tcp_tlv tlv = {FIBO_USB2TPC_MASK, 0};
    int cur_size = 0, retval = 0;

    // LogInfo("start\n");
    if (tlv.length == 0)
    {
        retval = read(fd, &tlv, sizeof(tlv));
        if (retval != sizeof(tlv))
        {
            LogInfo("error, retval:%d, errno: %d(%s)\n", retval, errno, strerror(errno));
            return -1;
        }

        if (tlv.type != FIBO_USB2TPC_MASK)
        {
            LogInfo("error: tlv.type: 0x%x\n", tlv.type);
            return -1;
        }
    }

    if (size > tlv.length) {
        size = tlv.length;
    }

    while (cur_size < size)
    {
        if (poll_wait(fd, POLLIN, timeout_msec)) {
            LogInfo("poll_wait timeout\n");
            break;
        }

        retval = read(fd, (uint8_t *)pbuf+cur_size, size-cur_size);
        if (retval <= 0)
        {
            LogInfo("error, retval:%d, errno: %d(%s)\n", retval, errno, strerror(errno));
            break;
        }
        cur_size += retval;
    }

    if (cur_size < size)
    {
        LogInfo("cur_size:%d, size:%d\n", cur_size, size);
    }
    tlv.length -= size;

    return cur_size;
}

static int usb2tcp_client_read(const void *handle, void *pbuf, int max_size, int min_size, int timeout_msec)
{
    fibo_usbdev_t *pdev = (fibo_usbdev_t *)handle;
    int cur_size = 0, retval = 0;

    // LogInfo("start\n");
    if (pdev->tcp_client_fd < 0) {
        LogInfo("tcp_client_fd is not opened\n");
        return -1;
    }

    if (min_size == 0)
        min_size = 1;

    timeout_msec = 150000; //Increase the timeout_msec for wifi or weak network
    while (cur_size < min_size)
    {
        if (poll_wait(pdev->tcp_client_fd, POLLIN, timeout_msec)) {
            LogInfo("timeout\n");
            break;
        }
        retval = tcp_read_once(pdev->tcp_client_fd, (uint8_t *)pbuf+cur_size, max_size-cur_size, timeout_msec);
        if (retval <= 0) {
            break;
        }
        cur_size += retval;
    }

    if (cur_size == 0 || retval <= 0)
    {
        LogInfo("cur_size:%d, max_size:%d, min_size:%d\n", cur_size, max_size, min_size);
    }

    return cur_size;
}

fibo_usbdev_t *fibo_usb2tcp_open(void)
{
    if (s_usb2tcp_dev.tcp_client_fd >= 0) {
        s_usb2tcp_dev.write = usb2tcp_client_write;
        s_usb2tcp_dev.read  = usb2tcp_client_read;
        return &s_usb2tcp_dev;
    }

    return NULL;
}

int fibo_usb2tcp_close(fibo_usbdev_t *pdev)
{
    if (pdev != NULL && pdev->tcp_client_fd >= 0) {
        close(pdev->tcp_client_fd);
        pdev->tcp_client_fd = -1;
    }

    return 0;
}
#endif
//end: tcp client function

//start: tcp server function
#if 1
static int usb2tcp_server_read(int fd, void *pbuf, int size)
{
    int cur_size = 0;

    // LogInfo("start\n");
    while (cur_size < size)
    {
        int ret = read(fd, (uint8_t *)pbuf+cur_size, size-cur_size);
        if (ret > 0) {
            cur_size += ret;
        }
        else if (ret < 0 && errno == EAGAIN)
        {
            struct pollfd pollfds[] = {{fd, POLLIN, 0}};
            ret = poll(pollfds, 1, -1);
            if (ret < 0) {
                LogInfo("poll failed, errno:%d(%s) \n", errno, strerror(errno));
                break;
            }
            if (pollfds[0].revents & (POLLERR | POLLHUP | POLLNVAL)) {
                LogInfo("error\n");
                break;
            }
        }
        else
        {
            LogInfo("fd:%d, read:%d, errno:%d(%s)\n", fd, ret, errno, strerror(errno));
            break;
        }
    }

    if (cur_size < size)
    {
        LogInfo("fd:%d, cur_size:%d, size:%d\n", fd, cur_size, size);
    }

    return cur_size;
}

static int usb2tcp_server_write(int fd, const void *pbuf, int size)
{
    int cur_size = 0;

    // LogInfo("start\n");
    while (cur_size < size)
    {
        int ret = write(fd, (uint8_t *)pbuf+cur_size, size-cur_size);
        if (ret > 0) {
            cur_size += ret;
        }
        else if (ret < 0 && errno == EAGAIN)
        {
            struct pollfd pollfds[] = {{fd, POLLOUT, 0}};
            ret = poll(pollfds, 1, -1);
            if (ret < 0) {
                LogInfo("poll failed, errno:%d(%s) \n", errno, strerror(errno));
                break;
            }
            if (pollfds[0].revents & (POLLERR | POLLHUP | POLLNVAL)) {
                LogInfo("error\n");
                break;
            }
        }
        else
        {
            LogInfo("fd:%d, write:%d, errno: %d(%s)\n", fd, ret, errno, strerror(errno));
            break;
        }
    }

    if (cur_size < size)
    {
        LogInfo("fd:%d, cur_size:%d, size:%d\n", fd, cur_size, (int)size);
    }

    return cur_size;
}

static volatile int g_tcp_server_processing = 1;

static void *thread_usb_bulk_read(void *usb_handle)
{
    fibo_usbdev_t *pdev = (fibo_usbdev_t *)usb_handle;
    void *databuf = NULL;

    if (fd_sockets[1] < 0) {
        LogInfo("error, fd_sockets[1]: %d\n", fd_sockets[1]);
        return NULL;
    }

    databuf = malloc(MAX_USBFS_BULK_IN_SIZE);
    if (databuf == NULL) {
        LogInfo("allocate databuf failed.\n");
        return NULL;
    }

    while (g_tcp_server_processing && usb_handle)
    {
        int retval = pdev->read(usb_handle, databuf, MAX_USBFS_BULK_IN_SIZE, 1, 300000); /* lishuai@2022-08-08 for jira MBB0051-258. */
        if (retval <= 0) {
            LogInfo("retval:%d\n", retval);
            break;
        }

        retval = write(fd_sockets[1], databuf, retval);
        if (retval <= 0) {
            LogInfo("retval:%d\n", retval);
            break;
        }

        retval = read(fd_sockets[1], databuf, 32);
        if (retval <= 0) {
            LogInfo("retval:%d\n", retval);
            break;
        }
    }

    close(fd_sockets[1]);
    if (databuf) {
        free(databuf);
        databuf = NULL;
    }

    g_tcp_server_processing = 0;

    return NULL;
}

static int wait_client_connect(int socket_fd, struct sockaddr_in *p_remote_ipaddr)
{
    int client_fd = -1;
    struct ifaddrs *ifap = NULL;
    struct ifaddrs *curp = NULL;
    socklen_t ipaddr_len = sizeof(struct sockaddr_in);

    LogInfo("\n");
    //start: show host ip addr
    if (getifaddrs(&ifap) || ifap == NULL) {
        LogInfo("getifaddrs failed!\n");
        return -1;
    }

    curp = ifap;
    while (curp != NULL)
    {
        if ((curp->ifa_addr != NULL) && (curp->ifa_addr->sa_family == AF_INET))
        {
            char ipv4_addr[50] = {0};

            //IPv4 addr
            inet_ntop(AF_INET, &((struct sockaddr_in *)curp->ifa_addr)->sin_addr, ipv4_addr, sizeof(ipv4_addr));
            if (!strstr(ipv4_addr, "127.0.0.1")) {
                LogInfo("============================\n");
                LogInfo("Current host IP: %s\n", ipv4_addr);
                LogInfo("============================\n");
            }
        }
        curp = curp->ifa_next;
    }
    freeifaddrs(ifap);
    //end: show host ip addr

    LogInfo("Waiting for connection\n");
    if (listen(socket_fd, 1)) {
        LogInfo("listen socket_fd: %d failed\n", socket_fd);
        return -1;
    }

    client_fd = accept(socket_fd, (struct sockaddr *)p_remote_ipaddr, &ipaddr_len);
    if (client_fd < 0) {
        LogInfo("accept failed, errno: %d(%s)\n", errno, strerror(errno));
        return -1;
    }

    LogInfo("============================\n");
    LogInfo("%s:%d connected.\n", inet_ntoa(p_remote_ipaddr->sin_addr), p_remote_ipaddr->sin_port);
    LogInfo("============================\n");

    return client_fd;
}

static int tcp_server_start(int tcp_port, struct sockaddr_in *p_remote_ipaddr)
{
    int reuse_addr = 1, socket_fd = -1, client_fd = -1;
    struct sockaddr_in sockaddr;

    LogInfo("tcp_port: %d\n", tcp_port);
    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd < 0) {
        LogInfo("socket tcp_port: %d failed, errno: %d(%s)\n", tcp_port, errno, strerror(errno));
        goto ERROR;
    }

    memset(&sockaddr, 0, sizeof(sockaddr));
    sockaddr.sin_family = AF_INET;
    sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    sockaddr.sin_port = htons(tcp_port);
    LogInfo("sin_port: %d\n", sockaddr.sin_port);
    if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &reuse_addr, sizeof(reuse_addr))) {
        LogInfo("setsockopt tcp_port: %d failed, errno: %d(%s)\n", tcp_port, errno, strerror(errno));
        goto ERROR;
    }

    if (bind(socket_fd, (struct sockaddr *)&sockaddr, sizeof(sockaddr)) < 0)
    {
        LogInfo("bind tcp_port: %d failed, errno: %d(%s)\n", tcp_port, errno, strerror(errno));
        goto ERROR;
    }

    client_fd = wait_client_connect(socket_fd, p_remote_ipaddr);
    if (client_fd < 0) {
        LogInfo("wait_client_connect failed, client_fd: %d\n", client_fd);
        goto ERROR;
    }

    return client_fd;
ERROR:
    if (socket_fd >= 0) {
        close(socket_fd);
        socket_fd = -1;
    }
    return -1;
}

int usb2tcp_server_main(fibo_usbdev_t *pdev)
{
    int ret = -1, client_fd = -1, qusb_fd = -1;
    tcp_tlv_modules tlv_usb;
    pthread_t thread_id;
    pthread_attr_t usb_thread_attr;
    struct sockaddr_in remote_ipaddr;
    char *pbuf = NULL;
    int tcp_port = 9008;

    LogInfo("start\n");

    memset(&remote_ipaddr, 0, sizeof(remote_ipaddr));

    pbuf = malloc(MAX_USBFS_BULK_IN_SIZE * 3);
    if (pbuf == NULL) {
        LogInfo("allocate pbuf failed.");
        goto END;
    }

    client_fd = tcp_server_start(tcp_port, &remote_ipaddr);
    if (client_fd < 0) {
        LogInfo("tcp_server_start failed, client_fd: %d\n", client_fd);
        goto END;
    }
    // LogInfo("client_fd: %d\n", client_fd);

    if (socketpair(AF_LOCAL, SOCK_STREAM, 0, fd_sockets) < 0) {
        printf("socketpair create failed, errno(%d): %s\n", errno, strerror(errno));
        goto END;
    }

    pthread_attr_init(&usb_thread_attr);
    pthread_attr_setdetachstate(&usb_thread_attr, PTHREAD_CREATE_DETACHED);
    if (pthread_create(&thread_id, &usb_thread_attr, thread_usb_bulk_read, pdev)) {
        printf("pthread_create create failed\n");
        goto END;
    }
    qusb_fd = fd_sockets[0];
    if (qusb_fd < 0) {
        LogInfo("qusb_fd is failed, qusb_fd: %d\n", qusb_fd);
        goto END;
    }
    // LogInfo("qusb_fd: %d\n", qusb_fd);

    //send the modules info to remote host
    tlv_usb.type = cpu_to_le32(FIBO_USB2TPC_MASK);
    tlv_usb.length = cpu_to_le32(sizeof(tlv_usb) - sizeof(tlv_usb.type));
    tlv_usb.idVendor = cpu_to_le32(pdev->idVendor);
    tlv_usb.idProduct = cpu_to_le32(pdev->idProduct);
    strncpy(tlv_usb.ModuleName, pdev->ModuleName, sizeof(tlv_usb.ModuleName));
    ret = write(client_fd, &tlv_usb, sizeof(tlv_usb));
    if (ret != sizeof(tlv_usb)) {
        LogInfo("write tlv_usb to client_fd failed, ret: %d\n", ret);
        goto END;
    }

    fcntl(qusb_fd, F_SETFL, fcntl(qusb_fd, F_GETFL) | O_NONBLOCK); //set qusb_fd O_NONBLOCK
    fcntl(client_fd, F_SETFL, fcntl(client_fd, F_GETFL) | O_NONBLOCK); //set client_fd O_NONBLOCK

    LogInfo("tcp processing\n");
    while (g_tcp_server_processing)
    {
        struct pollfd pollfds[] = {{qusb_fd, POLLIN, 0}, {client_fd, POLLIN, 0}};
        int i, nevents_num = sizeof(pollfds)/sizeof(pollfds[0]);

        do {
            ret = poll(pollfds, nevents_num, -1);
        } while (ret < 0 && errno == EINTR);

        if (ret < 0)
        {
            LogInfo("poll failed, ret:%d, errno:%d(%s)\n", ret, errno, strerror(errno));
            goto END;
        }

        if (pollfds[0].revents & (POLLERR | POLLHUP | POLLNVAL))
        {
            LogInfo("poll failed, qusb_fd: %d, revents: 0x%04x\n", qusb_fd, pollfds[0].revents);
            goto END;
        }

        if (pollfds[1].revents & (POLLERR | POLLHUP | POLLNVAL))
        {
            LogInfo("poll failed, client_fd: %d, revents: 0x%04x\n", client_fd, pollfds[1].revents);
            goto END;
        }

        for (i=0; i<nevents_num; i++)
        {
            int fd = pollfds[i].fd;
            tcp_tlv tlv = {FIBO_USB2TPC_MASK, 0};

            if ((pollfds[i].revents & POLLIN) == 0) {
                continue;
            }

            if (fd == qusb_fd)
            {
                int read_size = read(qusb_fd, pbuf, MAX_USBFS_BULK_IN_SIZE);
                if (read_size <= 0)
                {
                    LogInfo("qusb_fd:%d read_size:%d, errno: %d(%s)\n", fd, read_size, errno, strerror(errno));
                    goto END;
                }

                ret = write(qusb_fd, pbuf, 1);//wakeup thread_usb_bulk_read
                if (ret != 1)
                {
                    LogInfo("write failed, ret: %d, read_size: %d\n", ret, read_size);
                    goto END;
                }

                tlv.type = cpu_to_le32(FIBO_USB2TPC_MASK);
                tlv.length = cpu_to_le32(read_size);
                ret = usb2tcp_server_write(client_fd, &tlv, sizeof(tlv));
                if (ret != sizeof(tlv))
                {
                    LogInfo("usb2tcp_server_write failed, ret: %d, tlv size: %d\n", ret, (int)sizeof(tlv));
                    goto END;
                }

                ret = usb2tcp_server_write(client_fd, pbuf, read_size);
                if (ret != read_size)
                {
                    LogInfo("usb2tcp_server_write failed, ret: %d, read_size: %d\n", ret, read_size);
                    goto END;
                }
            }
            else if (fd == client_fd)
            {
                int read_size = usb2tcp_server_read(client_fd, &tlv, sizeof(tlv));
                if (read_size != sizeof(tlv))
                {
                    LogInfo("client_fd: %d, read_size: %d\n", client_fd, read_size);
                    goto END;
                }

                if (le32_to_cpu(tlv.type) != FIBO_USB2TPC_MASK)
                {
                    LogInfo("tlv.type:0x%08X is error\n", le32_to_cpu(tlv.type));
                    break;
                }

                read_size = le32_to_cpu(tlv.length);
                ret = usb2tcp_server_read(client_fd, pbuf, read_size);
                if (ret != read_size)
                {
                    LogInfo("ret: %d, read_size: %d\n", ret, read_size);
                    goto END;
                }

                ret = pdev->write(pdev, pbuf, read_size, read_size, 3000);
                if (ret != read_size) {
                    LogInfo("ret: %d, read_size: %d\n", ret, read_size);
                    goto END;

                }
            }
        }
    }

END:
    LogInfo("\n");
    g_tcp_server_processing = 0;
    if (qusb_fd >= 0) {
        close(qusb_fd);
        qusb_fd = -1;
    }
    LogInfo("\n");
    if (client_fd >= 0) {
        close(client_fd);
        client_fd = -1;
    }
    LogInfo("\n");
    if (pbuf) {
        free(pbuf);
        pbuf = NULL;
    }
    LogInfo("\n");
    usleep(100000); //wait 100ms for usb_tcp thread close
    LogInfo("============================\n");
    LogInfo("%s:%d disconnect\n", inet_ntoa(remote_ipaddr.sin_addr), remote_ipaddr.sin_port);
    LogInfo("============================\n");

    return 0;
}
#endif
//end: tcp server function


