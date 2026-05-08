
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <net/if.h>
#include <net/route.h>
#include <errno.h>
#include "misc_usb.h"

/*
### Handshake
Upon connecting, both sides will send a 4-byte handshake message to ensure they
are speaking the same protocol. This consists of the ASCII characters "FB"
followed by a 2-digit base-10 ASCII version number. For example, the version 1
handshake message will be [FB01].

If either side detects a malformed handshake, it should disconnect.

The protocol version to use must be the minimum of the versions sent by each
side; if either side cannot speak this protocol version, it should disconnect.

### Fastboot Data
Once the handshake is complete, fastboot data will be sent as follows:

    [data_size][data]

Where data\_size is an unsigned 8-byte big-endian binary value, and data is the
fastboot packet. The 8-byte length is intended to provide future-proofing even
though currently fastboot packets have a 4-byte maximum length.

### Example
In this example the fastboot host queries the device for two variables,
"version" and "none".

    Host    <connect to the device on port 5555>
    Host    FB01
    Device  FB01
    Host    [0x00][0x00][0x00][0x00][0x00][0x00][0x00][0x0E]getvar:version
    Device  [0x00][0x00][0x00][0x00][0x00][0x00][0x00][0x07]OKAY0.4
    Host    [0x00][0x00][0x00][0x00][0x00][0x00][0x00][0x0B]getvar:none
    Device  [0x00][0x00][0x00][0x00][0x00][0x00][0x00][0x14]FAILUnknown variable
    Host    <disconnect>
*/

#define SS_DEFAULT_IFNAME "mgif_raw"
#define SS_DEFAULT_IP "192.168.0.100" // server has a fixed IP 192.168.0.98
#define SS_DEFAULT_ROUTE "192.168.0.0"

#define SS_SOCK_CTRL_MAX 5

//[data_size][data]   8-byte length
#define SS_FASTBOOT_HANDSHAKE "FB01"

enum SOCK_USED
{
    SOCK_UNUSED = 0,
    SOCK_INUSE = 1
};

struct sock_control
{
    int used;
    int sock;
    long handle;
};

struct sock_control g_sock_ctrl[SS_SOCK_CTRL_MAX] = {[0 ... SS_SOCK_CTRL_MAX - 1] = {SOCK_UNUSED, -1, -1}};

static int ss_add_sock(int sock, void *handle)
{
    int i = 0;
    int max_len = sizeof(g_sock_ctrl) / sizeof(g_sock_ctrl[0]);

    for (i = 0; i < max_len; i++)
    {
        if (g_sock_ctrl[i].used == SOCK_UNUSED)
        {
            g_sock_ctrl[i].sock = sock;
            g_sock_ctrl[i].handle = (long)handle;
            g_sock_ctrl[i].used = SOCK_INUSE;
            break;
        }
    }

    LogInfo("fd=%d %s\n", sock, i < max_len ? "done" : "fail");
    return i < max_len ? 0 : -1;
}

static void ss_del_sock(int sock)
{
    int i = 0;
    int max_len = sizeof(g_sock_ctrl) / sizeof(g_sock_ctrl[0]);

    for (i = 0; i < max_len; i++)
    {
        if ((g_sock_ctrl[i].used == SOCK_INUSE) && (g_sock_ctrl[i].sock == sock))
        {
            g_sock_ctrl[i].sock = -1;
            g_sock_ctrl[i].handle = -1;
            g_sock_ctrl[i].used = SOCK_UNUSED;
            break;
        }
    }

    LogInfo("fd=%d %s\n", sock, i < max_len ? "done" : "fail");
    return;
}

static int ss_find_sock_by_handle(void *handle)
{
    int i = 0;
    int max_len = sizeof(g_sock_ctrl) / sizeof(g_sock_ctrl[0]);
    int sock = -1;

    for (i = 0; i < max_len; i++)
    {
        if ((g_sock_ctrl[i].used == SOCK_INUSE) && ((long)g_sock_ctrl[i].handle == (long)handle))
        {
            sock = g_sock_ctrl[i].sock;
            break;
        }
    }

    return sock;
}

// fastboot for net data format : [data_size][data]
int ss_make_fastboot_cmd(char *buffer, char *cmd)
{
    int i = 0;
    int data_size = 0;

    //[data_size] byte0~byte8
    data_size = strlen(cmd);
    for (i = 8; i > 0; i--)
    {
        buffer[i] = (data_size >> (8 * i)) & 0xFF;
    }

    memcpy(buffer + 8, cmd, data_size);

    return 0;
}

static int ss_tcp_write(int sock, const char *buf, int buf_len)
{
    int flags = 0; // MSG_DONTROUTE
    int send_len;
    int left_len;
    char *prt = (char *)buf;

    left_len = buf_len;
    while (left_len)
    {
        if (poll_wait(sock, POLLOUT, 5000))
        {
            LogInfo("fd=%d poll_wait POLLOUT timeout\n", sock);
            break;
        }
        send_len = send(sock, (void *)prt, left_len, flags);
        if (send_len < 0)
        {
            LogInfo("buf[%d]=%s fail %d:%s\n", left_len, prt, errno, strerror(errno));
            return -1;
        }

        left_len -= send_len;
        prt += send_len;
    }

    return buf_len;
}

static int ss_tcp_read(int sock, char *buf, int buf_len)
{
    int flags = 0; // MSG_DONTWAIT
    int recv_len;

    recv_len = recv(sock, (void *)buf, buf_len, flags);
    if (recv_len <= 0)
    {
        LogInfo("recv fail %d:%s\n", errno, strerror(errno));
        return -1;
    }

    return recv_len;
}

static int ss_if_set_up(const char *ifname)
{
    int sock = -1;
    struct ifreq ifr;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        LogInfo("socket create error %d:%s\n", errno, strerror(errno));
        return -1;
    }

    memset(&ifr, 0, sizeof(ifr));
    strcpy(ifr.ifr_name, ifname);

    if (ioctl(sock, SIOCGIFFLAGS, &ifr) < 0)
    {
        LogInfo("socket SIOCGIFFLAGS error %d:%s\n", errno, strerror(errno));
        close(sock);
        return -1;
    }

    ifr.ifr_ifru.ifru_flags |= IFF_UP;

    if (ioctl(sock, SIOCSIFFLAGS, &ifr) < 0)
    {
        LogInfo("socket SIOCSIFFLAGS error %d:%s\n", errno, strerror(errno));
        close(sock);
        return -1;
    }

    LogInfo("'%s' up done\n", ifname);
    close(sock);

    return 0;
}

static int ss_if_set_ip(const char *ifname, const char *ip)
{
    int sock = -1;
    struct ifreq ifr;
    struct sockaddr_in *sin;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        LogInfo("socket create error %d:%s\n", errno, strerror(errno));
        return -1;
    }

    memset(&ifr, 0, sizeof(ifr));
    sin = (struct sockaddr_in *)&ifr.ifr_addr;
    sin->sin_family = AF_INET;
    sin->sin_addr.s_addr = inet_addr(ip);
    strcpy(ifr.ifr_name, ifname);

    if (ioctl(sock, SIOCSIFADDR, &ifr) < 0)
    {
        LogInfo("socket SIOCSIFADDR '%s' ip %s error %d:%s\n", ifname, ip, errno, strerror(errno));
        close(sock);
        return -1;
    }

    LogInfo("'%s' set ip %s done\n", ifname, ip);
    close(sock);

    return 0;
}

static int ss_if_add_route(const char *ifname, const char *route)
{
    int sock = -1;
    struct rtentry rt;
    struct sockaddr_in *sin;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        LogInfo("socket create error %d:%s\n", errno, strerror(errno));
        return -1;
    }

    memset(&rt, 0, sizeof(rt));

    // gateway
    sin = (struct sockaddr_in *)&rt.rt_gateway;
    sin->sin_family = AF_INET;
    sin->sin_addr.s_addr = inet_addr("0.0.0.0");

    // dst ip
    sin = (struct sockaddr_in *)&rt.rt_dst;
    sin->sin_family = AF_INET;
    sin->sin_addr.s_addr = inet_addr(route);

    // mask
    sin = (struct sockaddr_in *)&rt.rt_genmask;
    sin->sin_family = AF_INET;
    sin->sin_addr.s_addr = inet_addr("255.255.255.0");

    rt.rt_dev = (char *)ifname;
    rt.rt_flags = RTF_UP; // | RTF_GATEWAY;

    if (ioctl(sock, SIOCADDRT, &rt) < 0)
    {
        LogInfo("socket SIOCADDRT '%s' route %s error %d:%s\n", ifname, route, errno, strerror(errno));
        close(sock);
        return -1;
    }

    LogInfo("'%s' add route %s done\n", ifname, route);
    close(sock);

    return 0;
}

static int ss_if_del_route(const char *ifname, const char *route, const char *genmask)
{
    int sock = -1;
    struct rtentry rt;
    struct sockaddr_in *sin;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        LogInfo("socket create error %d:%s\n", errno, strerror(errno));
        return -1;
    }

    memset(&rt, 0, sizeof(rt));

    // dst ip
    sin = (struct sockaddr_in *)&rt.rt_dst;
    sin->sin_family = AF_INET;
    sin->sin_addr.s_addr = inet_addr(route);

    // mask
    sin = (struct sockaddr_in *)&rt.rt_genmask;
    sin->sin_family = AF_INET;
    sin->sin_addr.s_addr = inet_addr(genmask);

    rt.rt_dev = (char *)ifname;
    rt.rt_flags = RTF_UP | RTF_GATEWAY;

    if (ioctl(sock, SIOCDELRT, &rt) < 0)
    {
        LogInfo("socket SIOCDELRT '%s' route %s error %d:%s\n", ifname, route, errno, strerror(errno));
        close(sock);
        return -1;
    }

    LogInfo("'%s' del route %s done\n", ifname, route);
    close(sock);
    return 0;
}

static int ss_if_find_route(const char *ifname, char *route, char *genmask)
{
    const char *ROUTE_PATH = "/proc/net/route";
    FILE *fp;
    char iface[64] = {0};
    unsigned long dst, gw, mask;
    int flags, ref, use, metric, mtu, win, irtt;
    int got_route = 0;
    struct in_addr in;

    /*
    ** EX. ip route add 192.168.0.0/24 dev eth0
    ** cat /proc/net/route
    ** Iface   Destination     Gateway         Flags   RefCnt  Use     Metric  Mask            MTU     Window  IRTT
    ** eth0    0000A8C0        00000000        0001    0       0       0       00FFFFFF        0       0       0
    */

    fp = fopen(ROUTE_PATH, "r");
    if (!fp)
    {
        LogInfo("fopen '%s' fail error %d:%s\n", ROUTE_PATH, errno, strerror(errno));
        return -1;
    }

    // skip the 1st line
    if (fscanf(fp, "%*[^\n]\n") < 0)
    {
        /*
        ** Empty line, read eror, or EOF. Yes, if routing table
        ** is completely empty, /proc/net/route has no header.
        */
        fclose(fp);
        return -1;
    }

    do
    {
        fscanf(fp, "%63s%lx%lx%x%d%d%d%lx%d%d%d\n",
               iface, &dst, &gw, &flags, &ref, &use, &metric, &mask, &mtu, &win, &irtt);
        if (strcmp(ifname, iface) == 0)
        {
            got_route = 1;
        }
    } while (!got_route && !feof(fp));

    fclose(fp);

    if (!got_route)
    {
        LogInfo("'%s' has no route set\n", iface);
        return -1;
    }

    memcpy(&in, &dst, 4);
    strcpy(route, inet_ntoa(in));
    memcpy(&in, &mask, 4);
    strcpy(genmask, inet_ntoa(in));
    LogInfo("'%s' find route info DST %s MASK %s FLAGS %d\n", iface, route, genmask, flags);

    return 0;
}

static int ss_if_config(char *ifname, char *ip, char *route)
{
    char route_old[16] = {0};
    char mask_old[16] = {0};

    if (ss_if_set_up(ifname) < 0)
    {
        return -1;
    }

    if (ss_if_set_ip(ifname, ip) < 0)
    {
        return -1;
    }

    if (ss_if_find_route(ifname, route_old, mask_old) == 0)
    {
        ss_if_del_route(ifname, route_old, mask_old);
    }

    if (ss_if_add_route(ifname, route))
    {
        return -1;
    }

    return 0;
}

static int ss_tcp_create_socket(char *ifname, char *server_ip, int server_port, int timeout_s)
{
    int sock = -1;
    struct ifreq ifr;
    struct sockaddr_in ser;
    char recv_buf[64] = {0};
    int wait_interval_ms = 500;
    int i = 0;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        LogInfo("socket create error %d:%s\n", errno, strerror(errno));
        return -1;
    }

    // bind to net device
    if ((ifname != NULL) && strlen(ifname) > 0)
    {
        memset(&ifr, 0, sizeof(ifr));
        strncpy(ifr.ifr_name, ifname, strlen(ifname));
        if (setsockopt(sock, SOL_SOCKET, SO_BINDTODEVICE, (char *)&ifr, sizeof(ifr)) < 0)
            LogInfo("Bind to device '%s' fail(%d:%s), but continue.\n", ifname, errno, strerror(errno));
        else
            LogInfo("Bind to device '%s' done\n", ifname);
    }

    // connect to server
    memset(&ser, 0, sizeof(ser));
    ser.sin_family = AF_INET;
    ser.sin_port = htons(server_port);
    ser.sin_addr.s_addr = inet_addr(server_ip);

    LogInfo("Actively connect to the server...\n");
    if (connect(sock, (struct sockaddr *)&ser, sizeof(ser)) < 0)
    {
        for (i = 0; i < timeout_s * 1000 / wait_interval_ms; i++)
        {
            if (connect(sock, (struct sockaddr *)&ser, sizeof(ser)) == 0)
            {
                goto ss_connect_succ;
            }

            usleep(timeout_s * 1000);
        }

        LogInfo("TCP connection fail timeout_s=%ds error %d:%s\n", timeout_s, errno, strerror(errno));
        close(sock);
        return -1;
    }

ss_connect_succ:
    LogInfo("TCP connection established %s:%d\n", server_ip, server_port);

    // fastboot handshake
    if (ss_tcp_write(sock, SS_FASTBOOT_HANDSHAKE, strlen(SS_FASTBOOT_HANDSHAKE)) <= 0)
    {
        LogInfo("send fastboot handshake fail %d:%s\n", errno, strerror(errno));
        close(sock);
        return -1;
    }

    if ((ss_tcp_read(sock, recv_buf, sizeof(recv_buf)) > 0) && (strcmp(recv_buf, SS_FASTBOOT_HANDSHAKE) == 0))
    {
        LogInfo("handshake done!\n");
    }
    else
    {
        LogInfo("recv fastboot handshake fail recv_buf=[%s] %d:%s\n", recv_buf, errno, strerror(errno));
        close(sock);
        return -1;
    }

    return sock;
}

int samsung_tcp_write(const void *handle, void *pbuf, int max_size, int min_size, int timeout_msec)
{
    int sock = ss_find_sock_by_handle((void *)handle);

    min_size = MAX(min_size, 1);
    if (timeout_msec == 0)
        timeout_msec = 3000;

    return ss_tcp_write(sock, pbuf, max_size);
}

int samsung_tcp_read(const void *handle, void *pbuf, int max_size, int min_size, int timeout_msec)
{
    int sock = ss_find_sock_by_handle((void *)handle);

    min_size = MAX(min_size, 1);
    if (timeout_msec == 0)
        timeout_msec = 3000;

    return ss_tcp_read(sock, pbuf, max_size);
}

int samsung_open_tcp(fibo_usbdev_t *pdev, char *ifname, char *server_ip, int server_port, int timeout_s)
{
    int sock = -1;

    if (ss_if_config(ifname, SS_DEFAULT_IP, SS_DEFAULT_ROUTE) < 0)
    {
        LogInfo("config '%s' fail\n", ifname);
        return -1;
    }

    sock = ss_tcp_create_socket(ifname, server_ip, server_port, timeout_s);
    if (sock < 0)
    {
        LogInfo("create sock fail!\n");
        return -1;
    }

    if (ss_add_sock(sock, (void *)pdev) < 0)
    {
        LogInfo("add sock ctrl fail!\n");
        close(sock);
        return -1;
    }

    pdev->write = samsung_tcp_write;
    pdev->read = samsung_tcp_read;

    return 0;
}

int samsung_close_tcp(fibo_usbdev_t *pdev)
{
    int sock = ss_find_sock_by_handle((void *)pdev);

    ss_del_sock(sock);
    pdev->write = NULL;
    pdev->read = NULL;
    close(sock);

    return 0;
}
