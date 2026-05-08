#include "dialing.h"
/* 功能：设定mt5700模组拨号方式
** 返回值：void
*/
void report_MT5700()
{
	char cmd[1024];
/* 	memset(cmd,0,sizeof(cmd));
	sprintf(cmd,"cmdat %s \"AT^CPOLICYRPT=0\"",path_usb);
	system(cmd);
	
	memset(cmd,0,sizeof(cmd));
	sprintf(cmd,"cmdat %s \"AT^CURC=0\"",path_usb);
	system(cmd);
	 */
	memset(cmd,0,sizeof(cmd));
	sprintf(cmd,"cmdat %s \"AT+CEREG=0\"",path_usb);
	system(cmd);
	
	memset(cmd,0,sizeof(cmd));
	sprintf(cmd,"cmdat %s \"AT+CGREG=0\"",path_usb);
	system(cmd);
	
	memset(cmd,0,sizeof(cmd));
	sprintf(cmd,"cmdat %s \"AT+C5GREG=0\"",path_usb);
	system(cmd);
	
/* 	memset(cmd,0,sizeof(cmd));
	sprintf(cmd,"cmdat %s \"AT+CREG=0\"",path_usb);
	system(cmd); */
}
void setModule_MT5700(char *tmp_mode_usb,char *tmp_mode_nat)
{
	char cmd[1024];
	int tmp_int_buff;
	char tmp_str_buff[1024];
	char debug[100];
	char tmp_debug[100];
	memset(debug,0,sizeof(debug));
	memset(tmp_debug,0,sizeof(tmp_debug));
	//sleep(10);
	//report_MT5700();
	//确认模组正常工作
	while(1)
	{
		//Debug("cmdat AT","daemon.debug");
		memset(cmd,0,sizeof(cmd));
		sprintf(cmd,"cmdat %s 'AT' | grep OK |wc -l",path_usb);
		tmp_int_buff = cmd_recieve_int(cmd);
		if (tmp_int_buff == 1)
			break;
		sleep(1);
	}
	//关闭主动上报
	report_MT5700();
	
	//设定拨号上网驱动
	if( tmp_mode_usb != NULL )
	{
		if(strlen(tmp_mode_usb)>0)
		{
			memset(cmd,0,sizeof(cmd));
			sprintf(cmd,"cmdat %s \"AT^SETMODE?\" | sed -n 2p",path_usb);
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
			sprintf(cmd,"cmdat %s \"AT^SETE5STICK?\" | sed -n 2p",path_usb,tmp_mode_nat);
			tmp_int_buff = cmd_recieve_int(cmd);
			//当前上网方式与设定方式不一致时进行设定
			if (tmp_int_buff == 1)
			{
				memset(cmd,0,sizeof(cmd));
				sprintf(cmd,"cmdat %s \"AT^SETE5STICK=0\" | grep \"OK\" |wc -l",path_usb,tmp_mode_nat);
				tmp_int_buff = cmd_recieve_int(cmd);
				if(tmp_int_buff == 1)
				{
					sprintf(tmp_debug,"set nat: %s OK; ",tmp_mode_nat);
				}
				else
				{
					sprintf(tmp_debug,"set nat: %s failed; ",tmp_mode_nat);
				}
			}else{	//一致时，打印当前上网方式
				memset(cmd,0,sizeof(cmd));
				sprintf(cmd,"cmdat %s \"AT^SETE5STICK?\" | sed -n 2p",path_usb);
				memset(tmp_str_buff,0,sizeof(tmp_str_buff));
				cmd_recieve_str(cmd,tmp_str_buff);
				sprintf(tmp_debug,"nat: %s; ",tmp_str_buff);
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
			memset(cmd,0,sizeof(cmd));
			sprintf(cmd,"cat /sys/class/leds/sim_sw/brightness");
			result = cmd_recieve_int(cmd);
			printf("Current card slot:%d\n",result);
			if (result == 255)
				result=1;
			if (result == 1) //[优先卡槽] == [当前卡槽]
			{
				sprintf(tmp_str_buff,"%s",value_defaultcard);
				//ActiveSIM_MT5700();
				setModem1();
			}else
			{
				memset(cmd,0,sizeof(cmd));
				sprintf(cmd,"echo 1 > /sys/class/leds/sim_sw/brightness");
				cmd_recieve_str(cmd,NULL);
				sprintf(tmp_str_buff,"%s",value_defaultcard);
				//sleep(1);
				setFlightmode();
				ActiveSIM_MT5700();
				setModem1();
			}
		}
		else if(strstr(board_name,"R680"))
		{
			memset(cmd,0,sizeof(cmd));
			sprintf(cmd,"cmdat %s \"AT^SCICHG?\" | grep \"SCICHG:\" | awk -F ' ' '{print$2}' | awk -F ',' '{print$1}'",path_usb);
			result = cmd_recieve_int(cmd);
			printf("Current card slot:%d\n",result);
			
			if (result == 0) //[优先卡槽] == [当前卡槽]
			{
				sprintf(tmp_str_buff,"%s",value_defaultcard);
				//ActiveSIM_MT5700();
				setModem1();
			}else
			{
				memset(cmd,0,sizeof(cmd));
				//sprintf(cmd,"echo 1 > /sys/class/leds/sim_sw/brightness");
				sprintf(cmd,"cmdat %s \"AT^SCICHG=0,1\"",path_usb);
				cmd_recieve_str(cmd,NULL);
				sprintf(tmp_str_buff,"%s",value_defaultcard);
				//sleep(1);
				//setFlightmode();
				//ActiveSIM_MT5700();
				setModem1();
			}
		}
		else if(strstr(board_name,"R660"))
		{
			memset(cmd,0,sizeof(cmd));
			sprintf(cmd,"cat /sys/class/gpio/sim_sw/value");
			result = cmd_recieve_int(cmd);
			printf("Current card slot:%d\n",result);
			if (result == 255)
				result=1;
			if (result == 1) //[优先卡槽] == [当前卡槽]
			{
				sprintf(tmp_str_buff,"%s",value_defaultcard);
				ActiveSIM_MT5700();
				setModem1();
			}else
			{
				memset(cmd,0,sizeof(cmd));
				sprintf(cmd,"echo 1 > /sys/class/gpio/sim_sw/value");
				cmd_recieve_str(cmd,NULL);
				sprintf(tmp_str_buff,"%s",value_defaultcard);
				//sleep(1);
				setFlightmode();
				ActiveSIM_MT5700();
				setModem1();
			}
		}
	}
	else if(strstr(value_defaultcard,"sim2"))
	{
		memset(cmd,0,sizeof(cmd));
		if(strstr(board_name,"R680-8TH") || strstr(board_name,"R690") || strstr(board_name,"RT990"))
		{
			memset(cmd,0,sizeof(cmd));
			sprintf(cmd,"cat /sys/class/leds/sim_sw/brightness");
			result = cmd_recieve_int(cmd);
			printf("Current card slot:%d\n",result);
			if (result == 255)
				result=1;
			if (result == 0) //[优先卡槽] == [当前卡槽]
			{
				sprintf(tmp_str_buff,"%s",value_defaultcard);
				//ActiveSIM_MT5700();
				setModem2();
			}else
			{
				memset(cmd,0,sizeof(cmd));
				sprintf(cmd,"echo 0 > /sys/class/leds/sim_sw/brightness");
				cmd_recieve_str(cmd,NULL);
				sprintf(tmp_str_buff,"%s",value_defaultcard);
				//sleep(1);
				setFlightmode();
				ActiveSIM_MT5700();
				setModem2();
			}
		}
		else if(strstr(board_name,"R680"))
		{
			memset(cmd,0,sizeof(cmd));
			sprintf(cmd,"cmdat %s \"AT^SCICHG?\" | grep \"SCICHG:\" | awk -F ' ' '{print$2}' | awk -F ',' '{print$1}'",path_usb);
			result = cmd_recieve_int(cmd);
			printf("Current card slot:%d\n",result);
			
			if (result == 1) //[优先卡槽] == [当前卡槽]
			{
				sprintf(tmp_str_buff,"%s",value_defaultcard);
				//ActiveSIM_MT5700();
				setModem2();
			}else
			{
				memset(cmd,0,sizeof(cmd));
				//sprintf(cmd,"echo 1 > /sys/class/leds/sim_sw/brightness");
				sprintf(cmd,"cmdat %s \"AT^SCICHG=1,0\"",path_usb);
				cmd_recieve_str(cmd,NULL);
				sprintf(tmp_str_buff,"%s",value_defaultcard);
				//sleep(1);
				//setFlightmode();
				//ActiveSIM_MT5700();
				setModem2();
			}
		}
		else if(strstr(board_name,"R660"))
		{
			memset(cmd,0,sizeof(cmd));
			sprintf(cmd,"cat /sys/class/gpio/sim_sw/value");
			result = cmd_recieve_int(cmd);
			printf("Current card slot:%d\n",result);
			if (result == 255)
				result=1;
			if (result == 0) //[优先卡槽] == [当前卡槽]
			{
				sprintf(tmp_str_buff,"%s",value_defaultcard);
				ActiveSIM_MT5700();
				setModem2();
			}else
			{
				memset(cmd,0,sizeof(cmd));
				sprintf(cmd,"echo 0 > /sys/class/gpio/sim_sw/value");
				cmd_recieve_str(cmd,NULL);
				sprintf(tmp_str_buff,"%s",value_defaultcard);
				//sleep(1);
				setFlightmode();
				ActiveSIM_MT5700();
				setModem2();
			}
		}
	}
	else
		setModem();
	//Debug("End set MT5700","daemon.debug");
}

void setModule_MT5711(char *tmp_mode_usb,char *tmp_mode_nat)
{
	char cmd[1024];
	int tmp_int_buff;
	char tmp_str_buff[1024];
	char debug[100];
	char tmp_debug[100];
	memset(debug,0,sizeof(debug));
	memset(tmp_debug,0,sizeof(tmp_debug));
	//sleep(10);
	//report_MT5700();
	//确认模组正常工作
	while(1)
	{
		//Debug("cmdat AT","daemon.debug");
		memset(cmd,0,sizeof(cmd));
		sprintf(cmd,"cmdat %s 'AT' | grep OK |wc -l",path_usb);
		tmp_int_buff = cmd_recieve_int(cmd);
		if (tmp_int_buff == 1)
			break;
		sleep(1);
	}
	//关闭主动上报
	report_MT5700();
	
	//设定拨号上网驱动
	if( tmp_mode_usb != NULL )
	{
		if(strlen(tmp_mode_usb)>0)
		{
			memset(cmd,0,sizeof(cmd));
			sprintf(cmd,"cmdat %s \"AT^SETMODE?\" | sed -n 2p",path_usb);
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
			sprintf(cmd,"cmdat %s \"AT^SETE5STICK?\" | sed -n 2p",path_usb,tmp_mode_nat);
			tmp_int_buff = cmd_recieve_int(cmd);
			//当前上网方式与设定方式不一致时进行设定
			if (tmp_int_buff == 1)
			{
				memset(cmd,0,sizeof(cmd));
				sprintf(cmd,"cmdat %s \"AT^SETE5STICK=0\" | grep \"OK\" |wc -l",path_usb,tmp_mode_nat);
				tmp_int_buff = cmd_recieve_int(cmd);
				if(tmp_int_buff == 1)
				{
					sprintf(tmp_debug,"set nat: %s OK; ",tmp_mode_nat);
				}
				else
				{
					sprintf(tmp_debug,"set nat: %s failed; ",tmp_mode_nat);
				}
			}else{	//一致时，打印当前上网方式
				memset(cmd,0,sizeof(cmd));
				sprintf(cmd,"cmdat %s \"AT^SETE5STICK?\" | sed -n 2p",path_usb);
				memset(tmp_str_buff,0,sizeof(tmp_str_buff));
				cmd_recieve_str(cmd,tmp_str_buff);
				sprintf(tmp_debug,"nat: %s; ",tmp_str_buff);
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
			memset(cmd,0,sizeof(cmd));
			sprintf(cmd,"cat /sys/class/leds/sim_sw/brightness");
			result = cmd_recieve_int(cmd);
			printf("Current card slot:%d\n",result);
			if (result == 255)
				result=1;
			if (result == 1) //[优先卡槽] == [当前卡槽]
			{
				sprintf(tmp_str_buff,"%s",value_defaultcard);
				//ActiveSIM_MT5700();
				setModem1();
			}else
			{
				memset(cmd,0,sizeof(cmd));
				sprintf(cmd,"echo 1 > /sys/class/leds/sim_sw/brightness");
				cmd_recieve_str(cmd,NULL);
				sprintf(tmp_str_buff,"%s",value_defaultcard);
				//sleep(1);
				setFlightmode();
				ActiveSIM_MT5700();
				setModem1();
			}
		}
		else if(strstr(board_name,"R680"))
		{
			memset(cmd,0,sizeof(cmd));
			sprintf(cmd,"cmdat %s AT^SIMSWITCH? | grep SIMSWITCH: | awk -F ':' '{print$2}' | awk -F ' ' '{print$1}'",path_usb);
			result = cmd_recieve_int(cmd);
			printf("Current card slot:%d\n",result);
			
			if (result == 0) //[优先卡槽] == [当前卡槽]
			{
				sprintf(tmp_str_buff,"%s",value_defaultcard);
				//ActiveSIM_MT5700();
				setModem1();
			}else
			{
				memset(cmd,0,sizeof(cmd));
				//sprintf(cmd,"echo 1 > /sys/class/leds/sim_sw/brightness");
				sprintf(cmd,"cmdat %s \"AT^SIMSWITCH=0,1\"",path_usb);
				cmd_recieve_str(cmd,NULL);
				sprintf(tmp_str_buff,"%s",value_defaultcard);
				//sleep(1);
				setFlightmode_MT5711();
				//ActiveSIM_MT5700();
				setModem1();
			}
		}
		else if(strstr(board_name,"R660"))
		{
			memset(cmd,0,sizeof(cmd));
			sprintf(cmd,"cat /sys/class/gpio/sim_sw/value");
			result = cmd_recieve_int(cmd);
			printf("Current card slot:%d\n",result);
			if (result == 255)
				result=1;
			if (result == 1) //[优先卡槽] == [当前卡槽]
			{
				sprintf(tmp_str_buff,"%s",value_defaultcard);
				ActiveSIM_MT5700();
				setModem1();
			}else
			{
				memset(cmd,0,sizeof(cmd));
				sprintf(cmd,"echo 1 > /sys/class/gpio/sim_sw/value");
				cmd_recieve_str(cmd,NULL);
				sprintf(tmp_str_buff,"%s",value_defaultcard);
				//sleep(1);
				setFlightmode();
				ActiveSIM_MT5700();
				setModem1();
			}
		}
	}
	else if(strstr(value_defaultcard,"sim2"))
	{
		memset(cmd,0,sizeof(cmd));
		if(strstr(board_name,"R680-8TH") || strstr(board_name,"R690") || strstr(board_name,"RT990"))
		{
			memset(cmd,0,sizeof(cmd));
			sprintf(cmd,"cat /sys/class/leds/sim_sw/brightness");
			result = cmd_recieve_int(cmd);
			printf("Current card slot:%d\n",result);
			if (result == 255)
				result=1;
			if (result == 0) //[优先卡槽] == [当前卡槽]
			{
				sprintf(tmp_str_buff,"%s",value_defaultcard);
				//ActiveSIM_MT5700();
				setModem2();
			}else
			{
				memset(cmd,0,sizeof(cmd));
				sprintf(cmd,"echo 0 > /sys/class/leds/sim_sw/brightness");
				cmd_recieve_str(cmd,NULL);
				sprintf(tmp_str_buff,"%s",value_defaultcard);
				//sleep(1);
				setFlightmode();
				ActiveSIM_MT5700();
				setModem2();
			}
		}
		else if(strstr(board_name,"R680"))
		{
			memset(cmd,0,sizeof(cmd));
			sprintf(cmd,"cmdat %s AT^SIMSWITCH? | grep SIMSWITCH: | awk -F ':' '{print$2}' | awk -F ' ' '{print$1}'",path_usb);
			result = cmd_recieve_int(cmd);
			printf("Current card slot:%d\n",result);
			
			if (result == 1) //[优先卡槽] == [当前卡槽]
			{
				sprintf(tmp_str_buff,"%s",value_defaultcard);
				//ActiveSIM_MT5700();
				setModem2();
			}else
			{
				memset(cmd,0,sizeof(cmd));
				//sprintf(cmd,"echo 1 > /sys/class/leds/sim_sw/brightness");
				sprintf(cmd,"cmdat %s \"AT^SIMSWITCH=1,1\"",path_usb);
				cmd_recieve_str(cmd,NULL);
				sprintf(tmp_str_buff,"%s",value_defaultcard);
				//sleep(1);
				setFlightmode_MT5711();
				//ActiveSIM_MT5700();
				setModem2();
			}
		}
		else if(strstr(board_name,"R660"))
		{
			memset(cmd,0,sizeof(cmd));
			sprintf(cmd,"cat /sys/class/gpio/sim_sw/value");
			result = cmd_recieve_int(cmd);
			printf("Current card slot:%d\n",result);
			if (result == 255)
				result=1;
			if (result == 0) //[优先卡槽] == [当前卡槽]
			{
				sprintf(tmp_str_buff,"%s",value_defaultcard);
				ActiveSIM_MT5700();
				setModem2();
			}else
			{
				memset(cmd,0,sizeof(cmd));
				sprintf(cmd,"echo 0 > /sys/class/gpio/sim_sw/value");
				cmd_recieve_str(cmd,NULL);
				sprintf(tmp_str_buff,"%s",value_defaultcard);
				//sleep(1);
				setFlightmode();
				ActiveSIM_MT5700();
				setModem2();
			}
		}
	}
	else
		setModem();
	//Debug("End set MT5700","daemon.debug");
}

void analyticRSRP_MT5700(int rsrp,int rsrq,int sinr)
{
	char cmd[1024];
	if(rsrp <= -156 )					//<-140			无信号
		value_signal=0;
	else if(rsrp <= -125 )			//-126~-140		1格
		value_signal=1;
	else if(rsrp <= -115 )			//-116~-125		2格
		value_signal=2;
	else if(rsrp <= -105 )			//-106~-115 	3格
		value_signal=3;
	else if(rsrp <= -95 )				//-96~-105		4格
		value_signal=4;
	else if(rsrp <= -10 )				//-11~-95 		5格
		value_signal=5;
	//printf("rsrp rsrq sinr : %d %d %d\n",rsrp,rsrq,sinr);
	uci_cellular_int("SVAL",value_signal);
	uci_cellular_int("RSRP",rsrp);
	uci_cellular_int("RSRQ",rsrq);
	uci_cellular_int("SINR",sinr);
	uci_cellular_delete("RSSI");
	char debug[1024];
	if(value_signal>2 && sighi_debug == 0)
	{
		memset(debug,0,sizeof(debug));
		sprintf(debug,"rsrp: %d dBm; rsrq: %d dB; sinr %d dB; signal value: %d",rsrp,rsrq,sinr,value_signal);
		Debug(debug,"cellular.info");
		sighi_debug=1;
		siglo_debug=0;
	}
	if(value_signal<2 && siglo_debug == 0)
	{
		memset(debug,0,sizeof(debug));
		sprintf(debug,"low signal, rsrp: %d dBm; rsrq: %d dB; sinr %d dB; signal value: %d",rsrp,rsrq,sinr,value_signal);
		Debug(debug,"cellular.warn");
		sighi_debug=0;
		siglo_debug=1;
	}
}

void analyticRSRP_MT5700_HCSQ(int rsrp,float rsrq,float sinr)
{
	char cmd[1024];
	int tmp_rsrp=rsrp-139;
	if(tmp_rsrp <= -156 )					//<-140			无信号
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
	//printf("rsrp rsrq sinr : %d %d %d\n",rsrp,rsrq,sinr);
	uci_cellular_int("SVAL",value_signal);
	float tmp_rsrq=0.0;
	float tmp_sinr=0.0;
	if(sinr<255)
		tmp_sinr=(float)(sinr-1)*2/10-20;
	if(rsrq<255)
		tmp_rsrq=(float)rsrq/2-20;
	uci_cellular_int("RSRP",tmp_rsrp);
	uci_cellular_float("RSRQ",(double)tmp_rsrq);
	uci_cellular_float("SINR",(double)tmp_sinr);
	uci_cellular_delete("RSSI");
	char debug[1024];
	if(value_signal>2 && sighi_debug == 0)
	{
		memset(debug,0,sizeof(debug));
		sprintf(debug,"rsrp: %d dBm; ss_rsrq: %.1f dB; ss_sinr %.1f dB; signal value: %d",tmp_rsrp,tmp_rsrq,tmp_sinr,value_signal);
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

/* 功能：解析mt5700所处网络小区信息
** 返回值：void
*/
void analyticINFO_MT5700()
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
	memset(arfcn,0,sizeof(arfcn));
	memset(pci,0,sizeof(pci));
	memset(eNB,0,sizeof(eNB));
	memset(cellID,0,sizeof(cellID));
	sprintf(cmd,"cmdat %s \"AT^MONSC\" > %s",path_usb,path_at);
	cmd_recieve_str(cmd,NULL);
	memset(tmp_buff,0,sizeof(tmp_buff));
	sprintf(cmd,"cat %s | grep \"MONSC:\" | awk -F ' ' '{print$NF}' | xargs echo -n",path_at);
	cmd_recieve_str(cmd,tmp_buff);
	//Debug(tmp_buff,"daemon.debug");
	if(strlen(tmp_buff)>0)
	{
		int count=countstr(tmp_buff,",");
		char array_CPSI[count+1][50];
		if(separate_string2array(tmp_buff,",",count+1,50,(char *)&array_CPSI) != count+1)
		{
			memset(cmd,0,sizeof(cmd));
			char buff[500];
			memset(buff,0,sizeof(buff));
			sprintf(cmd,"cmdat %s \"AT^MONSC\" | grep \"MONSC:\" | xargs echo -n",path_usb);
			cmd_recieve_str(cmd,buff);
			sprintf(debug,"formatting the current information of cellular failed, %s\n",buff);
			printf(debug);
		}
		//解析服务类型
		if(strstr(tmp_buff,"LTE-NR"))
		{
			strcat(nservice,"NR5G_NSA");
		}else{
			if(strcmp(array_CPSI[0],"NONE")==0)
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
					uci_cellular("ARFCN","-");
					uci_cellular("PCI","-");
					uci_cellular("eNB","-");
					uci_cellular("cellID","-");
				}
				return;
			}
			else if(strcmp(array_CPSI[0],"NR")==0)
				strcat(nservice,"NR5G");
			else if(strcmp(array_CPSI[0],"WCDMA")==0)
				strcat(nservice,"WCDMA");
			else if(strcmp(array_CPSI[0],"LTE")==0)
				strcat(nservice,"LTE");
		}
		
		if(strstr(nservice,"LTE") || strstr(nservice,"NR5G"))
		{
			uci_cellular("CPSI",nservice);
			memset(band,0,sizeof(band));
			memset(cmd,0,sizeof(cmd));
			sprintf(cmd,"cmdat %s AT^HFREQINFO? | grep \"HFREQINFO:\" | awk -F ',' '{print$3}'",path_usb);
			cmd_recieve_str(cmd,band);
			uci_cellular("BAND",band);
			
			if(strlen(array_CPSI[3])>0 )
			{
				//printf("array_CPSI[3]: %s",array_CPSI[3]);
				strcat(arfcn,array_CPSI[3]);
				//printf("ARFCN: %s",arfcn);
				uci_cellular("ARFCN",arfcn);
				
				// memset(cmd,0,sizeof(cmd));
				// sprintf(cmd,"echo \"+ARFCN: %s\" >> %s",arfcn,path_outfile);
				// cmd_recieve_str(cmd,NULL);
				
			}
			else
			{
				uci_cellular("ARFCN","-");
				sprintf(arfcn,"-");
			}
			
			if(strstr(nservice,"LTE"))
			{
				if(strlen(array_CPSI[5])>0 )
				{
					memset(cmd,0,sizeof(cmd));
					memset(pci,0,sizeof(pci));
					sprintf(cmd,"echo $((0x%s))",array_CPSI[5]);
					cmd_recieve_str(cmd,pci);
					
					uci_cellular("PCI",pci);
					
					// memset(cmd,0,sizeof(cmd));
					// sprintf(cmd,"echo \"+PCI: %s\" >> %s",pci,path_outfile);
					// cmd_recieve_str(cmd,NULL);
				}
				else
				{
					uci_cellular("PCI","-");
					sprintf(pci,"-");
				}
				
				if(strlen(array_CPSI[4])>0 && strcmp(array_CPSI[4],"FFFFFFF")!=0)
				{
					memset(cmd,0,sizeof(cmd));
					strncpy(eNB, array_CPSI[4], 5);
					eNB[5] = '\0';
					sprintf(cmd,"echo $((0x%s))",eNB);

					memset(eNB,0,sizeof(eNB));
					cmd_recieve_str(cmd,eNB);
						
					memset(cmd,0,sizeof(cmd));
					strcpy(cellID, array_CPSI[4] + 5);
					cellID[strlen(array_CPSI[4])-5] = '\0';
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
			}
			else
			{
				if(strlen(array_CPSI[6])>0 )
				{
					memset(cmd,0,sizeof(cmd));
					memset(pci,0,sizeof(pci));
					sprintf(cmd,"echo $((0x%s))",array_CPSI[6]);
					cmd_recieve_str(cmd,pci);
					
					uci_cellular("PCI",pci);
					
					// memset(cmd,0,sizeof(cmd));
					// sprintf(cmd,"echo \"+PCI: %s\" >> %s",pci,path_outfile);
					// cmd_recieve_str(cmd,NULL);
				}
				else
				{
					uci_cellular("PCI","-");
					sprintf(pci,"-");
				}
				
				if(strlen(array_CPSI[5])>0 && strcmp(array_CPSI[5],"FFFFFFF")!=0)
				{
					memset(cmd,0,sizeof(cmd));
					strncpy(eNB, array_CPSI[5], 6);
					eNB[6] = '\0';
					sprintf(cmd,"echo $((0x%s))",eNB);

					memset(eNB,0,sizeof(eNB));
					cmd_recieve_str(cmd,eNB);
						
					memset(cmd,0,sizeof(cmd));
					strcpy(cellID, array_CPSI[5] + 6);
					cellID[strlen(array_CPSI[5])-6] = '\0';
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
			}
			if(strcmp(debug_nservice,nservice)!=0 || strcmp(debug_band,band)!=0 || strcmp(debug_arfcn,arfcn)!=0 || strcmp(debug_pci,pci)!=0 || strcmp(debug_enb,eNB)!=0 || strcmp(debug_cellid,cellID)!=0)
			{
				memset(cmd,0,sizeof(cmd));
				sprintf(cmd,"cmdat %s AT+CGPADDR | grep \"+CGPADDR: 1\" | wc -l",path_usb);
				int result=cmd_recieve_int(cmd);
				if(result ==0 )	//信号变化且IP地址不存在时才打印
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
			memset(cmd,0,sizeof(cmd));
			sprintf(cmd,"cmdat %s \"AT^HCSQ?\" | grep \"HCSQ:\"",path_usb);
			memset(tmp_buff,0,sizeof(tmp_buff));
			cmd_recieve_str(cmd,tmp_buff);
			if(strlen(tmp_buff)>0)
			{
				count=countstr(tmp_buff,",");
				char array_HCSQ[count+1][50];
				separate_string2array(tmp_buff,",",count+1,50,(char *)&array_HCSQ);
				if(strstr(nservice,"LTE"))
					analyticRSRP_MT5700_HCSQ(atoi(array_HCSQ[2]), atof(array_HCSQ[4]), atof(array_HCSQ[3]));
				else if(strstr(nservice,"NR5G"))
					analyticRSRP_MT5700_HCSQ(atoi(array_HCSQ[1]), atof(array_HCSQ[3]), atof(array_HCSQ[2]));
			}
			else
			{
				if(strstr(nservice,"LTE"))
					analyticRSRP(atoi(array_CPSI[7]), atof(array_CPSI[8]), atof(array_CPSI[9]));
				else if(strstr(nservice,"NR"))
					analyticRSRP_MT5700(atoi(array_CPSI[8]), atoi(array_CPSI[9]), atoi(array_CPSI[10]));
			}
		}
		else
		analyticCSQ();												//其他解析CSQ
	}
	else
	{
		uci_cellular("CPSI","NO SERVICE");
		uci_cellular("BAND","-");
		if(strstr(value_module,"fm650") || strstr(value_module,"fm160"))
		{
			uci_cellular("ARFCN","-");
			uci_cellular("PCI","-");
		}
		strcat(nservice,"NO SERVICE");
		if(!strstr(debug_nservice,nservice))
		{
			Debug("acquire the current information of cellular failed","cellular.error");
			sprintf(debug_nservice,"%s",nservice);
			analyticCSQ();
		}
	}
}

void setRATMODE_MT5700(char *mode,char *wcdma,char *lte,char *nr,char *arfcn,char *psc,int scs,char *pci)
{

	char cmd[1024];
	char tmp_buff[1024];
	//设定服务类型
	char str_wcdma[100];
	char str_lte[100];
	char str_nr[100];
	char str_mode[100];
	int set=0;
	while(1)
	{
		//Debug("check at","daemon.debug");
		memset(cmd,0,sizeof(cmd));
		sprintf(cmd,"cmdat %s 'AT^SYSCFGEX?' | grep OK |wc -l",path_usb);
		int tmp_int_buff = cmd_recieve_int(cmd);
		if (tmp_int_buff == 1)
			break;
	}
	//Debug("ENTER SET RAT","daemon.debug");
	memset(str_mode,0,sizeof(str_mode));
	strcat(str_mode,mode);
	memset(str_wcdma,0,sizeof(str_wcdma));
	memset(str_lte,0,sizeof(str_lte));
	memset(str_nr,0,sizeof(str_nr));
	if(strlen(wcdma)>0)
	{
		sprintf(str_wcdma,"%s",wcdma);
	}
	if(strlen(lte)>0)
	{
		sprintf(str_lte,"%s",lte);
	} 
	else
	{
		sprintf(str_lte,"0");
	}
	if(strlen(nr)>0)
	{
		sprintf(str_nr,"%s",nr);
	}
	else
	{
		sprintf(str_nr,"0");
	}
	char tmp_mode[100];
	memset(cmd,0,sizeof(cmd));
	sprintf(cmd,"cmdat %s \"AT^SYSCFGEX?\" | grep \"SYSCFGEX:\" | awk -F '\"' '{print$2}'",path_usb);
	memset(tmp_mode,0,sizeof(tmp_mode));
	cmd_recieve_str(cmd,tmp_mode);
	memset(cmd,0,sizeof(cmd));
	sprintf(cmd,"cmdat %s 'AT^SYSCFGEX=\"%s\",3fffffff,1,2,7fffffffffffffff,,'",path_usb,str_mode);
	//Debug(str_mode,"daemon.debug");
	//Debug(tmp_mode,"daemon.debug");
	//Debug(nr,"daemon.debug");
	//Debug(str_nr,"daemon.debug");
	if(strcmp(str_mode,tmp_mode)!=0)
	{
		//Debug("ENTER SET RAT","daemon.debug");
		cmd_recieve_str(cmd,NULL);
		//setFlightmode();
		memset(tmp_buff,0,sizeof(tmp_buff));
		sprintf(tmp_buff,"AT^SYSCFGEX=\"%s\",3fffffff,1,2,7fffffffffffffff,,",str_mode);
		//Debug(cmd,"daemon.debug");
		set=1;
	}
	if(strstr(str_mode,"08") && strlen(str_nr)>0)
	{
		//Debug("ENTER SET 5G BAND","daemon.debug");
		//Debug(arfcn,"daemon.debug");
		//Debug(psc,"daemon.debug");
		set=set5GCELLLOCK_MT5700(str_nr,arfcn,psc,scs);
	}
	
	if(strstr(str_mode,"03") && strlen(str_lte)>0)
	{
		//Debug("ENTER SET 4G BAND","daemon.debug");
		set=setCELLLOCK_MT5700(str_lte,arfcn,pci);
	}
	
	if(set==1)
		setFlightmode();
	//获取当前拨号状态，处于拨号时挂断以重新拨号
	memset(cmd,0,sizeof(cmd));
	sprintf(cmd,"cmdat %s AT+CGPADDR | grep \"+CGPADDR: 1\" | wc -l",path_usb);
	int result=cmd_recieve_int(cmd);
	if(result == 1)
	{
		memset(cmd,0,sizeof(cmd));
		sprintf(cmd,"cmdat %s 'AT^NDISDUP=1,0'",path_usb);
		memset(tmp_buff,0,sizeof(tmp_buff));
		cmd_recieve_str(cmd,tmp_buff);
		//Debug(cmd,"daemon.debug");
	}
	
}


/* 功能：设定fm650模组5G频点/小区
** 返回值：void
*/
int set5GCELLLOCK_MT5700(char *nr, char* arfcn, char* psc, int scs)
{
	char cmd[1024];
	char tmp_buff[1024];
	char cmd_celllock[1024];
	memset(cmd_celllock,0,sizeof(cmd_celllock));
	memset(cmd,0,sizeof(cmd));
	sprintf(cmd,"cmdat %s 'AT^NRFREQLOCK?' | awk '{ print $0 \",\" }' | xargs | sed 's/ //g' | awk -F ':' '{print$2}' | awk '{print substr($0, 1, length($0)-4)}'",path_usb);
	cmd_recieve_str(cmd,cmd_celllock);
	//Debug("cmd_celllock","daemon.debug");
	//Debug(cmd_celllock,"daemon.debug");
	if(strlen(psc)>0) //锁小区
	{
		//Debug(nr,"daemon.debug");
		//Debug(arfcn,"daemon.debug");
		//Debug(psc,"daemon.debug");
		memset(cmd,0,sizeof(cmd));
		memset(tmp_buff,0,sizeof(tmp_buff));
		{
			if(strcmp(nr,"0")==0)
			{
				return resetCELLLOCK_MT5700();
				if(count_loop==0)
					Debug("please specify nr5g band if you want to lock the cell","daemon.error");
			}else
			{
				char cmd_setcell[1024];
				memset(cmd_setcell,0,sizeof(cmd_setcell));
				sprintf(cmd_setcell,"2,0,1,%s,%s,%d,%s",nr,arfcn,scs,psc);
				//Debug("cmd_celllock","daemon.debug");
				//Debug(cmd_celllock,"daemon.debug");
				//Debug("cmd_setcell","daemon.debug");
				//Debug(cmd_setcell,"daemon.debug");
				if(strcmp(cmd_setcell,cmd_celllock)!=0)
				{
					memset(cmd,0,sizeof(cmd));
					memset(tmp_buff,0,sizeof(tmp_buff));
					sprintf(cmd,"cmdat %s 'AT^NRFREQLOCK=2,0,1,\"%s\",\"%s\",\"%d\",\"%s\"'",path_usb,nr,arfcn,scs,psc);
					cmd_recieve_str(cmd,tmp_buff);
					//Debug(cmd_celllock,"daemon.debug");
					//Debug(cmd,"daemon.debug");
					return 1;
				}
			}
			
		}
	}
	else
	{
		//锁频
		if(strlen(arfcn)>0)
		{
			memset(cmd,0,sizeof(cmd));
			memset(tmp_buff,0,sizeof(tmp_buff)); 
			if(strcmp(nr,"0")==0)
			{
				return resetCELLLOCK_MT5700();
				if(count_loop==0)
					Debug("please specify nr5g band if you want to lock the frequence","daemon.error");
			}
			else
			{
				char cmd_setcell[1024];
				memset(cmd_setcell,0,sizeof(cmd_setcell));
				sprintf(cmd_setcell,"1,0,1,%s,%s,%d",nr,arfcn,scs);
				if(strcmp(cmd_setcell,cmd_celllock)!=0)
				{
					sprintf(cmd,"cmdat %s 'AT^NRFREQLOCK=1,0,1,\"%s\",\"%s\",\"%d\"'",path_usb,nr,arfcn,scs);
					cmd_recieve_str(cmd,tmp_buff);
					return 1;
				}
			}
		}
		else
		{
			memset(cmd,0,sizeof(cmd));
			memset(tmp_buff,0,sizeof(tmp_buff)); 
			if(strcmp(nr,"0")==0)
			{
				return resetCELLLOCK_MT5700();
			}
			else
			{
				char cmd_setcell[1024];
				memset(cmd_setcell,0,sizeof(cmd_setcell));
				sprintf(cmd_setcell,"3,0,1,%s",nr);
				if(strcmp(cmd_setcell,cmd_celllock)!=0)
				{
					sprintf(cmd,"cmdat %s 'AT^NRFREQLOCK=3,0,1,\"%s\"'",path_usb,nr);
					cmd_recieve_str(cmd,tmp_buff);
					return 1;
				}
			}
		}
	}
	return 0;
}

/* 功能：设定fm650模组4G频点/小区
** 返回值：void
*/
int setCELLLOCK_MT5700(char* lte, char* arfcn, char* pci)
{
	char cmd[1024];
	char tmp_buff[1024];
	char cmd_celllock[1024];
	memset(cmd_celllock,0,sizeof(cmd_celllock));
	memset(cmd,0,sizeof(cmd));
	sprintf(cmd,"cmdat %s 'AT^LTEFREQLOCK?' | awk '{ print $0 \",\" }' | xargs | sed 's/ //g' | awk -F ':' '{print$2}' | awk '{print substr($0, 1, length($0)-4)}'",path_usb);
	cmd_recieve_str(cmd,cmd_celllock);
	if(strlen(pci)>0) //锁小区
	{
		memset(cmd,0,sizeof(cmd));
		memset(tmp_buff,0,sizeof(tmp_buff));
		{
			if(strcmp(lte,"0")==0)
			{
				return resetCELLLOCK_MT5700();
				if(count_loop==0)
					Debug("please specify lte band if you want to lock the cell","daemon.error");
			}else
			{
				char cmd_setcell[1024];
				memset(cmd_setcell,0,sizeof(cmd_setcell));
				sprintf(cmd_setcell,"2,0,1,%s,%s,%s",lte,arfcn,pci);
				//Debug("cmd_celllock","daemon.debug");
				//Debug(cmd_celllock,"daemon.debug");
				//Debug(cmd_setcell,"daemon.debug");
				if(strcmp(cmd_setcell,cmd_celllock)!=0)
				{
					memset(cmd,0,sizeof(cmd));
					memset(tmp_buff,0,sizeof(tmp_buff));
					sprintf(cmd,"cmdat %s 'AT^LTEFREQLOCK=2,0,1,\"%s\",\"%s\",\"%s\"'",path_usb,lte,arfcn,pci);
					cmd_recieve_str(cmd,tmp_buff);
					//Debug(cmd,"daemon.debug");
					return 1;
				}
			}
			
		}
	}
	else
	{
		//锁频
		if(strlen(arfcn)>0)
		{
			memset(cmd,0,sizeof(cmd));
			memset(tmp_buff,0,sizeof(tmp_buff)); 
			if(strcmp(lte,"0")==0)
			{
				return resetCELLLOCK_MT5700();
				if(count_loop==0)
					Debug("please specify lte band if you want to lock the frequence","daemon.error");
			}
			else
			{
				char cmd_setcell[1024];
				memset(cmd_setcell,0,sizeof(cmd_setcell));
				sprintf(cmd_setcell,"1,0,1,%s,%s",lte,arfcn);
				//Debug("cmd_celllock","daemon.debug");
				//Debug(cmd_celllock,"daemon.debug");
				//Debug(cmd_setcell,"daemon.debug");
				if(strcmp(cmd_setcell,cmd_celllock)!=0)
				{
					sprintf(cmd,"cmdat %s 'AT^LTEFREQLOCK=1,0,1,\"%s\",\"%s\"'",path_usb,lte,arfcn);
					cmd_recieve_str(cmd,tmp_buff);
					//Debug(cmd,"daemon.debug");
					return 1;
				}
			}
		}
		else
		{
			memset(cmd,0,sizeof(cmd));
			memset(tmp_buff,0,sizeof(tmp_buff)); 
			if(strcmp(lte,"0")==0)
			{
				
				return resetCELLLOCK_MT5700();
			}
			else
			{
				char cmd_setcell[1024];
				memset(cmd_setcell,0,sizeof(cmd_setcell));
				sprintf(cmd_setcell,"3,0,1,%s",lte);
				//Debug("cmd_celllock","daemon.debug");
				//Debug(cmd_celllock,"daemon.debug");
				//Debug(cmd_setcell,"daemon.debug");
				if(strcmp(cmd_setcell,cmd_celllock)!=0)
				{
					sprintf(cmd,"cmdat %s 'AT^LTEFREQLOCK=3,0,1,\"%s\"'",path_usb,lte);
					cmd_recieve_str(cmd,tmp_buff);
					//Debug(cmd,"daemon.debug");
					return 1;
				}
			}
		}
	}
	return 0;
}

/* 功能：恢复fm650模组频点/小区
** 返回值：void
*/
int resetCELLLOCK_MT5700()
{
	char cmd[1024];
	memset(cmd,0,sizeof(cmd));
	sprintf(cmd,"cmdat %s 'AT^LTEFREQLOCK?' | grep \"LTEFREQLOCK: 0\" | wc -l",path_usb);
	int lte_lock=cmd_recieve_int(cmd);
	int set=0;
	if(lte_lock!=1)
	{
		sprintf(cmd,"cmdat %s 'AT^LTEFREQLOCK=0'",path_usb);
		system(cmd);
		set=1;
	}
	memset(cmd,0,sizeof(cmd));
	sprintf(cmd,"cmdat %s 'AT^NRFREQLOCK?' | grep \"NRFREQLOCK: 0\" | wc -l",path_usb);
	int nr_lock=cmd_recieve_int(cmd);
	if(nr_lock!=1)
	{
		sprintf(cmd,"cmdat %s 'AT^NRFREQLOCK=0'",path_usb);
		system(cmd);
		set=1;
	}
	return set;
}
/* 功能：对fm650模组进行拨号
** 返回值：void
*/
void Dial_MT5700()
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
	if( strstr(tmp_C5GREG,"0,1") || strstr(tmp_C5GREG,"0,5") || strstr(tmp_C5GREG,"0,4") 
		|| strstr(tmp_C5GREG,"2,1") || strstr(tmp_C5GREG,"2,5") || strstr(tmp_C5GREG,"2,4") 
		|| strstr(tmp_CGREG,"0,1") || strstr(tmp_CGREG,"0,5") || strstr(tmp_CGREG,"0,4")
		|| strstr(tmp_CGREG,"2,1") || strstr(tmp_CGREG,"2,5") || strstr(tmp_CGREG,"2,4")
		|| strstr(tmp_CEREG,"0,1") || strstr(tmp_CEREG,"0,5") || strstr(tmp_CEREG,"0,4")
		|| strstr(tmp_CEREG,"2,1") || strstr(tmp_CEREG,"2,5") || strstr(tmp_CEREG,"2,4"))
	{
		
		if(flag_ims!=1)
		{
			
			memset(cmd,0,sizeof(cmd));
			sprintf(cmd,"cmdat %s 'AT^IMSSWITCH?' | grep \"NRIMSSWITCH: 0,0,0\" | wc -l",path_usb);
			memset(buff,0,sizeof(buff));
			cmd_recieve_str(cmd,buff);
			if(!strstr(buff,"1"))
			{
				memset(cmd,0,sizeof(cmd));
				sprintf(cmd,"cmdat %s 'AT^IMSSWITCH=0,0,0'",path_usb);
				memset(buff,0,sizeof(buff));
				cmd_recieve_str(cmd,buff);
			}
			
			memset(cmd,0,sizeof(cmd));
			sprintf(cmd,"cmdat %s 'AT^NRIMSSWITCH?' | grep \"NRIMSSWITCH: 0\" | wc -l",path_usb);
			memset(buff,0,sizeof(buff));
			cmd_recieve_str(cmd,buff);
			if(!strstr(buff,"1"))
			{
				memset(cmd,0,sizeof(cmd));
				sprintf(cmd,"cmdat %s 'AT^NRIMSSWITCH=0'",path_usb);
				memset(buff,0,sizeof(buff));
				cmd_recieve_str(cmd,buff);
			}
		}
		else
		{
			memset(cmd,0,sizeof(cmd));
			sprintf(cmd,"cmdat %s 'AT^IMSSWITCH?' | grep \"NRIMSSWITCH: 1,1,1\" | wc -l",path_usb);
			memset(buff,0,sizeof(buff));
			cmd_recieve_str(cmd,buff);
			if(!strstr(buff,"1"))
			{
				memset(cmd,0,sizeof(cmd));
				sprintf(cmd,"cmdat %s 'AT^IMSSWITCH=1,1,1'",path_usb);
				memset(buff,0,sizeof(buff));
				cmd_recieve_str(cmd,buff);
			}
			
			memset(cmd,0,sizeof(cmd));
			sprintf(cmd,"cmdat %s 'AT^NRIMSSWITCH?' | grep \"NRIMSSWITCH: 1\" | wc -l",path_usb);
			memset(buff,0,sizeof(buff));
			cmd_recieve_str(cmd,buff);
			if(!strstr(buff,"1"))
			{
				memset(cmd,0,sizeof(cmd));
				sprintf(cmd,"cmdat %s 'AT^NRIMSSWITCH=1'",path_usb);
				memset(buff,0,sizeof(buff));
				cmd_recieve_str(cmd,buff);
			}
		}
		memset(cmd,0,sizeof(cmd));
		sprintf(cmd,"cmdat %s \"AT^NDISDUP=1,1\" > /dev/null 2>&1",path_usb);
		system(cmd);
		int checkip_count=0;
		//sleep(3);
		while(1)
		{
			sleep(1);
			//判断是否处于已拨号状态
			memset(cmd,0,sizeof(cmd));
			sprintf(cmd,"cmdat %s AT+CGPADDR | grep \"+CGPADDR: 1\" | wc -l",path_usb);
			result=cmd_recieve_int(cmd);
			if(result ==1 )	//是，重启网卡
			{
				system(cmd_ifup);
				if(strcmp(dialresult_debug,"yes")!=0){
					char rndis[200];
					memset(rndis,0,sizeof(rndis));
					memset(cmd,0,sizeof(cmd));
					sprintf(cmd,"cmdat %s AT+CGPADDR | grep \"+CGPADDR: 1\" | awk -F ' ' '{print$NF}' | awk '{print substr($0, 3)}'",path_usb);
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
			//Debug("sleep 23s check ip again","daemon.debug");
			memset(cmd,0,sizeof(cmd));
			sprintf(cmd,"cmdat %s AT+CGPADDR | grep \"+CGPADDR: 1\" | wc -l",path_usb);
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
				//Debug("sleep 23s check ip ok","daemon.debug");
				cmd_recieve_str(cmd_ifup,NULL);
				if(strcmp(dialresult_debug,"yes")!=0)
				{
					char rndis[200];
					memset(rndis,0,sizeof(rndis));
					memset(cmd,0,sizeof(cmd));
					sprintf(cmd,"cmdat %s AT+CGPADDR | grep \"+CGPADDR: 1\" | awk -F ' ' '{print$NF}' | awk '{print substr($0, 3)}'",path_usb);
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
			trans_register_status_MT5700(debug_cgreg,stat_cgreg);
			trans_register_status_MT5700(debug_c5greg,stat_c5greg);
			trans_register_status_MT5700(debug_cereg,stat_cereg);
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


//支持MT5700 FM160 双卡切换的：r660 rt990 r680 r990 r650
void SwitchSIM_MT5700(bool isdefault)
{
	char cmd[1024];
	char tmp_buff[1024];
	int value_nextcard=value_defaultcard;
	if(isdefault) //设置默认卡槽
	{
		memset(cmd,0,sizeof(cmd));
		if(strstr(board_name,"RT990") || strstr(board_name,"R690") || strstr(board_name,"R680-8TH"))
			sprintf(cmd,"echo 1 > /sys/class/leds/sim_sw/brightness");
		else if(strstr(board_name,"R660"))
			sprintf(cmd,"echo 1 > /sys/class/gpio/sim_sw/value");
		else if(strstr(board_name,"R650") || strstr(board_name,"R680"))
			sprintf(cmd,"cmdat %s \"AT^SCICHG=0,1\"",path_usb);
		
		cmd_recieve_str(cmd,NULL);
		//sleep(1);
		//if(!strstr(board_name,"R650")) //指令切换卡同样要飞行模式
		if(strcmp(board_name,"R680")!=0)
		{
			setFlightmode();
			ActiveSIM_MT5700();
		}
		setModem1();
	}
	else 		//获取当前卡槽并切换卡槽
	{
		memset(cmd,0,sizeof(cmd));
		/* if(strstr(board_name,"R650"))
			sprintf(cmd,"cmdat %s 'AT+GTDUALSIM?' | grep \"+GTDUALSIM:\" | awk -F ',' '{print $1}' | awk -F ' ' '{print $2}'",path_usb);
		else  */if(strstr(board_name,"R660"))
			sprintf(cmd,"cat /sys/class/gpio/sim_sw/value");
		else if(strcmp(board_name,"R680")==0)
			sprintf(cmd,"cmdat %s \"AT^SCICHG?\" | grep \"SCICHG:\" | awk -F ' ' '{print$2}' | awk -F ',' '{print$1}'",path_usb);
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
		/* if(strstr(board_name,"R650"))
			sprintf(cmd,"cmdat %s 'AT+GTDUALSIM=%d'",path_usb,value_nextcard);
		else  */if(strstr(board_name,"R660"))
			sprintf(cmd,"echo %d > /sys/class/gpio/sim_sw/value",value_nextcard);
		else if(strcmp(board_name,"R680")==0)
			sprintf(cmd,"cmdat %s \"AT^SCICHG=%d,%d\"",path_usb,value_nextcard,value_currentcard);
		else
			sprintf(cmd,"echo %d > /sys/class/leds/sim_sw/brightness",value_nextcard);
		cmd_recieve_str(cmd,NULL);
		memset(tmp_buff,0,sizeof(tmp_buff));
		//sleep(1);
		//if(!strstr(board_name,"R650"))
		if(strcmp(board_name,"R680")!=0)
		{
			setFlightmode();
			ActiveSIM_MT5700();
		}
		sleep(10);
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
			if(value_nextcard==0)
				setModem2();
			else if(value_nextcard==1)
				setModem1();
		}
	}
}

void SwitchSIM_MT5711(bool isdefault)
{
	char cmd[1024];
	char tmp_buff[1024];
	int value_nextcard=value_defaultcard;
	if(isdefault) //设置默认卡槽
	{
		memset(cmd,0,sizeof(cmd));
		if(strstr(board_name,"RT990") || strstr(board_name,"R690") || strstr(board_name,"R680-8TH"))
			sprintf(cmd,"echo 1 > /sys/class/leds/sim_sw/brightness");
		else if(strstr(board_name,"R660"))
			sprintf(cmd,"echo 1 > /sys/class/gpio/sim_sw/value");
		else if(strstr(board_name,"R650") || strstr(board_name,"R680"))
			sprintf(cmd,"cmdat %s \"AT^SIMSWITCH=0,1\"",path_usb);
		
		cmd_recieve_str(cmd,NULL);
		//sleep(1);
		//if(!strstr(board_name,"R650")) //指令切换卡同样要飞行模式
		if(strcmp(board_name,"R680")!=0)
		{
			setFlightmode_MT5711();
			//ActiveSIM_MT5700();
		}
		setModem1();
	}
	else 		//获取当前卡槽并切换卡槽
	{
		memset(cmd,0,sizeof(cmd));
		/* if(strstr(board_name,"R650"))
			sprintf(cmd,"cmdat %s 'AT+GTDUALSIM?' | grep \"+GTDUALSIM:\" | awk -F ',' '{print $1}' | awk -F ' ' '{print $2}'",path_usb);
		else  */if(strstr(board_name,"R660"))
			sprintf(cmd,"cat /sys/class/gpio/sim_sw/value");
		else if(strcmp(board_name,"R680")==0)
			sprintf(cmd,"cmdat %s AT^SIMSWITCH? | grep SIMSWITCH: | awk -F ':' '{print$2}' | awk -F ' ' '{print$1}'",path_usb);
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
		/* if(strstr(board_name,"R650"))
			sprintf(cmd,"cmdat %s 'AT+GTDUALSIM=%d'",path_usb,value_nextcard);
		else  */if(strstr(board_name,"R660"))
			sprintf(cmd,"echo %d > /sys/class/gpio/sim_sw/value",value_nextcard);
		else if(strcmp(board_name,"R680")==0)
			sprintf(cmd,"cmdat %s \"AT^SIMSWITCH=%d,1\"",path_usb,value_nextcard);
		else
			sprintf(cmd,"echo %d > /sys/class/leds/sim_sw/brightness",value_nextcard);
		cmd_recieve_str(cmd,NULL);
		memset(tmp_buff,0,sizeof(tmp_buff));
		//sleep(1);
		//if(!strstr(board_name,"R650"))
		if(strcmp(board_name,"R680")!=0)
		{
			setFlightmode_MT5711();
			//ActiveSIM_MT5700();
		}
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
			if(value_nextcard==0)
				setModem2();
			else if(value_nextcard==1)
				setModem1();
		}
	}
}

void setFlightmode_MT5711()
{
	char cmd[1024];
	//切换到飞行模式
	memset(cmd,0,sizeof(cmd));
	sprintf(cmd,"cmdat %s \"AT+CFUN=8\" > /dev/null 2>&1",path_usb);
	cmd_recieve_str(cmd,NULL);
	sleep(20);
}

void ActiveSIM_MT5700()
{
	char cmd[1024];
	memset(cmd,0,sizeof(cmd));
	sprintf(cmd,"cmdat %s AT^HVSST=1,1",path_usb);
	system(cmd);
	sleep(1);
}

void trans_register_status_MT5700(char *reg,char *status)
{
	if(strcmp(reg,"0,0")==0 || strcmp(reg,"2,0")==0)
	{
		sprintf(status,"not currently searching an operator to register to");
	}
	else if(strcmp(reg,"0,1")==0 || strcmp(reg,"2,1")==0)
	{
		sprintf(status,"registered home network");
	}
	else if(strcmp(reg,"0,2")==0 || strcmp(reg,"2,2")==0)
	{
		sprintf(status,"currently trying to attach or searching an operator to register to");
	}
	else if(strcmp(reg,"0,3")==0 || strcmp(reg,"2,3")==0)
	{
		sprintf(status,"registration denied");
	}
	else if(strcmp(reg,"0,4")==0 || strcmp(reg,"2,4")==0)
	{
		sprintf(status,"unknown");
	}
	else if(strcmp(reg,"0,5")==0 || strcmp(reg,"2,5")==0)
	{
		sprintf(status,"registered roaming");
	}
	else if(strcmp(reg,"0,6")==0 || strcmp(reg,"2,6")==0)
	{
		sprintf(status,"registered for \"SMS only\", home network (not applicable)");
	}
	else if(strcmp(reg,"0,7")==0 || strcmp(reg,"2,7")==0)
	{
		sprintf(status,"registered for \"SMS only\", roaming (not applicable)");
	}
	else if(strcmp(reg,"0,8")==0 || strcmp(reg,"2,8")==0)
	{
		sprintf(status,"attached for emergency bearer services only");
	}
	else if(strcmp(reg,"0,9")==0 || strcmp(reg,"2,9")==0)
	{
		sprintf(status,"registered for \"CSFB not preferred\", home network (not applicable)");
	}
	else if(strcmp(reg,"0,10")==0 || strcmp(reg,"2,10")==0)
	{
		sprintf(status,"registered for \"CSFB not preferred\", roaming (not applicable)");
	}
}