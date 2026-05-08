#include "pcie_download.h"


static int fibo_pcie_write(const void *handle, void *pbuf, int max_size, int min_size, int timeout_msec)
{
    int cur_size = 0, ret = 0;
    fibo_usbdev_t *pdev = (fibo_usbdev_t *)handle;

    // LogInfo("start\n");
    if (pdev->pcie_fd < 0) {
        LogInfo("dev port is not opened\n");
        return -1;
    }

    min_size = MAX(min_size, 1);
    while (cur_size < max_size)
    {
        if (poll_wait(pdev->pcie_fd, POLLOUT, timeout_msec)) {
            LogInfo("poll_wait timeout\n");
            break;
        }
        ret = write(pdev->pcie_fd, (uint8_t *)pbuf+cur_size, max_size-cur_size);

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

static int fibo_pcie_read(const void *handle, void *pbuf, int max_size, int min_size, int timeout_msec)
{
    fibo_usbdev_t *pdev = (fibo_usbdev_t *)handle;
    int cur_size = 0, ret = 0;

    // LogInfo("start\n");
    if (pdev->pcie_fd < 0) {
        LogInfo("dev port is not opened\n");
        return -1;
    }

    min_size = MAX(min_size, 1);
    while (cur_size < min_size)
    {
        if (poll_wait(pdev->pcie_fd, POLLIN, timeout_msec)) {
            LogInfo("poll_wait timeout\n");
            break;
        }
        ret = read(pdev->pcie_fd, (uint8_t *)pbuf+cur_size, max_size-cur_size);
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

fibo_usbdev_t s_pcie_dev =
{
    .ModuleName = "",
    .idVendor   = 0,
    .idProduct  = 0,
    .used_ifnum  = 0,
    .ttyfd = -1,
    .usbdev = -1,
    .tcp_client_fd = -1,
    .pcie_fd = -1,

    .write = fibo_pcie_write,
    .read  = fibo_pcie_read,
};

static int switch_to_edl_pcie_mode(char *devname)
{
    int ret = -1;
    char pcie_edl_cmd[] = "AT+SYSCMD=\"sys_reboot pcie_edl\"\r\n";
    char cmdbuf[MAX_PATH_LEN] = {0};
    int edl_retry = 30; //SDX55 require long time by now 20190412
    fibo_usbdev_t pcie_dev;

    LogInfo("start\n");
    memset(&pcie_dev, 0, sizeof(pcie_dev));
    pcie_dev.pcie_fd = open(devname, O_RDWR | O_NOCTTY);
    if (pcie_dev.pcie_fd < 0) {
        LogInfo("open %s failed\n", devname);
        return -1;
    }

    while (access(devname, R_OK) == 0 && edl_retry-- > 0)
    {
        int cur_ret = -1;
        do {
            cur_ret = fibo_pcie_read(&pcie_dev, cmdbuf , sizeof(cmdbuf), 0, 3000);
        } while (cur_ret > 0);

        LogInfo("switch to 'download mode'\n");
        cur_ret = fibo_pcie_write(&pcie_dev, pcie_edl_cmd, strlen(pcie_edl_cmd), strlen(pcie_edl_cmd), 3000);
        if (cur_ret < 0) {
            goto END;
        }

        do {
            cur_ret = fibo_pcie_read(&pcie_dev, cmdbuf, sizeof(cmdbuf), 0, 3000);
            if (cur_ret == (int)strlen(pcie_edl_cmd) && memcmp(cmdbuf, pcie_edl_cmd, strlen(pcie_edl_cmd)) == 0)
            {
                LogInfo("successful, wait module reboot\n");
                ret = 0;
                goto END;
            }
        } while (cur_ret > 0);
        sleep(1);
    }
END:
    close(pcie_dev.pcie_fd);
    pcie_dev.pcie_fd = -1;

    return ret;
}

fibo_usbdev_t *pcie_open_dl(char *firmware_name, char *pcie_portname)
{
    int bhifd = -1;
    FILE *fp = NULL;
    BHI_INFO_TYPE bhi_info;
    int filesize = 0;
    uint8_t *filebuf = NULL;
    char devname[MAX_PATH_LEN] = {0};
    char prog_firehose_path[MAX_PATH_LEN] = {0};
    fibo_usbdev_t *pdev = NULL;
    int bhi = COMMON_MODE;

    if (pcie_portname == NULL || pcie_portname[0] == 0) {
        LogInfo("pcie_portname is error.");
        goto END;
    }

    LogInfo("pcie_portname = %s\n",pcie_portname);
    sprintf(devname,"%s_DUN", pcie_portname);
    LogInfo("DUN devname: %s\n", devname);
    if (access(devname, R_OK)) {
        LogInfo("Fibocom module pcie port not found.\n");
        
        sprintf(devname,"%s_BHI", pcie_portname);
        LogInfo("BHI devname: %s\n", devname);
        if (access(devname, R_OK)) {
            LogInfo("Fibocom module pcie BHI port not found.\n");
            goto END;
        }
        LogInfo("PCIe Already in EDL mode.\n");
        bhi = EDL_MODE;
    }

    LogInfo("find Fibocom module pcie port.\n");
    if(bhi == COMMON_MODE){
        switch_to_edl_pcie_mode(devname);
        sprintf(devname,"%s_BHI", pcie_portname);
    }
    LogInfo("BHI devname: %s\n", devname);
    bhifd = open(devname, O_RDWR | O_NOCTTY);
    if (bhifd < 0)
    {
        LogInfo("fail to open %s, errno: %d(%s)\n", devname, errno, strerror(errno));
        goto END;
    }

    if (ioctl(bhifd, IOCTL_BHI_GETDEVINFO, &bhi_info))
    {
        LogInfo("fail to ioctl IOCTL_BHI_GETDEVINFO, errno: %d(%s)\n", errno, strerror(errno));
        goto END;
    }

    LogInfo("bhi_ee: %d\n", bhi_info.bhi_ee);
    if (bhi_info.bhi_ee != MHI_EE_EDL)
    {
        LogInfo("bhi_ee is not MHI_EE_EDL\n");
        goto END;
    }

    snprintf(prog_firehose_path, sizeof(prog_firehose_path), "%s/prog_firehose_sdx55.mbn", firmware_name);
    if (access(prog_firehose_path, R_OK)) {
        snprintf(prog_firehose_path, sizeof(prog_firehose_path), "%s/prog_firehose_lite.elf", firmware_name);
    }

    if (access(prog_firehose_path, R_OK)) {
        goto END;
    }

    fp = fopen(prog_firehose_path, "rb");
    if (fp == NULL)
    {
        LogInfo("fail to fopen %s, errno: %d(%s)\n", prog_firehose_path, errno, strerror(errno));
        goto END;
    }

    fseek(fp, 0, SEEK_END);
    filesize = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    filebuf = malloc(sizeof(filesize)+filesize);
    if (filebuf == NULL) {
        LogInfo("fail to allocate filebuf\n");
        goto END;
    }
    memcpy(filebuf, &filesize, sizeof(filesize));
    fread(filebuf + sizeof(filesize), 1, filesize, fp);
    fclose(fp);
    fp = NULL;

    if (ioctl(bhifd, IOCTL_BHI_WRITEIMAGE, filebuf))
    {
        LogInfo("fail to ioctl IOCTL_BHI_GETDEVINFO, errno: %d(%s)\n", errno, strerror(errno));
        goto END;
    }
    close(bhifd);
    bhifd = -1;

    sleep(1);
    sprintf(devname,"%s_EDL", pcie_portname);
    LogInfo("EDL port name: %s\n", devname);
    s_pcie_dev.pcie_fd = open(devname, O_RDWR | O_NOCTTY);
    if (s_pcie_dev.pcie_fd < 0)
    {
        LogInfo("fail to access %s, errno: %d(%s)\n", "/dev/mhi_EDL", errno, strerror(errno));
        goto END;
    }
    pdev = &s_pcie_dev;
END:
    if (bhifd >= 0) {
        close(bhifd);
        bhifd = -1;
    }

    if (fp != NULL) {
        fclose(fp);
        fp = NULL;
    }

    if (filebuf != NULL) {
        free(filebuf);
        filebuf = NULL;
    }
    return pdev;
}
