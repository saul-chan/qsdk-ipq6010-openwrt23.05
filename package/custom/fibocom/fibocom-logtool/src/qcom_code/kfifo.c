#include <netinet/ip.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include "log_control.h"
#include "qlog.h"


#define CACHE_SIZE (5*1024*1024)
#define FIFO_NUM   4

struct __kfifo 
{
    int fd;
    size_t in;
    size_t out;
    size_t size;
    void *data;
};

static struct __kfifo qkfifo[FIFO_NUM] = {{-1, 0, 0, 0, NULL}, {-1, 0, 0, 0, NULL}, {-1, 0, 0, 0, NULL}, {-1, 0, 0, 0, NULL}};
static const size_t cache_step = (256*1024);

static int __kfifo_write(struct __kfifo *fifo, const void *buf, size_t size)
{
    void *data;
    size_t unused, len;
    ssize_t nbytes;
    int fd = fifo->fd;

    if (fifo->out == fifo->in)
    {
        nbytes = log_poll_write(fd, buf, size);

        if(nbytes > 0)
        {
            if((size_t)nbytes == size)
            {
                return 1;
            }
            else
            {
                buf = (char *)buf + nbytes;
                size -= nbytes;
            }
        }
        else if (errno == ECONNRESET)
        {
            LogInfo("TODO: ECONNRESET\n");
            return 0;
        }
    }

    unused = fifo->size - fifo->in;
    if (unused < size && size < (unused + fifo->out))
    {
        memmove(fifo->data, (char *)fifo->data + fifo->out, fifo->in - fifo->out);
        fifo->in -=  fifo->out;
        fifo->out = 0;    
    }

    unused = fifo->size - fifo->in;
    if(unused < size && fifo->size < CACHE_SIZE)
    {
        data = malloc(fifo->size + cache_step);

        if (data)
        {
            LogInfo("cache[fd=%d] size %zd -> %zd KB\n", fd, fifo->size/1024, (fifo->size + cache_step)/1024);
            if(fifo->data)
            {
                len = fifo->in - fifo->out;
                if (len)
                    memcpy(data, (char *)fifo->data + fifo->out, len);
                free(fifo->data);
            }

            fifo->in -=  fifo->out;
            fifo->out = 0;
            fifo->size += cache_step;
            fifo->data = data;
        }
    }

    unused = fifo->size - fifo->in;
    if (unused < size)
    {
        static size_t drop = 0;
        static unsigned slient_msec = 0;
        unsigned now = qlog_msecs();

        drop += size;
        if ((slient_msec+5000) < now)
        {
            LogInfo("cache[fd=%d] full, total drop %zd\n", fd, drop);
            slient_msec = now;
        }
    }
    else
    {
        memcpy((char *)fifo->data + fifo->in, buf, size);
        fifo->in += size;
    }

    len = fifo->in - fifo->out;
    if (len)
    {
        nbytes = log_poll_write(fd, (char *)fifo->data + fifo->out, len);

        if (nbytes > 0)
        {
            fifo->out += nbytes;

            if (fifo->out == fifo->in)
            {
                fifo->in = 0;
                fifo->out = 0;
            }
        }
        else if(errno == ECONNRESET)
        {
            LogInfo("TODO: ECONNRESET\n");
            return 0;
        }
    }

    return 1;
}

int qkfifo_alloc(int fd)
{
    int idx = 0;
    int flags;

    if (fd == -1)
        return fd;

    for (idx = 0; idx < FIFO_NUM; idx++) {
        if (qkfifo[idx].fd == -1)
            break;
    }

    if (idx == FIFO_NUM) {
        LogInfo("No Free FIFO for fd = %d\n", fd);
        return -1;
    }

    qkfifo[idx].fd = fd;
    qkfifo[idx].in = qkfifo[idx].out = 0;

    flags = fcntl(fd, F_GETFL);
    if (flags != -1)
        fcntl(fd, F_SETFL, flags | O_NONBLOCK);

    LogInfo("%s [%d] = %d\n", __func__, idx, fd);
    return idx;
}

size_t qkfifo_write(int idx, const void *buf, size_t size)
{
    if (idx < 0 ||  idx >= FIFO_NUM)
        return 0;
    return __kfifo_write(&qkfifo[idx], buf, size) ? size : 0;
}

void qkfifo_free(int idx)
{
    if (idx < 0 || idx >= FIFO_NUM)
        return;
    LogInfo("%s [%d] = %d\n", __func__, idx, qkfifo[idx].fd);
    qkfifo[idx].fd = -1;
    qkfifo[idx].in = qkfifo[idx].out = 0;
}

int qkfifo_idx(int fd)
{
    int idx = 0;

    if (fd == -1)
        return fd;

    for (idx = 0; idx < FIFO_NUM; idx++) {
        if (qkfifo[idx].fd == fd)
            break;
    }

    if (idx == FIFO_NUM) {
        return -1;
    }

    return idx;
}
