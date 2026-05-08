#ifndef __SAMSUNG_DEVICES_LIST_H__
#define __SAMSUNG_DEVICES_LIST_H__

#include "misc_usb.h"

extern int samsung_log_main(int argc, char** argv);
extern int samsung_dumplog_main(int argc, char **argv);
extern int samsung_dumplog_tcp_main(int argc, char **argv);

static fibo_usbdev_t samsung_devices_table[] =
{
    //for PCIe serial
    {"FIBOCOM FM550", 0x1122, 0x3344, {5, -1},  samsung_log_main, -1, {-1, -1}, {0},{0},{0},{0},{0},0,0,NULL,NULL},

    //samsung USB
	{"FIBOCOM FM550", 0x04E8, 0x685E, {5, -1},  samsung_log_main, -1, {-1, -1}, {0},{0},{0},{0},{0},0,0,NULL,NULL},
	{"FIBOCOM FM550", 0x04E8, 0x6864, {5, -1},	samsung_log_main, -1, {-1, -1}, {0},{0},{0},{0},{0},0,0,NULL,NULL},

    //for dump log
	{"FIBOCOM FM550", 0x18d1, 0x0002, {0, -1},	samsung_dumplog_main, -1, {-1, -1}, {0},{0},{0},{0},{0},0,0,NULL,NULL},
	{"FIBOCOM FM550", 0xFFFF, 0xFF00, {0, -1},	samsung_dumplog_tcp_main, -1, {-1, -1}, {0},{0},{0},{0},{0},0,0,NULL,NULL},

    //USB port all
	{"FIBOCOM FM550", 0x2CB7, 0x0E01, {5, -1},	samsung_log_main, -1, {-1, -1}, {0},{0},{0},{0},{0},0,0,NULL,NULL},
	{"FIBOCOM FM550", 0x2CB7, 0x0E02, {5, -1},	samsung_log_main, -1, {-1, -1}, {0},{0},{0},{0},{0},0,0,NULL,NULL},
	{"FIBOCOM FM550", 0x2CB7, 0x0E03, {5, -1},	samsung_log_main, -1, {-1, -1}, {0},{0},{0},{0},{0},0,0,NULL,NULL},
	{"FIBOCOM FM550", 0x2CB7, 0x0E04, {5, -1},	samsung_log_main, -1, {-1, -1}, {0},{0},{0},{0},{0},0,0,NULL,NULL},
		
	{"FIBOCOM FM550", 0x2CB7, 0x0E05, {5, -1},	samsung_log_main, -1, {-1, -1}, {0},{0},{0},{0},{0},0,0,NULL,NULL},
	{"FIBOCOM FM550", 0x2CB7, 0x0E06, {5, -1},	samsung_log_main, -1, {-1, -1}, {0},{0},{0},{0},{0},0,0,NULL,NULL},
	{"FIBOCOM FM550", 0x2CB7, 0x0E07, {5, -1},	samsung_log_main, -1, {-1, -1}, {0},{0},{0},{0},{0},0,0,NULL,NULL},
	{"FIBOCOM FM550", 0x2CB7, 0x0E08, {5, -1},	samsung_log_main, -1, {-1, -1}, {0},{0},{0},{0},{0},0,0,NULL,NULL},
	{"FIBOCOM FM550", 0x2CB7, 0x0E09, {5, -1},	samsung_log_main, -1, {-1, -1}, {0},{0},{0},{0},{0},0,0,NULL,NULL},

};


#endif

