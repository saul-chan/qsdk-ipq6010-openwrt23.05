#include <cassert>
#include <cstdio>
#include <cstring>
#include <errno.h>
#include <iostream>
#include <cstdlib>
#include <time.h>
#include <pthread.h>

#ifdef WIN32

#include <winsock2.h>
#include <stdlib.h>
#include <io.h>

#else

#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>

#endif

#include <string.h>
#include <stdarg.h>

#include "udp.h"

using namespace std;
pthread_mutex_t mut;

void init_file_mut(int outLog)
{
	if(outLog == 2) pthread_mutex_init(&mut, NULL);
}

int dbgf(int outLog, const char *format, ...)
{
	va_list vl;
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);

    if(outLog == 0)     return 0;
	if(outLog == 1)
    {
		va_start(vl, format);
		printf("[%d-%02d-%02d %02d:%02d:%02d ", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
		vprintf(format, vl);
		va_end(vl);

        return 1;
	}

   	if(outLog == 2){
    	FILE *fp;
    	char logFile[20] = "/tmp/stun_log";
    	char logFileBack[20] = "/tmp/stun_log_back";
    	long fileMax = 2500000;
    	long fileSize = 0;
        
   		pthread_mutex_lock(&mut);
   		fp = fopen(logFile, "a+");
		if(fp == NULL)	{ pthread_mutex_unlock(&mut);	return -1; }

		va_start(vl, format);
		fprintf(fp, "[%d-%02d-%02d %02d:%02d:%02d ", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
		vfprintf(fp, format, vl);
		va_end(vl);

		fseek(fp, 0, SEEK_END);
		fileSize = ftell(fp);
		fclose(fp);
		
		if(fileSize > fileMax)
		{
			remove(logFileBack);
			rename(logFile, logFileBack);
		}
		pthread_mutex_unlock(&mut);

        return 2;
	}

	return -1;
}


Socket
openPort( unsigned short port, unsigned int interfaceIp, bool verbose )
{
   Socket fd;
    
   fd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
   if ( fd == INVALID_SOCKET )
   {
      int err = getErrno();
      cerr << "Could not create a UDP socket:" << err << endl;
      return INVALID_SOCKET;
   }
    
   struct sockaddr_in addr;
   memset((char*) &(addr),0, sizeof((addr)));
   addr.sin_family = AF_INET;
   addr.sin_addr.s_addr = htonl(INADDR_ANY);
   addr.sin_port = htons(port);
    
   if ( (interfaceIp != 0) && 
        ( interfaceIp != 0x100007f ) )
   {
      addr.sin_addr.s_addr = htonl(interfaceIp);
      if (verbose )
      {
         clog << "Binding to interface " 
              << hex << "0x" << htonl(interfaceIp) << dec << endl;
      }
   }
	
   if ( bind( fd,(struct sockaddr*)&addr, sizeof(addr)) != 0 )
   {
      int e = getErrno();
        
      switch (e)
      {
         case 0:
         {
            cerr << "Could not bind socket" << endl;
			close(fd);
            return INVALID_SOCKET;
         }
         case EADDRINUSE:
         {
            cerr << "Port " << port << " for receiving UDP is in use" << endl;
			close(fd);
            return INVALID_SOCKET;
         }
         break;
         case EADDRNOTAVAIL:
         {
            if ( verbose ) 
            {
               cerr << "Cannot assign requested address" << endl;
            }
			close(fd);
            return INVALID_SOCKET;
         }
         break;
         default:
         {
            cerr << "Could not bind UDP receive port"
                 << "Error=" << e << " " << strerror(e) << endl;
			close(fd);
            return INVALID_SOCKET;
         }
         break;
      }
   }
   if ( verbose )
   {
      clog << "Opened port " << port << " with fd " << fd << endl;
   }
   
   assert( fd != INVALID_SOCKET  );
	
   return fd;
}

bool getMessageTimeout(Socket fd, char* buf, int* len,
            unsigned int* srcIp, unsigned short* srcPort,
            bool verbose, int timeout)
/* Mic: 增加超时机制，防止因为丢包导致程序一直收不到响应包, timeout单位为秒*/ 
{
   fd_set rset;
   struct timeval tv;
   FD_ZERO(&rset);
   FD_SET(fd, &rset);
   tv.tv_sec = timeout;
   int nready = select(fd+1, &rset, NULL, NULL, &tv);
   if (nready == 0){
      /* 超时 */
      return false;
   }
   if (FD_ISSET(fd, &rset)){
      return getMessage(fd, buf, len, srcIp, srcPort, verbose);
   }else{
      /*其他因素失败*/
      return false;
   }
}

bool 
getMessage( Socket fd, char* buf, int* len,
            unsigned int* srcIp, unsigned short* srcPort,
            bool verbose)
{
   assert( fd != INVALID_SOCKET );
	
   int originalSize = *len;
   assert( originalSize > 0 );
   
   struct sockaddr_in from;
   int fromLen = sizeof(from);
	
   *len = recvfrom(fd,
                   buf,
                   originalSize,
                   0,
                   (struct sockaddr *)&from,
                   (socklen_t*)&fromLen);
	
   if ( *len == SOCKET_ERROR )
   {
      int err = getErrno();
		
      switch (err)
      {
         case ENOTSOCK:
            cerr << "Error fd not a socket" <<   endl;
            break;
         case ECONNRESET:
            cerr << "Error connection reset - host not reachable" <<   endl;
            break;
				
         default:
            cerr << "Socket Error=" << err << endl;
      }
		
      return false;
   }
	
   if ( *len < 0 )
   {
      clog << "socket closed? negative len" << endl;
      return false;
   }
    
   if ( *len == 0 )
   {
      clog << "socket closed? zero len" << endl;
      return false;
   }
    
   *srcPort = ntohs(from.sin_port);
   *srcIp = ntohl(from.sin_addr.s_addr);
	
   if ( (*len)+1 >= originalSize )
   {
      if (verbose)
      {
         clog << "Received a message that was too large" << endl;
      }
      return false;
   }
   buf[*len]=0;
   return true;
}


bool 
sendMessage( Socket fd, char* buf, int l, 
             unsigned int dstIp, unsigned short dstPort,
             bool verbose)
{
   assert( fd != INVALID_SOCKET );
    
   int s;
   if ( dstPort == 0 )
   {
      // sending on a connected port 
      assert( dstIp == 0 );
		
      s = send(fd,buf,l,0);
   }
   else
   {
      assert( dstIp != 0 );
      assert( dstPort != 0 );
        
      struct sockaddr_in to;
      int toLen = sizeof(to);
      memset(&to,0,toLen);
        
      to.sin_family = AF_INET;
      to.sin_port = htons(dstPort);
      to.sin_addr.s_addr = htonl(dstIp);
        
      s = sendto(fd, buf, l, 0,(sockaddr*)&to, toLen);
   }
    
   if ( s == SOCKET_ERROR )
   {
      int e = getErrno();
      switch (e)
      {
         case ECONNREFUSED:
         case EHOSTDOWN:
         case EHOSTUNREACH:
         {
            // quietly ignore this 
         }
         break;
         case EAFNOSUPPORT:
         {
            cerr << "err EAFNOSUPPORT in send" << endl;
         }
         break;
         default:
         {
            cerr << "err " << e << " "  << strerror(e) << " in send" << endl;
         }
      }
      return false;
   }
    
   if ( s == 0 )
   {
      cerr << "no data sent in send" << endl;
      return false;
   }
    
   if ( s != l )
   {
      if (verbose)
      {
         cerr << "only " << s << " out of " << l << " bytes sent" << endl;
      }
      return false;
   }
    
   return true;
}


void
initNetwork()
{
#ifdef WIN32 
   WORD wVersionRequested = MAKEWORD( 2, 2 );
   WSADATA wsaData;
   int err;
	
   err = WSAStartup( wVersionRequested, &wsaData );
   if ( err != 0 ) 
   {
      // could not find a usable WinSock DLL
      cerr << "Could not load winsock" << endl;
      assert(0); // is this is failing, try a different version that 2.2, 1.0 or later will likely work 
      exit(1);
   }
    
   /* Confirm that the WinSock DLL supports 2.2.*/
   /* Note that if the DLL supports versions greater    */
   /* than 2.2 in addition to 2.2, it will still return */
   /* 2.2 in wVersion since that is the version we      */
   /* requested.                                        */
    
   if ( LOBYTE( wsaData.wVersion ) != 2 ||
        HIBYTE( wsaData.wVersion ) != 2 ) 
   {
      /* Tell the user that we could not find a usable */
      /* WinSock DLL.                                  */
      WSACleanup( );
      cerr << "Bad winsock verion" << endl;
      assert(0); // is this is failing, try a different version that 2.2, 1.0 or later will likely work 
      exit(1);
   }    
#endif
}

char *parseIpPort(char *ipAndPort, UInt32 ip, UInt16 port)
{
    memset(ipAndPort, 0, sizeof(ipAndPort));
    sprintf ( ipAndPort, "%d.%d.%d.%d:%d", 
                ((int)(ip>>24)&0xFF), 
                ((int)(ip>>16)&0xFF),
                ((int)(ip>> 8)&0xFF),
                ((int)(ip>> 0)&0xFF),
                port );
    return ipAndPort;
}


//接收线程所要执行的函数 接收消息
void * recvMessageThread(void *arg)
{
	UdpMsg *udpmsg = (UdpMsg *)arg;
	Socket fd = udpmsg->fd;
	int verbose = udpmsg->verbose;
	int outLog = udpmsg->outLog;
	char buf[STUN_MAX_MESSAGE_SIZE];
    char fromIpAndPort[30];
	StunMessage res;
    static char IpAndPort[30];
	
	assert( fd != INVALID_SOCKET );
	
	int originalSize = sizeof(buf);
    assert( originalSize > 0 );
	
	struct sockaddr_in from;
	int fromLen = sizeof(from);

	int len = 0;
	//循环接收客户发送过来的数据  
	while(1)
	{	
	   len = recvfrom(fd,
	                   buf,
	                   originalSize,
	                   0,
	                   (struct sockaddr *)&from,
	                   (socklen_t*)&fromLen);

	   if ( len == SOCKET_ERROR )
	   {
	      int err = getErrno();
			
	      switch (err)
	      {
	         case ENOTSOCK:
	            cerr << "Error fd not a socket" <<   endl;
	            break;
	         case ECONNRESET:
	            cerr << "Error connection reset - host not reachable" <<   endl;
	            break;
					
	         default:
	            cerr << "Socket Error=" << err << endl;
	      }
			
	      continue;
	   }
		
	   if ( len < 0 )
	   {
	      clog << "socket closed? negative len" << endl;
	      continue;
	   }
	    
	   if ( len == 0 )
	   {
	      clog << "socket closed? zero len" << endl;
	      continue;
	   }
	    
		
	   if ( (len)+1 >= originalSize )
	   {
	      if (verbose)
	      {
	         clog << "Received a message that was too large" << endl;
	      }
	      continue;
	   }
	   buf[len]=0;
       
       memset(fromIpAndPort, 0, sizeof(fromIpAndPort));
       sprintf(fromIpAndPort, "%s:%ld", inet_ntoa(from.sin_addr), (long)ntohs(from.sin_port));
       
       memset(&res, 0, sizeof(StunMessage));
	   bool ok = stunParseMessage( buf, len, res, verbose);
	   if(ok)
	   {
		   memset(udpmsg->resp, 0, sizeof(StunMessage));
		   *(udpmsg->resp) = res;
           
           parseIpPort(udpmsg->localIpAndPort, udpmsg->resp->mappedAddress.ipv4.addr, udpmsg->resp->mappedAddress.ipv4.port);           
		   dbg(outLog, "%s <<< %s CLASSIC-STUN Binding Response", udpmsg->localIpAndPort, fromIpAndPort);
           if(strcmp(IpAndPort, udpmsg->localIpAndPort) != 0)
           {
               strcpy(IpAndPort, udpmsg->localIpAndPort);
               dbg(outLog, "NAT Change: localhost <==> %s <==> %s", IpAndPort, udpmsg->stunServerIpAndPort);
           }
		   write((udpmsg->pipe_stun_fd)[1], "0", 1);
	   }
	   else
	   {
	   	   dbg(outLog, "%s <<< %s Evoke inform uploads", udpmsg->localIpAndPort, fromIpAndPort);
		   write((udpmsg->pipe_udp_fd)[1], "0", 1);
	   }
	}
	//关闭通信socket
	
	close(fd);
	return NULL;
}

/* ====================================================================
 * The Vovida Software License, Version 1.0 
 * 
 * Copyright (c) 2000 Vovida Networks, Inc.  All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 
 * 3. The names "VOCAL", "Vovida Open Communication Application Library",
 *    and "Vovida Open Communication Application Library (VOCAL)" must
 *    not be used to endorse or promote products derived from this
 *    software without prior written permission. For written
 *    permission, please contact vocal@vovida.org.
 *
 * 4. Products derived from this software may not be called "VOCAL", nor
 *    may "VOCAL" appear in their name, without prior written
 *    permission of Vovida Networks, Inc.
 * 
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESSED OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, TITLE AND
 * NON-INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL VOVIDA
 * NETWORKS, INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT DAMAGES
 * IN EXCESS OF $1,000, NOR FOR ANY INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 * 
 * ====================================================================
 * 
 * This software consists of voluntary contributions made by Vovida
 * Networks, Inc. and many individuals on behalf of Vovida Networks,
 * Inc.  For more information on Vovida Networks, Inc., please see
 * <http://www.vovida.org/>.
 *
 */

// Local Variables:
// mode:c++
// c-file-style:"ellemtel"
// c-file-offsets:((case-label . +))
// indent-tabs-mode:nil
// End:
