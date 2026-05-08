#include "dialing.h"
/* 功能：设定rm500u模组拨号方式
** 返回值：void
*/
void setModule_RM500U(char *tmp_mode_usb,char *tmp_mode_nat)
{
	char cmd[1024];
	int tmp_int_buff;
	char tmp_str_buff[1024];
	char debug[100];
	char tmp_debug[100];
	memset(debug,0,sizeof(debug));
	memset(tmp_debug,0,sizeof(tmp_debug));
	//确认模组正常工作
	while(1)
	{
		memset(cmd,0,sizeof(cmd));
		sprintf(cmd,"cmdat %s 'AT' | grep OK |wc -l",path_usb);
		tmp_int_buff = cmd_recieve_int(cmd);
		if (tmp_int_buff == 1)
			break;
	}
	//设定拨号上网驱动
	if( tmp_mode_usb != NULL )
	{
		if(strlen(tmp_mode_usb)>0)
		{
			memset(cmd,0,sizeof(cmd));
			sprintf(cmd,"cmdat %s 'AT+QCFG=\"usbnet\"' | grep '\"usbnet\",%s' | wc -l",path_usb,tmp_mode_usb);
			tmp_int_buff = cmd_recieve_int(cmd);
			//当前拨号上网驱动与设定方式不一致时进行设定
			if (tmp_int_buff == 0)
			{
				memset(cmd,0,sizeof(cmd));
				sprintf(cmd,"cmdat %s 'AT+QCFG=\"usbnet\",%s' | grep \"OK\" |wc -l",path_usb,tmp_mode_usb);
				tmp_int_buff = cmd_recieve_int(cmd);
				if(tmp_int_buff == 1)
				{
					sprintf(tmp_debug,"set usbmode: %s OK; ",tmp_mode_usb);
					//memset(cmd,0,sizeof(cmd));
					//sprintf(cmd,"cmdat %s AT+CFUN=1,1",path_usb);
					sleep(60);
				}
				else
					sprintf(tmp_debug,"set usbmode: %s failed; ",tmp_mode_usb);
					
			}else{	//一致时，打印当前上网方式
				memset(cmd,0,sizeof(cmd));
				sprintf(cmd,"cmdat %s 'AT+QCFG=\"usbnet\"' | grep \"+GTUSBMODE: \" | awk -F ' ' '{print$2}'",path_usb);
				memset(tmp_str_buff,0,sizeof(tmp_str_buff));
				cmd_recieve_str(cmd,tmp_str_buff);
				sprintf(tmp_debug,"usbmode: %s OK; ",tmp_mode_usb);
				//Debug(tmp_str_buff,"AT+QCFG=usbnet");
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
			sprintf(cmd,"cmdat %s 'AT+QCFG=\"nat\"' | grep '\"nat\",%s' | wc -l",path_usb,tmp_mode_nat);
			tmp_int_buff = cmd_recieve_int(cmd);
			//当前上网方式与设定方式不一致时进行设定
			if (tmp_int_buff == 0)
			{
				memset(cmd,0,sizeof(cmd));
				sprintf(cmd,"cmdat %s 'AT+QCFG=\"nat\",%s' | grep \"OK\" | wc -l",path_usb,tmp_mode_nat);
				tmp_int_buff = cmd_recieve_int(cmd);
				if(tmp_int_buff == 1)
					sprintf(tmp_debug,"set ippass: %s OK; ",tmp_mode_nat);
				else
					sprintf(tmp_debug,"set ippass: %s failed; ",tmp_mode_nat);
					
			}else{	//一致时，打印当前上网方式
				memset(cmd,0,sizeof(cmd));
				sprintf(cmd,"cmdat %s 'AT+QCFG=\"nat\"' | grep '+QCFG: \"nat\"' | awk -F ',' '{print$2}'",path_usb);
				memset(tmp_str_buff,0,sizeof(tmp_str_buff));
				cmd_recieve_str(cmd,tmp_str_buff);
				sprintf(tmp_debug,"ippass: %s OK; ",tmp_mode_nat);
			}
		}
	}
	if(strlen(tmp_debug)>0)
	{
		strcat(debug,tmp_debug);
		memset(tmp_debug,0,sizeof(tmp_debug));
	}
	//设置支持SIM卡热插拔
	memset(cmd,0,sizeof(cmd));
	sprintf(cmd,"cmdat %s 'AT+QSIMDET?' | grep '+QSIMDET: 1,1' | wc -l",path_usb);
	tmp_int_buff = cmd_recieve_int(cmd);
	//当前不支持热插拔时
	if (tmp_int_buff == 0)
	{
		memset(cmd,0,sizeof(cmd));
		sprintf(cmd,"cmdat %s 'AT+QSIMDET=1,1' | grep \"OK\" | wc -l",path_usb,tmp_mode_nat);
		tmp_int_buff = cmd_recieve_int(cmd);
		if(tmp_int_buff == 1)
			sprintf(tmp_debug,"set card hot-plug OK; ");
		else
			sprintf(tmp_debug,"set card hot-plug failed; ");
			
	}else{	//支持热插拔时
		sprintf(tmp_debug,"card hot-plug OK; ");
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
	/*
	memset(cmd,0,sizeof(cmd));
	sprintf(cmd,"cmdat %s 'AT+QUIMSLOT?' | grep \"+QUIMSLOT:\" | awk -F ' ' '{print $2}'",path_usb);
	int result = cmd_recieve_int(cmd);
	//printf("Current card slot:%d\n",result);
	if(value_defaultcard == result) //[优先卡槽] == [当前卡槽]
	{
		memset(tmp_str_buff,0,sizeof(tmp_str_buff));
		sprintf(tmp_str_buff,"%d",result);
		Debug(tmp_str_buff,"AT+QUIMSLOT");
	}else
	{
		memset(cmd,0,sizeof(cmd));
		sprintf(cmd,"cmdat %s 'AT+QUIMSLOT=%d'",path_usb,value_defaultcard);
		cmd_recieve_str(cmd,NULL);
		memset(tmp_str_buff,0,sizeof(tmp_str_buff));
		sprintf(tmp_str_buff,"%d",value_defaultcard);
		Debug(tmp_str_buff,"Set AT+QUIMSLOT");
		sleep(5);
	}*/
	int result;
	if(strstr(value_defaultcard,"auto") || strstr(value_defaultcard,"sim1"))
		{
			memset(cmd,0,sizeof(cmd));
			if(strstr(board_name,"R680-8TH") || strstr(board_name,"R690") || strstr(board_name,"RT990"))
			{
				//gpio控制读卡先检测模组是否为默认读卡位置
				sprintf(cmd,"cmdat %s 'AT+QUIMSLOT?' | grep \"+QUIMSLOT:\" | awk -F ' ' '{print $2}'",path_usb);
				result = cmd_recieve_int(cmd);
				if (result != 1) 
				{
					memset(cmd,0,sizeof(cmd));
					sprintf(cmd,"cmdat %s 'AT+QUIMSLOT=1'",path_usb);
					cmd_recieve_str(cmd,NULL);
					//sleep(1);
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
					setModem1();
				}else
				{
					memset(cmd,0,sizeof(cmd));
					sprintf(cmd,"echo 1 > /sys/class/leds/sim_sw/brightness");
					cmd_recieve_str(cmd,NULL);
					sprintf(tmp_str_buff,"%s",value_defaultcard);
					//sleep(1);
					setFlightmode();
					setModem1();
				}
			}
			else if(strstr(board_name,"R660"))
			{
				//gpio控制读卡先检测模组是否为默认读卡位置
				sprintf(cmd,"cmdat %s 'AT+QUIMSLOT?' | grep \"+QUIMSLOT:\" | awk -F ' ' '{print $2}'",path_usb);
				result = cmd_recieve_int(cmd);
				if (result != 1) 
				{
					memset(cmd,0,sizeof(cmd));
					sprintf(cmd,"cmdat %s 'AT+QUIMSLOT=1'",path_usb);
					cmd_recieve_str(cmd,NULL);
					//sleep(1);
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
					setModem1();
				}else
				{
					memset(cmd,0,sizeof(cmd));
					sprintf(cmd,"echo 1 > /sys/class/gpio/sim_sw/value");
					cmd_recieve_str(cmd,NULL);
					sprintf(tmp_str_buff,"%s",value_defaultcard);
					//sleep(1);
					setFlightmode();
					setModem1();
				}
			}
			else if(strstr(board_name,"R650") || strstr(board_name,"R680"))
			{
				sprintf(cmd,"cmdat %s 'AT+QUIMSLOT?' | grep \"+QUIMSLOT:\" | awk -F ' ' '{print $2}'",path_usb);
				result = cmd_recieve_int(cmd);
				printf("Current card slot:%d\n",result);
				if (result == 1) //[优先卡槽] == [当前卡槽]
				{
					sprintf(tmp_str_buff,"%s",value_defaultcard);
					setModem1();
				}else
				{
					memset(cmd,0,sizeof(cmd));
					sprintf(cmd,"cmdat %s 'AT+QUIMSLOT=1'",path_usb);
					cmd_recieve_str(cmd,NULL);
					sprintf(tmp_str_buff,"%s",value_defaultcard);
					//sleep(1);
					setFlightmode();
					setModem1();
				}
			}
		}
		else if(strstr(value_defaultcard,"sim2"))
		{
			memset(cmd,0,sizeof(cmd));
			if(strstr(board_name,"R680-8TH") || strstr(board_name,"R690") || strstr(board_name,"RT990"))
			{
				//gpio控制读卡先检测模组是否为默认读卡位置
				sprintf(cmd,"cmdat %s 'AT+QUIMSLOT?' | grep \"+QUIMSLOT:\" | awk -F ' ' '{print $2}'",path_usb);
				result = cmd_recieve_int(cmd);
				if (result != 1) 
				{
					memset(cmd,0,sizeof(cmd));
					sprintf(cmd,"cmdat %s 'AT+QUIMSLOT=1'",path_usb);
					cmd_recieve_str(cmd,NULL);
					//sleep(1);
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
					setModem2();
				}else
				{
					memset(cmd,0,sizeof(cmd));
					sprintf(cmd,"echo 0 > /sys/class/leds/sim_sw/brightness");
					cmd_recieve_str(cmd,NULL);
					sprintf(tmp_str_buff,"%s",value_defaultcard);
					//sleep(1);
					setFlightmode();
					setModem2();
				}
			}
			else if(strstr(board_name,"R660"))
			{
				//gpio控制读卡先检测模组是否为默认读卡位置
				sprintf(cmd,"cmdat %s 'AT+QUIMSLOT?' | grep \"+QUIMSLOT:\" | awk -F ' ' '{print $2}'",path_usb);
				result = cmd_recieve_int(cmd);
				if (result != 1) 
				{
					memset(cmd,0,sizeof(cmd));
					sprintf(cmd,"cmdat %s 'AT+QUIMSLOT=1'",path_usb);
					cmd_recieve_str(cmd,NULL);
					//sleep(1);
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
					setModem2();
				}else
				{
					memset(cmd,0,sizeof(cmd));
					sprintf(cmd,"echo 0 > /sys/class/gpio/sim_sw/value");
					cmd_recieve_str(cmd,NULL);
					sprintf(tmp_str_buff,"%s",value_defaultcard);
					//sleep(1);
					setFlightmode();
					setModem2();
				}
			}
			else if(strstr(board_name,"R650") || strstr(board_name,"R680"))
			{
				sprintf(cmd,"cmdat %s 'AT+QUIMSLOT?' | grep \"+QUIMSLOT:\" | awk -F ' ' '{print $2}'",path_usb);
				result = cmd_recieve_int(cmd);
				printf("Current card slot:%d\n",result);
				if (result == 2) //[优先卡槽] == [当前卡槽]
				{
					sprintf(tmp_str_buff,"%s",value_defaultcard);
					setModem2();
				}else
				{
					memset(cmd,0,sizeof(cmd));
					sprintf(cmd,"cmdat %s 'AT+QUIMSLOT=2'",path_usb);
					cmd_recieve_str(cmd,NULL);
					sprintf(tmp_str_buff,"%s",value_defaultcard);
					//sleep(1);
					setFlightmode();
					setModem2();
				}
			}
		}
		else
			setModem();
}

void setModule_RG520N(char *tmp_mode_usb,char *tmp_mode_nat)
{
	char cmd[1024];
	int tmp_int_buff;
	char tmp_str_buff[1024];
	char debug[100];
	char tmp_debug[100];
	memset(debug,0,sizeof(debug));
	memset(tmp_debug,0,sizeof(tmp_debug));
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
	//判断是不是pci
	if(isPCIE())
	{
		//PCIe RC/EP mode —— 0 for EP
		memset(cmd,0,sizeof(cmd));
		sprintf(cmd,"cmdat %s 'AT+QCFG=\"pcie/mode\"' | grep '\"pcie/mode\",%s' | wc -l",path_usb,"0");
		tmp_int_buff = cmd_recieve_int(cmd);
		//当前非0时进行设定
		if (tmp_int_buff == 0)
		{
			memset(cmd,0,sizeof(cmd));
			sprintf(cmd,"cmdat %s 'AT+QCFG=\"pcie/mode\",%s' | grep \"OK\" |wc -l",path_usb,"0");
			tmp_int_buff = cmd_recieve_int(cmd);
			if(tmp_int_buff == 1)
				sprintf(tmp_debug,"set PCIe EP mode OK; ");
			else
				sprintf(tmp_debug,"set PCIe EP mode failed; ");
		}else{	//一致时，打印当前结果
			memset(cmd,0,sizeof(cmd));
			sprintf(cmd,"cmdat %s 'AT+QCFG=\"pcie/mode\"' | grep \"+QCFG: \" | awk -F ' ' '{print$2}'",path_usb);
			memset(tmp_str_buff,0,sizeof(tmp_str_buff));
			cmd_recieve_str(cmd,tmp_str_buff);
			sprintf(tmp_debug,"PCIe EP mode; ");
		}
		if(strlen(tmp_debug)>0)
		{
			strcat(debug,tmp_debug);
			memset(tmp_debug,0,sizeof(tmp_debug));
		}
		//Set Network Port/Diagnostic Port —— 1,0 for network in PCIe / diagnostic in USB
		memset(cmd,0,sizeof(cmd));
		sprintf(cmd,"cmdat %s 'AT+QCFG=\"data_interface\"' | grep '\"data_interface\",1,0' | wc -l",path_usb);
		tmp_int_buff = cmd_recieve_int(cmd);
		//当前Network Port/Diagnostic Port不是1，0时进行设定
		if (tmp_int_buff == 0)
		{
			memset(cmd,0,sizeof(cmd));
			sprintf(cmd,"cmdat %s 'AT+QCFG=\"data_interface\",1,0' | grep \"OK\" | wc -l",path_usb);
			tmp_int_buff = cmd_recieve_int(cmd);
			if(tmp_int_buff == 1)
				sprintf(tmp_debug,"set network port via PCIe OK; diagnostic port via USB OK");
			else
				sprintf(tmp_debug,"set network port via PCIe failed; diagnostic port via USB failed");
				
		}else{	//一致时，打印当前上网方式
			memset(cmd,0,sizeof(cmd));
			sprintf(cmd,"cmdat %s 'AT+QCFG=\"data_interface\"' | grep '+QCFG: ' | awk -F ' ' '{print$2}'",path_usb);
			memset(tmp_str_buff,0,sizeof(tmp_str_buff));
			cmd_recieve_str(cmd,tmp_str_buff);
			sprintf(tmp_debug,"network port via PCIe; diagnostic port via USB");
		}
		if(strlen(tmp_debug)>0)
		{
			strcat(debug,tmp_debug);
		}
		if(strlen(debug)>0)
		{
			Debug(debug,"module.info");
		}
		
		while(1)
		{
			memset(cmd,0,sizeof(cmd));
			sprintf(cmd,"cmdat /dev/mhi_DUN 'AT' | grep OK |wc -l");
			tmp_int_buff = cmd_recieve_int(cmd);
			//Debug("cmdat mhi_DUN","daemon.debug");
			if (tmp_int_buff == 1)
				break;
			sleep(1);
		}
	}
	else
	{
		//PCIe RC/EP mode —— 1 for RC
		memset(cmd,0,sizeof(cmd));
		sprintf(cmd,"cmdat %s 'AT+QCFG=\"pcie/mode\"' | grep '\"pcie/mode\",%s' | wc -l",path_usb,"1");
		tmp_int_buff = cmd_recieve_int(cmd);
		//当前非1时进行设定
		if (tmp_int_buff == 0)
		{
			memset(cmd,0,sizeof(cmd));
			sprintf(cmd,"cmdat %s 'AT+QCFG=\"pcie/mode\",%s' | grep \"OK\" |wc -l",path_usb,"1");
			tmp_int_buff = cmd_recieve_int(cmd);
			if(tmp_int_buff == 1)
				sprintf(tmp_debug,"set PCIe RC mode OK; ");
			else
				sprintf(tmp_debug,"set PCIe RC mode failed; ");
		}else{	//一致时，打印当前结果
			memset(cmd,0,sizeof(cmd));
			sprintf(cmd,"cmdat %s 'AT+QCFG=\"pcie/mode\"' | grep \"+QCFG: \" | awk -F ' ' '{print$2}'",path_usb);
			memset(tmp_str_buff,0,sizeof(tmp_str_buff));
			cmd_recieve_str(cmd,tmp_str_buff);
			sprintf(tmp_debug,"PCIe RC mode; ");
		}
		if(strlen(tmp_debug)>0)
		{
			strcat(debug,tmp_debug);
			memset(tmp_debug,0,sizeof(tmp_debug));
		}
		//Set Network Port/Diagnostic Port —— 0,0 for network in USB / diagnostic in USB
		memset(cmd,0,sizeof(cmd));
		sprintf(cmd,"cmdat %s 'AT+QCFG=\"data_interface\"' | grep '\"data_interface\",0,0' | wc -l",path_usb);
		tmp_int_buff = cmd_recieve_int(cmd);
		//当前Network Port/Diagnostic Port不是1，0时进行设定
		if (tmp_int_buff == 0)
		{
			memset(cmd,0,sizeof(cmd));
			sprintf(cmd,"cmdat %s 'AT+QCFG=\"data_interface\",0,0' | grep \"OK\" | wc -l",path_usb);
			tmp_int_buff = cmd_recieve_int(cmd);
			if(tmp_int_buff == 1)
				sprintf(tmp_debug,"set network port and diagnostic port via USB OK; ");
			else
				sprintf(tmp_debug,"set network port and diagnostic port via USB failed; ");
				
		}else{	//一致时，打印当前上网方式
			memset(cmd,0,sizeof(cmd));
			sprintf(cmd,"cmdat %s 'AT+QCFG=\"data_interface\"' | grep '+QCFG: ' | awk -F ' ' '{print$2}'",path_usb);
			memset(tmp_str_buff,0,sizeof(tmp_str_buff));
			cmd_recieve_str(cmd,tmp_str_buff);
			sprintf(tmp_debug,"network port and diagnostic port via USB; ");
		}
		if(strlen(tmp_debug)>0)
		{
			strcat(debug,tmp_debug);
			memset(tmp_debug,0,sizeof(tmp_debug));
		}
		//Set Network Port/Diagnostic Port —— 0,0 for network in USB / diagnostic in USB
		memset(cmd,0,sizeof(cmd));
		sprintf(cmd,"cmdat %s 'AT+QCFG=\"usbnet\"' | grep '\"usbnet\",0' | wc -l",path_usb);
		tmp_int_buff = cmd_recieve_int(cmd);
		//当前Network Port/Diagnostic Port不是1，0时进行设定
		if (tmp_int_buff == 0)
		{
			memset(cmd,0,sizeof(cmd));
			sprintf(cmd,"cmdat %s 'AT+QCFG=\"usbnet\",0' | grep \"OK\" | wc -l",path_usb);
			tmp_int_buff = cmd_recieve_int(cmd);
			if(tmp_int_buff == 1)
				sprintf(tmp_debug,"set usbnet: qmi OK");
			else
				sprintf(tmp_debug,"set usbnet: qmi failed");
				
		}else{	//一致时，打印当前上网方式
			memset(cmd,0,sizeof(cmd));
			sprintf(cmd,"cmdat %s 'AT+QCFG=\"usbnet\"' | grep '+QCFG: ' | awk -F ' ' '{print$2}'",path_usb);
			memset(tmp_str_buff,0,sizeof(tmp_str_buff));
			cmd_recieve_str(cmd,tmp_str_buff);
			sprintf(tmp_debug,"usbnet: qmi");
		}
		if(strlen(tmp_debug)>0)
		{
			strcat(debug,tmp_debug);
		}
		if(strlen(debug)>0)
		{
			Debug(debug,"module.info");
		}
	}
	
	int result=0;
	if(isPCIE())
	{
		//设定当前读卡位置
		if(strstr(value_defaultcard,"auto") || strstr(value_defaultcard,"sim1"))
		{
			sprintf(cmd,"cat /sys/class/leds/sim_sw/brightness");
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
				sprintf(cmd,"echo 0 > /sys/class/leds/sim_sw/brightness");
				cmd_recieve_str(cmd,NULL);
				sprintf(tmp_str_buff,"%s",value_defaultcard);
				//sleep(1);
				setFlightmode_first_reboot();
				setModem1();
			}
		}
		else if(strstr(value_defaultcard,"sim2"))
		{
			sprintf(cmd,"cat /sys/class/leds/sim_sw/brightness");
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
				sprintf(cmd,"echo 1 > /sys/class/leds/sim_sw/brightness");
				cmd_recieve_str(cmd,NULL);
				sprintf(tmp_str_buff,"%s",value_defaultcard);
				//sleep(1);
				setFlightmode_first_reboot();
				setModem2();
			}
		}
		else
			setModem();
	}
	else
	{
		if(strstr(value_defaultcard,"auto") || strstr(value_defaultcard,"sim1"))
		{
			memset(cmd,0,sizeof(cmd));
			if(strstr(board_name,"R680-8TH") || strstr(board_name,"R690") || strstr(board_name,"RT990"))
			{
				//gpio控制读卡先检测模组是否为默认读卡位置
				sprintf(cmd,"cmdat %s 'AT+QUIMSLOT?' | grep \"+QUIMSLOT:\" | awk -F ' ' '{print $2}'",path_usb);
				result = cmd_recieve_int(cmd);
				if (result != 1) 
				{
					memset(cmd,0,sizeof(cmd));
					sprintf(cmd,"cmdat %s 'AT+QUIMSLOT=1'",path_usb);
					cmd_recieve_str(cmd,NULL);
					//sleep(1);
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
					setModem1();
				}else
				{
					memset(cmd,0,sizeof(cmd));
					sprintf(cmd,"echo 1 > /sys/class/leds/sim_sw/brightness");
					cmd_recieve_str(cmd,NULL);
					sprintf(tmp_str_buff,"%s",value_defaultcard);
					//sleep(1);
					setFlightmode();
					setModem1();
				}
			}
			else if(strstr(board_name,"R660"))
			{
				//gpio控制读卡先检测模组是否为默认读卡位置
				sprintf(cmd,"cmdat %s 'AT+QUIMSLOT?' | grep \"+QUIMSLOT:\" | awk -F ' ' '{print $2}'",path_usb);
				result = cmd_recieve_int(cmd);
				if (result != 1) 
				{
					memset(cmd,0,sizeof(cmd));
					sprintf(cmd,"cmdat %s 'AT+QUIMSLOT=1'",path_usb);
					cmd_recieve_str(cmd,NULL);
					//sleep(1);
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
					setModem1();
				}else
				{
					memset(cmd,0,sizeof(cmd));
					sprintf(cmd,"echo 1 > /sys/class/gpio/sim_sw/value");
					cmd_recieve_str(cmd,NULL);
					sprintf(tmp_str_buff,"%s",value_defaultcard);
					//sleep(1);
					setFlightmode();
					setModem1();
				}
			}
			else if(strstr(board_name,"R650") || strstr(board_name,"R680"))
			{
				sprintf(cmd,"cmdat %s 'AT+QUIMSLOT?' | grep \"+QUIMSLOT:\" | awk -F ' ' '{print $2}'",path_usb);
				result = cmd_recieve_int(cmd);
				printf("Current card slot:%d\n",result);
				if (result == 1) //[优先卡槽] == [当前卡槽]
				{
					sprintf(tmp_str_buff,"%s",value_defaultcard);
					setModem1();
				}else
				{
					memset(cmd,0,sizeof(cmd));
					sprintf(cmd,"cmdat %s 'AT+QUIMSLOT=1'",path_usb);
					cmd_recieve_str(cmd,NULL);
					sprintf(tmp_str_buff,"%s",value_defaultcard);
					//sleep(1);
					setFlightmode();
					setModem1();
				}
			}
		}
		else if(strstr(value_defaultcard,"sim2"))
		{
			memset(cmd,0,sizeof(cmd));
			if(strstr(board_name,"R680-8TH") || strstr(board_name,"R690") || strstr(board_name,"RT990"))
			{
				//gpio控制读卡先检测模组是否为默认读卡位置
				sprintf(cmd,"cmdat %s 'AT+QUIMSLOT?' | grep \"+QUIMSLOT:\" | awk -F ' ' '{print $2}'",path_usb);
				result = cmd_recieve_int(cmd);
				if (result != 1) 
				{
					memset(cmd,0,sizeof(cmd));
					sprintf(cmd,"cmdat %s 'AT+QUIMSLOT=1'",path_usb);
					cmd_recieve_str(cmd,NULL);
					//sleep(1);
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
					setModem2();
				}else
				{
					memset(cmd,0,sizeof(cmd));
					sprintf(cmd,"echo 0 > /sys/class/leds/sim_sw/brightness");
					cmd_recieve_str(cmd,NULL);
					sprintf(tmp_str_buff,"%s",value_defaultcard);
					//sleep(1);
					setFlightmode();
					setModem2();
				}
			}
			else if(strstr(board_name,"R660"))
			{
				//gpio控制读卡先检测模组是否为默认读卡位置
				sprintf(cmd,"cmdat %s 'AT+QUIMSLOT?' | grep \"+QUIMSLOT:\" | awk -F ' ' '{print $2}'",path_usb);
				result = cmd_recieve_int(cmd);
				if (result != 1) 
				{
					memset(cmd,0,sizeof(cmd));
					sprintf(cmd,"cmdat %s 'AT+QUIMSLOT=1'",path_usb);
					cmd_recieve_str(cmd,NULL);
					//sleep(1);
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
					setModem2();
				}else
				{
					memset(cmd,0,sizeof(cmd));
					sprintf(cmd,"echo 0 > /sys/class/gpio/sim_sw/value");
					cmd_recieve_str(cmd,NULL);
					sprintf(tmp_str_buff,"%s",value_defaultcard);
					//sleep(1);
					setFlightmode();
					setModem2();
				}
			}
			else if(strstr(board_name,"R650") || strstr(board_name,"R680"))
			{
				sprintf(cmd,"cmdat %s 'AT+QUIMSLOT?' | grep \"+QUIMSLOT:\" | awk -F ' ' '{print $2}'",path_usb);
				result = cmd_recieve_int(cmd);
				printf("Current card slot:%d\n",result);
				if (result == 2) //[优先卡槽] == [当前卡槽]
				{
					sprintf(tmp_str_buff,"%s",value_defaultcard);
					setModem2();
				}else
				{
					memset(cmd,0,sizeof(cmd));
					sprintf(cmd,"cmdat %s 'AT+QUIMSLOT=2'",path_usb);
					cmd_recieve_str(cmd,NULL);
					sprintf(tmp_str_buff,"%s",value_defaultcard);
					//sleep(1);
					setFlightmode();
					setModem2();
				}
			}
		}
		else
			setModem();
	}
}


void setModule_EC20()
{
	char cmd[1024];
	int tmp_int_buff;
	char tmp_str_buff[1024];
	char debug[100];
	char tmp_debug[100];
	memset(debug,0,sizeof(debug));
	memset(tmp_debug,0,sizeof(tmp_debug));
	//确认模组正常工作
	while(1)
	{
		memset(cmd,0,sizeof(cmd));
		sprintf(cmd,"cmdat %s 'AT' | grep OK |wc -l",path_usb);
		tmp_int_buff = cmd_recieve_int(cmd);
		if (tmp_int_buff == 1)
			break;
		sleep(5);
	}
	memset(cmd,0,sizeof(cmd));
	sprintf(cmd,"cmdat %s 'AT+QCFG=\"usbnet\"' | grep '\"usbnet\",%s' | wc -l",path_usb,"0");
	tmp_int_buff = cmd_recieve_int(cmd);
	//当前非0时进行设定
	if (tmp_int_buff == 0)
	{
		memset(cmd,0,sizeof(cmd));
		sprintf(cmd,"cmdat %s 'AT+QCFG=\"usbnet\",%s' | grep \"OK\" |wc -l",path_usb,"0");
		tmp_int_buff = cmd_recieve_int(cmd);
		if(tmp_int_buff == 1)
			sprintf(tmp_debug,"set usbnet: rmnet OK; ");
		else
			sprintf(tmp_debug,"set usbnet: rmnet failed; ");
	}else{	//一致时，打印当前结果
		memset(tmp_str_buff,0,sizeof(tmp_str_buff));
		cmd_recieve_str(cmd,tmp_str_buff);
		sprintf(tmp_debug,"usbnet: rmnet; ");
	}
	if(strlen(tmp_debug)>0)
	{
		strcat(debug,tmp_debug);
		memset(tmp_debug,0,sizeof(tmp_debug));
	}
	//Set Network Port/Diagnostic Port —— 1,0 for network in PCIe / diagnostic in USB
	/* memset(cmd,0,sizeof(cmd));
	sprintf(cmd,"cmdat %s 'AT+QCFG=\"data_interface\"' | grep '\"data_interface\",1,0' | wc -l",path_usb);
	tmp_int_buff = cmd_recieve_int(cmd);
	//当前Network Port/Diagnostic Port不是1，0时进行设定
	if (tmp_int_buff == 0)
	{
		memset(cmd,0,sizeof(cmd));
		sprintf(cmd,"cmdat %s 'AT+QCFG=\"data_interface\",1,0' | grep \"OK\" | wc -l",path_usb);
		tmp_int_buff = cmd_recieve_int(cmd);
		if(tmp_int_buff == 1)
			sprintf(tmp_debug,"set network port via PCIe OK; diagnostic port via USB OK");
		else
			sprintf(tmp_debug,"set network port via PCIe failed; diagnostic port via USB failed");
			
	}else{	//一致时，打印当前上网方式
		memset(cmd,0,sizeof(cmd));
		sprintf(cmd,"cmdat %s 'AT+QCFG=\"data_interface\"' | grep '+QCFG: ' | awk -F ' ' '{print$2}'",path_usb);
		memset(tmp_str_buff,0,sizeof(tmp_str_buff));
		cmd_recieve_str(cmd,tmp_str_buff);
		sprintf(tmp_debug,"network port via PCIe; diagnostic port via USB");
	}
	if(strlen(tmp_debug)>0)
	{
		strcat(debug,tmp_debug);
		memset(tmp_debug,0,sizeof(tmp_debug));
	} */
	//开启GPS
	memset(cmd,0,sizeof(cmd));
	sprintf(cmd,"cmdat %s 'AT+QGPS?' | grep \"+QGPS: 1\" | wc -l",path_usb);
	tmp_int_buff = cmd_recieve_int(cmd);
	//当前非1时进行设定
	if (tmp_int_buff == 1)
	{
		memset(cmd,0,sizeof(cmd));
		sprintf(cmd,"cmdat %s 'AT+QGPS=1' | grep \"OK\" |wc -l",path_usb);
		tmp_int_buff = cmd_recieve_int(cmd);
		if(tmp_int_buff == 1)
			sprintf(tmp_debug,"set GPS mode OK; ");
		else
			sprintf(tmp_debug,"set GPS mode failed; ");
	}else{	//一致时，打印当前结果
		sprintf(tmp_debug,"GPS mode OK");
	}
	if(strlen(tmp_debug)>0)
	{
		strcat(debug,tmp_debug);
	}
	if(strlen(debug)>0)
	{
		Debug(debug,"module.info");
	}
	int result=0;
	//设定当前读卡位置
	if(strstr(value_defaultcard,"auto") || strstr(value_defaultcard,"sim1"))
	{
		sprintf(cmd,"cat /sys/class/leds/sim_sw/brightness");
		result = cmd_recieve_int(cmd);
		printf("Current card slot:%d\n",result);
		if (result == 255)
			result=1;
		if (result == 1) //[优先卡槽] == [当前卡槽]
		{
			sprintf(tmp_str_buff,"%s",value_defaultcard);
			setModem1();
		}else
		{
			memset(cmd,0,sizeof(cmd));
			sprintf(cmd,"echo 1 > /sys/class/leds/sim_sw/brightness");
			cmd_recieve_str(cmd,NULL);
			sprintf(tmp_str_buff,"%s",value_defaultcard);
			sleep(3);
			setFlightmode();
			setModem1();
		}
	}
	else if(strstr(value_defaultcard,"sim2"))
	{
		sprintf(cmd,"cat /sys/class/leds/sim_sw/brightness");
		result = cmd_recieve_int(cmd);
		printf("Current card slot:%d\n",result);
		if (result == 255)
			result=1;
		if (result == 0) //[优先卡槽] == [当前卡槽]
		{
			sprintf(tmp_str_buff,"%s",value_defaultcard);
			setModem2();
		}else
		{
			memset(cmd,0,sizeof(cmd));
			sprintf(cmd,"echo 0 > /sys/class/leds/sim_sw/brightness");
			cmd_recieve_str(cmd,NULL);
			sprintf(tmp_str_buff,"%s",value_defaultcard);
			sleep(3);
			setFlightmode();
			setModem2();
		}
	}
	else
		setModem();
}

/* 功能：设定fm650模组5G频点/小区
** 返回值：void
*/
void set5GCELLLOCK_RG520N(char *nr, char* arfcn, char* psc, char* scs)
{
	char cmd[1024];
	char tmp_buff[1024];
	char cmd_celllock[1024];
	char debug[1024];
	memset(cmd_celllock,0,sizeof(cmd_celllock));
	memset(cmd,0,sizeof(cmd));
	sprintf(cmd,"cmdat %s 'AT+QNWLOCK=\"common/5g\"' | grep \"+QNWLOCK: \" | awk -F '\"' '{print$3}'",path_usb);
	cmd_recieve_str(cmd,cmd_celllock);
	if(strlen(psc)>0)
	{
		memset(cmd,0,sizeof(cmd));
		memset(tmp_buff,0,sizeof(tmp_buff));
		{
			if(strcmp(nr,"0")==0)
			{
				if(count_loop==0)
					Debug("please specify nr5g band if you want to lock the cell","daemon.error");
			}else
			{
				char cmd_setcell[1024];
				memset(cmd_setcell,0,sizeof(cmd_setcell));
				sprintf(cmd_setcell,",%s,%s,%d,%s",psc,arfcn,scs,nr);
				int i=0;
				if(!strstr(cmd_celllock,cmd_setcell))
				{
_set_5cell_lock:
					memset(cmd,0,sizeof(cmd));
					memset(tmp_buff,0,sizeof(tmp_buff));
					sprintf(cmd,"cmdat %s 'AT+QNWLOCK=\"common/5g\",%s,%s,%d,%s'",path_usb,psc,arfcn,scs,nr);
					cmd_recieve_str(cmd,tmp_buff);
					
					memset(cmd_celllock,0,sizeof(cmd_celllock));
					memset(cmd,0,sizeof(cmd));
					sprintf(cmd,"cmdat %s 'AT+QNWLOCK=\"common/5g\"' | grep \"+QNWLOCK: \" | awk -F '\"' '{print$3}'",path_usb);
					cmd_recieve_str(cmd,cmd_celllock);
					
					if(strstr(cmd_celllock,cmd_setcell))
					{
						memset(debug,0,sizeof(debug));
						sprintf(debug,"lock 5g cell OK, band: %s; psc: %s; arfcn: %s; scs: %d",nr,psc,arfcn,scs);
						if(count_loop==0)
							Debug(debug,"daemon.info");
					}
					else
					{
						if(i<=10)
						{
							sleep(1);
							i++;
							goto _set_5cell_lock;
						}
						if(i>10)
						{
							memset(debug,0,sizeof(debug));
							sprintf(debug,"lock 5g cell failed, band: %s; psc: %s; arfcn: %s; scs: %d",nr,psc,arfcn,scs);
						if(count_loop==0)
							Debug(debug,"daemon.error");
						}
					}
					memset(cmd,0,sizeof(cmd));
					sprintf(cmd,"cmdat %s 'AT+QNWLOCK=\"save_ctrl\",1,1'",path_usb);
					cmd_recieve_str(cmd,NULL);
					setFlightmode();
				}
			}
			
		}
	}
}

/* 功能：设定fm650模组4G频点/小区
** 返回值：void
*/
void setCELLLOCK_RG520N(char* arfcn, char* pci)
{
	char cmd[1024];
	char tmp_buff[1024];
	char cmd_celllock[1024];
	char debug[1024];
	memset(cmd_celllock,0,sizeof(cmd_celllock));
	memset(cmd,0,sizeof(cmd));
	sprintf(cmd,"cmdat %s 'AT+QNWLOCK=\"common/4g\"' | grep \"+QNWLOCK: \" | awk -F '\"' '{print$3}'",path_usb);
	cmd_recieve_str(cmd,cmd_celllock);
	if(strlen(pci)>0)
	{
		char cmd_setcell[1024];
		memset(cmd_setcell,0,sizeof(cmd_setcell));
		sprintf(cmd_setcell,",1,%s,%s",arfcn,pci);
		int i=0;
		if(!strstr(cmd_celllock,cmd_setcell))
		{

_set_cell_lock:
			memset(cmd,0,sizeof(cmd));
			memset(tmp_buff,0,sizeof(tmp_buff));
			sprintf(cmd,"cmdat %s 'AT+QNWLOCK=\"common/4g\",1,%s,%s'",path_usb,arfcn,pci);
			cmd_recieve_str(cmd,tmp_buff);
			sleep(1);
			memset(cmd_celllock,0,sizeof(cmd_celllock));
			memset(cmd,0,sizeof(cmd));
			sprintf(cmd,"cmdat %s 'AT+QNWLOCK=\"common/4g\"' | grep \"+QNWLOCK: \" | awk -F '\"' '{print$3}'",path_usb);
			cmd_recieve_str(cmd,cmd_celllock);
			
			if(strstr(cmd_celllock,cmd_setcell))
			{
				memset(debug,0,sizeof(debug));
				sprintf(debug,"lock 4g cell OK, arfcn: %s; pci: %s; ",arfcn,pci);
				if(count_loop==0)
					Debug(debug,"daemon.info");
			}
			else
			{
				if(i<=10)
				{
					i++;
					goto _set_cell_lock;
				}
				if(i>10)
				{
					memset(debug,0,sizeof(debug));
					sprintf(debug,"lock 4g cell failed, arfcn: %s; pci: %s; ",arfcn,pci);
					if(count_loop==0)
						Debug(debug,"daemon.error");	
				}
			}
			memset(cmd,0,sizeof(cmd));
			sprintf(cmd,"cmdat %s 'AT+QNWLOCK=\"save_ctrl\",1,1'",path_usb);
			cmd_recieve_str(cmd,NULL);
			setFlightmode();
		}
	}
}

/* 功能：恢复fm650模组频点/小区
** 返回值：void
*/
void reset_RG520N()
{
	char cmd[1024];
	char tmp_buff[1024];
	char cmd_celllock[1024];
	int i=0;
_reset_cell_again:
	memset(cmd,0,sizeof(cmd));
	sprintf(cmd,"cmdat %s 'AT+QNWLOCK=\"common/4g\",0'",path_usb);
	memset(tmp_buff,0,sizeof(tmp_buff));
	cmd_recieve_str(cmd,tmp_buff);
	
	memset(cmd_celllock,0,sizeof(cmd_celllock));
	memset(cmd,0,sizeof(cmd));
	sprintf(cmd,"cmdat %s 'AT+QNWLOCK=\"common/4g\"' | grep \"+QNWLOCK: \" | awk -F '\"' '{print$3}'",path_usb);
	cmd_recieve_str(cmd,cmd_celllock);
	
	if(!strstr(cmd_celllock,",0") && i < 10)
	{
		i++;
		goto _reset_cell_again;
	}
	if(i==10)
	{
		if(count_loop==0)
			Debug("unlock 4g cell failed","daemon.error");
	}
	i=0;
_reset_5cell_again:
	memset(cmd,0,sizeof(cmd));
	sprintf(cmd,"cmdat %s 'AT+QNWLOCK=\"common/5g\",0'",path_usb);
	memset(tmp_buff,0,sizeof(tmp_buff));
	cmd_recieve_str(cmd,tmp_buff);
	
	memset(cmd_celllock,0,sizeof(cmd_celllock));
	memset(cmd,0,sizeof(cmd));
	sprintf(cmd,"cmdat %s 'AT+QNWLOCK=\"common/5g\"' | grep \"+QNWLOCK: \" | awk -F '\"' '{print$3}'",path_usb);
	cmd_recieve_str(cmd,cmd_celllock);
	
	if(!strstr(cmd_celllock,",0") && i < 10)
	{
		i++;
		goto _reset_5cell_again;
	}
	if(i==10)
	{
		if(count_loop==0)
			Debug("unlock 5g cell failed","daemon.error");
	}
	
	memset(cmd,0,sizeof(cmd));
	sprintf(cmd,"cmdat %s 'AT+QNWLOCK=\"save_ctrl\",1,1'",path_usb);
	cmd_recieve_str(cmd,NULL);
	
	//下面指令会导致识别不到模块，因此注释
	// memset(cmd,0,sizeof(cmd));
	// sprintf(cmd,"cmdat %s 'AT+QPRTPARA=3'",path_usb);
	// memset(tmp_buff,0,sizeof(tmp_buff));
	// cmd_recieve_str(cmd,tmp_buff);
}

extern void analyticRSRP_RG520N(int tmp_rsrp,float tmp_rsrq,float tmp_sinr)
{
	
	if(tmp_rsrp <= -140 )					//<-140			无信号
		value_signal=0;
	else if(tmp_rsrp <= -125 )			//-126~-140		1格
		value_signal=1;
	else if(tmp_rsrp <= -115 )			//-116~-125		2格
		value_signal=2;
	else if(tmp_rsrp <= -105 )			//-106~-115 	3格
		value_signal=3;
	else if(tmp_rsrp <= -95 )				//-96~-105		4格
		value_signal=4;
	else if(tmp_rsrp <= -10 )				//-11~-95 		5格
		value_signal=5;
	uci_cellular_int("SVAL",value_signal);
	uci_cellular_int("RSRP",tmp_rsrp);
	uci_cellular_delete("RSSI");
	uci_cellular_float("RSRQ",(double)tmp_rsrq);
	uci_cellular_float("SINR",(double)tmp_sinr);
	char cmd[1024];
	/* memset(cmd,0,sizeof(cmd));
	sprintf(cmd,"echo \"+SVAL: %d\" >> %s",value_signal,path_outfile);
	cmd_recieve_str(cmd,NULL);
	memset(cmd,0,sizeof(cmd));
	sprintf(cmd,"echo \"+RSRP: %d\" >> %s",tmp_rsrp,path_outfile);
	cmd_recieve_str(cmd,NULL);
	memset(cmd,0,sizeof(cmd));
	sprintf(cmd,"echo \"+RSRQ: %.1f\" >> %s",tmp_rsrq,path_outfile);
	cmd_recieve_str(cmd,NULL); */
	
	char debug[1024];	
	memset(debug,0,sizeof(debug));
	if(value_signal>2 && sighi_debug == 0)
	{
		sprintf(debug,"rsrp: %d dBm; rsrq: %.1f dB; sinr %.1f dBm; signal value: %d",tmp_rsrp,tmp_rsrq,tmp_sinr,value_signal);
		
		Debug(debug,"cellular.info");
		sighi_debug = 1;
		siglo_debug = 0;
	}
	if(value_signal<2 && siglo_debug == 0)
	{
		sprintf(debug,"low signal, rsrp: %d dBm; rsrq: %.1f dB; sinr %.1f dBm; signal value: %d",tmp_rsrp,tmp_rsrq,tmp_sinr,value_signal);
		Debug(debug,"cellular.warn");
		sighi_debug = 0;
		siglo_debug = 1;
	}
}

/* 功能：解析rm500u所处网络小区信息
** 返回值：void
*/
void analyticQENG_RM500U()
{
	char tmp_buff[2048];
	char cmd[1024];
	char nservice[100];
	char arfcn[100];
	char pci[100];
	char band[100];
	char eNB[100];
	char cellID[100];
	char debug[100];
	memset(debug,0,sizeof(debug));
	memset(cmd,0,sizeof(cmd));
	memset(tmp_buff,0,sizeof(tmp_buff));
	memset(nservice,0,sizeof(nservice));
	memset(band,0,sizeof(band));
	sprintf(cmd,"cmdat %s 'AT+QENG=\"servingcell\"' 500 > %s",path_usb,path_at);
	cmd_recieve_str(cmd,NULL);
	memset(tmp_buff,0,sizeof(tmp_buff));
	sprintf(cmd,"cat %s | grep -A 2 \"servingcell\" | xargs echo -n",path_at);
	cmd_recieve_str(cmd,tmp_buff);
	if(strstr(tmp_buff,"SEARCH"))
	{
		strcpy(nservice,"searching");
		uci_cellular("CPSI","Searching Service");
		uci_cellular("BAND","-");
		uci_cellular("ARFCN","-");
		uci_cellular("PCI","-");
		uci_cellular("eNB","-");
		uci_cellular("cellID","-");
		if(strcmp(debug_nservice,nservice)!=0)
		{
			Debug("searching service","cellular.error");
			strcpy(debug_nservice,nservice);
			strcpy(debug_band,"-");
			strcpy(debug_arfcn,"-");
			strcpy(debug_pci,"-");
			strcpy(debug_enb,"-");
			strcpy(debug_cellid,"-");
		}
		analyticCSQ();
		return;
	}
	else if(strstr(tmp_buff,"LIMSRV"))
	{
		strcpy(nservice,"NoService");
		uci_cellular("CPSI","NO SERVICE");
		uci_cellular("BAND","-");
		uci_cellular("ARFCN","-");
		uci_cellular("PCI","-");
		uci_cellular("eNB","-");
		uci_cellular("cellID","-");
		if(strcmp(debug_nservice,nservice)!=0)
		{
			Debug("no service","cellular.error");
			strcpy(debug_nservice,nservice);
			strcpy(debug_band,"-");
			strcpy(debug_arfcn,"-");
			strcpy(debug_pci,"-");
			strcpy(debug_enb,"-");
			strcpy(debug_cellid,"-");
		}
		analyticCSQ();
		return;
	}
	else{
		int count=countstr(tmp_buff,",");
		if(count<=0)
		{
			strcpy(nservice,"NoService");
			uci_cellular("CPSI","NO SERVICE");
			uci_cellular("BAND","-");
			uci_cellular("ARFCN","-");
			uci_cellular("PCI","-");
			uci_cellular("eNB","-");
			uci_cellular("cellID","-");
			if(strcmp(debug_nservice,nservice)!=0)
			{
				Debug("no service","cellular.error");
				strcpy(debug_nservice,nservice);
				strcpy(debug_band,"-");
				strcpy(debug_arfcn,"-");
				strcpy(debug_pci,"-");
				strcpy(debug_enb,"-");
				strcpy(debug_cellid,"-");
			}
			analyticCSQ();
			return;
		}
		char array_CPSI[count][50];
		if(separate_string2array(tmp_buff,",",count,50,(char *)&array_CPSI) != count)
		{
			sprintf(debug,"formatting the current information of cellular failed, %s\n",tmp_buff);
			printf(debug);
			/* uci_cellular("CPSI","NO SERVICE");
			uci_cellular("BAND","-");
			uci_cellular("ARFCN","-");
			uci_cellular("PCI","-"); */
		}
		//解析服务类型
		if(strstr(tmp_buff,"NR5G-NSA"))
		{
			memset(arfcn,0,sizeof(arfcn));
			strcpy(nservice,"NR5G_NSA");
			sprintf(band,"%s/%s",array_CPSI[8],array_CPSI[26]);
			
			memset(eNB,0,sizeof(eNB));
			memset(cellID,0,sizeof(cellID));
			if(strcmp(array_CPSI[5],"-")!=0 )
			{
				memset(cmd,0,sizeof(cmd));
				strncpy(eNB, array_CPSI[5], 5);
				eNB[5] = '\0';
				sprintf(cmd,"echo $((0x%s))",eNB);

				memset(eNB,0,sizeof(eNB));
				cmd_recieve_str(cmd,eNB);
				
				memset(cmd,0,sizeof(cmd));
				strcpy(cellID, array_CPSI[5] + 5);
				cellID[strlen(array_CPSI[5])-5] = '\0';
				sprintf(cmd,"echo $((0x%s))",cellID);
					
				memset(cellID,0,sizeof(cellID));
				cmd_recieve_str(cmd,cellID);
				
				uci_cellular("eNB",eNB);
				uci_cellular("cellID",cellID);
			}
			else
			{
				uci_cellular("eNB","-");
				uci_cellular("cellID","-");
				sprintf(eNB,"-");
				sprintf(cellID,"-");
			}
			
			if(strcmp(array_CPSI[7],"-")!=0 )
			{
				memset(cmd,0,sizeof(cmd));
				sprintf(arfcn,"%s/%s",array_CPSI[7],array_CPSI[25]);
				// sprintf(cmd,"echo \"+ARFCN: %s\" >> %s",arfcn,path_outfile);
				// cmd_recieve_str(cmd,NULL);
				uci_cellular("ARFCN",arfcn);
			}
			else
			{
				uci_cellular("ARFCN","-");
				sprintf(arfcn,"-");
			}
			memset(pci,0,sizeof(pci));
			if(strcmp(array_CPSI[6],"-")!=0 )
			{
				memset(cmd,0,sizeof(cmd));
				sprintf(pci,"%s/%s",array_CPSI[6],array_CPSI[21]);
				// sprintf(cmd,"echo \"+PCI: %s\" >> %s",pci,path_outfile);
				// cmd_recieve_str(cmd,NULL);
				uci_cellular("PCI",pci);
			}
			else
			{
				uci_cellular("PCI","-");
				sprintf(pci,"-");
			}
			int tmp_rsrp=atoi(array_CPSI[12]);
			float tmp_rsrq=atof(array_CPSI[13]);
			int tmp_rssi=atoi(array_CPSI[14]);
			analyticRSRP(tmp_rsrp,tmp_rsrq,tmp_rssi);
		}
		else if(strstr(tmp_buff,"NR5G-SA"))
		{
			strcpy(nservice,"NR5G_SA");
			strcat(band,array_CPSI[10]);
			int tmp_rsrp=atoi(array_CPSI[12]);
			float tmp_rsrq=atof(array_CPSI[13]);
			float tmp_sinr=atof(array_CPSI[14]);
			analyticRSRP_RG520N(tmp_rsrp,tmp_rsrq,tmp_sinr);
			{
				memset(eNB,0,sizeof(eNB));
				memset(cellID,0,sizeof(cellID));
				if(strcmp(array_CPSI[6],"-")!=0 )
				{
					memset(cmd,0,sizeof(cmd));
					strncpy(eNB, array_CPSI[6], 6);
					eNB[6] = '\0';
					sprintf(cmd,"echo $((0x%s))",eNB);

					memset(eNB,0,sizeof(eNB));
					cmd_recieve_str(cmd,eNB);
						
					memset(cmd,0,sizeof(cmd));
					strcpy(cellID, array_CPSI[6] + 6);
					cellID[strlen(array_CPSI[6])-6] = '\0';
					sprintf(cmd,"echo $((0x%s))",cellID);
						
					memset(cellID,0,sizeof(cellID));
					cmd_recieve_str(cmd,cellID);
					
					uci_cellular("eNB",eNB);
					uci_cellular("cellID",cellID);
				}
				else
				{
					uci_cellular("eNB","-");
					uci_cellular("cellID","-");
					sprintf(eNB,"-");
					sprintf(cellID,"-");
				}
				
				memset(arfcn,0,sizeof(arfcn));
				if(strcmp(array_CPSI[9],"-")!=0 )
				{
					memset(cmd,0,sizeof(cmd));
					sprintf(arfcn,array_CPSI[9]);
					// sprintf(cmd,"echo \"+ARFCN: %s\" >> %s",arfcn,path_outfile);
					// cmd_recieve_str(cmd,NULL);
					uci_cellular("ARFCN",arfcn);
				}
				else
				{
					uci_cellular("ARFCN","-");
					sprintf(arfcn,"-");
				}
				memset(pci,0,sizeof(pci));
				if(strcmp(array_CPSI[7],"-")!=0 )
				{
					memset(cmd,0,sizeof(cmd));
					sprintf(pci,array_CPSI[7]);
					// sprintf(cmd,"echo \"+PCI: %s\" >> %s",pci,path_outfile);
					// cmd_recieve_str(cmd,NULL);
					uci_cellular("PCI",pci);
				}
				else
				{
					uci_cellular("PCI","-");
					sprintf(pci,"-");
				}
			}
		}
		else if(strstr(tmp_buff,"LTE"))
		{
			strcpy(nservice,"LTE");
			strcat(band,array_CPSI[9]);
			int tmp_rsrp=atoi(array_CPSI[13]);
			float tmp_rsrq=atof(array_CPSI[14]);
			int tmp_rssi=atoi(array_CPSI[15]);
			analyticRSRP(tmp_rsrp,tmp_rsrq,tmp_rssi);
			{
				
				memset(eNB,0,sizeof(eNB));
				memset(cellID,0,sizeof(cellID));
				if(strcmp(array_CPSI[6],"-")!=0 )
				{
					memset(cmd,0,sizeof(cmd));
					strncpy(eNB, array_CPSI[6], 5);
					eNB[5] = '\0';
					sprintf(cmd,"echo $((0x%s))",eNB);

					memset(eNB,0,sizeof(eNB));
					cmd_recieve_str(cmd,eNB);
						
					memset(cmd,0,sizeof(cmd));
					strcpy(cellID, array_CPSI[6] + 5);
					cellID[strlen(array_CPSI[6])-5] = '\0';
					sprintf(cmd,"echo $((0x%s))",cellID);
						
					memset(cellID,0,sizeof(cellID));
					cmd_recieve_str(cmd,cellID);
					
					uci_cellular("eNB",eNB);
					uci_cellular("cellID",cellID);
				}
				else
				{
					uci_cellular("eNB","-");
					uci_cellular("cellID","-");
					sprintf(eNB,"-");
					sprintf(cellID,"-");
				}
				
				memset(arfcn,0,sizeof(arfcn));
				if(strcmp(array_CPSI[8],"-")!=0 )
				{
					memset(cmd,0,sizeof(cmd));
					sprintf(arfcn,array_CPSI[8]);
					// sprintf(cmd,"echo \"+ARFCN: %s\" >> %s",arfcn,path_outfile);
					// cmd_recieve_str(cmd,NULL);
					uci_cellular("ARFCN",arfcn);
				}
				else
				{
					uci_cellular("ARFCN","-");
					sprintf(pci,"-");
				}
				
				memset(pci,0,sizeof(pci));
				if(strcmp(array_CPSI[7],"-")!=0 )
				{
					memset(cmd,0,sizeof(cmd));
					sprintf(pci,array_CPSI[7]);
					// sprintf(cmd,"echo \"+PCI: %s\" >> %s",pci,path_outfile);
					// cmd_recieve_str(cmd,NULL);
					uci_cellular("PCI",pci);
				}
				else
				{
					uci_cellular("PCI","-");
					sprintf(pci,"-");
				}
			}
		}
		else if(strstr(tmp_buff,"WCDMA"))
		{
			strcpy(nservice,"WCDMA");
			strcpy(band,"WCDMA");
			analyticCSQ();
			uci_cellular_delete("eNB");
			uci_cellular_delete("cellID");
			uci_cellular_delete("ARFCN");
			uci_cellular_delete("PCI");
			sprintf(arfcn,"-");
			sprintf(pci,"-");
		}
		if(strlen(nservice)>0)
		{
			memset(cmd,0,sizeof(cmd));
			// sprintf(cmd,"echo \"+CPSI: %s\" >> %s",nservice,path_outfile);
			// cmd_recieve_str(cmd,NULL);
			
			if(strlen(band)>0)
			{
				memset(cmd,0,sizeof(cmd));
				// sprintf(cmd,"echo \"+BAND: %s\" >> %s",band,path_outfile);
				// cmd_recieve_str(cmd,NULL);
			}
			uci_cellular("CPSI",nservice);
			uci_cellular("BAND",band);
			
			if(strcmp(debug_nservice,nservice)!=0 || strcmp(debug_band,band)!=0 || strcmp(debug_arfcn,arfcn)!=0 || strcmp(debug_pci,pci)!=0 || strcmp(debug_enb,eNB)!=0 || strcmp(debug_cellid,cellID)!=0)
			{
				char value_ip_ifconfig[500];
				memset(value_ip_ifconfig,0,sizeof(value_ip_ifconfig));
				sprintf(cmd,"ifconfig %s | grep \"inet addr\" | awk -F ':' '{print$2}' | awk -F ' ' '{print$1}'",value_ifname);
				cmd_recieve_str(cmd,value_ip_ifconfig);
				if(value_ip_ifconfig == NULL || strlen(value_ip_ifconfig)==0)
				{
					sprintf(debug,"%s, band: %s; arfcn: %s; pci: %s; eNB: %s; cellID: %s",nservice,band,arfcn,pci,eNB,cellID);
					Debug(debug,"cellular.info");
				}
				memset(debug_nservice,0,sizeof(debug_nservice));
				memset(debug_band,0,sizeof(debug_band));
				memset(debug_arfcn,0,sizeof(debug_arfcn));
				memset(debug_pci,0,sizeof(debug_pci));
				memset(debug_enb,0,sizeof(debug_enb));
				memset(debug_cellid,0,sizeof(debug_cellid));
				strcpy(debug_nservice,nservice);
				strcpy(debug_band,band);
				strcpy(debug_arfcn,arfcn);
				strcpy(debug_pci,pci);
				strcpy(debug_enb,eNB);
				strcpy(debug_cellid,cellID);
			}
		}
	}
}


void analyticQENG_EC20()
{
	char tmp_buff[2048];
	char cmd[1024];
	char nservice[100];
	char band[100];
	char debug[100];
	memset(debug,0,sizeof(debug));
	memset(cmd,0,sizeof(cmd));
	memset(tmp_buff,0,sizeof(tmp_buff));
	memset(nservice,0,sizeof(nservice));
	memset(band,0,sizeof(band));
	sprintf(cmd,"cmdat %s 'AT+QNWINFO' > %s",path_usb,path_at);
	cmd_recieve_str(cmd,NULL);
	memset(tmp_buff,0,sizeof(tmp_buff));
	sprintf(cmd,"cat %s | grep \"+QNWINFO:\" | xargs echo -n",path_at);
	cmd_recieve_str(cmd,tmp_buff);
	if(strstr(tmp_buff,"SEARCH"))
	{
		strcpy(nservice,"Searching Service");
		uci_cellular("CPSI","Searching Service");
		uci_cellular("BAND","-");
		if(strcmp(debug_nservice,nservice)!=0)
		{
			Debug("searching service","cellular.error");
			strcpy(debug_nservice,nservice);
			strcpy(debug_band,"-");
		}
		return;
	}
	else if(strstr(tmp_buff,"No Service"))
	{
		strcpy(nservice,"No Service");
		uci_cellular("CPSI","NO SERVICE");
		uci_cellular("BAND","-");
		uci_cellular("ARFCN","-");
		uci_cellular("PCI","-");
		if(strcmp(debug_nservice,nservice)!=0)
		{
			Debug("no service","cellular.error");
			strcpy(debug_nservice,nservice);
			strcpy(debug_band,"-");
		}
		return;
	}
	else{
		int count=countstr(tmp_buff,",");
		if(count<=0)
		{
			strcpy(nservice,"No Service");
			uci_cellular("CPSI","NO SERVICE");
			uci_cellular("BAND","-");
			if(strcmp(debug_nservice,nservice)!=0)
			{
				Debug("no service","cellular.error");
				strcpy(debug_nservice,nservice);
				strcpy(debug_band,"-");
			}
			return;
		}
		char array_CPSI[count][50];
		if(separate_string2array(tmp_buff,",",count,50,(char *)&array_CPSI) != count)
		{
			sprintf(debug,"formatting the current information of cellular failed, %s\n",tmp_buff);
			printf(debug);
		}
		//解析服务类型
		if(strstr(tmp_buff,"LTE"))
		{
			strcpy(nservice,"LTE");
			strcat(band,array_CPSI[2]);
		}
		else if(strstr(tmp_buff,"WCDMA"))
		{
			strcpy(nservice,"WCDMA");
			strcat(band,array_CPSI[2]);
		}
		if(strlen(nservice)>0)
		{
			// memset(cmd,0,sizeof(cmd));
			// sprintf(cmd,"echo \"+CPSI: %s\" >> %s",nservice,path_outfile);
			// cmd_recieve_str(cmd,NULL);
			
			//if(strlen(band)>0)
			{
				// memset(cmd,0,sizeof(cmd));
				// sprintf(cmd,"echo \"+BAND: %s\" >> %s",band,path_outfile);
				// cmd_recieve_str(cmd,NULL);
			}
			uci_cellular("CPSI",nservice);
			uci_cellular("BAND",band);
			
			if(strcmp(debug_nservice,nservice)!=0 || strcmp(debug_band,band)!=0)
			{
				char value_ip_ifconfig[500];
				memset(value_ip_ifconfig,0,sizeof(value_ip_ifconfig));
				sprintf(cmd,"ifconfig %s | grep \"inet addr\" | awk -F ':' '{print$2}' | awk -F ' ' '{print$1}'",value_ifname);
				cmd_recieve_str(cmd,value_ip_ifconfig);
				if(value_ip_ifconfig == NULL || strlen(value_ip_ifconfig)==0)
				{
					sprintf(debug,"%s, band: %s",nservice,band);
					Debug(debug,"cellular.info");
				}
				memset(debug_nservice,0,sizeof(debug_nservice));
				memset(debug_band,0,sizeof(debug_band));
				strcpy(debug_nservice,nservice);
				strcpy(debug_band,band);
			}
		}
		analyticCSQ();
	}
}

/* 功能：设定rm500u模组上网服务类型
** 返回值：void
*/
void setRATMODE_RM500U(char *mode,char *wcdma,char *lte,char *nr)
{

	char cmd[1024];
	char tmp_buff[1024];
	//设定服务类型
_SETRAT_R500U_AGAIN:
	memset(cmd,0,sizeof(cmd));
	sprintf(cmd,"cmdat %s 'AT+QNWPREFCFG=\"mode_pref\",%s'",path_usb,mode);
	memset(tmp_buff,0,sizeof(tmp_buff));
	cmd_recieve_str(cmd,tmp_buff);
	memset(cmd,0,sizeof(cmd));
	sprintf(cmd,"%s",mode);
	if(strstr(tmp_buff,"OK"))				//获取设定结果，OK
		Debug(cmd,"Set AT+QNWPREFCFG=\"mode_pref\"");
	else if(strstr(tmp_buff,"ERROR"))		//获取设定结果，ERROR
		Debug(cmd,"Failed-1 to set AT+QNWPREFCFG=\"mode_pref\"");
	else{									//获取设定结果失败，通过goto再次设定
		Debug(cmd,"Failed-2 to set AT+QNWPREFCFG=\"mode_pref\"");
		goto _SETRAT_R500U_AGAIN;
	}
	//锁定频段
	char str_wcdma[100];
	char str_lte[100];
	char str_nr[100];
	char str_nsa[100];
	memset(str_wcdma,0,sizeof(str_wcdma));
	memset(str_lte,0,sizeof(str_lte));
	memset(str_nr,0,sizeof(str_nr));
	if(strcmp(mode,"AUTO")==0)
	{
		memset(cmd,0,sizeof(cmd));
		sprintf(cmd,"cmdat %s 'AT+QNWPREFCFG=\"all_band_reset\"'",path_usb);
		memset(tmp_buff,0,sizeof(tmp_buff));
		cmd_recieve_str(cmd,tmp_buff);
		/*目前测试设置成功后有时候无回复
		if(strstr(tmp_buff,"OK"))				//获取设定结果，OK
			Debug("Reset AT+CSYSSEL",NULL);
		else if(strstr(tmp_buff,"ERROR"))		//获取设定结果，ERROR
			Debug("Failed-1 to reset AT+CSYSSEL",NULL);
		*/
		Debug("Reset AT+QNWPREFCFG",NULL);
	}
	if(strlen(wcdma)>0)
	{
		if(strcmp(wcdma,"0")==0)
			strcpy(str_wcdma,"1:2:5:8");
		else
			sprintf(str_wcdma,"%s",wcdma);
		memset(cmd,0,sizeof(cmd));
		sprintf(cmd,"cmdat %s 'AT+QNWPREFCFG=\"gw_band\",%s'",path_usb,str_wcdma);
		memset(tmp_buff,0,sizeof(tmp_buff));
		cmd_recieve_str(cmd,tmp_buff);
		if(strstr(tmp_buff,"OK"))				//获取设定结果，OK
			Debug(str_wcdma,"Set AT+QNWPREFCFG=\"gw_band\"");
		else if(strstr(tmp_buff,"ERROR"))		//获取设定结果，ERROR
			Debug(str_wcdma,"Failed-1 to set AT+QNWPREFCFG=\"gw_band\"");
	}
	if(strlen(lte)>0)
	{
		if(strcmp(lte,"0")==0)
			strcpy(str_lte,"1:2:3:5:7:8:20:28:34:38:39:40:41");
		else
			sprintf(str_lte,"%s",lte);
		memset(cmd,0,sizeof(cmd));
		sprintf(cmd,"cmdat %s 'AT+QNWPREFCFG=\"lte_band\",%s'",path_usb,str_lte);
		memset(tmp_buff,0,sizeof(tmp_buff));
		cmd_recieve_str(cmd,tmp_buff);
		if(strstr(tmp_buff,"OK"))				//获取设定结果，OK
			Debug(str_lte,"Set AT+QNWPREFCFG=\"lte_band\"");
		else if(strstr(tmp_buff,"ERROR"))		//获取设定结果，ERROR
			Debug(str_lte,"Failed-1 to set AT+QNWPREFCFG=\"lte_band\"");
	}
	if(strlen(nr)>0)
	{	
		if(strcmp(nr,"0")==0)
			strcpy(str_nr,"1:28:41:77:78:79");
		else
			sprintf(str_nr,"%s",nr);
		memset(cmd,0,sizeof(cmd));
		sprintf(cmd,"cmdat %s 'AT+QNWPREFCFG=\"nr5g_band\",%s'",path_usb,str_nr);
		memset(tmp_buff,0,sizeof(tmp_buff));
		cmd_recieve_str(cmd,tmp_buff);
		if(strstr(tmp_buff,"OK"))				//获取设定结果，OK
			Debug(str_nr,"Set AT+QNWPREFCFG=\"nr5g_band\"");
		else if(strstr(tmp_buff,"ERROR"))		//获取设定结果，ERROR
			Debug(str_nr,"Failed-1 to set AT+QNWPREFCFG=\"nr5g_band\"");
	}
	memset(cmd,0,sizeof(cmd));
	sprintf(cmd,"cmdat %s \"AT+QNETDEVSTATUS=1\" 1000 | grep \"+QNETDEVSTATUS:\" | wc -l",path_usb);
	int result=cmd_recieve_int(cmd);
	if(result == 1)
	{
		memset(cmd,0,sizeof(cmd));
		sprintf(cmd,"cmdat %s 'AT+QNETDEVCTL=1,2,1' 1000",path_usb);
		memset(tmp_buff,0,sizeof(tmp_buff));
		cmd_recieve_str(cmd,tmp_buff);
		if(strstr(tmp_buff,"OK"))
			Debug("Close dial",NULL);
		else if(strstr(tmp_buff,"ERROR"))
			Debug("Failed-1 to close dial",NULL);
		else
			Debug("Failed-2 to close dial",NULL);
	}
}

/* 功能：设定rm500q模组上网服务类型
** 返回值：void
*/
void setRATMODE_RM500Q(char *mode,char *wcdma,char *lte,char *nr)
{

	char cmd[1024];
	char tmp_buff[1024];
	//设定服务类型
_SETRAT_RM500Q_AGAIN:
	memset(cmd,0,sizeof(cmd));
	sprintf(cmd,"cmdat %s 'AT+QNWPREFCFG=\"mode_pref\",%s'",path_usb,mode);
	memset(tmp_buff,0,sizeof(tmp_buff));
	cmd_recieve_str(cmd,tmp_buff);
	memset(cmd,0,sizeof(cmd));
	sprintf(cmd,"%s",mode);
	if(strstr(tmp_buff,"OK"))				//获取设定结果，OK
		Debug(cmd,"Set AT+QNWPREFCFG=\"mode_pref\"");
	else if(strstr(tmp_buff,"ERROR"))		//获取设定结果，ERROR
		Debug(cmd,"Failed-1 to set AT+QNWPREFCFG=\"mode_pref\"");
	else{									//获取设定结果失败，通过goto再次设定
		Debug(cmd,"Failed-2 to set AT+QNWPREFCFG=\"mode_pref\"");
		goto _SETRAT_RM500Q_AGAIN;
	}
	//锁定频段
	char str_wcdma[100];
	char str_lte[100];
	char str_nr[100];
	char str_nsa[100];
	memset(str_wcdma,0,sizeof(str_wcdma));
	memset(str_lte,0,sizeof(str_lte));
	memset(str_nr,0,sizeof(str_nr));
	if(strcmp(mode,"AUTO")==0)
	{
		memset(cmd,0,sizeof(cmd));
		sprintf(cmd,"cmdat %s 'AT+QNWPREFCFG=\"all_band_reset\"'",path_usb);
		memset(tmp_buff,0,sizeof(tmp_buff));
		cmd_recieve_str(cmd,tmp_buff);
		/*目前测试设置成功后有时候无回复
		if(strstr(tmp_buff,"OK"))				//获取设定结果，OK
			Debug("Reset AT+CSYSSEL",NULL);
		else if(strstr(tmp_buff,"ERROR"))		//获取设定结果，ERROR
			Debug("Failed-1 to reset AT+CSYSSEL",NULL);
		*/
		Debug("Reset AT+QNWPREFCFG",NULL);
	}
	if(strlen(wcdma)>0)
	{
		if(strcmp(wcdma,"0")==0)
			strcpy(str_wcdma,"1:2:3:4:5:6:8:19");
		else
			sprintf(str_wcdma,"%s",wcdma);
		memset(cmd,0,sizeof(cmd));
		sprintf(cmd,"cmdat %s 'AT+QNWPREFCFG=\"gw_band\",%s'",path_usb,str_wcdma);
		memset(tmp_buff,0,sizeof(tmp_buff));
		cmd_recieve_str(cmd,tmp_buff);
		if(strstr(tmp_buff,"OK"))				//获取设定结果，OK
			Debug(str_wcdma,"Set AT+QNWPREFCFG=\"gw_band\"");
		else if(strstr(tmp_buff,"ERROR"))		//获取设定结果，ERROR
			Debug(str_wcdma,"Failed-1 to set AT+QNWPREFCFG=\"gw_band\"");
	}
	if(strlen(lte)>0)
	{
		if(strcmp(lte,"0")==0)
			strcpy(str_lte,"1:2:3:4:5:7:8:12:13:14:17:18:19:20:25:26:28:29:30:32:34:38:39:40:41:42:43:46:48:66:71");
		else
			sprintf(str_lte,"%s",lte);
		memset(cmd,0,sizeof(cmd));
		sprintf(cmd,"cmdat %s 'AT+QNWPREFCFG=\"lte_band\",%s'",path_usb,str_lte);
		memset(tmp_buff,0,sizeof(tmp_buff));
		cmd_recieve_str(cmd,tmp_buff);
		if(strstr(tmp_buff,"OK"))				//获取设定结果，OK
			Debug(str_lte,"Set AT+QNWPREFCFG=\"lte_band\"");
		else if(strstr(tmp_buff,"ERROR"))		//获取设定结果，ERROR
			Debug(str_lte,"Failed-1 to set AT+QNWPREFCFG=\"lte_band\"");
	}
	if(strlen(nr)>0)
	{	
		if(strcmp(nr,"0")==0)
			strcpy(str_nr,"1:2:3:5:7:8:12:20:25:28:38:40:41:48:66:71:77:78:79");
		else
			sprintf(str_nr,"%s",nr);
		memset(cmd,0,sizeof(cmd));
		sprintf(cmd,"cmdat %s 'AT+QNWPREFCFG=\"nr5g_band\",%s'",path_usb,str_nr);
		memset(tmp_buff,0,sizeof(tmp_buff));
		cmd_recieve_str(cmd,tmp_buff);
		if(strstr(tmp_buff,"OK"))				//获取设定结果，OK
			Debug(str_nr,"Set AT+QNWPREFCFG=\"nr5g_band\"");
		else if(strstr(tmp_buff,"ERROR"))		//获取设定结果，ERROR
			Debug(str_nr,"Failed-1 to set AT+QNWPREFCFG=\"nr5g_band\"");
	}
	setFlightmode();
}

/* 功能：设定rm500q模组上网服务类型
** 返回值：void
*/
void setRATMODE_RG520N(char *mode,char *wcdma,char *lte,char *nr,char *nsa,char *arfcn)
{

	char cmd[1024];
	char tmp_buff[1024];
	char tmp_mode_pref[1024];
	char new_mode_pref[1024];
	char debug[1024];
	while(1)
	{
		memset(cmd,0,sizeof(cmd));
		sprintf(cmd,"cmdat %s 'AT' | grep OK |wc -l",path_usb);
		int tmp_int_buff = cmd_recieve_int(cmd);
		if (tmp_int_buff == 1)
		{
			break;
		}
	}
	//获取服务类型
	memset(cmd,0,sizeof(cmd));
	sprintf(cmd,"cmdat %s 'AT+QNWPREFCFG=\"mode_pref\"' | grep +QNWPREFCFG: | awk '{print$2}'",path_usb);
	memset(tmp_mode_pref,0,sizeof(tmp_mode_pref));
	cmd_recieve_str(cmd,tmp_mode_pref);
	printf("tmp_mode_pref: %s;\n",tmp_mode_pref);
	memset(new_mode_pref,0,sizeof(new_mode_pref));
	sprintf(new_mode_pref,"\"mode_pref\",%s",mode);
	if(strcmp(tmp_mode_pref,new_mode_pref)!=0)
	{
		//printf("tmp_mode_pref: %s;\n",tmp_mode_pref);
		//Debug(tmp_mode_pref,"daemon.debug");
		//printf("new_mode_pref: %s;\n",new_mode_pref);
		//Debug(new_mode_pref,"daemon.debug");
		//设定服务类型
_SETRAT_RG520N_AGAIN:
		memset(cmd,0,sizeof(cmd));
		sprintf(cmd,"cmdat %s 'AT+QNWPREFCFG=\"mode_pref\",%s'",path_usb,mode);
		cmd_recieve_str(cmd,NULL);
		
		memset(cmd,0,sizeof(cmd));
		sprintf(cmd,"cmdat %s 'AT+QNWPREFCFG=\"mode_pref\"' | grep mode_pref | grep %s | wc -l",path_usb,mode);
		printf("%s\n",cmd);
		int flag=cmd_recieve_int(cmd);
		//if(strstr(tmp_buff,"OK"))				//获取设定结果，OK
			//Debug(cmd,"Set AT+QNWPREFCFG=\"mode_pref\"");
		printf("flag: %d\n",flag);
		if(flag!=1)		//获取设定结果，ERROR
		{
			memset(debug,0,sizeof(debug));
			sprintf(debug,"set rat mode failed, %s",mode);
			if(count_loop==0)
				Debug(debug,"daemon.error");
			goto _SETRAT_RG520N_AGAIN;
		}
	}

	//锁定频段
	char str_wcdma[100];
	char str_lte[100];
	char str_nr[100];
	char str_nsa[100];
	memset(str_wcdma,0,sizeof(str_wcdma));
	memset(str_lte,0,sizeof(str_lte));
	memset(str_nr,0,sizeof(str_nr));
	memset(str_nsa,0,sizeof(str_nsa));
	if(strlen(wcdma)>0)
	{
		if(strcmp(wcdma,"0")==0)
			if(strstr(value_module,"cn"))
				strcpy(str_wcdma,"1:8");
			else if(strstr(value_module,"gl"))
				strcpy(str_wcdma,"1:2:4:5:8:19");
			else
				strcpy(str_wcdma,"1:5:8");
		else
			sprintf(str_wcdma,"%s",wcdma);
		memset(cmd,0,sizeof(cmd));
		sprintf(cmd,"cmdat %s 'AT+QNWPREFCFG=\"gw_band\",%s'",path_usb,str_wcdma);
		memset(tmp_buff,0,sizeof(tmp_buff));
		cmd_recieve_str(cmd,tmp_buff);
		if(count_loop==0)
		{
			if(strstr(tmp_buff,"ERROR"))
			{
				memset(debug,0,sizeof(debug));
				sprintf(debug,"lock wcdma bands failed, %s",str_wcdma);
				Debug(debug,"daemon.error");
			}
		}
	}
	if(strlen(lte)>0)
	{
		if(strcmp(lte,"0")==0)
			if(strstr(value_module,"cn"))
				strcpy(str_lte,"1:3:5:8:34:38:39:40:41");
			else if(strstr(value_module,"gl"))
				strcpy(str_lte,"1:2:3:4:5:7:8:12:13:14:17:18:19:20:25:26:28:29:30:32:34:38:39:40:41:42:43:46:48:66:71");
			else
				strcpy(str_lte,"1:3:5:7:8:20:28:32:38:40:41:42:43");
		else
			sprintf(str_lte,"%s",lte);
		memset(cmd,0,sizeof(cmd));
		sprintf(cmd,"cmdat %s 'AT+QNWPREFCFG=\"lte_band\",%s'",path_usb,str_lte);
		memset(tmp_buff,0,sizeof(tmp_buff));
		cmd_recieve_str(cmd,tmp_buff);
		if(count_loop==0)
		{
			if(strstr(tmp_buff,"ERROR"))
			{
				memset(debug,0,sizeof(debug));
				sprintf(debug,"lock lte bands failed, %s",str_lte);
				Debug(debug,"daemon.error");
			}
		}
	}
	if(strlen(nr)>0)
	{	
		if(strcmp(nr,"0")==0)
			if(strstr(value_module,"cn"))
				strcpy(str_nr,"1:3:5:8:28:41:78:79");
			else if(strstr(value_module,"gl"))
				strcpy(str_nr,"1:2:3:5:7:8:12:13:14:18:20:25:26:28:29:30:38:40:41:48:66:70:71:75:76:77:78:79");
			else
				strcpy(str_nr,"1:3:5:7:8:20:28:38:40:41:75:76:77:78");
		else
			sprintf(str_nr,"%s",nr);
		memset(cmd,0,sizeof(cmd));
		sprintf(cmd,"cmdat %s 'AT+QNWPREFCFG=\"nr5g_band\",%s'",path_usb,str_nr);
		memset(tmp_buff,0,sizeof(tmp_buff));
		cmd_recieve_str(cmd,tmp_buff);
		if(count_loop==0)
		{
			if(strstr(tmp_buff,"ERROR"))
			{
				memset(debug,0,sizeof(debug));
				sprintf(debug,"lock nr bands failed, %s",str_nr);
				Debug(debug,"daemon.error");
			}
		}
	}
	if(!strstr(tmp_mode_pref,new_mode_pref))
	{
		if(strcmp(mode,"AUTO")==0)
		{
			//Debug("reset_RG520N","daemon.debug");
			reset_RG520N();
		}
		if(strlen(arfcn)<=0)
			setFlightmode();
	}
}

void setRATMODE_EC20(char *mode,char *gsm,char *wcdma,char *lte)
{

	char cmd[1024];
	char tmp_buff[1024];
	char str_lte[100];
	char str_gsm[100];
	char str_wcdma[100];
	memset(str_lte,0,sizeof(str_lte));
	memset(str_gsm,0,sizeof(str_gsm));
	memset(str_wcdma,0,sizeof(str_wcdma));
	memset(cmd,0,sizeof(cmd));
	sprintf(cmd,"cmdat %s 'AT+QCFG=\"nwscanmode\",%s'",path_usb,mode);
	cmd_recieve_str(cmd,NULL);
	char band_buff[50];
	memset(cmd,0,sizeof(cmd));
	sprintf(cmd,"cmdat %s 'AT+QCFG=\"band\"' | grep '+QCFG:' | awk -F ' ' '{print$2}'",path_usb);
	memset(band_buff,0,sizeof(band_buff));
	cmd_recieve_str(cmd,band_buff);
	//Debug(band_buff,"daemon.debug");
	char band_cmp[50];
	char band_lock[50];
	memset(band_cmp,0,sizeof(band_cmp));
	memset(band_lock,0,sizeof(band_lock));
	if(strstr(value_module,"ec20-eux"))
	{
		if(strlen(gsm)>0)
		{
			if(strcmp(gsm,"0")==0)
				sprintf(str_gsm,"93");
			else
				sprintf(str_gsm,"%s",gsm);
			sprintf(band_cmp,"\"band\",0x%s,0x1a0080800c5,0x0",str_gsm);
			sprintf(band_lock,"\"band\",%s,1a0080800c5,0",str_gsm);
		}
		else if(strlen(wcdma)>0)
		{
			if(strcmp(wcdma,"0")==0)
				sprintf(str_wcdma,"93");
			else
				sprintf(str_wcdma,"%s",wcdma);
			sprintf(band_cmp,"\"band\",0x%s,0x1a0080800c5,0x0",str_wcdma);
			sprintf(band_lock,"\"band\",%s,1a0080800c5,0",str_wcdma);
		}
		else if(strlen(lte)>0)
		{
			if(strcmp(lte,"0")==0)
				sprintf(str_lte,"1a0080800c5");
			else
				sprintf(str_lte,"%s",lte);
			sprintf(band_cmp,"\"band\",0x93,0x%s,0x0",str_lte);
			sprintf(band_lock,"\"band\",93,%s,0",str_lte);
		}
		else
		{
			sprintf(band_cmp,"\"band\",0x93,0x1a0080800c5,0x0");
			sprintf(band_lock,"\"band\",93,1a0080800c5,0");
		}
	}
	if(strstr(value_module,"ec20-af"))
	{
		if(strlen(wcdma)>0)
		{
			if(strcmp(wcdma,"0")==0)
				sprintf(str_wcdma,"260");
			else
				sprintf(str_wcdma,"%s",wcdma);
			sprintf(band_cmp,"\"band\",0x%s,0x42000000000000381a,0x0",str_wcdma);
			sprintf(band_lock,"\"band\",%s,42000000000000381a,0",str_wcdma);
		}
		else if(strlen(lte)>0)
		{
			if(strcmp(lte,"0")==0)
				sprintf(str_lte,"1a0080800c5");
			else
				sprintf(str_lte,"%s",lte);
			sprintf(band_cmp,"\"band\",0x260,0x%s,0x0",str_lte);
			sprintf(band_lock,"\"band\",260,%s,0",str_lte);
		}
		else
		{
			sprintf(band_cmp,"\"band\",0x260,0x42000000000000381a,0x0");
			sprintf(band_lock,"\"band\",260,42000000000000381a,0");
		}
	}
	if(strcmp(band_buff,band_cmp)!=0)
	{
		//Debug(band_buff,"daemon.debug");
		//Debug(band_lock,"daemon.debug");
		memset(cmd,0,sizeof(cmd));
		memset(tmp_buff,0,sizeof(tmp_buff));
		sprintf(cmd,"cmdat %s 'AT+QCFG=%s'",path_usb,band_lock);
		cmd_recieve_str(cmd,tmp_buff);
		if(strstr(tmp_buff,"OK"))
		{
			//Debug("set band OK","daemon.debug");
		}
	}
	
}

/* 功能：对rm500u模组进行拨号
** 返回值：void
*/
void Dial_RM500U()
{
	int count=0;
	char cmd[1024];
	//设置modem网卡信息：协议、网卡名和跃点数
	memset(cmd,0,sizeof(cmd));
	sprintf(cmd,"uci set network.%s.proto='dhcp'",name_config);
	cmd_recieve_str(cmd,NULL);
	memset(cmd,0,sizeof(cmd));
	sprintf(cmd,"uci set network.%s.device='%s'",name_config,value_ifname);
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
	if(value_mtu != NULL && strlen(value_mtu)>0)
	{
		sprintf(cmd,"uci set network.%s.mtu='%s'",name_config,value_mtu);
		cmd_recieve_str(cmd,NULL);
	}else{
		sprintf(cmd,"uci set network.%s.mtu='1400'",name_config);
		cmd_recieve_str(cmd,NULL);
	}
	memset(cmd,0,sizeof(cmd));
	strcpy(cmd,"uci commit network");
	cmd_recieve_str(cmd,NULL);
	//获取SIM卡当前驻网状态
	char tmp_buff1[1024];
	char tmp_buff2[1024];
	char tmp_buff3[1024];
	char tmp_buff4[1024];
	memset(tmp_buff1,0,sizeof(tmp_buff1));
	memset(tmp_buff2,0,sizeof(tmp_buff2));
	memset(tmp_buff3,0,sizeof(tmp_buff3));
	memset(tmp_buff4,0,sizeof(tmp_buff4));
	memset(cmd,0,sizeof(cmd));
	sprintf(cmd,"cmdat %s 'AT+CEREG?'| grep \"0,1\" | wc -l",path_usb);
	int result1=cmd_recieve_int(cmd);
	memset(cmd,0,sizeof(cmd));
	sprintf(cmd,"cmdat %s 'AT+CEREG?'| grep \"0,5\" | wc -l",path_usb);
	int result2=cmd_recieve_int(cmd);
	memset(cmd,0,sizeof(cmd));
	sprintf(cmd,"cmdat %s 'AT+C5GREG?'| grep \"0,1\" | wc -l",path_usb);
	int result3=cmd_recieve_int(cmd);
	memset(cmd,0,sizeof(cmd));
	sprintf(cmd,"cmdat %s 'AT+C5GREG?'| grep \"0,5\" | wc -l",path_usb);
	int result4=cmd_recieve_int(cmd);
	//CEREG：0,1 0,5 时尝试拨号
	if( result1 ==1 || result2 ==1 || result3 ==1 || result4 ==1)
	{
		//判断是否处于已拨号状态
_DIAL_R500U_AGAIN:
		memset(cmd,0,sizeof(cmd));
		sprintf(cmd,"cmdat %s \"AT+QNETDEVSTATUS=1\" 1000 | grep \"+QNETDEVSTATUS:\" | wc -l",path_usb);
		int result3=cmd_recieve_int(cmd);
		if(result3 ==1 )	//是，重启网卡
		{
			cmd_recieve_str(cmd_ifup,NULL);
			//Debug(cmd_ifup,NULL);
			Debug("Dial R500X OK",NULL);
		}
		else{				//否，尝试拨号
			memset(cmd,0,sizeof(cmd));
			sprintf(cmd,"cmdat %s \"AT+QNETDEVCTL=1,3,1\" > /dev/null 2>&1",path_usb);
			cmd_recieve_str(cmd,NULL);
			sleep(1);
			if(count++<3){	//通过goto再次判断,最多3次
				goto _DIAL_R500U_AGAIN;
			}else{			//3次后判定拨号失败
				Debug("Dial R500X Fialed",NULL);
				count=0;
			}
		}
	}
	return;
}

void Dial_RM500Q()
{
	int count=0;
	char cmd[1024];
	//设置modem网卡信息：协议、网卡名和跃点数
	memset(cmd,0,sizeof(cmd));
	sprintf(cmd,"uci set network.%s.proto='dhcp'",name_config);
	cmd_recieve_str(cmd,NULL);
	memset(cmd,0,sizeof(cmd));
	sprintf(cmd,"uci set network.%s.device='%s'",name_config,value_ifname);
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
	if(value_mtu != NULL && strlen(value_mtu)>0)
	{
		sprintf(cmd,"uci set network.%s.mtu='%s'",name_config,value_mtu);
		cmd_recieve_str(cmd,NULL);
	}else{
		sprintf(cmd,"uci set network.%s.mtu='1400'",name_config);
		cmd_recieve_str(cmd,NULL);
	}
	memset(cmd,0,sizeof(cmd));
	strcpy(cmd,"uci commit network");
	cmd_recieve_str(cmd,NULL);
	//获取SIM卡当前驻网状态
	char tmp_buff1[1024];
	char tmp_buff2[1024];
	char tmp_buff3[1024];
	char tmp_buff4[1024];
	memset(tmp_buff1,0,sizeof(tmp_buff1));
	memset(tmp_buff2,0,sizeof(tmp_buff2));
	memset(tmp_buff3,0,sizeof(tmp_buff3));
	memset(tmp_buff4,0,sizeof(tmp_buff4));
	memset(cmd,0,sizeof(cmd));
	sprintf(cmd,"cmdat %s 'AT+CEREG?'| grep \"0,1\" | wc -l",path_usb);
	int result1=cmd_recieve_int(cmd);
	memset(cmd,0,sizeof(cmd));
	sprintf(cmd,"cmdat %s 'AT+CEREG?'| grep \"0,5\" | wc -l",path_usb);
	int result2=cmd_recieve_int(cmd);
	memset(cmd,0,sizeof(cmd));
	sprintf(cmd,"cmdat %s 'AT+C5GREG?'| grep \"0,1\" | wc -l",path_usb);
	int result3=cmd_recieve_int(cmd);
	memset(cmd,0,sizeof(cmd));
	sprintf(cmd,"cmdat %s 'AT+C5GREG?'| grep \"0,5\" | wc -l",path_usb);
	int result4=cmd_recieve_int(cmd);
	//CEREG：0,1 0,5 时尝试拨号
	if( result1 ==1 || result2 ==1 || result3 ==1 || result4 ==1)
	{
		//判断是否处于已拨号状态
_DIAL_R500Q_AGAIN:
		memset(cmd,0,sizeof(cmd));
		char value_ip_dial[100];
		memset(value_ip_dial,0,sizeof(value_ip_dial));
		sprintf(cmd,"cmdat %s AT+CGPADDR=1 | grep \"+CGPADDR:\" | awk -F ',' '{print$1}'| awk -F ' ' '{print$2}'",path_usb);
		cmd_recieve_str(cmd,value_ip_dial);
		//存在拨号IP，尝试拨号
		if(strlen(value_ip_dial)==1)
		//if(result3 ==1 )	//是，重启网卡
		{
			cmd_recieve_str(cmd_ifup,NULL);
			//Debug(cmd_ifup,NULL);
			Debug("dial R500Q OK","daemon.info");
		}
		else{				//否，尝试拨号
			memset(cmd,0,sizeof(cmd));
			sprintf(cmd,"cmdat %s \"AT+CGACT=1,1\" > /dev/null 2>&1",path_usb);
			cmd_recieve_str(cmd,NULL);
			sleep(1);
			if(count++<3){	//通过goto再次判断,最多3次
				goto _DIAL_R500Q_AGAIN;
			}else{			//3次后判定拨号失败
				Debug("Dial R500X Failed","daemon.info");
				count=0;
			}
		}
	}
	return;
}

/* 功能：对rg520n模组进行拨号
** 返回值：void
*/
void Dial_RG520N(char *apn,char *username,char *password,char *auth,char *ip)
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
	int result;
	//CEREG：0,1 0,5 时尝试拨号
	if( strstr(tmp_C5GREG,"0,1") || strstr(tmp_C5GREG,"0,5") || strstr(tmp_C5GREG,"0,4") || strstr(tmp_CGREG,"0,1") || strstr(tmp_CGREG,"0,5") || strstr(tmp_CGREG,"0,4") || strstr(tmp_CEREG,"0,1") || strstr(tmp_CEREG,"0,5") || strstr(tmp_CEREG,"0,4"))
	{
		if(flag_ims!=1)
		{
			memset(cmd,0,sizeof(cmd));
			sprintf(cmd,"cmdat %s 'AT+QCFG=\"ims\"' | grep \"+QCFG:\" | awk -F ',' '{print$(NF-1)}'",path_usb);
			memset(buff,0,sizeof(buff));
			cmd_recieve_str(cmd,buff);
			if(!strstr(buff,"0"))
			{
				memset(cmd,0,sizeof(cmd));
				sprintf(cmd,"cmdat %s 'AT+QCFG=\"ims\",0'",path_usb);
				memset(buff,0,sizeof(buff));
				cmd_recieve_str(cmd,buff);
			}
		}
		else
		{
			memset(cmd,0,sizeof(cmd));
			sprintf(cmd,"cmdat %s 'AT+QCFG=\"ims\"' | grep \"+QCFG:\" | awk -F ',' '{print$(NF-1)}'",path_usb);
			memset(buff,0,sizeof(buff));
			cmd_recieve_str(cmd,buff);
			if(!strstr(buff,"1"))
			{
				memset(cmd,0,sizeof(cmd));
				sprintf(cmd,"cmdat %s 'AT+QCFG=\"ims\",1'",path_usb);
				memset(buff,0,sizeof(buff));
				cmd_recieve_str(cmd,buff);
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
		
		if(apn != NULL && strlen(apn) > 0)
		{
			if(username != NULL && strlen(username) > 0)
			{
				memset(cmd,0,sizeof(cmd));
				sprintf(cmd,"quectel-CM -s %s %s %s %s %s &",apn,username,password,auth,tmp_type_ip);
				system(cmd);
			}else{
				memset(cmd,0,sizeof(cmd));
				sprintf(cmd,"quectel-CM -s %s %s &",apn,tmp_type_ip);
				system(cmd);
			}
		}
		else
		{
			memset(cmd,0,sizeof(cmd));
			sprintf(cmd,"quectel-CM %s &",tmp_type_ip);
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
				sleep(1);
				//额外作读卡判断
				memset(cmd,0,sizeof(cmd));
				sprintf(cmd,"cmdat %s 'AT+CPIN?' | grep \"+CPIN:\"",path_usb);
				memset(buff,0,sizeof(buff));
				cmd_recieve_str(cmd,buff);
				if(!strstr(buff,"READY"))
				{
					return;
				}
				if(count_checkip==0 || count_checkip==60)
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
					if(count_checkip==60)
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

void Dial_RM520N()
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
		memset(cmd,0,sizeof(cmd));
		sprintf(cmd,"cmdat %s 'AT+QMAP=\"auto_connect\"' | grep '+QMAP: \"auto_connect\",0' | awk -F ',' '{print$NF}'",path_usb);
		int auto_connect=cmd_recieve_int(cmd);
		if(auto_connect==1)
		{
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
					usleep(1500);
					int check_readcard=0;
					while(1)
					{
						//额外作读卡判断
						memset(cmd,0,sizeof(cmd));
						sprintf(cmd,"cmdat %s 'AT+CPIN?' | grep \"+CPIN:\"",path_usb);
						memset(buff,0,sizeof(buff));
						cmd_recieve_str(cmd,buff);
						if(strstr(buff,"READY"))
							break;
						if(check_readcard==3)
							return;
						usleep(500);
						check_readcard++;
					}
					if(count_checkip==2 || count_checkip==10 || count_checkip==60)
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
						if(count_checkip==60)
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
		else
		{
			memset(cmd,0,sizeof(cmd));
			sprintf(cmd,"cmdat %s 'AT+QMAP=\"auto_connect\",0,1'",path_usb);
			cmd_recieve_int(cmd);
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

void Dial_EC20(char *apn,char *username,char *password,char *auth,char *ip)
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
	//CEREG：0,1 0,5 时尝试拨号
	if( result1 ==1 || result2 ==1 || result3 ==1 || result4 ==1 || result5 ==1 || result6 ==1)
	{
		char tmp_type_ip[20];
		memset(tmp_type_ip,0,sizeof(tmp_type_ip));
		sprintf(tmp_type_ip,"");
		if (strstr(ip,"IPV4"))
			sprintf(tmp_type_ip,"");
		if(strstr(ip,"IPV6"))
			sprintf(tmp_type_ip,"-6");
		if(strstr(ip,"IPV4V6"))
			sprintf(tmp_type_ip,"-4 -6");
		if(apn != NULL && strlen(apn) > 0)
		{
			if(username != NULL && strlen(username) > 0)
			{
				memset(cmd,0,sizeof(cmd));
				sprintf(cmd,"quectel-CM -s %s %s %s %s %s &",apn,username,password,auth,tmp_type_ip);
				system(cmd);
			}else{
				memset(cmd,0,sizeof(cmd));
				sprintf(cmd,"quectel-CM -s %s %s &",apn,tmp_type_ip);
				system(cmd);
			}
		}
		else
		{
			memset(cmd,0,sizeof(cmd));
			sprintf(cmd,"quectel-CM %s &",tmp_type_ip);
			system(cmd);
		}
		int count_checkip=0;
		cmd_recieve_str(cmd_ifup,NULL);
		while(1) //执行quectel后循环查询网卡是否存在IP来确定是否要打印拨号成功标识
		{
			char value_ip_ifconfig[500];
			memset(value_ip_ifconfig,0,sizeof(value_ip_ifconfig));
			memset(cmd,0,sizeof(cmd));
			sprintf(cmd,"ifconfig %s | grep \"inet addr\" | awk -F ':' '{print$2}' | awk -F ' ' '{print$1}'",value_ifname);
			cmd_recieve_str(cmd,value_ip_ifconfig);
			//不存在网卡IP时，尝试拨号
			if(value_ip_ifconfig == NULL || strlen(value_ip_ifconfig)==0)
			{
				sleep(5);
				//额外作读卡判断
				memset(cmd,0,sizeof(cmd));
				sprintf(cmd,"cmdat %s 'AT+CPIN?' | grep \"+CPIN:\"",path_usb);
				memset(buff,0,sizeof(buff));
				cmd_recieve_str(cmd,buff);
				if(!strstr(buff,"READY"))
				{
					return;
				}
				if(count_checkip==60)
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
				}
				dial_flight=0;
				return;
			}
		}
	}
	else
	{
		char tmp_cgreg[100];
		char tmp_cereg[100];
		memset(tmp_cgreg,0,sizeof(tmp_cgreg));
		memset(tmp_cereg,0,sizeof(tmp_cereg));
		memset(cmd,0,sizeof(cmd));
		sprintf(cmd,"cmdat %s AT+CGREG? | grep \"+CGREG: \" | awk -F ' ' '{print$2}'",path_usb);
		cmd_recieve_str(cmd,tmp_cgreg);
		memset(cmd,0,sizeof(cmd));
		sprintf(cmd,"cmdat %s AT+CEREG? | grep \"+CEREG: \" | awk -F ' ' '{print$2}'",path_usb);
		cmd_recieve_str(cmd,tmp_cereg);
		
		if(strcmp(tmp_cgreg,debug_cgreg)!=0 || strcmp(tmp_cereg,debug_cereg)!=0)
		{
			if(strcmp(tmp_cgreg,debug_cgreg)!=0)
			{
				sprintf(debug_cgreg,"%s",tmp_cgreg);
			}
			
			if(strcmp(tmp_cereg,debug_cereg)!=0)
			{
				sprintf(debug_cereg,"%s",tmp_cereg);
			}
			
			char debug[100];
			memset(debug,0,sizeof(debug));
			char stat_cgreg[1024];
			char stat_cereg[1024];
			memset(stat_cgreg,0,sizeof(stat_cgreg));
			memset(stat_cereg,0,sizeof(stat_cereg));
			trans_register_status_RG520N(debug_cgreg,stat_cgreg);
			trans_register_status_RG520N(debug_cereg,stat_cereg);
			if(strcmp(debug_cgreg,debug_cereg)==0)
				sprintf(debug,"EPS & GPRS: %s, reboot cellular network",stat_cgreg);
			else
				sprintf(debug,"EPS: %s; GPRS: %s, reboot cellular network",stat_cereg,stat_cgreg);
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

void SwitchSIM_RM500U(bool isdefault)
{
	char cmd[1024];
	char tmp_buff[1024];
	int value_nextcard=value_defaultcard;
	if(isdefault) //设置默认卡槽
	{
		memset(cmd,0,sizeof(cmd));
		sprintf(cmd,"cmdat %s 'AT+QUIMSLOT=%d'",path_usb,value_defaultcard);
		cmd_recieve_str(cmd,NULL);
		sprintf(tmp_buff,"%d",value_defaultcard);
		Debug(tmp_buff,"Set AT+QUIMSLOT");
		sleep(3);
	}
	else 		//获取当前卡槽并切换卡槽
	{
		memset(cmd,0,sizeof(cmd));
		sprintf(cmd,"cmdat %s 'AT+QUIMSLOT?' | grep \"+QUIMSLOT:\" | awk -F ' ' '{print $2}'",path_usb); //获取当前卡槽
		int value_currentcard = cmd_recieve_int(cmd); 
		if(value_currentcard == 1) 			//[当前卡槽] 1
			value_nextcard = 2;
		else if(value_currentcard == 2)		//[当前卡槽] 2
			value_nextcard = 1;
		memset(tmp_buff,0,sizeof(tmp_buff));
		sprintf(tmp_buff,"%d",value_currentcard);
		Debug(tmp_buff,"Current card slot");
		//切换卡槽
		memset(cmd,0,sizeof(cmd));
		sprintf(cmd,"cmdat %s 'AT+QUIMSLOT=%d'",path_usb,value_nextcard);
		cmd_recieve_str(cmd,NULL);
		memset(tmp_buff,0,sizeof(tmp_buff));
		sprintf(tmp_buff,"%d",value_nextcard);
		Debug(tmp_buff,"Set AT+QUIMSLOT");
		sleep(3);
	}
}

void SwitchSIM_RG520N(bool isdefault)
{
	char cmd[1024];
	char tmp_buff[1024];
	int value_nextcard;
	if(isPCIE())
	{
		if(isdefault) //设置默认卡槽
		{
			memset(cmd,0,sizeof(cmd));
			sprintf(cmd,"echo 0 > /sys/class/leds/sim_sw/brightness");
			cmd_recieve_str(cmd,NULL);
			sprintf(tmp_buff,"%s",value_defaultcard);
			sleep(1);
			setFlightmode();
			setModem1();
		}
		else 		//获取当前卡槽并切换卡槽
		{
			memset(cmd,0,sizeof(cmd));
			sprintf(cmd,"cat /sys/class/leds/sim_sw/brightness"); //获取当前卡槽
			int value_currentcard = cmd_recieve_int(cmd); 
			if (value_currentcard == 255)
				value_currentcard = 1;
			if(value_currentcard == 0) 			//[当前卡槽] 0
				value_nextcard = 1;
			else if(value_currentcard == 1)		//[当前卡槽] 1
				value_nextcard = 0;
			memset(tmp_buff,0,sizeof(tmp_buff));
			sprintf(tmp_buff,"%d",value_currentcard);
			//切换卡槽
			memset(cmd,0,sizeof(cmd));
			sprintf(cmd,"echo %d > /sys/class/leds/sim_sw/brightness",value_nextcard);
			cmd_recieve_str(cmd,NULL);
			memset(tmp_buff,0,sizeof(tmp_buff));
			sprintf(tmp_buff,"%d",value_nextcard);
			sleep(1);
			setFlightmode();
			if(value_nextcard==0)
				setModem1();
			else if(value_nextcard==1)
				setModem2();
		}
	}
	else
	{
		if(isdefault) //设置默认卡槽
		{
			memset(cmd,0,sizeof(cmd));
			if(strstr(board_name,"R690") || strstr(board_name,"R680-8TH") || strstr(board_name,"RT990"))
				sprintf(cmd,"echo 1 > /sys/class/leds/sim_sw/brightness");
			else if(strstr(board_name,"R660"))
				sprintf(cmd,"echo 1 > /sys/class/gpio/sim_sw/value");
			else if(strstr(board_name,"R650") || strstr(board_name,"R680"))
				sprintf(cmd,"cmdat %s 'AT+QUIMSLOT=1'",path_usb);
			cmd_recieve_str(cmd,NULL);
			sprintf(tmp_buff,"%s",value_defaultcard);
			sleep(1);
			setFlightmode();
			setModem1();
		}
		else 		//获取当前卡槽并切换卡槽
		{
			memset(cmd,0,sizeof(cmd));
			if(strstr(board_name,"R650"))
				sprintf(cmd,"cmdat %s 'AT+QUIMSLOT?' | grep \"+QUIMSLOT:\" | awk -F ' ' '{print $2}'",path_usb);
			else if(strstr(board_name,"R660"))
				sprintf(cmd,"cat /sys/class/gpio/sim_sw/value");
			else if(strcmp(board_name,"R680")==0)
				sprintf(cmd,"cmdat %s 'AT+QUIMSLOT?' | grep \"+QUIMSLOT:\" | awk -F ' ' '{print $2}'",path_usb);
			else
				sprintf(cmd,"cat /sys/class/leds/sim_sw/brightness");
			
				
			int value_currentcard = cmd_recieve_int(cmd); 
			if (value_currentcard == 255)
				value_currentcard = 1;
			if(strcmp(board_name,"R680")==0 || strstr(board_name,"R650"))
			{
				if(value_currentcard == 1) 			//[当前卡槽] 1
					value_nextcard = 2;
				else if(value_currentcard == 2)		//[当前卡槽] 2
					value_nextcard = 1;
			}
			else
			{
				if(value_currentcard == 0) 			//[当前卡槽] 0
					value_nextcard = 1;
				else if(value_currentcard == 1)		//[当前卡槽] 1
					value_nextcard = 0;
			}
			memset(tmp_buff,0,sizeof(tmp_buff));
			sprintf(tmp_buff,"%d",value_currentcard);
			//切换卡槽
			memset(cmd,0,sizeof(cmd));
			if(strstr(board_name,"R650"))
				sprintf(cmd,"cmdat %s 'AT+QUIMSLOT=%d'",path_usb,value_nextcard);
			else if(strstr(board_name,"R660"))
				sprintf(cmd,"echo %d > /sys/class/gpio/sim_sw/value",value_nextcard);
			else if(strcmp(board_name,"R680")==0)
				sprintf(cmd,"cmdat %s 'AT+QUIMSLOT=%d'",path_usb,value_nextcard);
			else
				sprintf(cmd,"echo %d > /sys/class/leds/sim_sw/brightness",value_nextcard);
			cmd_recieve_str(cmd,NULL);
			memset(tmp_buff,0,sizeof(tmp_buff));
			
			setFlightmode();
			printf("value_nextcard:%d",value_nextcard);
			if(strstr(board_name,"R650") || strcmp(board_name,"R680")==0)
			{
				if(value_nextcard==1)
					setModem1();
				else if(value_nextcard==2)
					setModem2();
			}
			else if(strstr(board_name,"RT990") || strstr(board_name,"R690") || strstr(board_name,"R680-8TH") || strstr(board_name,"R660"))
			{
				
				{
					if(value_nextcard==0)
						setModem2();
					else if(value_nextcard==1)
						setModem1();
				}
			}
			//if(strstr(value_module,"fm160"))
				//setFlightmode();
		}
	}
}

void SwitchSIM_EC20(bool isdefault)
{
	char cmd[1024];
	char tmp_buff[1024];
	int value_nextcard=value_defaultcard;
	if(isdefault) //设置默认卡槽
	{
		memset(cmd,0,sizeof(cmd));
		if(strstr(board_name,"RT990") || strstr(board_name,"R680"))
			sprintf(cmd,"echo 1 > /sys/class/leds/sim_sw/brightness");
		else if(strstr(board_name,"R660"))
			sprintf(cmd,"echo 1 > /sys/class/gpio/sim_sw/value");
		cmd_recieve_str(cmd,NULL);
		//sleep(3);
		setFlightmode();
		setModem1();
	}
	else 		//获取当前卡槽并切换卡槽
	{
		memset(cmd,0,sizeof(cmd));
		if(strstr(board_name,"R660"))
			sprintf(cmd,"cat /sys/class/gpio/sim_sw/value");
		else
			sprintf(cmd,"cat /sys/class/leds/sim_sw/brightness");
		int value_currentcard = cmd_recieve_int(cmd); 
		if (value_currentcard == 255)
			value_currentcard = 1;
		if(value_currentcard == 0) 			//[当前卡槽] 0
			value_nextcard = 1;
		else if(value_currentcard == 1)		//[当前卡槽] 1
			value_nextcard = 0;
		memset(tmp_buff,0,sizeof(tmp_buff));
		sprintf(tmp_buff,"%d",value_currentcard);
		//切换卡槽
		memset(cmd,0,sizeof(cmd));
		if(strstr(board_name,"R660"))
			sprintf(cmd,"echo %d > /sys/class/gpio/sim_sw/value",value_nextcard);
		else
			sprintf(cmd,"echo %d > /sys/class/leds/sim_sw/brightness",value_nextcard);
		cmd_recieve_str(cmd,NULL);
		memset(tmp_buff,0,sizeof(tmp_buff));
		//sleep(3);
		setFlightmode();
		printf("value_nextcard:%d",value_nextcard);
		if(strstr(board_name,"RT990") || strstr(board_name,"R680") || strstr(board_name,"R660"))
		{
			if(value_nextcard==0)
				setModem2();
			else if(value_nextcard==1)
				setModem1();
		}
			
	}
}

void trans_register_status_RG520N(char *reg,char *status)
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

void get_time(char *time_str)
{
	memset(time_str,0,sizeof(time_str));
	char cmd[100];
	while(1)
	{
		memset(cmd,0,sizeof(cmd));
		sprintf(cmd,"cmdat %s 'AT' | grep OK |wc -l",path_usb);
		int tmp_int_buff = cmd_recieve_int(cmd);
		if (tmp_int_buff == 1)
			break;
		sleep(1);
	}
	memset(cmd,0,sizeof(cmd));
	sprintf(cmd,"cmdat %s 'AT+CTZU=1'",path_usb);
	system(cmd);
	setFlightmode();
	memset(cmd,0,sizeof(cmd));
	sprintf(cmd,"cmdat %s AT+CCLK? | grep +CCLK: | awk -F '\"' '{print$2}'",path_usb);
	cmd_recieve_str(cmd,time_str);
}