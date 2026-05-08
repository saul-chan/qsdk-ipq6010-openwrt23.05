#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include <stdint.h>
#include <pthread.h>
#include <termios.h>
#include <ctype.h>
#include <signal.h>
#include <errno.h>
//#include <error.h>
#include <netinet/ip.h>
#include <sys/wait.h>
#include <stdbool.h>

#define YES 1
#define NO 0

int get_syslog(void *buff);
long hexToDec(char *source);
int hextoint(char *buf,int);
int getIndexOfSingns(char ch);
char **explode(char sep, const char *str, int *size);
uint8_t char_to_hex(uint8_t *ch);
char* itoa(int num,char* str,int radix);


typedef struct node{
	int fd;
	struct sockaddr_in client;
}node_t;

void cmd_recieve_str(char *cmd, char *respond)
{
    FILE * p_file = NULL;
	char msg[1024];
	memset(msg,0,sizeof(msg));
	char tmp_cmd[1024];
	memset(tmp_cmd,0,sizeof(tmp_cmd));
	int len = 0;
	// 使用popen函数则用pclose 关闭文件
	printf("cmd: %s\n",cmd);
	sprintf(tmp_cmd,"%s",cmd);
    if((p_file=popen(tmp_cmd,"r"))==NULL)
    {
        perror("popen error");
    }
    else
    {
        while(fgets(msg,sizeof(msg),p_file)!=NULL)
        {
			if(respond != NULL)
			{
				strncat(respond,msg,sizeof(msg));
			}
        }
		if(respond != NULL)
			respond[strlen(respond)]='\0';
    }
	pclose(p_file);
}

void Debug(int fd)
{
	
	char time[30];
	char tmp_ptr1[1024];
	char tmp_ptr2[1024];
	memset(time,0,sizeof(time));
	
	//日志格式：系统时间 2021-11-03 16:17:32)
	
	cmd_recieve_str("date \"+%Y-%m-%d %H:%M:%S\"", time); //获取系统时间
	write(fd,time,19);
}

void *client_recv(void *arg)
{
    int fd;
	char buffer[1024];
    node_t *client_t=(node_t *)arg;
    fd = client_t->fd;
    struct sockaddr_in client = client_t->client;
    char tmp2[100];
    char Mesg[4096];
  
   /*  while(1)
    {
		int ret=read(fd,buf,sizeof(buf));
		
		memset(Mesg,0,sizeof(Mesg));
		cmd_recieve_str(buff,Mesg);
		if(strlen(Mesg)>0)
			write(fd,(char *)Mesg,strlen(Mesg));
    } */
	
	while (recv(fd, buffer, 1024, 0) > 0) {
        printf("Received: %s\n", buffer);
         // 发送回复到客户端
		memset(Mesg,0,sizeof(Mesg));
		cmd_recieve_str(buffer,Mesg);
         if (send(fd, Mesg, strlen(Mesg), 0) < 0) {
            perror("send");
		}
		memset(buffer,0,sizeof(buffer));
	}
	printf("退出\n");
	return NULL;
}

char* itoa(int num,char* str,int radix)
{
	char index[]="0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";//索引表
	unsigned unum;//存放要转换的整数的绝对值,转换的整数可能是负数
	int i=0,j,k;//i用来指示设置字符串相应位，转换之后i其实就是字符串的长度；转换后顺序是逆序的，有正负的情况，k用来指示调整顺序的开始位置;j用来指示调整顺序时的交换。
 
	//获取要转换的整数的绝对值
	if(radix==10&&num<0)//要转换成十进制数并且是负数
	{
		unum=(unsigned)-num;//将num的绝对值赋给unum
		str[i++]='-';//在字符串最前面设置为'-'号，并且索引加1
	}
	else unum=(unsigned)num;//若是num为正，直接赋值给unum
	//转换部分，注意转换后是逆序的
	do
	{
		str[i++]=index[unum%(unsigned)radix];//取unum的最后一位，并设置为str对应位，指示索引加1
		unum/=radix;//unum去掉最后一位
	}while(unum);//直至unum为0退出循环
	str[i]='\0';//在字符串最后添加'\0'字符，c语言字符串以'\0'结束。
	//将顺序调整过来
	if(str[0]=='-') k=1;//如果是负数，符号不用调整，从符号后面开始调整
	else k=0;//不是负数，全部都要调整
	char temp;//临时变量，交换两个值时用到
	for(j=k;j<=(i-1)/2;j++)//头尾一一对称交换，i其实就是字符串的长度，索引最大值比长度少1
	{
		temp=str[j];//头部赋值给临时变量
		str[j]=str[i-1+k-j];//尾部赋值给头部
		str[i-1+k-j]=temp;//将临时变量的值(其实就是之前的头部值)赋给尾部
	}
	return str;//返回转换后的字符串
}

uint8_t char_to_hex(uint8_t *ch)

{
    uint8_t value = 0;
    if(*ch >= 0 && *ch <= 9)
    {
        value = *ch + 0x30;
    }
    else if(*ch >=10 && *ch <=15)
    {
       // 大写字母
        value = *ch + 0x37;
    }
    return value;
}

//返回一个 char *arr[], size为返回数组的长度
char **explode(char sep, const char *str, int *size)
{
	int count = 0, i;
	for(i = 0; i < strlen(str); i++)
	{       
		if (str[i] == sep)
		{       
			count ++;
		}
	}
	char **ret = calloc(++count, sizeof(char *));
	int lastindex = -1;
	int j = 0;
	for(i = 0; i < strlen(str); i++)
	{       
		if (str[i] == sep)
		{       
			ret[j] = calloc(i - lastindex, sizeof(char)); //分配子串长度+1的内存空间
			memcpy(ret[j], str + lastindex + 1, i - lastindex - 1);
			j++;
			lastindex = i;
		}
	}
	//处理最后一个子串
	if (lastindex <= strlen(str) - 1)
	{
		ret[j] = calloc(strlen(str) - lastindex, sizeof(char));
		memcpy(ret[j], str + lastindex + 1, strlen(str) - 1 - lastindex);
		j++;
	}
	*size = j;
	return ret;
}

int getIndexOfSingns(char ch)
{
	if(ch>='0'&&ch<='9')
		return ch-'0';
	if(ch>='A'&&ch<='F')
		return ch-'A'+10;
	if(ch>='a'&&ch<='f')
		return ch-'a'+10;
	return -1;
}

long hexToDec(char *source)
{
	long sum = 0;
	long t=1;
	int i,len;
	len=strlen(source);
	for(i=len-1;i>=0;i--)
	{
		sum+=t*getIndexOfSingns(*(source+i));
		t*=16;
	}
	return sum;
}


int hextoint(char *buf,int ret)
{
	int i;
	int kk,sum=0;
	/* for(i=0;i<ret;i++)
	{
		kk=0xC0-'0';
		printf("kk:%d\n",kk);
	}
	return kk;*/
	for(i=0;i<ret;i++)
	{
		kk=buf[i]-'0';
		sum=sum*10+kk;
	}
	return sum;
}


int get_syslog(void *buff)
{
	;
}

int main(int argc,char *argv[])
{
	signal(SIGCHLD,SIG_IGN);
	int sock_fd,ret;
	int optval = 1;
	sock_fd=socket(AF_INET,SOCK_STREAM,0);

	if(sock_fd<0)
	{
		perror("socket");
		return -1;
	}
	setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
	struct sockaddr_in addr = {AF_INET,htons(1023),INADDR_ANY};

	if(bind(sock_fd,(void *)&addr,sizeof(addr))<0)
	{
		perror("bind");
		return -2;
	}

	listen(sock_fd,1000);

	int fd;
	struct sockaddr_in peer;
	socklen_t len = sizeof(peer);
	pthread_t pid;
	node_t client;

	while(1)
	{
		fd=accept(sock_fd,(void *)&peer,&len);
		client.fd=fd;
		client.client=peer;

		if(fd > 0)
		{
			getpeername(fd,(void *)&peer,&len);
			pthread_create(&pid,NULL,(void *)client_recv,&client);
		}
	}
	return 0;
}
