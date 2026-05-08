#include "dialing.h"
/* 功能：设定fm160模组拨号方式
** 返回值：void
*/
void setModule_FM160(char *tmp_mode_usb,char *tmp_mode_nat)
{
	char cmd[1024];
	char *end;
	int tmp_int_buff;
	char tmp_buff[1024];
	char tmp_str_buff[1024];
	char debug[100];
	char tmp_debug[100];
	memset(debug,0,sizeof(debug));
	memset(tmp_debug,0,sizeof(tmp_debug));
	memset(cmd,0,sizeof(cmd));
	memset(tmp_buff,0,sizeof(tmp_buff));
	sprintf(cmd,"cat /proc/uptime | awk -F. '{print $1}'");
	cmd_recieve_str(cmd,tmp_buff);
	long int time=strtol(tmp_buff, &end, 10);
	if(time<90)
		sleep(11);
	//确认模组正常工作
	while(1)
	{
		memset(cmd,0,sizeof(cmd));
		sprintf(cmd,"cmdat %s 'AT' | grep OK |wc -l",path_usb);
		tmp_int_buff = cmd_recieve_int(cmd);
		if (tmp_int_buff == 1)
			break;
		sleep(1);
	}
	
	//sleep(3);
	//设定拨号上网驱动
	if( tmp_mode_usb != NULL )
	{
		if(strlen(tmp_mode_usb)>0)
		{
			memset(cmd,0,sizeof(cmd));
			sprintf(cmd,"cmdat %s \"AT+GTUSBMODE?\" | grep \"+GTUSBMODE: %s\" | wc -l",path_usb,tmp_mode_usb);
			tmp_int_buff = cmd_recieve_int(cmd);
			//当前拨号上网驱动与设定方式不一致时进行设定
			if (tmp_int_buff == 0)
			{
				memset(cmd,0,sizeof(cmd));
				sprintf(cmd,"cmdat %s \"AT+GTUSBMODE=%s\" 100 | grep \"OK\" |wc -l",path_usb,tmp_mode_usb);
				tmp_int_buff = cmd_recieve_int(cmd);
				if(tmp_int_buff == 1)
				{
					sprintf(tmp_debug,"set usbmode: %s OK; ",tmp_mode_usb);
					sleep(60);
					while(1)
					{
						memset(cmd,0,sizeof(cmd));
						sprintf(cmd,"cmdat %s 'AT' | grep OK |wc -l",path_usb);
						tmp_int_buff = cmd_recieve_int(cmd);
						if (tmp_int_buff == 1)
							break;
						sleep(1);
					}
				}
				else
				{
					sprintf(tmp_debug,"set usbmode: %s failed; ",tmp_mode_usb);
				}
					
			}else{	//一致时，打印当前上网方式
				memset(cmd,0,sizeof(cmd));
				sprintf(cmd,"cmdat %s \"AT+GTUSBMODE?\" | grep \"+GTUSBMODE: \" | awk -F ' ' '{print$2}'",path_usb);
				memset(tmp_str_buff,0,sizeof(tmp_str_buff));
				cmd_recieve_str(cmd,tmp_str_buff);
				sprintf(tmp_debug,"usbmode: %s; ",tmp_str_buff);
			}
		}
	}
	if(strlen(tmp_debug)>0)
	{
		strcat(debug,tmp_debug);
		memset(tmp_debug,0,sizeof(tmp_debug));
	}
	//设定拨号上网方式
	if( tmp_mode_nat != NULL )
	{
		if(strlen(tmp_mode_nat)>0)
		{
			memset(cmd,0,sizeof(cmd));
			sprintf(cmd,"cmdat %s \"AT+GTIPPASS?\" | grep \"+GTIPPASS: %s\" | wc -l",path_usb,tmp_mode_nat);
			tmp_int_buff = cmd_recieve_int(cmd);
			//当前上网方式与设定方式不一致时进行设定
			if (tmp_int_buff == 0)
			{
				memset(cmd,0,sizeof(cmd));
				sprintf(cmd,"cmdat %s \"AT+GTIPPASS=%s\" 100 | grep \"OK\" |wc -l",path_usb,tmp_mode_nat);
				tmp_int_buff = cmd_recieve_int(cmd);
				if(tmp_int_buff == 1)
				{
					sprintf(tmp_debug,"set ippass: %s OK; ",tmp_mode_nat);
				}
				else
				{
					sprintf(tmp_debug,"set ippass: %s failed; ",tmp_mode_nat);
				}
			}else{	//一致时，打印当前上网方式
				memset(cmd,0,sizeof(cmd));
				sprintf(cmd,"cmdat %s \"AT+GTIPPASS?\" | grep \"+GTIPPASS: \" | awk -F ' ' '{print$2}'",path_usb);
				memset(tmp_str_buff,0,sizeof(tmp_str_buff));
				cmd_recieve_str(cmd,tmp_str_buff);
				sprintf(tmp_debug,"ippass: %s; ",tmp_str_buff);
			}
		}
	}
	if(strlen(tmp_debug)>0)
	{
		strcat(debug,tmp_debug);
		memset(tmp_debug,0,sizeof(tmp_debug));
	}
	if( tmp_mode_nat != NULL )
	{
		if(strlen(tmp_mode_nat)>0)
		{
			memset(cmd,0,sizeof(cmd));
			sprintf(cmd,"cmdat %s \"AT+GTIPPASSMODE?\" | grep \"+GTIPPASSMODE: %s\" | wc -l",path_usb,tmp_mode_nat);
			tmp_int_buff = cmd_recieve_int(cmd);
			//当前上网方式与设定方式不一致时进行设定
			if (tmp_int_buff == 0)
			{
				memset(cmd,0,sizeof(cmd));
				sprintf(cmd,"cmdat %s \"AT+GTIPPASSMODE=%s\" 100 | grep \"OK\" |wc -l",path_usb,tmp_mode_nat);
				tmp_int_buff = cmd_recieve_int(cmd);
				if(tmp_int_buff == 1)
				{
					sprintf(tmp_debug,"set ippassmode: %s OK; ",tmp_mode_nat);
				}
				else
				{
					sprintf(tmp_debug,"set ippassmode: %s failed; ",tmp_mode_nat);
				}
			}else{	//一致时，打印当前上网方式
				memset(cmd,0,sizeof(cmd));
				sprintf(cmd,"cmdat %s \"AT+GTIPPASSMODE?\" | grep \"+GTIPPASSMODE: \" | awk -F ' ' '{print$2}'",path_usb);
				memset(tmp_str_buff,0,sizeof(tmp_str_buff));
				cmd_recieve_str(cmd,tmp_str_buff);
				sprintf(tmp_debug,"ippassmode: %s",tmp_str_buff);
			}
		}
	}
	if(strlen(tmp_debug)>0)
	{
		strcat(debug,tmp_debug);
	}
	if(strlen(debug)>0)
	{
		Debug(debug,"module.info");
	}
	//设定当前读卡位置
	int result=0;
	if(strstr(value_defaultcard,"auto") || strstr(value_defaultcard,"sim1"))
	{
		memset(cmd,0,sizeof(cmd));
		if(strstr(board_name,"R680-8TH") || strstr(board_name,"R690") || strstr(board_name,"RT990"))
		{
			//gpio控制读卡先检测模组是否为默认读卡位置
			memset(cmd,0,sizeof(cmd));
			memset(tmp_buff,0,sizeof(tmp_buff));
			sprintf(cmd,"cat /proc/uptime | awk -F. '{print $1}'");
			cmd_recieve_str(cmd,tmp_buff);
			long int time=strtol(tmp_buff, &end, 10);
			if(time>90)
			{
				sprintf(cmd,"cmdat %s 'AT+GTDUALSIM?' | grep \"+GTDUALSIM:\" | awk -F ',' '{print $1}' | awk -F ' ' '{print $2}'",path_usb);
				result = cmd_recieve_int(cmd);
				if (result != 0) 
				{
					memset(cmd,0,sizeof(cmd));
					sprintf(cmd,"cmdat %s 'AT+GTDUALSIM=0'",path_usb);
					cmd_recieve_str(cmd,NULL);
					sleep(1);
				}
			}
			memset(cmd,0,sizeof(cmd));
			sprintf(cmd,"cat /sys/class/leds/sim_sw/brightness");
			result = cmd_recieve_int(cmd);
			printf("Current card slot:%d\n",result);
			if (result == 255)
				result=1;
			if (result == 1) //[优先卡槽] == [当前卡槽]
			{
				sprintf(tmp_str_buff,"%s",value_defaultcard);
				memset(cmd,0,sizeof(cmd));
				memset(tmp_buff,0,sizeof(tmp_buff));
				sprintf(cmd,"cat /proc/uptime | awk -F. '{print $1}'");
				cmd_recieve_str(cmd,tmp_buff);
				long int time=strtol(tmp_buff, &end, 10);
				if(time>90)
					setModem1();
			}else
			{
				memset(cmd,0,sizeof(cmd));
				sprintf(cmd,"echo 1 > /sys/class/leds/sim_sw/brightness");
				cmd_recieve_str(cmd,NULL);
				sprintf(tmp_str_buff,"%s",value_defaultcard);
				sleep(10);
				setModem1();
				setFlightmode();
			}
		}
		else if(strstr(board_name,"R660"))
		{
			//gpio控制读卡先检测模组是否为默认读卡位置
			memset(cmd,0,sizeof(cmd));
			memset(tmp_buff,0,sizeof(tmp_buff));
			sprintf(cmd,"cat /proc/uptime | awk -F. '{print $1}'");
			cmd_recieve_str(cmd,tmp_buff);
			long int time=strtol(tmp_buff, &end, 10);
			if(time>90)
			{
				sprintf(cmd,"cmdat %s 'AT+GTDUALSIM?' | grep \"+GTDUALSIM:\" | awk -F ',' '{print $1}' | awk -F ' ' '{print $2}'",path_usb);
				result = cmd_recieve_int(cmd);
				if (result != 0) 
				{
					memset(cmd,0,sizeof(cmd));
					sprintf(cmd,"cmdat %s 'AT+GTDUALSIM=0'",path_usb);
					cmd_recieve_str(cmd,NULL);
					sleep(1);
				}
			}
			memset(cmd,0,sizeof(cmd));
			sprintf(cmd,"cat /sys/class/gpio/sim_sw/value");
			result = cmd_recieve_int(cmd);
			printf("Current card slot:%d\n",result);
			if (result == 255)
				result=1;
			if (result == 1) //[优先卡槽] == [当前卡槽]
			{
				sprintf(tmp_str_buff,"%s",value_defaultcard);
				memset(cmd,0,sizeof(cmd));
				memset(tmp_buff,0,sizeof(tmp_buff));
				sprintf(cmd,"cat /proc/uptime | awk -F. '{print $1}'");
				cmd_recieve_str(cmd,tmp_buff);
				long int time=strtol(tmp_buff, &end, 10);
				if(time>90)
					setModem1();
			}else
			{
				memset(cmd,0,sizeof(cmd));
				sprintf(cmd,"echo 1 > /sys/class/gpio/sim_sw/value");
				cmd_recieve_str(cmd,NULL);
				sprintf(tmp_str_buff,"%s",value_defaultcard);
				//sleep(1);
				setModem1();
				setFlightmode();
			}
		}
		else if(strstr(board_name,"R650") || strstr(board_name,"R680"))
		{
			memset(cmd,0,sizeof(cmd));
			memset(tmp_buff,0,sizeof(tmp_buff));
			sprintf(cmd,"cat /proc/uptime | awk -F. '{print $1}'");
			cmd_recieve_str(cmd,tmp_buff);
			long int time=strtol(tmp_buff, &end, 10);
			if(time<90)
			{
				//Debug("return setmodem1","daemon.debug");
			}
			else
			{
				if(strstr(value_defaultcard,"auto"))
				{
					sprintf(cmd,"cmdat %s 'AT+GTDUALSIM?' | grep \"+GTDUALSIM:\" | awk -F ',' '{print $1}' | awk -F ' ' '{print $2}'",path_usb);
					result = cmd_recieve_int(cmd);
					if(result==0)
					{
						//Debug("set Modem1","daemon.debug");
						usleep(500);
						setModem1();
					}
					else if(result==1)
					{
						//Debug("set Modem2","daemon.debug");
						usleep(500);
						setModem2();
					}
				}
				else
				{
					sprintf(cmd,"cmdat %s 'AT+GTDUALSIM?' | grep \"+GTDUALSIM:\" | awk -F ',' '{print $1}' | awk -F ' ' '{print $2}'",path_usb);
					result = cmd_recieve_int(cmd);
					printf("Current card slot:%d\n",result);
					if (result == 255)
						result=1;
					if (result == 0) //[优先卡槽] == [当前卡槽]
					{
						sprintf(tmp_str_buff,"%s",value_defaultcard);
						setModem1();
					}else
					{
						memset(cmd,0,sizeof(cmd));
						sprintf(cmd,"cmdat %s 'AT+GTDUALSIM=0'",path_usb);
						cmd_recieve_str(cmd,NULL);
						sprintf(tmp_str_buff,"%s",value_defaultcard);
						sleep(10);
						setModem1();
						setFlightmode();
					}
				}
			}
		}
	}
	else if(strstr(value_defaultcard,"sim2"))
	{
		memset(cmd,0,sizeof(cmd));
		if(strstr(board_name,"R680-8TH") || strstr(board_name,"R690") || strstr(board_name,"RT990"))
		{
			//gpio控制读卡先检测模组是否为默认读卡位置
			memset(cmd,0,sizeof(cmd));
			memset(tmp_buff,0,sizeof(tmp_buff));
			sprintf(cmd,"cat /proc/uptime | awk -F. '{print $1}'");
			cmd_recieve_str(cmd,tmp_buff);
			long int time=strtol(tmp_buff, &end, 10);
			if(time>90)
			{
				sprintf(cmd,"cmdat %s 'AT+GTDUALSIM?' | grep \"+GTDUALSIM:\" | awk -F ',' '{print $1}' | awk -F ' ' '{print $2}'",path_usb);
				result = cmd_recieve_int(cmd);
				if (result != 0) 
				{
					memset(cmd,0,sizeof(cmd));
					sprintf(cmd,"cmdat %s 'AT+GTDUALSIM=0'",path_usb);
					cmd_recieve_str(cmd,NULL);
					sleep(1);
				}
			}
				memset(cmd,0,sizeof(cmd));
				sprintf(cmd,"cat /sys/class/leds/sim_sw/brightness");
				result = cmd_recieve_int(cmd);
				printf("Current card slot:%d\n",result);
				if (result == 255)
					result=1;
				if (result == 0) //[优先卡槽] == [当前卡槽]
				{
					sprintf(tmp_str_buff,"%s",value_defaultcard);
					memset(cmd,0,sizeof(cmd));
					memset(tmp_buff,0,sizeof(tmp_buff));
					sprintf(cmd,"cat /proc/uptime | awk -F. '{print $1}'");
					cmd_recieve_str(cmd,tmp_buff);
					long int time=strtol(tmp_buff, &end, 10);
					if(time>90)
						setModem2();
				}else
				{
					memset(cmd,0,sizeof(cmd));
					sprintf(cmd,"echo 0 > /sys/class/leds/sim_sw/brightness");
					cmd_recieve_str(cmd,NULL);
					sprintf(tmp_str_buff,"%s",value_defaultcard);
					sleep(10);
					memset(cmd,0,sizeof(cmd));
					memset(tmp_buff,0,sizeof(tmp_buff));
					sprintf(cmd,"cat /proc/uptime | awk -F. '{print $1}'");
					cmd_recieve_str(cmd,tmp_buff);
					long int time=strtol(tmp_buff, &end, 10);
					if(time>90)
						setModem2();
					setFlightmode();
				}
		}
		else if(strstr(board_name,"R660"))
		{
			//gpio控制读卡先检测模组是否为默认读卡位置
			memset(cmd,0,sizeof(cmd));
			memset(tmp_buff,0,sizeof(tmp_buff));
			sprintf(cmd,"cat /proc/uptime | awk -F. '{print $1}'");
			cmd_recieve_str(cmd,tmp_buff);
			long int time=strtol(tmp_buff, &end, 10);
			if(time>90)
			{
				sprintf(cmd,"cmdat %s 'AT+GTDUALSIM?' | grep \"+GTDUALSIM:\" | awk -F ',' '{print $1}' | awk -F ' ' '{print $2}'",path_usb);
				result = cmd_recieve_int(cmd);
				if (result != 0) 
				{
					memset(cmd,0,sizeof(cmd));
					sprintf(cmd,"cmdat %s 'AT+GTDUALSIM=0'",path_usb);
					cmd_recieve_str(cmd,NULL);
					sleep(1);
				}
			}
			memset(cmd,0,sizeof(cmd));
			sprintf(cmd,"cat /sys/class/gpio/sim_sw/value");
			result = cmd_recieve_int(cmd);
			printf("Current card slot:%d\n",result);
			if (result == 255)
				result=1;
			if (result == 0) //[优先卡槽] == [当前卡槽]
			{
				sprintf(tmp_str_buff,"%s",value_defaultcard);
				memset(cmd,0,sizeof(cmd));
				memset(tmp_buff,0,sizeof(tmp_buff));
				sprintf(cmd,"cat /proc/uptime | awk -F. '{print $1}'");
				cmd_recieve_str(cmd,tmp_buff);
				long int time=strtol(tmp_buff, &end, 10);
				if(time>90)
					setModem2();
			}else
			{
				memset(cmd,0,sizeof(cmd));
				sprintf(cmd,"echo 0 > /sys/class/gpio/sim_sw/value");
				cmd_recieve_str(cmd,NULL);
				sprintf(tmp_str_buff,"%s",value_defaultcard);
				sleep(10);
				setModem2();
				setFlightmode();
			}
		}
		else if(strstr(board_name,"R650") || strstr(board_name,"R680"))
		{
			memset(cmd,0,sizeof(cmd));
			memset(tmp_buff,0,sizeof(tmp_buff));
			sprintf(cmd,"cat /proc/uptime | awk -F. '{print $1}'");
			cmd_recieve_str(cmd,tmp_buff);
			long int time=strtol(tmp_buff, &end, 10);
			if(time<90)
			{
				//Debug("return setmodem1","daemon.debug");
			}
			else
			{
				sprintf(cmd,"cmdat %s 'AT+GTDUALSIM?' | grep \"+GTDUALSIM:\" | awk -F ',' '{print $1}' | awk -F ' ' '{print $2}'",path_usb);
				result = cmd_recieve_int(cmd);
				printf("Current card slot:%d\n",result);
				if (result == 255)
					result=1;
				if (result == 1) //[优先卡槽] == [当前卡槽]
				{
					sprintf(tmp_str_buff,"%s",value_defaultcard);
					setModem2();
				}else
				{
					memset(cmd,0,sizeof(cmd));
					sprintf(cmd,"cmdat %s 'AT+GTDUALSIM=1'",path_usb);
					cmd_recieve_str(cmd,NULL);
					sprintf(tmp_str_buff,"%s",value_defaultcard);
					sleep(10);
					
					setModem2();
					setFlightmode();
				}
			}
		}
	}
	else
		//Debug("setModem","Daemon.debug");
		setModem();
}

void setModule_FG160()
{
	char cmd[1024];
	char *end;
	int tmp_int_buff;
	char tmp_buff[1024];
	char tmp_str_buff[1024];
	char debug[100];
	char tmp_debug[100];
	memset(debug,0,sizeof(debug));
	memset(tmp_debug,0,sizeof(tmp_debug));
	memset(cmd,0,sizeof(cmd));
	memset(tmp_buff,0,sizeof(tmp_buff));
	sprintf(cmd,"cat /proc/uptime | awk -F. '{print $1}'");
	cmd_recieve_str(cmd,tmp_buff);
	long int time=strtol(tmp_buff, &end, 10);
	//if(time<90)
		//sleep(11);
	//确认模组正常工作
	while(1)
	{
		memset(cmd,0,sizeof(cmd));
		sprintf(cmd,"cmdat %s 'AT' | grep OK |wc -l",path_usb);
		tmp_int_buff = cmd_recieve_int(cmd);
		if (tmp_int_buff == 1)
			break;
		sleep(1);
	}
	
	memset(cmd,0,sizeof(cmd));
	sprintf(cmd,"cmdat %s 'AT+GTINTFMODE?' | grep '+GTINTFMODE: 0' | wc -l",path_usb);
	tmp_int_buff = cmd_recieve_int(cmd);
	//当前非0时进行设定
	if (tmp_int_buff == 0)
	{
		memset(cmd,0,sizeof(cmd));
		sprintf(cmd,"cmdat %s 'AT+GTINTFMODE=0' | grep \"OK\" |wc -l",path_usb);
		tmp_int_buff = cmd_recieve_int(cmd);
		if(tmp_int_buff == 1)
			sprintf(tmp_debug,"set INTFMODE:USB and PCle OK; ");
		else
			sprintf(tmp_debug,"set INTFMODE:USB and PCle failed; ");
	}
	else{	//一致时，打印当前结果
		sprintf(tmp_debug,"INTFMODE:USB and PCle; ");
	}
	if(strlen(tmp_debug)>0)
	{
		strcat(debug,tmp_debug);
	}
	memset(cmd,0,sizeof(cmd));
	sprintf(cmd,"cmdat %s 'AT+MSMPD?' | grep \"+MSMPD: 1\" | wc -l",path_usb);
	tmp_int_buff = cmd_recieve_int(cmd);
	if (tmp_int_buff == 0)
	{
		memset(cmd,0,sizeof(cmd));
		sprintf(cmd,"cmdat %s 'AT+MSMPD=1' | grep \"OK\" | wc -l",path_usb);
		tmp_int_buff = cmd_recieve_int(cmd);
		if(tmp_int_buff == 1)
			sprintf(tmp_debug,"set card hot-plug OK");
		else
			sprintf(tmp_debug,"set card hot-plug failed");
			
	}else{	//一致时，打印当前上网方式
		sprintf(tmp_debug,"card hot-plug OK");
	}
	if(strlen(tmp_debug)>0)
	{
		strcat(debug,tmp_debug);
	}
	if(strlen(debug)>0)
	{
		Debug(debug,"module.info");
	}

	/* while(1)
	{
		memset(cmd,0,sizeof(cmd));
		sprintf(cmd,"cmdat /dev/mhi_DUN 'AT' | grep OK |wc -l");
		tmp_int_buff = cmd_recieve_int(cmd);
		//Debug("cmdat mhi_DUN","daemon.debug");
		if (tmp_int_buff == 1)
			break;
		sleep(1);
	} */
	//设定当前读卡位置
	int result=0;
	if(strstr(value_defaultcard,"auto") || strstr(value_defaultcard,"sim1"))
	{
		memset(cmd,0,sizeof(cmd));
		if(strstr(board_name,"R680-8TH") || strstr(board_name,"R690") || strstr(board_name,"RT990"))
		{
			//gpio控制读卡先检测模组是否为默认读卡位置
			memset(cmd,0,sizeof(cmd));
			memset(tmp_buff,0,sizeof(tmp_buff));
			sprintf(cmd,"cat /proc/uptime | awk -F. '{print $1}'");
			cmd_recieve_str(cmd,tmp_buff);
			long int time=strtol(tmp_buff, &end, 10);
			//if(time>90)
			{
				sprintf(cmd,"cmdat %s 'AT+GTDUALSIM?' | grep \"+GTDUALSIM:\" | awk -F ',' '{print $1}' | awk -F ' ' '{print $2}'",path_usb);
				result = cmd_recieve_int(cmd);
				if (result != 0) 
				{
					memset(cmd,0,sizeof(cmd));
					sprintf(cmd,"cmdat %s 'AT+GTDUALSIM=0'",path_usb);
					cmd_recieve_str(cmd,NULL);
					sleep(1);
				}
			}
			memset(cmd,0,sizeof(cmd));
			sprintf(cmd,"cat /sys/class/leds/sim_sw/brightness");
			result = cmd_recieve_int(cmd);
			printf("Current card slot:%d\n",result);
			if (result == 255)
				result=1;
			if (result == 0) //[优先卡槽] == [当前卡槽]
			{
				sprintf(tmp_str_buff,"%s",value_defaultcard);
				memset(cmd,0,sizeof(cmd));
				memset(tmp_buff,0,sizeof(tmp_buff));
				sprintf(cmd,"cat /proc/uptime | awk -F. '{print $1}'");
				cmd_recieve_str(cmd,tmp_buff);
				long int time=strtol(tmp_buff, &end, 10);
				if(time>90)
					setModem1();
			}else
			{
				memset(cmd,0,sizeof(cmd));
				sprintf(cmd,"echo 0 > /sys/class/leds/sim_sw/brightness");
				cmd_recieve_str(cmd,NULL);
				sprintf(tmp_str_buff,"%s",value_defaultcard);
				//sleep(1);
				setModem1();
				setFlightmode();
			}
		}
		else if(strstr(board_name,"R660"))
		{
			//gpio控制读卡先检测模组是否为默认读卡位置
			memset(cmd,0,sizeof(cmd));
			memset(tmp_buff,0,sizeof(tmp_buff));
			sprintf(cmd,"cat /proc/uptime | awk -F. '{print $1}'");
			cmd_recieve_str(cmd,tmp_buff);
			long int time=strtol(tmp_buff, &end, 10);
			if(time>90)
			{
				sprintf(cmd,"cmdat %s 'AT+GTDUALSIM?' | grep \"+GTDUALSIM:\" | awk -F ',' '{print $1}' | awk -F ' ' '{print $2}'",path_usb);
				result = cmd_recieve_int(cmd);
				if (result != 0) 
				{
					memset(cmd,0,sizeof(cmd));
					sprintf(cmd,"cmdat %s 'AT+GTDUALSIM=0'",path_usb);
					cmd_recieve_str(cmd,NULL);
					sleep(1);
				}
			}
			memset(cmd,0,sizeof(cmd));
			sprintf(cmd,"cat /sys/class/gpio/sim_sw/value");
			result = cmd_recieve_int(cmd);
			printf("Current card slot:%d\n",result);
			if (result == 255)
				result=1;
			if (result == 1) //[优先卡槽] == [当前卡槽]
			{
				sprintf(tmp_str_buff,"%s",value_defaultcard);
				memset(cmd,0,sizeof(cmd));
				memset(tmp_buff,0,sizeof(tmp_buff));
				sprintf(cmd,"cat /proc/uptime | awk -F. '{print $1}'");
				cmd_recieve_str(cmd,tmp_buff);
				long int time=strtol(tmp_buff, &end, 10);
				if(time>90)
					setModem1();
			}else
			{
				memset(cmd,0,sizeof(cmd));
				sprintf(cmd,"echo 1 > /sys/class/gpio/sim_sw/value");
				cmd_recieve_str(cmd,NULL);
				sprintf(tmp_str_buff,"%s",value_defaultcard);
				//sleep(1);
				setModem1();
				setFlightmode();
			}
		}
		else if(strstr(board_name,"R650") || strstr(board_name,"R680"))
		{
			memset(cmd,0,sizeof(cmd));
			memset(tmp_buff,0,sizeof(tmp_buff));
			sprintf(cmd,"cat /proc/uptime | awk -F. '{print $1}'");
			cmd_recieve_str(cmd,tmp_buff);
			long int time=strtol(tmp_buff, &end, 10);
			if(time<90)
			{
				//Debug("return setmodem1","daemon.debug");
			}
			else
			{
				if(strstr(value_defaultcard,"auto"))
				{
					sprintf(cmd,"cmdat %s 'AT+GTDUALSIM?' | grep \"+GTDUALSIM:\" | awk -F ',' '{print $1}' | awk -F ' ' '{print $2}'",path_usb);
					result = cmd_recieve_int(cmd);
					if(result==0)
					{
						//Debug("set Modem1","daemon.debug");
						usleep(500);
						setModem1();
					}
					else if(result==1)
					{
						//Debug("set Modem2","daemon.debug");
						usleep(500);
						setModem2();
					}
				}
				else
				{
					sprintf(cmd,"cmdat %s 'AT+GTDUALSIM?' | grep \"+GTDUALSIM:\" | awk -F ',' '{print $1}' | awk -F ' ' '{print $2}'",path_usb);
					result = cmd_recieve_int(cmd);
					printf("Current card slot:%d\n",result);
					if (result == 255)
						result=1;
					if (result == 0) //[优先卡槽] == [当前卡槽]
					{
						sprintf(tmp_str_buff,"%s",value_defaultcard);
						setModem1();
					}else
					{
						memset(cmd,0,sizeof(cmd));
						sprintf(cmd,"cmdat %s 'AT+GTDUALSIM=0'",path_usb);
						cmd_recieve_str(cmd,NULL);
						sprintf(tmp_str_buff,"%s",value_defaultcard);
						setModem1();
						setFlightmode();
					}
				}
			}
		}
	}
	else if(strstr(value_defaultcard,"sim2"))
	{
		memset(cmd,0,sizeof(cmd));
		if(strstr(board_name,"R680-8TH") || strstr(board_name,"R690") || strstr(board_name,"RT990"))
		{
			//gpio控制读卡先检测模组是否为默认读卡位置
			memset(cmd,0,sizeof(cmd));
			memset(tmp_buff,0,sizeof(tmp_buff));
			sprintf(cmd,"cat /proc/uptime | awk -F. '{print $1}'");
			cmd_recieve_str(cmd,tmp_buff);
			long int time=strtol(tmp_buff, &end, 10);
			//if(time>90)
			{
				sprintf(cmd,"cmdat %s 'AT+GTDUALSIM?' | grep \"+GTDUALSIM:\" | awk -F ',' '{print $1}' | awk -F ' ' '{print $2}'",path_usb);
				result = cmd_recieve_int(cmd);
				if (result != 0) 
				{
					memset(cmd,0,sizeof(cmd));
					sprintf(cmd,"cmdat %s 'AT+GTDUALSIM=0'",path_usb);
					cmd_recieve_str(cmd,NULL);
					sleep(1);
				}
			}
				memset(cmd,0,sizeof(cmd));
				sprintf(cmd,"cat /sys/class/leds/sim_sw/brightness");
				result = cmd_recieve_int(cmd);
				printf("Current card slot:%d\n",result);
				if (result == 255)
					result=1;
				if (result == 1) //[优先卡槽] == [当前卡槽]
				{
					sprintf(tmp_str_buff,"%s",value_defaultcard);
					memset(cmd,0,sizeof(cmd));
					memset(tmp_buff,0,sizeof(tmp_buff));
					sprintf(cmd,"cat /proc/uptime | awk -F. '{print $1}'");
					cmd_recieve_str(cmd,tmp_buff);
					long int time=strtol(tmp_buff, &end, 10);
					if(time>90)
						setModem2();
				}else
				{
					memset(cmd,0,sizeof(cmd));
					sprintf(cmd,"echo 1 > /sys/class/leds/sim_sw/brightness");
					cmd_recieve_str(cmd,NULL);
					sprintf(tmp_str_buff,"%s",value_defaultcard);
					//sleep(1);
					memset(cmd,0,sizeof(cmd));
					memset(tmp_buff,0,sizeof(tmp_buff));
					sprintf(cmd,"cat /proc/uptime | awk -F. '{print $1}'");
					cmd_recieve_str(cmd,tmp_buff);
					long int time=strtol(tmp_buff, &end, 10);
					if(time>90)
						setModem2();
					setFlightmode();
				}
		}
		else if(strstr(board_name,"R660"))
		{
			//gpio控制读卡先检测模组是否为默认读卡位置
			memset(cmd,0,sizeof(cmd));
			memset(tmp_buff,0,sizeof(tmp_buff));
			sprintf(cmd,"cat /proc/uptime | awk -F. '{print $1}'");
			cmd_recieve_str(cmd,tmp_buff);
			long int time=strtol(tmp_buff, &end, 10);
			if(time>90)
			{
				sprintf(cmd,"cmdat %s 'AT+GTDUALSIM?' | grep \"+GTDUALSIM:\" | awk -F ',' '{print $1}' | awk -F ' ' '{print $2}'",path_usb);
				result = cmd_recieve_int(cmd);
				if (result != 0) 
				{
					memset(cmd,0,sizeof(cmd));
					sprintf(cmd,"cmdat %s 'AT+GTDUALSIM=0'",path_usb);
					cmd_recieve_str(cmd,NULL);
					sleep(1);
				}
			}
			memset(cmd,0,sizeof(cmd));
			sprintf(cmd,"cat /sys/class/gpio/sim_sw/value");
			result = cmd_recieve_int(cmd);
			printf("Current card slot:%d\n",result);
			if (result == 255)
				result=1;
			if (result == 0) //[优先卡槽] == [当前卡槽]
			{
				sprintf(tmp_str_buff,"%s",value_defaultcard);
				memset(cmd,0,sizeof(cmd));
				memset(tmp_buff,0,sizeof(tmp_buff));
				sprintf(cmd,"cat /proc/uptime | awk -F. '{print $1}'");
				cmd_recieve_str(cmd,tmp_buff);
				long int time=strtol(tmp_buff, &end, 10);
				if(time>90)
					setModem2();
			}else
			{
				memset(cmd,0,sizeof(cmd));
				sprintf(cmd,"echo 0 > /sys/class/gpio/sim_sw/value");
				cmd_recieve_str(cmd,NULL);
				sprintf(tmp_str_buff,"%s",value_defaultcard);
				//sleep(1);
				setModem2();
				setFlightmode();
			}
		}
		else if(strstr(board_name,"R650") || strstr(board_name,"R680"))
		{
			memset(cmd,0,sizeof(cmd));
			memset(tmp_buff,0,sizeof(tmp_buff));
			sprintf(cmd,"cat /proc/uptime | awk -F. '{print $1}'");
			cmd_recieve_str(cmd,tmp_buff);
			long int time=strtol(tmp_buff, &end, 10);
			if(time<90)
			{
				//Debug("return setmodem1","daemon.debug");
			}
			else
			{
				sprintf(cmd,"cmdat %s 'AT+GTDUALSIM?' | grep \"+GTDUALSIM:\" | awk -F ',' '{print $1}' | awk -F ' ' '{print $2}'",path_usb);
				result = cmd_recieve_int(cmd);
				printf("Current card slot:%d\n",result);
				if (result == 255)
					result=1;
				if (result == 1) //[优先卡槽] == [当前卡槽]
				{
					sprintf(tmp_str_buff,"%s",value_defaultcard);
					setModem2();
				}else
				{
					memset(cmd,0,sizeof(cmd));
					sprintf(cmd,"cmdat %s 'AT+GTDUALSIM=1'",path_usb);
					cmd_recieve_str(cmd,NULL);
					sprintf(tmp_str_buff,"%s",value_defaultcard);
					//sleep(1);
					
					setModem2();
					setFlightmode();
				}
			}
		}
	}
	else
		//Debug("setModem","Daemon.debug");
		setModem();
}

/* 功能：设定fm160模组上网服务类型
** 返回值：void
*/
void setRATMODE_FM160(char *mode,char *wcdma,char *lte,char *nr)
{
	//Debug("setRATMODE_FM160","daemon.debug");
	char cmd[1024];
	char tmp_buff[1024];
	//设定服务类型
	char str_wcdma[100];
	char str_lte[100];
	char str_nr[100];
	char str_mode[100];
	char *end;
	memset(cmd,0,sizeof(cmd));
	memset(tmp_buff,0,sizeof(tmp_buff));
	sprintf(cmd,"cat /proc/uptime | awk -F. '{print $1}'");
	cmd_recieve_str(cmd,tmp_buff);
	long int time=strtol(tmp_buff, &end, 10);
	if(time<90)
	{
		//Debug("return setRat","daemon.debug");
		return;
	}
	
	while(1)
	{
		//Debug("check act","daemon.debug");
		memset(cmd,0,sizeof(cmd));
		sprintf(cmd,"cmdat %s 'AT+GTACT?' | grep OK |wc -l",path_usb);
		int tmp_int_buff = cmd_recieve_int(cmd);
		if (tmp_int_buff == 1)
			break;
	}
	
	memset(str_mode,0,sizeof(str_mode));
	strcat(str_mode,mode);
	memset(str_wcdma,0,sizeof(str_wcdma));
	memset(str_lte,0,sizeof(str_lte));
	memset(str_nr,0,sizeof(str_nr));
	if(strlen(wcdma)>0)
	{
		if(strcmp(wcdma,"0")==0)
			memset(str_wcdma,0,sizeof(str_wcdma));
		else
		{
			sprintf(str_wcdma,",%s",wcdma);
			replacestr(str_wcdma," ",",");
			//Debug(str_wcdma,"daemon.debug");
		}
	}
	if(strlen(lte)>0)
	{
		if(strcmp(lte,"0")==0)
			memset(str_lte,0,sizeof(str_lte));
		else
		{
			sprintf(str_lte,",%s",lte);
			replacestr(str_lte," ",",");
			//Debug(str_lte,"daemon.debug");
		}
	} 
	if(strlen(nr)>0)
	{
		if(strcmp(nr,"0")==0)
			memset(str_nr,0,sizeof(str_nr));
		else
		{
			sprintf(str_nr,",%s",nr);
			replacestr(str_nr," ",",");
			//Debug(str_nr,"daemon.debug");
		}
	}
	char tmp_mode[100];
	char tmp_band[200];
	memset(cmd,0,sizeof(cmd));
	sprintf(cmd,"cmdat %s \"AT+GTACT?\" 100 | grep \"+GTACT: \" | awk -F ' ' '{print$2}' | awk -F ',' -vOFS=\",\" '{$1=\"\";$2=\"\";$3=\"\";$1=$1;$2=$2;$3=$3}3' | sed 's/...//'",path_usb);
	memset(tmp_band,0,sizeof(tmp_band));
	cmd_recieve_str(cmd,tmp_band);
	
	memset(cmd,0,sizeof(cmd));
	sprintf(cmd,"cmdat %s \"AT+GTACT?\" 100 | grep \"+GTACT: \" | awk -F ' ' '{print$2}' | awk -F ',' '{print$1}'",path_usb);
	memset(tmp_mode,0,sizeof(tmp_mode));
	cmd_recieve_str(cmd,tmp_mode);
	if(strlen(str_wcdma) > 0 || strlen(str_lte) > 0 || strlen(str_nr) > 0)
	{
		memset(cmd,0,sizeof(cmd));
		sprintf(cmd,"cmdat %s AT+GTACT=%s,,%s%s%s",path_usb,str_mode,str_wcdma,str_lte,str_nr);
	}
	else
	{
		memset(cmd,0,sizeof(cmd));
		sprintf(cmd,"cmdat %s AT+GTACT=%s",path_usb,str_mode);
	}

	
	//Debug(cmd,"daemon.debug");
	//Debug(tmp_mode,"daemon.debug");
	if(strcmp(str_mode,tmp_mode)!=0 || ((strlen(str_wcdma) > 0 || strlen(str_lte) > 0 || strlen(str_nr) > 0) && !strstr(cmd,tmp_band)))
	{
		cmd_recieve_str(cmd,NULL);
		//setFlightmode();
		memset(tmp_buff,0,sizeof(tmp_buff));
		sprintf(tmp_buff,"AT+GTACT=%s%s%s%s",str_mode,str_wcdma,str_lte,str_nr);
		//Debug(cmd,"daemon.debug");
	}
	//获取当前拨号状态，处于拨号时挂断以重新拨号
	memset(cmd,0,sizeof(cmd));
	sprintf(cmd,"cmdat %s \"AT+GTRNDIS?\" 200 | grep \"+GTRNDIS: 1\" | wc -l",path_usb);
	int result=cmd_recieve_int(cmd);
	if(result == 1)
	{
		memset(cmd,0,sizeof(cmd));
		sprintf(cmd,"cmdat %s 'AT+GTRNDIS=0,1'",path_usb);
		memset(tmp_buff,0,sizeof(tmp_buff));
		cmd_recieve_str(cmd,tmp_buff);
		//Debug(cmd,"daemon.debug");
	}
	//Debug("setRatmod END","daemon.debug");
}

/* 功能：对fm160模组进行拨号
** 返回值：void
*/
void Dial_FM160()
{
	//Debug("Dial_FM160","daemon.debug");
	char cmd[1024];
	char buff[1024];
	//设置modem网卡信息：协议、网卡名和跃点数
	memset(cmd,0,sizeof(cmd));
	sprintf(cmd,"uci -q get network.%s",name_config);
	memset(buff,0,sizeof(buff));
	cmd_recieve_str(cmd,buff);
	if (strlen(buff)==0)
	{
		memset(cmd,0,sizeof(cmd));
		sprintf(cmd,"uci set network.%s=interface",name_config);
		system(cmd);
		memset(cmd,0,sizeof(cmd));
		sprintf(cmd,"uci set network.%s_6=interface",name_config);
		system(cmd);
	}
	memset(cmd,0,sizeof(cmd));
	sprintf(cmd,"uci set network.%s.proto='dhcp'",name_config);
	system(cmd);
	memset(cmd,0,sizeof(cmd));
	sprintf(cmd,"uci set network.%s_6.proto='dhcpv6'",name_config);
	system(cmd);
	memset(cmd,0,sizeof(cmd));
	sprintf(cmd,"uci set network.%s.device='%s'",name_config,value_ifname);
	system(cmd);
	memset(cmd,0,sizeof(cmd));
	sprintf(cmd,"uci set network.%s_6.device='%s'",name_config,value_ifname);
	system(cmd);
	memset(cmd,0,sizeof(cmd));
	if(value_metric != NULL && strlen(value_metric)>0)
	{
		sprintf(cmd,"uci set network.%s.metric='%s'",name_config,value_metric);
		system(cmd);
	}else{
		sprintf(cmd,"uci set network.%s.metric='50'",name_config);
		system(cmd);
	}
	memset(cmd,0,sizeof(cmd));
	if(value_mtu != NULL && strlen(value_mtu)>0)
	{
		sprintf(cmd,"uci set network.%s.mtu='%s'",name_config,value_mtu);
		system(cmd);
	}else{
		sprintf(cmd,"uci set network.%s.mtu='1500'",name_config);
		system(cmd);
	}
	memset(cmd,0,sizeof(cmd));
	strcpy(cmd,"uci commit network");
	system(cmd);
	//Debug("set network","daemon.debug");
	//获取SIM卡当前驻网状态
	char tmp_CEREG[1024];
	char tmp_CGREG[1024];
	char tmp_C5GREG[1024];
	memset(cmd,0,sizeof(cmd));
	sprintf(cmd,"cmdat %s 'AT+CEREG?'| grep \"+CEREG: \" | awk -F ' ' '{print$2}'",path_usb);
	cmd_recieve_str(cmd,tmp_CEREG);
	memset(cmd,0,sizeof(cmd));
	sprintf(cmd,"cmdat %s 'AT+CGREG?'| grep \"+CGREG: \" | awk -F ' ' '{print$2}'",path_usb);
	cmd_recieve_str(cmd,tmp_CGREG);
	memset(cmd,0,sizeof(cmd));
	sprintf(cmd,"cmdat %s 'AT+C5GREG?'| grep \"+C5GREG: \" | awk -F ' ' '{print$2}'",path_usb);
	cmd_recieve_str(cmd,tmp_C5GREG); 
	//Debug("check register","daemon.debug");
	int result;
	//CEREG：0,1 0,5 时尝试拨号
	if( strstr(tmp_C5GREG,"0,1") || strstr(tmp_C5GREG,"0,5") || strstr(tmp_C5GREG,"0,4") || strstr(tmp_CGREG,"0,1") || strstr(tmp_CGREG,"0,5") || strstr(tmp_CGREG,"0,4") || strstr(tmp_CEREG,"0,1") || strstr(tmp_CEREG,"0,5") || strstr(tmp_CEREG,"0,4"))
	{
		system(cmd_ifdown);
		system(cmd_ifup);
		
		if(flag_ims!=1)
		{
			memset(cmd,0,sizeof(cmd));
			sprintf(cmd,"cmdat %s \"AT+CAVIMS?\" | grep \"+CAVIMS: 1\" | wc -l",path_usb);
			int tmp_ims=cmd_recieve_int(cmd);
			if(tmp_ims==1)
			{
				memset(cmd,0,sizeof(cmd));
				sprintf(cmd,"cmdat %s \"AT+CAVIMS=0\"",path_usb);
				system(cmd);
			}
		}
		else
		{
			memset(cmd,0,sizeof(cmd));
			sprintf(cmd,"cmdat %s \"AT+CAVIMS?\" | grep \"+CAVIMS: 1\" | wc -l",path_usb);
			int tmp_ims=cmd_recieve_int(cmd);
			if(tmp_ims==0)
			{
				memset(cmd,0,sizeof(cmd));
				sprintf(cmd,"cmdat %s \"AT+CAVIMS=1\"",path_usb);
				system(cmd);
			}
		}
		memset(cmd,0,sizeof(cmd));
		sprintf(cmd,"cmdat %s \"AT+GTRNDIS=1,1\" > /dev/null 2>&1",path_usb);
		system(cmd);
		int checkip_count=0;
		//sleep(3);
		while(1)
		{
			sleep(1);
			//判断是否处于已拨号状态
			memset(cmd,0,sizeof(cmd));
			sprintf(cmd,"cmdat %s \"AT+GTRNDIS?\" | grep \"+GTRNDIS: 1\" | wc -l",path_usb);
			result=cmd_recieve_int(cmd);
			if(result ==1 )	//是，重启网卡
			{
				system(cmd_ifup);
				
				if(strcmp(dialresult_debug,"yes")!=0){
					char rndis[200];
					memset(rndis,0,sizeof(rndis));
					memset(cmd,0,sizeof(cmd));
					sprintf(cmd,"cmdat %s at+gtrndis? | grep \"+GTRNDIS:\" | awk -F ' ' '{print$NF}' | awk '{print substr($0, 5)}'",path_usb);
					cmd_recieve_str(cmd,rndis);
					char debug[300];
					memset(debug,0,sizeof(debug));
					sprintf(debug,"dial OK, %s",rndis);
					Debug(debug,"cellular.info");
					memset(dialresult_debug,0,sizeof(dialresult_debug));
					strcpy(dialresult_debug,"yes");
				}
				dial_flight=0;
				status_flight=0;
				return;
			}
			//Debug("checkip","daemon.debug");
			if(checkip_count==5)
				break;
			checkip_count++;
		}
		{				//否，等待23s后再次查询
			sleep(23);
			memset(cmd,0,sizeof(cmd));
			sprintf(cmd,"cmdat %s \"AT+GTRNDIS?\" | grep \"+GTRNDIS: 1\" | wc -l",path_usb);
			result=cmd_recieve_int(cmd);
			if(result ==0 )	//否，打印拨号失败标识
			{
				if(strcmp(dialresult_debug,"no")!=0)
				{
					//Debug("dial failed","cellular.error");
					memset(dialresult_debug,0,sizeof(dialresult_debug));
					strcpy(dialresult_debug,"no");
				}
				if(dial_flight<5){ //进入飞行模式，
					if(dial_flight==0)
						Debug("dial failed, reboot cellular network","cellular.error");
					setFlightmode();
					sleep(sleep_delay[dial_flight]);
					dial_flight++;
				}
				else if(dial_flight==5) //累计5次飞行模式后硬复位
				{
					Debug("dial failure reaches 5 times, reboot module","cellular.error");
					resetModem();
					dial_flight=0;
				}
			}
			else //是，打印拨号成功标识
			{
				if(strcmp(dialresult_debug,"yes")!=0)
				{
					char rndis[200];
					memset(rndis,0,sizeof(rndis));
					memset(cmd,0,sizeof(cmd));
					sprintf(cmd,"cmdat %s AT+GTRNDIS? | grep \"+GTRNDIS:\" | awk -F ' ' '{print$NF}' | awk '{print substr($0, 5)}'",path_usb);
					cmd_recieve_str(cmd,rndis);
					char debug[300];
					memset(debug,0,sizeof(debug));
					sprintf(debug,"dial OK, %s",rndis);
					Debug(debug,"cellular.info");
					memset(dialresult_debug,0,sizeof(dialresult_debug));
					strcpy(dialresult_debug,"yes");
				}
				
				dial_flight=0;
			}
		}
		status_flight=0;
	}
	else
	{
		//Debug("拨号","daemon.debug");
		char tmp_cgreg[100];
		char tmp_cereg[100];
		char tmp_c5greg[100];
		memset(tmp_cgreg,0,sizeof(tmp_cgreg));
		memset(tmp_cereg,0,sizeof(tmp_cereg));
		memset(tmp_c5greg,0,sizeof(tmp_c5greg));
		memset(cmd,0,sizeof(cmd));
		sprintf(cmd,"cmdat %s AT+CGREG? | grep \"+CGREG: \" | awk -F ' ' '{print$2}'",path_usb);
		cmd_recieve_str(cmd,tmp_cgreg);
		//Debug(tmp_cgreg,"daemon.debug");
		memset(cmd,0,sizeof(cmd));
		sprintf(cmd,"cmdat %s AT+CEREG? | grep \"+CEREG: \" | awk -F ' ' '{print$2}'",path_usb);
		cmd_recieve_str(cmd,tmp_cereg);
		//Debug(tmp_cereg,"daemon.debug");
		memset(cmd,0,sizeof(cmd));
		sprintf(cmd,"cmdat %s AT+C5GREG? | grep \"+C5GREG: \" | awk -F ' ' '{print$2}'",path_usb);
		cmd_recieve_str(cmd,tmp_c5greg);
		//Debug(tmp_c5greg,"daemon.debug");
		int goto_debug=0;
		if(strlen(tmp_cgreg)>0 && strlen(tmp_cereg)>0 && strlen(tmp_c5greg)>0)
		{
			if(strcmp(tmp_cgreg,debug_cgreg)!=0 || strcmp(tmp_cereg,debug_cereg)!=0 || strcmp(tmp_c5greg,debug_c5greg)!=0)
			{
				if(strcmp(tmp_cgreg,debug_cgreg)!=0)
				{
					sprintf(debug_cgreg,"%s",tmp_cgreg);
				}
				
				if(strcmp(tmp_cereg,debug_cereg)!=0)
				{
					sprintf(debug_cereg,"%s",tmp_cereg);
				}
				
				if(strcmp(tmp_c5greg,debug_c5greg)!=0)
				{
					sprintf(debug_c5greg,"%s",tmp_c5greg);
				}
				goto_debug=1;
			}
			if(goto_debug==1)
			{
				char debug[100];
				memset(debug,0,sizeof(debug));
				char stat_cgreg[1024];
				char stat_c5greg[1024];
				char stat_cereg[1024];
				memset(stat_cgreg,0,sizeof(stat_cgreg));
				memset(stat_c5greg,0,sizeof(stat_c5greg));
				memset(stat_cereg,0,sizeof(stat_cereg));
				trans_register_status_FM160(debug_cgreg,stat_cgreg);
				trans_register_status_FM160(debug_c5greg,stat_c5greg);
				trans_register_status_FM160(debug_cereg,stat_cereg);
				if(strcmp(debug_c5greg,debug_cgreg)==0 && strcmp(debug_c5greg,debug_cereg)==0)
					sprintf(debug,"5GS & EPS & GPRS: %s, reboot cellular network",stat_c5greg);
				else if(strcmp(debug_c5greg,debug_cereg)==0)
					sprintf(debug,"5GS & EPS: %s; GPRS: %s, reboot cellular network",stat_c5greg,stat_cgreg);
				else if(strcmp(debug_cereg,debug_cgreg)==0)
					sprintf(debug,"5GS: %s; EPS & GPRS: %s, reboot cellular network",stat_c5greg,stat_cereg);
				else
					sprintf(debug,"5GS: %s; EPS: %s; GPRS: %s, reboot cellular network",stat_c5greg,stat_cereg,stat_cgreg);
				Debug(debug,"cellular.error");
				debug_reboot_status=1;
			}
			setFlightmode();
			//sleep(sleep_delay[status_flight]);
			sleep_delay_by_registation();
			status_flight++;
			if(status_flight>5)
			{
				if(debug_reboot_status==1)
				{
					Debug("registration failure reaches 5 times, reboot module","cellular.error");
					debug_reboot_status=0;
				}
				resetModem();
				sleep_delay_by_registation();
				status_flight=0;
			}
		}
	}
	return;
}

void Dial_FM160_PRE()
{
	//Debug("Dial_FM160","daemon.debug");
	char cmd[1024];
	char buff[1024];
	//设置modem网卡信息：协议、网卡名和跃点数
	memset(cmd,0,sizeof(cmd));
	
	{
		sprintf(cmd,"uci -q get network.%s",name_config);
		memset(buff,0,sizeof(buff));
		cmd_recieve_str(cmd,buff);
		if (strlen(buff)==0)
		{
			memset(cmd,0,sizeof(cmd));
			sprintf(cmd,"uci set network.%s=interface",name_config);
			system(cmd);
			memset(cmd,0,sizeof(cmd));
			sprintf(cmd,"uci set network.%s_6=interface",name_config);
			system(cmd);
		}
		memset(cmd,0,sizeof(cmd));
		sprintf(cmd,"uci set network.%s.proto='dhcp'",name_config);
		system(cmd);
		memset(cmd,0,sizeof(cmd));
		sprintf(cmd,"uci set network.%s_6.proto='dhcpv6'",name_config);
		system(cmd);
		memset(cmd,0,sizeof(cmd));
		sprintf(cmd,"uci set network.%s.device='%s'",name_config,value_ifname);
		system(cmd);
		memset(cmd,0,sizeof(cmd));
		sprintf(cmd,"uci set network.%s_6.device='%s'",name_config,value_ifname);
		system(cmd);
		memset(cmd,0,sizeof(cmd));
		if(value_metric != NULL && strlen(value_metric)>0)
		{
			sprintf(cmd,"uci set network.%s.metric='%s'",name_config,value_metric);
			system(cmd);
		}else{
			sprintf(cmd,"uci set network.%s.metric='50'",name_config);
			system(cmd);
		}
		memset(cmd,0,sizeof(cmd));
		if(value_mtu != NULL && strlen(value_mtu)>0)
		{
			sprintf(cmd,"uci set network.%s.mtu='%s'",name_config,value_mtu);
			system(cmd);
		}else{
			sprintf(cmd,"uci set network.%s.mtu='1500'",name_config);
			system(cmd);
		}
	}
	memset(cmd,0,sizeof(cmd));
	strcpy(cmd,"uci commit network");
	system(cmd);
	if(flag_ims!=1)
	{
		memset(cmd,0,sizeof(cmd));
		sprintf(cmd,"cmdat %s \"AT+CAVIMS?\" | grep \"+CAVIMS: 1\" | wc -l",path_usb);
		int tmp_ims=cmd_recieve_int(cmd);
		if(tmp_ims==1)
		{
			memset(cmd,0,sizeof(cmd));
			sprintf(cmd,"cmdat %s \"AT+CAVIMS=0\"",path_usb);
			system(cmd);
		}
	}
	else
	{
		memset(cmd,0,sizeof(cmd));
		sprintf(cmd,"cmdat %s \"AT+CAVIMS?\" | grep \"+CAVIMS: 1\" | wc -l",path_usb);
		int tmp_ims=cmd_recieve_int(cmd);
		if(tmp_ims==0)
		{
			memset(cmd,0,sizeof(cmd));
			sprintf(cmd,"cmdat %s \"AT+CAVIMS=1\"",path_usb);
			system(cmd);
		}
	}
	{
		system(cmd_ifdown);
		system(cmd_ifup);
		memset(cmd,0,sizeof(cmd));
		sprintf(cmd,"cmdat %s \"AT+GTRNDIS=1,1\" > /dev/null 2>&1",path_usb);
		system(cmd);
		int checkip_count=0;
		//sleep(3);
		int result;
		while(1)
		{
			sleep(1);
			//判断是否处于已拨号状态
			memset(cmd,0,sizeof(cmd));
			sprintf(cmd,"cmdat %s \"AT+GTRNDIS?\" | grep \"+GTRNDIS: 1\" | wc -l",path_usb);
			result=cmd_recieve_int(cmd);
			if(result ==1 )	//是，重启网卡
			{
				system(cmd_ifup);
				
				if(strcmp(dialresult_debug,"yes")!=0){
					char rndis[200];
					memset(rndis,0,sizeof(rndis));
					memset(cmd,0,sizeof(cmd));
					sprintf(cmd,"cmdat %s at+gtrndis? | grep \"+GTRNDIS:\" | awk -F ' ' '{print$NF}' | awk '{print substr($0, 5)}'",path_usb);
					cmd_recieve_str(cmd,rndis);
					char debug[300];
					memset(debug,0,sizeof(debug));
					sprintf(debug,"dial OK, %s",rndis);
					Debug(debug,"cellular.info");
					memset(dialresult_debug,0,sizeof(dialresult_debug));
					strcpy(dialresult_debug,"yes");
				}
				dial_flight=0;
				status_flight=0;
				return;
			}
			//Debug("checkip","daemon.debug");
			if(checkip_count==5)
				break;
			checkip_count++;
		}
		{				//否，等待23s后再次查询
			sleep(23);
			memset(cmd,0,sizeof(cmd));
			sprintf(cmd,"cmdat %s \"AT+GTRNDIS?\" | grep \"+GTRNDIS: 1\" | wc -l",path_usb);
			result=cmd_recieve_int(cmd);
			if(result ==0 )	//否，打印拨号失败标识
			{
				if(strcmp(dialresult_debug,"no")!=0)
				{
					//Debug("dial failed","cellular.error");
					memset(dialresult_debug,0,sizeof(dialresult_debug));
					strcpy(dialresult_debug,"no");
				}
				if(dial_flight<5){ //进入飞行模式，
					if(dial_flight==0)
						Debug("dial failed, reboot cellular network","cellular.error");
					setFlightmode();
					sleep(sleep_delay[dial_flight]);
					dial_flight++;
				}
				else if(dial_flight==5) //累计5次飞行模式后硬复位
				{
					Debug("dial failure reaches 5 times, reboot module","cellular.error");
					resetModem();
					dial_flight=0;
				}
			}
			else //是，打印拨号成功标识
			{
				if(strcmp(dialresult_debug,"yes")!=0)
				{
					char rndis[200];
					memset(rndis,0,sizeof(rndis));
					memset(cmd,0,sizeof(cmd));
					sprintf(cmd,"cmdat %s AT+GTRNDIS? | grep \"+GTRNDIS:\" | awk -F ' ' '{print$NF}' | awk '{print substr($0, 5)}'",path_usb);
					cmd_recieve_str(cmd,rndis);
					char debug[300];
					memset(debug,0,sizeof(debug));
					sprintf(debug,"dial OK, %s",rndis);
					Debug(debug,"cellular.info");
					memset(dialresult_debug,0,sizeof(dialresult_debug));
					strcpy(dialresult_debug,"yes");
				}
				dial_flight=0;
			}
		}
		status_flight=0;
	}
	
	return;
}

void Dial_FM160_PCI(char *apn,char *username,char *password,char *auth,char *ip)
{
	int count=0;
	char cmd[1024];
	char buff[1024];
	//设置modem网卡信息：协议、网卡名和跃点数
	memset(cmd,0,sizeof(cmd));
	sprintf(cmd,"uci -q get network.%s",name_config);
	memset(buff,0,sizeof(buff));
	cmd_recieve_str(cmd,buff);
	if (strlen(buff)==0)
	{
		memset(cmd,0,sizeof(cmd));
		sprintf(cmd,"uci set network.%s=interface",name_config);
		cmd_recieve_str(cmd,NULL);
		memset(cmd,0,sizeof(cmd));
		sprintf(cmd,"uci set network.%s_6=interface",name_config);
		cmd_recieve_str(cmd,NULL);
	}
	memset(cmd,0,sizeof(cmd));
	sprintf(cmd,"uci set network.%s.proto='dhcp'",name_config);
	cmd_recieve_str(cmd,NULL);
	memset(cmd,0,sizeof(cmd));
	sprintf(cmd,"uci set network.%s_6.proto='dhcpv6'",name_config);
	cmd_recieve_str(cmd,NULL);
	memset(cmd,0,sizeof(cmd));
	sprintf(cmd,"uci set network.%s.device='%s'",name_config,value_ifname);
	cmd_recieve_str(cmd,NULL);
	memset(cmd,0,sizeof(cmd));
	sprintf(cmd,"uci set network.%s_6.device='%s'",name_config,value_ifname);
	cmd_recieve_str(cmd,NULL);
	memset(cmd,0,sizeof(cmd));
	if(value_metric != NULL && strlen(value_metric)>0)
	{
		sprintf(cmd,"uci set network.%s.metric='%s'",name_config,value_metric);
		cmd_recieve_str(cmd,NULL);
	}else{
		sprintf(cmd,"uci set network.%s.metric='50'",name_config);
		cmd_recieve_str(cmd,NULL);
	}
	memset(cmd,0,sizeof(cmd));
	if(value_mtu != NULL && strlen(value_mtu)>0)
	{
		sprintf(cmd,"uci set network.%s.mtu='%s'",name_config,value_mtu);
		cmd_recieve_str(cmd,NULL);
	}else{
		sprintf(cmd,"uci set network.%s.mtu='1500'",name_config);
		cmd_recieve_str(cmd,NULL);
	}
	memset(cmd,0,sizeof(cmd));
	strcpy(cmd,"uci commit network");
	cmd_recieve_str(cmd,NULL);
	memset(cmd,0,sizeof(cmd));
	sprintf(cmd,"cmdat %s 'AT+CEREG?'| grep \"0,1\" | wc -l",path_usb);
	int result1=cmd_recieve_int(cmd);
	memset(cmd,0,sizeof(cmd));
	sprintf(cmd,"cmdat %s 'AT+CEREG?'| grep \"0,5\" | wc -l",path_usb);
	int result2=cmd_recieve_int(cmd);
	memset(cmd,0,sizeof(cmd));
	sprintf(cmd,"cmdat %s 'AT+CEREG?'| grep \"0,4\" | wc -l",path_usb);
	int result3=cmd_recieve_int(cmd);
	memset(cmd,0,sizeof(cmd));
	sprintf(cmd,"cmdat %s 'AT+CGREG?'| grep \"0,1\" | wc -l",path_usb);
	int result4=cmd_recieve_int(cmd);
	memset(cmd,0,sizeof(cmd));
	sprintf(cmd,"cmdat %s 'AT+CGREG?'| grep \"0,5\" | wc -l",path_usb);
	int result5=cmd_recieve_int(cmd);
	memset(cmd,0,sizeof(cmd));
	sprintf(cmd,"cmdat %s 'AT+CGREG?'| grep \"0,4\" | wc -l",path_usb);
	int result6=cmd_recieve_int(cmd);
	memset(cmd,0,sizeof(cmd));
	sprintf(cmd,"cmdat %s 'AT+C5GREG?'| grep \"0,1\" | wc -l",path_usb);
	int result7=cmd_recieve_int(cmd);
	memset(cmd,0,sizeof(cmd));
	sprintf(cmd,"cmdat %s 'AT+C5GREG?'| grep \"0,5\" | wc -l",path_usb);
	int result8=cmd_recieve_int(cmd);
	memset(cmd,0,sizeof(cmd));
	sprintf(cmd,"cmdat %s 'AT+C5GREG?'| grep \"0,4\" | wc -l",path_usb);
	int result9=cmd_recieve_int(cmd);
	//CEREG：0,1 0,5 时尝试拨号
	if( result1 ==1 || result2 ==1 || result3 ==1 || result4 ==1 || result5 ==1 || result6 ==1 || result7 ==1 || result8 ==1 || result9 ==1)
	{
		
		if(flag_ims!=1)
		{
			memset(cmd,0,sizeof(cmd));
			sprintf(cmd,"cmdat %s \"AT+CAVIMS?\" | grep \"+CAVIMS: 1\" | wc -l",path_usb);
			int tmp_ims=cmd_recieve_int(cmd);
			if(tmp_ims==1)
			{
				memset(cmd,0,sizeof(cmd));
				sprintf(cmd,"cmdat %s \"AT+CAVIMS=0\"",path_usb);
				system(cmd);
			}
		}
		else
		{
			memset(cmd,0,sizeof(cmd));
			sprintf(cmd,"cmdat %s \"AT+CAVIMS?\" | grep \"+CAVIMS: 1\" | wc -l",path_usb);
			int tmp_ims=cmd_recieve_int(cmd);
			if(tmp_ims==0)
			{
				memset(cmd,0,sizeof(cmd));
				sprintf(cmd,"cmdat %s \"AT+CAVIMS=1\"",path_usb);
				system(cmd);
			}
		}
		char tmp_type_ip[20];
		memset(tmp_type_ip,0,sizeof(tmp_type_ip));
		sprintf(tmp_type_ip,"");
		if(strstr(ip,"IPV4V6"))
			sprintf(tmp_type_ip,"-4 -6");
		else if (strstr(ip,"IPV4"))
			sprintf(tmp_type_ip,"");
		else if(strstr(ip,"IPV6"))
			sprintf(tmp_type_ip,"-6");
		if(check_pcie_ko())
		{
			while(1)
			{
				memset(cmd,0,sizeof(cmd));
				sprintf(cmd,"cmdat /dev/mhi_DUN 'AT' | grep OK |wc -l");
				int tmp_int_buff = cmd_recieve_int(cmd);
				//Debug("cmdat mhi_DUN","daemon.debug");
				if (tmp_int_buff == 1)
					break;
				sleep(1);
			}
			if(apn != NULL && strlen(apn) > 0)
			{
				if(username != NULL && strlen(username) > 0)
				{
					memset(cmd,0,sizeof(cmd));
					sprintf(cmd,"fibocom-dial -d /dev/mhi_QMI0 -s %s %s %s %s %s &",apn,username,password,auth,tmp_type_ip);
					system(cmd);
				}else{
					memset(cmd,0,sizeof(cmd));
					sprintf(cmd,"fibocom-dial -d /dev/mhi_QMI0 -s %s %s &",apn,tmp_type_ip);
					system(cmd);
				}
			}
			else
			{
				memset(cmd,0,sizeof(cmd));
				sprintf(cmd,"fibocom-dial -d /dev/mhi_QMI0 %s &",tmp_type_ip);
				system(cmd);
			}
			int count_checkip=0;
			cmd_recieve_str(cmd_ifup,NULL);
			while(1) //执行quectel后循环查询网卡是否存在IP来确定是否要打印拨号成功标识
			{
				char value_ip_ifconfig[500];
				memset(cmd,0,sizeof(cmd));
				sprintf(cmd,"ifconfig %s | grep \"inet addr\" | awk -F ':' '{print$2}' | awk -F ' ' '{print$1}'",value_ifname);
				cmd_recieve_str(cmd,value_ip_ifconfig);
				//不存在网卡IP时，尝试拨号
				if(value_ip_ifconfig == NULL || strlen(value_ip_ifconfig)==0)
				{
					sleep(2);
					//额外作读卡判断
					memset(cmd,0,sizeof(cmd));
					sprintf(cmd,"cmdat %s 'AT+CPIN?' | grep \"+CPIN:\"",path_usb);
					memset(buff,0,sizeof(buff));
					cmd_recieve_str(cmd,buff);
					if(!strstr(buff,"READY"))
					{
						return;
					}
					if(count_checkip==5 || count_checkip==60)
					{
						if(strcmp(dialresult_debug,"no")!=0){
							Debug("dial failed","cellular.error");
							memset(dialresult_debug,0,sizeof(dialresult_debug));
							strcpy(dialresult_debug,"no");
						}
						//在执行quectel-CM后3分钟内无法获取到IP地址视为拨号失败，执行飞行模式，睡眠时间依然按照sleep_delay循环
						setFlightmode();
						sleep(sleep_delay[dial_flight]);
						dial_flight++;
						if(dial_flight>5)
							dial_flight=0;
						return;
					}
					count_checkip++;
				}
				else
				{
					if(strcmp(dialresult_debug,"yes")!=0){
						char debug[300];
						memset(debug,0,sizeof(debug));
						char value_ipv6_ifconfig[500];
						char value_dnsv4_1[500];
						char value_dnsv4_2[500];
						char value_dnsv6_1[500];
						char value_dnsv6_2[500];
						memset(value_ipv6_ifconfig,0,sizeof(value_ipv6_ifconfig));
						memset(value_dnsv4_1,0,sizeof(value_dnsv4_1));
						memset(value_dnsv4_2,0,sizeof(value_dnsv4_2));
						memset(value_dnsv6_1,0,sizeof(value_dnsv6_1));
						memset(value_dnsv6_2,0,sizeof(value_dnsv6_2));
						memset(cmd,0,sizeof(cmd));
						sprintf(cmd,"ifconfig %s | grep \"inet6 addr\" | grep Global | awk -F ': ' '{print$2}' | awk -F ' ' '{print$1}'",value_ifname);
						cmd_recieve_str(cmd,value_ipv6_ifconfig);
						char path_resolvfile[100];
						memset(path_resolvfile,0,sizeof(path_resolvfile));
						cmd_recieve_str("uci -q get dhcp.@dnsmasq[0].resolvfile",path_resolvfile);
						if(path_resolvfile!=NULL || strlen(path_resolvfile)>0)
						{
							memset(cmd,0,sizeof(cmd));
							sprintf(cmd,"cat %s | grep -w -A 2 \"%s\" | grep nameserver | sed -n 1p | awk -F ' ' '{print$NF}'",path_resolvfile,name_config);
							cmd_recieve_str(cmd,value_dnsv4_1);
							memset(cmd,0,sizeof(cmd));
							sprintf(cmd,"cat %s | grep -w -A 2 \"%s\" | grep nameserver | sed -n 2p | awk -F ' ' '{print$NF}'",path_resolvfile,name_config);
							cmd_recieve_str(cmd,value_dnsv4_2);
							memset(cmd,0,sizeof(cmd));
							sprintf(cmd,"cat %s | grep -w -A 2 \"%s_6\" | grep nameserver | sed -n 1p | awk -F ' ' '{print$NF}'",path_resolvfile,name_config);
							cmd_recieve_str(cmd,value_dnsv6_1);
							memset(cmd,0,sizeof(cmd));
							sprintf(cmd,"cat %s | grep -w -A 2 \"%s_6\" | grep nameserver | sed -n 2p | awk -F ' ' '{print$NF}'",path_resolvfile,name_config);
							cmd_recieve_str(cmd,value_dnsv6_2);
						}
						sprintf(debug,"dial OK, %s,%s,%s,%s,%s,%s",value_ip_ifconfig,value_ipv6_ifconfig,value_dnsv4_1,value_dnsv6_1,value_dnsv4_2,value_dnsv6_2);
						Debug(debug,"cellular.info");
						memset(dialresult_debug,0,sizeof(dialresult_debug));
						strcpy(dialresult_debug,"yes");
						system("/etc/init.d/firewall restart &");
					}
					dial_flight=0;
					return;
				}
			}
		}
	}
	else
	{
		char tmp_cgreg[100];
		char tmp_cereg[100];
		char tmp_c5greg[100];
		memset(tmp_cgreg,0,sizeof(tmp_cgreg));
		memset(tmp_cereg,0,sizeof(tmp_cereg));
		memset(tmp_c5greg,0,sizeof(tmp_c5greg));
		memset(cmd,0,sizeof(cmd));
		sprintf(cmd,"cmdat %s AT+CGREG? | grep \"+CGREG: \" | awk -F ' ' '{print$2}'",path_usb);
		cmd_recieve_str(cmd,tmp_cgreg);
		memset(cmd,0,sizeof(cmd));
		sprintf(cmd,"cmdat %s AT+CEREG? | grep \"+CEREG: \" | awk -F ' ' '{print$2}'",path_usb);
		cmd_recieve_str(cmd,tmp_cereg);
		memset(cmd,0,sizeof(cmd));
		sprintf(cmd,"cmdat %s AT+C5GREG? | grep \"+C5GREG: \" | awk -F ' ' '{print$2}'",path_usb);
		cmd_recieve_str(cmd,tmp_c5greg);
		
		if(strcmp(tmp_cgreg,debug_cgreg)!=0 || strcmp(tmp_cereg,debug_cereg)!=0 || strcmp(tmp_c5greg,debug_c5greg)!=0)
		{
			if(strcmp(tmp_cgreg,debug_cgreg)!=0)
			{
				sprintf(debug_cgreg,"%s",tmp_cgreg);
			}
			
			if(strcmp(tmp_cereg,debug_cereg)!=0)
			{
				sprintf(debug_cereg,"%s",tmp_cereg);
			}
			
			if(strcmp(tmp_c5greg,debug_c5greg)!=0)
			{
				sprintf(debug_c5greg,"%s",tmp_c5greg);
			}
			
			char debug[100];
			memset(debug,0,sizeof(debug));
			char stat_cgreg[1024];
			char stat_c5greg[1024];
			char stat_cereg[1024];
			memset(stat_cgreg,0,sizeof(stat_cgreg));
			memset(stat_c5greg,0,sizeof(stat_c5greg));
			memset(stat_cereg,0,sizeof(stat_cereg));
			trans_register_status_RG520N(debug_cgreg,stat_cgreg);
			trans_register_status_RG520N(debug_c5greg,stat_c5greg);
			trans_register_status_RG520N(debug_cereg,stat_cereg);
			if(strcmp(debug_c5greg,debug_cgreg)==0 && strcmp(debug_c5greg,debug_cereg)==0)
				sprintf(debug,"5GS & EPS & GPRS: %s, reboot cellular network",stat_c5greg);
			else if(strcmp(debug_c5greg,debug_cereg)==0)
				sprintf(debug,"5GS & EPS: %s; GPRS: %s, reboot cellular network",stat_c5greg,stat_cgreg);
			else if(strcmp(debug_cereg,debug_cgreg)==0)
				sprintf(debug,"5GS: %s; EPS & GPRS: %s, reboot cellular network",stat_c5greg,stat_cereg);
			else
				sprintf(debug,"5GS: %s; EPS: %s; GPRS: %s, reboot cellular network",stat_c5greg,stat_cereg,stat_cgreg);
			Debug(debug,"cellular.error");
		}
		setFlightmode();
		sleep(sleep_delay[status_flight]);
		status_flight++;
		if(status_flight>5)
		{
			status_flight=0;
		}
	}
}

void trans_register_status_FM160(char *reg,char *status)
{
	if(strcmp(reg,"0,0")==0)
	{
		sprintf(status,"not currently searching an operator to register to");
	}
	else if(strcmp(reg,"0,1")==0)
	{
		sprintf(status,"registered home network");
	}
	else if(strcmp(reg,"0,2")==0)
	{
		sprintf(status,"currently trying to attach or searching an operator to register to");
	}
	else if(strcmp(reg,"0,3")==0)
	{
		sprintf(status,"registration denied");
	}
	else if(strcmp(reg,"0,4")==0)
	{
		sprintf(status,"unknown");
	}
	else if(strcmp(reg,"0,5")==0)
	{
		sprintf(status,"registered roaming");
	}
	else if(strcmp(reg,"0,6")==0)
	{
		sprintf(status,"registered for \"SMS only\", home network (not applicable)");
	}
	else if(strcmp(reg,"0,7")==0)
	{
		sprintf(status,"registered for \"SMS only\", home network (not applicable)");
	}
	else if(strcmp(reg,"0,8")==0)
	{
		sprintf(status,"attached for emergency bearer services only");
	}
	else if(strcmp(reg,"0,9")==0)
	{
		sprintf(status,"registered for \"CSFB not preferred\", home network (not applicable)");
	}
	else if(strcmp(reg,"0,10")==0)
	{
		sprintf(status,"registered for \"CSFB not preferred\", roaming (not applicable)");
	}
}