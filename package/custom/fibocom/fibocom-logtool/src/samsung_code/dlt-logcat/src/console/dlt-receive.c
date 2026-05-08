/*
 * SPDX license identifier: MPL-2.0
 *
 * Copyright (C) 2011-2015, BMW AG
 *
 * This file is part of GENIVI Project DLT - Diagnostic Log and Trace.
 *
 * This Source Code Form is subject to the terms of the
 * Mozilla Public License (MPL), v. 2.0.
 * If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * For further information see http://www.genivi.org/.
 */

/*!
 * \author Alexander Wenzel <alexander.aw.wenzel@bmw.de>
 *
 * \copyright Copyright © 2011-2015 BMW AG. \n
 * License MPL-2.0: Mozilla Public License version 2.0 http://mozilla.org/MPL/2.0/.
 *
 * \file dlt-receive.c
 */


/*******************************************************************************
**                                                                            **
**  SRC-MODULE: dlt-receive.c                                                 **
**                                                                            **
**  TARGET    : linux                                                         **
**                                                                            **
**  PROJECT   : DLT                                                           **
**                                                                            **
**  AUTHOR    : Alexander Wenzel Alexander.AW.Wenzel@bmw.de                   **
**              Markus Klein                                                  **
**                                                                            **
**  PURPOSE   :                                                               **
**                                                                            **
**  REMARKS   :                                                               **
**                                                                            **
**  PLATFORM DEPENDANT [yes/no]: yes                                          **
**                                                                            **
**  TO BE CHANGED BY USER [yes/no]: no                                        **
**                                                                            **
*******************************************************************************/

/*******************************************************************************
**                      Author Identity                                       **
********************************************************************************
**                                                                            **
** Initials     Name                       Company                            **
** --------     -------------------------  ---------------------------------- **
**  aw          Alexander Wenzel           BMW                                **
**  mk          Markus Klein               Fraunhofer ESK                     **
*******************************************************************************/

/*******************************************************************************
**                      Revision Control History                              **
*******************************************************************************/

/*
 * $LastChangedRevision: 1670 $
 * $LastChangedDate: 2011-04-08 15:12:06 +0200 (Fr, 08. Apr 2011) $
 * $LastChangedBy$
 * Initials    Date         Comment
 * aw          13.01.2010   initial
 */

#include <ctype.h>      /* for isprint() */
#include <stdlib.h>     /* for atoi() */
#include <sys/stat.h>   /* for S_IRUSR, S_IWUSR, S_IRGRP, S_IROTH */
#include <fcntl.h>      /* for open() */
#include <sys/uio.h>    /* for writev() */
#include <errno.h>
#include <string.h>
#include <glob.h>
#include <syslog.h>
#include <signal.h>
#include <sys/socket.h>
#ifdef __linux__
#   include <linux/limits.h>
#else
#   include <limits.h>
#endif
#include <inttypes.h>

#include "dlt_client.h"
#include "dlt-control-common.h"

//inotify monitor
#include <sys/inotify.h>
#include <stdio.h>

//for network api
#include "network_api.h"

//for no log case
#include <time.h>


#define DLT_RECEIVE_ECU_ID "RECV"

/*for network*/
extern const char *g_tftp_server_ip;
extern const char *g_ftp_server_ip;
extern const char *g_ftp_server_usr;
extern const char *g_ftp_server_pass;



#define EVENT_NUM 12
const char *event_str[EVENT_NUM] = {
"IN_ACCESS",
"IN_MODIFY",
"IN_ATTRIB",
"IN_CLOSE_WRITE",
"IN_CLOSE_NOWRITE",
"IN_OPEN",
"IN_MOVED_FROM",
"IN_MOVED_TO",
"IN_CREATE",
"IN_DELETE",
"IN_DELETE_SELF",
"IN_MOVE_SELF"
};


/*monitor forever  unless  ctrl+c are pressed*/
int dlt_receive_send_get_software_version_msg(void);
int dlt_receive_monitor_serial_path_create(DltClient * p_dltclient,char * serial_path);
int dlt_receive_send_stop_output_msg(void);
int dlt_receive_compare_current_interval_larger_than(long seconds);
int dlt_receive_new_thread_monitor(void);
int dlt_receive_sync_network_stop(void);




DltClient dltclient;

volatile int g_quit_flag = 0;

volatile struct timespec start_before_read = {0,0};


void signal_handler(int signal)
{
    if (g_quit_flag) {
        return;
    }
    switch (signal) {
    case SIGHUP:
        /* ignore SIGHUP which happened while removing and inserting USB cable */
        dlt_vlog(LOG_NOTICE, "ignore signal: %s\n",
                             strsignal(signal));
        break;
    case SIGINT:
    //case SIGHUP:
    case SIGTERM:
    case SIGQUIT:
        g_quit_flag = 0x11223344;
        dlt_vlog(LOG_NOTICE,"dlt signal_handler, signal=%d\n",signal);
        if (dltclient.sock != -1) {
            /*if no data in 100s , it should be an issue, no need to send stop output msg*/
            if (dlt_receive_compare_current_interval_larger_than(100) <= 0) {
                /*that means channel is valid*/
                dlt_receive_send_stop_output_msg();
            }
        }

        /* stop main loop */
        shutdown(dltclient.receiver.fd, SHUT_RD);
        if (dltclient.inotify_fd >= 0) {
            shutdown(dltclient.inotify_fd, SHUT_RD);
        }

        break;
    default:
        /* This case should never happen! */
        break;
    } /* switch */

}

/* Function prototypes */
int dlt_receive_message_callback(DltMessage *message, void *data);

typedef struct {
    int aflag;
    int sflag;
    int xflag;
    int mflag;
    int vflag;
    int yflag;
    int uflag;
    char *ovalue;
    char *ovaluebase; /* ovalue without ".dlt" */
    char *fvalue;       /* filename for space separated filter file (<AppID> <ContextID>) */
    char *jvalue;       /* filename for json filter file */
    char *evalue;
    int bvalue;
    int64_t climit;
    char ecuid[4];
    int ohandle;
    int64_t totalbytes; /* bytes written so far into the output file, used to check the file size limit */
    int part_num;    /* number of current output file if limit was exceeded */
    DltFile file;
    DltFilter filter;
    int port;
    int max_files_number;/*<=0 for no limit, max number is 100000 */
} DltReceiveData;

int dlt_receive_no_quit_unless_requested(DltClient * dltclient,DltReceiveData* dltdata, char* target_serial_path);


/**
 * Print usage information of tool.
 */
void usage()
{
    char version[255];

    dlt_get_version(version, 255);

    printf("Usage: dlt-logcat [options] hostname/serial_device_name\n");
    printf("Receive DLT messages from DLT daemon and print or store the messages.\n");
    printf("Use filters to filter received messages.\n");
    printf("%s \n", version);
    printf("Options:\n");
    printf("  -a            Print DLT messages; payload as ASCII\n");
    printf("  -x            Print DLT messages; payload as hex\n");
    printf("  -m            Print DLT messages; payload as hex and ASCII\n");
    printf("  -s            Print DLT messages; only headers\n");
    printf("  -v            Verbose mode\n");
    printf("  -h            Usage\n");
    printf("  -y            Serial device mode\n");
    printf("  -u            UDP multicast mode\n");
    printf("  -b baudrate   Serial device baudrate (Default: 115200)\n");
    printf("  -e ecuid      Set ECU ID (Default: RECV)\n");
    printf("  -o filename   Output messages in new DLT file\n");
    printf("  -c limit      Restrict file size to <limit> bytes when output to file\n");
    printf("                When limit is reached, a new file is opened. Use K,M,G as\n");
    printf("                suffix to specify kilo-, mega-, giga-bytes respectively\n");
    printf("  -f filename   Enable filtering of messages with space separated list (<AppID> <ContextID>)\n");
    printf("  -j filename   Enable filtering of messages with filter defined in json file\n");
    printf("  -p port       Use the given port instead the default port\n");
    printf("                Cannot be used with serial devices\n");
}


int64_t convert_arg_to_byte_size(char *arg)
{
    size_t i;
    int64_t factor;
    int64_t result;

    /* check if valid input */
    for (i = 0; i < strlen(arg) - 1; ++i)
        if (!isdigit(arg[i]))
            return -2;

    /* last character */
    factor = 1;

    if ((arg[strlen(arg) - 1] == 'K') || (arg[strlen(arg) - 1] == 'k'))
        factor = 1024;
    else if ((arg[strlen(arg) - 1] == 'M') || (arg[strlen(arg) - 1] == 'm'))
        factor = 1024 * 1024;
    else if ((arg[strlen(arg) - 1] == 'G') || (arg[strlen(arg) - 1] == 'g'))
        factor = 1024 * 1024 * 1024;
    else if (!isdigit(arg[strlen(arg) - 1]))
        return -2;

    /* range checking */
    int64_t const mult = atoll(arg);

    if (((INT64_MAX) / factor) < mult)
        /* Would overflow! */
        return -2;

    result = factor * mult;

    /* The result be at least the size of one message
     * One message consists of its header + user data:
     */
    DltMessage msg;
    int64_t min_size = sizeof(msg.headerbuffer);
    min_size += 2048 /* DLT_USER_BUF_MAX_SIZE */;

    if (min_size > result) {
        dlt_vlog(LOG_ERR,
                 "ERROR: Specified limit: %" PRId64 "is smaller than a the size of a single message: %" PRId64 "!\n",
                 result,
                 min_size);
        result = -2;
    }

    return result;
}


/*
 * open output file
 */
int dlt_receive_open_output_file(DltReceiveData *dltdata)
{
    /* if (file_already_exists) */
    glob_t outer;

    /*to support upload via tftp and ftp*/
    {
        if (is_tftp() || is_ftp()) {
            char filename[PATH_MAX + 1];
            int try_again = 0;

            //dlt_vlog(LOG_INFO, "dlt_receive_open_output_file tftp or ftp mode\n");

            filename[PATH_MAX] = 0;
TRY_AGAIN:
            dltdata->part_num++;
#if 0
            if (dltdata->max_files_number <= 0) {
                dltdata->part_num = dltdata->part_num%10000;
            } else {
                dltdata->part_num = dltdata->part_num%dltdata->max_files_number;
            }
#endif
            snprintf(filename, PATH_MAX, "%s.%i.dlt", dltdata->ovaluebase, dltdata->part_num);
            //dlt_vlog(LOG_INFO, "dlt_receive_open_output_file filename=%s\n",filename);
            dltdata->ohandle = slog_logfile_create_fullname(0, filename, 0, 0);
            /*for protection, try again only one time*/
            if (dltdata->ohandle < 0 && !try_again) {
                try_again = 1;
                goto TRY_AGAIN;
            }
            return dltdata->ohandle;
        }
    }

    if (glob(dltdata->ovalue,
#ifndef __ANDROID_API__
             GLOB_TILDE |
#endif
             GLOB_NOSORT, NULL, &outer) == 0) {
        if (dltdata->vflag)
            dlt_vlog(LOG_INFO, "File %s already exists, need to rename first\n", dltdata->ovalue);

        if (dltdata->part_num < 0) {
            char pattern[PATH_MAX + 1];
            pattern[PATH_MAX] = 0;
            snprintf(pattern, PATH_MAX, "%s.*.dlt", dltdata->ovaluebase);
            glob_t inner;

            /* sort does not help here because we have to traverse the
             * full result in any case. Remember, a sorted list would look like:
             * foo.1.dlt
             * foo.10.dlt
             * foo.1000.dlt
             * foo.11.dlt
             */
            if (glob(pattern,
#ifndef __ANDROID_API__
                     GLOB_TILDE |
#endif
                     GLOB_NOSORT, NULL, &inner) == 0) {
                /* search for the highest number used */
                size_t i;

                for (i = 0; i < inner.gl_pathc; ++i) {
                    /* convert string that follows the period after the initial portion,
                     * e.g. gt.gl_pathv[i] = foo.1.dlt -> atoi("1.dlt");
                     */
                    int cur = atoi(&inner.gl_pathv[i][strlen(dltdata->ovaluebase) + 1]);

                    if (cur > dltdata->part_num)
                        dltdata->part_num = cur;
                }
            }

            globfree(&inner);

            ++dltdata->part_num;

        }

        char filename[PATH_MAX + 1];
        filename[PATH_MAX] = 0;

        snprintf(filename, PATH_MAX, "%s.%i.dlt", dltdata->ovaluebase, dltdata->part_num++);

        if (rename(dltdata->ovalue, filename) != 0)
            dlt_vlog(LOG_ERR, "ERROR: rename %s to %s failed with error %s\n",
                     dltdata->ovalue, filename, strerror(errno));
        else if (dltdata->vflag)
            dlt_vlog(LOG_INFO, "Renaming existing file from %s to %s\n",
                     dltdata->ovalue, filename);
    } /* if (file_already_exists) */

    globfree(&outer);

    dltdata->ohandle = open(dltdata->ovalue, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    return dltdata->ohandle;
}


void dlt_receive_close_output_file(DltReceiveData *dltdata)
{
    /*for tftp or ftp mode*/
    if (is_tftp() || is_ftp()) {
        slog_logfile_close(dltdata->ohandle);
        dltdata->ohandle = -1;
        //dlt_vlog(LOG_INFO, "dlt_receive_close_output_file tftp or ftp mode\n");
        return ;
    }

    if (dltdata->ohandle) {
        close(dltdata->ohandle);
        dltdata->ohandle = -1;
    }
}


/**
 * Main function of tool.
 */
int main(int argc, char *argv[])
{
    DltReceiveData dltdata;
    int c;
    int index;
	int result = 0;

    /* Initialize dltdata */
    dltdata.aflag = 0;
    dltdata.sflag = 0;
    dltdata.xflag = 0;
    dltdata.mflag = 0;
    dltdata.vflag = 0;
    dltdata.yflag = 0;
    dltdata.uflag = 0;
    dltdata.ovalue = 0;
    dltdata.ovaluebase = 0;
    dltdata.fvalue = 0;
    dltdata.jvalue = 0;
    dltdata.evalue = 0;
    dltdata.bvalue = 0;
    dltdata.climit = -1; /* default: -1 = unlimited */
    dltdata.ohandle = -1;
    dltdata.totalbytes = 0;
    dltdata.part_num = -1;
    dltdata.port = 3490;

    /* Config signal handler */
    struct sigaction act;

    /* Initialize signal handler struct */
    memset(&act, 0, sizeof(act));
    act.sa_handler = signal_handler;
    sigemptyset(&act.sa_mask);
    sigaction(SIGHUP, &act, 0);
    sigaction(SIGTERM, &act, 0);
    sigaction(SIGINT, &act, 0);
    sigaction(SIGQUIT, &act, 0);

    /* Fetch command line arguments */
    opterr = 0;

	//for test
	#if 0
   	{
   	  dlt_vlog(LOG_INFO, "%s() argc=%d\n", __func__ , argc);
   	  if (argc > 0) {
   	      //dlt_vlog(LOG_INFO, "%s()\n", __func__);
   	      int i = 0;
		  while( i < argc) {
		  	printf("[%d] argv[%d]=%s\n",i,i,argv[i]);
			i++;
		  }
   	  }
    }
	#endif

    while ((c = getopt (argc, argv, "vashyuxmf:j:o:e:b:c:p:t:q:d:w:l:")) != -1)
        switch (c) {
        case 'v':
        {
            dltdata.vflag = 1;
            break;
        }
        case 'a':
        {
            dltdata.aflag = 1;
            break;
        }
        case 's':
        {
            dltdata.sflag = 1;
            break;
        }
        case 'x':
        {
            dltdata.xflag = 1;
            break;
        }
        case 'm':
        {
            dltdata.mflag = 1;
            break;
        }
        case 'h':
        {
            usage();
            return -1;
        }
        case 'y':
        {
            dltdata.yflag = 1;
            break;
        }
        case 'u':
        {
            dltdata.uflag = 1;
            break;
        }
        case 'f':
        {
            dltdata.fvalue = optarg;
            break;
        }
        case 'j':
        {
            #ifdef EXTENDED_FILTERING
            dltdata.jvalue = optarg;
            break;
            #else
            fprintf (stderr,
                     "Extended filtering is not supported. Please build with the corresponding cmake option to use it.\n");
            return -1;
            #endif
        }
        case 'o':
        {
            dltdata.ovalue = optarg;
            size_t to_copy = strlen(dltdata.ovalue);

            if (strcmp(&dltdata.ovalue[to_copy - 4], ".dlt") == 0)
                to_copy = to_copy - 4;

            dltdata.ovaluebase = (char *)calloc(1, to_copy + 1);

            if (dltdata.ovaluebase == NULL) {
                fprintf (stderr, "Memory allocation failed.\n");
                return -1;
            }

            dltdata.ovaluebase[to_copy] = '\0';
            memcpy(dltdata.ovaluebase, dltdata.ovalue, to_copy);

            /*setup up target folder for ftp*/
            target_folder_get_parse_for_ftp(dltdata.ovalue);
            break;
        }
        case 'e':
        {
            dltdata.evalue = optarg;
            break;
        }
        case 'b':
        {
            dltdata.bvalue = atoi(optarg);
            break;
        }
        case 'p':
        {
            dltdata.port = atoi(optarg);
            break;
        }

        case 'c':
        {
            dltdata.climit = convert_arg_to_byte_size(optarg);

            if (dltdata.climit < -1) {
                fprintf (stderr, "Invalid argument for option -c.\n");
                /* unknown or wrong option used, show usage information and terminate */
                usage();
                return -1;
            }

            break;
        }
        case 'l':/*max log files number*/
        {
            dltdata.max_files_number = atoi(optarg);
            //dlt_vlog(LOG_INFO, "max_files_number %d\n",dltdata.max_files_number);
            break;
        }
        case 't':/*tftp ip*/
        {
            g_tftp_server_ip = optarg;
            //dlt_vlog(LOG_INFO, "tftp got %s\n",g_tftp_server_ip);
            break;
        }
        case 'q':/*ftp ip*/
        {
            g_ftp_server_ip = optarg;
            //dlt_vlog(LOG_INFO, "g_ftp_server_ip got %s\n",g_ftp_server_ip);
            break;
        }
        case 'd':/*ftp account*/
        {
            g_ftp_server_usr = optarg;
            //dlt_vlog(LOG_INFO, "g_ftp_server_usr got %s\n",g_ftp_server_usr);
            break;
        }
        case 'w':/*ftp password*/
        {
            g_ftp_server_pass = optarg;
            //dlt_vlog(LOG_INFO, "g_ftp_server_pass got %s\n",g_ftp_server_pass);
            break;
        }
        case '?':
        {
            if ((optopt == 'o') || (optopt == 'f') || (optopt == 'c'))
                fprintf (stderr, "Option -%c requires an argument.\n", optopt);
            else if (isprint (optopt))
                fprintf (stderr, "Unknown option `-%c'.\n", optopt);
            else
                fprintf (stderr, "Unknown option character `\\x%x'.\n", optopt);

            /* unknown or wrong option used, show usage information and terminate */
            usage();
            return -1;
        }
        default:
        {
            abort ();
            return -1;    /*for parasoft */
        }
        }

    dltclient.inotify_fd = -1;
    /* Initialize DLT Client */
    dlt_client_init(&dltclient, dltdata.vflag);

    /* Register callback to be called when message was received */
    dlt_client_register_message_callback(dlt_receive_message_callback);

    /* Setup DLT Client structure */
    if(dltdata.uflag) {
        dltclient.mode = DLT_CLIENT_MODE_UDP_MULTICAST;
    }
    else {
        dltclient.mode = dltdata.yflag;
    }

    if (dltclient.mode == DLT_CLIENT_MODE_TCP || dltclient.mode == DLT_CLIENT_MODE_UDP_MULTICAST) {
        dltclient.port = dltdata.port;
        for (index = optind; index < argc; index++)
            if (dlt_client_set_server_ip(&dltclient, argv[index]) == -1) {
                fprintf(stderr, "set server ip didn't succeed\n");
                return -1;
            }

        if (dltclient.servIP == 0) {
            /* no hostname selected, show usage and terminate */
            fprintf(stderr, "ERROR: No hostname selected\n");
            usage();
            dlt_client_cleanup(&dltclient, dltdata.vflag);
            return -1;
        }
    }
    else {
        for (index = optind; index < argc; index++)
            if (dlt_client_set_serial_device(&dltclient, argv[index]) == -1) {
                fprintf(stderr, "set serial device didn't succeed\n");
                return -1;
            }

        if (dltclient.serialDevice == 0) {
            /* no serial device name selected, show usage and terminate */
            fprintf(stderr, "ERROR: No serial device name specified\n");
            usage();
            return -1;
        }

        dlt_client_setbaudrate(&dltclient, dltdata.bvalue);
    }

    /* initialise structure to use DLT file */
    dlt_file_init(&(dltdata.file), dltdata.vflag);

    /* first parse filter file if filter parameter is used */
    dlt_filter_init(&(dltdata.filter), dltdata.vflag);

    if (dltdata.fvalue) {
        if (dlt_filter_load(&(dltdata.filter), dltdata.fvalue, dltdata.vflag) < DLT_RETURN_OK) {
            dlt_file_free(&(dltdata.file), dltdata.vflag);
            return -1;
        }

        dlt_file_set_filter(&(dltdata.file), &(dltdata.filter), dltdata.vflag);
    }

    #ifdef EXTENDED_FILTERING

    if (dltdata.jvalue) {
        if (dlt_json_filter_load(&(dltdata.filter), dltdata.jvalue, dltdata.vflag) < DLT_RETURN_OK) {
            dlt_file_free(&(dltdata.file), dltdata.vflag);
            return -1;
        }

        dlt_file_set_filter(&(dltdata.file), &(dltdata.filter), dltdata.vflag);
    }

    #endif

    /* open DLT output file */
    if (dltdata.ovalue) {
        if (dltdata.climit > -1) {
            dlt_vlog(LOG_INFO, "Using file size limit of %" PRId64 "bytes\n",
                     dltdata.climit);
            dltdata.ohandle = dlt_receive_open_output_file(&dltdata);
        }
        else { /* in case no limit for the output file is given, we simply overwrite any existing file */
            dltdata.ohandle = open(dltdata.ovalue, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
        }

        if (dltdata.ohandle == -1) {
            dlt_file_free(&(dltdata.file), dltdata.vflag);
            fprintf(stderr, "ERROR: Output file %s cannot be opened!\n", dltdata.ovalue);
            return -1;
        }
    }

    if (dltdata.evalue)
        dlt_set_id(dltdata.ecuid, dltdata.evalue);
    else
        dlt_set_id(dltdata.ecuid, DLT_RECEIVE_ECU_ID);

    dltclient.serial_exists = 1;

    if (!g_quit_flag) {
        /* Connect to TCP socket or open serial device */
        if (dlt_client_connect(&dltclient, dltdata.vflag) != DLT_RETURN_ERROR) {
            DltReturnValue ret;

            /*start monitor thread*/
            dlt_receive_new_thread_monitor();

            /*send handshake msg firstly*/
            dlt_receive_send_get_software_version_msg();

            /* Dlt Client Main Loop */
            ret = dlt_client_main_loop(&dltclient, &dltdata, dltdata.vflag);
            //dlt_vlog(LOG_WARNING,"after dlt_client_main_loop, ret=%d\n",ret);
            if (ret == DLT_RETURN_PIPE_ERROR) {
                dlt_vlog(LOG_WARNING,"DLT_RETURN_PIPE_ERROR\n");
                result = DLT_RETURN_PIPE_ERROR;
            }
            //dlt_vlog(LOG_INFO,"main=%d\r\n",__LINE__);
            //should close fd
            if (dltclient.sock != -1) {
                close(dltclient.sock);
                dltclient.sock = -1;
            }
            //dlt_vlog(LOG_INFO,"main=%d\r\n",__LINE__);
            /*close fd of log file*/
            dlt_receive_close_output_file(&dltdata);

            dlt_receive_sync_network_stop();

            /*let's check monitor*/
            dlt_receive_no_quit_unless_requested(&dltclient, &dltdata, dltclient.serialDevice);

            /* Dlt Client Cleanup */
            dlt_client_cleanup(&dltclient, dltdata.vflag);
        }else {
            //should close fd
            if (dltclient.sock != -1) {
                close(dltclient.sock);
                dltclient.sock = -1;
            }
        }
    }else {
        /* Dlt Client Cleanup */
        dlt_client_cleanup(&dltclient, dltdata.vflag);
    }

    /*we need to check if it quits due to device node not exit */
    /*quit only after pressing ctrl+c*/
    dlt_vlog(LOG_INFO,"quit on the way\r\n");

    /*we should stop network upload status*/
    dlt_receive_sync_network_stop();

    /* dlt-receive cleanup */
    if (dltdata.ovalue)
        close(dltdata.ohandle);

    free(dltdata.ovaluebase);

    dlt_file_free(&(dltdata.file), dltdata.vflag);

    dlt_filter_free(&(dltdata.filter), dltdata.vflag);

    //return 0;
    return result;
}

int dlt_receive_message_callback(DltMessage *message, void *data)
{
    DltReceiveData *dltdata;
    static char text[DLT_RECEIVE_BUFSIZE];

    struct iovec iov[2];
    int bytes_written;

    if ((message == 0) || (data == 0))
        return -1;

    dltdata = (DltReceiveData *)data;

    /* prepare storage header */
    if (DLT_IS_HTYP_WEID(message->standardheader->htyp))
        dlt_set_storageheader(message->storageheader, message->headerextra.ecu);
    else
        dlt_set_storageheader(message->storageheader, dltdata->ecuid);

    if (((dltdata->fvalue || dltdata->jvalue) == 0) ||
        (dlt_message_filter_check(message, &(dltdata->filter), dltdata->vflag) == DLT_RETURN_TRUE)) {
        /* if no filter set or filter is matching display message */
        if (dltdata->xflag) {
            dlt_message_print_hex(message, text, DLT_RECEIVE_BUFSIZE, dltdata->vflag);
        }
        else if (dltdata->aflag)
        {

            dlt_message_header(message, text, DLT_RECEIVE_BUFSIZE, dltdata->vflag);

            printf("%s ", text);

            dlt_message_payload(message, text, DLT_RECEIVE_BUFSIZE, DLT_OUTPUT_ASCII, dltdata->vflag);

            printf("[%s]\n", text);
        }
        else if (dltdata->mflag)
        {
            dlt_message_print_mixed_plain(message, text, DLT_RECEIVE_BUFSIZE, dltdata->vflag);
        }
        else if (dltdata->sflag)
        {

            dlt_message_header(message, text, DLT_RECEIVE_BUFSIZE, dltdata->vflag);

            printf("%s \n", text);
        }

        /* if file output enabled write message */
        if (dltdata->ovalue) {
            iov[0].iov_base = message->headerbuffer;
            iov[0].iov_len = (uint32_t)message->headersize;
            iov[1].iov_base = message->databuffer;
            iov[1].iov_len = (uint32_t)message->datasize;

            if (dltdata->climit > -1) {
                uint32_t bytes_to_write = message->headersize + message->datasize;

                if ((bytes_to_write + dltdata->totalbytes > dltdata->climit)) {
                    dlt_receive_close_output_file(dltdata);

                    if (dlt_receive_open_output_file(dltdata) < 0) {
                        printf(
                            "ERROR: dlt_receive_message_callback: Unable to open log when maximum filesize was reached!\n");
                        return -1;
                    }

                    dltdata->totalbytes = 0;
                }
            }

            /*for tftp and ftp mode*/
            if (is_tftp() || is_ftp()) {
                size_t written_len = 0;
                written_len += slog_logfile_save(dltdata->ohandle, iov[0].iov_base, iov[0].iov_len);
                written_len += slog_logfile_save(dltdata->ohandle, iov[1].iov_base, iov[1].iov_len);

                if (written_len != (iov[0].iov_len + iov[1].iov_len)) {
                    dlt_vlog(LOG_ERR,"Failed to write whole messages %zu!=%zu\n",
                                     written_len,
                                     (iov[0].iov_len + iov[1].iov_len));
                }
                bytes_written = written_len;

                /*we may send more data to secure tftp/ftp connection*/

            } else {
                bytes_written = (int)writev(dltdata->ohandle, iov, 2);
            }

            dltdata->totalbytes += bytes_written;

            if (0 > bytes_written) {
                printf("dlt_receive_message_callback: writev(dltdata->ohandle, iov, 2); returned an error!");
                return -1;
            }
        }
    }

    return 0;
}

int dlt_receive_send_get_software_version_msg(void) {
    int result = 0;
    int try_times = 0;

    do {
        result = dlt_client_get_software_version(&dltclient);
        if ( try_times >= 5 ) {
            break;
        }
        try_times ++;
    }while(result != DLT_RETURN_OK);

    if (result != DLT_RETURN_OK) {
        dlt_vlog(LOG_ERR,"Failed to send handshake msg for output log! oh no!\n");
    }

    return result;
}

/*we use new defined msg to reach this purpose*/
int dlt_receive_send_stop_output_msg(void) {
    int result = 0;

    result = dlt_client_message_stop_output(&dltclient);

    if (result != DLT_RETURN_OK) {
        dlt_vlog(LOG_ERR,"Failed to send stop output msg for output log!\n");
    }

    return result;
}


/*use inotify to monitor*/
/*check if serial path exists or not*/
int dlt_receive_monitor_serial_path_create(DltClient * p_dltclient, char * serial_path)
{
    int wd;
    char buf[512];
    char *p_target_name = NULL;
    char *p_tmp;
    char monitor_path[512+1];
    int len;
    struct inotify_event *event;
    int fd;
    int nread;
    int i;
    int ret;
    int newfile_flag = 0;

    //dlt_vlog(LOG_WARNING,"dlt_receive_monitor_serial_path_create++ serial_path=%s\n",serial_path);

    fd = inotify_init();

    p_dltclient->inotify_fd = fd;

    if (fd< 0) {
        dlt_vlog(LOG_WARNING,"inotify_init failed errno=%d\n",errno);
        return -1;
    }

    strncpy(monitor_path, serial_path, sizeof(monitor_path)-1);

    //p_target_name = strrstr(monitor_path, "/");
    p_tmp = monitor_path;
    while (*p_tmp != '\0') {
        if (*p_tmp == '/') {
            p_target_name = p_tmp;
        }
        p_tmp++;
    }
    if (p_target_name != NULL) {
        *p_target_name = '\0';
        p_target_name++;
        dlt_vlog(LOG_DEBUG,"monitor_path=%s, name=%s\r\n",monitor_path,p_target_name);
    }

    wd = inotify_add_watch(fd, monitor_path, IN_CREATE);   // old is IN_ALL_EVENTS //IN_CLOSE
    if (wd < 0) {
        dlt_vlog(LOG_WARNING,"inotify_add_watch %s failed , %s,wd=%d\n", monitor_path, strerror(errno),wd);
        close(fd);
        p_dltclient->inotify_fd = -1;
        return -1;
    }

    buf[sizeof(buf) - 1] = 0;

    //it will block when no event? Yes
    while ( (newfile_flag == 0) && ((len = read(fd, buf, sizeof(buf) - 1)) > 0 ) ) {
        nread = 0;
        while ( len > 0 ) {
            event = (struct inotify_event *)&buf[nread];
            for (i=0; i<EVENT_NUM; i++) {
                if ((event->mask >> i) & 1) {
                    if (event->len > 0) {
                        if (strcmp("IN_CREATE", event_str[i]) == 0) {
                            //got it new file, later try to send.
                            //newfile_flag = 1;
                            if (event->name != NULL) {
                                //dlt_vlog(LOG_DEBUG,"event->name=0x%x: %s\r\n",event->name, event->name);
                                if (strcmp(p_target_name,event->name) == 0)
                                {
                                    newfile_flag = 1;
                                    //printf("event->name=%s\r\n",event->name);
                                }
                            }
                        }
                    }
                }
            }
            nread = nread + sizeof(struct inotify_event) + event->len;
            len = len - sizeof(struct inotify_event) - event->len;
        }
    }

    ret = inotify_rm_watch (fd, wd);
    ret = ret;
    wd = -1;

    close(fd);
    p_dltclient->inotify_fd = -1;

    //dlt_vlog(LOG_WARNING,"dlt_receive_monitor_serial_path_create-- %d\n",newfile_flag);

    return newfile_flag;
}

/*check if serial path deleted or not*/
int dlt_receive_monitor_serial_path_status(DltClient * p_dltclient, char * serial_path)
{
    int wd;
    char buf[512];
    char *p_target_name = NULL;
    char *p_tmp;
    char monitor_path[512+1];
    int len;
    struct inotify_event *event;
    int fd;
    int nread;
    int i;
    int ret;
    int newfile_flag = 0;

    //dlt_vlog(LOG_WARNING,"dlt_receive_monitor_serial_path_status++ serial_path=%s\n",serial_path);

    fd = inotify_init();

    if (fd< 0) {
        dlt_vlog(LOG_WARNING,"inotify_init failed errno=%d\n",errno);
        return -1;
    }

    strncpy(monitor_path, serial_path, sizeof(monitor_path)-1);

    p_tmp = monitor_path;
    while (*p_tmp != '\0') {
        if (*p_tmp == '/') {
            p_target_name = p_tmp;
        }
        p_tmp++;
    }
    if (p_target_name != NULL) {
        *p_target_name = '\0';
        p_target_name++;
        dlt_vlog(LOG_DEBUG,"monitor_path=%s, name=%s\r\n",monitor_path,p_target_name);
    }

    wd = inotify_add_watch(fd, monitor_path, IN_DELETE|IN_CREATE);   // old is IN_ALL_EVENTS //IN_CLOSE
    if (wd < 0) {
        dlt_vlog(LOG_WARNING,"inotify_add_watch %s failed , %s,wd=%d\n", monitor_path, strerror(errno),wd);
        close(fd);
        return -1;
    }

    buf[sizeof(buf) - 1] = 0;

    //it will block when no event? Yes
    while ( (newfile_flag == 0) && ((len = read(fd, buf, sizeof(buf) - 1)) > 0 ) ) {
        nread = 0;
        while ( len > 0 ) {
            event = (struct inotify_event *)&buf[nread];
            for (i=0; i<EVENT_NUM; i++) {
                if ((event->mask >> i) & 1) {
                    if (event->len > 0) {
                        if (strcmp("IN_DELETE", event_str[i]) == 0) {
                            if (event->name != NULL) {
                                //dlt_vlog(LOG_NOTICE,"event->name=0x%x: %s\r\n",event->name, event->name);
                                if (strcmp(p_target_name,event->name) == 0)
                                {
                                    p_dltclient->serial_exists = 0;
                                    dlt_vlog(LOG_NOTICE,"Got deleted event, receiver.fd=%d\r\n",p_dltclient->receiver.fd);
                                }
                            }
                        }else if (strcmp("IN_CREATE", event_str[i]) == 0) {
                            if (event->name != NULL) {
                                //dlt_vlog(LOG_NOTICE,"event->name=0x%x: %s\r\n",event->name, event->name);
                                if (strcmp(p_target_name,event->name) == 0)
                                {
                                    p_dltclient->serial_exists = 1;

                                    dlt_vlog(LOG_NOTICE,"Got create event, receiver.fd=%d\r\n",p_dltclient->receiver.fd);
                                }
                            }
                        }
                    }
                }
            }
            nread = nread + sizeof(struct inotify_event) + event->len;
            len = len - sizeof(struct inotify_event) - event->len;
        }
    }

    ret = inotify_rm_watch (fd, wd);
    ret = ret;
    wd = -1;

    close(fd);

    //dlt_vlog(LOG_WARNING,"dlt_receive_monitor_serial_path_status-- %d\n",newfile_flag);

    return newfile_flag;
}


int dlt_receive_no_quit_unless_requested(DltClient * dltclient,
    DltReceiveData* dltdata, char* target_serial_path) {
    int result = 0;
    /*let's check monitor*/
    while (!g_quit_flag && target_serial_path != NULL) {
        /*don't quit, we should detect this device node firstly*/
        {
            int newfd;
            int result2;
            do {
                newfd = open(target_serial_path, O_RDWR);
                if (newfd < 0) {
                    result2 = dlt_receive_monitor_serial_path_create(dltclient, target_serial_path);
                    if (result2 > 0) {
                        // wait for a while
                        // avoid the noise of ModemManager
                        if (!g_quit_flag) {
                            sleep(30);
                        }
                    }
                } else {
                    close(newfd);
                    newfd = -1;
                    break;
                }
            } while((newfd < 0) && (!g_quit_flag));

            /*quit right now for ctrl+c case*/
            if(g_quit_flag) {
                break;
            }

            //dlt_vlog(LOG_INFO,"let's retry open serial path(%s)\r\n",target_serial_path);

            /*open fd of log file*/
            /* open DLT output file */
            if (dltdata->ovalue) {
                if (dltdata->climit > -1) {
                    dltdata->ohandle = dlt_receive_open_output_file(dltdata);
                }
                else { /* in case no limit for the output file is given, we simply overwrite any existing file */
                    dltdata->ohandle = open(dltdata->ovalue, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
                }

                if (dltdata->ohandle == -1) {
                    // no need to free dltdata->file
                    //dlt_file_free(&(dltdata->file), dltdata->vflag);
                    dlt_vlog(LOG_WARNING,"ERROR: Output file %s cannot be opened!(%d) %s\n", dltdata->ovalue, errno,strerror(errno));
                    //return -1;
                }
            }

            if (dlt_client_connect(dltclient, dltdata->vflag) != DLT_RETURN_ERROR) {
                DltReturnValue ret2;

                /*send handshake msg firstly*/
                dlt_receive_send_get_software_version_msg();

                /* Dlt Client Main Loop */
                ret2 = dlt_client_main_loop(dltclient, dltdata, dltdata->vflag);
                if (ret2 == DLT_RETURN_PIPE_ERROR) {
                    dlt_vlog(LOG_WARNING,"DLT_RETURN_PIPE_ERROR %d\n",__LINE__);
                    result = DLT_RETURN_PIPE_ERROR;
                }
            }
            //dlt_vlog(LOG_INFO,"new loop main=%d\r\n",__LINE__);
            //should close fd
            if (dltclient->sock != -1) {
                close(dltclient->sock);
                dltclient->sock = -1;
            }
            //dlt_vlog(LOG_INFO,"new loop main=%d\r\n",__LINE__);

            /*close fd of log file*/
            dlt_receive_close_output_file(dltdata);

            dlt_receive_sync_network_stop();
        }
        sleep(2);
    }
    return result;
}

int dlt_receive_update_latest_read_time(void) {
    //dlt_vlog(LOG_WARNING,"dlt_receive_update_latest_read_time");
    clock_gettime(CLOCK_MONOTONIC, &start_before_read);
    return 0;
}

int dlt_receive_compare_current_interval_larger_than(long seconds) {
    long diff_in_s;
    struct timespec current_time = {0,0};

    clock_gettime(CLOCK_MONOTONIC, &current_time);

    diff_in_s = (current_time.tv_sec - start_before_read.tv_sec);
    //dlt_vlog(LOG_WARNING,"compare_current_interval diff_in_s=%ld,start.tv_sec=%ld\n",diff_in_s,start_before_read.tv_sec);
    if (diff_in_s > seconds) {
        return 1;
    }

    return 0;
}

void *thread_monitor_func(void *arg)
{
    arg = arg;
    while(!g_quit_flag) {
        dlt_receive_monitor_serial_path_status(&dltclient, dltclient.serialDevice);
    }
    return NULL;
}

int dlt_receive_new_thread_monitor(void) {
    int result = 0;
    {
        pthread_t thread_id;
        pthread_attr_t attr;
        struct sched_param param;

        pthread_attr_init(&attr);
        pthread_attr_getschedparam(&attr, &param);
        param.sched_priority=98;
        pthread_attr_setschedparam(&attr, &param);
        pthread_attr_setschedpolicy(&attr, SCHED_RR);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

        if (pthread_create(&thread_id, &attr, thread_monitor_func, NULL))
        {
            dlt_vlog(LOG_WARNING,"dlt_receive_new_thread_monitor create failed, errno:%d(%s)\n", errno, strerror(errno));
            pthread_attr_destroy(&attr);
			result = -1;
            goto ERROR;
        }
    }

ERROR:
	return result;

}

int dlt_receive_sync_network_stop(void) {
    /*give it chance to tftp thread for last ack.*/
    int wait_times = 0;
#define WAIT_TFTP_TIMES (6)
    while(is_transfer_ongoing() && (wait_times++ < WAIT_TFTP_TIMES)) {
        //dlt_vlog(LOG_NOTICE,"waiting for tftp thread quit...\n");
        sleep(1);
    }
    if (wait_times > WAIT_TFTP_TIMES) {
        dlt_vlog(LOG_NOTICE,"time out for waiting for tftp thread quit...\n");
    }

    /*ftp connection finish*/
    s_ftp_quit();

    return 0;
}
