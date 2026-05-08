#include "dialing.h"

/* 功能：控制灯（RSSI模式）,控制rssi1~rssi4
** 返回值：void
*/
void controlLED_RSSI()
{
	char cmd[1024];
	char tmp_buff[1024];
	memset(tmp_buff,0,sizeof(tmp_buff));
	memset(cmd,0,sizeof(cmd));
	sprintf(cmd,"uci -q -P /var/state get cellular.%s.SVAL | awk -F ' ' '{print$2}'",name_config);
	int signal=cmd_recieve_int(cmd);
	int i;
	int signal_array[4]={1,2,4,5};
	for(i=1;i<=4;i++){//使用从rssi1~rssi4的灯
		if(signal>=signal_array[i-1]){
			memset(cmd,0,sizeof(cmd));
			sprintf(cmd,"echo 1 > $(ls /sys/class/leds/*rssi%d/brightness)",i);
			cmd_recieve_str(cmd,NULL);
		}else{
			memset(cmd,0,sizeof(cmd));
			sprintf(cmd,"echo 0 > $(ls /sys/class/leds/*rssi%d/brightness)",i);
			cmd_recieve_str(cmd,NULL);
		}
	}
}
/* 功能：控制灯（RSSI模式）,控制rssi1~rssi8
** 返回值：void
*/
void controlLED_RSSI2()
{
	char cmd[1024];
	char tmp_buff[1024];
	memset(tmp_buff,0,sizeof(tmp_buff));
	memset(cmd,0,sizeof(cmd));
	sprintf(cmd,"uci -q -P /var/state get cellular.%s.SVAL | awk -F ' ' '{print$2}'",name_config);
	int signal=cmd_recieve_int(cmd);
	int i;
	int signal_array[4]={1,2,4,5};
	if(strstr(name_config,"SIM1"))
	{
		for(i=1;i<=4;i++){//使用从rssi1~rssi4的灯
			if(signal>=signal_array[i-1]){
				memset(cmd,0,sizeof(cmd));
				sprintf(cmd,"echo 1 > $(ls /sys/class/leds/*rssi%d/brightness)",i);
				cmd_recieve_str(cmd,NULL);
			}else{
				memset(cmd,0,sizeof(cmd));
				sprintf(cmd,"echo 0 > $(ls /sys/class/leds/*rssi%d/brightness)",i);
				cmd_recieve_str(cmd,NULL);
			}
		}
	}
	else if(strstr(name_config,"SIM2"))
	{
		for(i=1;i<=4;i++){//使用从rssi1~rssi4的灯
			if(signal>=signal_array[i-1]){
				memset(cmd,0,sizeof(cmd));
				sprintf(cmd,"echo 1 > $(ls /sys/class/leds/*rssi%d/brightness)",i+4);
				cmd_recieve_str(cmd,NULL);
			}else{
				memset(cmd,0,sizeof(cmd));
				sprintf(cmd,"echo 0 > $(ls /sys/class/leds/*rssi%d/brightness)",i+4);
				cmd_recieve_str(cmd,NULL);
			}
		}
	}
	
}
/* 功能：控制灯（RSSI模式）,控制rssi1~rssi8
** 返回值：void
*/
void controlLED_RSSI3()
{
	char cmd[1024];
	char tmp_buff[1024];
	memset(tmp_buff,0,sizeof(tmp_buff));
	memset(cmd,0,sizeof(cmd));
	sprintf(cmd,"uci -q -P /var/state get cellular.%s.SVAL | awk -F ' ' '{print$2}'",name_config);
	int signal=cmd_recieve_int(cmd);
	int i;
	int signal_array[4]={1,2,4,5};
	memset(cmd,0,sizeof(cmd));
	sprintf(cmd,"cat /sys/class/leds/sim_sw/brightness"); //获取当前卡槽
	int value_currentcard = cmd_recieve_int(cmd); 
	if(value_currentcard==255)
		value_currentcard=1;
	if(value_currentcard==1)
	{
		for(i=1;i<=4;i++){//使用从rssi1~rssi4的灯
			memset(cmd,0,sizeof(cmd));
			sprintf(cmd,"echo 0 > $(ls /sys/class/leds/*rssi%d/brightness)",i+4);
			cmd_recieve_str(cmd,NULL);
		}
		for(i=1;i<=4;i++){//使用从rssi1~rssi4的灯
			if(signal>=signal_array[i-1]){
				memset(cmd,0,sizeof(cmd));
				sprintf(cmd,"echo 1 > $(ls /sys/class/leds/*rssi%d/brightness)",i);
				cmd_recieve_str(cmd,NULL);
			}else{
				memset(cmd,0,sizeof(cmd));
				sprintf(cmd,"echo 0 > $(ls /sys/class/leds/*rssi%d/brightness)",i);
				cmd_recieve_str(cmd,NULL);
			}
		}
	}
	else if(value_currentcard==0)
	{
		for(i=1;i<=4;i++){//使用从rssi1~rssi4的灯
			memset(cmd,0,sizeof(cmd));
			sprintf(cmd,"echo 0 > $(ls /sys/class/leds/*rssi%d/brightness)",i);
			cmd_recieve_str(cmd,NULL);
		}
		for(i=1;i<=4;i++){//使用从rssi5~rssi8的灯
			if(signal>=signal_array[i-1]){
				memset(cmd,0,sizeof(cmd));
				sprintf(cmd,"echo 1 > $(ls /sys/class/leds/*rssi%d/brightness)",i+4);
				cmd_recieve_str(cmd,NULL);
			}else{
				memset(cmd,0,sizeof(cmd));
				sprintf(cmd,"echo 0 > $(ls /sys/class/leds/*rssi%d/brightness)",i+4);
				cmd_recieve_str(cmd,NULL);
			}
		}
		
	}
	
}
/* 功能：控制灯,这是一个总控分类，按照680、690、990、620、4008等划分需要控制的灯
** 返回值：void
*/
void controlLED()
{
	if(strstr(board_name,"RT990") || strstr(board_name,"R650") || strstr(board_name,"E940") || strstr(board_name,"R602")) //RT990灯控由/etc/slkapp/led_online_ctrl控制，不在此处理;R650\E940、R602无灯控
	{
		return;
	}
	else if(strstr(board_name,"R680"))
		controlLED_R680();
	else if(strstr(board_name,"R690"))
		if(!checkACM())
			controlLED_R690(); //R690可能是双模块
	else if(strstr(board_name,"R620"))
		controlLED_R620();
	else if(strstr(board_name,"R660"))
		controlLED_R660();
	else if(strstr(board_name,"R4008"))
		controlLED_R4008();
}

/* 功能：控制灯,R680需要控制rssi1~4，led_5g和led_2g共6个灯
** 返回值：void
*/
void controlLED_R680()
{
	char cmd[1024];
	char type_led[1024];
	char tmp_buff[1024];
	memset(cmd,0,sizeof(cmd));
	strcpy(cmd,"ls /sys/class/leds | xargs echo -n | sed 's/^[[:space:]]*//g'");
	memset(type_led,0,sizeof(type_led));
	cmd_recieve_str(cmd,type_led);
	if(strstr(type_led,"rssi"))				//rssi
	{
		controlLED_RSSI();
	}
	memset(cmd,0,sizeof(cmd));
	sprintf(cmd,"uci -q -P /var/state get cellular.%s.CPSI | awk -F ' ' '{print$2}'",name_config);
	memset(tmp_buff,0,sizeof(tmp_buff));
	cmd_recieve_str(cmd,tmp_buff);
	if(strstr(tmp_buff,"NR5G_NSA"))
	{
		memset(cmd,0,sizeof(cmd));
		sprintf(cmd,"echo 1 > $(ls /sys/class/leds/led_5g/brightness)");
		cmd_recieve_str(cmd,NULL);
		memset(cmd,0,sizeof(cmd));
		sprintf(cmd,"echo 1 > $(ls /sys/class/leds/led_2g/brightness)");
		cmd_recieve_str(cmd,NULL);
	}
	else if(strstr(tmp_buff,"NR5G") || strstr(tmp_buff,"NR"))
	{
		memset(cmd,0,sizeof(cmd));
		sprintf(cmd,"echo 0 > $(ls /sys/class/leds/led_5g/brightness)");
		cmd_recieve_str(cmd,NULL);
		memset(cmd,0,sizeof(cmd));
		sprintf(cmd,"echo 1 > $(ls /sys/class/leds/led_2g/brightness)");
		cmd_recieve_str(cmd,NULL);
	}
	else if(strstr(tmp_buff,"LTE"))
	{
		memset(cmd,0,sizeof(cmd));
		sprintf(cmd,"echo 1 > $(ls /sys/class/leds/led_5g/brightness)");
		cmd_recieve_str(cmd,NULL);
		if(!strstr(value_module,"sim7600"))
		{
			memset(cmd,0,sizeof(cmd));
			sprintf(cmd,"echo 0 > $(ls /sys/class/leds/led_2g/brightness)");
			cmd_recieve_str(cmd,NULL);
		}
	}
	else
	{
		memset(cmd,0,sizeof(cmd));
		sprintf(cmd,"echo 0 > $(ls /sys/class/leds/led_5g/brightness)");
		cmd_recieve_str(cmd,NULL);
		if(!strstr(value_module,"sim7600"))
		{
			memset(cmd,0,sizeof(cmd));
			sprintf(cmd,"echo 0 > $(ls /sys/class/leds/led_2g/brightness)");
			cmd_recieve_str(cmd,NULL);
		}
	}
}
/* 功能：控制灯,R690分两种情况，单模块需要控制rssi1~4，led_5g和led_2g共6个灯，双模块需要控制rssi1~8共8个灯
** 返回值：void
*/
void controlLED_R690()
{
	char cmd[1024];
	char type_led[1024];
	char tmp_buff[1024];
	memset(cmd,0,sizeof(cmd));
	strcpy(cmd,"ls /sys/class/leds | xargs echo -n | sed 's/^[[:space:]]*//g'");
	memset(type_led,0,sizeof(type_led));
	cmd_recieve_str(cmd,type_led);
	if(strstr(type_led,"rssi"))				//rssi
	{
		if(strcmp(name_config,"SIM")!=0)
			controlLED_RSSI2();	//双模块rssi灯控制
		else
			controlLED_RSSI3();	//单模块rssi灯控制
	}
	
	if(strcmp(name_config,"SIM")!=0)
	{
		//双模块此部分灯直接由system的led文件控制
		memset(cmd,0,sizeof(cmd));
		sprintf(cmd,"uci -q get system.%s",name_config);
		memset(tmp_buff,0,sizeof(tmp_buff));
		cmd_recieve_str(cmd,tmp_buff);
		if(!strstr(tmp_buff,"led"))
		{
			memset(cmd,0,sizeof(cmd));
			sprintf(cmd,"uci set system.%s=led",name_config);
			system(cmd);
			memset(cmd,0,sizeof(cmd));
			sprintf(cmd,"uci set system.%s.name=%s",name_config,name_config);
			system(cmd);
			if(strcmp(name_config,"SIM1")==0)
			{
				memset(cmd,0,sizeof(cmd));
				sprintf(cmd,"uci set system.%s.sysfs=led_5g",name_config);
				system(cmd);
			}
			else
			{
				memset(cmd,0,sizeof(cmd));
				sprintf(cmd,"uci set system.%s.sysfs=led_2g",name_config);
				system(cmd);
			}
			memset(cmd,0,sizeof(cmd));
			sprintf(cmd,"uci set system.%s.default=0",name_config);
			system(cmd);
			memset(cmd,0,sizeof(cmd));
			sprintf(cmd,"uci set system.%s.trigger=netdev",name_config);
			system(cmd);
			memset(cmd,0,sizeof(cmd));
			sprintf(cmd,"uci set system.%s.mode=link",name_config);
			system(cmd);
			system("uci commit system");
		}
		memset(cmd,0,sizeof(cmd));
		sprintf(cmd,"uci -q get system.%s.dev",name_config);
		memset(tmp_buff,0,sizeof(tmp_buff));
		cmd_recieve_str(cmd,tmp_buff);
		if(!strstr(tmp_buff,value_ifname))
		{
			memset(cmd,0,sizeof(cmd));
			sprintf(cmd,"uci set system.%s.dev=%s",name_config,value_ifname);
			system(cmd);
			system("uci commit system");
			system("/etc/init.d/led restart");
		}
		
	}
	else
	{
		memset(cmd,0,sizeof(cmd));
		sprintf(cmd,"uci -q -P /var/state get cellular.%s.CPSI | awk -F ' ' '{print$2}'",name_config);
		memset(tmp_buff,0,sizeof(tmp_buff));
		cmd_recieve_str(cmd,tmp_buff);
		if(strstr(tmp_buff,"NR5G_NSA") || strstr(tmp_buff,"NR5G") || strstr(tmp_buff,"LTE"))
		{
			memset(cmd,0,sizeof(cmd));
			sprintf(cmd,"cat /sys/class/leds/sim_sw/brightness"); //获取当前卡槽
			int value_currentcard = cmd_recieve_int(cmd); 
			if(value_currentcard==255)
				value_currentcard=1;
			if(value_currentcard==1)
			{
				memset(cmd,0,sizeof(cmd));
				sprintf(cmd,"echo 1 > $(ls /sys/class/leds/led_5g/brightness)");
				cmd_recieve_str(cmd,NULL);
				memset(cmd,0,sizeof(cmd));
				sprintf(cmd,"echo 0 > $(ls /sys/class/leds/led_2g/brightness)");
				cmd_recieve_str(cmd,NULL);
			}
			else if(value_currentcard==0)
			{
				memset(cmd,0,sizeof(cmd));
				sprintf(cmd,"echo 0 > $(ls /sys/class/leds/led_5g/brightness)");
				cmd_recieve_str(cmd,NULL);
				memset(cmd,0,sizeof(cmd));
				sprintf(cmd,"echo 1 > $(ls /sys/class/leds/led_2g/brightness)");
				cmd_recieve_str(cmd,NULL);
			}
		}
		else
		{
			memset(cmd,0,sizeof(cmd));
			sprintf(cmd,"echo 0 > $(ls /sys/class/leds/led_5g/brightness)");
			cmd_recieve_str(cmd,NULL);
			if(!strstr(value_module,"sim7600"))
			{
				memset(cmd,0,sizeof(cmd));
				sprintf(cmd,"echo 0 > $(ls /sys/class/leds/led_2g/brightness)");
				cmd_recieve_str(cmd,NULL);
			}
		}
	}
}
/* 功能：控制灯,R620控制rssi1~4共4个灯
** 返回值：void
*/
void controlLED_R620()
{
	char cmd[1024];
	char type_led[1024];
	char tmp_buff[1024];
	memset(cmd,0,sizeof(cmd));
	strcpy(cmd,"ls /sys/class/leds | xargs echo -n | sed 's/^[[:space:]]*//g'");
	memset(type_led,0,sizeof(type_led));
	cmd_recieve_str(cmd,type_led);
	if(strstr(type_led,"rssi"))				//rssi
	{
		controlLED_RSSI();
	}
}
/* 功能：控制灯,R660需要控制rssi1~4，5g共5个灯
** 返回值：void
*/
void controlLED_R660()
{
	char cmd[1024];
	char type_led[1024];
	char tmp_buff[1024];
	memset(cmd,0,sizeof(cmd));
	strcpy(cmd,"ls /sys/class/leds | xargs echo -n | sed 's/^[[:space:]]*//g'");
	memset(type_led,0,sizeof(type_led));
	cmd_recieve_str(cmd,type_led);
	if(strstr(type_led,"rssi"))				//rssi
	{
		controlLED_RSSI();
	}
	memset(cmd,0,sizeof(cmd));
	sprintf(cmd,"uci -q -P /var/state get cellular.%s.CPSI | awk -F ' ' '{print$2}'",name_config);
	memset(tmp_buff,0,sizeof(tmp_buff));
	cmd_recieve_str(cmd,tmp_buff);
	if(strstr(value_module,"fm650") || strstr(value_module,"fm160"))
	{
		if(strstr(tmp_buff,"NR5G_NSA"))
		{
			memset(cmd,0,sizeof(cmd));
			sprintf(cmd,"echo 1 > $(ls /sys/class/leds/5g/brightness)");
			cmd_recieve_str(cmd,NULL);
			/* memset(cmd,0,sizeof(cmd));
			sprintf(cmd,"echo 1 > $(ls /sys/class/leds/4g/brightness)");
			cmd_recieve_str(cmd,NULL); */
		}
		else if(strstr(tmp_buff,"NR5G"))
		{
			memset(cmd,0,sizeof(cmd));
			sprintf(cmd,"echo 1 > $(ls /sys/class/leds/5g/brightness)");
			cmd_recieve_str(cmd,NULL);
			/* memset(cmd,0,sizeof(cmd));
			sprintf(cmd,"echo 1 > $(ls /sys/class/leds/4g/brightness)");
			cmd_recieve_str(cmd,NULL); */
		}
		else if(strstr(tmp_buff,"LTE"))
		{
			memset(cmd,0,sizeof(cmd));
			sprintf(cmd,"echo 0 > $(ls /sys/class/leds/5g/brightness)");
			cmd_recieve_str(cmd,NULL);
			/* if(!strstr(value_module,"sim7600"))
			{
				memset(cmd,0,sizeof(cmd));
				sprintf(cmd,"echo 0 > $(ls /sys/class/leds/4g/brightness)");
				cmd_recieve_str(cmd,NULL);
			} */
		}
		else
		{
			memset(cmd,0,sizeof(cmd));
			sprintf(cmd,"echo 0 > $(ls /sys/class/leds/5g/brightness)");
			cmd_recieve_str(cmd,NULL);
			/* if(!strstr(value_module,"sim7600"))
			{
				memset(cmd,0,sizeof(cmd));
				sprintf(cmd,"echo 0 > $(ls /sys/class/leds/4g/brightness)");
				cmd_recieve_str(cmd,NULL);
			} */
		}
	}
	else
	{
		if(strstr(tmp_buff,"LTE"))
		{
			memset(cmd,0,sizeof(cmd));
			sprintf(cmd,"echo 1 > $(ls /sys/class/leds/5g/brightness)");
			cmd_recieve_str(cmd,NULL);
		}
		else
		{
			memset(cmd,0,sizeof(cmd));
			sprintf(cmd,"echo 0 > $(ls /sys/class/leds/5g/brightness)");
			cmd_recieve_str(cmd,NULL);
		}
	}
}
/* 功能：控制灯,R4008控制共3个灯
** 返回值：void
*/
void controlLED_R4008()
{
	initiate_controlLED_R4008(); //R4008 dts文件中未包含led的定义，所以需要初始化gpio
	char cmd[1024];
	char tmp_buff[1024];
	memset(tmp_buff,0,sizeof(tmp_buff));
	memset(cmd,0,sizeof(cmd));
	sprintf(cmd,"uci -q -P /var/state get cellular.%s.CPSI | awk -F ' ' '{print$2}'",name_config);
	int signal=cmd_recieve_int(cmd);

	if(signal>=1)
		cmd_recieve_str("echo 0 > $(ls /sys/class/gpio/gpio3/value)",NULL);
	else
		cmd_recieve_str("echo 1 > $(ls /sys/class/gpio/gpio3/value)",NULL);
	if(signal>2)
		cmd_recieve_str("echo 0 > $(ls /sys/class/gpio/gpio2/value)",NULL);
	else
		cmd_recieve_str("echo 1 > $(ls /sys/class/gpio/gpio2/value)",NULL);
	if(signal>=4)
		cmd_recieve_str("echo 0 > $(ls /sys/class/gpio/gpio0/value)",NULL);
	else
		cmd_recieve_str("echo 1 > $(ls /sys/class/gpio/gpio0/value)",NULL);
}


void initiate_controlLED_R4008()
{
	char cmd[1024];
	int result=0;
	
	result=cmd_recieve_int("ls /sys/class/gpio | grep gpio0 | wc -l");
	if(result!=1)
	{
		memset(cmd,0,sizeof(cmd));
		system("echo 0 > /sys/class/gpio/export");
		system("echo out > /sys/class/gpio/gpio0/direction");
		system("echo 1 > /sys/class/gpio/gpio0/value");
	}
	result=cmd_recieve_int("ls /sys/class/gpio | grep gpio2 | wc -l");
	if(result!=1)
	{
		memset(cmd,0,sizeof(cmd));
		system("echo 2 > /sys/class/gpio/export");
		system("echo out > /sys/class/gpio/gpio2/direction");
		system("echo 1 > /sys/class/gpio/gpio2/value");
	}
	result=cmd_recieve_int("ls /sys/class/gpio | grep gpio3 | wc -l");
	if(result!=1)
	{
		memset(cmd,0,sizeof(cmd));
		system("echo 3 > /sys/class/gpio/export");
		system("echo out > /sys/class/gpio/gpio3/direction");
		system("echo 1 > /sys/class/gpio/gpio3/value");
	}
}