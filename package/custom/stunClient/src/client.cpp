#include <cassert>
#include <cstring>
#include <iostream>
#include <cstdlib>
#include <pthread.h>
#include <cstdio>

#ifdef WIN32
#include <time.h>
#else
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <netinet/in.h>
#include <errno.h>
#include <signal.h>
#endif


#include <sys/stat.h>
#include <fcntl.h>

#include "udp.h"
#include "stun.h"

using namespace std;


void usage(char *name)
{
    cerr << "Usage:" << endl
    << name << " stunServerHostname [-v] [-f] [-keepalive keepaliveperiod] [-u userName] [-p password] [-port srcPort] [-looptime notifylooptime]" << endl
    << "stunServerHostname  ip:port or domain name" << endl
    << "-keepalive          Stun heartbeat pack time" << endl
    << "-looptime           Run 'ubus call tr069 notify' cycle time" << endl
    << "-u                  user" << endl
    << "-p                  password" << endl
    << "-port               stun local port" << endl
    << "-v                  Print the log" << endl
    << "-f                  Print the log to a file(/tmp/stun_log)" << endl
    << "--help              Print help information" << endl << endl
    << "STUN client version " << STUN_VERSION << endl;
}

#define MAX_NIC 3

bool    client_daemon = true;   /* 默认死循环，退出信号可改变这个值 */
bool    verbose = false;        /* 原版中的打印    标志，现在不使用，可以打开 */
int     outLog = 0;             /* 现在的打印标志，打印条件看dbg() */
int     timeOut = 5;
bool    timeOutSign = false;

char localIpAndPort[30];        /* 本地IP:端口 */
char stunServerIpAndPort[30];   /* 服务器IP:端口 */

int     pipefd[2];              /* 接收程序退出信息 */
int     pipe_udp_fd[2];         /* 线程间通信      recvMessageThread >>> otherUdpPacket */
int     pipe_stun_fd[2];        /* 线程间通信      recvMessageThread >>> aliveStun */
int     pipe_ubus_fd[2];        /* 线程间通信      otherUdpPacket >>> loopCheckValueChange */


/* 循环检测easycwmp脚本中的值变化，默认不使用，argv中加参数-looptime 30 可以开启，单位秒 */
void * loopCheckValueChange(void *arg)
{
	int loopCheckTime  = *(int*)arg;
	fd_set pipe_loop_check;
    struct timeval loop_check_time;
	int ret = 0;
	char buf[100];
	int sleep_s = 20, i = 0;
	if(loopCheckTime == 0)	return NULL;
	
	for(;;)
	{
		FD_ZERO(&pipe_loop_check);
		FD_SET(pipe_ubus_fd[0], &pipe_loop_check);
		loop_check_time.tv_sec = loopCheckTime;
		
		ret = select(pipe_ubus_fd[0]+1, &pipe_loop_check, NULL, NULL, &loop_check_time);
		if(ret == 0)
		{
			dbg(outLog, "ubus call tr069 notify");

			if ( verbose )
            {
               cout << "ubus call tr069 notify" << endl;
            }
			
			ret = system("ubus call tr069 notify");
			if(ret != 0)
			{
				dbg(outLog, "system() error");
				perror("system");
			}
			continue;
		}

		if(FD_ISSET(pipe_ubus_fd[0], &pipe_loop_check))
		{
			for(i = sleep_s; i > 0; i--)
			{
				/* 如果有可能一直在上报信息，就一直循环，防止ubus call tr069 notify影响正常运行 */
				sleep(3);
				memset(buf, 0, sizeof(buf));
				read(pipe_ubus_fd[0], buf, sizeof(buf));

				if(strlen(buf))	i = sleep_s;
			}
			continue;
		}
		
	}
	return NULL;
}

//收到其他UDP包处理
void * otherUdpPacket(void *arg)
{
	int loopCheckTime = *(int*)arg;
	fd_set pipe_udp;
	char buf[100];
	int buflen = sizeof(buf);
	int ret = 0;

	for(;;)
	{
		FD_ZERO(&pipe_udp);
		FD_SET(pipe_udp_fd[0], &pipe_udp);
		select(pipe_udp_fd[0]+1, &pipe_udp, NULL, NULL, NULL);

		if (FD_ISSET(pipe_udp_fd[0], &pipe_udp)){
			if ( verbose )
            {
               cout << "ubus call tr069 inform \'{\"event\":\"6 CONNECTION REQUEST\"}\'" << endl;
            }
			
			dbg(outLog, "ubus call tr069 inform \'{\"event\":\"6 CONNECTION REQUEST\"}\'");
			if(loopCheckTime)	write(pipe_ubus_fd[1], "0", 1);
		
			ret = system ( "ubus call tr069 inform \'{\"event\":\"6 CONNECTION REQUEST\"}\'" );
			if(ret != 0)
			{
				dbg(outLog, "system() error");
				perror("system");
            }
            sleep(15);
            //运行上面的命令会进入等待，清空管道，可以保证不频繁调用ubus
            read(pipe_udp_fd[0], buf, buflen);
		}
    }
    return NULL;
}

/* 发送stun包，保持存活 */
int aliveStun(int fd, char *buf, int len, StunAddress4 *stunServerAddr, StunMessage *resp, int firstSend)
{
    int ret = 0;
    char syscmd[256] = {0};
    char pipebuf[100];
    bool remsg = false;
    static char last_stun_ucra[256] = {0};
    fd_set pipe_stun;
    
    FD_ZERO(&pipe_stun);
    FD_SET(pipe_stun_fd[0], &pipe_stun);
   
    parseIpPort(stunServerIpAndPort, stunServerAddr->addr, stunServerAddr->port);   /* 解析服务器地址 */
    if(firstSend)   dbg(outLog, "First Send: CLASSIC-STUN Binding Request", localIpAndPort, stunServerIpAndPort);
    else            dbg(outLog, "%s >>> %s CLASSIC-STUN Binding Request", localIpAndPort, stunServerIpAndPort);
    
    remsg = sendMessage( fd, buf, len, stunServerAddr->addr, stunServerAddr->port, verbose);
    if(!remsg) {
        dbg(outLog, "Failed to send Binding Request!!!", localIpAndPort, stunServerIpAndPort);
        return -1;
    }
    
    struct timeval tv_re_timeout;
    tv_re_timeout.tv_sec = timeOut;   /* 设置stun回复包的超时时间 */

    ret = select(pipe_stun_fd[0] + 1, &pipe_stun, NULL, NULL, &tv_re_timeout);
    if ( ret != 0 )
    {
        timeOutSign = true;
        Socket s = openPort( 0, resp->mappedAddress.ipv4.addr, false );
        if ( s != INVALID_SOCKET )
        {
            closesocket(s);
            dbg(outLog, "not in NAT!!!");
            system ( "echo 0 > /tmp/stun_natdetected" );
        }
        else
        {				  
            system ( "echo 1 > /tmp/stun_natdetected" );
            sprintf ( syscmd, "echo %s > /tmp/stun_ucra", localIpAndPort);
                system (syscmd);
            if (strcmp(last_stun_ucra, syscmd) != 0){
                //地址、端口发生变化，主动连一下ACS
                strcpy(last_stun_ucra, syscmd);
                system ( "ubus call tr069 inform \'{\"event\":\"6 CONNECTION REQUEST\"}\'" );
            }
        }
        
        read(pipe_stun_fd[0], pipebuf, sizeof(pipebuf));
    }
    else
    {
        dbg(outLog, "Waiting for server reply timed out!!!");
        return -2;
    }

    return 0;
}

/* 打开接收UDP消息的套接字 */
Socket initSocket(int *fd, int srcPort)
{
    StunAddress4 sAddr[MAX_NIC];

    for ( int i=0; i<MAX_NIC; i++ )
    {
        sAddr[i].addr=0; 
        sAddr[i].port=0;
    }
    sAddr[0].port=srcPort;

    *fd = openPort(srcPort, sAddr[0].addr, verbose);
}

/* 封装stun消息 */
void initMessage(char *buf, int *len, char *username, char *password)
{
    StunMessage req = {0};
    StunAtrString _username = {0};
    StunAtrString _password = {0};
    const char *binding = "dslforum.org/TR-111";

    *len = STUN_MAX_MESSAGE_SIZE;
    if (username[0] != 0){
        stunCreateAtrString(_username, username);
    }
    if (password[0] != 0){
        stunCreateAtrString(_password, password);
    }
    
    stunBuildReqSimple(&req, _username, false, false, 0x0c);

    if (stunCreateAtrString(req.requestBinding, binding) ){
        req.hasRequestBinding = true;
    }

    *len = stunEncodeMessage( req, buf, *len, _password, verbose);
}

/* 解析argv参数 */
void initArg(int argc, char* argv[], 
                int *outLog, int *keepaliveperiod, int *loopCheckTime, int *srcPort,
                char *username, char *password, StunAddress4 *stunServerAddr)
{
    int arg;
    if(argc <=1 || (argc ==2 && !strcmp(argv[1], "--help")))
    {
        usage(argv[0]);
        exit(-1);
    }
    
    memset(username, 0, sizeof(username));
    memset(password, 0, sizeof(password));
    
    for ( arg = 1; arg < argc; arg++ )
    {
        if ( !strcmp( argv[arg] , "-v" ) )
        {
            *outLog = 1;
        }
        else if ( !strcmp( argv[arg] , "-f" ) )
        {
            *outLog = 2;
        }
        else if ( !strcmp(argv[arg] , "-u") )  /**Mic added */
        {
            arg++ ;
            strcpy(username, argv[arg]);
        }
        else if ( !strcmp(argv[arg], "-p")) /**Mic added */
        {
            arg++ ;
            strcpy(password, argv[arg]);
        }
        else if ( !strcmp(argv[arg], "-keepalive")) /* Mic added */
        {
            arg++ ;
            *keepaliveperiod = strtol( argv[arg], NULL, 10);
        }
        else if ( !strcmp(argv[arg], "-looptime")) /* Mic added */
        {
            arg++ ;
            *loopCheckTime = strtol( argv[arg], NULL, 10);
        }
        else if ( !strcmp( argv[arg] , "-port" ) )
        {
            arg++;
            if ( argc <= arg ) 
            {
                usage(argv[0]);
                exit(-1);
            }
            *srcPort = strtol( argv[arg], NULL, 10);
        }
        else    
        {
            stunServerAddr->addr = 0;
            if ( !stunParseServerName( argv[arg], *stunServerAddr) )
            {
                usage(argv[0]);
                exit(-1);
            }	
        }
    }

    if ( *srcPort == 0 )
    {
        *srcPort = stunRandomPort();
    }

    if (*keepaliveperiod == 0){
        *keepaliveperiod = 10 ;
    }else if (*keepaliveperiod <= 1){
        *keepaliveperiod = 1 ;
    } 

    if(*loopCheckTime == 0){
        *loopCheckTime = 0;
    }else if (*loopCheckTime < 30){
        *loopCheckTime = 30 ;
    }

}

/* 退出的信号的回调函数 */
void handle_sigint(int sig) 
{
   char buf;
   client_daemon = false;
   write(pipefd[1],&buf,1);
   dbg(outLog, "Caught signal %d", sig);
} 

/* 添加退出信号 */
void initSignal()  {
    signal(SIGTERM, handle_sigint);
    signal(SIGKILL, handle_sigint);
    signal(SIGHUP, handle_sigint);
    signal(SIGINT, handle_sigint);
}

/* 初始化结构体，接收udp包的进程会用到这些值 */
void initStruct(UdpMsg *udpmsg, Socket fd, StunMessage *resp)
{
    udpmsg->fd = fd;
    udpmsg->verbose = verbose;
    udpmsg->outLog = outLog;
    udpmsg->pipe_udp_fd = pipe_udp_fd;
    udpmsg->pipe_stun_fd = pipe_stun_fd;
    udpmsg->resp = resp;
    udpmsg->localIpAndPort = localIpAndPort;
    udpmsg->stunServerIpAndPort = stunServerIpAndPort;
}

/* 初始化管道 */
void initPipe()
{
    pipe(pipefd);
    pipe(pipe_udp_fd);
    pipe(pipe_stun_fd);
    pipe(pipe_ubus_fd);
}

/* 释放资源 */
void endClose(Socket fd)
{
    closesocket(fd);
    closesocket(pipefd[0]);
    closesocket(pipefd[1]);
    closesocket(pipe_udp_fd[0]);
    closesocket(pipe_udp_fd[1]);
    closesocket(pipe_stun_fd[0]);
    closesocket(pipe_stun_fd[1]);
    closesocket(pipe_ubus_fd[0]);
    closesocket(pipe_ubus_fd[1]);
}

/* 这个select()用起来有点问题，搞不清楚,原因跟时间更新有关，这里用点其他办法解决等待时间过长的问题，只在程序开始时运行一次 */
void * stunTimeOut(void *arg)
{
    Socket fd = *(Socket *)arg;
	sleep(timeOut+1);
    if(!timeOutSign)
    {
        dbg(outLog, "Waiting for server reply timed out!!!");
        dbg(outLog, "exit");
        endClose(fd);
        exit(-1);
    }

    return NULL;
}

int
main(int argc, char* argv[])
{
    assert( sizeof(UInt8 ) == 1 );
    assert( sizeof(UInt16) == 2 );
    assert( sizeof(UInt32) == 4 );

    int     srcPort=0;
    int     loopCheckTime=0;
    int     keepaliveperiod = 0;
    int     len = 0;
    int     msgLen = STUN_MAX_MESSAGE_SIZE;
    int     firstSend = 1;
    int     ret = 0;
    
    char    pipebuf[100];
    char    username[64];
    char    password[64];
    char    msg[STUN_MAX_MESSAGE_SIZE];
    char    buf[STUN_MAX_MESSAGE_SIZE];

    Socket  myFd;
    UdpMsg udpmsg;
    StunMessage resp;
    StunAddress4 stunServerAddr;
    pthread_t other_udp_packet_thread, recv_thread, loop_check, timeOutThread;

    initNetwork();
    initArg(argc, argv, &outLog, &keepaliveperiod, &loopCheckTime, &srcPort, username, password, &stunServerAddr);
    init_file_mut(outLog);
    initSocket(&myFd, srcPort);
    initStruct(&udpmsg, myFd, &resp);
    initMessage(buf, &len, username, password);
    initPipe();
    initSignal();

    dbg(outLog, "STUN client version %s", STUN_VERSION);
    pthread_create(&loop_check, NULL, loopCheckValueChange, (void *)&loopCheckTime);
    pthread_create(&other_udp_packet_thread, NULL, otherUdpPacket, (void *)&loopCheckTime);
    pthread_create(&recv_thread, NULL, recvMessageThread, (void*)&udpmsg);

    while (client_daemon)
    {	
        /* 第一次发送特殊处理，如果发送失败，直接退出，守护进程会重新启动程序并初始化，如果不退选择继续会有惊喜 */
        if(firstSend)
        {
            timeOutSign = false;
            pthread_create(&timeOutThread, NULL, stunTimeOut, (void*)&myFd);
            ret = aliveStun(myFd, buf, len, &stunServerAddr, &resp, firstSend);

            if(ret < 0)
            {
                endClose(myFd);
                dbg(outLog, "exit");
                return -1;
            }

            firstSend = 0;
            continue;
        }

        fd_set rset;
        FD_ZERO(&rset);
        FD_SET(pipefd[0], &rset);

        struct timeval tv_live;
        tv_live.tv_sec = keepaliveperiod;
        
        ret = select(pipefd[0] + 1, &rset, NULL, NULL, &tv_live);
        if(ret == 0)
        {
            aliveStun(myFd, buf, len, &stunServerAddr, &resp, firstSend);
        }

        if (FD_ISSET(pipefd[0], &rset)){
            /** 收到中断信号，退出程序*/
            dbg(outLog, "receive exit signal");
            break;
        }
    }

    return 0;
}

