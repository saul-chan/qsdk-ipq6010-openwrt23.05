#include "function.h"
#include "ctrl_leds.h"
#include "fm650.h"
#include "fm150_na.h"
#include "rm500u.h"
#include "sim7600.h"
#include "sim8200.h"
#include "mt5700.h"
#include "dialing.h"

int main(int argc, char *argv[])
{
	if(argc==2)
	{
		if(strstr(argv[1],"-h") || strstr(argv[1],"help"))
		{
			printf("Usage: dialing [options]\n\n");
			printf("Examples:\n");
			printf("Start network connection with single module\n");
			printf("    dialing\n");
			printf("Start network connection with one of the multiple modules\n");
			printf("    dialing SIM1\n\n");
			printf("Options:\n");
			printf("    SIM1, SIM2, SIM3, SIM4  dial with one of the multiple modules\n");
			printf("    -h, --help, help        print this message and quit\n");
			printf("    -v, --version, version  print this message and quit\n");
			return 0;
		}
		else if(strstr(argv[1],"-v") || strstr(argv[1],"version"))
		{
			printf("dialing version 3.0\n");
			return 0;
		}
	}
	/*获取开发板版本*/
	check_board();
	if(argc==2)
	{
		
		/*多模组拨号*/
		strcpy(name_config,argv[1]);
		if(strstr(name_config,"SIM3"))
			strcpy(flag_module,"1-1.1");
		//else if(strstr(name_config,"SIM2")) //RT990 双5G
		else if(strstr(name_config,"SIM1")) //R690 双5G
		{
			strcpy(flag_module,"2-1"); //990双5G
			//strcpy(flag_module,"1-1"); //690 双4G
		}
		//else if(strstr(name_config,"SIM1"))  //RT990 双5G
		else if(strstr(name_config,"SIM2")) //R690 双5G
			strcpy(flag_module,"3-1");
		else if(strstr(name_config,"SIM4"))
			strcpy(flag_module,"3-1.2");
		fillpath();
		readConfig();
	}
	else
	{
		/*单模组拨号*/
		strcpy(name_config,"SIM");
		strcpy(flag_module,"\\-1"); //通用模块
		//strcpy(flag_module,"3-1.1"); //FG160
		fillpath();
		if(strstr(board_name,"R680") || strstr(board_name,"R690") || strstr(board_name,"RT990") || strstr(board_name,"R660") || strstr(board_name,"R650"))
			readConfig_2card();
		else
			readConfig();
	}
	
	/*获取模组信息*/
	getModule();
	/*设置模组拨号方式*/
	setModule();
	while(1)
	{
		/*读取驻网信息*/
		getModem();
		/*信号灯控制*/
		controlLED();
		/*拨号*/
		Dial();
		/*网络监测*/
		checkDNS();
		/*记录循环次数*/
		func_loop();
	}
	return 0;
}



