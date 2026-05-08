#include <ctype.h>      /* for isdigit() */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>


#include <errno.h>
#include <time.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <signal.h>
#include <poll.h>
#include <assert.h>

#include <sys/types.h>

#include <network_api.h>

/*for print log*/
//#undef printf(
//#define printf(    dlt_vlog(LOG_INFO,


/*for quit case, like ctrl+c*/
extern int g_quit_flag;



#define CACHE_SIZE (5*1024*1024)
static const size_t cache_step = (256*1024);
int block_size = 16384;


const char *g_tftp_server_ip = NULL;
const char *g_ftp_server_ip = NULL;
const char *g_ftp_server_usr = NULL;
const char *g_ftp_server_pass = NULL;

char g_str_sub_ss_log_dir[128];
volatile int g_network_transfer_status = 0;


ssize_t slog_poll_write(int fd, const void *buf, size_t size, unsigned timeout_msec);

int is_Quiting() {
    int result = 0;

    if ( g_quit_flag != 0) {
        result = 1;
    }

    return result;
}

unsigned slog_msecs(void)
{
    static unsigned start = 0;
    unsigned now;
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    now = (unsigned)ts.tv_sec*1000 + (unsigned)(ts.tv_nsec / 1000000);
    if (start == 0)
        start = now;
    return now - start;
}


#define dbg(fmt, arg...)                                            \
    do                                                              \
    {                                                               \
        unsigned msec = slog_msecs();                               \
        printf("[%03d.%03d]" fmt, msec / 1000, msec % 1000, ##arg); \
    } while (0)

#define errno_dbg(fmt, ...)                                                                                             \
    do                                                                                                                  \
    {                                                                                                                   \
        dbg(fmt ", errno: %d (%s) at File: %s, Line: %d\n", ##__VA_ARGS__, errno, strerror(errno), __func__, __LINE__); \
    } while (0)



int is_tftp(void)
{
    return (g_tftp_server_ip != NULL);
}

int is_ftp(void)
{
    return (g_ftp_server_ip != NULL 
            && (g_ftp_server_usr != NULL)
            && (g_ftp_server_pass != NULL));
}



#ifdef CACHE_SIZE
#define min(x,y) (((x) < (y)) ? (x) : (y))
struct __kfifo {
    int fd;
    size_t in;
    size_t out;
    size_t size;
    void *data;
};

static int __kfifo_write(struct __kfifo *fifo, const void *buf, size_t size)
{
    void *data;
    size_t unused, len;
    ssize_t nbytes;
    int fd = fifo->fd;

    //LogInfo("size ===%d ,fifo->size====%d ,fifo->in====%d ,fifo->out===%d\n",size,fifo->size,fifo->in,fifo->out);
    if (fifo->out == fifo->in)
    {
        nbytes = slog_poll_write(fd, buf, size,0);

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
#if 0
                while(1)
                {
                    buf += nbytes;
                    size -= nbytes;
                    nbytes = ulog_poll_write(fd, buf, size,0);
                    //LogInfo("box =====nbytes == %d\n",nbytes);
                    if(nbytes == size)
                    {
                        //LogInfo("box =====nbytes == size\n");
                        return 1;
                    }
                }
#endif
            }
        }
        else if (errno == ECONNRESET)
        {
            printf("TODO: ECONNRESET\n");
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
            printf("cache[fd=%d] size %zd -> %zd KB\n", fd, fifo->size/1024, (fifo->size + cache_step)/1024);
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
        unsigned now = slog_msecs();

        drop += size;
        if ((slient_msec+5000) < now)
        {
            printf("cache[fd=%d] full, total drop %zd\n", fd, drop);
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
        nbytes = slog_poll_write(fd, (char *)fifo->data + fifo->out, len, 0);

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
            printf("TODO: ECONNRESET\n");
            return 0;
        }
    }

    return 1;
}

#define FIFO_NUM 4
static struct __kfifo kfifo[FIFO_NUM] = {{-1, 0, 0, 0, NULL}, {-1, 0, 0, 0, NULL}, {-1, 0, 0, 0, NULL}, {-1, 0, 0, 0, NULL}};

int kfifo_alloc(int fd) {
    int idx = 0;
    int flags;

    if (fd == -1)
        return fd;

    for (idx = 0; idx < FIFO_NUM; idx++) {
        if (kfifo[idx].fd == -1)
            break;
    }

    if (idx == FIFO_NUM) {
        printf("No Free FIFO for fd = %d\n", fd);
        return -1;
    }

    kfifo[idx].fd = fd;
    kfifo[idx].in = kfifo[idx].out = 0;

    flags = fcntl(fd, F_GETFL);
    if (flags != -1)
        fcntl(fd, F_SETFL, flags | O_NONBLOCK);

    printf("%s [%d] = %d\n", __func__, idx, fd);
    return idx;
}

size_t kfifo_write(int idx, const void *buf, size_t size) {
    if (idx < 0 ||  idx >= FIFO_NUM)
        return 0;
    return __kfifo_write(&kfifo[idx], buf, size) ? size : 0;
}

void kfifo_free(int idx) {
    if (idx < 0 || idx >= FIFO_NUM)
        return;
    printf("%s [%d] = %d\n", __func__, idx, kfifo[idx].fd);
    kfifo[idx].fd = -1;
    kfifo[idx].in = kfifo[idx].out = 0;
}

int kfifo_idx(int fd) {
    int idx = 0;

    if (fd == -1)
        return fd;

    for (idx = 0; idx < FIFO_NUM; idx++) {
        if (kfifo[idx].fd == fd)
            break;
    }

    if (idx == FIFO_NUM) {
        return -1;
    }

    return idx;
}
#endif


size_t slog_logfile_save(int logfd, const void *buf, size_t size)
{
    int idx = kfifo_idx(logfd); //dbg("slog_logfile_save %d size=%zu, idx=%d\n", logfd, size, idx);
    if (idx != -1 )
    {
        return kfifo_write(idx, buf, size);
    }

    return slog_poll_write(logfd, buf, size, 1000);
}

int slog_logfile_close(int logfd)
{
    kfifo_free(kfifo_idx(logfd));
    //kfifo_free(kfifo_idx(second_logfile));
    //safe_close_fd(second_logfile);
    return close(logfd);
}

ssize_t slog_poll_write(int fd, const void *buf, size_t size, unsigned timeout_msec)
{
    size_t wc = 0;
    ssize_t nbytes;

    //if (!qlog_read_com_data && fd == ulog_args->fds.dm_sockets[0])
    //{
    //    return ql_usbfs_write(ulog_args->fds.dm_usbfd, ulog_args->ql_dev->dm_intf.ep_out, buf, size);;
    //}
    //printf("slog_poll_write to write fd = %d, size=%zu\n", fd, size);

    nbytes = write(fd, (char *)buf+wc, size-wc);

    if (nbytes <= 0)
    {
        if (errno != EAGAIN)
        {
            printf("Fail to write fd = %d, errno : %d (%s)\n", fd, errno, strerror(errno));
            goto out;
        }
        else
        {
            nbytes = 0;
        }
    }

    wc += nbytes;

    if (timeout_msec == 0)
        return (wc);

    while (wc < size)
    {
        int ret;
        struct pollfd pollfds[] = {{fd, POLLOUT, 0}};

        do
        {
            ret = poll(pollfds, 1, timeout_msec);
        }  while (ret == -1 && errno == EINTR && (!is_Quiting()));

        if(ret <= 0)
        {
            printf("Fail to poll fd = %d, errno : %d (%s)\n", fd, errno, strerror(errno));
            break;
        }

        if(pollfds[0].revents & (POLLERR | POLLHUP | POLLNVAL))
        {
            printf("Fail to poll fd = %d, revents = %04x\n", fd, pollfds[0].revents);
            break;
        }

        if(pollfds[0].revents & (POLLOUT))
        {
            nbytes = write(fd, (char *)buf+wc, size-wc);

            if(nbytes <= 0)
            {
                printf("Fail to write fd = %d, errno : %d (%s)\n", fd, errno, strerror(errno));
                break;
            }
            wc += nbytes;
        }
    }

out:
    if (wc != size)
    {
        printf("%s fd=%d, size=%zd, timeout=%d, wc=%zd\n", __func__, fd, size, timeout_msec, wc);
    }

    return (wc);
}

int slog_logfile_create_fullname(int file_type, const char *fullname, long tftp_size, int is_dump)
{
    int fd = -1;
    if(!strncmp(fullname, "/dev/null", strlen("/dev/null")))
    {
        fd = open("/dev/null", O_CREAT | O_RDWR | O_TRUNC, 0444);
    }
    else if(is_tftp())
    {
        const char *filename = fullname;
        const char *p = strchr(filename, '/');
        while(p)
        {
            p++;
            filename = p;
            p = strchr(filename, '/');
        }
        fd = tftp_write_request(filename, tftp_size);
    }
    else if(is_ftp())
    {
        const char *filename = fullname;
        const char *p = strchr(filename, '/');
        while(p)
        {
            p++;
            filename = p;
            p = strchr(filename, '/');
        }
        printf("%s  filename:%s  g_ftp_server_pass:%s\n",__func__,filename, g_ftp_server_pass);
        fd = s_ftp_write_request(file_type, g_ftp_server_ip, g_ftp_server_usr, g_ftp_server_pass, filename);
        if (!is_dump)
            kfifo_alloc(fd);

        printf("ftp fd:%d\n",fd);
    }
    else
    {
        fd = open(fullname, O_CREAT | O_RDWR | O_TRUNC, 0444);
        if (!is_dump)
            kfifo_alloc(fd);
    }

    return fd;
}

/* opcodes we support */
#define TFTP_RRQ   1
#define TFTP_WRQ   2
#define TFTP_DATA  3
#define TFTP_ACK   4
#define TFTP_ERROR 5
#define TFTP_OACK  6

#define TFTP_MAX_RETRY  12
#define TFTP_TIME_RETRY 200

struct tftp_packet{
	uint16_t cmd;
	union{
		uint16_t code;
		uint16_t block;
		// For a RRQ and WRQ TFTP packet
		char filename[2];
	};
	uint8_t data[512];
};

struct tftp_data_packet {
    uint16_t cmd;
    uint16_t block;
    uint8_t data[16384];
};

// Socket fd this client use.
static struct sockaddr_in tftp_server;


#define N_DATA 1 // 4 get 10MB/s, 8 get 20MB/s
struct s_tftp_request {
    char *file;
    int sock;
    int pipe[2];
    pthread_t tid;
    int cur;
    //uint16_t block;
    struct sockaddr_in sender;
    socklen_t  addr_len;
    struct tftp_packet rx_pkt;
    struct tftp_data_packet tx_pkt[N_DATA];
};
volatile struct s_tftp_request *cur_s_req = NULL;

static int tftp_socket(const char *serv_ip) {
    int sock, reuse_addr = 1;

    sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (sock == -1) {
        errno_dbg("socket");
        return -1;
    }
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuse_addr,sizeof(reuse_addr));
    dbg("serv_ip is %s\r\n", serv_ip);
    return sock;
}


static int tftp_send_data_pkt(int sock, struct tftp_data_packet *tx_pkt, uint16_t block, int size, struct sockaddr_in *to, int is_sync) {
    struct tftp_packet rx_pkt;
    int wait_ack = 1;  
    int ret;
    struct sockaddr_in sender;
    socklen_t addr_len = sizeof(sender);
    int need_send = 1;

    //dbg("%s send = %d!\n", __func__, size);
    tx_pkt->cmd = htons(TFTP_DATA);
    tx_pkt->block = htons(block);

    while (wait_ack) {  
        struct pollfd pollfds[1] = {{sock, POLLIN, 0}};

        if (need_send) {
            ret = sendto(sock, tx_pkt, size + 4, 0, (struct sockaddr *)to, addr_len);
            if (ret == -1) {
                errno_dbg("sendto");
                return 0;
            }
        }

        if (!is_sync)
            return size;

        do  {
            ret = poll(pollfds, 1, TFTP_TIME_RETRY);
        } while ((ret < 0) && (errno == EINTR));

        if (ret <= 0) {
            dbg("wait ack 33 timeout, ret=%d, block=%d\n", ret, block);
            need_send = 1;
            if(is_Quiting()) {
                //break it
                dbg("break for quiting, ret=%d, block=%d\n", ret, block);
                goto OUT;
            }
            continue;
        }

        ret = recvfrom(sock, &rx_pkt, sizeof(struct tftp_packet), MSG_DONTWAIT, (struct sockaddr *)&sender, &addr_len);
        if(ret >= 4 && rx_pkt.cmd == htons(TFTP_ACK) && rx_pkt.block == htons(block)) {
            wait_ack = 0;  
            return size;
        }
        else if (ret >= 4 && rx_pkt.cmd == htons(TFTP_ACK)) {
            int i;

            //dbg("wait %u, but get %u\n", block, ntohs(rx_pkt.block));
            for (i = 1; i < N_DATA; i++) {
                if ((block + i) == ntohs(rx_pkt.block)) {
                    dbg("wait %u, but get %u\n", block, ntohs(rx_pkt.block));
                    wait_ack = 0; 
                    return size;
                }
            }
            need_send = 0;
        }   
        else if (ret >= 4 && rx_pkt.cmd == htons(TFTP_ERROR)) {
            dbg("Error Code, Code: %d, Message: %s, block=%u\n", ntohs(rx_pkt.code), rx_pkt.data, block);
            //assert(0);
            return -1;
        }         
        else {
            need_send = 1;
            errno_dbg("wait ack 11 timeout, ret=%d, block=%d\n", ret, block);
        }     
    }

OUT:
    //assert(0);
    return 0;
}


static void * tftp_write_thread(void *arg) {
    struct s_tftp_request *s_req = (struct s_tftp_request *)arg;
    s_req->cur = 0;
    //s_req->block = 1;
    uint32_t tx_block = 1;
    uint16_t rx_block = 0;
    uint8_t state[N_DATA] = {0};
    int w_idx = 0;
    int re_send = 0;
	int close_pipe = 1;

    //dbg("tftp_write_thread ++\n");

    tftp_ftp_set_transfer_status(1);

    while (1) {
        int ret;
        struct pollfd pollfds[2] = {{s_req->sock, POLLIN, 0}, {s_req->pipe[1], POLLIN, 0}};
        int n = 1;
        int i, r_idx;
        int busy = 0;
		int flags = 0;

        for (i = 0; i < N_DATA; i++)
            busy += state[i];

        flags = fcntl(s_req->pipe[0], F_GETFD);
        if (flags == -1)
        {
            //
            if (errno == EBADF)
            {
                /* s_req->pipe[0]*/
                /*don't change it to be -1, fd is still needed, we break it*/
                //s_req->pipe[1] = -1;
                if(busy == 0) {
                    //dbg("tftp_write_thread line %d quit!!!\n", __LINE__);
                    break;
                }
            }
        }
        if (s_req->pipe[1] == -1 && busy == 0) {
            //dbg("tftp_write_thread line %d quit!!!\n", __LINE__);
            break;
        }

        w_idx = tx_block%N_DATA;
        if (s_req->pipe[1] != -1 && state[w_idx] == 0)
            n = 2;

        //dbg("%s poll n = %d!\n", __func__, n);
        do  {
            ret = poll(pollfds, n, busy ? TFTP_TIME_RETRY : 12000); //if no data transfer in 1 second, tftp server will auto hangup connection
        } while ((ret < 0) && (errno == EINTR));

		//dbg("tftp_write_thread line=%d\n",__LINE__);

        if (ret <= 0) {
            dbg("poll=%d, rx_block=%u, re_send is %d\n", ret, rx_block, re_send);
            /*if (re_send++ > TFTP_MAX_RETRY) {
                break;
            }
            else */
            {
                //MCU -> PC: (= N_DATA) packet lost
                for (i = 0; i < N_DATA; i++) {
                    r_idx = (rx_block + 1)%N_DATA;

                    //dbg("state %d, block %u\n", state[r_idx], (rx_block + 1));
                    if (state[r_idx] == 0)
                        break;
                    //dbg("re-send %u\n", rx_block+1);
                    //assert((rx_block + 1) == ntohs(s_req->tx_pkt[r_idx].block));
                    tftp_send_data_pkt(s_req->sock, &s_req->tx_pkt[r_idx],
                        ntohs(s_req->tx_pkt[r_idx].block), block_size, &s_req->sender, 1);
                    rx_block = ntohs(s_req->tx_pkt[r_idx].block);
                    state[r_idx] = 0;
                }
            }
            continue;
        }

        if (pollfds[0].revents & POLLIN) {
            ret = recv(s_req->sock, &s_req->rx_pkt, sizeof(struct tftp_packet), MSG_DONTWAIT);
            //dbg("%s recv = %d!\n", __func__, ret);
           if(ret >= 4 && s_req->rx_pkt.cmd == htons(TFTP_ACK)) {
                if (((rx_block + 1) == htons(s_req->rx_pkt.block))) {
                    rx_block = htons(s_req->rx_pkt.block);
                    //assert(state[rx_block%N_DATA]);
                    state[rx_block%N_DATA] = 0;
                    re_send = 0;
                }
                else if (rx_block == htons(s_req->rx_pkt.block)) {
                    //MCU -> PC: (< N_DATA) packet lost
                    //dbg("double ACK %u\n", rx_block);

                    //r_idx = (rx_block + 1)%N_DATA;
                    //assert(state[r_idx]);

                    for (i = 0; i < N_DATA; i++) {
                        r_idx = (rx_block + 1)%N_DATA;

                        //dbg("state %d, block %u\n", state[r_idx], (rx_block + 1));
                        if (state[r_idx] == 0)
                            break;
                        //dbg("re-send %u\n", rx_block+1);
                        //assert((rx_block + 1) == ntohs(s_req->tx_pkt[r_idx].block));
                        tftp_send_data_pkt(s_req->sock, &s_req->tx_pkt[r_idx],
                            ntohs(s_req->tx_pkt[r_idx].block), block_size, &s_req->sender, 1);
                        rx_block = ntohs(s_req->tx_pkt[r_idx].block);
                        state[r_idx] = 0;
                        re_send = 0;
                    }
                    continue;
                }
                else {
                    //PC -> MCU: ACK packet lost
                    uint16_t num = (uint16_t)(htons(s_req->rx_pkt.block) - rx_block);
                    dbg("get %u, expect %u, num %u\n", htons(s_req->rx_pkt.block), (rx_block + 1), num);
                    if (num <= N_DATA) {
                        for (i = 0; i < num; i++) {
                            r_idx = (rx_block + 1 + i)%N_DATA;
                           // assert(state[r_idx]);
                            state[r_idx] = 0;
                        }
                        re_send = 0;
                        rx_block = htons(s_req->rx_pkt.block);
                        continue;
                    }
                    //assert(0);
                }
            }
        }

        if (n == 1)
            continue;

        if (pollfds[1].revents & POLLIN) {
            ret = read(s_req->pipe[1], s_req->tx_pkt[w_idx].data + s_req->cur, block_size - s_req->cur);
            //dbg("%s read = %d!\n", __func__, ret);
            if (ret > 0) {
                //int is_sync = (tx_block == 1) || (N_DATA == 1);
				int is_sync = 0;
                s_req->cur += ret;

                if (s_req->cur == block_size) {

                    tftp_send_data_pkt(s_req->sock, &s_req->tx_pkt[w_idx], tx_block&0xFFFF, s_req->cur, &s_req->sender, is_sync);
					if (is_sync) {
                        rx_block = tx_block&0xFFFF;
                        state[w_idx] = 0;
                    }
                    else {
                        state[w_idx] = 1;
                    }
                    tx_block++;
                    s_req->cur = 0;
					
					close_pipe = 0;
                }

                if (pollfds[1].revents & (POLLERR | POLLHUP))
                {
                    //dbg("read %d, wait for more\n",ret);
                    continue;
                }
            }
            else {
                if (ret < 0) errno_dbg("read: %d", ret);
                if (close_pipe == 1) {
				    dbg("line %d, close pipe 1 \n", __LINE__);
                    close(s_req->pipe[1]);
                    s_req->pipe[1] = -1;
				}
            }
        }

        if (pollfds[1].revents & (POLLERR | POLLHUP) && close_pipe == 1) {
            //errno_dbg("revents: %x", pollfds[0].revents);
			dbg("line %d, close pipe 1 \n", __LINE__);
            close(s_req->pipe[1]);
            s_req->pipe[1] = -1;
            //dbg("file %s close\n", s_req->file);
        }
    }

    tftp_send_data_pkt(s_req->sock, &s_req->tx_pkt[w_idx], tx_block++, s_req->cur, &s_req->sender, 1);
    //dbg("tftp_write_thread  quit, will not read from pipe1  !!!!!!!!!\n");
    //dbg("after last tftp_send_data_pkt! sock=%d\n",s_req->sock);

    close(s_req->sock);
    if (s_req->pipe[1] != -1)
        close(s_req->pipe[1]);
    //dbg("file %s exit\n", s_req->file);
    free(s_req->file);
    if (cur_s_req != s_req)
        free(s_req);

    tftp_ftp_set_transfer_status(0);
    
    return NULL;
}


int tftp_write_request(const char *filename, long tsize) {
    struct tftp_packet tx_pkt;
    int wait_ack;
    int ret, sock, size;
    char *charp;
    struct sockaddr_in sender;
    socklen_t  addr_len = sizeof(sender);

    if (cur_s_req) {
        pthread_join(cur_s_req->tid, NULL);
        free(cur_s_req);
        cur_s_req = NULL;
    }
    
    dbg("%s filename=%s, tsize=%ld\n", __func__, filename, tsize);

    tftp_ftp_set_transfer_status(1);

    sock = tftp_socket(g_tftp_server_ip);

    // new added
    tftp_server.sin_family = AF_INET;
    tftp_server.sin_port = htons(69);
    inet_pton(AF_INET, g_tftp_server_ip, &(tftp_server.sin_addr.s_addr));
    // new added end

    tx_pkt.cmd = htons(TFTP_WRQ);
    charp = (char *)tx_pkt.data; //tx_pkt->filename to avoid [-Wformat-overflow=]
    if (tsize)
        size = snprintf(charp, 512, "%.256s%c%s%c%s%c%d%c%s%c%ld%c", filename, 0, "octet", 0, "blksize", 0, block_size, 0, "tsize", 0, tsize, 0);
    else
        size = snprintf(charp, 512, "%.256s%c%s%c%s%c%d%c", filename, 0, "octet", 0, "blksize", 0, block_size, 0);
    memmove(&tx_pkt.cmd + 1, charp, size);

    for(wait_ack = 0; wait_ack < TFTP_MAX_RETRY; wait_ack++) {
        struct pollfd pollfds[1] = {{sock, POLLIN, 0}};

        ret = sendto(sock, &tx_pkt, size + 2, 0, (struct sockaddr*)&tftp_server, sizeof(tftp_server));
        if (ret == -1) {
            errno_dbg("sendto");

            ret = close(sock);
            if (ret != 0) {
                errno_dbg("close sock failed");
            }

            return 0;
        }

        do  {
            ret = poll(pollfds, 1, TFTP_TIME_RETRY);
        } while ((ret < 0) && (errno == EINTR));

        if (ret <= 0) {
            errno_dbg("wait ack 22 timeout, ret=%d", ret);
            continue;
        }

        ret = recvfrom(sock, &tx_pkt, sizeof(struct tftp_packet), MSG_DONTWAIT, (struct sockaddr *)&sender, &addr_len);
        if (ret >= 4 && tx_pkt.cmd == htons(TFTP_OACK)) {
            struct s_tftp_request *s_req = (struct s_tftp_request *)malloc(sizeof(struct s_tftp_request));
            pthread_attr_t attr;

            s_req->file = strdup(filename);
            s_req->sock = sock;
            s_req->sender = sender;
            s_req->addr_len = addr_len;
            if (socketpair(AF_LOCAL, SOCK_STREAM, 0, s_req->pipe)) errno_dbg("socketpair");
            
            pthread_attr_init (&attr);
            pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
            cur_s_req = s_req;
            //dbg("tftp_write_request before tftp_write_thread\n");
            if (pthread_create(&s_req->tid, cur_s_req ? NULL : &attr, tftp_write_thread, s_req)) errno_dbg("pthread_create");
            pthread_attr_destroy(&attr);
            return s_req->pipe[0];
        }
        else {
             errno_dbg("wait ack timeout");
        }
    }

    ret = close(sock);
    if (ret != 0) {
        errno_dbg("close sock failed");
    }

    tftp_ftp_set_transfer_status(0);

    return -1;
}

int tftp_ftp_set_transfer_status(int status) {
    g_network_transfer_status = status;
    return 0;
}

int is_transfer_ongoing() {
    return g_network_transfer_status;
}


/*ftp api*/
#define FTP_MAX_RETRY  5

struct s_ftp_tag {
    int server_fd;
    char login[4];
};

struct s_ftp_tag_combination {
    struct s_ftp_tag q_ftp_t[3];
};

struct s_ftp_tag_combination s_ftp_tag_comb;

static char buf[1024];
ssize_t s_ret_ftp_write = 0;
extern const char *g_ftp_server_ip;

static int connet_tcp_server(const char *server, int port)
{
    int ret = -1;
    int sockfd = -1;
    struct sockaddr_in ser;

    sockfd = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
    if (sockfd <0)
    {
        dbg("connet_tcp_server : socket : error\n");
        return -1;
    }

    memset(&ser,0,sizeof(ser));
    ser.sin_family = AF_INET;
    ser.sin_port = htons(port);
    ser.sin_addr.s_addr = inet_addr(server);

    dbg("connect to the server %s ...%d\n", server, port);
    ret = connect(sockfd,(struct sockaddr *)&ser,sizeof(ser));
    if (ret) {
        close(sockfd);
        sockfd = -1;
    }
    dbg("connect %s\n", sockfd == -1 ? "fail" : "successful");

    return sockfd;
}

void s_ftp_die(const char *msg)
{
    char *cp = buf; /* buf holds peer's response */

    /* Guard against garbage from remote server */
    while (*cp >= ' ' && *cp < '\x7f')
        cp++;
    *cp = '\0';
    dbg("unexpected server response%s%s: %s\n",(msg ? " to " : ""), (msg ? msg : ""), buf);
    //exit(-1); //debugging
}

int s_ftpcmd(const char *s1, const char *s2, int server_fd)
{
    unsigned n;

    dbg("cmd > %s %s\n", s1, s2);

    if (s1) {
        snprintf(buf, sizeof(buf), (s2 ? "%s %s\r\n" : "%s\r\n"),s1, s2);
        //snprintf(buf, sizeof(buf), (s2 ? "%s %s\r\n" : "%s %s\r\n"+3),s1, s2);
        //snprintf(buf, sizeof(buf), "%s %s\r\n",s1, s2);
        s_ret_ftp_write = write(server_fd, buf, strlen(buf));
    }

    do {
        int len;
        strcpy(buf, "EOF"); /* for ftp_die */
        len = read(server_fd, buf, sizeof(buf) - 2);
        if (len <= 0) {  //ftp connect error or timeout
            s_ftp_die(NULL);
            return 0;
        }
        else {
            buf[len] = 0;
        }
    } while (!isdigit(buf[0]) || (buf[3] != ' ' &&  buf[3] != '-'));

    buf[3] = '\0';
    dbg("cmd < %s %s\n", buf, buf+4);
    n = atoi(buf);
    buf[3] = ' ';
    return n;
}

void s_ftp_quit(void)
{
    int i;

    /*for protect, only in ftp mode*/
    if (!is_ftp()) {
        return ;
    }
    for(i=0;i<3;i++)
    {
        if (strncasecmp(s_ftp_tag_comb.q_ftp_t[i].login, "on", 2))
            continue;
        

        if (s_ftpcmd(NULL, NULL, s_ftp_tag_comb.q_ftp_t[i].server_fd) != 226) {
            s_ftp_die(NULL);
        }

        if (s_ftpcmd("QUIT", NULL, s_ftp_tag_comb.q_ftp_t[i].server_fd) != 221) {
            s_ftp_die("QUIT");
        }

        if (s_ftp_tag_comb.q_ftp_t[i].server_fd != -1)
            close(s_ftp_tag_comb.q_ftp_t[i].server_fd);
    }
}

static int ftp_login(const char *ftp_server, const char *user, const char *password)
{   //printf("ftp_login ftp_server=%s,user=%s,password=%s\n",ftp_server,user,password);
    /* Connect to the command socket */
    int server_fd = connet_tcp_server(ftp_server, 21);
    if (server_fd == -1) {
        return -1;
    }

    //read connect result
    if (s_ftpcmd(NULL, NULL, server_fd) != 220) {
        s_ftp_die(NULL);
    }

    /*  Login to the server */
    switch (s_ftpcmd("USER", user, server_fd)) {
    case 230:
        break;
    case 331:
        if (s_ftpcmd("PASS", password, server_fd) != 230) {
            s_ftp_die("PASS");
        }
        break;
    default:
        s_ftp_die("USER");
    }

    /*setup target folder*/
    if (s_ftpcmd("MKD", g_str_sub_ss_log_dir, server_fd) != 230) {
        s_ftp_die("MKD");
    }
    if (s_ftpcmd("CWD", g_str_sub_ss_log_dir, server_fd) != 230) {
        s_ftp_die("CWD");
    }

    /*change it to be binary mode*/
    if (s_ftpcmd("TYPE I", NULL , server_fd) != 200) {
        s_ftp_die("TYPE I");
    }

    return server_fd;
}

static int  parse_pasv_epsv(char *buff)
{
    char *ptr;
    int port;

    if (buff[2] == '7' /* "227" */) {
        /* Response is "227 garbageN1,N2,N3,N4,P1,P2[)garbage]"
         * Server's IP is N1.N2.N3.N4 (we ignore it)
         * Server's port for data connection is P1*256+P2 */
        ptr = strrchr(buff, ')');
        if (ptr) *ptr = '\0';

        ptr = strrchr(buff, ',');
        if (!ptr) return -1;
        *ptr = '\0';
        port = atoi(ptr + 1);

        ptr = strrchr(buff, ',');
        if (!ptr) return -1;
        *ptr = '\0';
        port += atoi(ptr + 1) * 256;
    } else {
        /* Response is "229 garbage(|||P1|)"
         * Server's port for data connection is P1 */
        ptr = strrchr(buff, '|');
        if (!ptr) return -1;
        *ptr = '\0';

        ptr = strrchr(buff, '|');
        if (!ptr) return -1;
        *ptr = '\0';
        port = atoi(ptr + 1);
    }

    return port;
}

static int ftp_send(const char *ftp_server, const char *filename, int server_fd) {
    int port_num;
    int sockfd = -1;
    int response;

    // for test
    //response = s_ftpcmd("DELE", filename, server_fd);
    //printf("ftp_send DELE filename=%s,response=%d\n",filename,response);


    if (s_ftpcmd("PASV", NULL, server_fd) != 227) {
        s_ftp_die("PASV");
    }

    port_num = parse_pasv_epsv(buf);
    if (port_num < 0)
        s_ftp_die("PASV");

    sockfd = connet_tcp_server(ftp_server, port_num);
    if (sockfd < 0)
        s_ftp_die("PASV");

    response = s_ftpcmd("STOR", filename, server_fd);
    switch (response) {
    case 125:
    case 150:
        break;
    //case 550:/*550 Permission denied*/
    //    response = s_ftpcmd("DELE", filename, server_fd);
    //    printf("DELE filename=%s,response=%d\n",filename,response);
    //    response = s_ftpcmd("STOR", filename, server_fd);
    //    printf("STOR filename=%s,response=%d\n",filename,response);
    //    break;
    default:    
        s_ftp_die("STOR");
    }

    return sockfd;
}

int s_ftp_write_request(int index, const char *ftp_server, const char *user, const char *pass, const char *filename)
{
    if (strncasecmp(s_ftp_tag_comb.q_ftp_t[index].login, "on", 2))
    {
        s_ftp_tag_comb.q_ftp_t[index].server_fd = ftp_login(ftp_server, user, pass);
        if (s_ftp_tag_comb.q_ftp_t[index].server_fd == -1)
        {	return -1;}
    }else
    {
        if (s_ftpcmd(NULL, NULL, s_ftp_tag_comb.q_ftp_t[index].server_fd) != 226) {
            s_ftp_die(NULL);
        }
    }

    strcpy(s_ftp_tag_comb.q_ftp_t[index].login, "on");

    int sockfd = ftp_send(ftp_server, filename, s_ftp_tag_comb.q_ftp_t[index].server_fd);
    
    return sockfd;
}

char * target_folder_get_parse_for_ftp(char * target_filename) {
    char * current_path = target_filename;
    char * sub_path = NULL;
    char * sub_path2 = NULL;
    char * sub_path3 = NULL;

    g_str_sub_ss_log_dir[0] = '\0';

    //setup log path
    if (target_filename != NULL) {
   
        /* 1. setup g_str_ss_log_dir*/
        //sprintf(g_str_ss_log_dir , "%s", current_path);
    
        /* 2. setup g_str_sub_ss_log_dir*/
        sub_path = current_path;
    
        do {
            sub_path = strchr(sub_path, '/');
    
            if(sub_path != NULL) {
                sub_path ++;
    
                sub_path2 = strchr(sub_path, '/');
                sub_path3 = strrchr(sub_path, '/');
    
                //if the last / is left, then it is over.
                if ((sub_path2 != NULL && (sub_path2 == sub_path3)))    {
                    *sub_path2 = '\0';
                    sprintf(g_str_sub_ss_log_dir, "%s", sub_path);
                    *sub_path2 = '/';
                    break;
                } else if ((sub_path2 == NULL)) {
                    sprintf(g_str_sub_ss_log_dir, "%s", sub_path);
                    break;
                }
            }
        } while(sub_path != NULL);
    }
    //printf("target_folder_get_parse_for_ftp g_str_sub_ss_log_dir=%s\n",g_str_sub_ss_log_dir);
    return g_str_sub_ss_log_dir;
}

