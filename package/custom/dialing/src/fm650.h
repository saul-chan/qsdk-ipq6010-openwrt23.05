#include "dialing.h"
/* 功能：设定fm650模组拨号方式
** 返回值：void
*/
void setModule_FM650(char *tmp_mode_usb,char *tmp_mode_nat)
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
	//设定拨号上网驱动
	if( tmp_mode_usb != NULL )
	{
		if(strlen(tmp_mode_usb)>0)
		{
			memset(cmd,0,sizeof(cmd));
			sprintf(cmd,"cmdat %s \"AT+GTUSBMODE?\" | grep \"+GTUSBMODE: \" | awk -F ' ' '{print$2}'",path_usb);
			memset(tmp_str_buff,0,sizeof(tmp_str_buff));
			cmd_recieve_str(cmd,tmp_str_buff);
			sprintf(tmp_debug,"usbmode: %s; ",tmp_str_buff);
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
				sprintf(cmd,"cmdat %s \"AT+GTIPPASS=%s\" | grep \"OK\" |wc -l",path_usb,tmp_mode_nat);
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
			sprintf(cmd,"cmdat %s 'AT+GTDUALSIM?' | grep \"+GTDUALSIM:\" | awk -F ',' '{print $1}' | awk -F ' ' '{print $2}'",path_usb);
			result = cmd_recieve_int(cmd);
			if (result != 0) 
			{
				memset(cmd,0,sizeof(cmd));
				sprintf(cmd,"cmdat %s 'AT+GTDUALSIM=0'",path_usb);
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
			sprintf(cmd,"cmdat %s 'AT+GTDUALSIM?' | grep \"+GTDUALSIM:\" | awk -F ',' '{print $1}' | awk -F ' ' '{print $2}'",path_usb);
			result = cmd_recieve_int(cmd);
			if (result != 0) 
			{
				memset(cmd,0,sizeof(cmd));
				sprintf(cmd,"cmdat %s 'AT+GTDUALSIM=0'",path_usb);
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
			sprintf(cmd,"cmdat %s 'AT+GTDUALSIM?' | grep \"+GTDUALSIM:\" | awk -F ',' '{print $1}' | awk -F ' ' '{print $2}'",path_usb);
			result = cmd_recieve_int(cmd);
			if (result != 0) 
			{
				memset(cmd,0,sizeof(cmd));
				sprintf(cmd,"cmdat %s 'AT+GTDUALSIM=0'",path_usb);
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
			sprintf(cmd,"cmdat %s 'AT+GTDUALSIM?' | grep \"+GTDUALSIM:\" | awk -F ',' '{print $1}' | awk -F ' ' '{print $2}'",path_usb);
			result = cmd_recieve_int(cmd);
			if (result != 0) 
			{
				memset(cmd,0,sizeof(cmd));
				sprintf(cmd,"cmdat %s 'AT+GTDUALSIM=0'",path_usb);
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
				setFlightmode();
				setModem2();
			}
		}
	}
	else
		setModem();
}

void setModule_NL668(char *tmp_mode_usb,char *tmp_mode_nat)
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
				}
				else
					sprintf(tmp_debug,"set usbmode: %s failed; ",tmp_mode_usb);
					
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
					sprintf(tmp_debug,"set ippass: %s OK ",tmp_mode_nat);
				else
					sprintf(tmp_debug,"set ippass: %s failed ",tmp_mode_nat);
					
			}else{	//一致时，打印当前上网方式
				memset(cmd,0,sizeof(cmd));
				sprintf(cmd,"cmdat %s \"AT+GTIPPASS?\" | grep \"+GTIPPASS: \" | awk -F ' ' '{print$2}'",path_usb);
				memset(tmp_str_buff,0,sizeof(tmp_str_buff));
				cmd_recieve_str(cmd,tmp_str_buff);
				sprintf(tmp_debug,"ippass: %s ",tmp_str_buff);
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
	if(strlen(value_defaultcard)>0 && (strstr(value_defaultcard,"auto") || strstr(value_defaultcard,"sim1")))
	{
		memset(cmd,0,sizeof(cmd));
		if(strstr(board_name,"R680"))
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
				//sleep(1);
				setFlightmode_first_reboot();
				setModem1();
			}
		}
		else if(strstr(board_name,"R660"))
		{
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
				setFlightmode_first_reboot();
				setModem1();
			}
		}
		else if(strstr(board_name,"RT990"))
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
				setFlightmode_first_reboot();
				setModem1();
			}
		}
	}
	else if(strlen(value_defaultcard)>0 && strstr(value_defaultcard,"sim2"))
	{
		memset(cmd,0,sizeof(cmd));
		if(strstr(board_name,"R680"))
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
				//sleep(1);
				setFlightmode_first_reboot();
				setModem2();
			}
		}
		else if(strstr(board_name,"R660"))
		{
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
				setFlightmode_first_reboot();
				setModem2();
			}
		}
		else if(strstr(board_name,"RT990"))
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
				//sleep(1);
				setFlightmode_first_reboot();
				setModem2();
			}
		}
	}
	else
	{
		setModem();
	}
	
}

void analyticRSRP_SSINR(int rsrp,float rsrq,float sinr)
{
	int tmp_rsrp=rsrp-141;
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
	float tmp_rsrq=0.0;
	tmp_rsrq=((float)rsrq-40)/2;
	float tmp_sinr=0.0;
	tmp_sinr=((float)sinr)/2;
	uci_cellular_int("SVAL",value_signal);
	uci_cellular_int("RSRP",tmp_rsrp);
	uci_cellular_delete("RSSI");
	if(sinr==0)
	{
		uci_cellular_delete("SINR");
	}
	else
	{
		uci_cellular_float("SINR",(double)tmp_sinr);
	}
	uci_cellular_float("RSRQ",(double)tmp_rsrq);
	
	/* memset(cmd,0,sizeof(cmd));
	sprintf(cmd,"echo \"+RSRP: %d\" >> %s",tmp_rsrp,path_outfile);
	cmd_recieve_str(cmd,NULL);
	memset(cmd,0,sizeof(cmd));
	sprintf(cmd,"echo \"+RSRQ: %.1f\" >> %s",tmp_rsrq,path_outfile);
	cmd_recieve_str(cmd,NULL);
	memset(cmd,0,sizeof(cmd));
	sprintf(cmd,"echo \"+SINR: %.1f\" >> %s",tmp_sinr,path_outfile);
	cmd_recieve_str(cmd,NULL); */
	
	char debug[1024];	
	memset(debug,0,sizeof(debug));
	if(value_signal>2 && sighi_debug == 0)
	{
		sprintf(debug,"rsrp: %d dBm; rsrq: %.1f dB; sinr %.1f dB; signal value: %d",tmp_rsrp,tmp_rsrq,tmp_sinr,value_signal);
		Debug(debug,"cellular.info");
		sighi_debug = 1;
		siglo_debug = 0;
	}
	if(value_signal<2 && siglo_debug == 0)
	{
		sprintf(debug,"low signal, rsrp: %d dBm; rsrq: %.1f dB; sinr %.1f dB; signal value: %d",tmp_rsrp,tmp_rsrq,tmp_sinr,value_signal);
		Debug(debug,"cellular.warn");
		sighi_debug = 0;
		siglo_debug = 1;
	}
}

/* 功能：解析SS_RSRP和SS_RSRQ，转换为信号值
** 返回值：void
*/
void analyticRSRP_SS(int rsrp,float rsrq,float sinr)
{
	char cmd[1024];
	if(rsrp == 255 )				//255		无信号
		value_signal=0;
	else if(rsrp <= 32 )			//ss_rsrp 0~32		RTSRP -156~-125		0
		value_signal=0;
	else if(rsrp <= 42 )			//ss_rsrp 32~42		RSRP -125~-115		1
		value_signal=1;
	else if(rsrp <= 52 )			//ss_rsrp 42~52		RSRP -115~-105		2
		value_signal=2;
	else if(rsrp <= 62 )			//ss_rsrp 52~62		RSRP -105~-95		3
		value_signal=3;
	else if(rsrp <= 72 )			//ss_rsrp 62~72		RSRP -95~-85		4
		value_signal=4;
	else if(rsrp <= 126 )			//ss_rsrp 72~126	RSRP -85~-31		5
		value_signal=5;
	uci_cellular_int("SVAL",value_signal);
	/* memset(cmd,0,sizeof(cmd));
	sprintf(cmd,"echo \"+SVAL: %d\" >> %s",value_signal,path_outfile);
	cmd_recieve_str(cmd,NULL); */
	int tmp_rsrp=0;
	if(rsrp<255)
		tmp_rsrp=(rsrp-157);
	float tmp_rsrq=0.0;
	if(rsrq<255)
		tmp_rsrq=((float)rsrq-87)/2;
	float tmp_sinr=0.0;
	if(sinr<255)
		tmp_sinr=((float)sinr-47)/2;
	uci_cellular_int("RSRP",tmp_rsrp);
	uci_cellular_float("RSRQ",(double)tmp_rsrq);
	uci_cellular_float("SINR",(double)tmp_sinr);
	uci_cellular_delete("RSSI");
	/* memset(cmd,0,sizeof(cmd));
	sprintf(cmd,"echo \"+RSRP: %d\" >> %s",tmp_rsrp,path_outfile);
	cmd_recieve_str(cmd,NULL);
	memset(cmd,0,sizeof(cmd));
	sprintf(cmd,"echo \"+RSRQ: %.1f\" >> %s",tmp_rsrq,path_outfile);
	cmd_recieve_str(cmd,NULL);
	memset(cmd,0,sizeof(cmd));
	sprintf(cmd,"echo \"+SINR: %.1f\" >> %s",tmp_sinr,path_outfile);
	cmd_recieve_str(cmd,NULL); */
	char debug[1024];
	if(value_signal>2 && sighi_debug == 0)
	{
		memset(debug,0,sizeof(debug));
		sprintf(debug,"ss_rsrp: %d dBm; ss_rsrq: %.1f dB; ss_sinr %.1f dB; signal value: %d",tmp_rsrp,tmp_rsrq,tmp_sinr,value_signal);
		Debug(debug,"cellular.info");
		sighi_debug=1;
		siglo_debug=0;
	}
	if(value_signal<2 && siglo_debug == 0)
	{
		memset(debug,0,sizeof(debug));
		sprintf(debug,"low signal, ss_rsrp: %d dBm; ss_rsrq: %.1f dB; ss_sinr %.1f dB; signal value: %d",tmp_rsrp,tmp_rsrq,tmp_sinr,value_signal);
		Debug(debug,"cellular.warn");
		sighi_debug=0;
		siglo_debug=1;
	}
}

/* 功能：解析CESQ，转换为信号值
** 返回值：void
*/
void analyticCESQ()
{
	char tmp_buff[1024];
	char cmd[1024];
	memset(cmd,0,sizeof(cmd));
	memset(tmp_buff,0,sizeof(tmp_buff));
	sprintf(cmd,"cmdat %s \"AT+CESQ\" | grep \"+CESQ: \" | awk -F ' ' '{print$NF}'",path_usb);
	cmd_recieve_str(cmd,tmp_buff);
	int count=countstr(tmp_buff,",");
	char array_CESQ[count+1][50];
	if(separate_string2array(tmp_buff,",",count+1,50,(char *)&array_CESQ) != count+1)
	{
		printf("formatting CESQ failed\n");
	}
	else
	{
		float rsrq=atof(array_CESQ[4]);
		int rsrp=atoi(array_CESQ[5]);
		float ss_rsrq=atof(array_CESQ[6]);
		int ss_rsrp=atoi(array_CESQ[7]);
		float ss_sinr=atof(array_CESQ[8]);
		if(ss_rsrp!=255)
		{
			//5G信号
			analyticRSRP_SS(ss_rsrp,ss_rsrq,ss_sinr);
		}
		else if(rsrp!=255)
		{
			//4G信号
			analyticRSRP_SSINR(rsrp,rsrq,0);
		}
		else
			analyticCSQ();
	}
	//<rxlev>,<ber>,<rscp>,<ecno>,<rsrq>,<rsrp>,<ss_rsrq>,<ss_rsrp>,<ss_sinr>
}

/* 功能：解析fm650所处网络小区信息
** 返回值：void
*/
void analyticINFO_FM650()
{
	char tmp_buff[2048];
	char cmd[1024];
	char nservice[100];
	char band[100];
	char arfcn[100];
	char pci[100];
	char eNB[100];
	char cellID[100];
	char debug[100];
	memset(debug,0,sizeof(debug));
	memset(cmd,0,sizeof(cmd));
	memset(tmp_buff,0,sizeof(tmp_buff));
	memset(nservice,0,sizeof(nservice));
	memset(band,0,sizeof(band));
	sprintf(cmd,"cmdat %s \"AT+GTCCINFO?\" > %s",path_usb,path_at);
	cmd_recieve_str(cmd,NULL);
	memset(tmp_buff,0,sizeof(tmp_buff));
	sprintf(cmd,"cat %s | grep -A 2 \"service cell\" | xargs echo -n",path_at);
	cmd_recieve_str(cmd,tmp_buff);
	if(strstr(tmp_buff,"service cell"))
	{
		int count=countstr(tmp_buff,",");
		char array_CPSI[count+1][50];
		if(separate_string2array(tmp_buff,",",count+1,50,(char *)&array_CPSI) != count+1)
		{
			memset(cmd,0,sizeof(cmd));
			char buff[500];
			memset(buff,0,sizeof(buff));
			sprintf(cmd,"cmdat %s \"AT+GTCCINFO?\" | grep -A 2 \"service cell\" | xargs echo -n",path_usb);
			cmd_recieve_str(cmd,buff);
			sprintf(debug,"formatting the current information of cellular failed, %s\n",buff);
			printf(debug);
			/* uci_cellular("CPSI","NO SERVICE");
			uci_cellular("BAND","-");
			if(strstr(value_module,"fm650") || strstr(value_module,"fm160"))
			{
				uci_cellular("ARFCN","-");
				uci_cellular("PCI","-");
			}  */
		}
		//解析服务类型
		if(strstr(tmp_buff,"LTE-NR"))
		{
			strcat(nservice,"NR5G_NSA");
		}else{
			if(strcmp(array_CPSI[1],"0")==0)
			{
				strcat(nservice,"INVALID NETWORK");
				if(!strstr(debug_nservice,nservice))
				{
					Debug("invalid network","cellular.error");
					sprintf(debug_nservice,"%s",nservice);
					analyticCSQ();
				}
				uci_cellular("CPSI","INVALID NETWORK");
				uci_cellular("BAND","-");
				//if(strstr(value_module,"fm650") || strstr(value_module,"fm160"))
				{
					uci_cellular("eNB","-");
					uci_cellular("cellID","-");
					uci_cellular("ARFCN","-");
					uci_cellular("PCI","-");
				}
				return;
			}
			else if(strcmp(array_CPSI[1],"1")==0)
				strcat(nservice,"GSM");
			else if(strcmp(array_CPSI[1],"2")==0)
				strcat(nservice,"WCDMA");
			else if(strcmp(array_CPSI[1],"3")==0)
				strcat(nservice,"TDSCDMA");
			else if(strcmp(array_CPSI[1],"4")==0)
				strcat(nservice,"LTE");
			else if(strcmp(array_CPSI[1],"5")==0)
				strcat(nservice,"eMTC");
			else if(strcmp(array_CPSI[1],"6")==0)
				strcat(nservice,"NB-IoT");
			else if(strcmp(array_CPSI[1],"7")==0)
				strcat(nservice,"CDMA");
			else if(strcmp(array_CPSI[1],"8")==0)
				strcat(nservice,"EVDO");
			else if(strcmp(array_CPSI[1],"9")==0)
				strcat(nservice,"NR5G");
		}
		
		uci_cellular("CPSI",nservice);
		uci_cellular("BAND",array_CPSI[8]);

		/* memset(cmd,0,sizeof(cmd));
		sprintf(cmd,"echo \"+CPSI: %s\" >> %s",nservice,path_outfile);
		cmd_recieve_str(cmd,NULL); */
		//解析上网频段
		/* memset(cmd,0,sizeof(cmd));
		sprintf(cmd,"echo \"+BAND: %s\" >> %s",array_CPSI[8],path_outfile);
		cmd_recieve_str(cmd,NULL); */
		
		
		//解析小区和射频信道
		//if(strstr(value_module,"fm650") || strstr(value_module,"fm160"))
		{
			if(strcmp(array_CPSI[5],"FFFFFFFFFF")!=0)
			{
				memset(cmd,0,sizeof(cmd));
				memset(eNB,0,sizeof(eNB));
				memset(cellID,0,sizeof(cellID));
				
				if(strstr(nservice,"NR5G"))
				{
					strncpy(eNB, array_CPSI[5], 6);
					eNB[6] = '\0';
					sprintf(cmd,"echo $((0x%s))",eNB);

					memset(eNB,0,sizeof(eNB));
					cmd_recieve_str(cmd,eNB);
					
					strcpy(cellID, array_CPSI[5] + 6);
					cellID[strlen(array_CPSI[5])-6] = '\0';
					sprintf(cmd,"echo $((0x%s))",cellID);
					
					memset(cellID,0,sizeof(cellID));
					cmd_recieve_str(cmd,cellID);
				}
				else
				{
					strncpy(eNB, array_CPSI[5], 5);
					eNB[5] = '\0';
					sprintf(cmd,"echo $((0x%s))",eNB);

					memset(eNB,0,sizeof(eNB));
					cmd_recieve_str(cmd,eNB);
					
					strcpy(cellID, array_CPSI[5] + 5);
					cellID[strlen(array_CPSI[5])-5] = '\0';
					sprintf(cmd,"echo $((0x%s))",cellID);
					
					memset(cellID,0,sizeof(cellID));
					cmd_recieve_str(cmd,cellID);
				}
				uci_cellular("eNB",eNB);
				
				uci_cellular("cellID",cellID);
				
			}
			else
			{
				uci_cellular("eNB","-");
				sprintf(eNB,"-");
				uci_cellular("cellID","-");
				sprintf(cellID,"-");
			}
			
			if(strcmp(array_CPSI[6],"FFFFFFFF")!=0 )
			{
				memset(cmd,0,sizeof(cmd));
				memset(arfcn,0,sizeof(arfcn));
				sprintf(cmd,"echo $((0x%s))",array_CPSI[6]);
				cmd_recieve_str(cmd,arfcn);
				
				uci_cellular("ARFCN",arfcn);
				
				/* memset(cmd,0,sizeof(cmd));
				sprintf(cmd,"echo \"+ARFCN: %s\" >> %s",arfcn,path_outfile);
				cmd_recieve_str(cmd,NULL); */
			}
			else
			{
				uci_cellular("ARFCN","-");
				sprintf(arfcn,"-");
			}
			
			if(strcmp(array_CPSI[7],"FFFF")!=0 )
			{
				memset(cmd,0,sizeof(cmd));
				memset(pci,0,sizeof(pci));
				sprintf(cmd,"echo $((0x%s))",array_CPSI[7]);
				cmd_recieve_str(cmd,pci);
				
				uci_cellular("PCI",pci);
				
				/* memset(cmd,0,sizeof(cmd));
				sprintf(cmd,"echo \"+PCI: %s\" >> %s",pci,path_outfile);
				cmd_recieve_str(cmd,NULL); */
			}
			else
			{
				uci_cellular("PCI","-");
				sprintf(pci,"-");
			}
			if(strcmp(debug_nservice,nservice)!=0 || strcmp(debug_band,array_CPSI[8])!=0 || strcmp(debug_arfcn,arfcn)!=0 || strcmp(debug_pci,pci)!=0 || strcmp(debug_enb,eNB)!=0 || strcmp(debug_cellid,cellID)!=0)
			{
				if(strstr(value_module,"fg160") || strcmp(name_config,"SIM2")==0)
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
				
				}
				else
				{
					memset(cmd,0,sizeof(cmd));
					sprintf(cmd,"cmdat %s \"AT+GTRNDIS?\" | grep \"+GTRNDIS: 1\" | wc -l",path_usb);
					int result=cmd_recieve_int(cmd);
					if(result ==0 )	//信号变化且IP地址不存在时才打印
					{
						sprintf(debug,"%s, band: %s; arfcn: %s; pci: %s",nservice,array_CPSI[8],arfcn,pci);
						Debug(debug,"cellular.info");
					}
				}
				memset(debug_nservice,0,sizeof(debug_nservice));
				memset(debug_band,0,sizeof(debug_band));
				memset(debug_arfcn,0,sizeof(debug_arfcn));
				memset(debug_pci,0,sizeof(debug_pci));
				memset(debug_enb,0,sizeof(debug_enb));
				memset(debug_cellid,0,sizeof(debug_cellid));
				strcpy(debug_nservice,nservice);
				strcpy(debug_band,array_CPSI[8]);
				strcpy(debug_arfcn,arfcn);
				strcpy(debug_pci,pci);
				strcpy(debug_enb,eNB);
				strcpy(debug_cellid,cellID);
			}
			
		}
		/* else
		{
			if(strcmp(debug_nservice,nservice)!=0 || strcmp(debug_band,array_CPSI[8])!=0)
			{
				memset(cmd,0,sizeof(cmd));
				sprintf(cmd,"cmdat %s \"AT+GTRNDIS?\" | grep \"+GTRNDIS: 1\" | wc -l",path_usb);
				int result=cmd_recieve_int(cmd);
				if(result ==0 )	//信号变化且IP地址不存在时才打印
				{
					sprintf(debug,"%s, band: %s",nservice,array_CPSI[8]);
					Debug(debug,"cellular.info");
				}
				memset(debug_nservice,0,sizeof(debug_nservice));
				memset(debug_band,0,sizeof(debug_band));
				strcpy(debug_nservice,nservice);
				strcpy(debug_band,array_CPSI[8]);
			}
		} */
		
		if(strstr(nservice,"NR5G"))	//5G解析CESQ
			analyticRSRP_SS(atoi(array_CPSI[12]),atof(array_CPSI[13]),atof(array_CPSI[10]));
		else if ( strstr(nservice,"NR5G_NSA") || strstr(nservice,"LTE"))
			analyticRSRP_SSINR(atoi(array_CPSI[12]),atof(array_CPSI[13]),atof(array_CPSI[10]));
		else
			analyticCSQ();												//其他解析CSQ
	}
	else
	{
		uci_cellular("CPSI","NO SERVICE");
		uci_cellular("BAND","-");
		///if(strstr(value_module,"fm650") || strstr(value_module,"fm160"))
		{
			uci_cellular("eNB","-");
			uci_cellular("cellID","-");
			uci_cellular("ARFCN","-");
			uci_cellular("PCI","-");
		}
		strcat(nservice,"NO SERVICE");
		if(!strstr(debug_nservice,nservice))
		{
			Debug("acquire the current information of cellular failed","cellular.error");
			sprintf(debug_nservice,"%s",nservice);
			analyticCESQ();
		}
	}
}

void setRATMODE_NL668(char *mode,char *gsm,char *umts,char *lte,char *scdma,char *cdma)
{
	char cmd[1024];
	char debug[1024];
	char tmp_buff[1024];
	char str_cdma[100];
	char str_lte[100];
	char str_gsm[100];
	char str_umts[100];
	char str_scdma[100];
	char str_mode[100];
	memset(str_mode,0,sizeof(str_mode));
	strcat(str_mode,mode);
	memset(str_cdma,0,sizeof(str_cdma));
	memset(str_lte,0,sizeof(str_lte));
	memset(str_gsm,0,sizeof(str_gsm));
	memset(str_umts,0,sizeof(str_umts));
	memset(str_scdma,0,sizeof(str_scdma));
	if(strlen(gsm)>0)
	{
		if(strcmp(gsm,"0")==0)
			memset(str_gsm,0,sizeof(str_gsm));
		else
			sprintf(str_gsm,",%s",gsm);
	}
	if(strlen(umts)>0)
	{
		if(strcmp(umts,"0")==0)
			memset(str_umts,0,sizeof(str_umts));
		else
			sprintf(str_umts,",%s",umts);
	}
	if(strlen(lte)>0)
	{
		if(strcmp(lte,"0")==0)
			memset(str_lte,0,sizeof(str_lte));
		else
			sprintf(str_lte,",%s",lte);
	}
	if(strlen(scdma)>0)
	{
		if(strcmp(scdma,"0")==0)
			memset(str_scdma,0,sizeof(str_scdma));
		else
			sprintf(str_scdma,",%s",scdma);
	}
	if(strlen(cdma)>0)
	{
		if(strcmp(cdma,"0")==0)
			memset(str_cdma,0,sizeof(str_cdma));
		else
			sprintf(str_cdma,",%s",cdma);
	}
	char tmp_mode[50];
	char tmp_band[200];
	memset(cmd,0,sizeof(cmd));
	sprintf(cmd,"cmdat %s \"AT+GTACT?\" | grep \"+GTACT: \" | awk -F ' ' '{print$2}' | awk -F ',' -vOFS=\",\" '{$1=\"\";$2=\"\";$3=\"\";$1=$1;$2=$2;$3=$3}3' | sed 's/...//'",path_usb);
	memset(tmp_band,0,sizeof(tmp_band));
	cmd_recieve_str(cmd,tmp_band);
	memset(cmd,0,sizeof(cmd));
	sprintf(cmd,"cmdat %s \"AT+GTACT?\" | grep \"+GTACT: \" | awk -F ' ' '{print$2}' | awk -F ',' '{print$1}'",path_usb);
	memset(tmp_mode,0,sizeof(tmp_mode));
	cmd_recieve_str(cmd,tmp_mode);
	if(strlen(str_gsm) > 0 || strlen(str_umts) > 0 || strlen(str_lte) > 0 || strlen(str_scdma) > 0 || strlen(str_cdma) > 0)
	{
		memset(cmd,0,sizeof(cmd));
		sprintf(cmd,"cmdat %s AT+GTACT=%s,,%s%s%s%s%s",path_usb,str_mode,str_gsm,str_umts,str_lte,str_scdma,str_cdma);
	}
	else
	{
		memset(cmd,0,sizeof(cmd));
		sprintf(cmd,"cmdat %s AT+GTACT=%s",path_usb,str_mode);
	}
	if(strstr(tmp_mode,"20"))
	{
		memset(tmp_mode,0,sizeof(tmp_mode));
		strcat(tmp_mode,"10");
	}
	if(!strstr(str_mode,tmp_mode) || ((strlen(str_gsm) > 0 || strlen(str_umts) > 0 || strlen(str_lte) > 0 || strlen(str_scdma) > 0 || strlen(str_cdma) > 0) && !strstr(cmd,tmp_band)))
	{
		//Debug(cmd,"daemon.debug");
		cmd_recieve_str(cmd,NULL);
		//setFlightmode();
	}
	//获取当前拨号状态，处于拨号时挂断以重新拨号
	memset(cmd,0,sizeof(cmd));
	sprintf(cmd,"cmdat %s \"AT+GTRNDIS?\" | grep \"+GTRNDIS: 1\" | wc -l",path_usb);
	int result=cmd_recieve_int(cmd);
	if(result == 1)
	{
		
		memset(cmd,0,sizeof(cmd));
		sprintf(cmd,"cmdat %s 'AT+GTRNDIS=0,1'",path_usb);
		memset(tmp_buff,0,sizeof(tmp_buff));
		cmd_recieve_str(cmd,tmp_buff);
		//Debug(cmd,"daemon.debug");
	}
	//Debug("setRatMode end","daemon.debug");
}

/* 功能：设定fm650模组5G频点/小区
** 返回值：void
*/
void set5GCELLLOCK_FM650(char *nr, char* arfcn, char* psc, char* scs)
{
	char cmd[1024];
	char tmp_buff[1024];
	char cmd_celllock[1024];
	memset(cmd_celllock,0,sizeof(cmd_celllock));
	memset(cmd,0,sizeof(cmd));
	sprintf(cmd,"cmdat %s 'AT+GTCELLLOCK?' | grep \"+GTCELLLOCK: \"",path_usb);
	cmd_recieve_str(cmd,cmd_celllock);
	if(strlen(psc)>0)
	{
		memset(cmd,0,sizeof(cmd));
		memset(tmp_buff,0,sizeof(tmp_buff));
		if(strstr(value_module,"fm650"))
		{
			char cmd_setcell[1024];
			memset(cmd_setcell,0,sizeof(cmd_setcell));
			sprintf(cmd_setcell,"+GTCELLLOCK: 1,1,0,%s,%s",arfcn,psc);
			if(strcmp(cmd_setcell,cmd_celllock)!=0)
			{
				sprintf(cmd,"cmdat %s 'AT+GTCELLLOCK=1,1,0,%s,%s'",path_usb,arfcn,psc);
				cmd_recieve_str(cmd,tmp_buff);
			}
		}
		else if(strstr(value_module,"fm160"))
		{
			if(strcmp(nr,"0")==0)
			{
				if(count_loop==0)
					Debug("please specify nr5g band if you want to lock the cell","daemon.error");
			}else
			{
				char cmd_setcell[1024];
				memset(cmd_setcell,0,sizeof(cmd_setcell));
				sprintf(cmd_setcell,"+GTCELLLOCK: 1,1,0,%s,%s,%d,%s",arfcn,psc,scs,nr);
				if(strcmp(cmd_setcell,cmd_celllock)!=0)
				{
					memset(cmd,0,sizeof(cmd));
					memset(tmp_buff,0,sizeof(tmp_buff));
					sprintf(cmd,"cmdat %s 'AT+GTCELLLOCK=1,1,0,%s,%s,%d,%s'",path_usb,arfcn,psc,scs,nr);
					cmd_recieve_str(cmd,tmp_buff);
					if(strcmp(name_config,"SIM2")!=0)
						resetModem();
				}
			}
			
		}
	}
	else
	{
		memset(cmd,0,sizeof(cmd));
		memset(tmp_buff,0,sizeof(tmp_buff));
		if(strstr(value_module,"fm650"))
		{
			char cmd_setcell[1024];
			memset(cmd_setcell,0,sizeof(cmd_setcell));
			sprintf(cmd_setcell,"+GTCELLLOCK: 1,1,1,%s",arfcn);
			if(strcmp(cmd_setcell,cmd_celllock)!=0)
			{
				sprintf(cmd,"cmdat %s 'AT+GTCELLLOCK=1,1,1,%s'",path_usb,arfcn);
				cmd_recieve_str(cmd,tmp_buff);
			}
		}
		else if(strstr(value_module,"fm160"))
		{
			char cmd_setcell[1024];
			memset(cmd_setcell,0,sizeof(cmd_setcell));
			sprintf(cmd_setcell,"+GTCELLLOCK: 1,1,1,%s,,%d",arfcn,scs);
			if(strcmp(cmd_setcell,cmd_celllock)!=0)
			{
				sprintf(cmd,"cmdat %s 'AT+GTCELLLOCK=1,1,1,%s,,%d'",path_usb,arfcn,scs);
				cmd_recieve_str(cmd,tmp_buff);
				if(strcmp(name_config,"SIM2")!=0)
					resetModem();
			}
		}
	}
}

/* 功能：设定fm650模组4G频点/小区
** 返回值：void
*/
void setCELLLOCK_FM650(char* arfcn, char* pci)
{
	char cmd[1024];
	char tmp_buff[1024];
	char cmd_celllock[1024];
GET_CELLLOCK:
	memset(cmd_celllock,0,sizeof(cmd_celllock));
	memset(cmd,0,sizeof(cmd));

	sprintf(cmd,"cmdat %s 'AT+GTCELLLOCK?' | grep \"+GTCELLLOCK: \"",path_usb);
	cmd_recieve_str(cmd,cmd_celllock);
	if(strlen(cmd_celllock)<=0)
	{
		goto GET_CELLLOCK;
	}
	if(strlen(pci)>0)
	{
		char cmd_setcell[1024];
		memset(cmd_setcell,0,sizeof(cmd_setcell));
		sprintf(cmd_setcell,"+GTCELLLOCK: 1,0,0,%s,%s",arfcn,pci);
		//Debug(cmd_celllock,"daemon.debug");
		//Debug(cmd_setcell,"daemon.debug");
		if(strcmp(cmd_setcell,cmd_celllock)!=0)
		{
			memset(cmd,0,sizeof(cmd));
			memset(tmp_buff,0,sizeof(tmp_buff));
			sprintf(cmd,"cmdat %s 'AT+GTCELLLOCK=1,0,0,%s,%s'",path_usb,arfcn,pci);
			cmd_recieve_str(cmd,tmp_buff);
			//Debug(cmd,"daemon.debug");
			if(strstr(value_module,"fm160"))
				if(strcmp(name_config,"SIM2")!=0)
					resetModem();
			else if(strstr(value_module,"nl668"))
				setFlightmode();
		}
	}
	else
	{
		char cmd_setcell[1024];
		memset(cmd_setcell,0,sizeof(cmd_setcell));
		sprintf(cmd_setcell,"+GTCELLLOCK: 1,0,1,%s",arfcn);
		//Debug(cmd_celllock,"daemon.debug");
		//Debug(cmd_setcell,"daemon.debug");
		if(strcmp(cmd_setcell,cmd_celllock)!=0)
		{
			memset(cmd,0,sizeof(cmd));
			memset(tmp_buff,0,sizeof(tmp_buff));
			sprintf(cmd,"cmdat %s 'AT+GTCELLLOCK=1,0,1,%s'",path_usb,arfcn);
			cmd_recieve_str(cmd,tmp_buff);
			//Debug(cmd,"daemon.debug");
			if(strstr(value_module,"fm160"))
				if(strcmp(name_config,"SIM2")!=0)
					resetModem();
			else if(strstr(value_module,"nl668"))
				setFlightmode();
		}
	}
}

/* 功能：恢复fm650模组频点/小区
** 返回值：void
*/
void resetCELLLOCK_FM650()
{
	char cmd[1024];
	char tmp_buff[1024];
	memset(cmd,0,sizeof(cmd));
	memset(tmp_buff,0,sizeof(tmp_buff));
	sprintf(cmd,"cmdat %s AT+GTCELLLOCK? | grep \"+GTCELLLOCK: 1\"",path_usb);
	cmd_recieve_str(cmd,tmp_buff);
	if(strlen(tmp_buff)>0)
	{
		//Debug("reset lockcell","daemon.debug");
		memset(cmd,0,sizeof(cmd));
		sprintf(cmd,"cmdat %s 'AT+GTCELLLOCK=0'",path_usb);
		cmd_recieve_str(cmd,NULL);
		if(strstr(value_module,"fm160"))
			if(strcmp(name_config,"SIM2")!=0)
				resetModem();
		else if(strstr(value_module,"nl668"))
			setFlightmode();
		
	}
}
/* 功能：对fm650模组进行拨号
** 返回值：void
*/
void Dial_FM650()
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
	fix_ifname();
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
	int result;
	//CEREG：0,1 0,5 时尝试拨号
	if( strstr(tmp_C5GREG,"0,1") || strstr(tmp_C5GREG,"0,5") || strstr(tmp_C5GREG,"0,4") || strstr(tmp_CGREG,"0,1") || strstr(tmp_CGREG,"0,5") || strstr(tmp_CGREG,"0,4") || strstr(tmp_CEREG,"0,1") || strstr(tmp_CEREG,"0,5") || strstr(tmp_CEREG,"0,4"))
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
		memset(cmd,0,sizeof(cmd));
		sprintf(cmd,"cmdat %s \"AT+GTRNDIS=1,1\" > /dev/null 2>&1",path_usb);
		system(cmd);
		int checkip_count=0;
		while(1)
		{
			sleep(1);
			//判断是否处于已拨号状态
			memset(cmd,0,sizeof(cmd));
			sprintf(cmd,"cmdat %s \"AT+GTRNDIS?\" | grep \"+GTRNDIS: 1\" | wc -l",path_usb);
			result=cmd_recieve_int(cmd);
			if(result ==1 )	//是，重启网卡
			{
				cmd_recieve_str(cmd_ifup,NULL);
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
		{				//否，尝试拨号
			sleep(23);
			memset(cmd,0,sizeof(cmd));
			sprintf(cmd,"cmdat %s \"AT+GTRNDIS?\" | grep \"+GTRNDIS: 1\" | wc -l",path_usb);
			result=cmd_recieve_int(cmd);
			if(result ==0 )	//是，重启网卡
			{
				if(strcmp(dialresult_debug,"no")!=0){
					//Debug("dial failed","cellular.error");
					memset(dialresult_debug,0,sizeof(dialresult_debug));
					strcpy(dialresult_debug,"no");
				}
				if(dial_flight<5){
					if(dial_flight==0)
						Debug("dial failed, reboot cellular network","cellular.error");
					setFlightmode();
					sleep(sleep_delay[dial_flight]);
					dial_flight++;
				}
				else if(dial_flight==5)
				{
					Debug("dial failure reaches 5 times, reboot module","cellular.error");
					resetModem();
					dial_flight=0;
				}
			}
			else
			{
				cmd_recieve_str(cmd_ifup,NULL);
				if(strcmp(dialresult_debug,"yes")!=0)
				{
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
			}
		}
		status_flight=0;
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
		int goto_debug=0;
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
			trans_register_status_FM650(debug_cgreg,stat_cgreg);
			trans_register_status_FM650(debug_c5greg,stat_c5greg);
			trans_register_status_FM650(debug_cereg,stat_cereg);
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
	return;
}

void Dial_NL668()
{
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
	//if(checkACM())
	fix_ifname();
	printf("ifname: %s",value_ifname);
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
/* 	char tmp_C5GREG[1024]; */
	memset(cmd,0,sizeof(cmd));
	sprintf(cmd,"cmdat %s 'AT+CEREG?'| grep \"+CEREG: \" | awk -F ' ' '{print$2}'",path_usb);
	cmd_recieve_str(cmd,tmp_CEREG);
	memset(cmd,0,sizeof(cmd));
	sprintf(cmd,"cmdat %s 'AT+CGREG?'| grep \"+CGREG: \" | awk -F ' ' '{print$2}'",path_usb);
	cmd_recieve_str(cmd,tmp_CGREG);
/* 	memset(cmd,0,sizeof(cmd));
	sprintf(cmd,"cmdat %s 'AT+C5GREG?'| grep \"+C5GREG: \" | awk -F ' ' '{print$2}'",path_usb);
	cmd_recieve_str(cmd,tmp_C5GREG); */
	
	//CEREG：0,1 0,5 时尝试拨号
	//Debug("check register","daemon.debug");
	int result;
	if( strstr(tmp_CGREG,"0,1") || strstr(tmp_CGREG,"0,5") || strstr(tmp_CGREG,"0,4") || strstr(tmp_CEREG,"0,1") || strstr(tmp_CEREG,"0,5") || strstr(tmp_CEREG,"0,4"))
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
		
		memset(cmd,0,sizeof(cmd));
		sprintf(cmd,"cmdat %s \"AT+GTRNDIS=1,1\" > /dev/null 2>&1",path_usb);
		system(cmd);
		int checkip_count=0;
		while(1)
		{
			sleep(1);
			//判断是否处于已拨号状态
			memset(cmd,0,sizeof(cmd));
			sprintf(cmd,"cmdat %s \"AT+GTRNDIS?\" | grep \"+GTRNDIS: 1\" | wc -l",path_usb);
			result=cmd_recieve_int(cmd);
			if(result ==1 )	//是，重启网卡
			{
				cmd_recieve_str(cmd_ifup,NULL);
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
		
		{				//否，尝试拨号
			sleep(23);
			memset(cmd,0,sizeof(cmd));
			sprintf(cmd,"cmdat %s \"AT+GTRNDIS?\" | grep \"+GTRNDIS: 1\" | wc -l",path_usb);
			result=cmd_recieve_int(cmd);
			if(result ==0 )	//是，重启网卡
			{
				if(strcmp(dialresult_debug,"no")!=0){
					//Debug("dial failed","cellular.error");
					memset(dialresult_debug,0,sizeof(dialresult_debug));
					strcpy(dialresult_debug,"no");
				}
				if(dial_flight<5){
					if(dial_flight==0)
						Debug("dial failed, reboot cellular network","cellular.error");
					setFlightmode();
					sleep(sleep_delay[dial_flight]);
					dial_flight++;
				}
				else if(dial_flight==5)
				{
					Debug("dial failure reaches 5 times, reboot module","cellular.error");
					resetModem();
					dial_flight=0;
				}
			}
			else
			{
				cmd_recieve_str(cmd_ifup,NULL);
				if(strcmp(dialresult_debug,"yes")!=0)
				{
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
			}
		}
		status_flight=0;
	}
	else
	{
		char tmp_cgreg[100];
		char tmp_cereg[100];
		memset(tmp_cgreg,0,sizeof(tmp_cgreg));
		memset(tmp_cereg,0,sizeof(tmp_cereg));
		int count=0;
		while(strlen(tmp_cgreg)<=0)
		{
			memset(cmd,0,sizeof(cmd));
			sprintf(cmd,"cmdat %s AT+CGREG? | grep \"+CGREG: \" | awk -F ' ' '{print$2}'",path_usb);
			cmd_recieve_str(cmd,tmp_cgreg);
			count++;
			if(count==3)
				break;
		}
		count=0;
		while(strlen(tmp_cereg)<=0)
		{
			memset(cmd,0,sizeof(cmd));
			sprintf(cmd,"cmdat %s AT+CEREG? | grep \"+CEREG: \" | awk -F ' ' '{print$2}'",path_usb);
			cmd_recieve_str(cmd,tmp_cereg);
			count++;
			if(count==3)
				break;
		}
		int goto_debug=0;
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
			goto_debug=1;
		}
		if(goto_debug==1)
		{
			char debug[100];
			memset(debug,0,sizeof(debug));
			char stat_cgreg[1024];
			char stat_cereg[1024];
			memset(stat_cgreg,0,sizeof(stat_cgreg));
			memset(stat_cereg,0,sizeof(stat_cereg));
			trans_register_status_FM650(debug_cgreg,stat_cgreg);
			trans_register_status_FM650(debug_cereg,stat_cereg);
			if(strcmp(debug_cereg,debug_cgreg)==0)
				sprintf(debug,"EPS & GPRS: %s, reboot cellular network",stat_cereg);
			else
				sprintf(debug,"EPS: %s; GPRS: %s, reboot cellular network",stat_cereg,stat_cgreg);
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
	return;
}

//支持FM650 FM160 双卡切换的：r660 rt990 r680 r990 r650
void SwitchSIM_FM650(bool isdefault)
{
	char cmd[1024];
	char tmp_buff[1024];
	int value_nextcard=value_defaultcard;
	if(isdefault) //设置默认卡槽
	{
		memset(cmd,0,sizeof(cmd));
		if(strstr(board_name,"R690") || strstr(board_name,"R680-8TH") || strstr(board_name,"RT990"))
			sprintf(cmd,"echo 1 > /sys/class/leds/sim_sw/brightness");
		else if(strstr(board_name,"R660"))
			sprintf(cmd,"echo 1 > /sys/class/gpio/sim_sw/value");
		else if(strstr(board_name,"R650") || strstr(board_name,"R680"))
			sprintf(cmd,"cmdat %s 'AT+GTDUALSIM=0'",path_usb);
		
		cmd_recieve_str(cmd,NULL);
		//sleep(1);
		//if(!strstr(board_name,"R650")) //指令切换卡同样要飞行模式
			setFlightmode();
		setModem1();
	}
	else 		//获取当前卡槽并切换卡槽
	{
		memset(cmd,0,sizeof(cmd));
		if(strstr(board_name,"R650"))
			sprintf(cmd,"cmdat %s 'AT+GTDUALSIM?' | grep \"+GTDUALSIM:\" | awk -F ',' '{print $1}' | awk -F ' ' '{print $2}'",path_usb);
		else if(strstr(board_name,"R660"))
			sprintf(cmd,"cat /sys/class/gpio/sim_sw/value");
		else if(strcmp(board_name,"R680")==0)
			sprintf(cmd,"cmdat %s 'AT+GTDUALSIM?' | grep \"+GTDUALSIM:\" | awk -F ',' '{print $1}' | awk -F ' ' '{print $2}'",path_usb);
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
		if(strstr(board_name,"R650"))
			sprintf(cmd,"cmdat %s 'AT+GTDUALSIM=%d'",path_usb,value_nextcard);
		else if(strstr(board_name,"R660"))
			sprintf(cmd,"echo %d > /sys/class/gpio/sim_sw/value",value_nextcard);
		else if(strcmp(board_name,"R680")==0)
			sprintf(cmd,"cmdat %s 'AT+GTDUALSIM=%d'",path_usb,value_nextcard);
		else
			sprintf(cmd,"echo %d > /sys/class/leds/sim_sw/brightness",value_nextcard);
		cmd_recieve_str(cmd,NULL);
		memset(tmp_buff,0,sizeof(tmp_buff));
		sleep(10);
		//if(!strstr(board_name,"R650"))
		if(strstr(value_module,"fm650"))
			setFlightmode();
		printf("value_nextcard:%d",value_nextcard);
		if(strstr(board_name,"R650") || strcmp(board_name,"R680")==0)
		{
			if(value_nextcard==0)
				setModem1();
			else if(value_nextcard==1)
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
		if(strstr(value_module,"fm160"))
			setFlightmode();
		/* 
		sleep(10); */
	}
}

void SwitchSIM_FG160(bool isdefault)
{
	char cmd[1024];
	char tmp_buff[1024];
	int value_nextcard=value_defaultcard;
	if(isdefault) //设置默认卡槽
	{
		memset(cmd,0,sizeof(cmd));
		sprintf(cmd,"echo 0 > /sys/class/leds/sim_sw/brightness");
		cmd_recieve_str(cmd,NULL);
		setFlightmode();
		//sleep(1);
		//if(!strstr(board_name,"R650")) //指令切换卡同样要飞行模式
			//setFlightmode();
		setModem1();
	}
	else 		//获取当前卡槽并切换卡槽
	{
		memset(cmd,0,sizeof(cmd));
		sprintf(cmd,"cat /sys/class/leds/sim_sw/brightness");
		
			
		int value_currentcard = cmd_recieve_int(cmd); 
		if (value_currentcard == 255)
			value_currentcard = 1;
		if(value_currentcard == 0) 			//[当前卡槽] 0--SIM1
			value_nextcard = 1;
		else if(value_currentcard == 1)		//[当前卡槽] 1--SIM2
			value_nextcard = 0;
		memset(tmp_buff,0,sizeof(tmp_buff));
		sprintf(tmp_buff,"%d",value_currentcard);
		//切换卡槽
		memset(cmd,0,sizeof(cmd));
		sprintf(cmd,"echo %d > /sys/class/leds/sim_sw/brightness",value_nextcard);
		cmd_recieve_str(cmd,NULL);
		
		if(value_nextcard==0)
			setModem1();
		else if(value_nextcard==1)
			setModem2();
		setFlightmode();
		//
	}
}
//支持NL668 双卡切换的：r660 rt990 r680
void SwitchSIM_NL668(bool isdefault)
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
		//sleep(1);
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
		//sleep(1);
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

void setagainFM650()
{
	if(strlen(value_defaultcard)>0)
	{
		char cmd[1024];
		char tmp_buff[1024];
		memset(cmd,0,sizeof(cmd));
		if(strstr(board_name,"R650"))
			sprintf(cmd,"cmdat %s 'AT+GTDUALSIM?' | grep \"+GTDUALSIM:\" | awk -F ',' '{print $1}' | awk -F ' ' '{print $2}'",path_usb);
		else if(strstr(board_name,"R660"))
			sprintf(cmd,"cat /sys/class/gpio/sim_sw/value");
		else if(strcmp(board_name,"R680")==0)
			sprintf(cmd,"cmdat %s 'AT+GTDUALSIM?' | grep \"+GTDUALSIM:\" | awk -F ',' '{print $1}' | awk -F ' ' '{print $2}'",path_usb);
		else
			sprintf(cmd,"cat /sys/class/leds/sim_sw/brightness");
		
			
		int value_currentcard = cmd_recieve_int(cmd); 
		if (value_currentcard == 255)
			value_currentcard = 1;
		if(strstr(board_name,"RT990") || strstr(board_name,"R650") || strcmp(board_name,"R680")==0)
		{
			if(value_currentcard==0)
				setModem1();
			else if(value_currentcard==1)
				setModem2();
		}
		else if(strstr(board_name,"R690") || strstr(board_name,"R680-8TH") || strstr(board_name,"R660"))
		{
			if(value_currentcard==0)
				setModem2();
			else if(value_currentcard==1)
				setModem1();
		}
	}
	else
		setModem();
}

void trans_register_status_FM650(char *reg,char *status)
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
		sprintf(status,"registered for \"SMS only\", roaming (not applicable)");
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