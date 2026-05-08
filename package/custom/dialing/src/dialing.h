#ifndef DIALING_H
#define DIALING_H

int flag_dial;
int flag_forcedial;
char value_apn[100];
int mode_rat;
char value_pin[100];
char value_auth[100];
char value_username[100];
char value_password[100];
char type_ip[100];
int flag_ping;
char value_pingaddr[500];
int count_ping;
int count_error=1;
char mode_usb[100];
char mode_nat[100];
char value_defaultcard[100];
char value_metric[100];
char value_mtu[100];
char mode_opera[100];
char band_wcdma[100];
char band_lte[100];
char band_nr[100];
char band_nsa[100];
char band_cdma[100];
char band_gsm[100];
char band_scdma[100];
char band_umts[100];
char value_arfcn[100];
char value_pci[100];
char value_psc[100];
char value_plmn[100];
int flag_ims;
int value_scs;
int flag_gps;
int flag_gpslog;
char path_gpslog[500];
int SMSIMCFG;
/**日志打印控制标识**/
int sig_flight=0;
int dial_flight=0;
int status_flight=0;
int dialip_debug=0;
char dialresult_debug[100]="null";
char dns_debug[100]="null"; 
int sighi_debug=0;
int siglo_debug=0;
int sleep_delay[5]={15,30,60,120,180};

//单个模块读取双卡配置——卡1配置
char value_apn_card1[100];
int mode_rat_card1;
char value_pin_card1[100];
char value_auth_card1[100];
char value_username_card1[100];
char value_password_card1[100];
char type_ip_card1[100];
char band_wcdma_card1[100];
char band_lte_card1[100];
char band_nr_card1[100];
char band_nsa_card1[100];
char band_cdma_card1[100];
char band_gsm_card1[100];
char band_scdma_card1[100];
char band_umts_card1[100];
char value_arfcn_card1[100];
char value_pci_card1[100];
char value_psc_card1[100];
char value_plmn_card1[100];
int value_scs_card1;
//单个模块读取双卡配置——卡2配置
char value_apn_card2[100];
int mode_rat_card2;
char value_pin_card2[100];
char value_auth_card2[100];
char value_username_card2[100];
char value_password_card2[100];
char type_ip_card2[100];
char band_wcdma_card2[100];
char band_lte_card2[100];
char band_nr_card2[100];
char band_nsa_card2[100];
char band_cdma_card2[100];
char band_gsm_card2[100];
char band_scdma_card2[100];
char band_umts_card2[100];
char value_arfcn_card2[100];
char value_pci_card2[100];
char value_psc_card2[100];
int value_scs_card2;
char value_plmn_card2[100];

//要打印的信息
char debug_dial_ip[100];
char debug_dial_gateway[100];
char debug_dial_dns[100];
char debug_nservice[100]="null";
char debug_band[100]="null";
char debug_cellid[100]="null";
char debug_enb[100]="null";
char debug_arfcn[100]="null";
char debug_pci[100]="null";
char debug_rssi[100];
char debug_rsrq[100];
char debug_rsrp[100];
char debug_sinr[100];
char debug_cgreg[100]="null";
char debug_cereg[100]="null";
char debug_c5greg[100]="null";
char debug_pincode[100]="null";
int debug_reboot_status=0;
char debug_cops[100]="null";
char debug_iccid[100]="null";
char debug_imsi[100]="null";
char debug_readcard[100]="null";

/****/
/* 功能：读SIM配置，单模块双SIM卡
** 返回值：void
*/
void readConfig_2card()
{
	//sleep(2);
	char tmp_buff[1024];
	char cmd[1024];
	memset(cmd,0,sizeof(cmd));
	sprintf(cmd,"uci -q show modem.@default[0]");
	memset(tmp_buff,0,sizeof(tmp_buff));
	cmd_recieve_str(cmd,tmp_buff);
	if(strlen(tmp_buff)<0)
	{
		Debug("not found config modem.@default[0], exit","daemon.error");
		exit(1);
	}
	//是否开启拨号功能
	memset(cmd,0,sizeof(cmd));
	sprintf(cmd,"uci -q get modem.@default[0].enable");
	flag_dial = cmd_recieve_int(cmd);
	printf("flag: %d",flag_dial);
	//是否开启强制拨号功能
	// memset(cmd,0,sizeof(cmd));
	// sprintf(cmd,"uci -q get modem.%s.force_dial",name_config);
	// flag_forcedial = cmd_recieve_int(cmd);
	//APN
	memset(cmd,0,sizeof(cmd));
	memset(value_apn_card1,0,sizeof(value_apn_card1));
	sprintf(cmd,"uci -q get modem.sim1.apn");
	cmd_recieve_str(cmd,value_apn_card1);
	//服务类型
	memset(cmd,0,sizeof(cmd));
	sprintf(cmd,"uci -q get modem.sim1.smode");
	mode_rat_card1 = cmd_recieve_int(cmd);
	//3G频段
	memset(cmd,0,sizeof(cmd));
	memset(band_wcdma_card1,0,sizeof(band_wcdma_card1));
	sprintf(cmd,"uci -q get modem.sim1.band_wcdma");
	cmd_recieve_str(cmd,band_wcdma_card1);
	//4G频段
	memset(cmd,0,sizeof(cmd));
	memset(band_lte_card1,0,sizeof(band_lte_card1));
	sprintf(cmd,"uci -q get modem.sim1.band_lte");
	cmd_recieve_str(cmd,band_lte_card1);
	//5G频段
	memset(cmd,0,sizeof(cmd));
	memset(band_nr_card1,0,sizeof(band_nr_card1));
	sprintf(cmd,"uci -q get modem.sim1.band_nr");
	cmd_recieve_str(cmd,band_nr_card1);
	//5GNSA频段
	memset(cmd,0,sizeof(cmd));
	memset(band_nsa_card1,0,sizeof(band_nsa_card1));
	sprintf(cmd,"uci -q get modem.sim1.band_nsa");
	cmd_recieve_str(cmd,band_nsa_card1);
	//CDMA
	memset(cmd,0,sizeof(cmd));
	memset(band_cdma_card1,0,sizeof(band_cdma_card1));
	sprintf(cmd,"uci -q get modem.sim1.band_cdma");
	cmd_recieve_str(cmd,band_cdma_card1);
	//GSM
	memset(cmd,0,sizeof(cmd));
	memset(band_gsm_card1,0,sizeof(band_gsm_card1));
	sprintf(cmd,"uci -q get modem.sim1.band_gsm");
	cmd_recieve_str(cmd,band_gsm_card1);
	//UMTS
	memset(cmd,0,sizeof(cmd));
	memset(band_umts_card1,0,sizeof(band_umts_card1));
	sprintf(cmd,"uci -q get modem.sim1.band_umts");
	cmd_recieve_str(cmd,band_umts_card1);
	//TD_SCDMA
	memset(cmd,0,sizeof(cmd));
	memset(band_scdma_card1,0,sizeof(band_scdma_card1));
	sprintf(cmd,"uci -q get modem.sim1.band_scdma");
	cmd_recieve_str(cmd,band_scdma_card1);
	//频点
	memset(cmd,0,sizeof(cmd));
	memset(value_arfcn_card1,0,sizeof(value_arfcn_card1));
	sprintf(cmd,"uci -q get modem.sim1.arfcn");
	cmd_recieve_str(cmd,value_arfcn_card1);
	//小区ID
	memset(cmd,0,sizeof(cmd));
	memset(value_pci_card1,0,sizeof(value_pci_card1));
	sprintf(cmd,"uci -q get modem.sim1.pci");
	cmd_recieve_str(cmd,value_pci_card1);
	//小区ID
	memset(cmd,0,sizeof(cmd));
	memset(value_psc_card1,0,sizeof(value_psc_card1));
	sprintf(cmd,"uci -q get modem.sim1.psc");
	cmd_recieve_str(cmd,value_psc_card1);
	//子载波间隔
	memset(cmd,0,sizeof(cmd));
	sprintf(cmd,"uci -q get modem.sim1.scs");
	value_scs_card1 = cmd_recieve_int(cmd);
	//PIN码
	memset(cmd,0,sizeof(cmd));
	memset(value_pin_card1,0,sizeof(value_pin_card1));
	sprintf(cmd,"uci -q get modem.sim1.pincode");
	cmd_recieve_str(cmd,value_pin_card1);
	//认证类型
	memset(cmd,0,sizeof(cmd));
	memset(value_auth_card1,0,sizeof(value_auth_card1));
	sprintf(cmd,"uci -q get modem.sim1.auth_type");
	cmd_recieve_str(cmd,value_auth_card1);
	
	//PLMN
	memset(cmd,0,sizeof(cmd));
	memset(value_plmn_card1,0,sizeof(value_plmn_card1));
	sprintf(cmd,"uci -q get modem.sim1.plmn");
	cmd_recieve_str(cmd,value_plmn_card1);
	//用户名
	memset(cmd,0,sizeof(cmd));
	memset(value_username_card1,0,sizeof(value_username_card1));
	sprintf(cmd,"uci -q get modem.sim1.username");
	cmd_recieve_str(cmd,value_username_card1);
	//密码
	memset(cmd,0,sizeof(cmd));
	memset(value_password_card1,0,sizeof(value_password_card1));
	sprintf(cmd,"uci -q get modem.sim1.password");
	cmd_recieve_str(cmd,value_password_card1);
	//拨号IP类型
	memset(cmd,0,sizeof(cmd));
	memset(type_ip_card1,0,sizeof(type_ip_card1));
	sprintf(cmd,"uci -q get modem.sim1.ipv4v6");
	cmd_recieve_str(cmd,type_ip_card1);
	
	//APN
	memset(cmd,0,sizeof(cmd));
	memset(value_apn_card2,0,sizeof(value_apn_card2));
	sprintf(cmd,"uci -q get modem.sim2.apn");
	cmd_recieve_str(cmd,value_apn_card2);
	//服务类型
	memset(cmd,0,sizeof(cmd));
	sprintf(cmd,"uci -q get modem.sim2.smode");
	mode_rat_card2 = cmd_recieve_int(cmd);
	//3G频段
	memset(cmd,0,sizeof(cmd));
	memset(band_wcdma_card2,0,sizeof(band_wcdma_card2));
	sprintf(cmd,"uci -q get modem.sim2.band_wcdma");
	cmd_recieve_str(cmd,band_wcdma_card2);
	//4G频段
	memset(cmd,0,sizeof(cmd));
	memset(band_lte_card2,0,sizeof(band_lte_card2));
	sprintf(cmd,"uci -q get modem.sim2.band_lte");
	cmd_recieve_str(cmd,band_lte_card2);
	//5G频段
	memset(cmd,0,sizeof(cmd));
	memset(band_nr_card2,0,sizeof(band_nr_card2));
	sprintf(cmd,"uci -q get modem.sim2.band_nr");
	cmd_recieve_str(cmd,band_nr_card2);
	//5GNSA频段
	memset(cmd,0,sizeof(cmd));
	memset(band_nsa_card2,0,sizeof(band_nsa_card2));
	sprintf(cmd,"uci -q get modem.sim2.band_nsa");
	cmd_recieve_str(cmd,band_nsa_card2);
	//CDMA
	memset(cmd,0,sizeof(cmd));
	memset(band_cdma_card2,0,sizeof(band_cdma_card2));
	sprintf(cmd,"uci -q get modem.sim2.band_cdma");
	cmd_recieve_str(cmd,band_cdma_card2);
	//GSM
	memset(cmd,0,sizeof(cmd));
	memset(band_gsm_card2,0,sizeof(band_gsm_card2));
	sprintf(cmd,"uci -q get modem.sim2.band_gsm");
	cmd_recieve_str(cmd,band_gsm_card2);
	//UMTS
	memset(cmd,0,sizeof(cmd));
	memset(band_umts_card2,0,sizeof(band_umts_card2));
	sprintf(cmd,"uci -q get modem.sim2.band_umts");
	cmd_recieve_str(cmd,band_umts_card2);
	//TD_SCDMA
	memset(cmd,0,sizeof(cmd));
	memset(band_scdma_card2,0,sizeof(band_scdma_card2));
	sprintf(cmd,"uci -q get modem.sim2.band_scdma");
	cmd_recieve_str(cmd,band_scdma_card2);
	//频点
	memset(cmd,0,sizeof(cmd));
	memset(value_arfcn_card2,0,sizeof(value_arfcn_card2));
	sprintf(cmd,"uci -q get modem.sim2.arfcn");
	cmd_recieve_str(cmd,value_arfcn_card2);
	//小区ID
	memset(cmd,0,sizeof(cmd));
	memset(value_pci_card2,0,sizeof(value_pci_card2));
	sprintf(cmd,"uci -q get modem.sim2.pci");
	cmd_recieve_str(cmd,value_pci_card2);
	//小区ID
	memset(cmd,0,sizeof(cmd));
	memset(value_psc_card2,0,sizeof(value_psc_card2));
	sprintf(cmd,"uci -q get modem.sim2.psc");
	cmd_recieve_str(cmd,value_psc_card2);
	//子载波间隔
	memset(cmd,0,sizeof(cmd));
	sprintf(cmd,"uci -q get modem.sim2.scs");
	value_scs_card2 = cmd_recieve_int(cmd);
	//PIN码
	memset(cmd,0,sizeof(cmd));
	memset(value_pin_card2,0,sizeof(value_pin_card2));
	sprintf(cmd,"uci -q get modem.sim2.pincode");
	cmd_recieve_str(cmd,value_pin_card2);
	//认证类型
	memset(cmd,0,sizeof(cmd));
	memset(value_auth_card2,0,sizeof(value_auth_card2));
	sprintf(cmd,"uci -q get modem.sim2.auth_type");
	cmd_recieve_str(cmd,value_auth_card2);
	
	//PLMN
	memset(cmd,0,sizeof(cmd));
	memset(value_plmn_card2,0,sizeof(value_plmn_card2));
	sprintf(cmd,"uci -q get modem.sim2.plmn");
	cmd_recieve_str(cmd,value_plmn_card2);
	//用户名
	memset(cmd,0,sizeof(cmd));
	memset(value_username_card2,0,sizeof(value_username_card2));
	sprintf(cmd,"uci -q get modem.sim2.username");
	cmd_recieve_str(cmd,value_username_card2);
	//密码
	memset(cmd,0,sizeof(cmd));
	memset(value_password_card2,0,sizeof(value_password_card2));
	sprintf(cmd,"uci -q get modem.sim2.password");
	cmd_recieve_str(cmd,value_password_card2);
	//拨号IP类型
	memset(cmd,0,sizeof(cmd));
	memset(type_ip_card2,0,sizeof(type_ip_card2));
	sprintf(cmd,"uci -q get modem.sim2.ipv4v6");
	cmd_recieve_str(cmd,type_ip_card2);
	//是否开启网络监测功能
	memset(cmd,0,sizeof(cmd));
	sprintf(cmd,"uci -q get modem.@default[0].pingen");
	flag_ping = cmd_recieve_int(cmd);
	//网络监测目标IP
	memset(cmd,0,sizeof(cmd));
	memset(value_pingaddr,0,sizeof(value_pingaddr));
	sprintf(cmd,"uci -q get modem.@default[0].pingaddr");
	cmd_recieve_str(cmd,value_pingaddr);
	//网络监测模式
	memset(cmd,0,sizeof(cmd));
	memset(mode_opera,0,sizeof(mode_opera));
	sprintf(cmd,"uci -q get modem.@default[0].opera_mode");
	cmd_recieve_str(cmd,mode_opera);
	//网络监测周期
	memset(cmd,0,sizeof(cmd));
	sprintf(cmd,"uci -q get modem.@default[0].count");
	count_ping = cmd_recieve_int(cmd);
	//拨号上网驱动
	memset(cmd,0,sizeof(cmd));
	memset(mode_usb,0,sizeof(mode_usb));
	sprintf(cmd,"uci -q get modem.@default[0].usbmode");
	cmd_recieve_str(cmd,mode_usb);
	//拨号上网方式
	memset(cmd,0,sizeof(cmd));
	memset(mode_nat,0,sizeof(mode_nat));
	sprintf(cmd,"uci -q get modem.@default[0].natmode");
	cmd_recieve_str(cmd,mode_nat);
	
	//默认SIM卡
	memset(cmd,0,sizeof(cmd));
	memset(value_defaultcard,0,sizeof(value_defaultcard));
	sprintf(cmd,"uci -q get modem.@default[0].default_card");
	cmd_recieve_str(cmd,value_defaultcard);
	
	//跃点数
	memset(cmd,0,sizeof(cmd));
	memset(value_metric,0,sizeof(value_metric));
	sprintf(cmd,"uci -q get modem.@default[0].metric");
	cmd_recieve_str(cmd,value_metric);
	
	//MTU
	memset(cmd,0,sizeof(cmd));
	memset(value_mtu,0,sizeof(value_mtu));
	sprintf(cmd,"uci -q get modem.@default[0].mtu");
	cmd_recieve_str(cmd,value_mtu);
	
	//IMS
	memset(cmd,0,sizeof(cmd));
	sprintf(cmd,"uci -q get modem.@default[0].ims");
	flag_ims=cmd_recieve_int(cmd);
	if(flag_ims!=1)
		flag_ims=0;
	
	//gps功能
	memset(cmd,0,sizeof(cmd));
	sprintf(cmd,"uci -q get modem.@default[0].gps_enable");
	flag_gps = cmd_recieve_int(cmd);
	
	memset(cmd,0,sizeof(cmd));
	sprintf(cmd,"uci -q get modem.@default[0].gpslog_enable");
	flag_gpslog = cmd_recieve_int(cmd);
	
	memset(cmd,0,sizeof(cmd));
	memset(path_gpslog,0,sizeof(path_gpslog));
	sprintf(cmd,"uci -q get modem.@default[0].gps_path");
	cmd_recieve_str(cmd,path_gpslog);
	
	memset(cmd,0,sizeof(cmd));
	sprintf(cmd,"uci -q get modem.@default[0].heartbeat");
	flag_heartbeat=cmd_recieve_int(cmd);
	if(flag_heartbeat!=1)
		flag_heartbeat=0;
	
	if (flag_gps == 1)
	{
		flag_gpslog =1;
		memset(path_gpslog,0,sizeof(path_gpslog));
		sprintf(path_gpslog,"/var/log/gps.log");
	}
	
	if ( flag_dial == 1 )
		Debug("dial enable, starting","daemon.info");
	/* else
	{
		Debug("dial disable, exit","daemon.info");
		exit(1);
	} */
}

void readConfig()
{
	//sleep(2);
	char tmp_buff[1024];
	char cmd[1024];
	char debug[500];
	memset(cmd,0,sizeof(cmd));
	sprintf(cmd,"uci -q show modem.%s",name_config);
	memset(tmp_buff,0,sizeof(tmp_buff));
	cmd_recieve_str(cmd,tmp_buff);
	if(strlen(tmp_buff)<0)
	{
		memset(debug,0,sizeof(debug));
		sprintf(debug,"not found config modem.%s, exit",name_config);
		Debug(debug,"daemon.error");
		exit(1);
	}
	//是否开启拨号功能
	memset(cmd,0,sizeof(cmd));
	sprintf(cmd,"uci -q get modem.%s.enable",name_config);
	flag_dial = cmd_recieve_int(cmd);
	//是否开启强制拨号功能
	memset(cmd,0,sizeof(cmd));
	sprintf(cmd,"uci -q get modem.%s.force_dial",name_config);
	flag_forcedial = cmd_recieve_int(cmd);
	//APN
	memset(cmd,0,sizeof(cmd));
	memset(value_apn,0,sizeof(value_apn));
	sprintf(cmd,"uci -q get modem.%s.apn",name_config);
	cmd_recieve_str(cmd,value_apn);
	//服务类型
	memset(cmd,0,sizeof(cmd));
	sprintf(cmd,"uci -q get modem.%s.smode",name_config);
	mode_rat = cmd_recieve_int(cmd);
	//3G频段
	memset(cmd,0,sizeof(cmd));
	memset(band_wcdma,0,sizeof(band_wcdma));
	sprintf(cmd,"uci -q get modem.%s.band_wcdma",name_config);
	cmd_recieve_str(cmd,band_wcdma);
	//4G频段
	memset(cmd,0,sizeof(cmd));
	memset(band_lte,0,sizeof(band_lte));
	sprintf(cmd,"uci -q get modem.%s.band_lte",name_config);
	cmd_recieve_str(cmd,band_lte);
	//5G频段
	memset(cmd,0,sizeof(cmd));
	memset(band_nr,0,sizeof(band_nr));
	sprintf(cmd,"uci -q get modem.%s.band_nr",name_config);
	cmd_recieve_str(cmd,band_nr);
	//5GNSA频段
	memset(cmd,0,sizeof(cmd));
	memset(band_nsa,0,sizeof(band_nsa));
	sprintf(cmd,"uci -q get modem.%s.band_nsa",name_config);
	cmd_recieve_str(cmd,band_nsa);
	//CDMA
	memset(cmd,0,sizeof(cmd));
	memset(band_cdma,0,sizeof(band_cdma));
	sprintf(cmd,"uci -q get modem.%s.band_cdma",name_config);
	cmd_recieve_str(cmd,band_cdma);
	//GSM
	memset(cmd,0,sizeof(cmd));
	memset(band_gsm,0,sizeof(band_gsm));
	sprintf(cmd,"uci -q get modem.%s.band_gsm",name_config);
	cmd_recieve_str(cmd,band_gsm);
	//UMTS
	memset(cmd,0,sizeof(cmd));
	memset(band_umts,0,sizeof(band_umts));
	sprintf(cmd,"uci -q get modem.%s.band_umts",name_config);
	cmd_recieve_str(cmd,band_umts);
	//TD_SCDMA
	memset(cmd,0,sizeof(cmd));
	memset(band_scdma,0,sizeof(band_scdma));
	sprintf(cmd,"uci -q get modem.%s.band_scdma",name_config);
	cmd_recieve_str(cmd,band_scdma);
	//频点
	memset(cmd,0,sizeof(cmd));
	memset(value_arfcn,0,sizeof(value_arfcn));
	sprintf(cmd,"uci -q get modem.%s.arfcn",name_config);
	cmd_recieve_str(cmd,value_arfcn);
	//小区ID
	memset(cmd,0,sizeof(cmd));
	memset(value_pci,0,sizeof(value_pci));
	sprintf(cmd,"uci -q get modem.%s.pci",name_config);
	cmd_recieve_str(cmd,value_pci);
	//小区ID
	memset(cmd,0,sizeof(cmd));
	memset(value_psc,0,sizeof(value_psc));
	sprintf(cmd,"uci -q get modem.%s.psc",name_config);
	cmd_recieve_str(cmd,value_psc);
	//子载波间隔
	memset(cmd,0,sizeof(cmd));
	sprintf(cmd,"uci -q get modem.%s.scs",name_config);
	value_scs = cmd_recieve_int(cmd);
	//PIN码
	memset(cmd,0,sizeof(cmd));
	memset(value_pin,0,sizeof(value_pin));
	sprintf(cmd,"uci -q get modem.%s.pincode",name_config);
	cmd_recieve_str(cmd,value_pin);
	//认证类型
	memset(cmd,0,sizeof(cmd));
	memset(value_auth,0,sizeof(value_auth));
	sprintf(cmd,"uci -q get modem.%s.auth_type",name_config);
	cmd_recieve_str(cmd,value_auth);
	//用户名
	memset(cmd,0,sizeof(cmd));
	memset(value_username,0,sizeof(value_username));
	sprintf(cmd,"uci -q get modem.%s.username",name_config);
	cmd_recieve_str(cmd,value_username);
	//密码
	memset(cmd,0,sizeof(cmd));
	memset(value_password,0,sizeof(value_password));
	sprintf(cmd,"uci -q get modem.%s.password",name_config);
	cmd_recieve_str(cmd,value_password);
	//拨号IP类型
	memset(cmd,0,sizeof(cmd));
	memset(type_ip,0,sizeof(type_ip));
	sprintf(cmd,"uci -q get modem.%s.ipv4v6",name_config);
	cmd_recieve_str(cmd,type_ip);
	//是否开启网络监测功能
	memset(cmd,0,sizeof(cmd));
	sprintf(cmd,"uci -q get modem.%s.pingen",name_config);
	flag_ping = cmd_recieve_int(cmd);
	//网络监测目标IP
	memset(cmd,0,sizeof(cmd));
	memset(value_pingaddr,0,sizeof(value_pingaddr));
	sprintf(cmd,"uci -q get modem.%s.pingaddr",name_config);
	cmd_recieve_str(cmd,value_pingaddr);
	//网络监测模式
	memset(cmd,0,sizeof(cmd));
	memset(mode_opera,0,sizeof(mode_opera));
	sprintf(cmd,"uci -q get modem.%s.opera_mode",name_config);
	cmd_recieve_str(cmd,mode_opera);
	//网络监测周期
	memset(cmd,0,sizeof(cmd));
	sprintf(cmd,"uci -q get modem.%s.count",name_config);
	count_ping = cmd_recieve_int(cmd);
	//拨号上网驱动
	memset(cmd,0,sizeof(cmd));
	memset(mode_usb,0,sizeof(mode_usb));
	sprintf(cmd,"uci -q get modem.%s.usbmode",name_config);
	cmd_recieve_str(cmd,mode_usb);
	//拨号上网方式
	memset(cmd,0,sizeof(cmd));
	memset(mode_nat,0,sizeof(mode_nat));
	sprintf(cmd,"uci -q get modem.%s.natmode",name_config);
	cmd_recieve_str(cmd,mode_nat);
	
	//默认SIM卡
	memset(cmd,0,sizeof(cmd));
	memset(value_defaultcard,0,sizeof(value_defaultcard));
	sprintf(cmd,"uci -q get modem.%s.default_card",name_config);
	cmd_recieve_str(cmd,value_defaultcard);
	
	//跃点数
	memset(cmd,0,sizeof(cmd));
	memset(value_metric,0,sizeof(value_metric));
	sprintf(cmd,"uci -q get modem.%s.metric",name_config);
	cmd_recieve_str(cmd,value_metric);
	
	//MTU
	memset(cmd,0,sizeof(cmd));
	memset(value_mtu,0,sizeof(value_mtu));
	sprintf(cmd,"uci -q get modem.%s.mtu",name_config);
	cmd_recieve_str(cmd,value_mtu);
	
	//gps功能
	memset(cmd,0,sizeof(cmd));
	sprintf(cmd,"uci -q get modem.%s.gps_enable",name_config);
	flag_gps = cmd_recieve_int(cmd);
	
	memset(cmd,0,sizeof(cmd));
	sprintf(cmd,"uci -q get modem.%s.gpslog_enable",name_config);
	flag_gpslog = cmd_recieve_int(cmd);
	
	memset(cmd,0,sizeof(cmd));
	memset(path_gpslog,0,sizeof(path_gpslog));
	sprintf(cmd,"uci -q get modem.%s.gps_path",name_config);
	cmd_recieve_str(cmd,path_gpslog);
	
	//PLMN
	memset(cmd,0,sizeof(cmd));
	memset(value_plmn,0,sizeof(value_plmn));
	sprintf(cmd,"uci -q get modem.%s.plmn",name_config);
	cmd_recieve_str(cmd,value_plmn);
	
	memset(cmd,0,sizeof(cmd));
	sprintf(cmd,"uci -q get modem.%s.heartbeat",name_config);
	flag_heartbeat=cmd_recieve_int(cmd);
	if(flag_heartbeat!=1)
		flag_heartbeat=0;
	
	memset(cmd,0,sizeof(cmd));
	sprintf(cmd,"uci -q get modem.%s.ims",name_config);
	flag_ims=cmd_recieve_int(cmd);
	if(flag_ims!=1)
		flag_ims=0;
	
	if ( flag_dial == 1 )
		Debug("dial enable, starting","daemon.info");
	/*else
	{
		Debug("dial disable, exit","daemon.info");
		exit(1);
	}*/
}

/* 功能：获取模组信息
** 返回结果：0 成功 -1 失败
** 返回值：int
*/
int getModule()
{
	char tmp_buff[4096];
	char value_devicenum[5];
	char cmd[1024];
	char debug[1024];
	while(1){
		//获取ttyUSB*
		memset(cmd,0,sizeof(cmd));
		sprintf(cmd,"ls -l /sys/bus/usb-serial/devices/ | grep \"%s/\" | awk -F '/' '{print $NF}' | xargs echo -n | sed 's/[[:space:]]//g'",flag_module);
		memset(tmp_buff,0,sizeof(tmp_buff));
		cmd_recieve_str(cmd,tmp_buff);
		if(strlen(tmp_buff)>0)
		{
			check_ttyUSB(tmp_buff);
			//获取接口名称
			memset(cmd,0,sizeof(cmd));
			sprintf(cmd,"dmesg | grep \": register\" | grep \"%s\" | tail -n 1 | awk -F ' ' '{print $5}'| awk -F ':' '{print $1}'",flag_module);
			memset(value_ifname,0,sizeof(value_ifname));
			cmd_recieve_str(cmd,value_ifname);
			if(strstr(value_ifname,"register"))
			{
				memset(cmd,0,sizeof(cmd));
				sprintf(cmd,"dmesg | grep \": register\" | grep \"%s\" | tail -n 1 | awk -F ' ' '{print $4}'| awk -F ':' '{print $1}'",flag_module);
				memset(value_ifname,0,sizeof(value_ifname));
				cmd_recieve_str(cmd,value_ifname);
			}
			//获取所加载模组的型号和版本
			memset(cmd,0,sizeof(cmd));
			sprintf(cmd,"cmdat %s ATI",path_usb);
			memset(tmp_buff,0,sizeof(tmp_buff));
			cmd_recieve_str(cmd,tmp_buff);
			memset(value_module,0,sizeof(value_module));
			char net[50];
			memset(net,0,sizeof(net));
			memset(cmd,0,sizeof(cmd));
			sprintf(cmd,"uci -q get modem.@default[0].net");
			cmd_recieve_str(cmd,net);
			if( strstr(tmp_buff,"FM650") )		//广和通模组
			{
				//设定模组标志：fm650
				strcpy(value_module,"fm650");
				if(strlen(net)>0)
				{
					memset(cmd,0,sizeof(cmd));
					sprintf(cmd,"uci -q set modem.@default[0].net='5G'");
					system(cmd);
					memset(cmd,0,sizeof(cmd));
					sprintf(cmd,"uci commit modem");
					system(cmd);
				}
				else
				{
					memset(cmd,0,sizeof(cmd));
					sprintf(cmd,"uci -q set modem.%s.net='5G'",name_config);
					system(cmd);
					memset(cmd,0,sizeof(cmd));
					sprintf(cmd,"uci commit modem");
					system(cmd);
				}
				if(strlen(value_ifname)<=0)
				{
					memset(value_ifname,0,sizeof(value_ifname));
					stpcpy(value_ifname,"usb0");
				}
				memset(debug,0,sizeof(debug));
				sprintf(debug,"modelname: Fibocom FM650; interface: %s; AT: %s",value_ifname,path_usb);
				Debug(debug,"module.info");
				memset(cmd,0,sizeof(cmd));
				sprintf(cmd,"uci -q get system.wifi1 | wc -l");
				int led=cmd_recieve_int(cmd);
				if(led==1)
				{
					system("uci delete system.wifi1");
					system("uci delete system.wifi0");
					system("uci commit");
				}
				return 0;
			}
			else if( strstr(tmp_buff,"FM160") )		//广和通模组
			{
				//设定模组标志：fm160
				strcpy(value_module,"fm160");
				if(strlen(net)>0)
				{
					memset(cmd,0,sizeof(cmd));
					sprintf(cmd,"uci -q set modem.@default[0].net='5G'");
					system(cmd);
					memset(cmd,0,sizeof(cmd));
					sprintf(cmd,"uci commit modem");
					system(cmd);
				}
				else
				{
					memset(cmd,0,sizeof(cmd));
					sprintf(cmd,"uci -q set modem.%s.net='5G'",name_config);
					system(cmd);
					memset(cmd,0,sizeof(cmd));
					sprintf(cmd,"uci commit modem");
					system(cmd);
				}
				if(strlen(value_ifname)<=0)
				{
					if(strcmp(name_config,"SIM2")==0)
					{
						memset(value_ifname,0,sizeof(value_ifname));
						stpcpy(value_ifname,"usb1");
					}
					else
					{
						memset(value_ifname,0,sizeof(value_ifname));
						stpcpy(value_ifname,"usb0");
					}
				}
				if(strcmp(name_config,"SIM2")==0 )//&& isPCIE())
				{
					memset(value_ifname,0,sizeof(value_ifname));
					stpcpy(value_ifname,"pcie_mhi0.1");
				}
				memset(debug,0,sizeof(debug));
				sprintf(debug,"modelname: Fibocom FM160; interface: %s; AT: %s",value_ifname,path_usb);
				Debug(debug,"module.info");
				memset(cmd,0,sizeof(cmd));
				sprintf(cmd,"uci -q get system.wifi1 | wc -l");
				int led=cmd_recieve_int(cmd);
				if(led==1)
				{
					system("uci delete system.wifi1");
					system("uci delete system.wifi0");
					system("uci commit");
				}
				return 0;
			}
			else if( strstr(tmp_buff,"FG160") )		//广和通模组
			{
				//设定模组标志：fm160
				strcpy(value_module,"fg160");
				if(strlen(net)>0)
				{
					memset(cmd,0,sizeof(cmd));
					sprintf(cmd,"uci -q set modem.@default[0].net='5G'");
					system(cmd);
					memset(cmd,0,sizeof(cmd));
					sprintf(cmd,"uci commit modem");
					system(cmd);
				}
				else
				{
					memset(cmd,0,sizeof(cmd));
					sprintf(cmd,"uci -q set modem.%s.net='5G'",name_config);
					system(cmd);
					memset(cmd,0,sizeof(cmd));
					sprintf(cmd,"uci commit modem");
					system(cmd);
				}
				
				memset(value_ifname,0,sizeof(value_ifname));
				stpcpy(value_ifname,"pcie_mhi0.1");
				
				memset(debug,0,sizeof(debug));
				sprintf(debug,"modelname: Fibocom FG160; interface: %s; AT: %s",value_ifname,path_usb);
				Debug(debug,"module.info");
				memset(cmd,0,sizeof(cmd));
				sprintf(cmd,"uci -q get system.wifi1 | wc -l");
				int led=cmd_recieve_int(cmd);
				if(led==1)
				{
					system("uci delete system.wifi1");
					system("uci delete system.wifi0");
					system("uci commit");
				}
				return 0;
			}
			else if( strstr(tmp_buff,"NL668") )		//广和通模组
			{
				//设定模组标志：nl668
				strcpy(value_module,"nl668");
				if(strlen(net)>0)
				{
					memset(cmd,0,sizeof(cmd));
					sprintf(cmd,"uci -q set modem.@default[0].net='4G'");
					system(cmd);
					memset(cmd,0,sizeof(cmd));
					sprintf(cmd,"uci commit modem");
					system(cmd);
				}
				else
				{
					memset(cmd,0,sizeof(cmd));
					sprintf(cmd,"uci -q set modem.%s.net='4G'",name_config);
					system(cmd);
					memset(cmd,0,sizeof(cmd));
					sprintf(cmd,"uci commit modem");
					system(cmd);
				}
				if(strlen(value_ifname)<=0)
				{
					memset(value_ifname,0,sizeof(value_ifname));
					stpcpy(value_ifname,"usb0");
				}
				memset(debug,0,sizeof(debug));
				sprintf(debug,"modelname: Fibocom NL668; interface: %s; AT: %s",value_ifname,path_usb);
				Debug(debug,"module.info");
				if(!checkACM())
				{
					memset(cmd,0,sizeof(cmd));
					sprintf(cmd,"uci -q get system.wifi1 | wc -l");
					int led=cmd_recieve_int(cmd);
					if(led==1)
					{
						//system("uci delete system.wifi0");
						system("uci delete system.wifi1");
						system("uci commit");
					}
				}
				return 0;
			}
			else if( strstr(tmp_buff,"RM500U") )		//移远5G模组
			{
				//设定模组标志：rm500u
				strcpy(value_module,"rm500u");
				memset(cmd,0,sizeof(cmd));
				sprintf(cmd,"uci -q get system.wifi1 | wc -l");
				int led=cmd_recieve_int(cmd);
				if(led==1)
				{
					system("uci delete system.wifi1");
					system("uci delete system.wifi0");
					system("uci commit");
				}
				return 0;
			}
			else if( strstr(tmp_buff,"RM500Q") )		//移远5G模组
			{
				//设定模组标志：rm500u
				strcpy(value_module,"rm500q");
				memset(debug,0,sizeof(debug));
				sprintf(debug,"modelname: Quectel RG520N; interface: %s; AT: %s",value_ifname,path_usb);
				Debug(debug,"module.info");
				memset(cmd,0,sizeof(cmd));
				sprintf(cmd,"uci -q get system.wifi1 | wc -l");
				int led=cmd_recieve_int(cmd);
				if(led==1)
				{
					system("uci delete system.wifi1");
					system("uci delete system.wifi0");
					system("uci commit");
				}
				return 0;
			}
			else if( strstr(tmp_buff,"RG520N") )		//移远5G模组
			{
				//设定模组标志：rg520n
				strcpy(value_module,"rg520n");
				memset(value_ifname,0,sizeof(value_ifname));
				strcpy(value_ifname,"rmnet_mhi0.1");
				// char time_str[100];
				// get_time(time_str);
				// if(strlen(time_str)>0)
					// set_time(time_str);
				memset(debug,0,sizeof(debug));
				sprintf(debug,"modelname: Quectel RG520N; interface: %s; AT: %s",value_ifname,path_usb);
				Debug(debug,"module.info");
				memset(cmd,0,sizeof(cmd));
				sprintf(cmd,"uci -q get system.wifi1 | wc -l");
				int led=cmd_recieve_int(cmd);
				if(led==1)
				{
					system("uci delete system.wifi1");
					system("uci delete system.wifi0");
					system("uci commit");
				}
				return 0;
			}
			else if( strstr(tmp_buff,"RM520N-CN") )		//移远5G模组
			{
				//设定模组标志：rm520n
				strcpy(value_module,"rg520n-cn");
				memset(value_ifname,0,sizeof(value_ifname));
				//if(isPCIE())
					strcpy(value_ifname,"rmnet_mhi0.1");
				/* else
					strcpy(value_ifname,"usb0"); */
				// char time_str[100];
				// get_time(time_str);
				// if(strlen(time_str)>0)
					// set_time(time_str);
				memset(debug,0,sizeof(debug));
				sprintf(debug,"modelname: Quectel RM520N; interface: %s; AT: %s",value_ifname,path_usb);
				Debug(debug,"module.info");
				memset(cmd,0,sizeof(cmd));
				sprintf(cmd,"uci -q get system.wifi1 | wc -l");
				int led=cmd_recieve_int(cmd);
				if(led==1)
				{
					system("uci delete system.wifi1");
					system("uci delete system.wifi0");
					system("uci commit");
				}
				return 0;
			}
			else if( strstr(tmp_buff,"RM520N-GL") )		//移远5G模组
			{
				//设定模组标志：rm520n
				strcpy(value_module,"rg520n-gl");
				if(isPCIE())
				{
					memset(value_ifname,0,sizeof(value_ifname));
					strcpy(value_ifname,"rmnet_mhi0.1");
				}
				else
				{
					if(strlen(value_ifname)<=0)
					{
						memset(value_ifname,0,sizeof(value_ifname));
						stpcpy(value_ifname,"wwan0_1");
					}
					if(strcmp(value_ifname,"wwan0")==0)
					{
						memset(value_ifname,0,sizeof(value_ifname));
						stpcpy(value_ifname,"wwan0_1");
					}
				}
				memset(debug,0,sizeof(debug));
				sprintf(debug,"modelname: Quectel RM520N; interface: %s; AT: %s",value_ifname,path_usb);
				Debug(debug,"module.info");
				memset(cmd,0,sizeof(cmd));
				sprintf(cmd,"uci -q get system.wifi1 | wc -l");
				int led=cmd_recieve_int(cmd);
				if(led==1)
				{
					system("uci delete system.wifi1");
					system("uci delete system.wifi0");
					system("uci commit");
				}
				return 0;
			}
			else if( strstr(tmp_buff,"EC2") )		//移远4G模组
			{
				//设定模组标志：rg520n
				strcpy(value_module,"ec20");
				if(strstr(tmp_buff,"EC25EUX"))
				{
					memset(value_module,0,sizeof(value_module));
					strcpy(value_module,"ec20-eux");
				}
				if(strstr(tmp_buff,"EC25AF"))
				{
					memset(value_module,0,sizeof(value_module));
					strcpy(value_module,"ec20-af");
				}
					
				if(strlen(net)>0)
				{
					memset(cmd,0,sizeof(cmd));
					sprintf(cmd,"uci -q set modem.@default[0].net='4G'");
					system(cmd);
					if(strstr(board_name,"R620")||strstr(board_name,"R650")||strstr(board_name,"R660"))
					{
						memset(cmd,0,sizeof(cmd));
						sprintf(cmd,"uci -q set system.@system[0].H_ver='V1.1'");
						system(cmd);
					}
					memset(cmd,0,sizeof(cmd));
					sprintf(cmd,"uci commit");
					system(cmd);
				}
				else
				{
					memset(cmd,0,sizeof(cmd));
					sprintf(cmd,"uci -q set modem.%s.net='4G'",name_config);
					system(cmd);
					if(strstr(board_name,"R620")||strstr(board_name,"R650")||strstr(board_name,"R660"))
					{
						memset(cmd,0,sizeof(cmd));
						sprintf(cmd,"uci -q set system.@system[0].H_ver='V1.1'");
						system(cmd);
					}
					memset(cmd,0,sizeof(cmd));
					sprintf(cmd,"uci commit");
					system(cmd);
				}
				if(strlen(value_ifname)<=0)
				{
					memset(value_ifname,0,sizeof(value_ifname));
					strcpy(value_ifname,"usb0");
				}
				memset(debug,0,sizeof(debug));
				sprintf(debug,"modelname: Quectel EC2X; interface: %s; AT: %s",value_ifname,path_usb);
				Debug(debug,"module.info");
				memset(cmd,0,sizeof(cmd));
				sprintf(cmd,"uci -q get system.wifi1 | wc -l");
				int led=cmd_recieve_int(cmd);
				if(led==1)
				{
					system("uci delete system.wifi1");
					//system("uci delete system.wifi0");
					system("uci commit");
				}
				return 0;
			}
			else if( strstr(tmp_buff,"SIM7600") )		//芯讯通4G模组
			{
				//设定模组标志：sim7600
				strcpy(value_module,"sim7600");
				if(strlen(value_ifname)<=0)
				{
					memset(value_ifname,0,sizeof(value_ifname));
					stpcpy(value_ifname,"wwan0");
				}
				memset(debug,0,sizeof(debug));
				sprintf(debug,"modelname: Simcom SIM7600; interface: %s; AT: %s",value_ifname,path_usb);
				Debug(debug,"module.info");
				memset(cmd,0,sizeof(cmd));
				sprintf(cmd,"uci -q get system.wifi1 | wc -l");
				int led=cmd_recieve_int(cmd);
				if(led==1)
				{
					system("uci delete system.wifi1");
					//system("uci delete system.wifi0");
					system("uci commit");
				}
				return 0;
			}
			else if( strstr(tmp_buff,"SIM8200") )		//芯讯通4G模组
			{
				//设定模组标志：sim8200
				strcpy(value_module,"sim8200");
				memset(value_ifname,0,sizeof(value_ifname));
				strcpy(value_ifname,"wwan0_1");
				memset(debug,0,sizeof(debug));
				sprintf(debug,"modelname: Simcom SIM8200; interface: %s; AT: %s",value_ifname,path_usb);
				Debug(debug,"module.info");
				memset(cmd,0,sizeof(cmd));
				sprintf(cmd,"uci -q get system.wifi1 | wc -l");
				int led=cmd_recieve_int(cmd);
				if(led==1)
				{
					system("uci delete system.wifi1");
					system("uci delete system.wifi0");
					system("uci commit");
				}
				return 0;
			}
			else if( strstr(tmp_buff,"MT5700"))		//广和通模组
			{
				//设定模组标志：fm650
				strcpy(value_module,"mt5700");
				if(strlen(net)>0)
				{
					memset(cmd,0,sizeof(cmd));
					sprintf(cmd,"uci -q set modem.@default[0].net='5G'");
					system(cmd);
					memset(cmd,0,sizeof(cmd));
					sprintf(cmd,"uci commit modem");
					system(cmd);
				}
				else
				{
					memset(cmd,0,sizeof(cmd));
					sprintf(cmd,"uci -q set modem.%s.net='5G'",name_config);
					system(cmd);
					memset(cmd,0,sizeof(cmd));
					sprintf(cmd,"uci commit modem");
					system(cmd);
				}
				if(strlen(value_ifname)<=0)
				{
					memset(value_ifname,0,sizeof(value_ifname));
					stpcpy(value_ifname,"eth5");
				}
				memset(debug,0,sizeof(debug));
				sprintf(debug,"modelname: TD MT5700; interface: %s; AT: %s",value_ifname,path_usb);
				Debug(debug,"module.info");
				memset(cmd,0,sizeof(cmd));
				sprintf(cmd,"uci -q get system.wifi1 | wc -l");
				int led=cmd_recieve_int(cmd);
				if(led==1)
				{
					system("uci delete system.wifi1");
					system("uci delete system.wifi0");
					system("uci commit");
				}
				return 0;
			}
			else if( strstr(tmp_buff,"TD Tech Ltd") )
			{
				memset(cmd,0,sizeof(cmd));
				sprintf(cmd,"cmdat %s AT^VERSION?",path_usb);
				memset(tmp_buff,0,sizeof(tmp_buff));
				cmd_recieve_str(cmd,tmp_buff);
				if( strstr(tmp_buff,"MT5711"))		//广和通模组
				{
					//设定模组标志：fm650
					strcpy(value_module,"mt5711");
					if(strlen(net)>0)
					{
						memset(cmd,0,sizeof(cmd));
						sprintf(cmd,"uci -q set modem.@default[0].net='5G'");
						system(cmd);
						memset(cmd,0,sizeof(cmd));
						sprintf(cmd,"uci commit modem");
						system(cmd);
					}
					else
					{
						memset(cmd,0,sizeof(cmd));
						sprintf(cmd,"uci -q set modem.%s.net='5G'",name_config);
						system(cmd);
						memset(cmd,0,sizeof(cmd));
						sprintf(cmd,"uci commit modem");
						system(cmd);
					}
					if(strlen(value_ifname)<=0)
					{
						memset(value_ifname,0,sizeof(value_ifname));
						stpcpy(value_ifname,"usb0");
					}
					memset(debug,0,sizeof(debug));
					sprintf(debug,"modelname: TD MT5711; interface: %s; AT: %s",value_ifname,path_usb);
					Debug(debug,"module.info");
					memset(cmd,0,sizeof(cmd));
					sprintf(cmd,"uci -q get system.wifi1 | wc -l");
					int led=cmd_recieve_int(cmd);
					if(led==1)
					{
						system("uci delete system.wifi1");
						system("uci delete system.wifi0");
						system("uci commit");
					}
					return 0;
				}
				else if( strstr(tmp_buff,"MT5710"))		//广和通模组
				{
					//设定模组标志：fm650
					strcpy(value_module,"mt5711_10");
					if(strlen(net)>0)
					{
						memset(cmd,0,sizeof(cmd));
						sprintf(cmd,"uci -q set modem.@default[0].net='5G'");
						system(cmd);
						memset(cmd,0,sizeof(cmd));
						sprintf(cmd,"uci commit modem");
						system(cmd);
					}
					else
					{
						memset(cmd,0,sizeof(cmd));
						sprintf(cmd,"uci -q set modem.%s.net='5G'",name_config);
						system(cmd);
						memset(cmd,0,sizeof(cmd));
						sprintf(cmd,"uci commit modem");
						system(cmd);
					}
					if(strlen(value_ifname)<=0)
					{
						memset(value_ifname,0,sizeof(value_ifname));
						stpcpy(value_ifname,"usb0");
					}
					memset(debug,0,sizeof(debug));
					sprintf(debug,"modelname: TD MT5710; interface: %s; AT: %s",value_ifname,path_usb);
					Debug(debug,"module.info");
					memset(cmd,0,sizeof(cmd));
					sprintf(cmd,"uci -q get system.wifi1 | wc -l");
					int led=cmd_recieve_int(cmd);
					if(led==1)
					{
						system("uci delete system.wifi1");
						system("uci delete system.wifi0");
						system("uci commit");
					}
					return 0;
				}
			}
			else
				continue;
		}
		else
		{
			sleep(1);
		}
	}
}

/* 功能：设置模组拨号方式
** 返回值：void
*/
void setModule()
{
	char tmp_mode_usb[5];
	char tmp_mode_nat[5];
	if(strstr(value_module,"fm650"))		//广和通模组
	{
		//设定拨号上网驱动
		memset(tmp_mode_usb,0,sizeof(tmp_mode_usb));
		if(strstr(mode_usb,"0"))
			strcpy(tmp_mode_usb,"35");	//ECM
		else if(strstr(mode_usb,"1"))
			strcpy(tmp_mode_usb,"39");	//RNDIS
		else if(strstr(mode_usb,"2"))
			strcpy(tmp_mode_usb,"36");	//NCM
		else
			strcpy(tmp_mode_usb,"36");
		//设定拨号上网方式
		memset(tmp_mode_nat,0,sizeof(tmp_mode_nat));
		if(strstr(mode_nat,"0"))
			strcpy(tmp_mode_nat,"1");	//网卡模式
		else if(strstr(mode_nat,"1"))
			strcpy(tmp_mode_nat,"0");	//路由模式
		else
			strcpy(tmp_mode_nat,"1");
		//设定fm650模组拨号方式
		setModule_FM650(tmp_mode_usb,tmp_mode_nat);
	}
	else if(strstr(value_module,"fm160"))		//广和通模组
	{
		//设定拨号上网驱动
		memset(tmp_mode_usb,0,sizeof(tmp_mode_usb));
		if(strstr(mode_usb,"0"))
			strcpy(tmp_mode_usb,"33");	//ECM
		else if(strstr(mode_usb,"1"))
			strcpy(tmp_mode_usb,"24");	//RNDIS
		else if(strstr(mode_usb,"2"))
			strcpy(tmp_mode_usb,"32");	//NCM
		else
			strcpy(tmp_mode_usb,"33");
		//设定拨号上网方式
		memset(tmp_mode_nat,0,sizeof(tmp_mode_nat));
		if(strstr(mode_nat,"0"))
			strcpy(tmp_mode_nat,"1");	//网卡模式
		else if(strstr(mode_nat,"1"))
			strcpy(tmp_mode_nat,"0");	//路由模式
		else
			strcpy(tmp_mode_nat,"1");
		//设定fm160模组拨号方式
		setModule_FM160(tmp_mode_usb,tmp_mode_nat);
	}
	else if(strstr(value_module,"fg160"))		//广和通模组
	{
		//设定fm160模组拨号方式
		setModule_FG160();
	}
	else if(strstr(value_module,"nl668"))		//广和通模组
	{
		//设定nl668模组拨号方式
		
		//设定拨号上网方式
		memset(tmp_mode_nat,0,sizeof(tmp_mode_nat));
		if(strstr(mode_nat,"0"))
			strcpy(tmp_mode_nat,"1");	//网卡模式
		else if(strstr(mode_nat,"1"))
			strcpy(tmp_mode_nat,"0");	//路由模式
		else
			strcpy(tmp_mode_nat,"1");
		setModule_NL668("18",tmp_mode_nat);
		//setFlightmode();
	}
	else if(strstr(value_module,"rm500u"))	//移远模组
	{
		//设定拨号上网驱动
		memset(tmp_mode_usb,0,sizeof(tmp_mode_usb));
		if(strstr(mode_usb,"0"))
			strcpy(tmp_mode_usb,"1");	//ECM
		else if(strstr(mode_usb,"1"))
			strcpy(tmp_mode_usb,"3");	//RNDIS
		else if(strstr(mode_usb,"2"))
			strcpy(tmp_mode_usb,"5");	//NCM
		else
			strcpy(tmp_mode_usb,"5");
		//设定拨号上网方式
		memset(tmp_mode_nat,0,sizeof(tmp_mode_nat));
		if(strstr(mode_nat,"0"))
			strcpy(tmp_mode_nat,"0");	//网卡模式
		else if(strstr(mode_nat,"1"))
			strcpy(tmp_mode_nat,"1");	//路由模式
		else
			strcpy(tmp_mode_nat,"0");
		setModule_RM500U(tmp_mode_usb,tmp_mode_nat);
	}
	else if(strstr(value_module,"rm500q"))	//移远模组
	{
		//设定拨号上网驱动
		memset(tmp_mode_usb,0,sizeof(tmp_mode_usb));
		if(strstr(mode_usb,"0"))
			strcpy(tmp_mode_usb,"1");	//ECM
		else if(strstr(mode_usb,"1"))
			strcpy(tmp_mode_usb,"3");	//RNDIS
		else if(strstr(mode_usb,"2"))
			strcpy(tmp_mode_usb,"0");	//QMI/gobinet
		else
			strcpy(tmp_mode_usb,"1");
		//设定拨号上网方式
		memset(tmp_mode_nat,0,sizeof(tmp_mode_nat));
		if(strstr(mode_nat,"0"))
			strcpy(tmp_mode_nat,"0");	//网卡模式
		else if(strstr(mode_nat,"1"))
			strcpy(tmp_mode_nat,"1");	//路由模式
		else
			strcpy(tmp_mode_nat,"0");
		setModule_RM500U(tmp_mode_usb,tmp_mode_nat);
	}
	else if(strstr(value_module,"rg520n"))
	{
		setModule_RG520N();
		cmd_recieve_str(cmd_ifup,NULL); 
	}
	else if(strstr(value_module,"ec20"))
	{
		setModule_EC20(); 
	}
	else if(strstr(value_module,"sim8200"))
	{
		setModule_SIM8200();
	}
	else if(strstr(value_module,"sim7600"))
	{
		setModule_SIM7600();
		setFlightmode();
	}
	else if(strstr(value_module,"mt5700"))		//鼎桥模组
	{
		//设定拨号上网驱动
		memset(tmp_mode_usb,0,sizeof(tmp_mode_usb));
		if(strstr(mode_usb,"0"))
			strcpy(tmp_mode_usb,"0");	//ECM
		else if(strstr(mode_usb,"2"))
			strcpy(tmp_mode_usb,"4");	//NCM
		else
			strcpy(tmp_mode_usb,"4");
		//设定拨号上网方式
		memset(tmp_mode_nat,0,sizeof(tmp_mode_nat));
		if(strstr(mode_nat,"0"))
			strcpy(tmp_mode_nat,"0");	//网卡模式-stick
		else if(strstr(mode_nat,"1"))
			strcpy(tmp_mode_nat,"1");	//路由模式-E5
		else
			strcpy(tmp_mode_nat,"0");
		//设定mt5700模组拨号方式
		setModule_MT5700(tmp_mode_usb,tmp_mode_nat);
	}
	else if(strstr(value_module,"mt5711"))		//鼎桥模组
	{
		//设定拨号上网驱动
		memset(tmp_mode_usb,0,sizeof(tmp_mode_usb));
		if(strstr(mode_usb,"0"))
			strcpy(tmp_mode_usb,"0");	//ECM
		else if(strstr(mode_usb,"2"))
			strcpy(tmp_mode_usb,"4");	//NCM
		else
			strcpy(tmp_mode_usb,"4");
		//设定拨号上网方式
		memset(tmp_mode_nat,0,sizeof(tmp_mode_nat));
		if(strstr(mode_nat,"0"))
			strcpy(tmp_mode_nat,"0");	//网卡模式-stick
		else if(strstr(mode_nat,"1"))
			strcpy(tmp_mode_nat,"1");	//路由模式-E5
		else
			strcpy(tmp_mode_nat,"0");
		//设定mt5700模组拨号方式
		setModule_MT5711(tmp_mode_usb,tmp_mode_nat);
	}
	if(flag_dial!=1)
	{
		Debug("dial disable, exit","daemon.info");
		setFlightmode();
		cmd_recieve_str(cmd_ifdown,NULL);
		sleep(1);
		cmd_recieve_str(cmd_ifup,NULL);
		exit(1);
	}
}
/* 功能：读取驻网信息
** 返回值：void
*/
void getModem()
{
	//打印模组驻网信息到/tmp/tmp_modem文件中
	char cmd[1024];
	char tmp_buff[1024];
	//Debug("getModem: send at","daemon.debug");
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
	/* memset(cmd,0,sizeof(cmd));
	sprintf(cmd,"echo \"######%s######\" > %s",value_module,path_outfile);
	system(cmd); */
	//if(count_loop==0)
	{
		//Debug("getIMEI","daemon.debug");
		//获取IMEI
		getIMEI();
		//Debug("getVERS","daemon.debug");
		//获取版本
		getVERS();
	}
	//获取读卡状态
	if(strlen(value_defaultcard)>0)
	{
		//判断是否是新副板
		if(is_card_hotplug())
			getCPIN_2card_hotplug();
		else
			getCPIN_2card();
		//获取当前卡位
		getSLOT();
	}
	else
	{
		if(checkACM())
			getCPIN_acm();
		else
			getCPIN();
	}
	
	//获取IMSI
	getIMSI();
	
	//获取ICCID
	getICCID();
	
	//获取运营商信息
	getCOPS();
	
	debug_card_info("ready");
	//sleep(5);
	if(strcmp(name_config,"SIM2")!=0)
		pre_Dial();
	
	//获取注册状态
	getCGREG();
	getCEREG();
	if(!strstr(value_module,"sim7600") && !strstr(value_module,"nl668") && !strstr(value_module,"ec20"))
		getC5GREG();


	//获取温度
	//Debug("getTEMP","daemon.debug");
	getTEMP();
	
	//获取信号强度
	getCSQ();
	
	getGPS();
	
	//解析模组所处网络小区信息
	if(strstr(value_module,"sim7600"))		//芯讯通4G模组
		analyticCPSI_SIM7600();
	else if(strstr(value_module,"sim8200"))		//芯讯通5G模组
		analyticCPSI_SIM8200();
	else if(strstr(value_module,"fm650") || strstr(value_module,"fm160") || strstr(value_module,"fg160") || strstr(value_module,"nl668"))		//广和通模组
		analyticINFO_FM650();
	else if(strstr(value_module,"rm500u") || strstr(value_module,"rm500q"))		//移远模组
		analyticQENG_RM500U();
	else if(strstr(value_module,"rg520n"))		//移远模组
		analyticQENG_RM500U();
	else if(strstr(value_module,"ec20"))		//移远模组
		analyticQENG_RM500U();
	else if(strstr(value_module,"mt5700") || strstr(value_module,"mt5711"))		//海思模组
		analyticINFO_MT5700();
	
	//将/tmp/tmp_mdoem文件复制到/tmp/modem文件中
	/* memset(cmd,0,sizeof(cmd));
	sprintf(cmd,"cat %s > %s",path_outfile,path_formfile);
	system(cmd); */
	//cmd_recieve_str(cmd,NULL);
}

void getVERS()
{
	char tmp_buff[1024];
	char tmp_buff1[1024];
	char cmd[1024];
	int count=0;
	memset(cmd,0,sizeof(cmd));
	memset(tmp_buff,0,sizeof(tmp_buff));
	memset(tmp_buff1,0,sizeof(tmp_buff1));
	if(strcmp(name_config,"SIM")!=0)
	{
		sprintf(cmd,"uci -q get system.@system[0].%s_version",name_config);
	}
	else
	{
		sprintf(cmd,"uci -q get system.@system[0].module_version");
	}
	cmd_recieve_str(cmd,tmp_buff);
	
	if(strcmp(name_config,"SIM")!=0)
	{
		sprintf(cmd,"uci -q get system.@system[0].%s_name",name_config);
	}
	else
	{
		sprintf(cmd,"uci -q get system.@system[0].module_name");
	}
	cmd_recieve_str(cmd,tmp_buff1);
	if(strlen(tmp_buff)>0 && strlen(tmp_buff1)>0)
		return;
	while(1)
	{
		if(strstr(value_module,"mt5711"))
		{
			memset(cmd,0,sizeof(cmd));
			memset(tmp_buff,0,sizeof(tmp_buff));
			sprintf(cmd,"cmdat %s \"AT^VERSION?\" > %s",path_usb,path_at);
			cmd_recieve_str(cmd,NULL);
			sprintf(cmd,"cat %s | grep \"VERSION:V\" | awk -F ':' '{print $2}'",path_at);
			cmd_recieve_str(cmd,tmp_buff);
			if(tmp_buff != NULL && strlen(tmp_buff)>0)
			{
				if(strcmp(name_config,"SIM")!=0)
				{
					sprintf(cmd,"uci set system.@system[0].%s_version=\"%s\"",name_config,tmp_buff);
				}
				else
				{
					sprintf(cmd,"uci set system.@system[0].module_version=\"%s\"",tmp_buff);
				}
				system(cmd);
				
				//读取型号
				memset(cmd,0,sizeof(cmd));
				memset(tmp_buff,0,sizeof(tmp_buff));
				sprintf(cmd,"cat %s | grep \"VERSION:EXTU\" | awk -F ':' '{print $3}'",path_at);
				cmd_recieve_str(cmd,tmp_buff); 
				memset(cmd,0,sizeof(cmd));
				char *buff;
				buff = malloc(sizeof(tmp_buff));
				strcpy(buff,tmp_buff);
				replacestr(buff,"MT","");
				
				memset(cmd,0,sizeof(cmd));
				
				if(strcmp(name_config,"SIM")!=0)
				{
					//Debug("daemon.debug",name_config);
					sprintf(cmd,"uci set system.@system[0].%s_name=\"%s\"",name_config,buff);
				}
				else
				{
					sprintf(cmd,"uci set system.@system[0].module_name=\"%s\"",buff);
				}
				system(cmd);
				//Debug(cmd,"daemon.debug");
				system("uci commit system");
				break;
			}
		}
		else
		{
			//获取版本号，最多10次
			memset(cmd,0,sizeof(cmd));
			memset(tmp_buff,0,sizeof(tmp_buff));
			sprintf(cmd,"cmdat %s \"ATI\" > %s",path_usb,path_at);
			cmd_recieve_str(cmd,NULL);
			sprintf(cmd,"cat %s | grep \"Revision\" | awk -F ' ' '{print $2}'",path_at);
			cmd_recieve_str(cmd,tmp_buff);
			//判断结果是否为空
			if(tmp_buff != NULL && strlen(tmp_buff)>0)
			{	//读取版本
				/* memset(cmd,0,sizeof(cmd));
				sprintf(cmd,"echo \"+VERS: %s\" >> %s",tmp_buff,path_outfile);
				cmd_recieve_str(cmd,NULL); */
				memset(cmd,0,sizeof(cmd));
				if(strstr(value_module,"rg520n"))
				{
					memset(cmd,0,sizeof(cmd));
					memset(tmp_buff,0,sizeof(tmp_buff));
					sprintf(cmd,"cmdat %s AT+QGMR | grep \"520N\" | sed -n 1p | awk -F ' ' '{print$1}'",path_usb);
					cmd_recieve_str(cmd,tmp_buff);
				}
				if(strcmp(name_config,"SIM")!=0)
				{
					sprintf(cmd,"uci set system.@system[0].%s_version=\"%s\"",name_config,tmp_buff);
				}
				else
				{
					sprintf(cmd,"uci set system.@system[0].module_version=\"%s\"",tmp_buff);
				}
				//printf("%s\n"cmd);
				system(cmd);
				
				//读取型号
				memset(cmd,0,sizeof(cmd));
				memset(tmp_buff,0,sizeof(tmp_buff));
				sprintf(cmd,"cat %s | grep \"Model\" | awk -F ' ' '{print $2}'",path_at);
				cmd_recieve_str(cmd,tmp_buff); 
				memset(cmd,0,sizeof(cmd));
				char *buff;
				buff = malloc(sizeof(tmp_buff));
				strcpy(buff,tmp_buff);
				if(strstr(value_module,"fm650") || strstr(value_module,"fm160"))
					replacestr(buff,"FM","");
				else if( strstr(value_module,"fg160"))
					replacestr(buff,"F","");
				else if(strstr(value_module,"nl668"))
					replacestr(buff,"NL","");
				else if(strstr(value_module,"sim8200"))
					replacestr(buff,"SIMCOM_SIM","");
				else if(strstr(value_module,"sim7600"))
					replacestr(buff,"SIMCOM_SIM","");
				else if(strstr(value_module,"mt5700"))
					replacestr(buff,"MT","");
				else if(strstr(value_module,"rm500u"))
				{
					sprintf(cmd,"cat %s | grep \"RM500U\" | sed -n 1p",path_at);
					cmd_recieve_str(cmd,buff); 
				}
				else if(strstr(value_module,"rm500q"))
				{
					sprintf(cmd,"cat %s | grep \"RM500Q\" | sed -n 1p",path_at);
					cmd_recieve_str(cmd,buff); 
				}
					
				else if(strstr(value_module,"rg520n"))
				{
					sprintf(cmd,"cat %s | grep \"520N\" | sed -n 1p | awk -F ' ' '{print$1}'",path_at);
					cmd_recieve_str(cmd,buff);
				}
					
				else if(strstr(value_module,"ec20"))
				{
					sprintf(cmd,"cat %s | grep \"EC2\" | sed -n 1p | awk -F ' ' '{print$1}'",path_at);
					cmd_recieve_str(cmd,buff); 
				}
				
				// if(strstr(value_module,"fm650") || strstr(value_module,"fm160"))
					// replacestr(buff,"FM","SLTX");
				// else if(strstr(value_module,"sim8200"))
					// replacestr(buff,"SIMCOM_SIM","SLTX");
				// else if(strstr(value_module,"sim7600"))
					// replacestr(buff,"SIMCOM_SIM","SLTX");
				// else if(strstr(value_module,"rm500u"))
					// strcat(buff,"SLTX500U-CN");
				// else if(strstr(value_module,"rm500q"))
					// strcat(buff,"SLTX500Q-GL");
				/* memset(cmd,0,sizeof(cmd));			
				sprintf(cmd,"echo \"+MODEL: %s\" >> %s",buff,path_outfile);
				system(cmd); */
				
				memset(cmd,0,sizeof(cmd));
				
				if(strcmp(name_config,"SIM")!=0)
				{
					//Debug("daemon.debug",name_config);
					sprintf(cmd,"uci set system.@system[0].%s_name=\"%s\"",name_config,buff);
				}
				else
				{
					sprintf(cmd,"uci set system.@system[0].module_name=\"%s\"",buff);
				}
				system(cmd);
				//Debug(cmd,"daemon.debug");
				system("uci commit system");
				break;
			}
		}
		if(count==9)
			break;
		count++;
	}
}

/* 功能：获取温度
** 返回值：void
*/
void getTEMP()
{
	char tmp_buff[50];
	char cmd[1024];
	if(strstr(value_module,"fm650") || strstr(value_module,"fm160") || strstr(value_module,"fg160") || strstr(value_module,"nl668"))
	{
		memset(cmd,0,sizeof(cmd));
		memset(tmp_buff,0,sizeof(tmp_buff));
		sprintf(cmd,"cmdat %s 'AT+MTSM=1' | grep \"+MTSM:\" | awk -F ' ' '{print $2}'",path_usb);
		cmd_recieve_str(cmd,tmp_buff);
		uci_cellular("TEMP",tmp_buff);
		/* memset(cmd,0,sizeof(cmd));
		sprintf(cmd,"echo \"+TEMP: %s\" >> %s",tmp_buff,path_outfile);
		cmd_recieve_str(cmd,NULL); */
		uci_cellular("TEMP",tmp_buff);
	}
	else if (strstr(value_module,"sim8200") || strstr(value_module,"sim7600"))
	{
		memset(cmd,0,sizeof(cmd));
		memset(tmp_buff,0,sizeof(tmp_buff));
		sprintf(cmd,"cmdat %s 'AT+CPMUTEMP' | grep \"+CPMUTEMP:\" | awk -F ' ' '{print $2}'",path_usb);
		cmd_recieve_str(cmd,tmp_buff);
		/* memset(cmd,0,sizeof(cmd));
		sprintf(cmd,"echo \"+TEMP: %s\" >> %s",tmp_buff,path_outfile);
		cmd_recieve_str(cmd,NULL); */
		uci_cellular("TEMP",tmp_buff);
	}
	else if(strstr(value_module,"rm500u"))
	{
		memset(cmd,0,sizeof(cmd));
		memset(tmp_buff,0,sizeof(tmp_buff));
		sprintf(cmd,"cmdat %s 'AT+QTEMP' | grep \"soc-thermal\" | awk -F '\"' '{print $4}'",path_usb);
		cmd_recieve_str(cmd,tmp_buff);
		/* memset(cmd,0,sizeof(cmd));
		sprintf(cmd,"echo \"+TEMP: %s\" >> %s",tmp_buff,path_outfile);
		cmd_recieve_str(cmd,NULL); */
		uci_cellular("TEMP",tmp_buff);
	}
	else if(strstr(value_module,"rm500q"))
	{
		memset(cmd,0,sizeof(cmd));
		memset(tmp_buff,0,sizeof(tmp_buff));
		sprintf(cmd,"cmdat %s 'AT+QTEMP' | grep \"cpu0-a7-usr\" | awk -F '\"' '{print $4}'",path_usb);
		cmd_recieve_str(cmd,tmp_buff);
		/* memset(cmd,0,sizeof(cmd));
		sprintf(cmd,"echo \"+TEMP: %s\" >> %s",tmp_buff,path_outfile);
		cmd_recieve_str(cmd,NULL); */
		uci_cellular("TEMP",tmp_buff);
	}
	else if(strstr(value_module,"rg520n"))
	{
		memset(cmd,0,sizeof(cmd));
		memset(tmp_buff,0,sizeof(tmp_buff));
		sprintf(cmd,"cmdat %s 'AT+QTEMP' | grep \"cpuss-0-usr\" | awk -F '\"' '{print $4}'",path_usb);
		cmd_recieve_str(cmd,tmp_buff);
	/* 	memset(cmd,0,sizeof(cmd));
		sprintf(cmd,"echo \"+TEMP: %s\" >> %s",tmp_buff,path_outfile);
		cmd_recieve_str(cmd,NULL); */
		uci_cellular("TEMP",tmp_buff);
	}
	else if(strstr(value_module,"mt5700"))
	{
		memset(cmd,0,sizeof(cmd));
		memset(tmp_buff,0,sizeof(tmp_buff));
		sprintf(cmd,"cmdat %s AT^CHIPTEMP? | grep 'CHIPTEMP: ' | awk -F ',' '{print$9}'",path_usb);
		cmd_recieve_str(cmd,tmp_buff);
		float temp=atof(tmp_buff);
		temp=(float)temp/10;
		memset(tmp_buff,0,sizeof(tmp_buff));
		sprintf(tmp_buff,"%.1f",temp);
		uci_cellular("TEMP",tmp_buff);
	}
	else if(strstr(value_module,"mt5711"))
	{
		memset(cmd,0,sizeof(cmd));
		memset(tmp_buff,0,sizeof(tmp_buff));
		sprintf(cmd,"cmdat %s AT^TDCHIPTEMP? | grep 'CHIPTEMP: ' | awk -F ',' '{print$8}'",path_usb);
		cmd_recieve_str(cmd,tmp_buff);
		/* float temp=atof(tmp_buff);
		temp=(float)temp/10;
		memset(tmp_buff,0,sizeof(tmp_buff));
		sprintf(tmp_buff,"%.1f",temp); */
		uci_cellular("TEMP",tmp_buff);
	}
}

void  getSLOT()
{
	char tmp_buff[1024];
	char cmd[1024];
	int value_currentcard=1;
	//printf("0:value_currentcard:%d\r\n",value_currentcard);
	if(strstr(board_name,"R650"))
	{
		sprintf(cmd,"cmdat %s 'AT+GTDUALSIM?' | grep \"+GTDUALSIM:\" | awk -F ',' '{print $1}' | awk -F ' ' '{print $2}'",path_usb);
		value_currentcard = cmd_recieve_int(cmd);
	}
	else if(strstr(board_name,"R660"))
	{
		sprintf(cmd,"cat /sys/class/gpio/sim_sw/value"); //获取当前卡槽
		value_currentcard = cmd_recieve_int(cmd); 
	}
	else if(strcmp(board_name,"R680")==0)
	{
		if(is_card_hotplug())
		{
			sprintf(cmd,"cat /sys/class/leds/sim_sw/brightness"); //获取当前卡槽
			value_currentcard = cmd_recieve_int(cmd); 
		}
		else
		{
			if(strstr(value_module,"nl668"))
			{
				sprintf(cmd,"cat /sys/class/leds/sim_sw/brightness"); //获取当前卡槽
				value_currentcard = cmd_recieve_int(cmd); 
			}
			else if(strstr(value_module,"mt5700"))
			{
				sprintf(cmd,"cmdat %s \"AT^SCICHG?\" | grep \"SCICHG:\" | awk -F ' ' '{print$2}' | awk -F ',' '{print$1}'",path_usb);
				value_currentcard = cmd_recieve_int(cmd);
			}
			else if(strstr(value_module,"mt5711"))
			{
				sprintf(cmd,"cmdat %s AT^SIMSWITCH? | grep SIMSWITCH: | awk -F ':' '{print$2}' | awk -F ' ' '{print$1}'",path_usb);
				value_currentcard = cmd_recieve_int(cmd);
			}
			else if(strstr(value_module,"rg520n"))
			{
				sprintf(cmd,"cmdat %s 'AT+QUIMSLOT?' | grep +QUIMSLOT: | awk -F ' ' '{print $2}'",path_usb);
				
				value_currentcard = cmd_recieve_int(cmd);
				//printf("1:value_currentcard:%d\r\n",value_currentcard);
			}
			else
			{
				sprintf(cmd,"cmdat %s 'AT+GTDUALSIM?' | grep \"+GTDUALSIM:\" | awk -F ',' '{print $1}' | awk -F ' ' '{print $2}'",path_usb);
				value_currentcard = cmd_recieve_int(cmd);
			}
		}
	}
	else
	{
		sprintf(cmd,"cat /sys/class/leds/sim_sw/brightness"); //获取当前卡槽
		value_currentcard = cmd_recieve_int(cmd); 
	}
	//printf("2:value_currentcard:%d\r\n",value_currentcard);
	if(value_currentcard==255)
		value_currentcard=1;
	//printf("3:value_currentcard:%d\r\n",value_currentcard);
	if(strstr(board_name,"R690") || strstr(board_name,"R660"))
	{
		if(value_currentcard==0)
			value_currentcard=2;
	}
	else if(strstr(board_name,"RT990"))
	{
		if(strstr(value_module,"rg520n"))
		{
			//if(isPCIE())
			{
				value_currentcard++;
			}
			//else
				//value_currentcard++;
		}
		if(strstr(value_module,"fg160"))
		{
			//if(isPCIE())
			{
				value_currentcard++;
			}
			//else
				//value_currentcard++;
		}
		else
		{
			if(value_currentcard==0)
				value_currentcard=2;
		}
	}
	else if(strstr(board_name,"R650"))
	{
		value_currentcard++;
	}
	else if(strstr(board_name,"R680"))
	{
		//printf("4:value_currentcard:%d\r\n",value_currentcard);
		if(is_card_hotplug())
		{
			if(value_currentcard==0)
				value_currentcard=2;
		}
		else
		{
			//printf("5:value_currentcard:%d\r\n",value_currentcard);
			if(strstr(value_module,"nl668") || strstr(board_name,"R680-8TH"))
			{
				if(value_currentcard==0)
					value_currentcard=2;
			}
			else if(strstr(value_module,"rg520n"))
			{
				//printf("6:value_currentcard:%d\r\n",value_currentcard);
				//value_currentcard=value_currentcard;
			}
			else
			{
				value_currentcard++;
			}
		}
	}
	uci_cellular_int("SLOT",value_currentcard);
	/* memset(cmd,0,sizeof(cmd));
	sprintf(cmd,"echo \"+SLOT: %d\" >> %s",value_currentcard,path_outfile);
	cmd_recieve_str(cmd,NULL); */
}
/* 功能：获取IMEI
** 返回值：void
*/
void getIMEI()
{
	char tmp_buff[1024];
	char cmd[1024];
	int count=0;
	while(1)
	{
		//获取IMEI，最多10次
		memset(cmd,0,sizeof(cmd));
		memset(tmp_buff,0,sizeof(tmp_buff));
		if(strcmp(value_module,"mt5711")==0)
			sprintf(cmd,"cmdat %s \"AT+CGSN\" | sed -n '2p' | awk -F ' ' '{print$2}'",path_usb);
		else
			sprintf(cmd,"cmdat %s \"AT+CGSN\" | sed -n '2p' | awk -F ' ' '{print$1}'",path_usb);
		cmd_recieve_str(cmd,tmp_buff); 
		//printf("IMEI:%s\n",tmp_buff);
		//判断结果是否为空或符合IMEI标准
		if(tmp_buff != NULL && isInt(tmp_buff)==0)
		{
			uci_cellular("IMEI",tmp_buff);
			/* memset(cmd,0,sizeof(cmd));
			sprintf(cmd,"echo \"+IMEI: %s\" >> %s",tmp_buff,path_outfile);
			cmd_recieve_str(cmd,NULL); */
			break;
		}
		if(count==9)
			break;
		count++;
	}
}
/* 功能：从/tmp/tmp_modem文件中移除上一次读卡记录
** 返回值：void
*/
void removeCPIN()
{
	char cmd[1024];
	memset(cmd,0,sizeof(cmd));
	sprintf(cmd,"sed -i '/+CPIN:/d' %s",path_outfile);
	cmd_recieve_str(cmd,NULL);
}
/* 功能：切换飞行模式
** 返回值：void
*/
void setFlightmode()
{
	char cmd[1024];
	//切换到飞行模式
	memset(cmd,0,sizeof(cmd));
	if(strstr(value_module,"fm650"))	//广和通模组
	{
		sprintf(cmd,"cmdat %s \"AT+CFUN=4\" > /dev/null 2>&1",path_usb);
		cmd_recieve_str(cmd,NULL);
	}else{								//其他模组
		sprintf(cmd,"cmdat %s \"AT+CFUN=0\" > /dev/null 2>&1",path_usb);
		cmd_recieve_str(cmd,NULL);
	}
	sleep(1);
	//切换到正常通讯模式
	memset(cmd,0,sizeof(cmd));
	sprintf(cmd,"cmdat %s \"AT+CFUN=1\" > /dev/null 2>&1",path_usb);
	cmd_recieve_str(cmd,NULL);
	sleep(1);
	if(strstr(value_module,"fm160") || strstr(value_module,"fg160"))
	{
		//sleep(1);
		if(strcmp(board_name,"R680")==0)
		{
			//sleep(1);
			memset(cmd,0,sizeof(cmd));
			sprintf(cmd,"cmdat %s \"AT+CFUN=1\" > /dev/null 2>&1",path_usb);
			cmd_recieve_str(cmd,NULL);
			sleep(4);
		}
	}
}

void setFlightmode_first_reboot()
{
	char cmd[1024];
	//切换到飞行模式
	memset(cmd,0,sizeof(cmd));
	if(strstr(value_module,"fm650"))	//广和通模组
	{
		sprintf(cmd,"cmdat %s \"AT+CFUN=4\" > /dev/null 2>&1",path_usb);
		cmd_recieve_str(cmd,NULL);
	}else{								//其他模组
		sprintf(cmd,"cmdat %s \"AT+CFUN=0\" > /dev/null 2>&1",path_usb);
		cmd_recieve_str(cmd,NULL);
	}
	usleep(500);
	//切换到正常通讯模式
	memset(cmd,0,sizeof(cmd));
	sprintf(cmd,"cmdat %s \"AT+CFUN=1\" > /dev/null 2>&1",path_usb);
	cmd_recieve_str(cmd,NULL);
	usleep(2000);
}

void setFlightmode2()
{
	char cmd[1024];
	//切换到飞行模式
	memset(cmd,0,sizeof(cmd));
	sprintf(cmd,"cmdat %s \"AT+CFUN=1,1\" > /dev/null 2>&1",path_usb);
	cmd_recieve_str(cmd,NULL);
	sleep(180);
}

/* 功能：硬复位
** 返回值：void
*/
void resetModem()
{
	char cmd[1024];
	//切换到飞行模式
	memset(cmd,0,sizeof(cmd));
	if(strstr(value_module,"fm650")||strstr(value_module,"fm160")||strstr(value_module,"nl668"))	//广和通模组
	{
		sprintf(cmd,"cmdat %s \"AT+CFUN=15\" > /dev/null 2>&1",path_usb);
		cmd_recieve_str(cmd,NULL);
	}else if(strstr(value_module,"sim8200")||strstr(value_module,"sim7600")){						//SIMCOM模组
		sprintf(cmd,"cmdat %s \"AT+CRESET\" > /dev/null 2>&1",path_usb);
		cmd_recieve_str(cmd,NULL);
	}
	else if(strstr(value_module,"mt5700") || strstr(value_module,"mt5711")){															//海思模组
		sprintf(cmd,"cmdat %s \"AT+CFUN=8\" > /dev/null 2>&1",path_usb);
		cmd_recieve_str(cmd,NULL);
	}
	sleep(60);
	wait_for_modem_ready();
	if(strstr(value_module,"fm650"))
	{
		setagainFM650();	
	}
}
/* 功能：获取读卡状态
** 返回值：void
*/
void getCPIN()
{
	char tmp_buff[1024];
	char cmd[1024];
	int count=0;
	while(1)
	{
		memset(cmd,0,sizeof(cmd));
		sprintf(cmd,"cmdat %s 'AT' | grep OK |wc -l",path_usb);
		int tmp_int_buff = cmd_recieve_int(cmd);
		if (tmp_int_buff == 1)
			break;
		//sleep(1);
	}
	while(1)
	{
		memset(cmd,0,sizeof(cmd));
		memset(tmp_buff,0,sizeof(tmp_buff));
		sprintf(cmd,"cmdat %s 'AT+CPIN?' | grep \"+CPIN:\"",path_usb);
		cmd_recieve_str(cmd,tmp_buff);
		
		if(strstr(tmp_buff,"READY"))	//读卡，打印结果到/tmp/tmp_mdoem文件，跳出循环
		{
			//removeCPIN();
			uci_cellular("CPIN","READY");
			/* memset(cmd,0,sizeof(cmd));
			sprintf(cmd,"echo \"%s\" >> %s",tmp_buff,path_outfile);
			cmd_recieve_str(cmd,NULL); */
			break;
		}
		else if(strstr(tmp_buff,"SIM PIN"))
		{
			//removeCPIN();
			uci_cellular("CPIN","SIM PIN");
			clear_default_cellular_info();
			/* memset(cmd,0,sizeof(cmd));
			sprintf(cmd,"echo \"%s\" >> %s",tmp_buff,path_outfile);
			cmd_recieve_str(cmd,NULL); */
			debug_card_info("pin");
			setPINCode(value_pin);
		}else{
			/* if(count==1)
			{
				memset(cmd,0,sizeof(cmd));
				sprintf(cmd,"cmdat %s \"AT+CFUN=1\" > /dev/null 2>&1",path_usb);
				cmd_recieve_str(cmd,NULL);
			} */
			if(count==4)		//第4次不读卡，打印结果到/tmp/tmp_mdoem文件
			{
				//removeCPIN();
				uci_cellular("CPIN","SIM card not inserted");
				uci_cellular("CCID"," ");
				clear_default_cellular_info();
				/* memset(cmd,0,sizeof(cmd));
				sprintf(cmd,"echo \"+CPIN: SIM card not inserted\" >> %s",path_outfile);
				cmd_recieve_str(cmd,NULL); */
				cmd_recieve_str(cmd_ifdown,NULL);
				cmd_recieve_str(cmd_ifup,NULL);
				//将/tmp/tmp_mdoem文件复制到/tmp/modem文件中
				/* memset(cmd,0,sizeof(cmd));
				sprintf(cmd,"cat %s > %s",path_outfile,path_formfile);
				cmd_recieve_str(cmd,NULL); */
				debug_card_info("unplug");
				//信号灯控制
				controlLED();
			}
			else if(count==9) 	//飞行模式
			{
				setFlightmode();
				
				if(strstr(value_module,"rg520n"))
					cmd_recieve_str(cmd_ifup,NULL);
				else if(strstr(value_module,"mt5700"))
						if(strcmp(board_name,"R680")!=0)
							ActiveSIM_MT5700();
				else if(strstr(value_module,"mt5711"))
					setFlightmode_MT5711();
				count=-1;
			}
			sleep(2);
		}
		count++;
		getGPS(); //在这里读GPS信号，即时不插卡也不影响。
		if(flag_heartbeat==1)
			heartbeat();
	}
}

/* 功能：获取读卡状态
** 返回值：void
*/
void getCPIN_2card()
{
	char tmp_buff[1024];
	char cmd[1024];
	int count=0;
	//确认模组正常工作
	//Debug("getCPIN_2card:AT","daemon.debug");
	while(1)
	{
		memset(cmd,0,sizeof(cmd));
		sprintf(cmd,"cmdat %s 'AT' | grep OK |wc -l",path_usb);
		int tmp_int_buff = cmd_recieve_int(cmd);
		if (tmp_int_buff == 1)
			break;
	}
	//Debug("getCPIN_2card","daemon.debug");
	while(1)
	{
		printf("%d\n",count);
		memset(cmd,0,sizeof(cmd));
		memset(tmp_buff,0,sizeof(tmp_buff));
		sprintf(cmd,"cmdat %s 'AT+CPIN?' | grep \"+CPIN:\"",path_usb);
		cmd_recieve_str(cmd,tmp_buff);
		
		if(strstr(tmp_buff,"READY"))	//读卡，打印结果到/tmp/tmp_mdoem文件，跳出循环
		{
			uci_cellular("CPIN","READY");
			/* if(strstr(value_module,"nl668"))
			{
				memset(cmd,0,sizeof(cmd));
				sprintf(cmd,"ifconfig %s up",value_ifname);
				system(cmd);
				Debug(cmd,"daemon.debug");
			} */
			break;
		}
		else if(strstr(tmp_buff,"SIM PIN"))
		{
			uci_cellular("CPIN","SIM PIN");
			clear_default_cellular_info();
			
			debug_card_info("pin");
			memset(cmd,0,sizeof(cmd));
			if(strstr(value_module,"fm650") || strstr(value_module,"fm160"))
			{
				if(strstr(board_name,"R650") || strcmp(board_name,"R680")==0)
					sprintf(cmd,"cmdat %s 'AT+GTDUALSIM?' | grep \"+GTDUALSIM:\" | awk -F ',' '{print $1}' | awk -F ' ' '{print $2}'",path_usb); //获取当前卡槽
				else if(strstr(board_name,"R660"))
					sprintf(cmd,"cat /sys/class/gpio/sim_sw/value");
				else
					sprintf(cmd,"cat /sys/class/leds/sim_sw/brightness");
				int value_currentcard = cmd_recieve_int(cmd); 
				if (value_currentcard == 255)
					value_currentcard = 1;
				if(strstr(board_name,"R690") || strstr(board_name,"R680-8TH") || strstr(board_name,"R660") || strstr(board_name,"RT990"))
				{
					if(value_currentcard == 0) 			//[当前卡槽] 0——SIM2
						setPINCode(value_pin_card2);
					else if(value_currentcard == 1)		//[当前卡槽] 1——SIM1
						setPINCode(value_pin_card1);
				}
				else if (strstr(board_name,"R650") || strcmp(board_name,"R680")==0)
				{
					if(value_currentcard == 0) 			//[当前卡槽] 0——SIM1
						setPINCode(value_pin_card1);
					else if(value_currentcard == 1)		//[当前卡槽] 1——SIM2
						setPINCode(value_pin_card2);
				}
			}
			else if(strstr(value_module,"fg160"))
			{
				sprintf(cmd,"cat /sys/class/leds/sim_sw/brightness");
				int value_currentcard = cmd_recieve_int(cmd); 
				if (value_currentcard == 255)
					value_currentcard = 1;
				//if(strstr(board_name,"R690") || strstr(board_name,"R680-8TH") || strstr(board_name,"R660") || strstr(board_name,"RT990"))
				{
					if(value_currentcard == 0) 			//[当前卡槽] 0——SIM1
						setPINCode(value_pin_card1);
					else if(value_currentcard == 1)		//[当前卡槽] 1——SIM2
						setPINCode(value_pin_card2);
				}
			}
			else if(strstr(value_module,"nl668"))
			{
				if(strstr(board_name,"R660"))
					sprintf(cmd,"cat /sys/class/gpio/sim_sw/value");
				else
					sprintf(cmd,"cat /sys/class/leds/sim_sw/brightness");
				int value_currentcard = cmd_recieve_int(cmd); 
				if (value_currentcard == 255)
					value_currentcard = 1;
				if(strstr(board_name,"R680") || strstr(board_name,"RT990") || strstr(board_name,"R660"))
				{
					if(value_currentcard == 0) 			//[当前卡槽] 0——SIM2
						setPINCode(value_pin_card2);
					else if(value_currentcard == 1)		//[当前卡槽] 1——SIM1
						setPINCode(value_pin_card1);
				}
			}
			else if(strstr(value_module,"ec20"))
			{
				if(strstr(board_name,"R660"))
					sprintf(cmd,"cat /sys/class/gpio/sim_sw/value");
				else
					sprintf(cmd,"cat /sys/class/leds/sim_sw/brightness");
				int value_currentcard = cmd_recieve_int(cmd); 
				if (value_currentcard == 255)
					value_currentcard = 1;
				if(strstr(board_name,"R680") || strstr(board_name,"R660") || strstr(board_name,"RT990"))
				{
					if(value_currentcard == 0) 			//[当前卡槽] 0——SIM2
						setPINCode(value_pin_card2);
					else if(value_currentcard == 1)		//[当前卡槽] 1——SIM1
						setPINCode(value_pin_card1);
				}
			}
		}
		else if(strstr(tmp_buff,"BUSY") || strstr(tmp_buff,"busy"))
			continue;
		else
		{
			while(1)
			{
				memset(cmd,0,sizeof(cmd));
				sprintf(cmd,"cmdat %s 'AT' | grep OK |wc -l",path_usb);
				int tmp_int_buff = cmd_recieve_int(cmd);
				if (tmp_int_buff == 1)
					break;
			}
			/* if(!strstr(value_module,"fm160"))
				if(count==0 || count==6)
				{
					memset(cmd,0,sizeof(cmd));
					sprintf(cmd,"cmdat %s \"AT+CFUN=1\" > /dev/null 2>&1",path_usb);
					cmd_recieve_str(cmd,NULL);
				} */
			if(count==4 || count==9)		//第1次不读卡，打印结果到/tmp/tmp_mdoem文件
			{
				memset(cmd,0,sizeof(cmd));
				sprintf(cmd,"print unplug %d",count);
				//Debug(cmd,"daemon.debug");
				//removeCPIN();
				uci_cellular("CPIN","SIM card not inserted");
				uci_cellular("CCID"," ");
				clear_default_cellular_info();
				if(strstr(value_module,"nl668"))
				{
					memset(cmd,0,sizeof(cmd));
					sprintf(cmd,"ifconfig %s down",value_ifname);
					system(cmd);
					//Debug(cmd,"daemon.debug");
				}
				else
				{
					cmd_recieve_str(cmd_ifdown,NULL);
					cmd_recieve_str(cmd_ifup,NULL);
				}
				debug_card_info("unplug");
				//信号灯控制
				controlLED();
			}
			if(strstr(value_defaultcard,"auto"))
			{
				
				if(count==4 || count==1) 	//第5次不读卡，切换SIM卡
				{
					memset(cmd,0,sizeof(cmd));
					sprintf(cmd,"switch to sim2 %d",count);
					//Debug(cmd,"daemon.debug");
					//修改当前读卡位置
					if(strstr(value_module,"fm650") || strstr(value_module,"fm160"))
						SwitchSIM_FM650(false);
					else if(strstr(value_module,"fg160"))
						SwitchSIM_FG160(false);
					else if(strstr(value_module,"nl668"))
						SwitchSIM_NL668(false);
					else if(strstr(value_module,"sim8200"))
						SwitchSIM_SIM8200(false);
					else if(strstr(value_module,"sim7600"))
					{
						SwitchSIM_SIM7600(false);	//sim7600切换卡槽后，需要飞行模式才可读卡
						setFlightmode();
					}
					else if(strstr(value_module,"rm500u") || strstr(value_module,"rm500q"))
						SwitchSIM_RG520N(false);
					else if(strstr(value_module,"rg520n"))
					{
						SwitchSIM_RG520N(false);	//rg520n切换卡槽后，需要飞行模式才可读卡
						cmd_recieve_str(cmd_ifup,NULL);
					}
					else if(strstr(value_module,"ec20"))
						SwitchSIM_EC20(false);
					else if(strstr(value_module,"mt5700"))
					{
						SwitchSIM_MT5700(false);	//mt5700切换卡槽后，需要飞行模式, 并且激活SIM卡才可读卡
					}
					else if(strstr(value_module,"mt5711") )
					{
						SwitchSIM_MT5711(false);	//mt5700切换卡槽后，需要飞行模式, 并且激活SIM卡才可读卡
					}
					sleep(1);
				}
				else if(count==9 || count==3) 	//第10次不读卡，切换为[优先卡槽]并飞行模式
				{
					memset(cmd,0,sizeof(cmd));
					sprintf(cmd,"switch back sim1 %d",count);
					//Debug(cmd,"daemon.debug");
					//设置[当前卡槽]为[优先卡槽]
					if(strstr(value_module,"fm650") || strstr(value_module,"fm160"))
						SwitchSIM_FM650(true);
					else if(strstr(value_module,"fg160"))
						SwitchSIM_FG160(true);
					else if(strstr(value_module,"nl668"))
						SwitchSIM_NL668(true);
					else if(strstr(value_module,"sim8200"))
						SwitchSIM_SIM8200(true);
					else if(strstr(value_module,"sim7600"))
						SwitchSIM_SIM7600(true);
					else if(strstr(value_module,"rm500u") || strstr(value_module,"rm500q"))
						SwitchSIM_RG520N(false);
					else if(strstr(value_module,"rg520n"))
						SwitchSIM_RG520N(true);
					else if(strstr(value_module,"ec20"))
						SwitchSIM_EC20(true);
					else if(strstr(value_module,"mt5700"))
					{
						SwitchSIM_MT5700(true);
					}
					else if(strstr(value_module,"mt5711"))
					{
						SwitchSIM_MT5711(true);
					}
					
					if(strstr(value_module,"rg520n"))
						cmd_recieve_str(cmd_ifup,NULL);
					
					if(count==9)
						count=-1;
					sleep(1);
				}
				
			}
			else
			{
				if(count==9) 	//第10次不读卡，切换为[优先卡槽]并飞行模式
				{
					setFlightmode();
					if(strstr(value_module,"rg520n"))
						cmd_recieve_str(cmd_ifup,NULL);
					else if(strstr(value_module,"mt5700"))
						if(strcmp(board_name,"R680")!=0)
							ActiveSIM_MT5700();
					else if(strstr(value_module,"mt5711"))
						setFlightmode_MT5711();
					count=-1;
				}
			}
		}
		count++;
		/* memset(cmd,0,sizeof(cmd));
		sprintf(cmd,"end %d",count);
		Debug(cmd,"daemon.debug"); */
		getGPS(); //在这里读GPS信号，即时不插卡也不影响。
		if(flag_heartbeat==1)
			heartbeat();
	}
}

/* 功能：获取读卡状态
** 返回值：void
*/
void getCPIN_2card_hotplug()
{
	char tmp_buff[1024];
	char cmd[1024];
	int count=0;
	
	//注册GPIO29
	memset(cmd,0,sizeof(cmd));
	int result;
	if(!is_openwrt23_05())
	{
		sprintf(cmd,"ls /sys/class/gpio | grep gpio29 | wc -l");
		result=cmd_recieve_int(cmd);
		if(result==0)
		{
			system("echo 29 > /sys/class/gpio/export");
			system("echo in > /sys/class/gpio/gpio29/direction");
		}
	}
	else
	{
		sprintf(cmd,"ls /sys/class/gpio | grep gpio461 | wc -l");
		result=cmd_recieve_int(cmd);
		if(result==0)
		{
			system("echo 461 > /sys/class/gpio/export");
			system("echo in > /sys/class/gpio/gpio461/direction");
		}
	}
	//读取gpio29
	while(1)
	{
		memset(cmd,0,sizeof(cmd));
		if(!is_openwrt23_05())
			sprintf(cmd,"cat /sys/class/gpio/gpio29/value");
		else
			sprintf(cmd,"cat /sys/class/gpio/gpio461/value");
		result=cmd_recieve_int(cmd);
		if(result==255)
			result=1;
		if(result==1)//gpio29说明已插卡
		{
			if(strstr(value_defaultcard,"auto"))
				uci_cellular("CPIN","READY");
			//直接读一次卡
			memset(cmd,0,sizeof(cmd));
			memset(tmp_buff,0,sizeof(tmp_buff));
			sprintf(cmd,"cmdat %s 'AT+CPIN?' | grep \"+CPIN:\"",path_usb);
			cmd_recieve_str(cmd,tmp_buff);
			if(strstr(tmp_buff,"READY")) //读卡
			{
_read_card_READY:
				uci_cellular("CPIN","READY");
				break;
			}
			else if(strstr(tmp_buff,"SIM PIN")) //锁卡
			{
_read_card_PIN:	
				uci_cellular("CPIN","SIM PIN");
				clear_default_cellular_info();
				memset(cmd,0,sizeof(cmd));
				debug_card_info("pin");
				if(strstr(value_module,"fm650") || strstr(value_module,"fm160"))
				{
					if(strstr(board_name,"R650"))
						sprintf(cmd,"cmdat %s 'AT+GTDUALSIM?' | grep \"+GTDUALSIM:\" | awk -F ',' '{print $1}' | awk -F ' ' '{print $2}'",path_usb); //获取当前卡槽
					else if(strstr(board_name,"R660"))
						sprintf(cmd,"cat /sys/class/gpio/sim_sw/value");
					else
						sprintf(cmd,"cat /sys/class/leds/sim_sw/brightness");
					int value_currentcard = cmd_recieve_int(cmd); 
					if (value_currentcard == 255)
						value_currentcard = 1;
					if(strstr(board_name,"R690") || strstr(board_name,"R680") || strstr(board_name,"R660") || strstr(board_name,"RT990"))
					{
						if(value_currentcard == 0) 			//[当前卡槽] 0——SIM2
							setPINCode(value_pin_card2);
						else if(value_currentcard == 1)		//[当前卡槽] 1——SIM1
							setPINCode(value_pin_card1);
					}
					else if (strstr(board_name,"R650"))
					{
						if(value_currentcard == 0) 			//[当前卡槽] 0——SIM1
							setPINCode(value_pin_card1);
						else if(value_currentcard == 1)		//[当前卡槽] 1——SIM2
							setPINCode(value_pin_card2);
					}
				}
				else if(strstr(value_module,"nl668"))
				{
					if(strstr(board_name,"R660"))
						sprintf(cmd,"cat /sys/class/gpio/sim_sw/value");
					else
						sprintf(cmd,"cat /sys/class/leds/sim_sw/brightness");
					int value_currentcard = cmd_recieve_int(cmd); 
					if (value_currentcard == 255)
						value_currentcard = 1;
					if(strstr(board_name,"R680") || strstr(board_name,"RT990") || strstr(board_name,"R660"))
					{
						if(value_currentcard == 0) 			//[当前卡槽] 0——SIM2
							setPINCode(value_pin_card2);
						else if(value_currentcard == 1)		//[当前卡槽] 1——SIM1
							setPINCode(value_pin_card1);
					}
				}
				else if(strstr(value_module,"ec20"))
				{
					if(strstr(board_name,"R660"))
						sprintf(cmd,"cat /sys/class/gpio/sim_sw/value");
					else
						sprintf(cmd,"cat /sys/class/leds/sim_sw/brightness");
					int value_currentcard = cmd_recieve_int(cmd); 
					if (value_currentcard == 255)
						value_currentcard = 1;
					if(strstr(board_name,"R680") || strstr(board_name,"R660") || strstr(board_name,"RT990"))
					{
						if(value_currentcard == 0) 			//[当前卡槽] 0——SIM2
							setPINCode(value_pin_card2);
						else if(value_currentcard == 1)		//[当前卡槽] 1——SIM1
							setPINCode(value_pin_card1);
					}
				}
				continue;
			}
			else //不读卡
			{
				//飞行模式，再读一次卡
				setFlightmode();
				while(1)
				{
					memset(cmd,0,sizeof(cmd));
					sprintf(cmd,"cmdat %s 'AT' | grep OK |wc -l",path_usb);
					int tmp_int_buff = cmd_recieve_int(cmd);
					if (tmp_int_buff == 1)
						break;
				}
				memset(cmd,0,sizeof(cmd));
				memset(tmp_buff,0,sizeof(tmp_buff));
				sprintf(cmd,"cmdat %s 'AT+CPIN?' | grep \"+CPIN:\"",path_usb);
				cmd_recieve_str(cmd,tmp_buff);
				if(strstr(tmp_buff,"READY")) //读卡
				{
					goto _read_card_READY;
				}
				else if(strstr(tmp_buff,"SIM PIN")) //锁卡
				{
					goto _read_card_PIN;
				}
				else //不读卡
				{
					memset(cmd,0,sizeof(cmd));
					sprintf(cmd,"print unplug %d",count);
						
					uci_cellular("CPIN","SIM card not inserted");
					uci_cellular("SLOT"," ");
					uci_cellular("CCID"," ");
					clear_default_cellular_info();
					if(strstr(value_module,"nl668"))
					{
						memset(cmd,0,sizeof(cmd));
						sprintf(cmd,"ifconfig %s down",value_ifname);
						system(cmd);
						//Debug(cmd,"daemon.debug");
					}
					else
					{
						cmd_recieve_str(cmd_ifdown,NULL);
						cmd_recieve_str(cmd_ifup,NULL);
					}
					debug_card_info("unplug");
					//信号灯控制
					controlLED();
					if(strstr(value_defaultcard,"auto")) //自动读卡时候切换读卡
					{
						//获取当前读卡位置，切换到另一个位置，飞行模式
						if(strstr(value_module,"fm650") || strstr(value_module,"fm160"))
							SwitchSIM_FM650(false);
						else if(strstr(value_module,"nl668"))
							SwitchSIM_NL668(false);
						else if(strstr(value_module,"sim8200"))
							SwitchSIM_SIM8200(false);
						else if(strstr(value_module,"sim7600"))
						{
							SwitchSIM_SIM7600(false);	//sim7600切换卡槽后，需要飞行模式才可读卡
							setFlightmode();
						}
						else if(strstr(value_module,"rm500u") || strstr(value_module,"rm500q"))
							SwitchSIM_RM500U(false);
						else if(strstr(value_module,"rg520n"))
						{
							SwitchSIM_RG520N(false);	//rg520n切换卡槽后，需要飞行模式才可读卡
							cmd_recieve_str(cmd_ifup,NULL);
						}
						else if(strstr(value_module,"ec20"))
							SwitchSIM_EC20(false);
						else if(strstr(value_module,"mt5700"))
						{
							SwitchSIM_MT5700(false);	//mt5700切换卡槽后，需要飞行模式, 并且激活SIM卡才可读卡
						}
						sleep(1);
					}
				}
			}
		}
		else
		{
			//打印不读卡
			memset(cmd,0,sizeof(cmd));
			sprintf(cmd,"print unplug %d",count);
				
			uci_cellular("CPIN","SIM card not inserted");
			uci_cellular("SLOT"," ");
			uci_cellular("CCID"," ");
			clear_default_cellular_info();
			if(strstr(value_module,"nl668"))
			{
				memset(cmd,0,sizeof(cmd));
				sprintf(cmd,"ifconfig %s down",value_ifname);
				system(cmd);
				//Debug(cmd,"daemon.debug");
			}
			else
			{
				cmd_recieve_str(cmd_ifdown,NULL);
				cmd_recieve_str(cmd_ifup,NULL);
			}
			debug_card_info("unplug");
			//信号灯控制
			controlLED();
			//查询gps
			sleep(1);
			
		}
		getGPS(); //在这里读GPS信号，即时不插卡也不影响。
		if(flag_heartbeat==1)
			heartbeat();
	}
}
static int pin_acm=0;
void getCPIN_acm()
{
	char tmp_buff[1024];
	char cmd[1024];
	int count=0;
	int result;
	while(1)
	{
		memset(cmd,0,sizeof(cmd));
		sprintf(cmd,"cmdat %s 'AT' | grep OK |wc -l",path_usb);
		int tmp_int_buff = cmd_recieve_int(cmd);
		if (tmp_int_buff == 1)
			break;
		//sleep(1);
	}
	while(1)
	{
		memset(cmd,0,sizeof(cmd));
		sprintf(cmd,"uci -P /var/state -q get cellular.%s.sim_status",name_config);
		printf("cmd: %s\n",cmd);
		result=cmd_recieve_int(cmd);
		printf("result: %d\n",result);
		if(result==0)
		{
			memset(cmd,0,sizeof(cmd));
			memset(tmp_buff,0,sizeof(tmp_buff));
			sprintf(cmd,"cmdat %s 'AT+CPIN?' | grep \"+CPIN:\"",path_usb);
			cmd_recieve_str(cmd,tmp_buff);
			
			if(strstr(tmp_buff,"READY"))	//读卡，打印结果到/tmp/tmp_mdoem文件，跳出循环
			{
				//removeCPIN();
				uci_cellular("CPIN","READY");
				/* memset(cmd,0,sizeof(cmd));
				sprintf(cmd,"echo \"%s\" >> %s",tmp_buff,path_outfile);
				cmd_recieve_str(cmd,NULL); */
				break;
			}
			else if(strstr(tmp_buff,"SIM PIN"))
			{
				//removeCPIN();
				uci_cellular("CPIN","SIM PIN");
				clear_default_cellular_info();
				/* memset(cmd,0,sizeof(cmd));
				sprintf(cmd,"echo \"%s\" >> %s",tmp_buff,path_outfile);
				cmd_recieve_str(cmd,NULL); */
				debug_card_info("pin");
				setPINCode(value_pin);
			}else{
				/* if(count==1)
				{
					memset(cmd,0,sizeof(cmd));
					sprintf(cmd,"cmdat %s \"AT+CFUN=1\" > /dev/null 2>&1",path_usb);
					cmd_recieve_str(cmd,NULL);
				} */
				//if(count==4)		//第4次不读卡，打印结果到/tmp/tmp_mdoem文件
				{
					//removeCPIN();
					uci_cellular("CPIN","SIM card not inserted");
					uci_cellular("CCID"," ");
					clear_default_cellular_info();
					/* memset(cmd,0,sizeof(cmd));
					sprintf(cmd,"echo \"+CPIN: SIM card not inserted\" >> %s",path_outfile);
					cmd_recieve_str(cmd,NULL); */
					cmd_recieve_str(cmd_ifdown,NULL);
					cmd_recieve_str(cmd_ifup,NULL);
					//将/tmp/tmp_mdoem文件复制到/tmp/modem文件中
					/* memset(cmd,0,sizeof(cmd));
					sprintf(cmd,"cat %s > %s",path_outfile,path_formfile);
					cmd_recieve_str(cmd,NULL); */
					debug_card_info("unplug");
					//信号灯控制
					controlLED();
				}
				//else if(count==9) 	//飞行模式
				{
					setFlightmode();
					
					if(strstr(value_module,"rg520n"))
						cmd_recieve_str(cmd_ifup,NULL);
					else if(strstr(value_module,"mt5700"))
							if(strcmp(board_name,"R680")!=0)
								ActiveSIM_MT5700();
					else if(strstr(value_module,"mt5711"))
						setFlightmode_MT5711();
					//count=-1;
				}
				sleep(1);
			}
			pin_acm=result;
			//count++;
		}
		else
		{
			uci_cellular("CPIN","SIM card not inserted");
			uci_cellular("CCID"," ");
			clear_default_cellular_info();
			debug_card_info("unplug");
			if(pin_acm==0)
			{
				
						/* memset(cmd,0,sizeof(cmd));
						sprintf(cmd,"echo \"+CPIN: SIM card not inserted\" >> %s",path_outfile);
						cmd_recieve_str(cmd,NULL); */
				setFlightmode();
				cmd_recieve_str(cmd_ifdown,NULL);
				cmd_recieve_str(cmd_ifup,NULL);
						//将/tmp/tmp_mdoem文件复制到/tmp/modem文件中
						/* memset(cmd,0,sizeof(cmd));
						sprintf(cmd,"cat %s > %s",path_outfile,path_formfile);
						cmd_recieve_str(cmd,NULL); */
				
						//信号灯控制
				controlLED();
				pin_acm=1;
			}
				
			sleep(2);
		}
		getGPS(); //在这里读GPS信号，即时不插卡也不影响。
		if(flag_heartbeat==1)
			heartbeat();
	}
}

/* 功能：获取运营商信息
** 返回值：void
*/
void getCOPS()
{
	char tmp_buff[1024];
	char cmd[1024];
	int count=0;
	while(1)
	{
		//循环获取运营商信息，最多2次
		memset(cmd,0,sizeof(cmd));
		memset(tmp_buff,0,sizeof(tmp_buff));
		sprintf(cmd,"cmdat %s \"AT+COPS?\" | grep \"+COPS:\" | tr -d '\"' | awk -F ': ' '{print$2}'",path_usb);
		cmd_recieve_str(cmd,tmp_buff);
		if(strlen(tmp_buff)>0)
		{
			uci_cellular("COPS",tmp_buff);
			/* memset(cmd,0,sizeof(cmd));
			sprintf(cmd,"echo \"+COPS: %s\" >> %s",tmp_buff,path_outfile);
			cmd_recieve_str(cmd,NULL); */
			sprintf(debug_cops,"%s",tmp_buff);
			break;
		}
		
		if(count==1)
			break;
		count++;
	}
}
/* 功能：获取IMSI
** 返回值：void
*/
void getIMSI()
{
	char tmp_buff[1024];
	char cmd[1024];
	int count=0;
	while(1)
	{
		//获取IMSI，最多2次
		memset(cmd,0,sizeof(cmd));
		memset(tmp_buff,0,sizeof(tmp_buff));
		sprintf(cmd,"cmdat %s \"AT+CIMI\" | sed -n '2p' | awk -F ' ' '{print$1}'",path_usb);
		cmd_recieve_str(cmd,tmp_buff);
		//判断结果是否为空或符合IMSI标准
		if(tmp_buff != NULL && isInt(tmp_buff)==0)
		{
			uci_cellular("IMSI",tmp_buff);
			/* memset(cmd,0,sizeof(cmd));
			sprintf(cmd,"echo \"+IMSI: %s\" >> %s",tmp_buff,path_outfile);
			cmd_recieve_str(cmd,NULL); */
			sprintf(debug_imsi,"%s",tmp_buff);
			break;
		}
		
		if(count==1)
			break;
		count++;
	}
}
/* 功能：获取ICCID
** 返回值：void
*/
void getICCID()
{
	char tmp_buff[1024];
	char cmd[1024];
	int count=0;
	while(1)
	{
		//获取IMSI，最多2次
		memset(cmd,0,sizeof(cmd));
		memset(tmp_buff,0,sizeof(tmp_buff));
		if(strstr(value_module,"mt5700") || strstr(value_module,"mt5711"))
		{
			sprintf(cmd,"cmdat %s \"AT^ICCID?\" | grep \"ICCID:\" | awk -F ' ' '{print$2}'",path_usb);
		}
		else
		{
			sprintf(cmd,"cmdat %s \"AT+CCID\" | grep \"+CCID:\" | awk -F ' ' '{print$2}'",path_usb);
		}
		cmd_recieve_str(cmd,tmp_buff);
		//判断结果是否为空
		if(tmp_buff != NULL)
		{
			uci_cellular("CCID",tmp_buff);
			/* memset(cmd,0,sizeof(cmd));
			sprintf(cmd,"echo \"+CCID: %s\" >> %s",tmp_buff,path_outfile);
			cmd_recieve_str(cmd,NULL); */
			sprintf(debug_iccid,"%s",tmp_buff);
			break;
		}
		
		if(count==1)
			break;
		count++;
	}
}
/* 功能：获取信号强度
** 返回值：void
*/
void getCSQ()
{
	char tmp_buff[1024];
	char cmd[1024];
	memset(cmd,0,sizeof(cmd));
	memset(tmp_buff,0,sizeof(tmp_buff));
	sprintf(cmd,"cmdat %s \"AT+CSQ\" | grep \"+CSQ:\"",path_usb);
	cmd_recieve_str(cmd,tmp_buff);
	if(strstr(tmp_buff,"+CSQ:"))
	{
		uci_cellular("CSQ",tmp_buff);
		/* memset(cmd,0,sizeof(cmd));
		sprintf(cmd,"echo \"%s\" >> %s",tmp_buff,path_outfile);
		cmd_recieve_str(cmd,NULL); */
	}
	while(1)
	{
		memset(cmd,0,sizeof(cmd));
		sprintf(cmd,"cmdat %s 'AT' | grep OK |wc -l",path_usb);
		int tmp_int_buff = cmd_recieve_int(cmd);
		if (tmp_int_buff == 1)
			break;
		//sleep(1);
		//Debug("send at","daemon.debug");
	}
}

/* 功能：获取注册状态
** 返回值：void
*/
void getCEREG()
{
	char tmp_buff[1024];
	char cmd[1024];
	if(strstr(value_module,"mt5700") || strstr(value_module,"mt5711"))
	{
		memset(cmd,0,sizeof(cmd));
		sprintf(cmd,"cmdat %s \"AT+CEREG=0\"",path_usb);
		cmd_recieve_str(cmd,NULL);
	}
	memset(cmd,0,sizeof(cmd));
	memset(tmp_buff,0,sizeof(tmp_buff));
	sprintf(cmd,"cmdat %s \"AT+CEREG?\" | grep \"+CEREG:\"",path_usb);
	cmd_recieve_str(cmd,tmp_buff);
	if(strstr(tmp_buff,"+CEREG:"))
	{
		uci_cellular("CEREG",tmp_buff);
		/* memset(cmd,0,sizeof(cmd));
		sprintf(cmd,"echo \"%s\" >> %s",tmp_buff,path_outfile);
		cmd_recieve_str(cmd,NULL); */
	}
}

/* 功能：获取注册状态
** 返回值：void
*/
void getCGREG()
{
	char tmp_buff[1024];
	char cmd[1024];
	if(strstr(value_module,"mt5700") || strstr(value_module,"mt5711"))
	{
		memset(cmd,0,sizeof(cmd));
		sprintf(cmd,"cmdat %s \"AT+CGREG=0\"",path_usb);
		cmd_recieve_str(cmd,NULL);
	}
	memset(cmd,0,sizeof(cmd));
	memset(tmp_buff,0,sizeof(tmp_buff));
	sprintf(cmd,"cmdat %s \"AT+CGREG?\" | grep \"+CGREG:\"",path_usb);
	cmd_recieve_str(cmd,tmp_buff);
	if(strstr(tmp_buff,"+CGREG:"))
	{
		uci_cellular("CGREG",tmp_buff);
		/* memset(cmd,0,sizeof(cmd));
		sprintf(cmd,"echo \"%s\" >> %s",tmp_buff,path_outfile);
		cmd_recieve_str(cmd,NULL); */
	}
}

/* 功能：获取5G注册状态
** 返回值：void
*/
void getC5GREG()
{
	char tmp_buff[1024];
	char cmd[1024];
	if(strstr(value_module,"mt5700") || strstr(value_module,"mt5711"))
	{
		memset(cmd,0,sizeof(cmd));
		sprintf(cmd,"cmdat %s \"AT+C5GREG=0\"",path_usb);
		cmd_recieve_str(cmd,NULL);
	}
	memset(cmd,0,sizeof(cmd));
	memset(tmp_buff,0,sizeof(tmp_buff));
	sprintf(cmd,"cmdat %s \"AT+C5GREG?\" | grep \"+C5GREG:\"",path_usb);
	cmd_recieve_str(cmd,tmp_buff);
	if(strstr(tmp_buff,"+C5GREG:"))
	{
		uci_cellular("C5GREG",tmp_buff);
		/* memset(cmd,0,sizeof(cmd));
		sprintf(cmd,"echo \"%s\" >> %s",tmp_buff,path_outfile);
		cmd_recieve_str(cmd,NULL); */
	}
}
/* 功能：支持读取GPS信号的模块，获取GPS信号
** 返回值：void
*/
void getGPS()
{
	char cmd[1024];
	memset(cmd,0,sizeof(cmd));
	if(flag_gps==1)
	{
		if(strstr(value_module,"fm160") || strstr(value_module,"nl668") || strstr(value_module,"fg160"))
		{
			sprintf(cmd,"cmdat %s AT+GTGPSPOWER? | grep \"+GTGPSPOWER: 1\" | wc -l",path_usb);
			int result = cmd_recieve_int(cmd);
			if ( result == 0)
			{
				memset(cmd,0,sizeof(cmd));
				sprintf(cmd,"cmdat %s 'AT+GTGPSPOWER=1'",path_usb);
				cmd_recieve_str(cmd,NULL);
			}
			if (strstr(value_module,"fg160"))
			{
				memset(cmd,0,sizeof(cmd));
				sprintf(cmd,"cmdat %s 'AT+GTSET?' | grep NMEAEN | awk -F ',' '{print $2}'",path_usb);
				result = cmd_recieve_int(cmd);
				if ( result == 0)
				{
					memset(cmd,0,sizeof(cmd));
					sprintf(cmd,"cmdat %s 'AT+GTSET=\"NMEAEN\",1'",path_usb);
					cmd_recieve_str(cmd,NULL);
				}
			}
			
			if(flag_gps==1 && flag_gpslog ==1 && strlen(path_gpslog)>0)
			{
				/* if( get_file_size(path_gpslog) > 100000 ){
					char tmp_tail[200]; //tail命令
					// /etc/debug文件的最后800复制到/etc/tmp_debug文件
					memset(tmp_tail,0,sizeof(tmp_tail));
					sprintf(tmp_tail,"tail -n 800 %s > /var/log/tmp_gps_%s.log",path_gpslog,name_config);
					system(tmp_tail);
					//删除/etc/debug文件
					memset(tmp_tail,0,sizeof(tmp_tail));
					sprintf(tmp_tail,"%s %s","rm",path_gpslog);
					system(tmp_tail);
					// /etc/tmp_debug文件复制到/etc/debug
					memset(tmp_tail,0,sizeof(tmp_tail));
					sprintf(tmp_tail,"mv /var/log/tmp_gps_%s.log %s",name_config,path_gpslog);
					system(tmp_tail);
				} */
				sprintf(cmd,"cmdat %s 'AT+GTGPS?' | awk 'NR>2{print}' > %s",path_usb,path_gpslog);
				cmd_recieve_str(cmd,NULL);
			}
		}
	}
}

/* 功能：解析RSRP，转换为信号值
** 返回值：void
*/
extern void analyticRSRP(int tmp_rsrp,float tmp_rsrq,int tmp_rssi)
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
	if(tmp_rssi!=999)
		uci_cellular_int("RSSI",tmp_rssi);
	else
		uci_cellular_delete("RSSI");
	uci_cellular_delete("SINR");
	uci_cellular_float("RSRQ",(double)tmp_rsrq);
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
	if(tmp_rssi!=999)
	{
		uci_cellular_int("RSSI",tmp_rssi);
		/* memset(cmd,0,sizeof(cmd));
		sprintf(cmd,"echo \"+RSSI: %d\" >> %s",tmp_rssi,path_outfile);
		cmd_recieve_str(cmd,NULL); */
	}
	char debug[1024];	
	memset(debug,0,sizeof(debug));
	if(value_signal>2 && sighi_debug == 0)
	{
		if(tmp_rssi!=999)
			sprintf(debug,"rsrp: %d dBm; rsrq: %.1f dB; rssi %d dBm; signal value: %d",tmp_rsrp,tmp_rsrq,tmp_rssi,value_signal);
		else
			sprintf(debug,"rsrp: %d dBm; rsrq: %.1f dB; signal value: %d",tmp_rsrp,tmp_rsrq,value_signal);
		Debug(debug,"cellular.info");
		sighi_debug = 1;
		siglo_debug = 0;
	}
	if(value_signal<2 && siglo_debug == 0)
	{
		if(tmp_rssi!=999)
			sprintf(debug,"low signal, rsrp: %d dBm; rsrq: %.1f dB; rssi %d dBm; signal value: %d",tmp_rsrp,tmp_rsrq,tmp_rssi,value_signal);
		else
			sprintf(debug,"low signal, rsrp: %d dBm; rsrq: %.1f dB; signal value: %d",tmp_rsrp,tmp_rsrq,value_signal);
		Debug(debug,"cellular.warn");
		sighi_debug = 0;
		siglo_debug = 1;
	}
}

/* 功能：解析CSQ，转换为信号值
** 返回值：void
*/
void analyticCSQ()
{
	char tmp_buff[1024];
	char cmd[1024];
	memset(cmd,0,sizeof(cmd));
	memset(tmp_buff,0,sizeof(tmp_buff));
	sprintf(cmd,"cmdat %s \"AT+CSQ\" | grep \"+CSQ:\"",path_usb);
	cmd_recieve_str(cmd,tmp_buff);
	if(strstr(tmp_buff,"+CSQ:"))
	{
		memset(cmd,0,sizeof(cmd));
		sprintf(cmd,"echo \"%s\" | awk '{print $2}' | awk -F ',' '{print $1}'",tmp_buff);
		int tmp_csq;
		tmp_csq=cmd_recieve_int(cmd);
		if(tmp_csq == 99 ) 			//99,99		无信号
			value_signal=0;
		else if(tmp_csq <= 7 )		//0-6,99	无信号
			value_signal=0;
		else if(tmp_csq <= 15 )		//7-14,99	1格
			value_signal=1;
		else if(tmp_csq <= 18 )		//15-17,99	2格
			value_signal=2;
		else if(tmp_csq <= 21 )		//18-20,99	3格
			value_signal=3;
		else if(tmp_csq <= 25 )		//21-24,99	4格
			value_signal=4;
		else if(tmp_csq <= 31 )		//25-31,99	5格
			value_signal=5;
		int rssi=0;
		if(tmp_csq<99)
			rssi=(tmp_csq*2-113);
		uci_cellular_int("SVAL",value_signal);
		uci_cellular_int("RSSI",rssi);
		uci_cellular_delete("RSRP");
		uci_cellular_delete("RSRQ");
		uci_cellular_delete("SINR");
		/* memset(cmd,0,sizeof(cmd));
		sprintf(cmd,"echo \"+SVAL: %d\" >> %s",value_signal,path_outfile);
		cmd_recieve_str(cmd,NULL); */
		/* memset(cmd,0,sizeof(cmd));
		sprintf(cmd,"echo \"+RSSI: %d\" >> %s",rssi,path_outfile);
		cmd_recieve_str(cmd,NULL); */
		char debug[1024];	
		memset(debug,0,sizeof(debug));
		if(value_signal>2 && sighi_debug == 0)
		{
			memset(debug,0,sizeof(debug));
			sprintf(debug,"rssi: %d dBm; signal value: %d",rssi,value_signal);
			Debug(debug,"cellular.info");
			sighi_debug=1;
			siglo_debug=0;
		}
		if(value_signal<2 && siglo_debug == 0)
		{
			memset(debug,0,sizeof(debug));
			sprintf(debug,"low signal, rssi: %d dBm; signal value: %d",rssi,value_signal);
			Debug(debug,"cellular.warn");
			sighi_debug=0;
			siglo_debug=1;
		}
	}
}
/* 功能：设定PIN码
** 返回值：void
*/
void setPINCode(char* pin)
{
	if(strlen(pin)>0)
	{
		char cmd[1024];
		memset(cmd,0,sizeof(cmd));
		if(strstr(value_module,"nl668") || strstr(value_module,"ec20"))
		{
			sprintf(cmd,"cmdat %s AT+CPIN=%s",path_usb,pin);
		}
		else
		{
			sprintf(cmd,"cmdat %s 'AT+CPIN=\"%s\"'",path_usb,pin);
		}
		char tmp_buff[1024];
		memset(tmp_buff,0,sizeof(tmp_buff));
		cmd_recieve_str(cmd,tmp_buff);
		if (strcmp(pin,debug_pincode)!=0 || count_loop == 0)
		{
			char debug[100];
			memset(debug,0,sizeof(debug));
			if(strstr(tmp_buff,"OK"))
			{
				sprintf(debug,"set pin code %s OK",pin);
				Debug(debug,"daemon.info");
				sprintf(debug_pincode,"%s",pin);
			}
			else if(strstr(tmp_buff,"ERROR"))
			{
				sprintf(debug,"set pin code %s failed",pin);
				Debug(debug,"daemon.error");
			}
		}
	}
}
/* 功能：设定拨号信息：APN、服务类型和*锁频
** 返回值：void
*/
void setModem()
{
	if(count_loop!=0)	//仅设置一次
		return;
	char cmd[1024];
	char tmp_buff[1024];
	//设定PIN码
	if(strlen(value_pin)>0)
	{
		setPINCode(value_pin);
	}
	//设定APN、IP类型、用户名、密码和认证方式
	if(strstr(value_module,"rm500u") || strstr(value_module,"rm500q"))														//移远模组
	{
		int tmp_type_ip=3;
		if(strstr(type_ip,"IPV4"))
			tmp_type_ip=1;
		if(strstr(type_ip,"IPV6"))
			tmp_type_ip=2;
		if(strstr(type_ip,"IPV4V6"))
			tmp_type_ip=3;

_SETAPN_AGAIN1:
		memset(cmd,0,sizeof(cmd));
		sprintf(cmd,"cmdat %s 'AT+QICSGP=1,%d,\"%s\",\"%s\",\"%s\",%s'",path_usb,tmp_type_ip,value_apn,value_username,value_password,value_auth);
		memset(tmp_buff,0,sizeof(tmp_buff));
		cmd_recieve_str(cmd,tmp_buff);
	}
	else
	{
	//设定APN和拨号IP类型
_SETAPN_AGAIN2:
		memset(tmp_buff,0,sizeof(tmp_buff));
		memset(cmd,0,sizeof(cmd));
		sprintf(cmd,"cmdat %s 'AT+CGDCONT?'",path_usb);
		cmd_recieve_str(cmd,tmp_buff);
		int count=countstr(tmp_buff,",");
		char array_CGDCONT[count+1][50];
		char debug[100];
		memset(debug,0,sizeof(debug));
		if(separate_string2array(tmp_buff,",",count+1,50,(char *)&array_CGDCONT) != count+1)
		{
			printf("formatting CGDCONT failed\n");
		}
		char tmp_iptype[50];
		memset(tmp_iptype,0,sizeof(tmp_iptype));
		sprintf(tmp_iptype,"\"%s\"",type_ip);
		char tmp_apn[50];
		memset(tmp_apn,0,sizeof(tmp_apn));
		sprintf(tmp_apn,"\"%s\"",value_apn);
		if(strcmp(array_CGDCONT[1],tmp_iptype)!=0 || strcmp(array_CGDCONT[2],tmp_apn)!=0)
		{
			/* Debug(array_CGDCONT[1],"daemon.debug1");
			Debug(tmp_iptype,"daemon.debug2");
			Debug(array_CGDCONT[2],"daemon.debug3");
			Debug(tmp_apn,"daemon.debug4"); */
			memset(cmd,0,sizeof(cmd));
			sprintf(cmd,"cmdat %s 'AT+CGDCONT=1,\"%s\",\"%s\"'",path_usb,type_ip,value_apn);
			memset(tmp_buff,0,sizeof(tmp_buff));
			cmd_recieve_str(cmd,NULL);
			if(strstr(value_module,"mt5700") || strstr(value_module,"mt5711"))
			{
				memset(cmd,0,sizeof(cmd));
				sprintf(cmd,"cmdat %s 'AT+CGDCONT=0,\"%s\",\"%s\"'",path_usb,type_ip,value_apn);
				memset(tmp_buff,0,sizeof(tmp_buff));
				cmd_recieve_str(cmd,NULL);
			}
		}
			
		if(strstr(value_module,"fm650") || strstr(value_module,"fm160") || strstr(value_module,"nl668") || strstr(value_module,"fg160"))		//广和通模组
		{
			//设定APN下的用户名、密码和认证方式
			if(strlen(value_username) > 0)
			{
				memset(cmd,0,sizeof(cmd));
				sprintf(cmd,"cmdat %s 'AT+MGAUTH=1,%s,\"%s\",\"%s\"'",path_usb,value_auth,value_username,value_password);
				cmd_recieve_str(cmd,NULL);
			}
			// if(strlen(value_apn)>0)
			// {
				// memset(cmd,0,sizeof(cmd));
				// sprintf(cmd,"cmdat %s 'AT+CAVIMS?' | grep \"+CAVIMS:\" | awk -F ' ' '{print$NF}'",path_usb);
				// int cavims=cmd_recieve_int(cmd);
				// if(cavims==1)
				// {
					// memset(cmd,0,sizeof(cmd));
					// sprintf(cmd,"cmdat %s 'AT+CAVIMS=0'",path_usb);
					// cmd_recieve_str(cmd,NULL);
				// }
			// }
		}
		// else if(strstr(value_module,"rg520n"))											//移远RG520N模组
		// {
			// memset(cmd,0,sizeof(cmd));
			// sprintf(cmd,"cmdat %s 'AT+QCFG=\"ims\"' | grep \"+QCFG:\" | awk -F ',' '{print$(NF-1)}'",path_usb);
			// memset(tmp_buff,0,sizeof(tmp_buff));
			// cmd_recieve_str(cmd,tmp_buff);
			// if(!strstr(tmp_buff,"0"))
			// {
				// memset(cmd,0,sizeof(cmd));
				// sprintf(cmd,"cmdat %s 'AT+QCFG=\"ims\",0'",path_usb);
				// memset(tmp_buff,0,sizeof(tmp_buff));
				// cmd_recieve_str(cmd,tmp_buff);
			// }
		// }
		else if(strstr(value_module,"mt5700") || strstr(value_module,"mt5711"))
		{
			if(strlen(value_username_card1)>0)
			{
				//设定APN下的用户名、密码和认证方式
				memset(cmd,0,sizeof(cmd));
				sprintf(cmd,"cmdat %s 'AT^AUTHDATA=1,%s,\"%s\",\"%s\",\"%s\"'",path_usb,value_auth,value_plmn,value_password,value_username);
				cmd_recieve_str(cmd,NULL);
				
				memset(cmd,0,sizeof(cmd));
				sprintf(cmd,"cmdat %s 'AT^AUTHDATA=0,%s,\"%s\",\"%s\",\"%s\"'",path_usb,value_auth,value_plmn,value_password,value_username);
				cmd_recieve_str(cmd,NULL);
			}
		}
	}
	//设定服务类型
	if(strstr(value_module,"sim7600"))		//芯讯通4G模组
	{
		int tmp_mode_rat;
		if(mode_rat==0)							//0：2 自动
			tmp_mode_rat=2;
		else if(mode_rat==1)					//1: 14	仅3G
			tmp_mode_rat=14;
		else if(mode_rat==2)					//2: 38	仅4G
			tmp_mode_rat=38;
		else if(mode_rat==3)					//3：54	4G/3G
			tmp_mode_rat=54;
		else if(mode_rat==5)					//5: 13	仅2G
			tmp_mode_rat=13;
		else if(mode_rat==8)					//8: 19	2G/3G
			tmp_mode_rat=19;
		else if(mode_rat==9)					//9: 51 2G/4G
			tmp_mode_rat=51;
		else if(mode_rat==10)					//10: 39 2G/3G/4G
			tmp_mode_rat=39;
		else
			tmp_mode_rat=2;
		//设定sim7600模组上网服务类型
		setRATMODE_SIM7600(tmp_mode_rat);
	}
	else if(strstr(value_module,"sim8200"))		//芯讯通5G模组
	{
		int tmp_mode_rat;
		char tmp_band_wcdma[50];
		char tmp_band_lte[50];
		char tmp_band_nr[50];
		char tmp_band_nsa[50];
		memset(tmp_band_wcdma,0,sizeof(tmp_band_wcdma));
		memset(tmp_band_lte,0,sizeof(tmp_band_lte));
		memset(tmp_band_nr,0,sizeof(tmp_band_nr));
		memset(tmp_band_nsa,0,sizeof(tmp_band_nsa));
		if(mode_rat==0)							//0：2   自动
			tmp_mode_rat=2;
		else if(mode_rat==1)					//1: 14	 仅3G
		{
			tmp_mode_rat=14;
			strcpy(tmp_band_wcdma,band_wcdma);
		}
		else if(mode_rat==2)					//2: 38	 仅4G
		{
			tmp_mode_rat=38;
			strcpy(tmp_band_lte,band_lte);
		}
		else if(mode_rat==3)					//3：54	 4G/3G
		{
			tmp_mode_rat=54;
			strcpy(tmp_band_wcdma,band_wcdma);
			strcpy(tmp_band_lte,band_lte);
		}
		else if(mode_rat==4)					//4: 71	 仅5G
		{
			tmp_mode_rat=71;
			strcpy(tmp_band_nr,band_nr);
			strcpy(tmp_band_nsa,band_nsa);
		}
		else if(mode_rat==6)					//6: 109 5G/4G
		{
			tmp_mode_rat=109;
			strcpy(tmp_band_nr,band_nr);
			strcpy(tmp_band_nsa,band_nsa);
			strcpy(tmp_band_lte,band_lte);
		}
		else if(mode_rat==7)					//7: 55 5G/4G/3G
		{
			tmp_mode_rat=55;
			strcpy(tmp_band_nr,band_nr);
			strcpy(tmp_band_nsa,band_nsa);
			strcpy(tmp_band_lte,band_lte);
			strcpy(tmp_band_wcdma,band_wcdma);
		}
		else
			tmp_mode_rat=2;
		//设定sim8200模组上网服务类型
		setRATMODE_SIM8200(tmp_mode_rat,tmp_band_wcdma,tmp_band_lte,tmp_band_nr,tmp_band_nsa);
	}
	else if(strstr(value_module,"fm650") || strstr(value_module,"fm160") || strstr(value_module,"fg160"))		//广和通模组
	{
		char tmp_mode_rat[100];
		char tmp_band_wcdma[50];
		char tmp_band_lte[50];
		char tmp_band_nr[50];
		memset(tmp_band_wcdma,0,sizeof(tmp_band_wcdma));
		memset(tmp_band_lte,0,sizeof(tmp_band_lte));
		memset(tmp_band_nr,0,sizeof(tmp_band_nr));
		memset(tmp_mode_rat,0,sizeof(tmp_mode_rat));
		if(mode_rat==0)							//0：10 自动
		{	
			strcpy(tmp_mode_rat,"20");
		}
		else if(mode_rat==1)					//1: 1	仅3G
		{	
			strcpy(tmp_mode_rat,"1");
			strcpy(tmp_band_wcdma,band_wcdma);
		}
		else if(mode_rat==2)					//2: 2	仅4G
		{	
			strcpy(tmp_mode_rat,"2");
			strcpy(tmp_band_lte,band_lte);
		}
		else if(mode_rat==3)					//3：4	4G/3G
		{	
			strcpy(tmp_mode_rat,"4");
			strcpy(tmp_band_wcdma,band_wcdma);
			strcpy(tmp_band_lte,band_lte);
		}
		else if(mode_rat==4)					//4: 14	仅5G
		{	
			strcpy(tmp_mode_rat,"14");
			strcpy(tmp_band_nr,band_nr);
		}
		else if(mode_rat==6)					//6: 17	5G/4G
		{	
			strcpy(tmp_mode_rat,"17");
			strcpy(tmp_band_lte,band_lte);
			strcpy(tmp_band_nr,band_nr);
		}
		else if(mode_rat==7)					//7: 20 5G/4G/3G
		{	
			strcpy(tmp_mode_rat,"20");
			strcpy(tmp_band_lte,band_lte);
			strcpy(tmp_band_nr,band_nr);
			strcpy(tmp_band_wcdma,band_wcdma);
		}
		else
		{	
			strcpy(tmp_mode_rat,"20");
		}
		//设定fm160 fm650模组频点和小区
		
		setRATMODE_FM160(tmp_mode_rat,tmp_band_wcdma,tmp_band_lte,tmp_band_nr);
		if(((mode_rat==4) || (mode_rat==2))&& strlen(value_arfcn)>0)
		{
			if(mode_rat==4)
			{
				set5GCELLLOCK_FM650(tmp_band_nr,value_arfcn,value_psc,value_scs);
			}
			else
			{
				setCELLLOCK_FM650(value_arfcn,value_pci);
			}
			
		}
		else
			resetCELLLOCK_FM650();
	}
	else if(strstr(value_module,"nl668"))
	{
		char tmp_mode_rat[100];
		char tmp_band_cdma[50];
		char tmp_band_lte[50];
		char tmp_band_gsm[50];
		char tmp_band_umts[50];
		char tmp_band_scdma[50];
		memset(tmp_band_cdma,0,sizeof(tmp_band_cdma));
		memset(tmp_band_lte,0,sizeof(tmp_band_lte));
		memset(tmp_band_gsm,0,sizeof(tmp_band_gsm));
		memset(tmp_band_umts,0,sizeof(tmp_band_umts));
		memset(tmp_band_scdma,0,sizeof(tmp_band_scdma));
		memset(tmp_mode_rat,0,sizeof(tmp_mode_rat));
		if(mode_rat==0)							//0：10 自动
		{	
			strcpy(tmp_mode_rat,"10");
		}
		else if(mode_rat==1)					//1: 0	GSM
		{	
			strcpy(tmp_mode_rat,"0");
			strcpy(tmp_band_gsm,band_gsm);
		}
		else if(mode_rat==2)					//2: 1	UMTS
		{	
			strcpy(tmp_mode_rat,"1");
			strcpy(tmp_band_umts,band_umts);
		}
		else if(mode_rat==3)					//3：2	LTE
		{	
			strcpy(tmp_mode_rat,"2");
			strcpy(tmp_band_lte,band_lte);
		}
		else if(mode_rat==4)					//4: 3	GSM UMTS
		{	
			strcpy(tmp_mode_rat,"3");
			strcpy(tmp_band_gsm,band_gsm);
			strcpy(tmp_band_umts,band_umts);
		}
		else if(mode_rat==5)					//5: 4	LTE UMTS
		{	
			strcpy(tmp_mode_rat,"4");
			strcpy(tmp_band_lte,band_lte);
			strcpy(tmp_band_umts,band_umts);
		}
		else if(mode_rat==6)					//6: 5 	LTE GSM
		{	
			strcpy(tmp_mode_rat,"5");
			strcpy(tmp_band_lte,band_lte);
			strcpy(tmp_band_gsm,band_gsm);
		}
		else if(mode_rat==7)					//7: 6 	LTE GSM UMTS
		{	
			strcpy(tmp_mode_rat,"6");
			strcpy(tmp_band_lte,band_lte);
			strcpy(tmp_band_gsm,band_gsm);
			strcpy(tmp_band_umts,band_umts);
		}
		else if(mode_rat==8)					//8: 7 TD_SCDMA
		{	
			strcpy(tmp_mode_rat,"7");
			strcpy(tmp_band_scdma,band_scdma);
		}
		else if(mode_rat==11)					//11: 11 CDMA
		{	
			strcpy(tmp_mode_rat,"11");
			strcpy(tmp_band_cdma,band_cdma);
		}
		else if(mode_rat==12)					//12: 12 CDMA EVDO
		{	
			strcpy(tmp_mode_rat,"12");
			strcpy(tmp_band_cdma,band_cdma);
		}
		else if(mode_rat==13)					//13: 13 EVDO
		{	
			strcpy(tmp_mode_rat,"13");
			strcpy(tmp_band_cdma,band_cdma);
		}
		else
		{	
			strcpy(tmp_mode_rat,"10");
		}
		setRATMODE_NL668(tmp_mode_rat,tmp_band_gsm,tmp_band_umts,tmp_band_lte,tmp_band_scdma,tmp_band_cdma);
		if((mode_rat==3)&& strlen(value_arfcn)>0)
		{
			setCELLLOCK_FM650(value_arfcn,value_pci);
		}
		else
			resetCELLLOCK_FM650();
	}
	else if(strstr(value_module,"rm500u"))
	{
		char tmp_mode_rat[100];
		char tmp_band_wcdma[50];
		char tmp_band_lte[50];
		char tmp_band_nr[50];
		char tmp_band_nsa[50];
		memset(tmp_band_wcdma,0,sizeof(tmp_band_wcdma));
		memset(tmp_band_lte,0,sizeof(tmp_band_lte));
		memset(tmp_band_nr,0,sizeof(tmp_band_nr));
		memset(tmp_band_nsa,0,sizeof(tmp_band_nsa));
		memset(tmp_mode_rat,0,sizeof(tmp_mode_rat));
		if(mode_rat==0)							//0：20 自动
			strcpy(tmp_mode_rat,"AUTO");
		else if(mode_rat==1)					//1: 2	仅3G
		{
			strcpy(tmp_mode_rat,"WCDMA");
			strcpy(tmp_band_wcdma,band_wcdma);
		}
		else if(mode_rat==2)					//2: 3	仅4G
		{
			strcpy(tmp_mode_rat,"LTE");
			strcpy(tmp_band_lte,band_lte);
		}
		else if(mode_rat==3)					//3：4	4G/3G
		{
			strcpy(tmp_mode_rat,"LTE:WCDMA");
			strcpy(tmp_band_wcdma,band_wcdma);
			strcpy(tmp_band_lte,band_lte);
		}
		else if(mode_rat==4)					//4: 14	仅5G
		{
			strcpy(tmp_mode_rat,"NR5G");
			strcpy(tmp_band_nr,band_nr);
		}
		else if(mode_rat==6)					//6: 16	5G/4G
		{
			strcpy(tmp_mode_rat,"NR5G:LTE");
			strcpy(tmp_band_lte,band_lte);
			strcpy(tmp_band_nr,band_nr);
		}
		else if(mode_rat==7)					//7: 20 5G/4G/3G
		{
			strcpy(tmp_mode_rat,"NR5G:LTE:WCDMA");
			strcpy(tmp_band_wcdma,band_wcdma);
			strcpy(tmp_band_lte,band_lte);
			strcpy(tmp_band_nr,band_nr);
		}
		else
			strcpy(tmp_mode_rat,"AUTO");
		setRATMODE_RM500U(tmp_mode_rat,tmp_band_wcdma,tmp_band_lte,tmp_band_nr);
	}	
	else if(strstr(value_module,"rm500q"))
	{
		char tmp_mode_rat[100];
		char tmp_band_wcdma[50];
		char tmp_band_lte[50];
		char tmp_band_nr[50];
		char tmp_band_nsa[50];
		memset(tmp_band_wcdma,0,sizeof(tmp_band_wcdma));
		memset(tmp_band_lte,0,sizeof(tmp_band_lte));
		memset(tmp_band_nr,0,sizeof(tmp_band_nr));
		memset(tmp_band_nsa,0,sizeof(tmp_band_nsa));
		memset(tmp_mode_rat,0,sizeof(tmp_mode_rat));
		if(mode_rat==0)							//0：20 自动
			strcpy(tmp_mode_rat,"AUTO");
		else if(mode_rat==1)					//1: 2	仅3G
		{
			strcpy(tmp_mode_rat,"WCDMA");
			strcpy(tmp_band_wcdma,band_wcdma);
		}
		else if(mode_rat==2)					//2: 3	仅4G
		{
			strcpy(tmp_mode_rat,"LTE");
			strcpy(tmp_band_lte,band_lte);
		}
		else if(mode_rat==3)					//3：4	4G/3G
		{
			strcpy(tmp_mode_rat,"LTE:WCDMA");
			strcpy(tmp_band_wcdma,band_wcdma);
			strcpy(tmp_band_lte,band_lte);
		}
		else if(mode_rat==4)					//4: 14	仅5G
		{
			strcpy(tmp_mode_rat,"NR5G");
			strcpy(tmp_band_nr,band_nr);
		}
		else if(mode_rat==6)					//6: 16	5G/4G
		{
			strcpy(tmp_mode_rat,"NR5G:LTE");
			strcpy(tmp_band_lte,band_lte);
			strcpy(tmp_band_nr,band_nr);
		}
		else if(mode_rat==7)					//7: 20 5G/4G/3G
		{
			strcpy(tmp_mode_rat,"NR5G:LTE:WCDMA");
			strcpy(tmp_band_wcdma,band_wcdma);
			strcpy(tmp_band_lte,band_lte);
			strcpy(tmp_band_nr,band_nr);
		}
		else
			strcpy(tmp_mode_rat,"AUTO");
		setRATMODE_RM500Q(tmp_mode_rat,tmp_band_wcdma,tmp_band_lte,tmp_band_nr);
	}
	else if(strstr(value_module,"rg520n"))
	{
		char tmp_mode_rat[100];
		char tmp_band_wcdma[50];
		char tmp_band_lte[50];
		char tmp_band_nr[50];
		char tmp_band_nsa[50];
		memset(tmp_band_wcdma,0,sizeof(tmp_band_wcdma));
		memset(tmp_band_lte,0,sizeof(tmp_band_lte));
		memset(tmp_band_nr,0,sizeof(tmp_band_nr));
		memset(tmp_band_nsa,0,sizeof(tmp_band_nsa));
		memset(tmp_mode_rat,0,sizeof(tmp_mode_rat));
		if(mode_rat==0)							//0：20 自动
			strcpy(tmp_mode_rat,"AUTO");
		else if(mode_rat==1)					//1: 2	仅3G
		{
			strcpy(tmp_mode_rat,"WCDMA");
			strcpy(tmp_band_wcdma,band_wcdma);
		}
		else if(mode_rat==2)					//2: 3	仅4G
		{
			strcpy(tmp_mode_rat,"LTE");
			strcpy(tmp_band_lte,band_lte);
		}
		else if(mode_rat==3)					//3：4	4G/3G
		{
			strcpy(tmp_mode_rat,"LTE:WCDMA");
			strcpy(tmp_band_wcdma,band_wcdma);
			strcpy(tmp_band_lte,band_lte);
		}
		else if(mode_rat==4)					//4: 14	仅5G
		{
			strcpy(tmp_mode_rat,"NR5G");
			strcpy(tmp_band_nr,band_nr);
		}
		else if(mode_rat==6)					//6: 16	5G/4G
		{
			strcpy(tmp_mode_rat,"NR5G:LTE");
			strcpy(tmp_band_lte,band_lte);
			strcpy(tmp_band_nr,band_nr);
		}
		else if(mode_rat==7)					//7: 20 5G/4G/3G
		{
			strcpy(tmp_mode_rat,"NR5G:LTE:WCDMA");
			strcpy(tmp_band_wcdma,band_wcdma);
			strcpy(tmp_band_lte,band_lte);
			strcpy(tmp_band_nr,band_nr);
		}
		else
			strcpy(tmp_mode_rat,"AUTO");
		
		setRATMODE_RG520N(tmp_mode_rat,tmp_band_wcdma,tmp_band_lte,tmp_band_nr,tmp_band_nsa,value_arfcn);
		if(((mode_rat==4) || (mode_rat==2))&& strlen(value_arfcn)>0)
		{
			if(mode_rat==4)
			{
				set5GCELLLOCK_RG520N(tmp_band_nr,value_arfcn,value_psc,value_scs);
			}
			else
			{
				setCELLLOCK_RG520N(value_arfcn,value_pci);
			}
		}
		else
		{
			reset_RG520N();
		}
	}
	else if(strstr(value_module,"ec20"))
	{
		char tmp_mode_rat[100];
		if(mode_rat==0)							//0：10 自动
		{	
			strcpy(tmp_mode_rat,"0");
		}
		else if(mode_rat==1)					//1: 0	GSM
		{	
			strcpy(tmp_mode_rat,"1");
		}
		else if(mode_rat==2)					//2: 1	WCDMA
		{	
			strcpy(tmp_mode_rat,"2");
		}
		else if(mode_rat==3)					//5：1	LTE
		{	
			strcpy(tmp_mode_rat,"3");
		}
		else
		{	
			strcpy(tmp_mode_rat,"0");
		}
		setRATMODE_EC20(tmp_mode_rat);
		
	}
	else if(strstr(value_module,"mt5700"))		//海思模组
	{
		//Debug("set RAT","daemon.debug");
		char tmp_mode_rat[100];
		char tmp_band_wcdma[50];
		char tmp_band_lte[50];
		char tmp_band_nr[50];
		memset(tmp_band_wcdma,0,sizeof(tmp_band_wcdma));
		memset(tmp_band_lte,0,sizeof(tmp_band_lte));
		memset(tmp_band_nr,0,sizeof(tmp_band_nr));
		memset(tmp_mode_rat,0,sizeof(tmp_mode_rat));
		if(mode_rat==0)							//0：10 自动
		{	
			strcpy(tmp_mode_rat,"080302");
		}
		else if(mode_rat==1)					//1: 1	仅3G
		{	
			strcpy(tmp_mode_rat,"02");
			strcpy(tmp_band_wcdma,band_wcdma);
		}
		else if(mode_rat==2)					//2: 2	仅4G
		{	
			strcpy(tmp_mode_rat,"03");
			strcpy(tmp_band_lte,band_lte);
		}
		else if(mode_rat==3)					//3：4	4G/3G
		{	
			strcpy(tmp_mode_rat,"0203");
			strcpy(tmp_band_wcdma,band_wcdma);
			strcpy(tmp_band_lte,band_lte);
		}
		else if(mode_rat==4)					//4: 14	仅5G
		{	
			strcpy(tmp_mode_rat,"08");
			strcpy(tmp_band_nr,band_nr);
		}
		else if(mode_rat==6)					//6: 17	5G/4G
		{	
			strcpy(tmp_mode_rat,"0803");
			strcpy(tmp_band_lte,band_lte);
			strcpy(tmp_band_nr,band_nr);
		}
		else if(mode_rat==7)					//7: 20 5G/4G/3G
		{	
			strcpy(tmp_mode_rat,"080302");
			strcpy(tmp_band_lte,band_lte);
			strcpy(tmp_band_nr,band_nr);
			strcpy(tmp_band_wcdma,band_wcdma);
		}
		else
		{	
			strcpy(tmp_mode_rat,"080302");
		}
		//设定mt700模组频点和小区
		
		setRATMODE_MT5700(tmp_mode_rat,tmp_band_wcdma,tmp_band_lte,tmp_band_nr,value_arfcn,value_psc,value_scs,value_pci);
		/* if(((mode_rat_card1==4) || (mode_rat_card1==2))&& strlen(value_arfcn_card1)>0)
		{
			if(mode_rat_card1==4)
			{
				set5GCELLLOCK_MT5700(tmp_band_nr,value_arfcn_card1,value_psc_card1,value_scs_card1);
			}
			else
			{
				setCELLLOCK_MT5700(value_arfcn_card1,value_pci_card1);
			}
		}
		else
			resetCELLLOCK_MT5700(); */
		//Debug("End Setmodem1","daemon.debug");
	}
	else if(strstr(value_module,"mt5711"))		//海思模组
	{
		//Debug("set RAT","daemon.debug");
		char tmp_mode_rat[100];
		char tmp_band_wcdma[50];
		char tmp_band_lte[50];
		char tmp_band_nr[50];
		memset(tmp_band_wcdma,0,sizeof(tmp_band_wcdma));
		memset(tmp_band_lte,0,sizeof(tmp_band_lte));
		memset(tmp_band_nr,0,sizeof(tmp_band_nr));
		memset(tmp_mode_rat,0,sizeof(tmp_mode_rat));
		if(mode_rat==0)							//0：10 自动
		{	
			strcpy(tmp_mode_rat,"00");
		}
		else if(mode_rat==1)					//1: 1	仅3G
		{	
			strcpy(tmp_mode_rat,"02");
			strcpy(tmp_band_wcdma,band_wcdma);
		}
		else if(mode_rat==2)					//2: 2	仅4G
		{	
			strcpy(tmp_mode_rat,"03");
			strcpy(tmp_band_lte,band_lte);
		}
		else if(mode_rat==3)					//3：4	4G/3G
		{	
			strcpy(tmp_mode_rat,"0302");
			strcpy(tmp_band_wcdma,band_wcdma);
			strcpy(tmp_band_lte,band_lte);
		}
		else if(mode_rat==4)					//4: 14	仅5G
		{	
			strcpy(tmp_mode_rat,"08");
			strcpy(tmp_band_nr,band_nr);
		}
		else if(mode_rat==6)					//6: 17	5G/4G
		{	
			strcpy(tmp_mode_rat,"00");
			strcpy(tmp_band_lte,band_lte);
			strcpy(tmp_band_nr,band_nr);
		}
		else if(mode_rat==7)					//7: 20 5G/4G/3G
		{	
			strcpy(tmp_mode_rat,"0803");
			strcpy(tmp_band_lte,band_lte);
			strcpy(tmp_band_nr,band_nr);
			strcpy(tmp_band_wcdma,band_wcdma);
		}
		else
		{	
			strcpy(tmp_mode_rat,"00");
		}
		//设定mt700模组频点和小区
		
		setRATMODE_MT5700(tmp_mode_rat,tmp_band_wcdma,tmp_band_lte,tmp_band_nr,value_arfcn,value_psc,value_scs,value_pci);
		/* if(((mode_rat_card1==4) || (mode_rat_card1==2))&& strlen(value_arfcn_card1)>0)
		{
			if(mode_rat_card1==4)
			{
				set5GCELLLOCK_MT5700(tmp_band_nr,value_arfcn_card1,value_psc_card1,value_scs_card1);
			}
			else
			{
				setCELLLOCK_MT5700(value_arfcn_card1,value_pci_card1);
			}
		}
		else
			resetCELLLOCK_MT5700(); */
		//Debug("End Setmodem1","daemon.debug");
	}
}


/* 功能：设定SIM1拨号信息：APN、服务类型和*锁频
** 返回值：void
*/
void setModem1()
{
	/* if(count_loop!=0)	//仅设置一次
		return; */
	char cmd[1024];
	char tmp_buff[1024];
	//设定PIN码
	char debug[1024];
	
	if(strlen(value_pin_card1)>0)
	{
		memset(debug,0,sizeof(debug));
		sprintf(debug,"set pin code %s",value_pin_card1);
		//Debug(debug,"daemon.debug");
		setPINCode(value_pin_card1);
	}
	//设定APN、IP类型、用户名、密码和认证方式
	if(strstr(value_module,"rm500u") || strstr(value_module,"rm500q"))														//移远模组
	{
		int tmp_type_ip=3;
		if(strstr(type_ip_card1,"IPV4"))
			tmp_type_ip=1;
		if(strstr(type_ip_card1,"IPV6"))
			tmp_type_ip=2;
		if(strstr(type_ip_card1,"IPV4V6"))
			tmp_type_ip=3;

_SETAPN_AGAIN1:
		memset(cmd,0,sizeof(cmd));
		sprintf(cmd,"cmdat %s 'AT+QICSGP=1,%d,\"%s\",\"%s\",\"%s\",%s'",path_usb,tmp_type_ip,value_apn_card1,value_username_card1,value_password_card1,value_auth_card1);
		memset(tmp_buff,0,sizeof(tmp_buff));
		cmd_recieve_str(cmd,tmp_buff);
	}
	else
	{
		//Debug("set APN","daemon.debug");
	//设定APN和拨号IP类型
_SETAPN_AGAIN2:
		memset(tmp_buff,0,sizeof(tmp_buff));
		memset(cmd,0,sizeof(cmd));
		sprintf(cmd,"cmdat %s 'AT+CGDCONT?'",path_usb);
		cmd_recieve_str(cmd,tmp_buff);
		int count=countstr(tmp_buff,",");
		char array_CGDCONT[count+1][50];
		char debug[100];
		memset(debug,0,sizeof(debug));
		if(separate_string2array(tmp_buff,",",count+1,50,(char *)&array_CGDCONT) != count+1)
		{
			printf("formatting CGDCONT failed\n");
		}
		char tmp_iptype[50];
		memset(tmp_iptype,0,sizeof(tmp_iptype));
		sprintf(tmp_iptype,"\"%s\"",type_ip_card1);
		char tmp_apn[50];
		memset(tmp_apn,0,sizeof(tmp_apn));
		sprintf(tmp_apn,"\"%s\"",value_apn_card1);
		if(strcmp(array_CGDCONT[1],tmp_iptype)!=0 || strcmp(array_CGDCONT[2],tmp_apn)!=0)
		{
			memset(debug,0,sizeof(debug));
			sprintf(debug,"set apn %s",tmp_apn);
			//Debug(debug,"daemon.debug");
			memset(cmd,0,sizeof(cmd));
			sprintf(cmd,"cmdat %s 'AT+CGDCONT=1,\"%s\",\"%s\"'",path_usb,type_ip_card1,value_apn_card1);
			memset(tmp_buff,0,sizeof(tmp_buff));
			cmd_recieve_str(cmd,NULL);
			
			if(strstr(value_module,"mt5700") || strstr(value_module,"mt5711"))
			{
				memset(cmd,0,sizeof(cmd));
				sprintf(cmd,"cmdat %s 'AT+CGDCONT=0,\"%s\",\"%s\"'",path_usb,type_ip_card1,value_apn_card1);
				memset(tmp_buff,0,sizeof(tmp_buff));
				cmd_recieve_str(cmd,NULL);
			}
			
		}
		if(strstr(value_module,"fm650") || strstr(value_module,"fm160") || strstr(value_module,"nl668") || strstr(value_module,"fg160"))		//广和通模组
		{
			if(strlen(value_username_card1)>0)
			{
				//设定APN下的用户名、密码和认证方式
				memset(cmd,0,sizeof(cmd));
				sprintf(cmd,"cmdat %s 'AT+MGAUTH=1,%s,\"%s\",\"%s\"'",path_usb,value_auth_card1,value_username_card1,value_password_card1);
				cmd_recieve_str(cmd,NULL);
			}
			// if(strlen(value_apn_card1)>0)
			// {
				// memset(cmd,0,sizeof(cmd));
				// sprintf(cmd,"cmdat %s 'AT+CAVIMS?' | grep \"+CAVIMS:\" | awk -F ' ' '{print$NF}'",path_usb);
				// int cavims=cmd_recieve_int(cmd);
				// if(cavims==1)
				// {
					// memset(debug,0,sizeof(debug));
					// sprintf(debug,"close cavims");
					//Debug(debug,"daemon.debug");
					// memset(cmd,0,sizeof(cmd));
					// sprintf(cmd,"cmdat %s 'AT+CAVIMS=0'",path_usb);
					// cmd_recieve_str(cmd,NULL);
				// }
			// }
		}
		// else if(strstr(value_module,"rg520n"))											//移远RG520N模组
		// {
			// memset(cmd,0,sizeof(cmd));
			// sprintf(cmd,"cmdat %s 'AT+QCFG=\"ims\"' | grep \"+QCFG:\" | awk -F ',' '{print$(NF-1)}'",path_usb);
			// memset(tmp_buff,0,sizeof(tmp_buff));
			// cmd_recieve_str(cmd,tmp_buff);
			// if(!strstr(tmp_buff,"0"))
			// {
				// memset(cmd,0,sizeof(cmd));
				// sprintf(cmd,"cmdat %s 'AT+QCFG=\"ims\",0'",path_usb);
				// memset(tmp_buff,0,sizeof(tmp_buff));
				// cmd_recieve_str(cmd,tmp_buff);
			// }
		// }
		else if(strstr(value_module,"mt5700") || strstr(value_module,"mt5711"))
		{
			if(strlen(value_username_card1)>0)
			{
				//设定APN下的用户名、密码和认证方式
				memset(cmd,0,sizeof(cmd));
				sprintf(cmd,"cmdat %s 'AT^AUTHDATA=1,%s,\"%s\",\"%s\",\"%s\"'",path_usb,value_auth_card1,value_plmn_card1,value_password_card1,value_username_card1);
				cmd_recieve_str(cmd,NULL);
				
				memset(cmd,0,sizeof(cmd));
				sprintf(cmd,"cmdat %s 'AT^AUTHDATA=0,%s,\"%s\",\"%s\",\"%s\"'",path_usb,value_auth_card1,value_plmn_card1,value_password_card1,value_username_card1);
				cmd_recieve_str(cmd,NULL);
			}
		}
	}
	//设定服务类型
	if(strstr(value_module,"sim7600"))		//芯讯通4G模组
	{
		int tmp_mode_rat;
		if(mode_rat_card1==0)							//0：2 自动
			tmp_mode_rat=2;
		else if(mode_rat_card1==1)					//1: 14	仅3G
			tmp_mode_rat=14;
		else if(mode_rat_card1==2)					//2: 38	仅4G
			tmp_mode_rat=38;
		else if(mode_rat_card1==3)					//3：54	4G/3G
			tmp_mode_rat=54;
		else if(mode_rat_card1==5)					//5: 13	仅2G
			tmp_mode_rat=13;
		else if(mode_rat_card1==8)					//8: 19	2G/3G
			tmp_mode_rat=19;
		else if(mode_rat_card1==9)					//9: 51 2G/4G
			tmp_mode_rat=51;
		else if(mode_rat_card1==10)					//10: 39 2G/3G/4G
			tmp_mode_rat=39;
		else
			tmp_mode_rat=2;
		//设定sim7600模组上网服务类型
		setRATMODE_SIM7600(tmp_mode_rat);
	}
	else if(strstr(value_module,"sim8200"))		//芯讯通5G模组
	{
		int tmp_mode_rat;
		char tmp_band_wcdma[50];
		char tmp_band_lte[50];
		char tmp_band_nr[50];
		char tmp_band_nsa[50];
		memset(tmp_band_wcdma,0,sizeof(tmp_band_wcdma));
		memset(tmp_band_lte,0,sizeof(tmp_band_lte));
		memset(tmp_band_nr,0,sizeof(tmp_band_nr));
		memset(tmp_band_nsa,0,sizeof(tmp_band_nsa));
		if(mode_rat_card1==0)							//0：2   自动
			tmp_mode_rat=2;
		else if(mode_rat_card1==1)					//1: 14	 仅3G
		{
			tmp_mode_rat=14;
			strcpy(tmp_band_wcdma,band_wcdma);
		}
		else if(mode_rat_card1==2)					//2: 38	 仅4G
		{
			tmp_mode_rat=38;
			strcpy(tmp_band_lte,band_lte);
		}
		else if(mode_rat_card1==3)					//3：54	 4G/3G
		{
			tmp_mode_rat=54;
			strcpy(tmp_band_wcdma,band_wcdma);
			strcpy(tmp_band_lte,band_lte);
		}
		else if(mode_rat_card1==4)					//4: 71	 仅5G
		{
			tmp_mode_rat=71;
			strcpy(tmp_band_nr,band_nr);
			strcpy(tmp_band_nsa,band_nsa);
		}
		else if(mode_rat_card1==6)					//6: 109 5G/4G
		{
			tmp_mode_rat=109;
			strcpy(tmp_band_nr,band_nr);
			strcpy(tmp_band_nsa,band_nsa);
			strcpy(tmp_band_lte,band_lte);
		}
		else if(mode_rat_card1==7)					//7: 55 5G/4G/3G
		{
			tmp_mode_rat=55;
			strcpy(tmp_band_nr,band_nr);
			strcpy(tmp_band_nsa,band_nsa);
			strcpy(tmp_band_lte,band_lte);
			strcpy(tmp_band_wcdma,band_wcdma);
		}
		else
			tmp_mode_rat=2;
		//设定sim8200模组上网服务类型
		setRATMODE_SIM8200(tmp_mode_rat,tmp_band_wcdma,tmp_band_lte,tmp_band_nr,tmp_band_nsa);
	}
	else if(strstr(value_module,"fm650") || strstr(value_module,"fm160") || strstr(value_module,"fg160"))		//广和通模组
	{
		char tmp_mode_rat[100];
		char tmp_band_wcdma[50];
		char tmp_band_lte[50];
		char tmp_band_nr[50];
		memset(tmp_band_wcdma,0,sizeof(tmp_band_wcdma));
		memset(tmp_band_lte,0,sizeof(tmp_band_lte));
		memset(tmp_band_nr,0,sizeof(tmp_band_nr));
		memset(tmp_mode_rat,0,sizeof(tmp_mode_rat));
		if(mode_rat_card1==0)							//0：20 自动
		{	
			strcpy(tmp_mode_rat,"20");
		}
		else if(mode_rat_card1==1)					//1: 1	仅3G
		{	
			strcpy(tmp_mode_rat,"1");
			strcpy(tmp_band_wcdma,band_wcdma_card1);
		}
		else if(mode_rat_card1==2)					//2: 2	仅4G
		{	
			strcpy(tmp_mode_rat,"2");
			strcpy(tmp_band_lte,band_lte_card1);
		}
		else if(mode_rat_card1==3)					//3：4	4G/3G
		{	
			strcpy(tmp_mode_rat,"4");
			strcpy(tmp_band_wcdma,band_wcdma_card1);
			strcpy(tmp_band_lte,band_lte_card1);
		}
		else if(mode_rat_card1==4)					//4: 14	仅5G
		{	
			strcpy(tmp_mode_rat,"14");
			strcpy(tmp_band_nr,band_nr_card1);
		}
		else if(mode_rat_card1==6)					//6: 17	5G/4G
		{	
			strcpy(tmp_mode_rat,"17");
			strcpy(tmp_band_lte,band_lte_card1);
			strcpy(tmp_band_nr,band_nr_card1);
		}
		else if(mode_rat_card1==7)					//7: 20 5G/4G/3G
		{	
			strcpy(tmp_mode_rat,"20");
			strcpy(tmp_band_lte,band_lte_card1);
			strcpy(tmp_band_nr,band_nr_card1);
			strcpy(tmp_band_wcdma,band_wcdma_card1);
		}
		else
		{	
			strcpy(tmp_mode_rat,"20");
		}
		//设定fm160 fm650模组频点和小区
		//Debug("goto setRATMODE_FM160","daemon.debug");
		setRATMODE_FM160(tmp_mode_rat,tmp_band_wcdma,tmp_band_lte,tmp_band_nr);
		if(((mode_rat_card1==4) || (mode_rat_card1==2))&& strlen(value_arfcn_card1)>0)
		{
			if(mode_rat_card1==4)
			{
				set5GCELLLOCK_FM650(tmp_band_nr,value_arfcn_card1,value_psc_card1,value_scs_card1);
			}
			else
			{
				setCELLLOCK_FM650(value_arfcn_card1,value_pci_card1);
			}
		}
		else
			resetCELLLOCK_FM650();
	}
	else if(strstr(value_module,"nl668"))
	{
		char tmp_mode_rat[100];
		char tmp_band_cdma[50];
		char tmp_band_lte[50];
		char tmp_band_gsm[50];
		char tmp_band_umts[50];
		char tmp_band_scdma[50];
		memset(tmp_band_cdma,0,sizeof(tmp_band_cdma));
		memset(tmp_band_lte,0,sizeof(tmp_band_lte));
		memset(tmp_band_gsm,0,sizeof(tmp_band_gsm));
		memset(tmp_band_umts,0,sizeof(tmp_band_umts));
		memset(tmp_band_scdma,0,sizeof(tmp_band_scdma));
		memset(tmp_mode_rat,0,sizeof(tmp_mode_rat));
		if(mode_rat_card1==0)							//0：10 自动
		{	
			strcpy(tmp_mode_rat,"10");
		}
		else if(mode_rat_card1==1)					//1: 0	GSM
		{	
			strcpy(tmp_mode_rat,"0");
			strcpy(tmp_band_gsm,band_gsm_card1);
		}
		else if(mode_rat_card1==2)					//2: 1	UMTS
		{	
			strcpy(tmp_mode_rat,"1");
			strcpy(tmp_band_umts,band_umts_card1);
		}
		else if(mode_rat_card1==3)					//3：2	LTE
		{	
			strcpy(tmp_mode_rat,"2");
			strcpy(tmp_band_lte,band_lte_card1);
		}
		else if(mode_rat_card1==4)					//4: 3	GSM UMTS
		{	
			strcpy(tmp_mode_rat,"3");
			strcpy(tmp_band_gsm,band_gsm_card1);
			strcpy(tmp_band_umts,band_umts_card1);
		}
		else if(mode_rat_card1==5)					//5: 4	LTE UMTS
		{	
			strcpy(tmp_mode_rat,"4");
			strcpy(tmp_band_lte,band_lte_card1);
			strcpy(tmp_band_umts,band_umts_card1);
		}
		else if(mode_rat_card1==6)					//6: 5 	LTE GSM
		{	
			strcpy(tmp_mode_rat,"5");
			strcpy(tmp_band_lte,band_lte_card1);
			strcpy(tmp_band_gsm,band_gsm_card1);
		}
		else if(mode_rat_card1==7)					//7: 6 	LTE GSM UMTS
		{	
			strcpy(tmp_mode_rat,"6");
			strcpy(tmp_band_lte,band_lte_card1);
			strcpy(tmp_band_gsm,band_gsm_card1);
			strcpy(tmp_band_umts,band_umts_card1);
		}
		else if(mode_rat_card1==8)					//8: 7 TD_SCDMA
		{	
			strcpy(tmp_mode_rat,"7");
			strcpy(tmp_band_scdma,band_scdma_card1);
		}
		else if(mode_rat_card1==11)					//11: 11 CDMA
		{	
			strcpy(tmp_mode_rat,"11");
			strcpy(tmp_band_cdma,band_cdma_card1);
		}
		else if(mode_rat_card1==12)					//12: 12 CDMA EVDO
		{	
			strcpy(tmp_mode_rat,"12");
			strcpy(tmp_band_cdma,band_cdma_card1);
		}
		else if(mode_rat_card1==13)					//13: 13 EVDO
		{	
			strcpy(tmp_mode_rat,"13");
			strcpy(tmp_band_cdma,band_cdma_card1);
		}
		else
		{	
			strcpy(tmp_mode_rat,"10");
		}
		memset(debug,0,sizeof(debug));
		sprintf(debug,"setRATMODE_NL668");
		//Debug(debug,"daemon.debug");
		setRATMODE_NL668(tmp_mode_rat,tmp_band_gsm,tmp_band_umts,tmp_band_lte,tmp_band_scdma,tmp_band_cdma);
		if((mode_rat_card1==3)&& strlen(value_arfcn_card1)>0)
		{
			setCELLLOCK_FM650(value_arfcn_card1,value_pci_card1);
		}
		else
			resetCELLLOCK_FM650();
	}
	else if(strstr(value_module,"rm500u"))
	{
		char tmp_mode_rat[100];
		char tmp_band_wcdma[50];
		char tmp_band_lte[50];
		char tmp_band_nr[50];
		char tmp_band_nsa[50];
		memset(tmp_band_wcdma,0,sizeof(tmp_band_wcdma));
		memset(tmp_band_lte,0,sizeof(tmp_band_lte));
		memset(tmp_band_nr,0,sizeof(tmp_band_nr));
		memset(tmp_band_nsa,0,sizeof(tmp_band_nsa));
		memset(tmp_mode_rat,0,sizeof(tmp_mode_rat));
		if(mode_rat_card1==0)							//0：20 自动
			strcpy(tmp_mode_rat,"AUTO");
		else if(mode_rat_card1==1)					//1: 2	仅3G
		{
			strcpy(tmp_mode_rat,"WCDMA");
			strcpy(tmp_band_wcdma,band_wcdma_card1);
		}
		else if(mode_rat_card1==2)					//2: 3	仅4G
		{
			strcpy(tmp_mode_rat,"LTE");
			strcpy(tmp_band_lte,band_lte_card1);
		}
		else if(mode_rat_card1==3)					//3：4	4G/3G
		{
			strcpy(tmp_mode_rat,"LTE:WCDMA");
			strcpy(tmp_band_wcdma,band_wcdma_card1);
			strcpy(tmp_band_lte,band_lte_card1);
		}
		else if(mode_rat_card1==4)					//4: 14	仅5G
		{
			strcpy(tmp_mode_rat,"NR5G");
			strcpy(tmp_band_nr,band_nr);
		}
		else if(mode_rat_card1==6)					//6: 16	5G/4G
		{
			strcpy(tmp_mode_rat,"NR5G:LTE");
			strcpy(tmp_band_lte,band_lte_card1);
			strcpy(tmp_band_nr,band_nr_card1);
		}
		else if(mode_rat_card1==7)					//7: 20 5G/4G/3G
		{
			strcpy(tmp_mode_rat,"NR5G:LTE:WCDMA");
			strcpy(tmp_band_wcdma,band_wcdma_card1);
			strcpy(tmp_band_lte,band_lte_card1);
			strcpy(tmp_band_nr,band_nr_card1);
		}
		else
			strcpy(tmp_mode_rat,"AUTO");
		setRATMODE_RM500U(tmp_mode_rat,tmp_band_wcdma,tmp_band_lte,tmp_band_nr);
	}	
	else if(strstr(value_module,"rm500q"))
	{
		char tmp_mode_rat[100];
		char tmp_band_wcdma[50];
		char tmp_band_lte[50];
		char tmp_band_nr[50];
		char tmp_band_nsa[50];
		memset(tmp_band_wcdma,0,sizeof(tmp_band_wcdma));
		memset(tmp_band_lte,0,sizeof(tmp_band_lte));
		memset(tmp_band_nr,0,sizeof(tmp_band_nr));
		memset(tmp_band_nsa,0,sizeof(tmp_band_nsa));
		memset(tmp_mode_rat,0,sizeof(tmp_mode_rat));
		if(mode_rat_card1==0)							//0：20 自动
			strcpy(tmp_mode_rat,"AUTO");
		else if(mode_rat_card1==1)					//1: 2	仅3G
		{
			strcpy(tmp_mode_rat,"WCDMA");
			strcpy(tmp_band_wcdma,band_wcdma_card1);
		}
		else if(mode_rat_card1==2)					//2: 3	仅4G
		{
			strcpy(tmp_mode_rat,"LTE");
			strcpy(tmp_band_lte,band_lte_card1);
		}
		else if(mode_rat_card1==3)					//3：4	4G/3G
		{
			strcpy(tmp_mode_rat,"LTE:WCDMA");
			strcpy(tmp_band_wcdma,band_wcdma_card1);
			strcpy(tmp_band_lte,band_lte_card1);
		}
		else if(mode_rat_card1==4)					//4: 14	仅5G
		{
			strcpy(tmp_mode_rat,"NR5G");
			strcpy(tmp_band_nr,band_nr_card1);
		}
		else if(mode_rat_card1==6)					//6: 16	5G/4G
		{
			strcpy(tmp_mode_rat,"NR5G:LTE");
			strcpy(tmp_band_lte,band_lte_card1);
			strcpy(tmp_band_nr,band_nr_card1);
		}
		else if(mode_rat_card1==7)					//7: 20 5G/4G/3G
		{
			strcpy(tmp_mode_rat,"NR5G:LTE:WCDMA");
			strcpy(tmp_band_wcdma,band_wcdma_card1);
			strcpy(tmp_band_lte,band_lte_card1);
			strcpy(tmp_band_nr,band_nr_card1);
		}
		else
			strcpy(tmp_mode_rat,"AUTO");
		setRATMODE_RM500Q(tmp_mode_rat,tmp_band_wcdma,tmp_band_lte,tmp_band_nr);
	}
	else if(strstr(value_module,"rg520n"))
	{
		char tmp_mode_rat[100];
		char tmp_band_wcdma[50];
		char tmp_band_lte[50];
		char tmp_band_nr[50];
		char tmp_band_nsa[50];
		memset(tmp_band_wcdma,0,sizeof(tmp_band_wcdma));
		memset(tmp_band_lte,0,sizeof(tmp_band_lte));
		memset(tmp_band_nr,0,sizeof(tmp_band_nr));
		memset(tmp_band_nsa,0,sizeof(tmp_band_nsa));
		memset(tmp_mode_rat,0,sizeof(tmp_mode_rat));
		if(mode_rat_card1==0)							//0：20 自动
		{
			strcpy(tmp_mode_rat,"AUTO");
			strcpy(tmp_band_wcdma,"0");
			strcpy(tmp_band_lte,"0");
			strcpy(tmp_band_nr,"0");
		}
		else if(mode_rat_card1==1)					//1: 2	仅3G
		{
			strcpy(tmp_mode_rat,"WCDMA");
			strcpy(tmp_band_wcdma,band_wcdma_card1);
		}
		else if(mode_rat_card1==2)					//2: 3	仅4G
		{
			strcpy(tmp_mode_rat,"LTE");
			strcpy(tmp_band_lte,band_lte_card1);
		}
		else if(mode_rat_card1==3)					//3：4	4G/3G
		{
			strcpy(tmp_mode_rat,"LTE:WCDMA");
			strcpy(tmp_band_wcdma,band_wcdma_card1);
			strcpy(tmp_band_lte,band_lte_card1);
		}
		else if(mode_rat_card1==4)					//4: 14	仅5G
		{
			strcpy(tmp_mode_rat,"NR5G");
			strcpy(tmp_band_nr,band_nr_card1);
		}
		else if(mode_rat_card1==6)					//6: 16	5G/4G
		{
			strcpy(tmp_mode_rat,"NR5G:LTE");
			strcpy(tmp_band_lte,band_lte_card1);
			strcpy(tmp_band_nr,band_nr_card1);
		}
		else if(mode_rat_card1==7)					//7: 20 5G/4G/3G
		{
			strcpy(tmp_mode_rat,"NR5G:LTE:WCDMA");
			strcpy(tmp_band_wcdma,band_wcdma_card1);
			strcpy(tmp_band_lte,band_lte_card1);
			strcpy(tmp_band_nr,band_nr_card1);
		}
		else
			strcpy(tmp_mode_rat,"AUTO");
		setRATMODE_RG520N(tmp_mode_rat,tmp_band_wcdma,tmp_band_lte,tmp_band_nr,tmp_band_nsa,value_arfcn_card1);
		if(((mode_rat_card1==4) || (mode_rat_card1==2))&& strlen(value_arfcn_card1)>0)
		{
			if(mode_rat_card1==4)
			{
				set5GCELLLOCK_RG520N(tmp_band_nr,value_arfcn_card1,value_psc_card1,value_scs_card1);
			}
			else
			{
				setCELLLOCK_RG520N(value_arfcn_card1,value_pci_card1);
			}
		}
		else
		{
			reset_RG520N();
		}
	}
	else if(strstr(value_module,"mt5700"))		//海思模组
	{
		//Debug("set RAT","daemon.debug");
		char tmp_mode_rat[100];
		char tmp_band_wcdma[50];
		char tmp_band_lte[50];
		char tmp_band_nr[50];
		memset(tmp_band_wcdma,0,sizeof(tmp_band_wcdma));
		memset(tmp_band_lte,0,sizeof(tmp_band_lte));
		memset(tmp_band_nr,0,sizeof(tmp_band_nr));
		memset(tmp_mode_rat,0,sizeof(tmp_mode_rat));
		if(mode_rat_card1==0)							//0：10 自动
		{	
			strcpy(tmp_mode_rat,"080302");
		}
		else if(mode_rat_card1==1)					//1: 1	仅3G
		{	
			strcpy(tmp_mode_rat,"02");
			strcpy(tmp_band_wcdma,band_wcdma_card1);
		}
		else if(mode_rat_card1==2)					//2: 2	仅4G
		{	
			strcpy(tmp_mode_rat,"03");
			strcpy(tmp_band_lte,band_lte_card1);
		}
		else if(mode_rat_card1==3)					//3：4	4G/3G
		{	
			strcpy(tmp_mode_rat,"0203");
			strcpy(tmp_band_wcdma,band_wcdma_card1);
			strcpy(tmp_band_lte,band_lte_card1);
		}
		else if(mode_rat_card1==4)					//4: 14	仅5G
		{	
			strcpy(tmp_mode_rat,"08");
			strcpy(tmp_band_nr,band_nr_card1);
		}
		else if(mode_rat_card1==6)					//6: 17	5G/4G
		{	
			strcpy(tmp_mode_rat,"0803");
			strcpy(tmp_band_lte,band_lte_card1);
			strcpy(tmp_band_nr,band_nr_card1);
		}
		else if(mode_rat_card1==7)					//7: 20 5G/4G/3G
		{	
			strcpy(tmp_mode_rat,"080302");
			strcpy(tmp_band_lte,band_lte_card1);
			strcpy(tmp_band_nr,band_nr_card1);
			strcpy(tmp_band_wcdma,band_wcdma_card1);
		}
		else
		{	
			strcpy(tmp_mode_rat,"080302");
		}
		//设定mt700模组频点和小区
		
		setRATMODE_MT5700(tmp_mode_rat,tmp_band_wcdma,tmp_band_lte,tmp_band_nr,value_arfcn_card1,value_psc_card1,value_scs_card1,value_pci_card1);
		/* if(((mode_rat_card1==4) || (mode_rat_card1==2))&& strlen(value_arfcn_card1)>0)
		{
			if(mode_rat_card1==4)
			{
				set5GCELLLOCK_MT5700(tmp_band_nr,value_arfcn_card1,value_psc_card1,value_scs_card1);
			}
			else
			{
				setCELLLOCK_MT5700(value_arfcn_card1,value_pci_card1);
			}
		}
		else
			resetCELLLOCK_MT5700(); */
		//Debug("End Setmodem1","daemon.debug");
	}
	else if(strstr(value_module,"mt5711"))		//海思模组
	{
		//Debug("set RAT","daemon.debug");
		char tmp_mode_rat[100];
		char tmp_band_wcdma[50];
		char tmp_band_lte[50];
		char tmp_band_nr[50];
		memset(tmp_band_wcdma,0,sizeof(tmp_band_wcdma));
		memset(tmp_band_lte,0,sizeof(tmp_band_lte));
		memset(tmp_band_nr,0,sizeof(tmp_band_nr));
		memset(tmp_mode_rat,0,sizeof(tmp_mode_rat));
		if(mode_rat_card1==0)							//0：10 自动
		{	
			strcpy(tmp_mode_rat,"00");
		}
		else if(mode_rat_card1==1)					//1: 1	仅3G
		{	
			strcpy(tmp_mode_rat,"02");
			strcpy(tmp_band_wcdma,band_wcdma_card1);
		}
		else if(mode_rat_card1==2)					//2: 2	仅4G
		{	
			strcpy(tmp_mode_rat,"03");
			strcpy(tmp_band_lte,band_lte_card1);
		}
		else if(mode_rat_card1==3)					//3：4	4G/3G
		{	
			strcpy(tmp_mode_rat,"0302");
			strcpy(tmp_band_wcdma,band_wcdma_card1);
			strcpy(tmp_band_lte,band_lte_card1);
		}
		else if(mode_rat_card1==4)					//4: 14	仅5G
		{	
			strcpy(tmp_mode_rat,"08");
			strcpy(tmp_band_nr,band_nr_card1);
		}
		else if(mode_rat_card1==6)					//6: 17	5G/4G
		{	
			strcpy(tmp_mode_rat,"00");
			strcpy(tmp_band_lte,band_lte_card1);
			strcpy(tmp_band_nr,band_nr_card1);
		}
		else if(mode_rat_card1==7)					//7: 20 5G/4G/3G
		{	
			strcpy(tmp_mode_rat,"0803");
			strcpy(tmp_band_lte,band_lte_card1);
			strcpy(tmp_band_nr,band_nr_card1);
			strcpy(tmp_band_wcdma,band_wcdma_card1);
		}
		else
		{	
			strcpy(tmp_mode_rat,"00");
		}
		//设定mt700模组频点和小区
		
		setRATMODE_MT5700(tmp_mode_rat,tmp_band_wcdma,tmp_band_lte,tmp_band_nr,value_arfcn_card1,value_psc_card1,value_scs_card1,value_pci_card1);
		/* if(((mode_rat_card1==4) || (mode_rat_card1==2))&& strlen(value_arfcn_card1)>0)
		{
			if(mode_rat_card1==4)
			{
				set5GCELLLOCK_MT5700(tmp_band_nr,value_arfcn_card1,value_psc_card1,value_scs_card1);
			}
			else
			{
				setCELLLOCK_MT5700(value_arfcn_card1,value_pci_card1);
			}
		}
		else
			resetCELLLOCK_MT5700(); */
		//Debug("End Setmodem1","daemon.debug");
	}
	else if(strstr(value_module,"ec20-eux"))
	{
		char tmp_mode_rat[100];
		char tmp_band_wcdma[50];
		char tmp_band_lte[50];
		char tmp_band_gsm[50];
		memset(tmp_band_wcdma,0,sizeof(tmp_band_wcdma));
		memset(tmp_band_lte,0,sizeof(tmp_band_lte));
		memset(tmp_band_gsm,0,sizeof(tmp_band_gsm));
		if(mode_rat_card1==0)						//0：0 自动
		{	
			strcpy(tmp_mode_rat,"0");
		}
		else if(mode_rat_card1==1)					//1: 1	GSM
		{	
			strcpy(tmp_mode_rat,"1");
			strcpy(tmp_band_gsm,band_gsm_card1);
		}
		else if(mode_rat_card1==2)					//2: 2	WCDMA
		{	
			strcpy(tmp_mode_rat,"2");
			strcpy(tmp_band_wcdma,band_wcdma_card1);
		}
		else if(mode_rat_card1==3)					//3：3	LTE
		{	
			strcpy(tmp_mode_rat,"3");
			strcpy(tmp_band_lte,band_lte_card1);
		}
		else
		{	
			strcpy(tmp_mode_rat,"0");
		}
		setRATMODE_EC20(tmp_mode_rat,tmp_band_gsm,tmp_band_wcdma,tmp_band_lte);
	}
	else if(strstr(value_module,"ec20-af"))
	{
		char tmp_mode_rat[100];
		char tmp_band_wcdma[50];
		char tmp_band_lte[50];
		char tmp_band_gsm[50];
		memset(tmp_band_wcdma,0,sizeof(tmp_band_wcdma));
		memset(tmp_band_lte,0,sizeof(tmp_band_lte));
		memset(tmp_band_gsm,0,sizeof(tmp_band_gsm));
		if(mode_rat_card1==0)						//0：0 自动
		{	
			strcpy(tmp_mode_rat,"0");
		}
		else if(mode_rat_card1==2)					//2: 2	WCDMA
		{	
			strcpy(tmp_mode_rat,"2");
			strcpy(tmp_band_wcdma,band_wcdma_card1);
		}
		else if(mode_rat_card1==3)					//3：3	LTE
		{	
			strcpy(tmp_mode_rat,"3");
			strcpy(tmp_band_lte,band_lte_card1);
		}
		else
		{	
			strcpy(tmp_mode_rat,"0");
		}
		setRATMODE_EC20(tmp_mode_rat,tmp_band_gsm,tmp_band_wcdma,tmp_band_lte);
	}
}


/* 功能：设定SIM2拨号信息：APN、服务类型和*锁频
** 返回值：void
*/
void setModem2()
{
	char cmd[1024];
	char tmp_buff[1024];
	//设定PIN码
	if(strlen(value_pin_card2)>0)
	{
		setPINCode(value_pin_card2);
	}
	//设定APN、IP类型、用户名、密码和认证方式
	if(strstr(value_module,"rm500u") || strstr(value_module,"rm500q"))														//移远模组
	{
		int tmp_type_ip=3;
		if(strstr(type_ip_card2,"IPV4"))
			tmp_type_ip=1;
		if(strstr(type_ip_card2,"IPV6"))
			tmp_type_ip=2;
		if(strstr(type_ip_card2,"IPV4V6"))
			tmp_type_ip=3;

_SETAPN_AGAIN1:
		memset(cmd,0,sizeof(cmd));
		sprintf(cmd,"cmdat %s 'AT+QICSGP=1,%d,\"%s\",\"%s\",\"%s\",%s'",path_usb,tmp_type_ip,value_apn_card2,value_username_card2,value_password_card2,value_auth_card2);
		memset(tmp_buff,0,sizeof(tmp_buff));
		cmd_recieve_str(cmd,tmp_buff);
	}
	else
	{
	//设定APN和拨号IP类型
_SETAPN_AGAIN2:
		memset(tmp_buff,0,sizeof(tmp_buff));
		memset(cmd,0,sizeof(cmd));
		sprintf(cmd,"cmdat %s 'AT+CGDCONT?'",path_usb);
		cmd_recieve_str(cmd,tmp_buff);
		int count=countstr(tmp_buff,",");
		char array_CGDCONT[count+1][50];
		char debug[100];
		memset(debug,0,sizeof(debug));
		if(separate_string2array(tmp_buff,",",count+1,50,(char *)&array_CGDCONT) != count+1)
		{
			printf("formatting CGDCONT failed\n");
		}
		char tmp_iptype[50];
		memset(tmp_iptype,0,sizeof(tmp_iptype));
		sprintf(tmp_iptype,"\"%s\"",type_ip_card2);
		char tmp_apn[50];
		memset(tmp_apn,0,sizeof(tmp_apn));
		sprintf(tmp_apn,"\"%s\"",value_apn_card2);
		if(strcmp(array_CGDCONT[1],tmp_iptype)!=0 || strcmp(array_CGDCONT[2],tmp_apn)!=0)
		{
			memset(cmd,0,sizeof(cmd));
			sprintf(cmd,"cmdat %s 'AT+CGDCONT=1,\"%s\",\"%s\"'",path_usb,type_ip_card2,value_apn_card2);
			memset(tmp_buff,0,sizeof(tmp_buff));
			cmd_recieve_str(cmd,NULL);
			
			if(strstr(value_module,"mt5700") || strstr(value_module,"mt5711"))
			{
				memset(cmd,0,sizeof(cmd));
				sprintf(cmd,"cmdat %s 'AT+CGDCONT=0,\"%s\",\"%s\"'",path_usb,type_ip_card2,value_apn_card2);
				memset(tmp_buff,0,sizeof(tmp_buff));
				cmd_recieve_str(cmd,NULL);
			}
			
		}
		if(strstr(value_module,"fm650") || strstr(value_module,"fm160") || strstr(value_module,"fg160"))		//广和通模组
		{
			//设定APN下的用户名、密码和认证方式
			if(strlen(value_username_card2) > 0)
			{
				memset(cmd,0,sizeof(cmd));
				sprintf(cmd,"cmdat %s 'AT+MGAUTH=1,%s,\"%s\",\"%s\"'",path_usb,value_auth_card2,value_username_card2,value_password_card2);
				cmd_recieve_str(cmd,NULL);
			}
			// if(strlen(value_apn_card2)>0)
			// {
				// memset(cmd,0,sizeof(cmd));
				// sprintf(cmd,"cmdat %s 'AT+CAVIMS?' | grep \"+CAVIMS:\" | awk -F ' ' '{print$NF}'",path_usb);
				// int cavims=cmd_recieve_int(cmd);
				// if(cavims==1)
				// {
					// memset(cmd,0,sizeof(cmd));
					// sprintf(cmd,"cmdat %s 'AT+CAVIMS=0'",path_usb);
					// cmd_recieve_str(cmd,NULL);
				// }
			// }
		}
		// else if(strstr(value_module,"rg520n"))											//移远RG520N模组
		// {
			// memset(cmd,0,sizeof(cmd));
			// sprintf(cmd,"cmdat %s 'AT+QCFG=\"ims\"' | grep \"+QCFG:\" | awk -F ',' '{print$(NF-1)}'",path_usb);
			// memset(tmp_buff,0,sizeof(tmp_buff));
			// cmd_recieve_str(cmd,tmp_buff);
			// if(!strstr(tmp_buff,"0"))
			// {
				// memset(cmd,0,sizeof(cmd));
				// sprintf(cmd,"cmdat %s 'AT+QCFG=\"ims\",0'",path_usb);
				// memset(tmp_buff,0,sizeof(tmp_buff));
				// cmd_recieve_str(cmd,tmp_buff);
			// }
		// }
		else if(strstr(value_module,"mt5700") || strstr(value_module,"mt5711"))
		{
			if(strlen(value_username_card2)>0)
			{
				//设定APN下的用户名、密码和认证方式
				memset(cmd,0,sizeof(cmd));
				sprintf(cmd,"cmdat %s 'AT^AUTHDATA=1,%s,\"%s\",\"%s\",\"%s\"'",path_usb,value_auth_card2,value_plmn_card2,value_password_card2,value_username_card2);
				cmd_recieve_str(cmd,NULL);
				
				memset(cmd,0,sizeof(cmd));
				sprintf(cmd,"cmdat %s 'AT^AUTHDATA=0,%s,\"%s\",\"%s\",\"%s\"'",path_usb,value_auth_card2,value_plmn_card2,value_password_card2,value_username_card2);
				cmd_recieve_str(cmd,NULL);
			}
		}
	}
	//设定服务类型
	if(strstr(value_module,"sim7600"))		//芯讯通4G模组
	{
		int tmp_mode_rat;
		if(mode_rat_card2==0)							//0：2 自动
			tmp_mode_rat=2;
		else if(mode_rat_card2==1)					//1: 14	仅3G
			tmp_mode_rat=14;
		else if(mode_rat_card2==2)					//2: 38	仅4G
			tmp_mode_rat=38;
		else if(mode_rat_card2==3)					//3：54	4G/3G
			tmp_mode_rat=54;
		else if(mode_rat_card2==5)					//5: 13	仅2G
			tmp_mode_rat=13;
		else if(mode_rat_card2==8)					//8: 19	2G/3G
			tmp_mode_rat=19;
		else if(mode_rat_card2==9)					//9: 51 2G/4G
			tmp_mode_rat=51;
		else if(mode_rat_card2==10)					//10: 39 2G/3G/4G
			tmp_mode_rat=39;
		else
			tmp_mode_rat=2;
		//设定sim7600模组上网服务类型
		setRATMODE_SIM7600(tmp_mode_rat);
	}
	else if(strstr(value_module,"sim8200"))		//芯讯通5G模组
	{
		int tmp_mode_rat;
		char tmp_band_wcdma[50];
		char tmp_band_lte[50];
		char tmp_band_nr[50];
		char tmp_band_nsa[50];
		memset(tmp_band_wcdma,0,sizeof(tmp_band_wcdma));
		memset(tmp_band_lte,0,sizeof(tmp_band_lte));
		memset(tmp_band_nr,0,sizeof(tmp_band_nr));
		memset(tmp_band_nsa,0,sizeof(tmp_band_nsa));
		if(mode_rat_card2==0)							//0：2   自动
			tmp_mode_rat=2;
		else if(mode_rat_card2==1)					//1: 14	 仅3G
		{
			tmp_mode_rat=14;
			strcpy(tmp_band_wcdma,band_wcdma);
		}
		else if(mode_rat_card2==2)					//2: 38	 仅4G
		{
			tmp_mode_rat=38;
			strcpy(tmp_band_lte,band_lte);
		}
		else if(mode_rat_card2==3)					//3：54	 4G/3G
		{
			tmp_mode_rat=54;
			strcpy(tmp_band_wcdma,band_wcdma);
			strcpy(tmp_band_lte,band_lte);
		}
		else if(mode_rat_card2==4)					//4: 71	 仅5G
		{
			tmp_mode_rat=71;
			strcpy(tmp_band_nr,band_nr);
			strcpy(tmp_band_nsa,band_nsa);
		}
		else if(mode_rat_card2==6)					//6: 109 5G/4G
		{
			tmp_mode_rat=109;
			strcpy(tmp_band_nr,band_nr);
			strcpy(tmp_band_nsa,band_nsa);
			strcpy(tmp_band_lte,band_lte);
		}
		else if(mode_rat_card2==7)					//7: 55 5G/4G/3G
		{
			tmp_mode_rat=55;
			strcpy(tmp_band_nr,band_nr);
			strcpy(tmp_band_nsa,band_nsa);
			strcpy(tmp_band_lte,band_lte);
			strcpy(tmp_band_wcdma,band_wcdma);
		}
		else
			tmp_mode_rat=2;
		//设定sim8200模组上网服务类型
		setRATMODE_SIM8200(tmp_mode_rat,tmp_band_wcdma,tmp_band_lte,tmp_band_nr,tmp_band_nsa);
	}
	else if(strstr(value_module,"fm650") || strstr(value_module,"fm160") || strstr(value_module,"fg160"))		//广和通模组
	{
		char tmp_mode_rat[100];
		char tmp_band_wcdma[50];
		char tmp_band_lte[50];
		char tmp_band_nr[50];
		memset(tmp_band_wcdma,0,sizeof(tmp_band_wcdma));
		memset(tmp_band_lte,0,sizeof(tmp_band_lte));
		memset(tmp_band_nr,0,sizeof(tmp_band_nr));
		memset(tmp_mode_rat,0,sizeof(tmp_mode_rat));
		if(mode_rat_card2==0)							//0：10 自动
		{	
			strcpy(tmp_mode_rat,"20");
		}
		else if(mode_rat_card2==1)					//1: 1	仅3G
		{	
			strcpy(tmp_mode_rat,"1");
			strcpy(tmp_band_wcdma,band_wcdma_card2);
		}
		else if(mode_rat_card2==2)					//2: 2	仅4G
		{	
			strcpy(tmp_mode_rat,"2");
			strcpy(tmp_band_lte,band_lte_card2);
		}
		else if(mode_rat_card2==3)					//3：4	4G/3G
		{	
			strcpy(tmp_mode_rat,"4");
			strcpy(tmp_band_wcdma,band_wcdma_card2);
			strcpy(tmp_band_lte,band_lte_card2);
		}
		else if(mode_rat_card2==4)					//4: 14	仅5G
		{	
			strcpy(tmp_mode_rat,"14");
			strcpy(tmp_band_nr,band_nr_card2);
		}
		else if(mode_rat_card2==6)					//6: 17	5G/4G
		{	
			strcpy(tmp_mode_rat,"17");
			strcpy(tmp_band_lte,band_lte_card2);
			strcpy(tmp_band_nr,band_nr_card2);
		}
		else if(mode_rat_card2==7)					//7: 20 5G/4G/3G
		{	
			strcpy(tmp_mode_rat,"20");
			strcpy(tmp_band_lte,band_lte_card2);
			strcpy(tmp_band_nr,band_nr_card2);
			strcpy(tmp_band_wcdma,band_wcdma_card2);
		}
		else
		{	
			strcpy(tmp_mode_rat,"20");
		}
		//设定fm160 fm650模组频点和小区
		
		setRATMODE_FM160(tmp_mode_rat,tmp_band_wcdma,tmp_band_lte,tmp_band_nr);
		if(((mode_rat_card2==4) || (mode_rat_card2==2))&& strlen(value_arfcn_card2)>0)
		{
			if(mode_rat_card2==4)
			{
				set5GCELLLOCK_FM650(tmp_band_nr,value_arfcn_card2,value_psc_card2,value_scs_card2);
			}
			else
			{
				setCELLLOCK_FM650(value_arfcn_card2,value_pci_card2);
			}
			
		}
		else
			resetCELLLOCK_FM650();
	}
	else if(strstr(value_module,"nl668"))
	{
		char tmp_mode_rat[100];
		char tmp_band_cdma[50];
		char tmp_band_lte[50];
		char tmp_band_gsm[50];
		char tmp_band_umts[50];
		char tmp_band_scdma[50];
		memset(tmp_band_cdma,0,sizeof(tmp_band_cdma));
		memset(tmp_band_lte,0,sizeof(tmp_band_lte));
		memset(tmp_band_gsm,0,sizeof(tmp_band_gsm));
		memset(tmp_band_umts,0,sizeof(tmp_band_umts));
		memset(tmp_band_scdma,0,sizeof(tmp_band_scdma));
		memset(tmp_mode_rat,0,sizeof(tmp_mode_rat));
		if(mode_rat_card2==0)							//0：10 自动
		{	
			strcpy(tmp_mode_rat,"10");
		}
		else if(mode_rat_card2==1)					//1: 0	GSM
		{	
			strcpy(tmp_mode_rat,"0");
			strcpy(tmp_band_gsm,band_gsm_card2);
		}
		else if(mode_rat_card2==2)					//2: 1	UMTS
		{	
			strcpy(tmp_mode_rat,"1");
			strcpy(tmp_band_umts,band_umts_card2);
		}
		else if(mode_rat_card2==3)					//3：2	LTE
		{	
			strcpy(tmp_mode_rat,"2");
			strcpy(tmp_band_lte,band_lte_card2);
		}
		else if(mode_rat_card2==4)					//4: 3	GSM UMTS
		{	
			strcpy(tmp_mode_rat,"3");
			strcpy(tmp_band_gsm,band_gsm_card2);
			strcpy(tmp_band_umts,band_umts_card2);
		}
		else if(mode_rat_card2==5)					//5: 4	LTE UMTS
		{	
			strcpy(tmp_mode_rat,"4");
			strcpy(tmp_band_lte,band_lte_card2);
			strcpy(tmp_band_umts,band_umts_card2);
		}
		else if(mode_rat_card2==6)					//6: 5 	LTE GSM
		{	
			strcpy(tmp_mode_rat,"5");
			strcpy(tmp_band_lte,band_lte_card2);
			strcpy(tmp_band_gsm,band_gsm_card2);
		}
		else if(mode_rat_card2==7)					//7: 6 	LTE GSM UMTS
		{	
			strcpy(tmp_mode_rat,"6");
			strcpy(tmp_band_lte,band_lte_card2);
			strcpy(tmp_band_gsm,band_gsm_card2);
			strcpy(tmp_band_umts,band_umts_card2);
		}
		else if(mode_rat_card2==8)					//8: 7 TD_SCDMA
		{	
			strcpy(tmp_mode_rat,"7");
			strcpy(tmp_band_scdma,band_scdma_card2);
		}
		else if(mode_rat_card2==11)					//11: 11 CDMA
		{	
			strcpy(tmp_mode_rat,"11");
			strcpy(tmp_band_cdma,band_cdma_card2);
		}
		else if(mode_rat_card2==12)					//12: 12 CDMA EVDO
		{	
			strcpy(tmp_mode_rat,"12");
			strcpy(tmp_band_cdma,band_cdma_card2);
		}
		else if(mode_rat_card2==13)					//13: 13 EVDO
		{	
			strcpy(tmp_mode_rat,"13");
			strcpy(tmp_band_cdma,band_cdma_card2);
		}
		else
		{	
			strcpy(tmp_mode_rat,"10");
		}
		setRATMODE_NL668(tmp_mode_rat,tmp_band_gsm,tmp_band_umts,tmp_band_lte,tmp_band_scdma,tmp_band_cdma);
		if((mode_rat_card2==3)&& strlen(value_arfcn_card2)>0)
		{
			setCELLLOCK_FM650(value_arfcn_card2,value_pci_card2);
		}
		else
			resetCELLLOCK_FM650();
	}
	else if(strstr(value_module,"rm500u"))
	{
		char tmp_mode_rat[100];
		char tmp_band_wcdma[50];
		char tmp_band_lte[50];
		char tmp_band_nr[50];
		char tmp_band_nsa[50];
		memset(tmp_band_wcdma,0,sizeof(tmp_band_wcdma));
		memset(tmp_band_lte,0,sizeof(tmp_band_lte));
		memset(tmp_band_nr,0,sizeof(tmp_band_nr));
		memset(tmp_band_nsa,0,sizeof(tmp_band_nsa));
		memset(tmp_mode_rat,0,sizeof(tmp_mode_rat));
		if(mode_rat_card2==0)							//0：20 自动
			strcpy(tmp_mode_rat,"AUTO");
		else if(mode_rat_card2==1)					//1: 2	仅3G
		{
			strcpy(tmp_mode_rat,"WCDMA");
			strcpy(tmp_band_wcdma,band_wcdma_card2);
		}
		else if(mode_rat_card2==2)					//2: 3	仅4G
		{
			strcpy(tmp_mode_rat,"LTE");
			strcpy(tmp_band_lte,band_lte_card2);
		}
		else if(mode_rat_card2==3)					//3：4	4G/3G
		{
			strcpy(tmp_mode_rat,"LTE:WCDMA");
			strcpy(tmp_band_wcdma,band_wcdma_card2);
			strcpy(tmp_band_lte,band_lte_card2);
		}
		else if(mode_rat_card2==4)					//4: 14	仅5G
		{
			strcpy(tmp_mode_rat,"NR5G");
			strcpy(tmp_band_nr,band_nr_card2);
		}
		else if(mode_rat_card2==6)					//6: 16	5G/4G
		{
			strcpy(tmp_mode_rat,"NR5G:LTE");
			strcpy(tmp_band_lte,band_lte_card2);
			strcpy(tmp_band_nr,band_nr_card2);
		}
		else if(mode_rat_card2==7)					//7: 20 5G/4G/3G
		{
			strcpy(tmp_mode_rat,"NR5G:LTE:WCDMA");
			strcpy(tmp_band_wcdma,band_wcdma_card2);
			strcpy(tmp_band_lte,band_lte_card2);
			strcpy(tmp_band_nr,band_nr_card2);
		}
		else
			strcpy(tmp_mode_rat,"AUTO");
		setRATMODE_RM500U(tmp_mode_rat,tmp_band_wcdma,tmp_band_lte,tmp_band_nr);
	}	
	else if(strstr(value_module,"rm500q"))
	{
		char tmp_mode_rat[100];
		char tmp_band_wcdma[50];
		char tmp_band_lte[50];
		char tmp_band_nr[50];
		char tmp_band_nsa[50];
		memset(tmp_band_wcdma,0,sizeof(tmp_band_wcdma));
		memset(tmp_band_lte,0,sizeof(tmp_band_lte));
		memset(tmp_band_nr,0,sizeof(tmp_band_nr));
		memset(tmp_band_nsa,0,sizeof(tmp_band_nsa));
		memset(tmp_mode_rat,0,sizeof(tmp_mode_rat));
		if(mode_rat_card2==0)							//0：20 自动
			strcpy(tmp_mode_rat,"AUTO");
		else if(mode_rat_card2==1)					//1: 2	仅3G
		{
			strcpy(tmp_mode_rat,"WCDMA");
			strcpy(tmp_band_wcdma,band_wcdma_card2);
		}
		else if(mode_rat_card2==2)					//2: 3	仅4G
		{
			strcpy(tmp_mode_rat,"LTE");
			strcpy(tmp_band_lte,band_lte_card2);
		}
		else if(mode_rat_card2==3)					//3：4	4G/3G
		{
			strcpy(tmp_mode_rat,"LTE:WCDMA");
			strcpy(tmp_band_wcdma,band_wcdma_card2);
			strcpy(tmp_band_lte,band_lte_card2);
		}
		else if(mode_rat_card2==4)					//4: 14	仅5G
		{
			strcpy(tmp_mode_rat,"NR5G");
			strcpy(tmp_band_nr,band_nr_card2);
		}
		else if(mode_rat_card2==6)					//6: 16	5G/4G
		{
			strcpy(tmp_mode_rat,"NR5G:LTE");
			strcpy(tmp_band_lte,band_lte_card2);
			strcpy(tmp_band_nr,band_nr_card2);
		}
		else if(mode_rat_card2==7)					//7: 20 5G/4G/3G
		{
			strcpy(tmp_mode_rat,"NR5G:LTE:WCDMA");
			strcpy(tmp_band_wcdma,band_wcdma_card2);
			strcpy(tmp_band_lte,band_lte_card2);
			strcpy(tmp_band_nr,band_nr_card2);
		}
		else
			strcpy(tmp_mode_rat,"AUTO");
		setRATMODE_RM500Q(tmp_mode_rat,tmp_band_wcdma,tmp_band_lte,tmp_band_nr);
	}
	else if(strstr(value_module,"rg520n"))
	{
		char tmp_mode_rat[100];
		char tmp_band_wcdma[50];
		char tmp_band_lte[50];
		char tmp_band_nr[50];
		char tmp_band_nsa[50];
		memset(tmp_band_wcdma,0,sizeof(tmp_band_wcdma));
		memset(tmp_band_lte,0,sizeof(tmp_band_lte));
		memset(tmp_band_nr,0,sizeof(tmp_band_nr));
		memset(tmp_band_nsa,0,sizeof(tmp_band_nsa));
		memset(tmp_mode_rat,0,sizeof(tmp_mode_rat));
		if(mode_rat_card2==0)							//0：20 自动
		{
			strcpy(tmp_mode_rat,"AUTO");
			strcpy(tmp_band_wcdma,"0");
			strcpy(tmp_band_lte,"0");
			strcpy(tmp_band_nr,"0");
		}
		else if(mode_rat_card2==1)					//1: 2	仅3G
		{
			strcpy(tmp_mode_rat,"WCDMA");
			strcpy(tmp_band_wcdma,band_wcdma_card2);
		}
		else if(mode_rat_card2==2)					//2: 3	仅4G
		{
			strcpy(tmp_mode_rat,"LTE");
			strcpy(tmp_band_lte,band_lte_card2);
		}
		else if(mode_rat_card2==3)					//3：4	4G/3G
		{
			strcpy(tmp_mode_rat,"LTE:WCDMA");
			strcpy(tmp_band_wcdma,band_wcdma_card2);
			strcpy(tmp_band_lte,band_lte_card2);
		}
		else if(mode_rat_card2==4)					//4: 14	仅5G
		{
			strcpy(tmp_mode_rat,"NR5G");
			strcpy(tmp_band_nr,band_nr_card2);
		}
		else if(mode_rat_card2==6)					//6: 16	5G/4G
		{
			strcpy(tmp_mode_rat,"NR5G:LTE");
			strcpy(tmp_band_lte,band_lte_card2);
			strcpy(tmp_band_nr,band_nr_card2);
		}
		else if(mode_rat_card2==7)					//7: 20 5G/4G/3G
		{
			strcpy(tmp_mode_rat,"NR5G:LTE:WCDMA");
			strcpy(tmp_band_wcdma,band_wcdma_card2);
			strcpy(tmp_band_lte,band_lte_card2);
			strcpy(tmp_band_nr,band_nr_card2);
		}
		else
			strcpy(tmp_mode_rat,"AUTO");
		
		setRATMODE_RG520N(tmp_mode_rat,tmp_band_wcdma,tmp_band_lte,tmp_band_nr,tmp_band_nsa,value_arfcn_card2);
		if(((mode_rat_card2==4) || (mode_rat_card2==2))&& strlen(value_arfcn_card2)>0)
		{
			if(mode_rat_card2==4)
			{
				set5GCELLLOCK_RG520N(tmp_band_nr,value_arfcn_card2,value_psc_card2,value_scs_card2);
			}
			else
			{
				setCELLLOCK_RG520N(value_arfcn_card2,value_pci_card2);
			}
		}
		else
		{
			reset_RG520N();
		}
	}
	else if(strstr(value_module,"mt5700"))		//海思模组
	{
		char tmp_mode_rat[100];
		char tmp_band_wcdma[50];
		char tmp_band_lte[50];
		char tmp_band_nr[50];
		memset(tmp_band_wcdma,0,sizeof(tmp_band_wcdma));
		memset(tmp_band_lte,0,sizeof(tmp_band_lte));
		memset(tmp_band_nr,0,sizeof(tmp_band_nr));
		memset(tmp_mode_rat,0,sizeof(tmp_mode_rat));
		if(mode_rat_card2==0)							//0：10 自动
		{	
			strcpy(tmp_mode_rat,"080302");
		}
		else if(mode_rat_card2==1)					//1: 1	仅3G
		{	
			strcpy(tmp_mode_rat,"02");
			strcpy(tmp_band_wcdma,band_wcdma_card2);
		}
		else if(mode_rat_card2==2)					//2: 2	仅4G
		{	
			strcpy(tmp_mode_rat,"03");
			strcpy(tmp_band_lte,band_lte_card2);
		}
		else if(mode_rat_card2==3)					//3：4	4G/3G
		{	
			strcpy(tmp_mode_rat,"0203");
			strcpy(tmp_band_wcdma,band_wcdma_card2);
			strcpy(tmp_band_lte,band_lte_card2);
		}
		else if(mode_rat_card2==4)					//4: 14	仅5G
		{	
			strcpy(tmp_mode_rat,"08");
			strcpy(tmp_band_nr,band_nr_card2);
		}
		else if(mode_rat_card2==6)					//6: 17	5G/4G
		{	
			strcpy(tmp_mode_rat,"0803");
			strcpy(tmp_band_lte,band_lte_card2);
			strcpy(tmp_band_nr,band_nr_card2);
		}
		else if(mode_rat_card2==7)					//7: 20 5G/4G/3G
		{
			strcpy(tmp_mode_rat,"080302");
			strcpy(tmp_band_lte,band_lte_card2);
			strcpy(tmp_band_nr,band_nr_card2);
			strcpy(tmp_band_wcdma,band_wcdma_card2);
		}
		else
		{	
			strcpy(tmp_mode_rat,"0803");
		}
		//设定mt700模组频点和小区
		setRATMODE_MT5700(tmp_mode_rat,tmp_band_wcdma,tmp_band_lte,tmp_band_nr,value_arfcn_card2,value_psc_card2,value_scs_card2,value_pci_card2);
		/* if(((mode_rat_card2==4) || (mode_rat_card2==2))&& strlen(value_arfcn_card2)>0)
		{
			if(mode_rat_card2==4)
			{
				set5GCELLLOCK_MT5700(tmp_band_nr,value_arfcn_card2,value_psc_card2,value_scs_card2);
			}
			else
			{
				setCELLLOCK_MT5700(value_arfcn_card2,value_pci_card2);
			}
		}
		else
			resetCELLLOCK_MT5700(); */
	}
	else if(strstr(value_module,"mt5711"))		//海思模组
	{
		char tmp_mode_rat[100];
		char tmp_band_wcdma[50];
		char tmp_band_lte[50];
		char tmp_band_nr[50];
		memset(tmp_band_wcdma,0,sizeof(tmp_band_wcdma));
		memset(tmp_band_lte,0,sizeof(tmp_band_lte));
		memset(tmp_band_nr,0,sizeof(tmp_band_nr));
		memset(tmp_mode_rat,0,sizeof(tmp_mode_rat));
		if(mode_rat_card2==0)							//0：10 自动
		{	
			strcpy(tmp_mode_rat,"00");
		}
		else if(mode_rat_card2==1)					//1: 1	仅3G
		{	
			strcpy(tmp_mode_rat,"02");
			strcpy(tmp_band_wcdma,band_wcdma_card2);
		}
		else if(mode_rat_card2==2)					//2: 2	仅4G
		{	
			strcpy(tmp_mode_rat,"03");
			strcpy(tmp_band_lte,band_lte_card2);
		}
		else if(mode_rat_card2==3)					//3：4	4G/3G
		{	
			strcpy(tmp_mode_rat,"0203");
			strcpy(tmp_band_wcdma,band_wcdma_card2);
			strcpy(tmp_band_lte,band_lte_card2);
		}
		else if(mode_rat_card2==4)					//4: 14	仅5G
		{	
			strcpy(tmp_mode_rat,"08");
			strcpy(tmp_band_nr,band_nr_card2);
		}
		else if(mode_rat_card2==6)					//6: 17	5G/4G
		{	
			strcpy(tmp_mode_rat,"00");
			strcpy(tmp_band_lte,band_lte_card2);
			strcpy(tmp_band_nr,band_nr_card2);
		}
		else if(mode_rat_card2==7)					//7: 20 5G/4G/3G
		{
			strcpy(tmp_mode_rat,"00");
			strcpy(tmp_band_lte,band_lte_card2);
			strcpy(tmp_band_nr,band_nr_card2);
			strcpy(tmp_band_wcdma,band_wcdma_card2);
		}
		else
		{	
			strcpy(tmp_mode_rat,"0803");
		}
		//设定mt700模组频点和小区
		setRATMODE_MT5700(tmp_mode_rat,tmp_band_wcdma,tmp_band_lte,tmp_band_nr,value_arfcn_card2,value_psc_card2,value_scs_card2,value_pci_card2);
		/* if(((mode_rat_card2==4) || (mode_rat_card2==2))&& strlen(value_arfcn_card2)>0)
		{
			if(mode_rat_card2==4)
			{
				set5GCELLLOCK_MT5700(tmp_band_nr,value_arfcn_card2,value_psc_card2,value_scs_card2);
			}
			else
			{
				setCELLLOCK_MT5700(value_arfcn_card2,value_pci_card2);
			}
		}
		else
			resetCELLLOCK_MT5700(); */
	}
	else if(strstr(value_module,"ec20-eux"))
	{
		char tmp_mode_rat[100];
		char tmp_band_wcdma[50];
		char tmp_band_lte[50];
		char tmp_band_gsm[50];
		memset(tmp_band_wcdma,0,sizeof(tmp_band_wcdma));
		memset(tmp_band_lte,0,sizeof(tmp_band_lte));
		memset(tmp_band_gsm,0,sizeof(tmp_band_gsm));
		if(mode_rat_card2==0)						//0：0 自动
		{	
			strcpy(tmp_mode_rat,"0");
		}
		else if(mode_rat_card2==1)					//1: 1	GSM
		{	
			strcpy(tmp_mode_rat,"1");
			strcpy(tmp_band_gsm,band_gsm_card2);
		}
		else if(mode_rat_card2==2)					//2: 2	WCDMA
		{	
			strcpy(tmp_mode_rat,"2");
			strcpy(tmp_band_wcdma,band_wcdma_card2);
		}
		else if(mode_rat_card2==3)					//3：3	LTE
		{	
			strcpy(tmp_mode_rat,"3");
			strcpy(tmp_band_lte,band_lte_card2);
		}
		else
		{	
			strcpy(tmp_mode_rat,"0");
		}
		setRATMODE_EC20(tmp_mode_rat,tmp_band_gsm,tmp_band_wcdma,tmp_band_lte);
	}
	else if(strstr(value_module,"ec20-af"))
	{
		char tmp_mode_rat[100];
		char tmp_band_wcdma[50];
		char tmp_band_lte[50];
		char tmp_band_gsm[50];
		memset(tmp_band_wcdma,0,sizeof(tmp_band_wcdma));
		memset(tmp_band_lte,0,sizeof(tmp_band_lte));
		memset(tmp_band_gsm,0,sizeof(tmp_band_gsm));
		if(mode_rat_card2==0)						//0：0 自动
		{	
			strcpy(tmp_mode_rat,"0");
		}
		else if(mode_rat_card2==2)					//2: 2	WCDMA
		{	
			strcpy(tmp_mode_rat,"2");
			strcpy(tmp_band_wcdma,band_wcdma_card2);
		}
		else if(mode_rat_card2==3)					//3：3	LTE
		{	
			strcpy(tmp_mode_rat,"3");
			strcpy(tmp_band_lte,band_lte_card2);
		}
		else
		{	
			strcpy(tmp_mode_rat,"0");
		}
		setRATMODE_EC20(tmp_mode_rat,tmp_band_gsm,tmp_band_wcdma,tmp_band_lte);
	}
}

/* 功能：拨号
** 返回值：void
*/
void Dial()
{
	//开启拨号功能
	if(flag_dial==1)
	{
		char cmd[1024];
		char value_ip_dial[500];
		char value_ip_ifconfig[500];
		//信号格大于2或者开启强制拨号时进行拨号
		if(value_signal>=2 || flag_forcedial == 1)
		{
			if(value_signal>=2)
			{
				sig_flight=0;
			}
			if(strstr(value_module,"sim7600"))		//芯讯通4G模组
			{
				memset(value_ip_ifconfig,0,sizeof(value_ip_ifconfig));
				//获取模组拨号状态
				memset(cmd,0,sizeof(cmd));
				sprintf(cmd,"cmdat %s 'AT$QCRMCALL?' 1000 | grep \"$QCRMCALL:\" | grep \"V4\" | wc -l",path_usb);
				int result=cmd_recieve_int(cmd);
				//未拨号
				if(result == 0)
				{
					//对sim7600模组进行拨号
					if(dialip_debug==0)
						Debug("initiate a dial-up request","daemon.warn");
					Dial_SIM7600();
				}
				//获取网卡IP
				memset(cmd,0,sizeof(cmd));
				sprintf(cmd,"ifconfig %s | grep \"inet addr\" | awk -F ':' '{print$2}' | awk -F ' ' '{print$1}'",value_ifname);
				cmd_recieve_str(cmd,value_ip_ifconfig);
				//不存在网卡IP时，重启网卡
				if(strlen(value_ip_ifconfig)==0)
				{
					cmd_recieve_str(cmd_ifup,NULL);
				}
			}
			else if(strstr(value_module,"sim8200"))		//芯讯通5G模组
			{
				memset(value_ip_ifconfig,0,sizeof(value_ip_ifconfig));
				memset(cmd,0,sizeof(cmd));
				sprintf(cmd,"ifconfig %s | grep \"inet addr\" | awk -F ':' '{print$2}' | awk -F ' ' '{print$1}'",value_ifname);
				cmd_recieve_str(cmd,value_ip_ifconfig);
				//不存在网卡IP时，尝试拨号
				if(value_ip_ifconfig == NULL || strlen(value_ip_ifconfig)==0)
				{
					if(dialip_debug==0){
						Debug("initiate a dial-up request","daemon.warn");
						dialip_debug=1;
					}
					int pid=0;
					memset(cmd,0,sizeof(cmd));
					sprintf(cmd,"ps | grep simcom-cm | grep -v grep | wc -l");
					pid=cmd_recieve_int(cmd);
					if(pid==1)
					{
						Debug("simcim-cm is running","daemon.warn");
						setFlightmode();
					}
					else{
						Dial_SIM8200();
						sleep(5);
						cmd_recieve_str(cmd_ifup,NULL);
					}
				}
				else
				{
					dialip_debug=0;
				}
				int route;
				memset(cmd,0,sizeof(cmd));
				sprintf(cmd,"route | grep default | grep %s | sed -n 1p | awk -F ' ' '{print $5}'",value_ifname);
				route=cmd_recieve_int(cmd);
				if(route==0)
				{
					cmd_recieve_str(cmd_ifup,NULL);
				}
			}
			else if(strstr(value_module,"fm650"))		//广和通模组
			{
				
				memset(value_ip_dial,0,sizeof(value_ip_dial));
				memset(value_ip_ifconfig,0,sizeof(value_ip_ifconfig));
				//获取模组拨号IP
				memset(cmd,0,sizeof(cmd));
				sprintf(cmd,"cmdat %s AT+GTRNDIS? | grep \"+GTRNDIS: 1\" | awk -F ',' '{print$3}'| awk -F '\"' '{print$2}'",path_usb);
				cmd_recieve_str(cmd,value_ip_dial);
				//不存在拨号IP，尝试拨号
				if(strlen(value_ip_dial)==0)
				{
					//对fm650模组进行拨号
					if(dialip_debug==0){
						Debug("initiate a dial-up request","daemon.warn");
						dialip_debug=1;
						memset(dialresult_debug,0,sizeof(dialresult_debug));
						strcpy(dialresult_debug,"null");
					}
					Dial_FM650();
				}
				else
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
					dialip_debug=0;
					dial_flight=0;
					//memset(dialresult_debug,0,sizeof(dialresult_debug));
				}
				//获取网卡IP
				memset(cmd,0,sizeof(cmd));
				sprintf(cmd,"ifconfig %s | grep \"inet addr\" | awk -F ':' '{print$2}' | awk -F ' ' '{print$1}'",value_ifname);
				cmd_recieve_str(cmd,value_ip_ifconfig);
				//不存在网卡IP或模组IP和网卡IP不一致时，重启网卡
				if(strlen(value_ip_ifconfig)==0 || !strstr(value_ip_dial,value_ip_ifconfig))
				{
					cmd_recieve_str(cmd_ifup,NULL);
				}
			}
			else if(strstr(value_module,"fm160"))
			{
				
				if(strcmp(name_config,"SIM2")==0) //&& isPCIE())
				{
					memset(value_ip_ifconfig,0,sizeof(value_ip_ifconfig));
					memset(cmd,0,sizeof(cmd));
					sprintf(cmd,"ifconfig %s | grep \"inet addr\" | awk -F ':' '{print$2}' | awk -F ' ' '{print$1}'",value_ifname);
					cmd_recieve_str(cmd,value_ip_ifconfig);
					//不存在网卡IP时，尝试拨号
					//Debug(value_ip_ifconfig,"daemon.warn");
					if(value_ip_ifconfig == NULL || strlen(value_ip_ifconfig)==0)
					{
						if(dialip_debug==0){
							Debug("initiate a dial-up request","daemon.warn");
							
							dialip_debug=1;
							memset(dialresult_debug,0,sizeof(dialresult_debug));
							strcpy(dialresult_debug,"null");
						}
						int pid=0;
						memset(cmd,0,sizeof(cmd));
						sprintf(cmd,"ps | grep fibocom-dial | grep -v grep");
						pid=cmd_recieve_int(cmd);
						if(pid>0)
						{
							setFlightmode();
							cmd_recieve_str(cmd_ifup,NULL);
						}
						else{
							if(strlen(value_defaultcard)>0)
							{
								memset(cmd,0,sizeof(cmd));
								sprintf(cmd,"cat /sys/class/leds/sim_sw/brightness"); //获取当前卡槽
								int value_currentcard = cmd_recieve_int(cmd); 
								if (value_currentcard == 255)
									value_currentcard = 1;
								if(value_currentcard == 0) 			//[当前卡槽] 0
									Dial_FM160_PCI(value_apn_card2,value_username_card2,value_password_card2,value_auth_card2,type_ip_card2);
								else if(value_currentcard == 1)		//[当前卡槽] 1
									Dial_FM160_PCI(value_apn_card1,value_username_card1,value_password_card1,value_auth_card1,type_ip_card1);
							}
							else
							{
								Dial_FM160_PCI(value_apn,value_username,value_password,value_auth,type_ip);
							}
							sleep(3);
							cmd_recieve_str(cmd_ifup,NULL);
							//system("/etc/init.d/dnsmasq restart &");
						}
					}
					else
					{
						if(strcmp(dialresult_debug,"yes")!=0)
						{
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
						dialip_debug=0;
						dial_flight=0;
					}
					int route;
					memset(cmd,0,sizeof(cmd));
					sprintf(cmd,"route | grep default | grep %s | sed -n 1p | awk -F ' ' '{print $5}'",value_ifname);
					route=cmd_recieve_int(cmd);
					if(route==0)
					{
						cmd_recieve_str(cmd_ifup,NULL);
					}
				}
				else
				{
					memset(value_ip_dial,0,sizeof(value_ip_dial));
					
					//获取模组拨号IP
					memset(cmd,0,sizeof(cmd));
					sprintf(cmd,"cmdat %s AT+GTRNDIS? | grep \"+GTRNDIS: 1\" | awk -F ',' '{print$3}'| awk -F '\"' '{print$2}'",path_usb);
					cmd_recieve_str(cmd,value_ip_dial);
					//不存在拨号IP，尝试拨号
					if(strlen(value_ip_dial)==0)
					{
						//对fm160模组进行拨号
						if(dialip_debug==0){
							Debug("initiate a dial-up request","daemon.warn");
							dialip_debug=1;
							memset(dialresult_debug,0,sizeof(dialresult_debug));
							strcpy(dialresult_debug,"null");
						}
						Dial_FM160();
					}
					else
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
						dialip_debug=0;
						dial_flight=0;
						//memset(dialresult_debug,0,sizeof(dialresult_debug));
					}
					//获取网卡IP
					memset(cmd,0,sizeof(cmd));
					memset(value_ip_ifconfig,0,sizeof(value_ip_ifconfig));
					sprintf(cmd,"ifconfig %s | grep \"inet addr\" | awk -F ':' '{print$2}' | awk -F ' ' '{print$1}'",value_ifname);
					cmd_recieve_str(cmd,value_ip_ifconfig);
						
					if(strlen(value_ip_dial)>0 && strlen(value_ip_ifconfig)<0)
					{
						cmd_recieve_str(cmd_ifup,NULL);
						setFlightmode();
						memset(dialresult_debug,0,sizeof(dialresult_debug));
						strcpy(dialresult_debug,"null");
						dialip_debug=0;
						dial_flight=0;
					}
					if(!strstr(value_ip_dial,value_ip_ifconfig))
					{
						cmd_recieve_str(cmd_ifup,NULL);
						setFlightmode();
						memset(dialresult_debug,0,sizeof(dialresult_debug));
						strcpy(dialresult_debug,"null");
						dialip_debug=0;
						dial_flight=0;
					}
				}
				
			}
			else if(strstr(value_module,"fg160"))
			{
				
				//if(strcmp(name_config,"SIM2")==0 && isPCIE())
				{
					memset(value_ip_ifconfig,0,sizeof(value_ip_ifconfig));
					memset(cmd,0,sizeof(cmd));
					sprintf(cmd,"ifconfig %s | grep \"inet addr\" | awk -F ':' '{print$2}' | awk -F ' ' '{print$1}'",value_ifname);
					cmd_recieve_str(cmd,value_ip_ifconfig);
					//不存在网卡IP时，尝试拨号
					int pid=0;
					memset(cmd,0,sizeof(cmd));
					sprintf(cmd,"ps | grep fibocom-dial | grep -v grep");
					pid=cmd_recieve_int(cmd);
					if(value_ip_ifconfig == NULL || strlen(value_ip_ifconfig)==0 || pid<=0)
					{
						if(dialip_debug==0){
							Debug("initiate a dial-up request","daemon.warn");
							dialip_debug=1;
							memset(dialresult_debug,0,sizeof(dialresult_debug));
							strcpy(dialresult_debug,"null");
						}
						
						if(pid>0)
						{
							setFlightmode();
							cmd_recieve_str(cmd_ifup,NULL);
						}
						else{
							if(strlen(value_defaultcard)>0)
							{
								memset(cmd,0,sizeof(cmd));
								sprintf(cmd,"cat /sys/class/leds/sim_sw/brightness"); //获取当前卡槽
								int value_currentcard = cmd_recieve_int(cmd); 
								if (value_currentcard == 255)
									value_currentcard = 1;
								if(value_currentcard == 0) 			//[当前卡槽] 0——SIM1
									Dial_FM160_PCI(value_apn_card1,value_username_card1,value_password_card1,value_auth_card1,type_ip_card1);
								else if(value_currentcard == 1)		//[当前卡槽] 1——SIM2
									Dial_FM160_PCI(value_apn_card2,value_username_card2,value_password_card2,value_auth_card2,type_ip_card2);
							}
							else
							{
								Dial_FM160_PCI(value_apn,value_username,value_password,value_auth,type_ip);
							}
							sleep(3);
							cmd_recieve_str(cmd_ifup,NULL);
							//system("/etc/init.d/dnsmasq restart &");
						}
					}
					else
					{
						if(strcmp(dialresult_debug,"yes")!=0)
						{
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
						dialip_debug=0;
						dial_flight=0;
					}
					// int route;
					// memset(cmd,0,sizeof(cmd));
					// sprintf(cmd,"route | grep default | grep %s | sed -n 1p | awk -F ' ' '{print $5}'",value_ifname);
					// route=cmd_recieve_int(cmd);
					// if(route==0)
					// {
						// cmd_recieve_str(cmd_ifup,NULL);
					// }
					/* memset(cmd,0,sizeof(cmd));
					sprintf(cmd,"route | grep default | grep %s | wc -l",value_ifname);
					route=cmd_recieve_int(cmd);
					if(route==0)
					{
						setFlightmode();
					} */
				}
			}
			else if(strstr(value_module,"nl668"))		//广和通模组
			{
				memset(value_ip_dial,0,sizeof(value_ip_dial));
				memset(value_ip_ifconfig,0,sizeof(value_ip_ifconfig));
				//获取模组拨号IP
				memset(cmd,0,sizeof(cmd));
				sprintf(cmd,"cmdat %s AT+GTRNDIS? | grep \"+GTRNDIS: 1\" | awk -F ',' '{print$3}'| awk -F '\"' '{print$2}'",path_usb);
				cmd_recieve_str(cmd,value_ip_dial);
				//不存在拨号IP，尝试拨号
				if(strlen(value_ip_dial)==0)
				{
					//对fm650模组进行拨号
					if(dialip_debug==0){
						Debug("initiate a dial-up request","daemon.warn");
						dialip_debug=1;
						memset(dialresult_debug,0,sizeof(dialresult_debug));
						strcpy(dialresult_debug,"null");
					}
					memset(cmd,0,sizeof(cmd));
					sprintf(cmd,"ifconfig %s up",value_ifname);
					system(cmd);
					system(cmd_ifup);
					//Debug(cmd,"daemon.debug");
					Dial_NL668();
				}
				else
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
					dialip_debug=0;
				}
				//获取网卡IP
				memset(cmd,0,sizeof(cmd));
				sprintf(cmd,"ifconfig %s | grep \"inet addr\" | awk -F ':' '{print$2}' | awk -F ' ' '{print$1}'",value_ifname);
				cmd_recieve_str(cmd,value_ip_ifconfig);
				//不存在网卡IP或模组IP和网卡IP不一致时，重启网卡
				if(strlen(value_ip_ifconfig)==0)
				{
					memset(cmd,0,sizeof(cmd));
					sprintf(cmd,"ifconfig %s up",value_ifname);
					system(cmd);
				}
				if(strlen(value_ip_ifconfig)==0 || !strstr(value_ip_dial,value_ip_ifconfig))
				{
					if(strlen(mode_nat)<=0 || strstr(mode_nat,"0"))
						cmd_recieve_str(cmd_ifup,NULL);
				}
				
			}
			else if(strstr(value_module,"rm500u"))		//移远模组
			{
				memset(value_ip_dial,0,sizeof(value_ip_dial));
				memset(value_ip_ifconfig,0,sizeof(value_ip_ifconfig));
				//获取模组拨号IP
				memset(cmd,0,sizeof(cmd));
				sprintf(cmd,"cmdat %s AT+QNETDEVSTATUS=1 | grep \"+QNETDEVSTATUS:\" | awk -F ',' '{print$1}'| awk -F ' ' '{print$2}'",path_usb);
				cmd_recieve_str(cmd,value_ip_dial);
				//不存在拨号IP，尝试拨号
				if(strlen(value_ip_dial)==0)
				{
					//对fm650模组进行拨号
					if(dialip_debug==0)
					Debug("initiate a dial-up request","daemon.warn");
					Dial_RM500U();
					dialip_debug=1;
				}
				else{
					dialip_debug=0;
				}
				//获取网卡IP
				memset(cmd,0,sizeof(cmd));
				sprintf(cmd,"ifconfig %s | grep \"inet addr\" | awk -F ':' '{print$2}' | awk -F ' ' '{print$1}'",value_ifname);
				cmd_recieve_str(cmd,value_ip_ifconfig);
				//不存在网卡IP或模组IP和网卡IP不一致时，重启网卡
				if(strlen(value_ip_ifconfig)==0 || !strstr(value_ip_dial,value_ip_ifconfig))
				{
					cmd_recieve_str(cmd_ifup,NULL);
				}
			}
			else if(strstr(value_module,"rm500q"))		//移远模组
			{
				memset(value_ip_dial,0,sizeof(value_ip_dial));
				memset(value_ip_ifconfig,0,sizeof(value_ip_ifconfig));
				//获取模组拨号IP
				memset(cmd,0,sizeof(cmd));
				sprintf(cmd,"cmdat %s AT+CGPADDR=1 | grep \"+CGPADDR:\" | awk -F ',' '{print$1}'| awk -F ' ' '{print$2}'",path_usb);
				cmd_recieve_str(cmd,value_ip_dial);
				//存在拨号IP，尝试拨号
				if(strlen(value_ip_dial)==1)
				{
					//对fm650模组进行拨号
					if(dialip_debug==0){
						Debug("initiate a dial-up request","daemon.warn");
						dialip_debug=1;
						Dial_RM500Q();
					}
					
				}else
				{
					dialip_debug=0;
				}
				//获取网卡IP
				/*memset(cmd,0,sizeof(cmd));
				sprintf(cmd,"ifconfig %s | grep \"inet addr\" | awk -F ':' '{print$2}' | awk -F ' ' '{print$1}'",value_ifname);
				cmd_recieve_str(cmd,value_ip_ifconfig);
				//不存在网卡IP或模组IP和网卡IP不一致时，重启网卡
				if(strlen(value_ip_ifconfig)==0)
				{
					//设置modem网卡信息：协议、网卡名和跃点数
					memset(cmd,0,sizeof(cmd));
					sprintf(cmd,"uci set network.%s.proto='dhcp'",name_config);
					cmd_recieve_str(cmd,NULL);
					memset(cmd,0,sizeof(cmd));
					sprintf(cmd,"uci set network.%s.ifname='%s'",name_config,value_ifname);
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
					setFlightmode();
					cmd_recieve_str(cmd_ifup,NULL);
				}*/
			}
			else if(strstr(value_module,"rg520n"))		//芯讯通5G模组
			{
				memset(value_ip_ifconfig,0,sizeof(value_ip_ifconfig));
				memset(cmd,0,sizeof(cmd));
				sprintf(cmd,"ifconfig %s | grep \"inet addr\" | awk -F ':' '{print$2}' | awk -F ' ' '{print$1}'",value_ifname);
				cmd_recieve_str(cmd,value_ip_ifconfig);
				//不存在网卡IP时，尝试拨号
				if(value_ip_ifconfig == NULL || strlen(value_ip_ifconfig)==0)
				{
					if(dialip_debug==0){
						Debug("initiate a dial-up request","daemon.warn");
						dialip_debug=1;
						memset(dialresult_debug,0,sizeof(dialresult_debug));
						strcpy(dialresult_debug,"null");
					}
					//if(isPCIE())
					{
						int pid=0;
						memset(cmd,0,sizeof(cmd));
						sprintf(cmd,"ps | grep quectel-CM | grep -v grep");
						pid=cmd_recieve_int(cmd);
						if(pid>0)
						{
							setFlightmode();
							cmd_recieve_str(cmd_ifup,NULL);
						}
						else{
							if(strlen(value_defaultcard)>0)
							{
								memset(cmd,0,sizeof(cmd));
								sprintf(cmd,"cat /sys/class/leds/sim_sw/brightness"); //获取当前卡槽
								int value_currentcard = cmd_recieve_int(cmd); 
								if(strstr(board_name,"RT990"))
								{
									if (value_currentcard == 255)
										value_currentcard = 1;
									if(value_currentcard == 1) 			//[当前卡槽] 1
										Dial_RG520N(value_apn_card2,value_username_card2,value_password_card2,value_auth_card2,type_ip_card2);
									else if(value_currentcard == 0)		//[当前卡槽] 2
										Dial_RG520N(value_apn_card1,value_username_card1,value_password_card1,value_auth_card1,type_ip_card1);
								}
								else
								{
									if (value_currentcard == 255)
										value_currentcard = 1;
									if(value_currentcard == 0) 			//[当前卡槽] 0
										Dial_RG520N(value_apn_card2,value_username_card2,value_password_card2,value_auth_card2,type_ip_card2);
									else if(value_currentcard == 1)		//[当前卡槽] 1
										Dial_RG520N(value_apn_card1,value_username_card1,value_password_card1,value_auth_card1,type_ip_card1);
								}
							}
							else
							{
								Dial_RG520N(value_apn,value_username,value_password,value_auth,type_ip);
							}
							//sleep(3);
							//cmd_recieve_str(cmd_ifup,NULL);
							system("/etc/init.d/dnsmasq restart &");
						}
					}
					/* else
					{
						Dial_RM520N();
					} */
				}
				else
				{
					if(strcmp(dialresult_debug,"yes")!=0)
					{
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
					dialip_debug=0;
					dial_flight=0;
				}
				int route;
				memset(cmd,0,sizeof(cmd));
				sprintf(cmd,"route | grep default | grep %s | sed -n 1p | awk -F ' ' '{print $5}'",value_ifname);
				route=cmd_recieve_int(cmd);
				if(route==0)
				{
					cmd_recieve_str(cmd_ifup,NULL);
				}
			}
			else if(strstr(value_module,"ec20"))		//芯讯通5G模组
			{
				memset(value_ip_ifconfig,0,sizeof(value_ip_ifconfig));
				memset(cmd,0,sizeof(cmd));
				sprintf(cmd,"ifconfig %s | grep \"inet addr\" | awk -F ':' '{print$2}' | awk -F ' ' '{print$1}'",value_ifname);
				cmd_recieve_str(cmd,value_ip_ifconfig);
				int pid=0;
				memset(cmd,0,sizeof(cmd));
				sprintf(cmd,"ps | grep quectel-CM | grep -v grep");
				pid=cmd_recieve_int(cmd);
				//不存在网卡IP时，尝试拨号
				if(value_ip_ifconfig == NULL || strlen(value_ip_ifconfig)==0 || pid<=0)
				{
					if(dialip_debug==0){
						Debug("initiate a dial-up request","daemon.warn");
						dialip_debug=1;
						memset(dialresult_debug,0,sizeof(dialresult_debug));
						strcpy(dialresult_debug,"null");
					}
					
					{
						
						if(pid>0)
						{
							setFlightmode();
							cmd_recieve_str(cmd_ifup,NULL);
						}
						else{
							if(strlen(value_defaultcard)>0)
							{
								memset(cmd,0,sizeof(cmd));
								sprintf(cmd,"cat /sys/class/leds/sim_sw/brightness"); //获取当前卡槽
								int value_currentcard = cmd_recieve_int(cmd); 
								if (value_currentcard == 255)
									value_currentcard = 1;
								if(value_currentcard == 0) 			//[当前卡槽] 0
									Dial_EC20(value_apn_card2,value_username_card2,value_password_card2,value_auth_card2,type_ip_card2);
								else if(value_currentcard == 1)		//[当前卡槽] 1
									Dial_EC20(value_apn_card1,value_username_card1,value_password_card1,value_auth_card1,type_ip_card1);
							}
							else
							{
								Dial_EC20(value_apn,value_username,value_password,value_auth,type_ip);
							}
							//sleep(3);
							//cmd_recieve_str(cmd_ifup,NULL);
							//system("/etc/init.d/dnsmasq restart &");
						}
					}
				}
				else
				{
					if(strcmp(dialresult_debug,"yes")!=0)
					{
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
					dialip_debug=0;
					dial_flight=0;
				}
				int route;
				memset(cmd,0,sizeof(cmd));
				sprintf(cmd,"route | grep default | grep %s | sed -n 1p | awk -F ' ' '{print $5}'",value_ifname);
				route=cmd_recieve_int(cmd);
				if(route==0)
				{
					cmd_recieve_str(cmd_ifup,NULL);
				}
			}
			else if(strstr(value_module,"mt5700") || strstr(value_module,"mt5711"))		//广和通模组
			{
				memset(value_ip_dial,0,sizeof(value_ip_dial));
				memset(value_ip_ifconfig,0,sizeof(value_ip_ifconfig));
				//获取模组拨号IP
				memset(cmd,0,sizeof(cmd));
				sprintf(cmd,"cmdat %s AT+CGPADDR | grep \"+CGPADDR: 1\" | awk -F ',' '{print$2}'| awk -F '\"' '{print$2}'",path_usb);
				cmd_recieve_str(cmd,value_ip_dial);
				//不存在拨号IP，尝试拨号
				if(strlen(value_ip_dial)==0)
				{
					//对fm650模组进行拨号
					if(dialip_debug==0){
						Debug("initiate a dial-up request","daemon.warn");
						dialip_debug=1;
						memset(dialresult_debug,0,sizeof(dialresult_debug));
						strcpy(dialresult_debug,"null");
					}
					Dial_MT5700();
				}
				else
				{
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
					dialip_debug=0;
				}
				//获取网卡IP
				memset(cmd,0,sizeof(cmd));
				sprintf(cmd,"ifconfig %s | grep \"inet addr\" | awk -F ':' '{print$2}' | awk -F ' ' '{print$1}'",value_ifname);
				cmd_recieve_str(cmd,value_ip_ifconfig);
				//不存在网卡IP或模组IP和网卡IP不一致时，重启网卡
				if(strlen(value_ip_ifconfig)==0 || !strstr(value_ip_dial,value_ip_ifconfig))
				{
					if(strlen(mode_nat)<=0 || strstr(mode_nat,"0"))
						cmd_recieve_str(cmd_ifup,NULL);
				}
			}
		}
		if(value_signal<2){
			if(strstr(value_module,"fm160")||strstr(value_module,"fm650")||strstr(value_module,"nl668"))
			{
				memset(value_ip_dial,0,sizeof(value_ip_dial));
				memset(cmd,0,sizeof(cmd));
				sprintf(cmd,"cmdat %s AT+GTRNDIS? | grep \"+GTRNDIS: 1\" | awk -F ',' '{print$3}'| awk -F '\"' '{print$2}'",path_usb);
				cmd_recieve_str(cmd,value_ip_dial);
				//不存在拨号IP，尝试拨号
				if(strlen(value_ip_dial)==0)
				{
					if(sig_flight<5)
					{
						if(sig_flight==0)
						{
							Debug("low signal causes connectivity loss , reboot cellular network","cellular.error");
							clear_debug_info();
						}
						setFlightmode();
						//sleep(sleep_delay[sig_flight]);
						sleep_delay_by_registation();
						sig_flight=sig_flight+1;
						
					}
					else if(sig_flight==5)
					{
						if(strcmp(name_config,"SIM2")==0 )//&& isPCIE())
						{
							clear_debug_info();
						}
						else
						{
							Debug("detecting low signal reaches 5 times, reboot modem","cellular.error");
							clear_debug_info();
							resetModem();
							sleep_delay_by_registation();
						}
						sig_flight=0;
					}
				}
				else
				{
					sig_flight=0;
				}
			}
			if(strstr(value_module,"sim8200"))
			{
				memset(value_ip_ifconfig,0,sizeof(value_ip_ifconfig));
				memset(cmd,0,sizeof(cmd));
				sprintf(cmd,"ifconfig %s | grep \"inet addr\" | awk -F ':' '{print$2}' | awk -F ' ' '{print$1}'",value_ifname);
				cmd_recieve_str(cmd,value_ip_ifconfig);
				//不存在网卡IP时，尝试拨号
				if(value_ip_ifconfig == NULL || strlen(value_ip_ifconfig)==0)
				{
					if(sig_flight<5)
					{
						if(sig_flight==0)
						{
							clear_debug_info();
							Debug("low signal causes connectivity loss , reboot cellular network","cellular.error");
						}
						setFlightmode();
						//sleep(sleep_delay[sig_flight]);
						sleep_delay_by_registation();
						sig_flight=sig_flight+1;
						
					}
					else if(sig_flight==5)
					{
						clear_debug_info();
						Debug("detecting low signal reaches 5 times, reboot modem","cellular.error");
						resetModem();
						sleep_delay_by_registation();
						sig_flight=0;
					}
				}
				else
				{
					sig_flight=0;
				}
			}
			if(strstr(value_module,"fg160"))
			{
				memset(value_ip_ifconfig,0,sizeof(value_ip_ifconfig));
				memset(cmd,0,sizeof(cmd));
				sprintf(cmd,"ifconfig %s | grep \"inet addr\" | awk -F ':' '{print$2}' | awk -F ' ' '{print$1}'",value_ifname);
				cmd_recieve_str(cmd,value_ip_ifconfig);
				//不存在网卡IP时，尝试拨号
				if(value_ip_ifconfig == NULL || strlen(value_ip_ifconfig)==0)
				{
					if(sig_flight<5)
					{
						if(sig_flight==0)
						{	
							memset(cmd,0,sizeof(cmd));
							sprintf(cmd,"low signal causes connectivity loss , reboot cellular network");
							Debug(cmd,"cellular.error");
							clear_debug_info();
						}
						setFlightmode();
						//sleep(sleep_delay[sig_flight]);
						sleep_delay_by_registation();
						sig_flight=sig_flight+1;
						
					}
					else if(sig_flight==5)
					{
						//Debug("detecting low signal reaches 5 times, reboot modem","cellular.error");
						clear_debug_info();
						//resetModem();
						sleep_delay_by_registation();
						sig_flight=0;
					}
				}
				else
				{
					sig_flight=0;
				}
			}
			if(strstr(value_module,"rg520n"))
			{
				memset(value_ip_ifconfig,0,sizeof(value_ip_ifconfig));
				memset(cmd,0,sizeof(cmd));
				sprintf(cmd,"ifconfig %s | grep \"inet addr\" | awk -F ':' '{print$2}' | awk -F ' ' '{print$1}'",value_ifname);
				cmd_recieve_str(cmd,value_ip_ifconfig);
				//不存在网卡IP时，尝试拨号
				if(value_ip_ifconfig == NULL || strlen(value_ip_ifconfig)==0)
				{
					if(sig_flight<5)
					{
						if(sig_flight==0)
						{	
							memset(cmd,0,sizeof(cmd));
							sprintf(cmd,"low signal causes connectivity loss , reboot cellular network");
							Debug(cmd,"cellular.error");
							clear_debug_info();
						}
						setFlightmode();
						//sleep(sleep_delay[sig_flight]);
						sleep_delay_by_registation();
						sig_flight=sig_flight+1;
						
					}
					else if(sig_flight==5)
					{
						//Debug("detecting low signal reaches 5 times, reboot modem","cellular.error");
						clear_debug_info();
						//resetModem();
						sleep_delay_by_registation();
						sig_flight=0;
					}
				}
				else
				{
					sig_flight=0;
				}
			}
			if(strstr(value_module,"ec20"))
			{
				memset(value_ip_ifconfig,0,sizeof(value_ip_ifconfig));
				memset(cmd,0,sizeof(cmd));
				sprintf(cmd,"ifconfig %s | grep \"inet addr\" | awk -F ':' '{print$2}' | awk -F ' ' '{print$1}'",value_ifname);
				cmd_recieve_str(cmd,value_ip_ifconfig);
				//不存在网卡IP时，尝试拨号
				if(value_ip_ifconfig == NULL || strlen(value_ip_ifconfig)==0)
				{
					if(sig_flight<5)
					{
						if(sig_flight==0)
						{
							Debug("low signal causes connectivity loss , reboot cellular network","cellular.error");
							clear_debug_info();
						}
						setFlightmode();
						//sleep(sleep_delay[sig_flight]);
						sleep_delay_by_registation();
						sig_flight=sig_flight+1;
						
					}
					else if(sig_flight==5)
					{
						//Debug("detecting low signal reaches 5 times, reboot modem","cellular.error");
						clear_debug_info();
						//resetModem();
						sleep_delay_by_registation();
						sig_flight=0;
					}
				}
				else
				{
					sig_flight=0;
				}
			}
			if(strstr(value_module,"mt5700") || strstr(value_module,"mt5711"))
			{
				memset(value_ip_dial,0,sizeof(value_ip_dial));
				memset(cmd,0,sizeof(cmd));
				sprintf(cmd,"cmdat %s AT+CGPADDR | grep \"+CGPADDR: 1\" | awk -F ',' '{print$2}'| awk -F '\"' '{print$2}'",path_usb);
				cmd_recieve_str(cmd,value_ip_dial);
				//不存在拨号IP，尝试拨号
				if(strlen(value_ip_dial)==0)
				{
					if(sig_flight<5)
					{
						if(sig_flight==0)
						{
							Debug("low signal causes connectivity loss , reboot cellular network","cellular.error");
							clear_debug_info();
						}
						setFlightmode();
						//sleep(sleep_delay[sig_flight]);
						sleep_delay_by_registation();
						sig_flight=sig_flight+1;
						
					}
					else if(sig_flight==5)
					{
						{
							Debug("detecting low signal reaches 5 times, reboot modem","cellular.error");
							clear_debug_info();
							resetModem();
							sleep_delay_by_registation();
						}
						sig_flight=0;
					}
				}
				else
				{
					sig_flight=0;
				}
			}
		}
	}
}

void pre_Dial()
{
	//Debug("pre_Dial","daemon.debug");
	char cmd[1024];
	char value_ip_dial[100];
	char value_ip_ifconfig[100];
	if(strstr(value_module,"fm160"))
	{	
		{
			memset(value_ip_dial,0,sizeof(value_ip_dial));
			//获取模组拨号IP
			memset(cmd,0,sizeof(cmd));
			sprintf(cmd,"cmdat %s AT+GTRNDIS? | grep \"+GTRNDIS: 1\" | awk -F ',' '{print$3}'| awk -F '\"' '{print$2}'",path_usb);
			cmd_recieve_str(cmd,value_ip_dial);
			//不存在拨号IP，尝试拨号
			if(strlen(value_ip_dial)==0)
			{
				//对fm160模组进行拨号
				/* if(dialip_debug==0){
					Debug("initiate a dial-up request","daemon.warn");
					dialip_debug=1;
					memset(dialresult_debug,0,sizeof(dialresult_debug));
					strcpy(dialresult_debug,"null");
				} */
				Dial_FM160_PRE();
			}
			else{
				//获取网卡IP
				memset(cmd,0,sizeof(cmd));
				sprintf(cmd,"ifconfig %s | grep \"inet addr\" | awk -F ':' '{print$2}' | awk -F ' ' '{print$1}'",value_ifname);
				cmd_recieve_str(cmd,value_ip_ifconfig);
				if(strlen(value_ip_dial)>0 && strlen(value_ip_ifconfig)<0)
				{
					cmd_recieve_str(cmd_ifup,NULL);
					setFlightmode();
				}
				if(strcmp(value_ip_dial,value_ip_ifconfig)!=0)
				{
					cmd_recieve_str(cmd_ifup,NULL);
				}
				//Debug("check dial OK","daemon.debug");
			}
		}
	}
}

void checkDNS()
{
	//开启拨号功能和PING功能时，进行网络监测
	if(flag_dial==1 && flag_ping==1 && strlen(value_pingaddr)>0)
	{
		//Debug("check ping","debug.info");
		//获取ping状态
		int result=ping_status(value_pingaddr,value_ifname,1);
		char cmd[50];
		if(result==0) //网络正常
		{
			if(strstr(dns_debug,"null") || strstr(dns_debug,"error")){
				memset(cmd,0,sizeof(cmd));
				sprintf(cmd,"ping %s OK",value_pingaddr);
				Debug(cmd,"daemon.info");
				memset(dns_debug,0,sizeof(dns_debug));
				strcpy(dns_debug,"ok");
			}
			count_error=1;
		}else	//网络异常
		{	
			if(strstr(dns_debug,"null") || strstr(dns_debug,"ok")){
				memset(cmd,0,sizeof(cmd));
				sprintf(cmd,"ping %s failed",value_pingaddr);
				Debug(cmd,"daemon.error");
				memset(dns_debug,0,sizeof(dns_debug));
				strcpy(dns_debug,"error");
			}
			if(count_error==count_ping)
			{
				if(strstr(mode_opera,"reboot_route"))		//重启设备
				{
					system("reboot");
				}
				else if(strstr(mode_opera,"switch_card"))	//切换SIM卡
				{
					if(strstr(value_module,"fm650") || strstr(value_module,"fm160"))
						SwitchSIM_FM650(false);
					else if(strstr(value_module,"fg160"))
						SwitchSIM_FG160(false);
					else if(strstr(value_module,"nl668"))
						SwitchSIM_NL668(false);
					else if(strstr(value_module,"sim8200"))
						SwitchSIM_SIM8200(false);
					else if(strstr(value_module,"sim7600"))
						SwitchSIM_SIM7600(false);
					else if(strstr(value_module,"rm500u") || strstr(value_module,"rm500q"))
						SwitchSIM_RG520N(false);
					else if(strstr(value_module,"ec20"))
						SwitchSIM_EC20(false);
					else if(strstr(value_module,"mt5700"))
						SwitchSIM_MT5700(false);
					else if(strstr(value_module,"mt5711"))
						SwitchSIM_MT5711(false);
					setFlightmode();	
				}
				else if(strstr(mode_opera,"airplane_mode"))	//飞行模式
				{
					//Debug("setFlightmode",NULL);
					setFlightmode();
				}
				count_error=1;
			}
			else
				count_error=count_error+1;
		}
	}
}

void clear_debug_info()
{
	memset(debug_nservice,0,sizeof(debug_nservice));
	memset(debug_band,0,sizeof(debug_band));
	memset(debug_arfcn,0,sizeof(debug_arfcn));
	memset(debug_pci,0,sizeof(debug_pci));
	strcat(debug_nservice,"null");
	strcat(debug_band,"-");
	strcat(debug_arfcn,"-");
	strcat(debug_pci,"-");
}

void sleep_delay_by_registation()
{
	char cmd[200];
	char cgreg[200];
	char cereg[200];
	char c5greg[200];
	char buff[200];
	int count=0;
	while(1)
	{
		memset(cgreg,0,sizeof(cgreg));
		memset(cereg,0,sizeof(cereg));
		memset(c5greg,0,sizeof(c5greg));
		
		memset(cmd,0,sizeof(cmd));
		sprintf(cmd,"cmdat %s AT+CGREG? | grep \"+CGREG: \" | awk -F ' ' '{print$2}'",path_usb);
		cmd_recieve_str(cmd,cgreg);
		
		uci_cellular("CGREG",cgreg);
		
		memset(cmd,0,sizeof(cmd));
		sprintf(cmd,"cmdat %s AT+CEREG? | grep \"+CEREG: \" | awk -F ' ' '{print$2}'",path_usb);
		cmd_recieve_str(cmd,cereg);
		
		uci_cellular("CEREG",cereg);
		
		if(strstr(value_module,"fm650") || strstr(value_module,"fm160") || strstr(value_module,"rg520n") || strstr(value_module,"fg160"))
		{
			memset(cmd,0,sizeof(cmd));
			sprintf(cmd,"cmdat %s AT+C5GREG? | grep \"+C5GREG: \" | awk -F ' ' '{print$2}'",path_usb);
			cmd_recieve_str(cmd,c5greg);
			
			uci_cellular("C5GREG",c5greg);
			
			if(!strstr(cgreg,"0,2") && !strstr(cereg,"0,2") && !strstr(c5greg,"0,2"))
			{
				memset(cmd,0,sizeof(cmd));
				sprintf(cmd,"cgreg: %s; cereg: %s; c5greg: %s",cgreg,cereg,c5greg);
				//Debug(cmd,"daemon.debug");
				return;
			}
		}
		else
		{
			if(!strstr(cgreg,"0,2") && !strstr(cereg,"0,2"))
			{
				memset(cmd,0,sizeof(cmd));
				sprintf(cmd,"cgreg: %s; cereg: %s; c5greg: %s",cgreg,cereg,c5greg);
				//Debug(cmd,"daemon.debug");
				return;
			}
		}
		if(count>24)
			return;
		if(strstr(value_module,"fm650") || strstr(value_module,"fm160") || strstr(value_module,"rg520n") || strstr(value_module,"fg160") || strstr(value_module,"nl668"))
		{
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
		}
		sleep(5);
		count++;
	}
	
}

void debug_card_info(char *status)
{
	char cmd[300];
	if(strcmp(debug_readcard,status)!=0) //状态不一致时打印
	{
		if(strstr(status,"ready"))
		{
			memset(cmd,0,sizeof(cmd));
			sprintf(cmd,"read card, ccid: %s; imsi: %s; operator: %s",debug_iccid,debug_imsi,debug_cops);
			Debug(cmd,"daemon.info");
			sprintf(debug_readcard,"ready");
		}
		else if(strstr(status,"unplug"))
		{
			memset(cmd,0,sizeof(cmd));
			sprintf(cmd,"card not inserted");
			Debug(cmd,"daemon.error");
			sprintf(debug_readcard,"unplug");
		}
		else if(strstr(status,"pin"))
		{
			memset(cmd,0,sizeof(cmd));
			sprintf(cmd,"card pin");
			Debug(cmd,"daemon.error");
			sprintf(debug_readcard,"pin");
		}
	}
}
#endif