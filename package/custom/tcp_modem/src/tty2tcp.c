/**
 * tty2tcp.c - source code of agent tool tty2tcp
 *
 * This is used for producing an agent tool, which is used to 
 * transmit data between SIMCom module and remote controller.
 *
 */
#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <ctype.h>
#include <time.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/epoll.h>
#include <sys/poll.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/timeb.h>
#include <stddef.h>
#include <netinet/in.h>

struct __kfifo {
    unsigned int in;
    unsigned int out;
    unsigned int mask;
    void *data;
};

#define MAX_PORT 10

int grantpt(int fd);
int unlockpt( int  fd );
int ptsname_r( int  fd, char*  buf, size_t  buflen);
char *inet_ntoa(struct in_addr in);
unsigned int inet_addr(const char *cp);
                    
pthread_mutex_t s_printfmutex = PTHREAD_MUTEX_INITIALIZER;

static const char * logtime(void)
{
    static char logtime_buf[32];
    struct timeb tb;
    struct tm *ti;

    ftime(&tb);
    ti = localtime(&tb.time);
    sprintf(logtime_buf, "[%02d-%02d_%02d:%02d:%02d:%03d] ", ti->tm_mon+1,
		ti->tm_mday, ti->tm_hour, ti->tm_min, ti->tm_sec, tb.millitm);

    return logtime_buf;
}

#define dprintf(fmt, arg...) \
    do { \
        pthread_mutex_lock(&s_printfmutex); \
        printf("%s" fmt, logtime(), ##arg); \
        pthread_mutex_unlock(&s_printfmutex);\
    } while(0)

#define min(x,y) (((x) < (y)) ? (x) : (y))

#define SYSCHECK(c)  \
    do { \
        if ((c)<0) { \
            dprintf("%s %d error: '%s' (code: %d)\n", __func__, __LINE__, strerror(errno), errno); \
            return -1; \
        } \
    } while(0)

#define cfmakenoblock(fd) \
    do { \
        fcntl(fd, F_SETFL, fcntl(fd,F_GETFL) | O_NONBLOCK); \
    } while(0) 

static int open_serial(const char *tty_port)
{
    int ttyfd;
    struct termios  ios;

    SYSCHECK(ttyfd = open (tty_port, O_RDWR | O_NDELAY));

    /* disable echo on serial ports */
    memset(&ios, 0, sizeof(ios));
    tcgetattr( ttyfd, &ios );
    cfmakeraw(&ios);
    ios.c_lflag = 0;
    cfsetispeed(&ios, B115200);
    cfsetospeed(&ios, B115200);
    tcsetattr( ttyfd, TCSANOW, &ios );
    tcflush(ttyfd, TCIOFLUSH);

    dprintf("open %s ttyfd = %d\n", tty_port, ttyfd);
    
    return ttyfd;
}

static int open_pts(const char *pts_name)
{
    int ptsfd;
    struct termios  ios;
    char pts_r[64] = {0};
    char* pts = NULL;
           
    SYSCHECK(ptsfd = open ("/dev/ptmx", O_RDWR | O_NDELAY));

    if (ptsname_r(ptsfd, pts_r, sizeof(pts_r)) == 0) {
        pts = pts_r;
    }
    
    if (pts == NULL) {
        SYSCHECK(-1);
    }

    if(symlink(pts, pts_name) != 0) {
        dprintf("Create link %s Error : %d (%s)", pts_name, errno, strerror(errno));
        return -1;
    }

    /* disable echo on serial ports */
    memset(&ios, 0, sizeof(ios));
    tcgetattr( ptsfd, &ios );
    cfmakeraw(&ios);
    ios.c_lflag = 0;
    cfsetispeed(&ios, B115200);
    cfsetospeed(&ios, B115200);
    tcsetattr( ptsfd, TCSANOW, &ios );
    tcflush(ptsfd, TCIOFLUSH);

    SYSCHECK(grantpt(ptsfd));
    SYSCHECK(unlockpt(ptsfd));

    dprintf("open %s -> %s ptsfd = %d\n", pts_name, pts_r, ptsfd);
    
    return ptsfd;
}

int __kfifo_alloc(struct __kfifo *fifo, unsigned int size)
{
    /*
    * round down to the next power of 2, since our 'let the indices
    * wrap' technique works only in this case.
    */
    fifo->in = 0;
    fifo->out = 0;

    if (size < 2) {
        fifo->data = NULL;
        fifo->mask = 0;
        return -EINVAL;
    }

    fifo->data = malloc(size);
    if (!fifo->data) {
        fifo->mask = 0;
        return -ENOMEM;
    }
    fifo->mask = size - 1;

    return 0;
}

unsigned int kfifo_used(struct __kfifo *fifo, void **pp_buf)
{
    unsigned int off = fifo->out & fifo->mask;
    unsigned int len = min(fifo->in - fifo->out, (fifo->mask + 1) - off);

    if (pp_buf) {
        *pp_buf = len ? (fifo->data + off) : NULL;
    }
    
    return len;
}

unsigned int kfifo_unused(struct __kfifo *fifo, void **pp_buf)
{
    unsigned int off = fifo->in & fifo->mask;
    unsigned int len = min((fifo->mask + 1) - (fifo->in - fifo->out), (fifo->mask + 1) - off);

    if (pp_buf) {
        *pp_buf = len ? (fifo->data + off) : NULL;
    }
    
    return len;
}

void __kfifo_in(struct __kfifo *fifo, unsigned len)
{
    fifo->in += len;
}

void __kfifo_out(struct __kfifo *fifo, unsigned len)
{
    fifo->out += len;
}

void __kfifo_free(struct __kfifo *fifo)
{
    free(fifo->data);
    fifo->in = 0;
    fifo->out = 0;
    fifo->data = NULL;
    fifo->mask = 0;
}

void __kfifo_reset(struct __kfifo *fifo)
{
    fifo->in = 0;
    fifo->out = 0;   
}

static int create_tcp_server(int socket_port)
{
    int sockfd = -1;
    int reuse_addr = 1;
    struct sockaddr_in sockaddr;
    
    /*Create server socket*/
    SYSCHECK(sockfd = socket(AF_INET, SOCK_STREAM, 0));
        
    memset(&sockaddr, 0, sizeof(sockaddr));
    sockaddr.sin_family = AF_INET;
    sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    sockaddr.sin_port = htons(socket_port);
    
    SYSCHECK(bind(sockfd, (struct sockaddr *)&sockaddr, sizeof(sockaddr)));
    SYSCHECK(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse_addr,sizeof(reuse_addr)));

    dprintf("tcp server: %d sockfd = %d\n", socket_port, sockfd);
    
    return sockfd;
}

static int connect_tcp_server(const char *tcp_host, int tcp_port)
{
    int sockfd = -1;
    struct sockaddr_in sockaddr;
    
    /*Create server socket*/
    SYSCHECK(sockfd = socket(AF_INET, SOCK_STREAM, 0));
        
    memset(&sockaddr, 0, sizeof(sockaddr));
    sockaddr.sin_family = AF_INET;
    sockaddr.sin_addr.s_addr = inet_addr(tcp_host);
    sockaddr.sin_port = htons(tcp_port);
    
    if(connect(sockfd, (struct sockaddr *) &sockaddr, sizeof(sockaddr)) < 0) {
        close(sockfd);
        return -1;
    }
    
    dprintf("tcp client: %s %d sockfd = %d\n", tcp_host, tcp_port, sockfd);
    
    return sockfd;
}

static int swap_fd_fd_thread(void *argv[])
{
    int swapfds[2];
    struct __kfifo *fifos[2];

    swapfds[0] = *((int *)argv[0]);
    swapfds[1] = *((int *)argv[1]);
    fifos[0] = (struct __kfifo *)argv[2];
    fifos[1] = (struct __kfifo *)argv[3];

    cfmakenoblock(swapfds[0]);
    cfmakenoblock(swapfds[1]);

    while (1) {
        struct pollfd pollfds[] = {{swapfds[0], 0, 0}, {swapfds[1], 0, 0}};
        int ne, ret, nevents = sizeof(pollfds)/sizeof(pollfds[0]);

        pollfds[0].events |= kfifo_unused(fifos[0], NULL) ? POLLIN : 0;
        pollfds[0].events |= kfifo_used(fifos[1], NULL) ? POLLOUT : 0;
        pollfds[1].events |= kfifo_unused(fifos[1], NULL) ? POLLIN : 0;
        pollfds[1].events |= kfifo_used(fifos[0], NULL) ? POLLOUT : 0;

        do {
            ret = poll(pollfds, nevents, -1);
         } while (ret < 0 && errno == EINTR);

        if (ret <= 0) {
            dprintf("%s poll=%d, errno: %d (%s)\n", __func__, ret, errno, strerror(errno));
            break;
        }

        for (ne = 0; ne < nevents; ne++) {
            int fd = pollfds[ne].fd;
            short revents = pollfds[ne].revents;

            if (revents & (POLLERR | POLLHUP | POLLNVAL)) {          
                dprintf("%s poll fd = %d, revents = %04x\n", __func__, fd,revents);
                return fd;
            }

            if (fd == swapfds[0] || fd == swapfds[1]) {                
                if (revents & POLLOUT) {
                    void *buf;
                    struct __kfifo *fifo = (fd == swapfds[0] ? fifos[1] : fifos[0]);
                    unsigned int len = kfifo_used(fifo, &buf);
                    
                    if (len) {
                        ssize_t nwrites = write(fd, buf, len);
                        if (nwrites <= 0) {
                            dprintf("%s write=%zd, errno: %d (%s)\n", __func__, nwrites, errno, strerror(errno));
                            return fd;
                        }
                        __kfifo_out(fifo, nwrites);
                    }
                }          
            }
        }

        for (ne = 0; ne < nevents; ne++) {
            int fd = pollfds[ne].fd;
            short revents = pollfds[ne].revents;

            if (fd == swapfds[0] || fd == swapfds[1]) {      
                if (revents & POLLIN) {
                    void *buf;
                    struct __kfifo *fifo = (fd == swapfds[0] ? fifos[0] : fifos[1]);
                    unsigned int len = kfifo_unused(fifo, &buf);
                    
                    if (len) {
                        ssize_t nreads = read(fd, buf, len);
                        if (nreads <= 0) {
                            dprintf("%s read=%zd, errno: %d (%s)\n", __func__, nreads, errno, strerror(errno));
                            return fd;
                        }

                        __kfifo_in(fifo, nreads);
                    }
                }
            }
        }
    }

    return 0;
}

static  void * swap_tcp_tty_thread(void *param)
{
    const char *tty_port = (const char *)(((void **)param)[0]);
    int tcp_port = *((int *)(((void **)param)[1]));
    unsigned int fifo_size = *((unsigned int *)(((void **)param)[2]));
    const char *tcp_host = (const char *)(((void **)param)[3]);
    int ttyfd = -1, serverfd = -1, clientfd = -1;
    struct __kfifo fifo[2];
    void *argv[12];
    struct sockaddr_in addr;
    socklen_t addr_size = sizeof(addr);
    int kfifo_init = 0;
    
    while (1) {
        /* wait for tcp client */
        clientfd = -1;
        if (tcp_host != NULL) {
            dprintf("wait for %s %d\n", tcp_host, tcp_port);
            while (clientfd < 0) {
                clientfd = connect_tcp_server(tcp_host, tcp_port);
                if (clientfd < 0)
                    sleep(1);
            }
        }

        /* wait for tty port */
        if (access(tty_port, R_OK | W_OK) && errno == ENOENT) {
            dprintf("wait for %s connect\n", tty_port);
        }
        
        while (access(tty_port, R_OK | W_OK)) {
            if (errno != ENOENT) {
                dprintf("fail access %s errno:%d (%s)\n", tty_port, errno, strerror(errno));
                return NULL;
            }
            sleep(1);
        }

        ttyfd = open_serial(tty_port);

        if (tcp_host != NULL) {
            goto __connect_tcp_server_succ;
        }

        serverfd = create_tcp_server(tcp_port);
        listen(serverfd, 1);
        cfmakenoblock(serverfd);

        /* wait for tcp client */
        while (1) {
            struct pollfd pollfds[] = {{ttyfd, 0, 0}, {serverfd, POLLIN, 0}};
            int ne, ret, nevents = sizeof(pollfds)/sizeof(pollfds[0]);

            do {
                ret = poll(pollfds, nevents, -1);
             } while (ret < 0 && errno == EINTR);

            if (ret <= 0) {
                dprintf("%s poll=%d, errno: %d (%s)\n", __func__, ret, errno, strerror(errno));
                break;
            }

            for (ne = 0; ne < nevents; ne++) {
                int fd = pollfds[ne].fd;
                short revents = pollfds[ne].revents;

                if (revents & (POLLERR | POLLHUP | POLLNVAL)) {          
                    dprintf("%s poll fd = %d, revents = %04x\n", __func__, fd,revents);
                    goto __get_tcp_client_fail;
                }

                if ((revents & POLLIN) && fd == serverfd) {
                    goto __get_tcp_client_succ;
                }
            }          
        }

__get_tcp_client_succ:
        addr_size = sizeof(addr);
        clientfd = accept(serverfd, (struct sockaddr *)(&addr), &addr_size);
        dprintf("tcp client: %s:%d <> %s clientfd = %d\n", inet_ntoa(addr.sin_addr), addr.sin_port, tty_port, clientfd);

        if (serverfd != -1) {
            close(serverfd);
        }
        serverfd = -1;

__connect_tcp_server_succ:
        argv[0] = (void *)&ttyfd;
        argv[1] = (void *)&clientfd;
        argv[2] = (void *)&fifo[0];
        argv[3] = (void *)&fifo[1];

        if (kfifo_init == 0) {
            __kfifo_alloc(&fifo[0], fifo_size);
            __kfifo_alloc(&fifo[1], fifo_size);
            kfifo_init = 1;
        }
        __kfifo_reset(&fifo[0]);
        __kfifo_reset(&fifo[1]);
        swap_fd_fd_thread(argv);

__get_tcp_client_fail:
        if (clientfd != -1) {
            close(clientfd);
        } 
        clientfd = -1;

        if (ttyfd != -1) {
            close(ttyfd);
        }
        ttyfd = -1;

        sleep(5);
    }

    return NULL;
}

static  void * swap_pts_tcp_thread(void *param)
{
    const char *pts_name = (const char *)(((void **)param)[0]);
    int tcp_port = *((int *)(((void **)param)[1]));
    unsigned int fifo_size = *((unsigned int *)(((void **)param)[2]));
    const char *tcp_host = (const char *)(((void **)param)[3]);
    int ptsfd = -1, clientfd = -1;
    struct __kfifo fifo[2];
    void *argv[12];
    int kfifo_init = 0;
    char command[128];

    sprintf(command,"rm %s -f", pts_name);

    while (1) {
        /* wait for tcp server */
        system(command);
        dprintf("wait for %s %d\n", tcp_host, tcp_port);
        while (clientfd < 0) {
            clientfd = connect_tcp_server(tcp_host, tcp_port);
            if (clientfd < 0)
                sleep(1);
        }

        ptsfd = open_pts(pts_name);
        if (ptsfd < 0) {
            return NULL;
        }
        
        argv[0] = (void *)&ptsfd;
        argv[1] = (void *)&clientfd;
        argv[2] = (void *)&fifo[0];
        argv[3] = (void *)&fifo[1];

        if (kfifo_init == 0) {
            __kfifo_alloc(&fifo[0], fifo_size);
            __kfifo_alloc(&fifo[1], fifo_size);
            kfifo_init = 1;
        }

        __kfifo_reset(&fifo[0]);
        __kfifo_reset(&fifo[1]);
        swap_fd_fd_thread(argv);

        if (clientfd != -1) {
            close(clientfd);
        }
        clientfd = -1;

        if (ptsfd != -1) {
            close(ptsfd);
        }
        ptsfd = -1;

        sleep(5);
    }

    return NULL;
}

int main(int argc, char *argv[])
{
    void *params[MAX_PORT*4];
    int opt;
    char *tcp_host = NULL;
    int tcp_port = 9000;
    int count = 1;
    int virt_tty_mode = 0;

    char tty_port[32] = "/dev/ttyUSB0";
    unsigned int fifo_size = 512 * 1024;
    pthread_t thread_process;

    while ( -1 != (opt = getopt(argc, argv, "c:p:n:t:v"))) {
        switch (opt) {
            case 'c':
                tcp_host = optarg;
            break;
            case 'p':
                tcp_port = atoi(optarg);
            break;
            case 'n':
                count = atoi(optarg);
                if (count > MAX_PORT) {
                    count = MAX_PORT;
                }
            break;
            case 't':
                sprintf(tty_port, "/dev/%s", optarg);
                printf("SIMCOM-TRACE: Target tty port is %s\n", tty_port);
            break;
            case 'v':
                virt_tty_mode = 1;
            break;
            default:
            break;
        }
   }

    params[0] = tty_port;
    params[1] = &tcp_port;
    params[2] = &fifo_size;
    params[3] = tcp_host;

    if (tcp_host != NULL && virt_tty_mode) {
        pthread_create(&thread_process, NULL, swap_pts_tcp_thread, &params[0]);
    } else {
        pthread_create(&thread_process, NULL, swap_tcp_tty_thread, &params[0]);
    }

    while(1) {
        sleep(60);
    }

    return 0;
}
