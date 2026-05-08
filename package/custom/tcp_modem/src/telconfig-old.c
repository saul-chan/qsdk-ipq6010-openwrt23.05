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
#include <error.h>
#include <netinet/ip.h>
#include <sys/wait.h>

#define YES 1
#define NO 0

typedef void sigfunc(int);
int get_ports(void);
int get_devices(char *buff);
void get_portMesg(char *buff);
int get_workMod(unsigned char *buff);
int get_syslog(void *buff);
int get_checkPort(void);
void get_connect(int port);
void set_port1(char *buf);
void set_port2(char *buf);
void set_port4(char *buf);
void set_port5(char *buf);
void set_port6(char *buf);
void set_IP(unsigned char *buf);
void set_hostname(unsigned char *buf);
long hexToDec(char *source);
void set_syslog(unsigned char *buf);
int hextoint(char *buf,int);
int getIndexOfSingns(char ch);
void fun(int n);
int8_t CheckSum(uint8_t *Buf,uint8_t Len);
int select_number(char *buff,int *tmp);
char Four[300];
char Mac[40];
char Name[300];
char TimeModel[2][50];
char **explode(char sep, const char *str, int *size);
uint8_t char_to_hex(uint8_t *ch);
char* itoa(int num,char* str,int radix);
int prots,ports,status;
static int counts = 0;

typedef struct node{
	int fd;
	struct sockaddr_in client;
}node_t;

void *client_recv(void *arg)
{
	int ret,i,j,o,fd,a[1024],num,num1,aa1[30],k1;
	unsigned char buf[1024],kk,send[1024];
	int count = 0;
	node_t *client_t=(node_t *)arg;
	fd = client_t->fd;
	struct sockaddr_in client = client_t->client;
	unsigned char tmp0[100],tmp1;
	char tmp2[100];
	unsigned char Splicing[1024];
	int num_first,num_second;
	unsigned char *str;
	char Mesg[1024],Mesg2[1024],t1[30],t2[30],t3[30];

    while(1)
    {
        ret=read(fd,buf,sizeof(buf));
        if(ret>0)
        {
           buf[ret]=0;

           hexToDec(buf);
           CheckSum(buf,ret);
           if(buf[0] == 'h' && buf[1]== 0x00 && buf[2] == 0x01 && buf[ret-1] == 0x16)
           {

                  switch(buf[3]){
                      case 0x00:ports=get_ports();
                                kk = 0x80;
                                break;
                      case 0x08:
                                get_devices(Mesg);
                                kk = 0x88;
                                num =  select_number(Four,a);
                                break;
                      case 0x09:
                                //get_portMesg(Mesg);
                                prots = buf[6];
                                kk = 0x89;
                                break;
                      case 0x0B:get_workMod(buf);
                                kk = 0x8B;
                                break;
                      case 0x0C:get_syslog(Mesg);break;
                      case 0x0D:
								prots = buf[6];
								kk = 0x8D;
								break;
					  case 0x0E:
								prots = buf[6];
								//get_connect(prots);
								kk = 0x8E;
								break;
                      // 设置
                      case 0x01:
                                set_port1(buf);
                                kk = 0x81;
                                break;
                      case 0x02:set_port2(buf);
                                kk = 0x82;
                                break;
                      case 0x03:set_IP(buf);
                                kk = 0x83;
                                break;
                      case 0x04:set_port4(buf);kk = 0x84;break;
                      case 0x05:set_port5(buf);kk = 0x85;break;
                      case 0x06:set_port6(buf);kk = 0x86;break;
                      case 0x07:set_syslog(buf);break;
                      case 0x0A:set_hostname(buf);
                                kk = 0x8A;
                                break;
                      default :break;
                  }

           }

        }
        else{
           break;
        }
           // char *pp="server";
            int cc,sum;
           // char *aa = "BD";
            FILE *fp;
            int number1 = 32;
            char bufs[512] ;
            //int number2 = -123456;
            char string[16] = {0};
            char s[11][100];
            char *sp;
            int Baud;
            int checks;
            int stops;
            int datas;
			int connect;
           // char mac[20] = "BD:CD:0D:00:0A:10";
            send[count++]=0x68;
            send[count++]=0x00;
            send[count++]=0x01;
            send[count++]=kk;
            count+=2;
            switch(kk){
                case 0x80:
                          itoa(ports,string,10);
                          sscanf("32", "0x%x", bufs);
                          count=6;
                          send[count++]=ports;
                          break;
                case 0x88:
                          if(status == 0x01)
                          {
                               send[count++]=status;
                          }
                          else
                          {
                          send[count++]=0x00;
                          sprintf((char*)send+count,"%s",Name);
                          count+=strlen(Name);
                          str = strtok(Mac, ":");
                          i=0;
                          while( str != NULL ) {
                              strcat(tmp0,str);
                              i++;
                              str = strtok(NULL, ":");  
                          }
                          // printf("i:%s\n",str);
                          for(j=0;j<i*2;j++)
                              send[count++]=tmp0[j];
                          for(i=0;i<num;i++)
                          {
                              send[count++]=a[i];
                          }
                          }
                           break;
                case 0x89:
                         prots = prots -1;
                         sprintf(s[0],"uci get nport.@port%d[0].baud",prots);
                         if((fp=popen(s[0],"r"))==NULL)
                         {
                               perror("popen error");
                              //return -2;
                         }
                        else
                        {
                             memset(s[0],0,sizeof(s[0]));
                             while(fgets(s[0],sizeof(s[0]),fp)!=NULL)
                            {
                                //printf("msg:%s",remsg);
                            }
                               s[0][strlen(s[0])-1]='\0';
                         }
                         fclose(fp);
                         sprintf(s[1],"uci get nport.@port%d[0].parity",prots);
                         if((fp=popen(s[1],"r"))==NULL)
                         {
                               perror("popen error");
                              //return -2;
                         }
                        else
                        {
                             memset(s[1],0,sizeof(s[1]));
                             while(fgets(s[1],sizeof(s[1]),fp)!=NULL)
                            {
                                //printf("msg:%s",remsg);
                            }
                               s[1][strlen(s[1])-1]='\0';
                         }
                          fclose(fp);
                         sprintf(s[2],"uci get nport.@port%d[0].stop",prots);
                         if((fp=popen(s[2],"r"))==NULL)
                         {
                               perror("popen error");
                              //return -2;
                         }
                        else
                        {
                             memset(s[2],0,sizeof(s[2]));
                             while(fgets(s[2],sizeof(s[2]),fp)!=NULL)
                            {
                                //printf("msg:%s",remsg);
                            }
                               s[2][strlen(s[2])-1]='\0';
                        }
                         fclose(fp);
                         sprintf(s[3],"uci get nport.@port%d[0].data",prots);
                         if((fp=popen(s[3],"r"))==NULL)
                         {
                               perror("popen error");
                              //return -2;
                         }
                        else
                        {
                            memset(s[3],0,sizeof(s[3]));
                            while(fgets(s[3],sizeof(s[3]),fp)!=NULL)
                            {
                                //printf("msg:%s",remsg);
                            }
                            s[3][strlen(s[3])-1]='\0';
                        }
                         fclose(fp);
                        if(s[0] != NULL || s[1] !=NULL || s[2] != NULL || s[3] != NULL)
                        {
                            send[count++]=0x00;
                        }
						 
                        sp = strtok(s[1],"\n");
                        Baud = strcmp(s[0],"300");
                        if(Baud == 0)
                            send[count++]=0x01;    
                          
                        Baud = strcmp(s[0],"600");
                        if(Baud == 0)
                            send[count++]=0x02;  
                          
                        Baud = strcmp(s[0],"1200");
                        if(Baud == 0)
                            send[count++]=0x03;    

                        Baud = strcmp(s[0],"2400");
                        if(Baud == 0)
                            send[count++]=0x04;    

                        Baud = strcmp(s[0],"4800");
                        if(Baud == 0)
                            send[count++]=0x05;    

                        Baud = strcmp(s[0],"9600");
                        if(Baud == 0)
                            send[count++]=0x06;     
                        
                        Baud = strcmp(s[0],"19200");
                        if(Baud == 0)
                            send[count++]=0x07;    
                        
                        Baud = strcmp(s[0],"38400");
                        if(Baud == 0)
                            send[count++]=0x08;    
                        
                        Baud = strcmp(s[0],"57600");
                            if(Baud == 0)
                                send[count++]=0x09; 
   
                        Baud = strcmp(s[0],"115200");
                            if(Baud == 0)
                                send[count++]=0x0A;    
                          
                        //NONE 校验位    1STOPBIT 8DATABITS
                        Baud = strcmp(sp,"NONE");
                            if(Baud == 0)
                                send[count++]=0x00;   
                                                    
                        Baud = strcmp(sp,"EVEN");
                            if(Baud == 0)
                                send[count++]=0x01;    
                          
                        Baud = strcmp(sp,"ODD");
                            if(Baud == 0)
                                send[count++]=0x02;    
                          
                        Baud = strcmp(sp,"MARK");
                            if(Baud == 0)
                                send[count++]=0x03;   
 
                        Baud = strcmp(sp,"SPACE");
                            if(Baud == 0)
                                send[count++]=0x04;    
                          
                        // 停止位
                        Baud = strcmp(s[2],"1STOPBIT");
                            if(Baud == 0)
                                send[count++]=0x01;    
                        Baud = strcmp(s[2],"2STOPBITS");
                            if(Baud == 0)
                                send[count++]=0x02;

                         // 数据位
                        Baud = strcmp(s[3],"5DATABITS");
                            if(Baud == 0)
                                send[count++]=0x01;    
                        Baud = strcmp(s[3],"6DATABITS");
                            if(Baud == 0)
                                send[count++]=0x02;    

                        Baud = strcmp(s[3],"7DATABITS");
                            if(Baud == 0)
                                send[count++]=0x03;    
                        Baud = strcmp(s[3],"8DATABITS");
                            if(Baud == 0)
                                send[count++]=0x04;    
                        break;
                case 0x8B:
                        if(status == 0x01)
                        {
                            send[count++] = status;
                            break;
                        }
                        send[count++] = status;
                        Baud = strcmp(TimeModel[0],"tcpserver");
                            if(Baud == 0)
                                 send[count++]=0x01;    
                        Baud = strcmp(TimeModel[0],"udpserver");
                            if(Baud == 0)
                                send[count++]=0x02;    
                        Baud = strcmp(TimeModel[0],"udpclient");
                            if(Baud == 0)
                                send[count++]=0x03;  
                        Baud = strcmp(TimeModel[0],"modbustcp");
                            if(Baud == 0)
                                send[count++]=0x04;
                        if(TimeModel[1] != NULL)
                        {
                            sum = hextoint(TimeModel[1],3);
                            send[count++]=sum/256;
                            send[count++]=sum%256;
                        }
                        break;  
                case 0x81:send[count++]=status;break;
                case 0x82:send[count++]=status;break;
                case 0x83:send[count++]=status;break; 
                case 0x84:send[count++]=status;break; 
                case 0x85:send[count++]=status;break; 
                case 0x86:send[count++]=status;break; 
                case 0x8A:send[count++]=0x00;break;
				case 0x8D:
						  prots = prots - 1;
						  printf("%d",prots);
                          sprintf(s[4],"uci get nport.@port%d[0].localport",prots);
                          if((fp=popen(s[4],"r"))==NULL)
                          {
								perror("popen error");
                              //return -2;
                          }
                          else
                          {
                               memset(s[4],0,sizeof(s[4]));
                               while(fgets(s[4],sizeof(s[4]),fp)!=NULL)
                               {
                                  //printf("msg:%s",remsg);
                               }
                               s[4][strlen(s[4])-1]='\0';
                          }
                          fclose(fp);
						  sprintf(s[5],"netstat -nat | grep %s | grep ESTABLISHED |awk '{print $5}'|awk -F: '{print $1}'",s[4]);
						  if((fp=popen(s[5],"r"))==NULL)
                          {
								perror("popen error");
                              //return -2;
                          }
                          else
                          {
                               memset(s[5],0,sizeof(s[5]));
                               while(fgets(s[5],sizeof(s[5]),fp)!=NULL)
                               {
                                  //printf("msg:%s",remsg);
                               }
                               s[5][strlen(s[5])-1]='\0';
                          }
                          fclose(fp);
						  printf("LENs[5]=%d\n",strlen(s[5]));
						  send[count++]=0x00;
						  if((strlen(s[5]))!=0)
						  {
								  //端口下有连接
								  send[count++]=0x00;
								  num1 = select_number(s[5],aa1);
								  for(k1=0;k1<num1;k1++)
								  {
									  send[count++]=aa1[k1];
								  }
						  } 
						  else
						  {
							  printf("dfsfjd\n");
							  send[count++]=0x01;  //端口下无连接
						  }
						  printf("555\n");
						  break;
					case 0x8E:
						  prots = prots - 1;
						  printf("port=%d\n",prots);
						  sprintf(s[7],"uci get nport.@port%d[0].localport",prots);
                          if((fp=popen(s[7],"r"))==NULL)
                          {
								perror("popen error");
                              //return -2;
                          }
                          else
                          {
                               memset(s[7],0,sizeof(s[7]));
                               while(fgets(s[7],sizeof(s[7]),fp)!=NULL)
                               {
                                  //printf("msg:%s",remsg);
                               }
                               s[7][strlen(s[7])-1]='\0';
                          }
                          fclose(fp);
						  sprintf(s[8],"netstat -nat | grep %s | grep ESTABLISHED | awk '{print $5}'|awk -F: '{print $1}'",s[7]);
						  if((fp=popen(s[8],"r"))==NULL)
                          {
								perror("popen error");
                              //return -2;
                          }
                          else
                          {
                               memset(s[8],0,sizeof(s[8]));
                               while(fgets(s[8],sizeof(s[8]),fp)!=NULL)
                               {
                                  //printf("msg:%s",remsg);
                               }
                               s[8][strlen(s[8])-1]='\0';
                          }
                          fclose(fp);
						  printf("\nlens[8]=%d\n",strlen(s[8]));
						  send[count++]=0x00;
						  if((strlen(s[8]))!=0)
						  {
							  //端口下有连接，剔除成功
							  //printf("1111\n");s[9]是pid号
							  printf("端口号：%s\n",s[7]);
							  sprintf(s[9],"netstat -anp|grep %s | sed -n '2p' | awk '{print $7}' | awk -F '[/]' '{print $1}'",s[7]);
							  if((fp=popen(s[9],"r"))==NULL)
							  {
									perror("popen error");
									//return -2;
							  }
							  else
							  {
									memset(s[9],0,sizeof(s[9]));
									while(fgets(s[9],sizeof(s[9]),fp)!=NULL)
									{
										//printf("msg:%s",remsg);
									}
									s[9][strlen(s[9])-1]='\0';
							  }
                              fclose(fp);
							  printf("pid:%s\n",s[9]);
							  sprintf(t2,"kill -9 %s",s[9]);
							  system(t2);
							  sprintf(t1,"nice -n 0 ser2net -c /etc/usb/port%d",prots);
							  system(t1);
							  sprintf(s[10],"lsof -i:%s",s[7]);
							  if((fp=popen(s[10],"r"))==NULL)
							  {
									perror("popen error");
									//return -2;
							  }
							  else
							  {
									memset(s[10],0,sizeof(s[10]));
									while(fgets(s[10],sizeof(s[10]),fp)!=NULL)
									{
										//printf("msg:%s",remsg);
									}
									s[10][strlen(s[10])-1]='\0';
							  }
							  if(s[10]=="")
							  {
								  sprintf(t1,"nice -n 0 ser2net -c /etc/usb/port%d",prots);
								  system(t1);
							  }  
							  send[count++]=0x00;
						  }
						  else
						  {
							  printf("fdfd\n");
							  //端口下无连接，剔除失败
							  sprintf(t3,"/etc/setusb %d",ports);
							  system(t3);
							  send[count++]=0x01;
						  }
						  printf("666\n");
						  break;
            }

            //send[count++]=cc;
            //sprintf((char*)send+count,"%s",pp);
            //count+=strlen(pp);
            
            if((count-7) >= 256)
            {
               num_first=(count-6)/256;
               num_second=(count-6)%256;
            }
            else
            {
               num_first=0;
               num_second=count-6;
            }
            //sprintf((char*)send+4,"%04x",count-7);
            send[4]=num_first;
            send[5]=num_second;            
           // sprintf((char*)send+4,"%02x",count-7);

            //校验和
            send[count++]=CheckSum(send,count);

            send[count++]=0x16;
            write(fd,(char *)send,count);
            count=0;
            memset(Name,0,sizeof(Name));
            memset(Mac,0,sizeof(Mac));
            memset(a,0,sizeof(a));
            memset(send,0,sizeof(send));
            memset(Four,0,sizeof(Four));
            if(kk == 0x83){
                sprintf(tmp2,"/etc/init.d/network restart");
                system(tmp2);
            }else if(kk ==0x86)
            {
                sprintf(tmp2,"uci get nport.@port%d[0].restart",num);
                system(tmp2);
            }
    }
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

int select_number(char *buff,int *tmp)
{
    int i,count;
    int p,q,r;
    r=0;
    count=0;
    while(1)
    {
        while(buff[r]&&(buff[r]<'0'||buff[r]>'9'))
           r++;
        if(buff[r])
        {
           p=r;
           q=r+1;
           tmp[count] = buff[r]-'0';
           while(buff[q]>='0'&&buff[q]<='9')
           {
              tmp[count] = 10*tmp[count]+(buff[q]-'0');
              q++;
           }
           r=q;
           count++;
        }
        else
           break;
    }
    return count;
}

int8_t CheckSum(uint8_t *Buf,uint8_t Len)
{
  uint8_t i =0;
  uint8_t sum =0;
  uint8_t checksum =0;
  for(i=0; i<Len; i++)
  {
	  sum += *Buf++;
  }
  checksum = sum &0xff;
  return checksum; 
}

void fun(int n)
{
	int a1,count=0,j;//count 用于角标的计数，j 控制 for 循环 
	int a[100];  
	while(n!=0) 
	{ 
		a1=n;
 		n=n/16;
  		a[count]=a1%16; 
  		count++; 
    } 
  	for(j=count-1;j>=0;j--)		
	{ 
  		if(a[j]>9&&a[j]<16) 
        {
  		    //printf("%d",(a[j]-10+'A') * (-1)); 
  		}
        else
        { 
  	       //printf("%d",a[j]); 
        }
	} 
	//return 0;
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

void set_port2(char *buf)
{

    ports=get_ports();
    struct termios *opt;
    int speed;
    char *check,*stop;
    char *data;
    char *model;
    char *types;
    char *control,tmp[1024];
    int com,fd;
    com=buf[6]-1;
    switch(buf[7]){
         case 0x00:break;
         case 0x01:speed=300;break;
         case 0x02:speed=600;break;
         case 0x03:speed=1200;break;
         case 0x04:speed=2400;break;
         case 0x05:speed=4800;break;
         case 0x06:speed=9600;break;
         case 0x07:speed=19200;break;
         case 0x08:speed=38400;break;
         case 0x09:speed=57600;break;
         case 0x0A:speed=115200;break;
         default:speed=9600;break;
    }

    switch(buf[8]){
         case 0x00:break;
         case 0x01:data = "5DATABITS";break;
         case 0x02:data = "6DATABITS";break;
         case 0x03:data = "7DATABITS";break;
         case 0x04:data = "8DATABITS";break;
         default:data = "8DATABITS";break;
    }

    switch(buf[9]){
         case 0x00:break;
         case 0x01:check = "NONE";break;
         case 0x02:check = "EVEN";break;
         case 0x03:check = "ODD";break;
         case 0x04:check = "MARK";break;
         case 0x05:check = "SPACE";break;
         default:check = "NONE";break;
    }

    switch(buf[10]){
         case 0x00:break;
         case 0x01:stop = "1STOPBIT";break;
         case 0x02:stop = "2STOPBITS";break;
         default:stop = "1STOPBIT";break;
    }

    switch(buf[11]){
         case 0x00:break;
         case 0x01:model = "tcpserver";break;
         case 0x02:model = "udpserver";break;
         case 0x03:model = "udpclient";break;
         case 0x04:model = "modbustcp";break;
         default:model = "tcpserver";break;
    }

    switch(buf[12]){
         case 0x00:break;
         case 0x01:types = "232";break;
         case 0x02:types = "422";break;
         case 0x03:types = "485";break;
         default:types = "232";break;
    }


    switch(buf[13]){
         case 0x00:break;
         case 0x01:control = "NONE";break;
         case 0x02:control = "RTS/CTS";break;
         case 0x03:control = "XON/XOFF";break;
         case 0x04:control = "DTR/DSR";break;
         default:control = "RTS/CTS";break;
    }

     if(buf[6] == -1)
    {
        
          //  波特率
          sprintf(tmp,"uci set nport.@allport[0].baud=%d",speed);
         
          system(tmp);
          memset(tmp,0,200);

         // 数据位
          sprintf(tmp,"uci set nport.@allport[0].data=%s",data);
          
          system(tmp);
          memset(tmp,0,200);

         // 校验位
          sprintf(tmp,"uci set nport.@allport[0].parity=%s",check);
          
          system(tmp);
          memset(tmp,0,200);

         // 停止位
          sprintf(tmp,"uci set nport.@allport[0].stop=%s",stop);
         
          system(tmp);
          memset(tmp,0,200);

         // 模式状态
          sprintf(tmp,"uci set nport.@allport[0].netpro=%s",model);
          
          system(tmp);
          memset(tmp,0,200);

    }
    else if(com >= 0 && com < ports)
    {
           sprintf(tmp,"uci set nport.@port%d[0].baud=%d",com,speed);
           
           system(tmp);
           memset(tmp,0,200);

           sprintf(tmp,"uci set nport.@port%d[0].data=%s",com,data);
           system(tmp);
           memset(tmp,0,200);

           sprintf(tmp,"uci set nport.@port%d[0].parity=%s",com,check);
           system(tmp);
           memset(tmp,0,200);

           sprintf(tmp,"uci set nport.@port%d[0].stop=%s",com,stop);
           system(tmp);
           memset(tmp,0,200);

           sprintf(tmp,"uci set nport.@port%d[0].netpro=%s",com,model);
           system(tmp);
           memset(tmp,0,200);

    }
    else if(com+1>ports)
    {
            status = 0x02;
    }
    else
    {
             status = 0x01;     
    }
    
     system("uci commit");   
     status = 0x00;
     if(buf[6] == -1)
     {    
          system("/etc/init.d/allport start"); 
      }
      else
      {
          sprintf(tmp,"/etc/setusb %d",com);
          system(tmp);
      }

}
void set_port4(char *buf)
{
    long port,port2;
    int num=buf[6]-1,i;
    char model[20];
    char port_num[20],time_out[20],order[200];
    switch(buf[7]){
          case 0x01:strcpy(model,"tcpserver");break;
          case 0x02:strcpy(model,"udpserver");break;
          case 0x03:strcpy(model,"udpclient");break;
          case 0x04:strcpy(model,"modbustcp");break;
          default:break;
    }
    sprintf(port_num,"%02x%02x",buf[8],buf[9]);
    port = hexToDec(port_num);
    sprintf(time_out,"%02x%02x",buf[10],buf[11]);
    port2 = hexToDec(time_out);
    if(buf[6] == -1)
    {

            sprintf(order,"uci set nport.@allport[0].localport=%ld",port);
            system(order);
            memset(order,0,200);
          
            // 网络协议
            sprintf(order,"uci set nport.@allport[0].netpro=%s",model);
            system(order);
            memset(order,0,200);
            // 间隔时间
            sprintf(order,"uci set nport.@allport[0].timeout=%ld",port2);
            system(order);
            memset(order,0,200);
            system("uci commit");
    }
    else{
    if(port>=1001 && port<= 65535)
    {
        //  端口
        sprintf(order,"uci set nport.@port%d[0].localport=%ld",num,port);
        system(order);
        memset(order,0,200);

        // 网络协议
        sprintf(order,"uci set nport.@port%d[0].netpro=%s",num,model);
        system(order);
        memset(order,0,200);
        // 间隔时间
        sprintf(order,"uci set nport.@port%d[0].timeout=%ld",num,port2);
        system(order);
        memset(order,0,200);
    }
    else if(port <1001 || port >65535)
    {
          status = 0x02;
    }
    else
    {
           status = 0x01;
    }
    }
    system("uci commit");
    status = 0x00;

}
void set_port5(char *buf)
{
    ports=get_ports();
    int num=buf[6]-1;
    int i;
    char order[200];

    if(buf[6]==0x40)
    {
       system("reboot");
    }
    else if(buf[6] == -1)
    {
       //   >/dev/null 2>&1
       //system("/etc/init.d/ser2netd start");
       system("/etc/init.d/allport start");
    }
    else if(buf[6] > ports)
    {
         status = 0x01;
    }
    else//port restart
    {
       sprintf(order,"/etc/setusb %d",num);
       system(order);
       memset(order,0,200);
    }
    status = 0x00;

}
void set_port6(char *buf)
{
   long port,port2;
    int num=buf[6]-1,i;
    char model[20];
    char port_num[20],time_out[20],order[200];
    switch(buf[7]){
          case 0x01:strcpy(model,"tcpserver");break;
          case 0x02:strcpy(model,"udpserver");break;
          case 0x03:strcpy(model,"udpclient");break;
          case 0x04:strcpy(model,"modbustcp");break;
          default:break;
    }
    sprintf(port_num,"%02x%02x",buf[8],buf[9]);
    port = hexToDec(port_num);
    sprintf(time_out,"%02x%02x",buf[10],buf[11]);
    port2 = hexToDec(time_out);
    if(buf[6] == -1)
    {
            sprintf(order,"uci set nport.@allport[0].localport=%ld",port);
            system(order);
            memset(order,0,200);
          
            // 网络协议
            sprintf(order,"uci set nport.@allport[0].netpro=%s",model);
            system(order);
            memset(order,0,200);
            // 间隔时间
            sprintf(order,"uci set nport.@allport[0].timeout=%ld",port2);
            system(order);
            memset(order,0,200);
            system("uci commit");
    }
    else{
     if(port>=1001 && port<= 65535)
    {
        //  端口
        sprintf(order,"uci set nport.@port%d[0].localport=%ld",num,port);
        system(order);
        memset(order,0,200);

        //sprintf(time_out,"%02x%02x",buf[10],buf[11]);
        //port2 = hexToDec(time_out);
        // 网络协议
        sprintf(order,"uci set nport.@port%d[0].netpro=%s",num,model);
        system(order);
        memset(order,0,200);
        // 间隔时间
        sprintf(order,"uci set nport.@port%d[0].timeout=%ld",num,port2);
        system(order);
        memset(order,0,200);
        status = 0x00;
    }
    else if(port <1001 || port >65535)
    {
          status = 0x02;
    }
    else
    {
           status = 0x01;
    }
    }
    system("uci commit");

    if(buf[6] == -1)
    {
          system("/etc/init.d/allport start");
    }

    if(status == 0x00)
    {
         sprintf(order,"/etc/setusb %d",num);
         system(order);
         memset(order,0,200);
    }

}
void set_IP(unsigned char *buf)
{
    unsigned char tmp[20];
    char tmp2[200];
    if(buf[6] == 0x01)// DHCP 
    {
          system("uci set network.lan.proto=dhcp");
          system("uci commit");
    }
    else if(buf[6] != 0x00 && buf[6] != 0x01)
    {
         status = 0x01;    
    }
    else
    {
        // static
        system("uci set network.lan.proto=static");
        // IP
        snprintf(tmp,20,"%d.%d.%d.%d",buf[7],buf[8],buf[9],buf[10]);
        
        sprintf(tmp2,"uci set network.lan.ipaddr=%s",tmp);
        system(tmp2);
        memset(tmp2,0,200);
        // 掩码
        snprintf(tmp,20,"%d.%d.%d.%d",buf[11],buf[12],buf[13],buf[14]);
        
        sprintf(tmp2,"uci set network.lan.netmask=%s",tmp);
        system(tmp2);
        memset(tmp2,0,200);
        // 网关
        snprintf(tmp,20,"%d.%d.%d.%d",buf[15],buf[16],buf[17],buf[18]);
        
        sprintf(tmp2,"uci set network.lan.gateway=%s",tmp);
        system(tmp2);
        memset(tmp2,0,200);
        // DNS
        snprintf(tmp,20,"%d.%d.%d.%d",buf[19],buf[20],buf[21],buf[22]);
        
        sprintf(tmp2,"uci set network.lan.dns=%s",tmp);
        system(tmp2);
        memset(tmp2,0,200);
        system("uci commit");
    }
     status = 0x00;
}
void set_hostname(unsigned char *buf)
{
    int len=buf[6],i;
    char tmp[255]={0};
    char tmp2[300];
    for(i=0;i<len;i++)
    {
       tmp[i]=buf[7+i];
       
    }
    
    sprintf(tmp2,"uci set system.@system[0].hostname=%s",tmp);
    system(tmp2);
    memset(tmp2,0,200);      
    system("uci commit");
    
}
void set_syslog(unsigned char *buf)
{
;
}


int get_workMod(unsigned char *buff)
{
    int com,sum;
    FILE *fp;
    com = buff[6] -1;
    char tmp2[200],timeout[200];
    char msg_Netpro[40];
    sprintf(tmp2,"uci get nport.@port%d[0].netpro",com);
    if((fp=popen(tmp2,"r"))==NULL)
    {
        perror("popen error");
        return -2;
    }
    else
    {
        memset(msg_Netpro,0,sizeof(msg_Netpro));
        while(fgets(msg_Netpro,sizeof(msg_Netpro),fp)!=NULL)
        {
            //printf("msg:%s",remsg);
        }
        msg_Netpro[strlen(msg_Netpro)-1]='\0';
    }
    char msg_Timeout[40];
    sprintf(timeout,"uci get nport.@port%d[0].timeout",com);
    if((fp=popen(timeout,"r"))==NULL)
    {
        perror("popen error");
        return -2;
    }
    else
    {
        memset(msg_Timeout,0,sizeof(msg_Timeout));
        while(fgets(msg_Timeout,sizeof(msg_Timeout),fp)!=NULL)
        {
            //printf("msg:%s",remsg);
            
         }
         msg_Timeout[strlen(msg_Timeout)-1]='\0';
    }
    //strcpy(msg_Netpro,"tcpserver");
   // strcpy(msg_Timeout,"300");

    strcpy(TimeModel[0],msg_Netpro);
    strcpy(TimeModel[1],msg_Timeout);

    if(TimeModel[0] != NULL && TimeModel[1] !=NULL)
           status = 0x00;
    else
           status = 0x01;

    
}



int get_syslog(void *buff)
{
	;
}
int get_checkPort(void)
{
	;
}



int main(int argc,char *argv[])
{
	signal(SIGCHLD,SIG_IGN);
    int sock_fd,ret;
	
    sock_fd=socket(AF_INET,SOCK_STREAM,0);

    if(sock_fd<0)
    {
       perror("socket");
       return -1;
    }


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

void get_portMesg(char *buff)
{
    
    //int N = 100;
    FILE *fp;
    //char str[N + 1];
    fp = fopen("/etc/ser2net.conf","r");
    
    if(fp == NULL)
    {
       perror("failed open");
      // return -3;
    }

    if(fgets(buff,sizeof(buff), fp) != NULL )
    {
        //printf("\n%s\n", buff);
    }
    
    fclose(fp);
}

void set_port1(char *buf)
{
    
    ports=get_ports();
    struct termios *opt;
    int speed;
    char *check,*stop;
    char *data;
    char *model;
    char *types;
    char *control,tmp[1024];
    int com,fd;
    com=buf[6]-1;
    
    switch(buf[7]){
         case 0x00:break;
         case 0x01:speed=300;break;
         case 0x02:speed=600;break;
         case 0x03:speed=1200;break;
         case 0x04:speed=2400;break;
         case 0x05:speed=4800;break;
         case 0x06:speed=9600;break;
         case 0x07:speed=19200;break;
         case 0x08:speed=38400;break;
         case 0x09:speed=57600;break;
         case 0x0A:speed=115200;break;
         default:speed=9600;break;
    }

    switch(buf[8]){
         case 0x00:break;
         case 0x01:data = "5DATABITS";break;
         case 0x02:data = "6DATABITS";break;
         case 0x03:data = "7DATABITS";break;
         case 0x04:data = "8DATABITS";break;
         default:data = "8DATABITS";break;
    }

    switch(buf[9]){
         case 0x00:break;
         case 0x01:check = "NONE";break;
         case 0x02:check = "EVEN";break;
         case 0x03:check = "ODD";break;
         case 0x04:check = "MARK";break;
         case 0x05:check = "SPACE";break;
         default:check = "NONE";break;
    }

    switch(buf[10]){
         case 0x00:break;
         case 0x01:stop = "1STOPBIT";break;
         case 0x02:stop = "2STOPBITS";break;
         default:stop = "1STOPBIT";break;
    }

    switch(buf[11]){
         case 0x00:break;
         case 0x01:model = "tcpserver";break;
         case 0x02:model = "udpserver";break;
         case 0x03:model = "udpclient";break;
         case 0x04:model = "modbustcp";break;
         default:model = "tcpserver";break;
    }

    switch(buf[12]){
         case 0x00:break;
         case 0x01:types = "232";break;
         case 0x02:types = "422";break;
         case 0x03:types = "485";break;
         default:types = "232";break;
    }


    switch(buf[13]){
         case 0x00:break;
         case 0x01:control = "NONE";break;
         case 0x02:control = "RTS/CTS";break;
         case 0x03:control = "XON/XOFF";break;
         case 0x04:control = "DTR/DSR";break;
         default:control = "RTS/CTS";break;
    }
     
     if(buf[6] == -1)
    {
          //  波特率
          sprintf(tmp,"uci set nport.@allport[0].baud=%d",speed);
          
          system(tmp);
          memset(tmp,0,200);

         // 数据位
          sprintf(tmp,"uci set nport.@allport[0].data=%s",data);
          
          system(tmp);
          memset(tmp,0,200);

         // 校验位
          sprintf(tmp,"uci set nport.@allport[0].parity=%s",check);
          
          system(tmp);
          memset(tmp,0,200);

         // 停止位
          sprintf(tmp,"uci set nport.@allport[0].stop=%s",stop);
          
          system(tmp);
          memset(tmp,0,200);

         // 模式状态
          sprintf(tmp,"uci set nport.@allport[0].netpro=%s",model);
          
          system(tmp);
          memset(tmp,0,200);
    }
    else if(com >= 0 && com <= ports)
    {
           sprintf(tmp,"uci set nport.@port%d[0].baud=%d",com,speed);
           
           system(tmp);
           memset(tmp,0,200);
           sprintf(tmp,"uci set nport.@port%d[0].data=%s",com,data);
           system(tmp);
           memset(tmp,0,200);
           sprintf(tmp,"uci set nport.@port%d[0].parity=%s",com,check);
           system(tmp);
           memset(tmp,0,200);
           sprintf(tmp,"uci set nport.@port%d[0].stop=%s",com,stop);
           system(tmp);
           memset(tmp,0,200);
           sprintf(tmp,"uci set nport.@port%d[0].netpro=%s",com,model);
           
           system(tmp);
           memset(tmp,0,200);
    }
    else if(com+1>ports)
    {
            status = 0x02;
    }
    else
    {
             status = 0x01;     
    }
    
     system("uci commit");   
     status = 0x00;
}
int get_ports(void)
{
    FILE *fp;
    char remsg[10];
    if((fp=popen("ls  /dev/ttyUSB* | wc -l","r"))==NULL)
    {
        perror("popen error");
        return -2;
    }
    else
    {
        memset(remsg,0,sizeof(remsg));
        while(fgets(remsg,sizeof(remsg),fp)!=NULL)
        {
            //printf("msg:%s",remsg);
        }
        remsg[strlen(remsg)-1]='\0';
    }
    return atoi(remsg);

}

int get_devices(char *buff)
{
   
    FILE *fp;
    char msg_name[100];
    if((fp=popen("uci get system.@system[0].hostname","r"))==NULL)
    {   
        perror("popen error");
        return -2;
    }
    else
    {   
        memset(msg_name,0,sizeof(msg_name));
        while(fgets(msg_name,sizeof(msg_name),fp)!=NULL)
        {   
            //printf("msg:%s\n",msg_name);
        }
        msg_name[strlen(msg_name)-1]='\0';
    }
    // 型号
    char msg_ModelName[40];
    if((fp=popen("uci get system.@system[0].model","r"))==NULL)
    {
        perror("popen error");
        return -2;
    }
    else
    {
        memset(msg_ModelName,0,sizeof(msg_ModelName));
        while(fgets(msg_ModelName,sizeof(msg_ModelName),fp)!=NULL)
        {
            //printf("msg:%s",remsg);
        }
        msg_ModelName[strlen(msg_ModelName)-1]='\0';
    }

    // MAC
    char msg_MAC[40];
    if((fp=popen("ifconfig br-lan | sed -n '/HWaddr/ s/^.*HWaddr *//pg'","r"))==NULL)
    {
        perror("popen error");
        return -2;
    }
    else
    {
        memset(msg_MAC,0,sizeof(msg_MAC));
        while(fgets(msg_MAC,sizeof(msg_MAC),fp)!=NULL)
        {
            //printf("msg:%s",remsg);
        }
        msg_MAC[strlen(msg_MAC)-1]='\0';
    }

    // 版本号
    char msg_Version[40];
    if((fp=popen("uci get system.@system[0].F_ver","r"))==NULL)
    {
        perror("popen error");
        return -2;
    }
    else
    {
        memset(msg_Version,0,sizeof(msg_Version));
        while(fgets(msg_Version,sizeof(msg_Version),fp)!=NULL)
        {
            //printf("msg:%s",remsg);
        }
        msg_Version[strlen(msg_Version)-1]='\0';
    }

    // ip
    char msg_Ip[40];
    if((fp=popen("uci get network.lan.ipaddr","r"))==NULL)
    {
        perror("popen error");
        return -2;
    }
    else
    {
        memset(msg_Ip,0,sizeof(msg_Ip));
        while(fgets(msg_Ip,sizeof(msg_Ip),fp)!=NULL)
        {
            //printf("msg:%s",remsg);
        }
        msg_Ip[strlen(msg_Ip)-1]='\0';
    }

    // mask
    char msg_Mask[40];
    if((fp=popen("uci get network.lan.netmask","r"))==NULL)
    {
        perror("popen error");
        return -2;
    }
    else
    {
        memset(msg_Mask,0,sizeof(msg_Mask));
        while(fgets(msg_Mask,sizeof(msg_Mask),fp)!=NULL)
        {
            //printf("msg:%s",remsg);
        }
        msg_Mask[strlen(msg_Mask)-1]='\0';
    }

    // Gateway
    char msg_Gateway[40];
    if((fp=popen("uci get network.lan.gateway","r"))==NULL)
    {
        perror("popen error");
        return -2;
    }
    else
    {
        memset(msg_Gateway,0,sizeof(msg_Gateway));
        while(fgets(msg_Gateway,sizeof(msg_Gateway),fp)!=NULL)
        {
            //printf("msg:%s",remsg);
        }
        msg_Gateway[strlen(msg_Gateway)-1]='\0';
    }

    // DNS
    char msg_DNS[40];
    if((fp=popen("uci get network.lan.dns","r"))==NULL)
    {
        perror("popen error");
        return -2;
    }
    else
    {
        memset(msg_DNS,0,sizeof(msg_DNS));
        while(fgets(msg_DNS,sizeof(msg_DNS),fp)!=NULL)
        {
            //printf("msg:%s",remsg);
        }
        msg_DNS[strlen(msg_DNS)-1]='\0';
    }
  
    //sprintf(Mac,"%s",msg_MAC);
    //sprintf(Name,"%s%s%s",msg_name,msg_ModelName,msg_Version);
    
    strcat(Name,msg_name);
    
    strcat(Name,msg_ModelName);
    
    strcat(Name,msg_Version);
    
    strcat(Mac,msg_MAC);
    strcat(Four,msg_Ip);
    strcat(Four," ");
    strcat(Four,msg_Mask);
    strcat(Four," ");
    strcat(Four,msg_Gateway);
    strcat(Four," ");
    strcat(Four,msg_DNS);
    printf("Four:%s\n",Four);
    //sprintf(Four,"%s %s %s %s %d",msg_Ip,msg_Mask,msg_Gateway,msg_DNS,6); 
    if(Name != NULL && Mac !=NULL && Four !=NULL)
          status = 0x00;
    else
          status = 0x01;
    return snprintf(buff,1024,"%d",3);
}
