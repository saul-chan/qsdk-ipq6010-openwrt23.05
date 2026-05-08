#include <stdio.h>
#include <fcntl.h>
#include <linux/serial.h>
#include <asm-generic/ioctl.h>
#include <asm-generic/ioctls.h>
#include <sys/ioctl.h>
#include "linux_comm.h"
#include "utils.h"
#include <unistd.h>
#define termios asmtermios
#include <asm-generic/termbits.h>
#undef  termios
#include <termios.h>

int32_t g_handle = -1;
int32_t open_port(char *devPath, uint32_t baud, uint32_t timeout_ms)
{
    int32_t fd = -1;
    
    fd = open(devPath, O_RDWR);
    if (fd == -1)
    {
        DLOG("OpenPort fail");
        return -1;
    }
    if (0 != init(fd))
    {
        DLOG("Initial set fail");
        return -1;
    }
    m_sleep(10);
    if (0 != set_baud(fd, baud))
    {
        DLOG("Set baud fail");
        return -1;
    }
    m_sleep(10);
    if (0 != set_buffer(fd))
    {
        DLOG("SetBuffer fail");
        /*some linux kernel will set buffer fail, but it does not affect the actual upgrade results*/
        //return -1;
    } 
    m_sleep(10);
    if (0 != set_timeout(fd, timeout_ms))
    {
        DLOG("SetTimeout fail");
        return -1;
    }
    m_sleep(10);
    g_handle = fd;

    return 0;
}

int32_t close_port()
{
    int32_t ret;

    if (g_handle == -1)
        return 0;
    ret = close(g_handle);
    g_handle = -1;

    return ret;
}

int32_t write_data(uint8_t *data, uint64_t size)
{
    ssize_t dwWritten = 0;
    size_t remain;
    uint8_t *_p = NULL;
    uint32_t i;
    uint32_t wLen;

    if (g_handle == -1)
    {
        DLOG("Write, invalid fd");
        return -1;
    }
    if ((NULL == data)  || (size == 0))
    {
        DLOG("Data point is null or size is 0");
        return -1;
    }

    _p = data;
    remain = size;
    while (remain > 0)
    {
        if (remain > MAX_TRANSPORT_SIZE)
            wLen = MAX_TRANSPORT_SIZE;
        else
            wLen = remain;
        dwWritten = write(g_handle, _p, (size_t)wLen);
        if (dwWritten == -1)
        {
            return -1;
        }
        remain -= dwWritten;
        _p += dwWritten;
        DLOG("write");
        for (i=0; i<dwWritten; i++)
        {
            BINDLOG("%02x ", *(_p-dwWritten+i));
        }
        BINDLOG("\n");
    }

    return 0;
}

int32_t read_data(uint8_t *buffer, uint64_t size)
{
    ssize_t read_len=0;
    uint64_t total=0;
    int32_t max_count = MAX_READ_COUNT;
    uint32_t i = 0;
    if (g_handle == -1)
    {
        DLOG("Write, invalid fd");
        return -1;
    }
    if ((buffer == NULL)  || (size == 0))
    {
        DLOG("Buffer point is null or size is 0");
        return -1;
    }

    while (max_count-- > 0)
    {
        read_len = read(g_handle, buffer+total, size-total);
        if (read_len == -1)
        {
            DLOG("read_data len -1\r\n");
            return -1;
        }
        total += read_len;
        if (total >= size)
        {
            DLOG("read");
            for (i=0; i<size; i++)
            {
                BINDLOG("%02x ", *(buffer+i));
            }
            BINDLOG("\n");
            return 0;
        }
	usleep(50000);
    }
    return -1;
}

int32_t clear_read_buffer()
{
    if (g_handle == -1)
    {
        return -1;
    }
    return ioctl(g_handle, TCFLSH, 2);
}

int32_t init(int32_t fd)
{
    struct termios opt;
    tcgetattr(fd, &opt);

    /* c_lflag 本地模式 */
    opt.c_lflag &= ~(ICANON | ECHO | ECHOE |  ISIG); //ICANON启用标准模式;ECHO回显输入字符;ECHOE如果同时设置了 ICANON，字符 ERASE 擦除前一个输入字符，WERASE 擦除前一个词;ISIG当接受到字符 INTR, QUIT, SUSP, 或 DSUSP 时，产生相应的信号

    /* c_oflag 输出模式 */
    opt.c_oflag &= ~ OPOST;             //OPOST启用具体实现自行定义的输出处理
    opt.c_oflag &= ~(ONLCR | OCRNL);    //ONLCR将输出中的新行符映射为回车-换行,OCRNL将输出中的回车映射为新行符

    /* c_iflag 输入模式 */
    opt.c_iflag &= ~ INPCK;           //不启用输入奇偶检测
    opt.c_iflag &= ~(ICRNL |  INLCR);          //ICRNL将输入中的回车翻译为新行 (除非设置了 IGNCR),INLCR将输入中的 NL 翻译为 CR
    opt.c_iflag &= ~(IXON | IXOFF | IXANY);    //IXON启用输出的 XON/XOFF流控制,IXOFF启用输入的 XON/XOFF流控制,IXANY(不属于 POSIX.1；XSI) 允许任何字符来重新开始输出

    /* c_cflag 控制模式 */
    opt.c_cflag &= ~ CSIZE;     //字符长度掩码,取值为 CS5, CS6, CS7, 或 CS8,加~就是无
    opt.c_cflag |= (CLOCAL |  CREAD); //CLOCAL忽略 modem 控制线,CREAD打开接受者
    opt.c_cflag |=  CS8;        //数据宽度是8bit
    opt.c_cflag &= ~ CSTOPB;    //CSTOPB设置两个停止位，而不是一个,加~就是设置一个停止位
    opt.c_cflag &= ~ PARENB;    //PARENB允许输出产生奇偶信息以及输入的奇偶校验,加~就是无校验

    tcflush(fd, TCIOFLUSH);         //刷串口清缓存
    
    return tcsetattr(fd, TCSANOW, &opt);   //设置终端控制属性,TCSANOW：不等数据传输完毕就立即改变属性
}

int32_t set_baud(int32_t fd, uint32_t baud)
{
    int32_t ret;
    struct termios opt;
    uint32_t baudVal;

    baudVal= get_baudVal(baud);
    if (baudVal == 0)
    {
        ret = set_customer_def_baud(fd, baud);
    }
    else
    {
        ret = tcgetattr(fd, &opt);
        ret |= cfsetispeed(&opt, baudVal);
        ret |= cfsetospeed(&opt, baudVal);
        ret |= tcflush (fd, TCIOFLUSH);
        ret |= tcsetattr(fd, TCSANOW, &opt);
    }

    return ret;
}

uint32_t set_customer_def_baud(int32_t fd, uint32_t baud)
{
    struct termios2 opt2 = {0};
    ioctl(fd, TCGETS2, &opt2);
    opt2.c_cflag = BOTHER;
    opt2.c_ispeed = baud;
    opt2.c_ospeed = baud;

    return ioctl(fd, TCSETS2, &opt2);
}

int32_t set_buffer(int32_t fd)
{
    struct serial_struct serial;
    if (ioctl(fd, TIOCGSERIAL, &serial) !=0)
        return -1;
    serial.xmit_fifo_size = MAX_BUFFER_SIZE;
    if (ioctl(fd, TIOCSSERIAL, &serial) !=0)
        return -1;
    return 0;
}

int32_t set_timeout(int32_t fd, uint32_t timeout_ms)
{
    struct termios opt;
    tcgetattr(fd, &opt);

    /*
    VTIME: max wait time for a single symbol
    vMIN: minimum length of the symbol read
    VTIME和VMIN需配合使用，它们的组合关系如下：
    1、VTIME=0，VMIN=0：此时即使读取不到任何数据，函数read也会返回，返回值是0。
    2、VTIME=0，VMIN>0：read调用一直阻塞，直到读到VMIN个字符后立即返回。
    3、VTIME>0，VMIN=0：read调用读到数据则立即返回，否则将为每个字符最多等待 VTIME*100ms 时间。
    4、VTIME>0，VMIN>0：read调用将保持阻塞直到读取到第一个字符，读到了第一个字符之后开始计时，此后若时间到了 VTIME*100ms 或者时间未到但已读够了VMIN个字符则会返回。
    */
    opt.c_cc[VTIME]     	 = timeout_ms/100;
    opt.c_cc[VMIN]         	 = 0;


    tcflush (fd, TCIOFLUSH);
    return tcsetattr(fd, TCSANOW, &opt);
}

uint32_t get_baudVal(uint32_t baudrate)
{
    if (baudrate == 57600)
        return B57600;
    if (baudrate == 115200)
        return B115200;
    if (baudrate == 230400)
        return B230400;
    if (baudrate == 460800)
        return B460800;
    if (baudrate == 500000)
        return B500000;
    if (baudrate == 576000)
        return B576000;
    if (baudrate == 921600)
        return B921600;
    if (baudrate == 1000000)
        return B1000000;
    if (baudrate == 1152000)
        return B1152000;
    if (baudrate == 1500000)
        return B1500000;
    if (baudrate == 2000000)
        return B2000000;
    if (baudrate == 2500000)
        return B2500000;
    if (baudrate == 3000000)
        return B3000000;
    if (baudrate == 3500000)
        return B3500000;
    if (baudrate == 4000000)
        return B4000000;
    
    return 0;
}
