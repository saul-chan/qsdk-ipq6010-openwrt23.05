#include <stdio.h>
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
#include <netinet/ip.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <ctype.h>
#include <fcntl.h> 
#include <stdbool.h>
#define MAX_B_COUNT 50  // 连续检测到"b"的最大次数
static int b_count = 0; // 记录连续检测到"b"的次数

int flag_debug = 2;
int value_signal = 0;
int flag_ttyUSB = 0;
int flag_heartbeat = 0;
char value_module[100];
char value_ifname[100];
char flag_module[5];
char name_config[10];
char path_usb[100];
char path_at[100] = "/tmp/at_";
char path_outfile[100] = "/tmp/tmp_";
char path_formfile[100] = "/tmp/";
char path_debuglog[100] = "/etc/debug_";
char tmp_debuglog[500] = "/etc/tmp_debug_";
char cmd_ifup[100] = "ifup ";
char cmd_ifdown[100] = "ifdown ";
char heartbeat_file[100] = "/tmp/heartbeat_file_";
char board_name[100];
int count_loop=0;
int speed_arr[] = { B115200, B9600, };    
int name_arr[] = {115200, 9600, };

bool is_openwrt23_05(void);

int detectProcessByName(char * processName)
{
    //char *program_name = "your_program_name"; // 替换为你想检查的进程名
    char cmd[256];
    int ret;
 
    // 构造ps命令，查找特定名字的进程
    sprintf(cmd, "ps | grep '%s' | grep -v grep | awk '{print $1}'", processName);
 
    // 执行命令，获取进程ID
    FILE *fp = popen(cmd, "r");
    if (fp == NULL) {
        perror("popen failed:");
        return -1;
    }
 
    // 读取进程ID
    char pid_str[10];
    while (fgets(pid_str, sizeof(pid_str), fp) != NULL) {
        pid_t pid = atoi(pid_str);
        if (pid != getpid()) { // 避免杀死自己
            // 使用kill系统调用杀死进程
            ret = kill(pid, 9); // 9是SIGKILL信号
            if (ret != 0) {
                perror("kill failed:");
            } else {
                printf("Process %d killed.\n", pid);
            }
        }
    }
 
    pclose(fp);
    return 0;
}

/* 功能：读取命令返回值 
** 传参：cmd 命令；respond 返回结果
** 返回值：void
*/
void cmd_recieve_str(char *cmd, char *respond)
{
	if(strcmp(name_config,"SIM")==0)
		detectProcessByName("cmdat");
    FILE * p_file = NULL;
	char msg[1024];
	memset(msg,0,sizeof(msg));
	char tmp_cmd[1024];
	memset(tmp_cmd,0,sizeof(tmp_cmd));
	int len = 0;
	if(strstr(cmd,"cmdat"))
	{
		find_ttyUSB();
	}
	// 使用popen函数则用pclose 关闭文件
	sprintf(tmp_cmd,"%s | awk '{printf$0}' | sed 's/[[:space:]]*$//'",cmd);
    if((p_file=popen(tmp_cmd,"r"))==NULL)
    {
        perror("popen error");
        //return -2;
    }
    else
    {
        while(fgets(msg,sizeof(msg),p_file)!=NULL)
        {
            //printf("msg:%s",msg);
			if(respond != NULL)
			{
				//printf("msg:%s",msg);
				strncat(respond,msg,sizeof(msg));
			}
        }
		if(respond != NULL)
			respond[strlen(respond)]='\0';
		//printf("respond:%s",respond);
    }
	pclose(p_file);
	/* printf("sizeof respond:%d\n",sizeof(respond));
	if(sizeof(respond)>0)
	{
		printf("respond:%s\n",respond);
	}else{
		printf("no respond\n");
	} */
}

/* 功能：读取命令返回值
** 传参：cmd 命令
** 返回值：int
*/
int cmd_recieve_int(char * cmd)
{
	if(strcmp(name_config,"SIM")==0)
		detectProcessByName("cmdat");
    FILE * p_file = NULL;
	if(strstr(cmd,"cmdat"))
	{
		find_ttyUSB();
	}
	char msg[1024];
	memset(msg,0,sizeof(msg));
	// 使用popen函数则用pclose 关闭文件
    if((p_file=popen(cmd,"r"))==NULL)
    {
        perror("popen error");
        //return -2;
    }
    else
    {
        while(fgets(msg,sizeof(msg),p_file)!=NULL)
        {
			//printf("msg:%s\n",msg);
			//strcat(msg,msg);
        }
       
    }
	pclose(p_file);
	
	//printf("strlen respond:%d\n",strlen(msg));
	if(strlen(msg)>0)
	{
		//printf("respond:%s\n",msg);
		msg[strlen(msg)-1]='\0';
		return atoi(msg);
	}else{
		//printf("no respond\n");
		return -1;
	}
}

/* 功能：获取文件大小
** 传参：path /路径/文件名
** 返回值：long int
*/ 
unsigned long get_file_size(char * path)
{  
    unsigned long filesize = -1;      
    struct stat statbuff;  
    if(stat(path, &statbuff) < 0){  
        return filesize;  
    }else{  
        filesize = statbuff.st_size;  
    }  
    return filesize;  
}

/* 功能：打印日志
** 传参：str 内容；head 项名
** 返回值：void
*/ 
void Debug(char *str,char *head)
{
	if( flag_debug == 0 ) //不打印日志
		return;
	char time[30];
	char tmp_ptr1[1024];
	char tmp_ptr2[1024];
	memset(time,0,sizeof(time));
	memset(tmp_ptr1,0,sizeof(tmp_ptr1));
	memset(tmp_ptr2,0,sizeof(tmp_ptr2));
	//日志格式：系统时间（2021-11-03 16:17:32）项（APN：）内容（3gnet）
	if( head != NULL && strlen(head) > 0 ) //存在项名且不为空
		sprintf(tmp_ptr1,"%s : %s",head,str);
	else //不存在项名，直接打印时间和内容
		strcpy(tmp_ptr1,str);
	cmd_recieve_str("date \"+%Y-%m-%d %H:%M:%S\" | xargs echo -n", time); //获取系统时间
	sprintf(tmp_ptr2, "%s %s",time, tmp_ptr1);
	if ( flag_debug == 1 ) //后台显示日志
	{
		printf("%s\n", tmp_ptr2);
	}
	else if( flag_debug == 2 ) //将日志输出到/etc/debug文件中
	{
		char tmp_ptr[1024];
		memset(tmp_ptr,0,sizeof(tmp_ptr));
		
		sprintf(tmp_ptr,"%s%s%s%s","echo \"",tmp_ptr2,"\">>",path_debuglog);
		system(tmp_ptr);
		if ( get_file_size(path_debuglog) > 100000 ){
			char tmp_tail[200]; //tail命令
			//将/etc/debug文件的最后十行复制到/etc/tmp_debug文件
			memset(tmp_tail,0,sizeof(tmp_tail));
			sprintf(tmp_tail,"%s %s %s %s","tail -n 10",path_debuglog,">",tmp_debuglog);
			system(tmp_tail);
			//删除/etc/debug文件
			memset(tmp_tail,0,sizeof(tmp_tail));
			sprintf(tmp_tail,"%s %s","rm",path_debuglog);
			system(tmp_tail);
			//将/etc/tmp_debug文件复制到/etc/debug
			memset(tmp_tail,0,sizeof(tmp_tail));
			sprintf(tmp_tail,"%s %s %s","mv",tmp_debuglog,path_debuglog);
			system(tmp_tail);
		}
	}
}
/*-----------------------------------------------------------------------------  
  函数名:      set_parity  
  参数:        int fd  
  返回值:      int  
  描述:        设置fd表述符的奇偶校验  
 *-----------------------------------------------------------------------------*/    
int set_parity(int fd)    
{    
    struct termios opt;    
    
    if(tcgetattr(fd,&opt) != 0)                 //或许原先的配置信息     
    {    
        perror("Get opt in parity error:");    
        return -1;    
    }    
    
    /*通过设置opt数据结构，来配置相关功能，以下为八个数据位，不使能奇偶校验*/    
    opt.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP    
                | INLCR | IGNCR | ICRNL | IXON);    
    opt.c_oflag &= ~OPOST;    
    opt.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);    
    opt.c_cflag &= ~(CSIZE | PARENB);    
    opt.c_cflag |= CS8;    
    
    tcflush(fd,TCIFLUSH);                           //清空输入缓存     
    
    if(tcsetattr(fd,TCSANOW,&opt) != 0)    
    {    
        perror("set attr parity error:");    
        return -1;    
    }    
    
    return 0;    
}
/*-----------------------------------------------------------------------------  
  函数名:      set_speed  
  参数:        int fd ,int speed  
  返回值:      void  
  描述:        设置fd表述符的串口波特率  
 *-----------------------------------------------------------------------------*/    
void set_speed(int fd ,int speed)    
{    
    struct termios opt;    
    int i;    
    int status;    
    
    tcgetattr(fd,&opt);    
    for(i = 0;i < sizeof(speed_arr)/sizeof(int);i++)    
    {    
        if(speed == name_arr[i])                        //找到标准的波特率与用户一致     
        {    
            tcflush(fd,TCIOFLUSH);                      //清除IO输入和输出缓存     
            cfsetispeed(&opt,speed_arr[i]);         //设置串口输入波特率     
            cfsetospeed(&opt,speed_arr[i]);         //设置串口输出波特率     
    
            status = tcsetattr(fd,TCSANOW,&opt);    //将属性设置到opt的数据结构中，并且立即生效     
            if(status != 0)    
                perror("tcsetattr fd:");                //设置失败     
            return ;    
        }    
        tcflush(fd,TCIOFLUSH);                          //每次清除IO缓存     
    }    
} 
/*-----------------------------------------------------------------------------  
  函数名:      serial_init  
  参数:        char *dev_path,int speed,int is_block  
  返回值:      初始化成功返回打开的文件描述符  
  描述:        串口初始化，根据串口文件路径名，串口的速度，和串口是否阻塞,block为1表示阻塞  
 *-----------------------------------------------------------------------------*/   
int serial_init(char *dev_path,int speed,int is_block)    
{    
    int fd;    
    int flag;    
    
    flag = 0;    
    flag |= O_RDWR;                     //设置为可读写的串口属性文件     
    if(is_block == 0)    
        flag |=O_NONBLOCK;              //若为0则表示以非阻塞方式打开     
    
    fd = open(dev_path,flag);               //打开设备文件     
    if(fd < 0)    
    {    
        perror("Open device file err:");    
        close(fd);    
        return -1;    
    }    
    
    /*打开设备文件后，下面开始设置波特率*/    
    set_speed(fd,speed);                //考虑到波特率可能被单独设置，所以独立成函数     
    
    /*设置奇偶校验*/    
    if(set_parity(fd) != 0)    
    {    
        perror("set parity error:");    
        close(fd);                      //一定要关闭文件，否则文件一直为打开状态     
        return -1;    
    }    
    
    return fd;    
}
/*-----------------------------------------------------------------------------  
  函数名:      serial_send  
  参数:        int fd,char *str,unsigned int len  
  返回值:      发送成功返回发送长度，否则返回小于0的值  
  描述:        向fd描述符的串口发送数据，长度为len，内容为str  
 *-----------------------------------------------------------------------------*/    
int serial_send(int fd,char *str,unsigned int len)    
{    
    int ret;    
    
    if(len > strlen(str))                    //判断长度是否超过str的最大长度     
        len = strlen(str);    
    
    ret = write(fd,str,len);    
    if(ret < 0)    
    {    
        perror("serial send err:");    
        return -1;    
    }    
    
    return ret;    
}    
    
/*-----------------------------------------------------------------------------  
  函数名:      serial_read  
  参数:        int fd,char *str,unsigned int len,unsigned int timeout  
  返回值:      在规定的时间内读取数据，超时则退出，超时时间为ms级别  
  描述:        向fd描述符的串口接收数据，长度为len，存入str，timeout 为超时时间  
 *-----------------------------------------------------------------------------*/    
int serial_read(int fd, char *str, unsigned int len, unsigned int timeout)    
{    
    fd_set rfds;    
    struct timeval tv;    
    int ret;                                //每次读的结果     
    int sret;                               //select监控结果     
    int readlen = 0;                        //实际读到的字节数     
    char * ptr;    
    
    ptr = str;                          //读指针，每次移动，因为实际读出的长度和传入参数可能存在差异     
    
    FD_ZERO(&rfds);                     //清除文件描述符集合     
    FD_SET(fd,&rfds);                   //将fd加入fds文件描述符，以待下面用select方法监听     
    
    /*传入的timeout是ms级别的单位，这里需要转换为struct timeval 结构的*/    
    tv.tv_sec  = timeout / 1000;    
    tv.tv_usec = (timeout%1000)*1000;    
    
    /*防止读数据长度超过缓冲区*/    
    //if(sizeof(&str) < len)     
    //  len = sizeof(str);     
    
    
    /*开始读*/    
    while(readlen < len)    
    {    
        sret = select(fd+1,&rfds,NULL,NULL,&tv);        //检测串口是否可读     
    
        if(sret == -1)                              //检测失败     
        {    
            perror("select:");    
            break;    
        }    
        else if(sret > 0)                         
        {    
            ret = read(fd,ptr,1);    
            if(ret < 0)    
            {    
                perror("read err:");    
                break;    
            }    
            else if(ret == 0)    
                break;    
    
            readlen += ret;                             //更新读的长度     
            ptr     += ret;                             //更新读的位置     
        }    
        else                                                    //超时     
        {    
          //  printf("timeout!\n");    
            break;    
        }    
    }    
    
    return readlen;    
}
int cmdat(char *at, unsigned int timeout,char *respond)
{
    int fd;
    char buf[1024]; 
	char att[100];
	int slen=0;
	memset(att,0,sizeof(att));
	memset(buf,0,sizeof(buf));
	memset(respond,0,sizeof(respond));
	
    fd =  serial_init(path_usb,115200,1);
    if(fd < 0)    
    {    
        //perror("serial init err:");    
        return -1;    
    }     
	strcat(att,at);
	strcat(att,"\r");
	slen=strlen(att);
    serial_send(fd,att,slen);       
    serial_read(fd,buf,sizeof(buf),timeout);    
	//printf("%s",buf);
	if(respond != NULL)
		strcpy(respond,buf);
    close(fd);
	return 1; 
}

/* 功能：路径补全
** 返回值：void
*/
void fillpath()
{
	strcat(path_at,name_config);
	strcat(path_outfile,name_config);
	strcat(path_formfile,name_config);
	strcat(path_debuglog,name_config);
	strcat(tmp_debuglog,name_config);
	strcat(cmd_ifup,name_config);
	strcat(cmd_ifdown,name_config);
	strcat(heartbeat_file,name_config);
}

/* 功能：判断字符串是否纯数字
** 传参：str 被判断字符串
** 返回值：int
** 返回结果：0 纯数字；-1 非纯数字
*/
int isInt(char* str)
{
	int len;
	len = strlen(str);
	//printf("len:%d\n",len);
	int i=0;
	for(i;i<len-1;i++)
	{
		if(!isdigit(str[i]))
			return -1;
	}
	return 0;
}
/* 功能：获取ping状态
** 传参：ip 目标IP；ifname 指定网卡;count PING周期
** 返回值：int
** 返回结果：0 ping成功；-1 ping失败
*/
int ping_status(char *svrip,char *ifname,int count)
{
    int time=1;
    while(1)
    {
        pid_t pid;
        if((pid = vfork())<0)
        {
            printf("\nvfork error\n");
            return 0;
        }
        else if(pid == 0)
        {       
			//if(execlp("ping", "ping","-c","1",svrip, (char*)0) < 0)
			if(execlp("ping", "ping","-c","1","-w","1","-I",ifname,svrip, (char*)0) < 0)
			{
				printf("\nexeclp error\n");
				return 0;
			}
		}
        int stat;
		waitpid(pid, &stat, 0);
		if (stat == 0)
		{
			printf("ping ok\n");
			return 0;
		}
		if(time++ == count)
		{
			printf("ping error\n");
			return -1;
		}
       // sleep(1);
    }
	return 0;
}

/*循环延时*/
void func_loop()
{
	char str[50];
	memset(str,0,sizeof(str));
	sprintf(str,"While loop %d",count_loop);
	count_loop++;
	if(flag_heartbeat==1)
		heartbeat();
}

void heartbeat()
{
    char str[100];
    char buff[100];
    FILE *fp;
    
    memset(buff, 0, sizeof(buff));
    sprintf(str, "cat %s",heartbeat_file);
    cmd_recieve_str(str, buff);
    // 心跳文件处理逻辑
    if (strcmp(buff, "a") == 0) {
        // 如果是"a"，写入"b"并重置计数器
        fp = fopen(heartbeat_file, "w");
        if (fp != NULL) {
            fputs("b", fp);
            fclose(fp);
            b_count = 0; // 重置计数器
        }
    } 
    else if (strcmp(buff, "b") == 0) {
        // 如果是"b"，增加计数器
        b_count++;
        
        // 如果连续检测到"b"超过最大次数，重启脚本
        if (b_count >= MAX_B_COUNT) {
            // 重启脚本的逻辑
			if(strcmp(name_config,"SIM2")==0)
			{
				detectProcessByName("online_check 1");
				// system("killall -9 your_script_name"); // 先结束原有进程
				system("/etc/slkapp/online_check 1 &"); // 重新启动脚本
			}
			if(strcmp(name_config,"SIM2")==0)
			{
				detectProcessByName("online_check 2");
				// system("killall -9 your_script_name"); // 先结束原有进程
				system("/etc/slkapp/online_check 2 &"); // 重新启动脚本
			}
			if(strcmp(name_config,"SIM")==0)
			{
				detectProcessByName("online_check");
				// system("killall -9 your_script_name"); // 先结束原有进程
				system("/etc/slkapp/online_check &"); // 重新启动脚本
			}
            b_count = 0; // 重置计数器
        }
    }
    else {
        // 其他情况重置计数器
        b_count = 0;
    }
}

/* 功能：获取子字符串数量
** 传参：father 父字符串child 子字符串
** 返回值：int
** 返回结果：子字符串出现次数
*/
int countstr(char *father,char *child)
{
	int count=0;
	int len_father=strlen(father);
	int len_child=strlen(child);
	while(*father != '\0')
	{
		if(strncmp(father,child,len_child)==0)
		{
			count=count+1;
			father=father+len_child;
		}else{
			father++;
		}
	}
	return count;
}
/*分割字符串*/
int separate_string2array(char *father,char *child,unsigned int num,unsigned int size,char *respond)
{
	char str[1024]={0};
	memset(&str,0x0,sizeof(str));
	memcpy(str,father,strlen(father)); 
	char *token=NULL,*token_saveptr;
	unsigned int count=0;
	token=strtok_r(str,child,&token_saveptr);
	while(token!=NULL)
	{
		memcpy(respond+count*size,token,size);
		if(++count>=num)
			break;
		token=strtok_r(NULL,child,&token_saveptr);
	}
	
	return count;
}
/*将所有ttyUSB排序*/
void sort_ttyUSB(char array[][3],int num)
{
	int i,j;
	int tmp_f;
	int tmp_s;
	for(j=0;j<num-1;j++)
	{
		for(i=0;i<num-j-1;i++)
		{
			tmp_f=atoi(array[i]);
			tmp_s=atoi(array[i+1]);
			if(tmp_f>tmp_s)
			{
				sprintf(array[i],"%d",tmp_s);
				sprintf(array[i+1],"%d",tmp_f);
			}
		}
	}
}

bool is_device_in_list(const char *device, const char *list) {
    char device_name[32];  // 存储设备名（如 "ttyUSB0"）
    char list_copy[256];   // 用于处理列表的副本
    char *token;
    
    // 从完整路径中提取设备名（如从 "/dev/ttyUSB0" 提取 "ttyUSB0"）
    const char *slash = strrchr(device, '/');
    if (slash) {
        strcpy(device_name, slash + 1);
    } else {
        strcpy(device_name, device);
    }
    
    // 创建列表的副本以便处理（strtok会修改原始字符串）
    strncpy(list_copy, list, sizeof(list_copy) - 1);
    list_copy[sizeof(list_copy) - 1] = '\0';
    
    // 使用strtok分割列表字符串
    token = strtok(list_copy, "ttyUSB");
    while (token != NULL) {
        char current_device[32];
        snprintf(current_device, sizeof(current_device), "ttyUSB%s", token);
        
        if (strcmp(device_name, current_device) == 0) {
            return true;
        }
        
        token = strtok(NULL, "ttyUSB");
    }
    
    return false;
}

/*检验正确的ttyUSB*/
int check_ttyUSB(char *buff)
{
	char cmd[1024];
	char tmp_buff[200];
	memset(cmd,0,sizeof(cmd));
	memset(tmp_buff,0,sizeof(tmp_buff));
	sprintf(cmd,"uci -q get cellular.%s.ttyUSB",name_config);
	cmd_recieve_str(cmd,tmp_buff);
	//printf("%s",tmp_buff);
	if(strlen(tmp_buff)>0 && is_device_in_list(tmp_buff,buff))
	{
		int k;
		memset(path_usb,0,sizeof(path_usb));
		sprintf(path_usb,"%s",tmp_buff);
		for(k=0;k<3;k++)
		{
			memset(cmd,0,sizeof(cmd));
			sprintf(cmd,"cmdat %s AT",path_usb);
			printf("%s\n",cmd);
			memset(tmp_buff,0,sizeof(tmp_buff));
			cmd_recieve_str(cmd,tmp_buff);
			if(strstr(tmp_buff,"OK"))
			{
				memset(cmd,0,sizeof(cmd));
				//uci_cellular("ttyUSB",path_usb);
				return 0;
			}
		}
	}
	//Debug(buff,"daemon.debug");
	int i=countstr(buff,"ttyUSB");
	char array_ttyUSB[i][3];
	if(separate_string2array(buff,"ttyUSB",i,3,(char *)&array_ttyUSB) != i)
	{
		printf("format ttyUSB error");
		//return -1;
	}
	int j,count_out=0;
	sort_ttyUSB(array_ttyUSB,i);
	memset(tmp_buff,0,sizeof(tmp_buff));
	while(strstr(tmp_buff,"timeout") || !strstr(tmp_buff,"OK"))
	{
		for(j=0;j<i;j++)
		{
			/* if(strstr(array_ttyUSB[j],"0") || strstr(array_ttyUSB[j],"1"))
				continue; */
			memset(path_usb,0,sizeof(path_usb));
			sprintf(path_usb,"/dev/ttyUSB%s",array_ttyUSB[j]);
			int count;
			for(count=0;count<2;count++)
			{
				memset(cmd,0,sizeof(cmd));
				sprintf(cmd,"cmdat %s AT",path_usb);
				printf("%s\n",cmd);
				memset(tmp_buff,0,sizeof(tmp_buff));
				cmd_recieve_str(cmd,tmp_buff);
				if (strstr(tmp_buff,"timeout"))
				{
					printf("timeout\n");
					continue;
				}
				else if(strstr(tmp_buff,"OK"))
				{
					memset(cmd,0,sizeof(cmd));
					sprintf(cmd,"uci -q set cellular.%s.ttyUSB='%s'",name_config,path_usb);
					system(cmd);
					system("uci commit cellular");
					//uci_cellular("ttyUSB",path_usb);
					return 0;
				}
			}
		}
		if(count_out==5)
		{
			memset(path_usb,0,sizeof(path_usb));
			Debug("not found valid ttyUSBs, exit","daemon.error");
			exit(1);
		}
		count_out++;
	}
	return 0;
}


char *replacestr(char *sdev,char *strs,char *strr)
{
    char *p;
    int lens = strlen(strs),lenr;
    int len;
    int slen = strlen(sdev);
    if(strr == NULL) lenr = 0;
    else lenr = strlen(strr);

    len    = lenr - lens;
    while((p = strstr(sdev,strs))!=NULL){
        //printf("%d - %s\n",slen,sdev);
        if(len > 0) {    //替换的文字比原来的长，则要扩堆。 比如 用"UUU" 替换"ui"。以防原来空间不够
            sdev = (char *)realloc(sdev,slen+len);
            p = strstr(sdev,strs);
            memmove(p+lenr,p+lens,strlen(p+lens));
            memcpy(p,strr,lenr);
        } else if(len == 0) {
            memcpy(p,strr,lenr);       
        } else {
            memmove(p+lenr,p+lens,strlen(p+lens));
            if(lenr!=0)    memcpy(p,strr,lenr);       
        }
        slen +=len;
    }
    *(sdev+slen) = 0;
    return sdev;
}

void check_board()
{
	char cmd[1024];
	char tmp_buff[1024];
	memset(cmd,0,sizeof(cmd));
	memset(tmp_buff,0,sizeof(tmp_buff));
	sprintf(cmd,"ubus call system board | grep model | awk -F ' ' '{print$NF}'");
	cmd_recieve_str(cmd,tmp_buff);
	if(strstr(tmp_buff,"AP-CP01-C1"))
	{
		memset(cmd,0,sizeof(cmd));
		int is8th=cmd_recieve_int("slktool /dev/mmcblk0p25 undefined1 | grep 8th | wc -l");
		if(is8th==1)
			sprintf(board_name,"R680-8TH");
		else
			sprintf(board_name,"R680");
	}
	else if(strstr(tmp_buff,"AP-CP01-C2"))
	{
		sprintf(board_name,"R690");
	}
	else if(strstr(tmp_buff,"AP-CP01-C3"))
	{
		sprintf(board_name,"RT990");
	}
	else if(strstr(tmp_buff,"R660"))
	{
		sprintf(board_name,"R660");
	}
	else if(strstr(tmp_buff,"R650"))
	{
		sprintf(board_name,"R650");
	}
	else if(strstr(tmp_buff,"R620"))
	{
		sprintf(board_name,"R620");
	}
	else if(strstr(tmp_buff,"R4008"))
	{
		sprintf(board_name,"R4008");
	}
	else
	{
		sprintf(board_name,"others");
	}
}


void uci_cellular(char *type,char *info)
{
	char cmd[500];
	memset(cmd,0,sizeof(cmd));
	sprintf(cmd,"sed -i '/%s.%s/d' /tmp/state/cellular", name_config, type);
	system(cmd);
	memset(cmd,0,sizeof(cmd));
	if(strstr(info,type))
		sprintf(cmd,"uci -P /var/state set cellular.%s.%s=\"%s\"",name_config,type,info);
	else
		sprintf(cmd,"uci -P /var/state set cellular.%s.%s=\"+%s: %s\"",name_config,type,type,info);
	system(cmd);
}

void uci_cellular_int(char *type,int info)
{
	char cmd[500];
	memset(cmd,0,sizeof(cmd));
	sprintf(cmd,"sed -i '/%s.%s/d' /tmp/state/cellular", name_config, type);
	system(cmd);
	memset(cmd,0,sizeof(cmd));
	sprintf(cmd,"uci -P /var/state set cellular.%s.%s=\"+%s: %d\"",name_config,type,type,info);
	system(cmd);
}

extern void uci_cellular_float(char *type,double f)
{
	char cmd[500];
	memset(cmd,0,sizeof(cmd));
	sprintf(cmd,"sed -i '/%s.%s/d' /tmp/state/cellular", name_config, type);
	system(cmd);
	memset(cmd,0,sizeof(cmd));
	sprintf(cmd,"uci -P /var/state set cellular.%s.%s=\"+%s: %.1f\"",name_config,type,type,f);
	system(cmd);
}

void uci_cellular_delete(char *type)
{
	char cmd[500];
	memset(cmd,0,sizeof(cmd));
	sprintf(cmd,"sed -i '/%s.%s/d' /tmp/state/cellular", name_config, type);
	system(cmd);
	memset(cmd,0,sizeof(cmd));
	sprintf(cmd,"uci -q -P /var/state delete cellular.%s.%s",name_config,type);
	system(cmd);
}

void clear_default_cellular_info()
{
	uci_cellular("COPS"," ");
	uci_cellular("CPSI"," ");
	uci_cellular("BAND"," ");
	uci_cellular("CEREG","0,0");
	uci_cellular("CGREG","0,0");
	if(strstr(value_module,"fm650") || strstr(value_module,"fm160") || strstr(value_module,"rg520n") || strstr(value_module,"fg160") || strstr(value_module,"mt5700") || strstr(value_module,"mt5711"))
		uci_cellular("C5GREG","0,0");
	uci_cellular("ARFCN"," ");
	uci_cellular("PCI"," ");
	uci_cellular("eNB"," ");
	uci_cellular("cellID"," ");
	uci_cellular("CCID"," ");
	uci_cellular_int("SVAL",0);
	uci_cellular_int("RSRP",0);
	uci_cellular_int("RSRQ",0);
	uci_cellular_int("RSSI",0);
	uci_cellular("IMSI"," ");
	uci_cellular_int("SINR",0);
	uci_cellular("SLOT"," ");
	uci_cellular("CSQ"," ");
}

void wait_for_modem_ready()
{
	char cmd[50];
	while(1)
	{
		memset(cmd,0,sizeof(cmd));
		sprintf(cmd,"ls /dev/ | grep ttyUSB | wc -l");
		int dev=cmd_recieve_int(cmd);
		if(dev>0)
		{
			return 0;
		}
		else
		{
			sleep(3);
		}
	}
}

bool isPCIE()
{
	char cmd[500];
	char tmp_buff[500];
	memset(cmd,0,sizeof(cmd));
	memset(tmp_buff,0,sizeof(tmp_buff));
	sprintf(cmd,"lspci");
	cmd_recieve_str(cmd,tmp_buff);
	if(strlen(tmp_buff)>0){
		return true;
		memset(cmd,0,sizeof(cmd));
		memset(tmp_buff,0,sizeof(tmp_buff));
		sprintf(cmd,"ls /dev | grep mhi_DUN");
		cmd_recieve_str(cmd,tmp_buff);
		if(strlen(tmp_buff)>0)
		{
			memset(cmd,0,sizeof(cmd));
			sprintf(cmd,"cmdat /dev/mhi_DUN 'AT' | grep OK |wc -l");
			int tmp_int_buff = cmd_recieve_int(cmd);
			if (tmp_int_buff == 1)
				return true;
		}
	}
	
	return false;
}
//通过GPIO32判断新旧板子，以确认是否使用GPIO29检测读卡
bool is_card_hotplug()
{
	if(strstr(board_name,"R680"))
	{
		char cmd[500];
		char tmp_buff[500];
		memset(cmd,0,sizeof(cmd));
		int result;
		if(!is_openwrt23_05())
		{
			sprintf(cmd,"ls /sys/class/gpio | grep gpio32 | wc -l");
			result=cmd_recieve_int(cmd);
			if(result==0)
			{
				system("echo 32 > /sys/class/gpio/export");
				system("echo in > /sys/class/gpio/gpio32/direction");
			}
			memset(cmd,0,sizeof(cmd));
			sprintf(cmd,"cat /sys/class/gpio/gpio32/value");
		}
		else //openwrt23.05 gpio0对应gpiochip432，所以是x+32
		{
			sprintf(cmd,"ls /sys/class/gpio | grep gpio464 | wc -l");
			result=cmd_recieve_int(cmd);
			if(result==0)
			{
				system("echo 464 > /sys/class/gpio/export");
				system("echo in > /sys/class/gpio/gpio464/direction");
			}
			memset(cmd,0,sizeof(cmd));
			sprintf(cmd,"cat /sys/class/gpio/gpio464/value");
		}
		result=cmd_recieve_int(cmd);
		if(result==0) //低电平是旧板子
		{
			return false;
		}
		else //高电平是新板子
		{
			return true;
		}
	}
	else
	{
		return false;
	}
}

void find_ttyUSB()
{
	char cmd[500];
	char tmp_buff[500];
	int count=0;
	while(1)
	{
		memset(cmd,0,sizeof(cmd));
		memset(tmp_buff,0,sizeof(tmp_buff));
		//printf("path_usb: %s\n",path_usb);
		sprintf(cmd,"ls /dev | grep \"$(echo %s | awk -F '/' '{print$3}')\"",path_usb);
		cmd_recieve_str(cmd,tmp_buff);
		//printf("cmd: %s\n",cmd);
		//printf("tmp_buff: %s\n",tmp_buff);
		if(strlen(tmp_buff)>0)
		{
			//printf("tmp_buff: %s\n",tmp_buff);
			if(strstr(path_usb,"ttyUSB0"))
			{
				memset(cmd,0,sizeof(cmd));
				memset(tmp_buff,0,sizeof(tmp_buff));
				sprintf(cmd,"ls /dev | grep \"$(echo %s | awk -F '/' '{print$3}')\"","ttyUSB1");
				cmd_recieve_str(cmd,tmp_buff);
				if(strlen(tmp_buff)>0)
				{
					return; //同时存在ttyUSB0和ttyUSB1视为正常
				}
			}
			else if(strstr(path_usb,"ttyUSB4"))
			{
				memset(cmd,0,sizeof(cmd));
				memset(tmp_buff,0,sizeof(tmp_buff));
				sprintf(cmd,"ls /dev | grep \"$(echo %s | awk -F '/' '{print$3}')\"","ttyUSB5");
				cmd_recieve_str(cmd,tmp_buff);
				if(strlen(tmp_buff)>0)
				{
					return; //同时存在ttyUSB4和ttyUSB5视为正常
				}
			}
			else if(strstr(path_usb,"ttyUSB5"))
			{
				memset(cmd,0,sizeof(cmd));
				memset(tmp_buff,0,sizeof(tmp_buff));
				sprintf(cmd,"ls /dev | grep \"$(echo %s | awk -F '/' '{print$3}')\"","ttyUSB6");
				cmd_recieve_str(cmd,tmp_buff);
				if(strlen(tmp_buff)>0)
				{
					return; //同时存在ttyUSB5和ttyUSB6视为正常
				}
			}
			else
			{
				return;
			}
		}
		if(count==20)
		{
			Debug("ttyUSBs are lost, wait for reset","daemon.error");
			reboot_Module();
		}
		sleep(2);
		count++;
	}
}

void reboot_Module()
{
	if(strstr(board_name,"RT990") || strstr(board_name,"R690"))
	{
		if(strstr(value_module,"rg520n"))
		{
			//pcie模块不作处理
		}
		else if(strstr(value_module,"fg160") || strcmp(name_config,"SIM2")==0)
		{
			//pcie模块不作处理
		}
		else
		{
			system("echo 0 > /sys/class/leds/pwr_usb/brightness");
			sleep(1);
			system("echo 1 > /sys/class/leds/pwr_usb/brightness");
		}
	}
	else if(strstr(board_name,"R650"))
	{
		system("echo 0 > /sys/class/gpio/m2_reset/value");
		sleep(1);
		system("echo 1 > /sys/class/gpio/m2_reset/value");
		system("echo 1 > /sys/class/gpio/m2_pwr_en/value");
		sleep(1);
		system("echo 0 > /sys/class/gpio/m2_pwr_en/value");
	}
	else if(strstr(board_name,"E940"))
	{
		
	}
	else if(strstr(board_name,"R680"))
	{
		system("echo 0 > /sys/class/leds/pwr_en1/brightness");
		sleep(1);
		system("echo 1 > /sys/class/leds/pwr_en1/brightness");
	}
	else if(strstr(board_name,"R620"))
	{
		system("echo 0 > /sys/class/gpio/m2_reset/value");
		sleep(1);
		system("echo 1 > /sys/class/gpio/m2_reset/value");
		system("echo 0 > /sys/class/gpio/m2_pwr_en/value");
		sleep(1);
		system("echo 1 > /sys/class/gpio/m2_pwr_en/value");
	}
	else if(strstr(board_name,"R660"))
	{
		system("echo 0 > /sys/class/gpio/m2_reset/value");
		sleep(1);
		system("echo 1 > /sys/class/gpio/m2_reset/value");
		system("echo 0 > /sys/class/gpio/m2_pwr_en/value");
		sleep(1);
		system("echo 1 > /sys/class/gpio/m2_pwr_en/value");
	}
	else if(strstr(board_name,"R4008") || strstr(board_name,"R602"))
	{
		system("echo 1 > /sys/class/gpio/m2_pwr_en/value");
		sleep(1);
		system("echo 0 > /sys/class/gpio/m2_pwr_en/value");
	}
}

bool check_pcie_ko()
{
	char cmd[1024];
	char tmp_buff[1024];
	char *end;
	//Debug("check pcie.ko","daemon.info");
	int rec;
	while(1)
	{
		memset(cmd,0,sizeof(cmd));
		sprintf(cmd,"lsmod | grep 'fibo_mhi' | wc -l");
		rec=cmd_recieve_int(cmd);
		//Debug("lsmod fibo","daemon.info");
		if(rec==0)
		{
			//Debug("lsmod fibo_mhi.ko failed","daemon.info");
			memset(cmd,0,sizeof(cmd));
			sprintf(cmd,"ls /lib/modules/`uname -r`/ | grep 'fibo_mhi.ko' | wc -l");
			rec=cmd_recieve_int(cmd);
			if(rec==1)
			{	
				//Debug("ls fibo_mhi.ko ok","daemon.info");
				while(1)
				{
					memset(cmd,0,sizeof(cmd));
					memset(tmp_buff,0,sizeof(tmp_buff));
					sprintf(cmd,"cat /proc/uptime | awk -F. '{print $1}'");
					cmd_recieve_str(cmd,tmp_buff);
					long int time=strtol(tmp_buff, &end, 10);
					if(time>26)
					{
						break;
					}
					else
					{
						sleep(1);
					}
				}
				system("insmod fibo_mhi.ko");
				break;
			}
			else
			{
				//Debug("ls fibo_mhi.ko failed","daemon.info");
				sleep(1);
			}
		}
		else
		{
			//Debug("lsmod fibo_mhi.ko ok","daemon.info");
			return true;
		}
	}
	memset(cmd,0,sizeof(cmd));
	sprintf(cmd,"lsmod | grep 'fibo_mhi' | wc -l");
	rec=cmd_recieve_int(cmd);
	if(rec==1)
		return true;
	else
		return false;
}

/*time_t parse_modem_time(const char *time_str) {
    struct tm tm_time = {0};
    
    // 解析格式: "YY/MM/DD,HH:MM:SS+ZZ"
    if (sscanf(time_str, "%02d/%02d/%02d,%02d:%02d:%02d", 
               &tm_time.tm_year, &tm_time.tm_mon, &tm_time.tm_mday,
               &tm_time.tm_hour, &tm_time.tm_min, &tm_time.tm_sec) != 6) {
        return (time_t)-1;
    }
    
    // 调整年份（2025年，25表示2025）
    tm_time.tm_year += 100;  // 2025 - 1900 = 125
    tm_time.tm_mon -= 1;     // 月份从0开始
    
    // 忽略时区偏移（+32），直接返回UTC时间
    return mktime(&tm_time);
}*/

time_t parse_modem_time(const char *time_str) {
    struct tm tm_time = {0};
    int tz_offset_quarters = 0;
    char tz_sign = '+';
    
    // 解析格式: "YY/MM/DD,HH:MM:SS±ZZ"
    if (sscanf(time_str, "%02d/%02d/%02d,%02d:%02d:%02d%c%02d", 
               &tm_time.tm_year, &tm_time.tm_mon, &tm_time.tm_mday,
               &tm_time.tm_hour, &tm_time.tm_min, &tm_time.tm_sec,
               &tz_sign, &tz_offset_quarters) < 6) {
        return (time_t)-1;
    }
    
    // 调整年份（25表示2025）
    tm_time.tm_year += 100;  // 2025 - 1900 = 125
    tm_time.tm_mon -= 1;     // 月份从0开始
    
    // 计算时区偏移量（以秒为单位）
    int tz_offset_minutes = tz_offset_quarters * 15;
    int tz_offset_seconds = tz_offset_minutes * 60;
    
    // 如果是负偏移量，调整符号
    if (tz_sign == '-') {
        tz_offset_seconds = -tz_offset_seconds;
    }
    
    // 将本地时间转换为time_t（假设输入时间是本地时间）
    time_t local_time = mktime(&tm_time);
    if (local_time == (time_t)-1) {
        return (time_t)-1;
    }
    
    // 减去时区偏移量得到UTC时间
    // 因为mktime()假设输入是本地时间，所以需要减去偏移量来得到UTC
    time_t utc_time = local_time + tz_offset_seconds;
    
    return utc_time;
}

int set_system_time(time_t new_time) {
    struct timeval tv = {
        .tv_sec = new_time,
        .tv_usec = 0
    };
    
    return settimeofday(&tv, NULL);
}

int set_time(char *modem_time_str)
{
	//char modem_time_str[64] = {0};
    time_t system_time, modem_time;
    
    // 获取系统时间
    system_time = time(NULL);
    if (system_time == (time_t)-1) {
        //Debug("获取系统时间失败","daemon.info");
        return 1;
    }
	//Debug(modem_time_str,"module.info");
	modem_time = parse_modem_time(modem_time_str);
    if (modem_time == (time_t)-1) {
        //Debug("解析模块时间失败","daemon.info");
        return 1;
    }
	char str[50];
	memset(str,0,sizeof(str));
	sprintf(str,"%ld",system_time);
	//Debug(str,"system.time");
	memset(str,0,sizeof(str));
	sprintf(str,"%ld",modem_time);
    //Debug(str,"module.time");
	if (modem_time > system_time) {
		char dd[1024];
		memset(dd,0,sizeof(dd));
		sprintf(dd,"get module time %s, updating system time",modem_time_str);
        Debug(dd,"daemon.info");
        
        if (set_system_time(modem_time) != 0) {
            //Debug("设置系统时间失败","daemon.info");
            return 1;
        }
        
        //Debug("系统时间已更新","daemon.info");
    } else {
        //Debug("系统时间已是最新，无需更新","daemon.info");
    }
    
    return 0;
}

bool is_openwrt23_05(void)
{
	char cmd[1024];
	char tmp_buff[1024];
	memset(cmd,0,sizeof(cmd));
	memset(tmp_buff,0,sizeof(tmp_buff));
	sprintf(cmd,"ubus call system board | grep version | awk -F ' ' '{print$NF}'");
	cmd_recieve_str(cmd,tmp_buff);
	if(strstr(tmp_buff,"23.05"))
		return true;
	return false;
	
}