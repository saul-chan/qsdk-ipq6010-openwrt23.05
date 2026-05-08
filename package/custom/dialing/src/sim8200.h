#include "dialing.h"
/* 功能：设定sim8200模组拨号方式
** 返回值：void
*/
void setModule_SIM8200(char *tmp_mode_usb,char *tmp_mode_nat)
{
	//终止simcom-cm程序
	system("killall simcom-cm");
	char cmd[1024];
	int tmp_int_buff;
	char tmp_str_buff[1024];
	//确认模组正常工作
	while(1)
	{
		memset(cmd,0,sizeof(cmd));
		sprintf(cmd,"cmdat %s 'AT' | grep OK |wc -l",path_usb);
		tmp_int_buff = cmd_recieve_int(cmd);
		if (tmp_int_buff == 1)
			break;
	}
	
	/* //设定当前读卡位置
	memset(cmd,0,sizeof(cmd));
	sprintf(cmd,"cmdat %s 'AT+SMSIMCFG=1,%d'",path_usb,(value_defaultcard+1));
	cmd_recieve_str(cmd,NULL);
	memset(tmp_str_buff,0,sizeof(tmp_str_buff));
	sprintf(tmp_str_buff,"%d",(value_defaultcard+1));
	Debug(tmp_str_buff,"Set AT+SMSIMCFG");
	SMSIMCFG=value_defaultcard;
	sleep(2); */
	
	//设定当前读卡位置
	/* 
	memset(cmd,0,sizeof(cmd));
	sprintf(cmd,"cat /sys/class/leds/pwr_sim2/brightness");
	int result = cmd_recieve_int(cmd);
	//printf("Current card slot:%d\n",result);
	if (result == 255)
	{
		result=1;
	}
	if (result == value_defaultcard) //[优先卡槽] == [当前卡槽]
	{
		sprintf(tmp_str_buff,"%d",value_defaultcard);
		Debug(tmp_str_buff,"Current card slot");
	}else
	{
		memset(cmd,0,sizeof(cmd));
		sprintf(cmd,"echo %d > /sys/class/leds/pwr_sim2/brightness",value_defaultcard);
		cmd_recieve_str(cmd,NULL);
		sprintf(tmp_str_buff,"%d",value_defaultcard);
		Debug(tmp_str_buff,"Set card slot");
		sleep(3);
	} */
	
}

/* 功能：解析sim8200所处网络小区信息
** 返回值：void
*/
void analyticCPSI_SIM8200()
{
	char tmp_buff[2048];
	char cmd[1024];
	char nservice[100];
	char band[100];
	memset(cmd,0,sizeof(cmd));
	memset(tmp_buff,0,sizeof(tmp_buff));
	memset(nservice,0,sizeof(nservice));
	memset(band,0,sizeof(band));
	sprintf(cmd,"cmdat %s \"AT+CPSI?\" 500 > %s",path_usb,path_at);
	cmd_recieve_str(cmd,NULL);
	memset(tmp_buff,0,sizeof(tmp_buff));
	sprintf(cmd,"cat %s | grep \"+CPSI:\" | awk -F ': ' '{print $2}'",path_at);
	cmd_recieve_str(cmd,tmp_buff);
	if(strstr(tmp_buff,"NO SERVICE") || strstr(tmp_buff,"32768"))
	{
		Debug("NO SERVICE",NULL);
		return;
	}
	else if(strstr(tmp_buff,"Online"))
	{
		int count=countstr(tmp_buff,",");
		char array_CPSI[count][50];
		if(separate_string2array(tmp_buff,",",count,50,(char *)&array_CPSI) != count)
		{
			Debug("Format CPSI error!",NULL);
		}
		//解析服务类型
		if(strstr(tmp_buff,"NR"))
		{
			strcat(nservice,"NR5G");
			//解析上网频段
			strcat(band,array_CPSI[6]);
			int tmp_rsrp=atoi(array_CPSI[8])/10;
			float tmp_rsrq=atof(array_CPSI[9])/10;
			//int tmp_rssi=atoi(array_CPSI[8])/10;
			analyticRSRP(tmp_rsrp,tmp_rsrq,999);
		}
		else if(strstr(tmp_buff,"LTE"))
		{
			memset(cmd,0,sizeof(cmd));
			sprintf(cmd,"cat %s | grep \"NR5G_NSA\" | awk -F ': ' '{print $2}'",path_at);
			memset(tmp_buff,0,sizeof(tmp_buff));
			cmd_recieve_str(cmd,tmp_buff);
			if(strstr(tmp_buff,"NR5G_NSA"))
				strcat(nservice,"NR5G_NSA");
			else
				strcat(nservice,"LTE");
			//解析上网频段
			strcat(band,array_CPSI[6]);
			int tmp_rsrp=atoi(array_CPSI[11])/10;
			float tmp_rsrq=atof(array_CPSI[10])/10;
			int tmp_rssi=atof(array_CPSI[12])/10;
			analyticRSRP(tmp_rsrp,tmp_rsrq,tmp_rssi);
		}else if(strstr(tmp_buff,"CDMA"))
		{
			strcat(nservice,"WCDMA");
			//解析上网频段
			strcat(band,array_CPSI[5]);
			analyticCSQ();
		}else if(strstr(tmp_buff,"GSM"))
		{
			strcat(nservice,"GSM");
			//解析上网频段
			strcat(band,array_CPSI[5]);
			analyticCSQ();
		}else{
			strcat(nservice,array_CPSI[0]);
			//解析上网频段
			strcat(band,array_CPSI[6]);
			analyticCSQ();
		}	
		if(strcmp(debug_nservice,nservice)!=0 || strcmp(debug_band,band)!=0)
		{
			memset(cmd,0,sizeof(cmd));
			sprintf(cmd,"%s BAND=%s",nservice,band);
			Debug(cmd,NULL);
			memset(debug_nservice,0,sizeof(debug_nservice));
			memset(debug_band,0,sizeof(debug_band));
			strcpy(debug_nservice,nservice);
			strcpy(debug_band,band);
		}
		
		memset(cmd,0,sizeof(cmd));
		sprintf(cmd,"echo \"+CPSI: %s\" >> %s",nservice,path_outfile);
		cmd_recieve_str(cmd,NULL);
		
		memset(cmd,0,sizeof(cmd));
		sprintf(cmd,"echo \"+BAND: %s\" >> %s",band,path_outfile);
		cmd_recieve_str(cmd,NULL);
	}
}
/* 功能：设定sim8200模组上网服务类型
** 返回值：void
*/
void setRATMODE_SIM8200(int mode,char *wcdma,char *lte,char *nr,char *nsa)
{

	char cmd[1024];
	char tmp_buff[1024];
	//设定服务类型
_SETRAT_SIM8200_AGAIN:	
	memset(cmd,0,sizeof(cmd));
	sprintf(cmd,"cmdat %s \"AT+CNMP=%d\"",path_usb,mode);
	memset(tmp_buff,0,sizeof(tmp_buff));
	cmd_recieve_str(cmd,tmp_buff);
	memset(cmd,0,sizeof(cmd));
	sprintf(cmd,"%d",mode);
	if(strstr(tmp_buff,"OK"))				//获取设定结果，OK
		Debug(cmd,"Set AT+CNMP");
	else if(strstr(tmp_buff,"ERROR"))		//获取设定结果，ERROR
		Debug(cmd,"Failed-1 to set AT+CNMP");
	else{									//获取设定结果失败，通过goto再次设定
		Debug(cmd,"Failed-2 to set AT+CNMP");
		goto _SETRAT_SIM8200_AGAIN;
	}
	//锁定频段
	char str_wcdma[100];
	char str_lte[100];
	char str_nr[100];
	char str_nsa[100];
	memset(str_wcdma,0,sizeof(str_wcdma));
	memset(str_lte,0,sizeof(str_lte));
	memset(str_nr,0,sizeof(str_nr));
	if(mode==2)
	{
		memset(cmd,0,sizeof(cmd));
		sprintf(cmd,"cmdat %s \"AT+CSYSSEL\"",path_usb);
		memset(tmp_buff,0,sizeof(tmp_buff));
		cmd_recieve_str(cmd,tmp_buff);
		if(strstr(tmp_buff,"OK"))				//获取设定结果，OK
			Debug("Reset AT+CSYSSEL",NULL);
		else if(strstr(tmp_buff,"ERROR"))		//获取设定结果，ERROR
			Debug("Failed-1 to reset AT+CSYSSEL",NULL);
	}
	if(strlen(wcdma)>0)
	{
		if(strcmp(wcdma,"0")==0)
			strcpy(str_wcdma,"1:3:5:6:8:9:19");
		else
			sprintf(str_wcdma,"%s",wcdma);
		memset(cmd,0,sizeof(cmd));
		sprintf(cmd,"cmdat %s 'AT+CSYSSEL=\"w_band\",%s'",path_usb,str_wcdma);
		memset(tmp_buff,0,sizeof(tmp_buff));
		cmd_recieve_str(cmd,tmp_buff);
		if(strstr(tmp_buff,"OK"))				//获取设定结果，OK
			Debug(str_wcdma,"Set AT+CSYSSEL=\"w_band\"");
		else if(strstr(tmp_buff,"ERROR"))		//获取设定结果，ERROR
			Debug(str_wcdma,"Failed-1 to set AT+CSYSSEL=\"w_band\"");
	}
	if(strlen(lte)>0)
	{
		if(strcmp(lte,"0")==0)
			strcpy(str_lte,"1:3:5:7:8:18:19:26:28:34:38:39:40:41:42:43:48");
		else
			sprintf(str_lte,"%s",lte);
		memset(cmd,0,sizeof(cmd));
		sprintf(cmd,"cmdat %s 'AT+CSYSSEL=\"lte_band\",%s'",path_usb,str_lte);
		memset(tmp_buff,0,sizeof(tmp_buff));
		cmd_recieve_str(cmd,tmp_buff);
		if(strstr(tmp_buff,"OK"))				//获取设定结果，OK
			Debug(str_lte,"Set AT+CSYSSEL=\"lte_band\"");
		else if(strstr(tmp_buff,"ERROR"))		//获取设定结果，ERROR
			Debug(str_lte,"Failed-1 to set AT+CSYSSEL=\"lte_band\"");
	}
	if(strlen(nr)>0)
	{	
		if(strcmp(nr,"0")==0)
			strcpy(str_nr,"1:3:5:7:8:28:38:40:41:48:77:78:79");
		else
			sprintf(str_nr,"%s",nr);
		memset(cmd,0,sizeof(cmd));
		sprintf(cmd,"cmdat %s 'AT+CSYSSEL=\"nr5g_band\",%s'",path_usb,str_nr);
		memset(tmp_buff,0,sizeof(tmp_buff));
		cmd_recieve_str(cmd,tmp_buff);
		if(strstr(tmp_buff,"OK"))				//获取设定结果，OK
			Debug(str_nr,"Set AT+CSYSSEL=\"nr5g_band\"");
		else if(strstr(tmp_buff,"ERROR"))		//获取设定结果，ERROR
			Debug(str_nr,"Failed-1 to set AT+CSYSSEL=\"nr5g_band\"");
	}
	if(strlen(nsa)>0)
	{	
		if(strcmp(nsa,"0")==0)
			strcpy(str_nsa,"1:3:5:7:8:28:38:40:41:48:77:78:79");
		else
			sprintf(str_nsa,"%s",nsa);
		memset(cmd,0,sizeof(cmd));
		sprintf(cmd,"cmdat %s 'AT+CSYSSEL=\"nsa_nr5g_band\",%s'",path_usb,str_nsa);
		memset(tmp_buff,0,sizeof(tmp_buff));
		cmd_recieve_str(cmd,tmp_buff);
		if(strstr(tmp_buff,"OK"))				//获取设定结果，OK
			Debug(str_nsa,"Set AT+CSYSSEL=\"nsa_nr5g_band\"");
		else if(strstr(tmp_buff,"ERROR"))		//获取设定结果，ERROR
			Debug(str_nsa,"Failed-1 to set AT+CSYSSEL=\"nsa_nr5g_band\"");
	}
}
/* 功能：对sim8200模组进行拨号
** 返回值：void
*/
void Dial_SIM8200()
{
	int count=0;
	char cmd[1024];
	char tmp_buff[1024];
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
	char tmp_type_ip[20];
	memset(tmp_type_ip,0,sizeof(tmp_type_ip));
	sprintf(tmp_type_ip,"");
	if (strstr(type_ip,"IPV4"))
		sprintf(tmp_type_ip,"");
	if(strstr(type_ip,"IPV6"))
		sprintf(tmp_type_ip,"-6");
	if(strstr(type_ip,"IPV4V6"))
		sprintf(tmp_type_ip,"-4 -6");
	//Debug(type_ip,NULL);
	//Debug(tmp_type_ip,NULL);
	if(value_apn != NULL && strlen(value_apn) > 0)
	{
		if(value_username != NULL && strlen(value_username) > 0)
		{
			memset(cmd,0,sizeof(cmd));
			sprintf(cmd,"simcom-cm -s %s %s %s %s %s &",value_apn,value_username,value_password,value_auth,tmp_type_ip);
			system(cmd);
			Debug(cmd,"Dial SIM8200 OK");
		}else{
			memset(cmd,0,sizeof(cmd));
			sprintf(cmd,"simcom-cm -s %s %s &",value_apn,tmp_type_ip);
			system(cmd);
			Debug(cmd,"Dial SIM8200 OK");
		}
	}else{
		memset(cmd,0,sizeof(cmd));
		sprintf(cmd,"simcom-cm %s &",tmp_type_ip);
		Debug(cmd,NULL);
		system(cmd);
		Debug("Dial SIM8200 OK",NULL);
	}
	sleep(5);
}

/* 功能：双卡模式切换SIM卡
** 返回值：void
*/
void SwitchSIM_SIM8200(bool isdefault)
{
	char cmd[1024];
	/* char tmp_buff[1024];
	int value_nextcard=value_defaultcard;
	int result1,result2;
	if(isdefault) //设置默认卡槽
	{
		memset(cmd,0,sizeof(cmd));
		sprintf(cmd,"cmdat %s 'AT+SMSIMCFG=1,%d'",path_usb,(value_defaultcard+1));
		cmd_recieve_str(cmd,NULL);
		memset(tmp_buff,0,sizeof(tmp_buff));
		sprintf(tmp_buff,"%d",(value_defaultcard+1));
		Debug(tmp_buff,"Set AT+SMSIMCFG");
		SMSIMCFG=value_defaultcard;
		sleep(2);
	}
	else		//获取未激活卡槽并激活
	{
		if(SMSIMCFG==0)
			value_nextcard = 1;
		else if(SMSIMCFG==1)
			value_nextcard = 0;
		//激活卡槽
		memset(cmd,0,sizeof(cmd));
		sprintf(cmd,"cmdat %s 'AT+SMSIMCFG=1,%d'",path_usb,(value_nextcard+1));
		cmd_recieve_str(cmd,NULL);
		memset(tmp_buff,0,sizeof(tmp_buff));
		sprintf(tmp_buff,"%d",(value_nextcard+1));
		Debug(tmp_buff,"Set AT+SMSIMCFG");
		SMSIMCFG=value_nextcard;
		sleep(2);
	} */
	/* char cmd[1024];
	char tmp_buff[1024];
	int value_nextcard=value_defaultcard;
	if(isdefault) //设置默认卡槽
	{
		memset(cmd,0,sizeof(cmd));
		sprintf(cmd,"echo %d > /sys/class/leds/pwr_sim2/brightness",value_defaultcard);
		cmd_recieve_str(cmd,NULL);
		sprintf(tmp_buff,"%d",value_defaultcard);
		Debug(tmp_buff,"Set card slot");
		sleep(3);
	}
	else 		//获取当前卡槽并切换卡槽
	{ */
		/* memset(cmd,0,sizeof(cmd));
		sprintf(cmd,"cat /sys/class/leds/pwr_sim2/brightness"); //获取当前卡槽
		int value_currentcard = cmd_recieve_int(cmd); 
		if (value_currentcard == 255)
			value_currentcard = 1;
		if(value_currentcard == 0) 			//[当前卡槽] 0
			value_nextcard = 1;
		else if(value_currentcard == 1)		//[当前卡槽] 1
			value_nextcard = 0;
		memset(tmp_buff,0,sizeof(tmp_buff));
		sprintf(tmp_buff,"%d",value_currentcard);
		Debug(tmp_buff,"Current card slot");
		//切换卡槽
		memset(cmd,0,sizeof(cmd));
		sprintf(cmd,"echo %d > /sys/class/leds/pwr_sim2/brightness",value_nextcard);
		cmd_recieve_str(cmd,NULL);
		memset(tmp_buff,0,sizeof(tmp_buff));
		sprintf(tmp_buff,"%d",value_nextcard);
		Debug(tmp_buff,"Set card slot");
		sleep(3); */
	//}
}