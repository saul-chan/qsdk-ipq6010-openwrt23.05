/*******************************************************************
 *          CopyRight(C) 2024-2028  Fibocom Wireless Inc
 *******************************************************************/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <signal.h>
#include "samsung_devices_list.h"


#include "fifo.h"
#include "log_control.h"
#include <sys/stat.h>
#include <sys/inotify.h>
#include "ss_ftpclient.h"

//new added
#include <stdbool.h> 
#include <sys/statfs.h>

#include "samsung_log_main.h"

#include <sys/types.h>
#include <sys/wait.h>


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


#define LOG_BUFFER_SIZE     (32*1024) // 32K
#define LOG_WRITE_BUFFER_SIZE (128*1024)
#define FIFO_BUFFER_SIZE    (512*1024)

#define BUFFER_SIZE (64*1024)
#define MAX_BUFFER_SIZE (500*1024)

#define BUF_SIZE               1024

#define PATH_SIZE (256)

#define ERROR_UPLOAD_RETRY  (-1000)

#define TFTP_F "tftp:"
#define FTP_F  "ftp:"
//about 2 hour // 10s for about 270MB local test
#define FTP_UPLOADING_WAITING_TIME      (3600*2)

#define  AP_LOG_NAME "ap_log.dlt"
#define  CP_SILENT_LOG_NAME "sbuff_main.sdm"
#define  CP_SILENT_HIDDEN_HEADER_FILE_NAME ".sbuff_header.sdm"


#define LOGFILE_SIZE_MIN (2*1024*1024)
#define LOGFILE_SIZE_MAX (512*1024*1024)
#define LOGFILE_SIZE_DEFAULT (512*1024*1024)
#define LOGFILE_NUM 512
#define PCIE_LOG_DEVICES_AP_NO      0

#define AP_LOG_PCIE_DEVICE_PATH    "/dev/ttyMD_ap0"


#define CP_SILENT_LOGGING_A_PARAMETER      (2)
#define CP_SILENT_LOGGING_PCIE_DEVICE_PATH    "/dev/ttyMD_dm0"

#define CONFIG_SCOM_DUMP_LOCAL
#define SDUMP_LOG_NEED_SPACE    1024

typedef struct {
    int (*init_filter)(int fd, const char *conf);
    int (*clean_filter)(int fd);
    int (*logfile_create)(const char *logfile_dir, const char *logfile_suffix, unsigned logfile_seq);
    int (*logfile_init)(int logfd, unsigned logfile_seq);
    size_t (*logfile_save)(int logfd, const void *buf, size_t size, int logtype);
    int (*logfile_close)(int logfd);
} slog_ops_t;


typedef struct
{
    const slog_ops_t *ops;
    int fd;
    const char *filter;
} init_filter_cfg_t;

typedef struct
{
    int usbfd;
    int ep;
    int outfd;
    int rx_size;
    const char *dev;
} usbfs_read_cfg_t;
#if 0
typedef struct
{
    int dm_ttyfd;
    int dm_usbfd;
    int dm_sockets[2];
    pthread_t dm_tid;
    usbfs_read_cfg_t cfg;

    int general_ttyfd;
    int general_usbfd;
    int general_sockets[2];
    pthread_t general_tid;
} ql_fds_t;
#endif

struct argument
{
    // arguments
    char ttyDM[256];
    char logdir[256];
    char ttyGENERAL[256];

    // configurations
    int logfile_num;
    int logfile_sz;
    const char *filter_cfg;
    const char *delete_logs; // Remove all logfiles in the logdir before catching logs

    //const  struct ql_usb_device_info *ql_dev;
    // profiles

    //ql_fds_t fds;
};
//static struct argument *slog_args;





char s_set_ftpipaddr=0;
char s_set_username=0;
char s_set_passwd=0;

char username[128];/* username for ftp */
char password[128];/* password for ftp */
char ftpservipaddr[128];

char g_ftpfilename[256];
//char use_ftp=0;
char g_filename[128];
pthread_t g_tid_logtest;
char g_str_ss_log_dir[1024] ;

char g_str_sub_ss_log_dir[128];
char taskinfo1_portname[512];

//this is for dlt log filter file
char s_filter_filename[256];

const char *s_ftp_server_ip = NULL;
const char *s_ftp_server_usr = NULL;
const char *s_ftp_server_pass = NULL;
int ftp_put_ss_flag = 0;
static volatile int s_log_process = 0;
static char s_logpath[FIBO_BUF_SIZE+EXTEND] = ".";
static int single_logfile_size = 50*1024*1024;
//static struct fifo *plog_fifo = NULL;
static int s_max_file_num = 0;
static platform_enum samsung_platform = NONE;

static int samsung_tftp_mode = 0;

//static int g_fd_port = -1;
static volatile int g_is_ss_ap_logging = 0;
static volatile int g_exit_flag = 0;

extern char *s_tftp_server_ip;
static pid_t ap_log_child_pid = -1;

static char s_serial_path[PATH_SIZE] = {0};

int s_work_folder_changed = 0;

int s_dlt_log_in_debug_mode = 0;
int s_cp_silent_logging_mode = 0;
int s_cp_tcp_log_config = -1;



//functions
char * monitor_new_file(char * monitor_path);

int samsung_ap_log_and_transfer(void) ;
int ss_new_thread_monitor_log_file(void);
int delete_ap_log_file(const char *filename);
char *join_path(const char *path, const char *name);
char * ss_fibo_create_log_folder_path(char*log_dir);



static int is_tftp(void)
{
    return (s_tftp_server_ip != NULL);
}

static int is_ftp(void)
{
    //return (s_ftp_server_ip != NULL);
    return (s_set_ftpipaddr && s_set_username && s_set_passwd);
}


fibo_usbdev_t *samsung_find_devices_in_table(int idvendor, int idproduct)
{
    int i, size = 0;

    size = sizeof(samsung_devices_table)/sizeof(samsung_devices_table[0]);
    for (i=0; i<size; i++)
    {
        fibo_usbdev_t *udev = &samsung_devices_table[i];

        if ((udev->idVendor == idvendor) && (udev->idProduct == idproduct)) {
            return udev;
        }
    }

    return NULL;
}

#if 0
static int fibo_send_HeartBeat_data(fibo_usbdev_t *pdev)
{
    const char databuf[] = {0x01,0xaa,0xaa,0xaa,0x01,0x55,0x73,0x01,0x14,0x00,0x00,0x00,0x06,0x67,0xbb,0xbb,0x04,
                      0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x03,0x00,0x00,0x00,0x44,0x09,0x7e,};
    int write_len = 0, left_len = sizeof(databuf);

    while (left_len > 0) {
        int retval = pdev->write(pdev, (char *)databuf+write_len, left_len, 0);
        if (retval < 0) {
            LogInfo("%s error,retval:%d\n", __func__, retval);
            return retval;
        }
        else if (retval == 0) {
            break;
        }
        left_len -= retval;
        write_len += retval;
    }

    if (write_len != sizeof(databuf)) {
        LogInfo("%s failed. write_len:%d\n", __func__, write_len);
    }

    return 0;
}
#endif

platform_enum fibo_get_samsung_platform(fibo_usbdev_t *pdev)
{
    int bInterfaceSubClass=0;

    if(pdev == NULL)
        return NONE;

    if((pdev->idVendor == 0x2CB7) && (pdev->idProduct == 0x0001))
    {
        bInterfaceSubClass = fibo_get_usb_Interface6_SubClass(pdev);
    }

    if(bInterfaceSubClass == 0x42)
        return V3T;
    else if(bInterfaceSubClass == 0xFF)
        return V3E;
    else
        return NONE;
}

static void fibo_exit_function(int msg)
{
    /*willa.liu@20241111 JIRA:MBB0678-502 begin*/
    static int exit_protect_flag = 0;
    if (exit_protect_flag > 0) return;

    exit_protect_flag++;
    LogInfo(" : %d,%d\n",  msg, exit_protect_flag);
    /*willa.liu@20241111 JIRA:MBB0678-502 end*/
    log_storage_control(NULL, 0, 0);
    s_log_process = 0;

    //sleep(1);
    do {
        sleep(1);
    } while(ap_log_child_pid >= 0);
    //printf("fibo_exit_function quit, no log now...");
    /*willa.liu@20241111 JIRA:MBB0678-502 begin*/
#if 0
    LogInfo("\n%s: %d Enable Ctrl+C\n", __func__, msg);
    signal(SIGINT, SIG_DFL); //Enable Ctrl+C to exit
#else
    exit(-0xe);
#endif
    /*willa.liu@20241111 JIRA:MBB0678-502 end*/
}

static void usage(char *arg)
{
    LogInfo("========================================\n");
    LogInfo("Usage:\n");
    LogInfo("%s <-d [diag port]> <-s [log save dir]> <-m [single logfile size(MB)]> <-n [max_log_filenum]> <-i [tftp:][ip] <-u username -w password>> <-f ap_log_filter_file>\n", arg);
    LogInfo("example: %s\n", arg);
    LogInfo("========================================\n");
}

#if 0
static void parser_ftp(const char *str)
{
    printf("parser_ftp  str=%s\n",str);
    if(!strncmp(str, FTP_F, strlen(FTP_F)))
    {
        static char g_ftp_server_ip_temp[16] = {0};
        static char g_ftp_server_usr_temp[32] = {0};
        static char g_ftp_server_pass_temp[32] = {0};
        char *buf_temp1 = NULL;
        char *buf_temp2 = NULL;
        buf_temp1 = strstr(str,"user:");
        buf_temp2 = strstr(str,"pass:");
        if(!buf_temp1 || !buf_temp2)
             exit(1);

        strncpy(g_ftp_server_ip_temp, str+4, buf_temp1 - str - 5);
        strncpy(g_ftp_server_usr_temp, buf_temp1+5, buf_temp2 - buf_temp1 - 6);
        strncpy(g_ftp_server_pass_temp, buf_temp2+5, strlen(buf_temp2) - 5);
        s_ftp_server_ip = g_ftp_server_ip_temp;
        s_ftp_server_usr = g_ftp_server_usr_temp;
        s_ftp_server_pass = g_ftp_server_pass_temp;

		printf("s_ftp_server_ip=%s\n",s_ftp_server_ip);
		printf("s_ftp_server_usr=%s\n",s_ftp_server_usr);
		printf("s_ftp_server_pass=%s\n",s_ftp_server_pass);
    }
}
#endif


int samsung_log_main(int argc, char **argv)
{
    int opt = -1;  //ret = -1, 
    char portname[FIBO_BUF_SIZE] = {0};
    char log_dir[FIBO_BUF_SIZE] = ".";

    LogInfo("start\n" );

	s_filter_filename[0] = '\0';

    optind = 1;//must set to 1

	// "d:p:P:s:a:m:n:i:u:w:f:t:h"   
    //while ((opt = getopt(argc, argv, "d:s:m:n:t:h:f:i")) != -1)   // -i optarg=0, crash
    while ((opt = getopt(argc, argv, "d:p:P:s:a:m:n:i:u:w:f:t:hvc:g:")) != -1)   //works normally
    {
        switch (opt)
        {
            case 'd':
                strcpy(portname, optarg);
                {
                    int fd = open(portname, O_RDWR);
                    if (fd < 0 ) {
                        LogInfo("Failed to open serial port(%s) fd: %d,errno=%d(%s)\n",portname, fd, errno, strerror(errno));
                        exit(1);
                    } else {
                        close(fd);
                        strncpy(s_serial_path , portname , (sizeof(s_serial_path) - 1));
                    }
                }
                break;
            case 's':
                strncpy(log_dir, optarg, sizeof(log_dir)-1);
                break;
            case 'm'://atoll
                {
                    int mm = atoi(optarg);
                
                    //single_logfile_size = atoi(optarg)*1024*1024;
                    single_logfile_size = mm*1024*1024;
                    if (single_logfile_size <= 0) {
                        LogInfo("[Warning]mm=%d,single_logfile_size=%d\n",mm , single_logfile_size);
                    }
                }
                break;
            case 'n':
                s_max_file_num = atoi(optarg);
                break;
            case 't':
                samsung_platform = atoi(optarg);
                LogInfo("platform is samsung\n");
                break;
            case 'f':
                strncpy(s_filter_filename, optarg, sizeof(s_filter_filename)-1);
                LogInfo("filter file is provided(%s)\n" , s_filter_filename);
                break;

            case 'i':
                if (!strncmp(optarg, TFTP_F, strlen(TFTP_F))) {
                    s_tftp_server_ip = optarg+strlen(TFTP_F);
                    printf("checking Tftp connection...\n");
                    if (samsung_tftp_test_server(s_tftp_server_ip)) {
                        LogInfo("save log to tftp server %s\n", s_tftp_server_ip);
                    } else {
                        //goto error;
                        exit(1);
                    }
                } else {
                    snprintf(ftpservipaddr, 128, "%s", optarg);
                    s_set_ftpipaddr = 1;
                    LogInfo("set_ftpipaddr successfully,is %s\n", ftpservipaddr);
                }
                break;
            case 'u':
                snprintf(username, 128, "%s", optarg);
                s_set_username = 1;
                LogInfo("set_username successfully,is %s\n", username);
                break;
            case 'w':
                snprintf(password, 128, "%s", optarg);
                s_set_passwd = 1;
                LogInfo("set_password successfully,is %s\n", password);
                break;
            case 'h':
                usage(argv[0]);
                return 0;
            case 'v':
                s_dlt_log_in_debug_mode = 1;
                break;
            case 'c':
            {
                int log_type = atoi(optarg);
                if ( CP_SILENT_LOGGING_A_PARAMETER == log_type) {
                    s_cp_silent_logging_mode = 1;
                    LogInfo("CP silent logging mode enabled!\n");
                }
            }
                break;
            case 'g':
                s_cp_tcp_log_config = atoi(optarg);
                LogInfo("tcp log %d\n",s_cp_tcp_log_config);
                break;
            case '?':
                LogInfo("input parameter is NOT correct! please check.\n");
                exit(1);
                break;
            default:;
        }
    }

    if (samsung_tftp_mode > 0 || samsung_tftp_mode <= 0) {
        char * current_path;
        char * sub_path = NULL;
        char * sub_path2 = NULL;
        char * sub_path3 = NULL;

        if (s_serial_path[0] == '\0') {
            LogInfo("No serial path is provided! please check -d config.\n");
            goto ERROR_QUIT;
        }

        //give another chance to detect cp silent logging mode
        if (strcmp(CP_SILENT_LOGGING_PCIE_DEVICE_PATH, s_serial_path) == 0) {
            if (s_cp_silent_logging_mode !=1 ) {
                s_cp_silent_logging_mode = 1;
                LogInfo("CP silent logging mode enabled auto fixed!\n");
            }
        }
        if (strcmp(AP_LOG_PCIE_DEVICE_PATH, s_serial_path) == 0) {
            if (s_cp_silent_logging_mode == 1 ) {
                s_cp_silent_logging_mode = 0;
                LogInfo("AP logging mode enabled auto fixed!\n");
            }
        }


        if (is_ftp()) {
            //test ftp server firstly.
            char* ss_ftp_detect_param[4]={"ss_ftp_detect_main", ftpservipaddr,username,password};
            printf("checking FTP connection...\n");
            if (ss_ftp_server_test_connection_is_ok(4,ss_ftp_detect_param)) {
                exit(1);
            }
        }

        // for init g_str_sub_ss_log_dir
        g_str_sub_ss_log_dir[0] = '\0';

        current_path = ss_fibo_create_log_folder_path(log_dir);

        //setup log path
        if (current_path != NULL) {
            /*new mode, without storing log file for new tftp/ftp mode, so don't check left storage*/
            if ( !is_tftp() && !is_ftp()) {
                //check left storage, double is needed.
                if (!slog_avail_space_for_dump(current_path , 2*(single_logfile_size/(1024*1024))) ) {
                    LogInfo("No enough storage to store log! please leave at least %dMB.\n",2*(single_logfile_size/(1024*1024)));
                    goto ERROR_QUIT;
                }
            }

            /* 1. setup g_str_ss_log_dir*/
            sprintf(g_str_ss_log_dir , "%s", current_path);

            /* 2. setup g_str_sub_ss_log_dir*/
            sub_path = current_path;

            do {
                sub_path = strchr(sub_path, '/');

                if(sub_path != NULL) {
                    sub_path ++;

                    sub_path2 = strchr(sub_path, '/');
                    sub_path3 = strrchr(sub_path, '/');

                    //if the last / is left, then it is over.
                    if ((sub_path2 != NULL && (sub_path2 == sub_path3)))	{
                        *sub_path2 = '\0';
                        sprintf(g_str_sub_ss_log_dir, "%s", sub_path);
                        *sub_path2 = '/';
                        break;
                    } else if ((sub_path2 == NULL)) {
                        sprintf(g_str_sub_ss_log_dir, "%s", sub_path);
                        break;
                    }
                }
            } while(sub_path != NULL);
        }

        //tftp and ftp handled in child process directly

        //for ap log..
        samsung_ap_log_and_transfer();

        return 0;
    }

ERROR_QUIT:
    exit(1);
    return -1;
}

// new added
//start another sub exec to run dlt-logcat tool
int ss_serial_logstart(const char *portname, const char *syspath, const char *log_dir, const char *cfg_name)
{
    int result;
    result = ss_new_thread_start_dlt_log_process();
    return result;
}

static int ss_serial_logstop()
{
    int ret = -1;

    if (g_is_ss_ap_logging)
    {

    }

    ret = 0;
//END:

    printf("[%s]\n", __func__);

    return ret;
}


int samsung_ap_log_and_transfer(void) {
    g_is_ss_ap_logging = 1;

    signal(SIGINT, fibo_exit_function);
    signal(SIGTERM, fibo_exit_function);

    if (ss_serial_logstart(NULL, NULL, s_logpath, NULL) < 0)
    {
        /*failed to start log process*/
        exit(1);

        goto error_exit;
    }

    /*start monitor log file only for non-tftp and non-ftp mode*/
    if (!is_tftp() && !is_ftp()) {
        //start new thread.
        if (ss_new_thread_monitor_log_file() < 0 ){
            exit(1);
        }
    }

    while (g_is_ss_ap_logging)
    {
        sleep(5);
    }

error_exit:
    ss_serial_logstop();

    //printf("samsung_ap_log_and_transfer --\n");

    return 0;
}

int can_open_message_file(char* jason_message_file)
{
    int result ;
    FILE *pfile;

    pfile = fopen(jason_message_file, "rb");
    if (!pfile) {
        result = 0; //failed to open file, then retry next file.
        printf("WARNING: Cannot open %s: %s,errno=%d\n",
                jason_message_file, strerror(errno),errno);
    } else {
        result = 1;
        fclose(pfile);
    }

    return result;
}


/*caller should free return char * if it is valid*/
/* except for ap log file */
char* get_next_message_file(const char *dirname)
{
    DIR *d;
    struct dirent *de;
    struct stat st;

    char* next_file = NULL;

    d = opendir(dirname);
    if (!d) {
        printf("scan_folder failed to opendir dirname=%s, %s\n", dirname,strerror(errno));
        return NULL;
    }

    while ((de = readdir(d)) != NULL) {
        char *tmpname = NULL;

        if (strcmp(de->d_name, ".")==0
            || strcmp(de->d_name, "..")==0
            || strcmp(de->d_name, AP_LOG_NAME)==0   // AP_LOG_NAME is specical
            || strcmp(de->d_name, CP_SILENT_LOG_NAME) == 0    // CP_SILENT_LOG_NAME is specical
            || strcmp(de->d_name, CP_SILENT_HIDDEN_HEADER_FILE_NAME) == 0)   //keep CP_SILENT_HIDDEN_HEADER_FILE_NAME
            continue;

        tmpname = join_path(dirname, de->d_name);
        if (tmpname != NULL) {
            if (lstat(tmpname, &st) < 0) {
                printf("stat(%s): %s\n", tmpname, strerror(errno));
                free(tmpname);
                tmpname = NULL;
                continue;
            }

            if (S_ISREG(st.st_mode)) {
                //find one

                if (!can_open_message_file(tmpname)) {
                    //then try next file.
                    printf("failed open, then try next file\n");

                    free(tmpname);
                    tmpname = NULL;
                    continue;
                }

                next_file = tmpname;
                tmpname = NULL;
                break;
            } else if (S_ISDIR(st.st_mode)) {
                //do nothing...
            }

            free(tmpname);
        }
    }

    closedir(d);

    return next_file;
}

int slog_control_new_file(char * monitor_path, char * new_file) {
    static char detected_new_file[2*PATH_SIZE+1];

    snprintf(detected_new_file ,sizeof(detected_new_file),  "%s%s" ,monitor_path, new_file);

    //added for new
    if (s_max_file_num >= 2) {
        log_storage_control(detected_new_file, (s_max_file_num - 1), 1);
    } else if (s_max_file_num == 1) {
        //remove it directly, keep only one file.
        delete_ap_log_file(detected_new_file);
    }

    return 0;
}


/*caller should free return char * if it is valid*/
/*add record right now*/
char * monitor_new_file(char * monitor_path)
{
    int wd;
    char buf[256];
    char * newfile = NULL;
    int len;
    struct inotify_event *event;
    int fd;
    int nread;
    int i;
    int ret;
    int newfile_flag = 0;
    size_t copy_len = 0;
    //printf("monitor_new_file++\n");

    fd = inotify_init();

    if (fd< 0) {
        printf("inotify_init failed\n");
        return newfile;
    }

    wd = inotify_add_watch(fd, monitor_path, IN_MOVED_TO);   // old is IN_ALL_EVENTS //IN_CLOSE
    if (wd < 0) {
        printf("inotify_add_watch %s failed , %s,wd=%d\n", monitor_path, strerror(errno),wd);
        close(fd);
        return newfile;
    }
    buf[sizeof(buf) - 1] = 0;

    //it will block when no event?
    while ( (newfile_flag == 0) && ((len = read(fd, buf, sizeof(buf) - 1)) > 0 ) ) {
        nread = 0;
        while ( len > 0 ) {
            event = (struct inotify_event *)&buf[nread];
            for (i=0; i<EVENT_NUM; i++) {
                if ((event->mask >> i) & 1) {
                    if (event->len > 0) {
                    //for test, remove it later
                    #if 0
                    if (i != 1)  // except IN_MODIFY
                    {
                        static char record_file_name[512] ;
                        //got it new file, later try to send.
                        //newfile_flag = 1;
                        //newfile = (char*)malloc(PATH_SIZE);
                            {
                                memset(record_file_name, 0x00, sizeof(record_file_name));

                                if (event->len >= PATH_SIZE) {
                                    copy_len = PATH_SIZE-1;
                                } else {
                                    copy_len = event->len;
                                }
                                memcpy(record_file_name, event->name, copy_len);
								printf("event[%d]=%s, filename=%s\n",i,event_str[i],record_file_name);
                            }	
                     }
                     #endif
                        //if (strcmp("IN_CLOSE_WRITE", event_str[i]) == 0
                        //    ||strcmp("IN_CLOSE_NOWRITE", event_str[i]) == 0) {
                        if (strcmp("IN_MOVED_TO", event_str[i]) == 0) {
                            #if 0
                            //got it new file, later try to send.
                            newfile_flag = 1;
                            newfile = (char*)malloc(PATH_SIZE);
                            if (newfile != NULL) {
                                memset(newfile, 0x00, PATH_SIZE);

                                if (event->len >= PATH_SIZE) {
                                    copy_len = PATH_SIZE-1;
                                } else {
                                    copy_len = event->len;
                                }
                                memcpy(newfile, event->name, copy_len);
                            }
                            #else
                            static char got_file[PATH_SIZE+1];
                            memset(got_file, 0x00, sizeof(got_file));

                            if (event->len >= PATH_SIZE) {
                                copy_len = PATH_SIZE-1;
                            } else {
                                copy_len = event->len;
                            }
                            memcpy(got_file, event->name, copy_len);

                            slog_control_new_file(monitor_path, got_file);
                            #endif
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

    //printf("monitor_new_file-- %lx\n",(unsigned long)newfile);

    return newfile;
}

/* thread function --monitor log folders*/
static void *thread_monitor_logfile(void *arg)
{
    char * new_log_file;
	char * monitor_path = s_logpath;

    //printf("%s ++\n", __func__);

    while(g_is_ss_ap_logging) {
        //if (s_work_folder_changed > 0) {
		//    monitor_path = "./";
        //}
        new_log_file = monitor_new_file(monitor_path);
        if (new_log_file != NULL) {
            free(new_log_file);
        }

        //sleep(1);
    }


    //printf("%s --\n", __func__);

    return NULL;
}


int ss_new_thread_monitor_log_file(void) {
    int result = 0;
    //if (s_set_ftpipaddr && s_set_username && s_set_passwd)
    {
        pthread_t thread_id;
        pthread_attr_t attr;
        struct sched_param param;

        //printf("ss_new_thread_monitor_log_file\n");
        pthread_attr_init(&attr);
        pthread_attr_getschedparam(&attr, &param);
        param.sched_priority=98;
        pthread_attr_setschedparam(&attr, &param);
        pthread_attr_setschedpolicy(&attr, SCHED_RR);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

        if (pthread_create(&thread_id, &attr, thread_monitor_logfile, NULL))
        {
            printf("pthread_create log monitor thread create failed, errno:%d(%s)\n", errno, strerror(errno));
            pthread_attr_destroy(&attr);
			result = -1;
            goto ERROR;
        } else {
            //printf("create monitor thread ok\n");
        }
    }


ERROR:
	return result;

}

static void *thread_monitor_child_dlt_log_process(void *arg)
{
    int status;
    pid_t child_id;
    pid_t return_id;
    //printf("[%s]\n", __func__);
    //char tmp_log_dir[20] = "./";

    /* create child process to catch ap log*/
    switch((child_id = fork())){
        case -1:
            fprintf(stderr,"fork dlt log failed\n");
            exit(1);
            break;
        case 0:
            if (s_cp_silent_logging_mode != 1)
            {
                //	dlt-logcat -o dlt_log.dlt -c 1M -y /dev/ttyACM1
                char* argv[24];
                char log_path[1024] = {0};
                char limited_size_perFile[64] = {0};
                char ap_log_max_count[64] = {0};
                int index = 0;

                snprintf(log_path , sizeof(log_path)-1, "%s%s", s_logpath, AP_LOG_NAME);

                if (single_logfile_size < 1024) {
                    single_logfile_size = 50*1024;
                    sprintf(limited_size_perFile,"%dK",single_logfile_size/(1024));
                } else if (single_logfile_size < 1024*1024) {
                    single_logfile_size = 1024*1024;
                    sprintf(limited_size_perFile,"%dM",single_logfile_size/(1024*1024));
                } else {
                    sprintf(limited_size_perFile,"%dM",single_logfile_size/(1024*1024));
                }
	
                argv[index++] = "./dlt-logcat";
                argv[index++] = "-o";
                argv[index++] = log_path; //"dlt_log.dlt";
                argv[index++] = "-c";
                argv[index++] = limited_size_perFile; //"100K";
                argv[index++] = "-y";
                argv[index++] = s_serial_path; // "/dev/ttyACM1";

                //if (s_filter_in_jason_filename[0] != '\0') {
                //	argv[index++] = "-j";
                //	argv[index++] = s_filter_in_jason_filename;
                //} else 
                if (s_filter_filename[0] != '\0') {
                    argv[index++] = "-f";
                    argv[index++] = s_filter_filename;
                }

                /*for tftp and ftp in dlt-logcat*/
                if (is_tftp()) {
                    argv[index++] = "-t";
                    argv[index++] = s_tftp_server_ip;
                } else if(is_ftp()) {
                    argv[index++] = "-q";
                    argv[index++] = ftpservipaddr;
                    argv[index++] = "-d";
                    argv[index++] = username;
                    argv[index++] = "-w";
                    argv[index++] = password;
                }

                //for max count
                if (s_max_file_num > 0) {
                    argv[index++] = "-l";
                    sprintf(ap_log_max_count, "%d",s_max_file_num);
                    argv[index++] = ap_log_max_count;
                }

                //debug option
                if (s_dlt_log_in_debug_mode > 0) {
	                argv[index++] = "-v";
                }

                argv[index++] = NULL;

                execvp("./dlt-logcat", argv) ;
                perror("TT exec dlt-logcat failed! ");
                exit(0);
            }else {
                //	./silentlogging -d /dev/ttyUSB2 -l slog2
                char* argv[32];
                char log_path[512] = {0};
                char limited_size_perFile[64] = {0};
                int index = 0;
                char cp_log_level[64] = {0};
                char cp_log_max_count[64] = {0};
                char tmp_buf[64] = {0};

                //snprintf(log_path , sizeof(log_path)-1, "%s%s", s_logpath, CP_SILENT_LOG_NAME);
                snprintf(log_path , sizeof(log_path)-1, "%s",  CP_SILENT_LOG_NAME);

                if (single_logfile_size < 1024*1024) {
                    single_logfile_size = 1024*1024;
                    sprintf(limited_size_perFile,"%d",single_logfile_size/(1024*1024));
                } else {
                    sprintf(limited_size_perFile,"%d",single_logfile_size/(1024*1024));
                }
	
                argv[index++] = "./silentlogging";
                argv[index++] = "-l";      //log path
                argv[index++] = s_logpath; //;
                /*no need to set -o for tftp/ftp case*/
                if (!is_tftp() && !is_ftp()) {
                    argv[index++] = "-o";      //log file name only for silent logging
                    argv[index++] = log_path; //;
                }
                argv[index++] = "-s";
                argv[index++] = limited_size_perFile; //per MB;
                argv[index++] = "-d";
                argv[index++] = s_serial_path; // "/dev/ttyUSB2";

                if (s_filter_filename[0] != '\0') {
                    int p_log_level = atoi(s_filter_filename);
                    argv[index++] = "-p";
                    if ( p_log_level < 0 || p_log_level > 3) {
                        p_log_level = 0;
                    }
                    sprintf(cp_log_level, "%d",p_log_level);
                    argv[index++] = cp_log_level;
                }

                //for max count
                if (s_max_file_num > 0) {
                    argv[index++] = "-c";
                    sprintf(cp_log_max_count, "%d",s_max_file_num);
                    argv[index++] = cp_log_max_count;
                }

                /*for tftp and ftp in silentlogging*/
                if (is_tftp()) {
                    argv[index++] = "-t";
                    argv[index++] = s_tftp_server_ip;
                } else if(is_ftp()) {
                    argv[index++] = "-q";
                    argv[index++] = ftpservipaddr;
                    argv[index++] = "-f";
                    argv[index++] = username;
                    argv[index++] = "-w";
                    argv[index++] = password;
                }

                /*for tcp log config*/
                if( s_cp_tcp_log_config >= 0 ) {
                    argv[index++] = "-g";
                    sprintf(tmp_buf, "%d",s_cp_tcp_log_config);
                    argv[index++] = tmp_buf;
                }

                //the last
                argv[index++] = NULL;

//for test
#if 0
                {
                  printf("for cp log argc=%d\n", index);
                  if (index > 0) {
                      int i = 0;
                      while( i < index) {
                        printf("[%d] argv[%d]=%s\n",i,i,argv[i]);
                        i++;
                      }
                  }
                }
#endif

                execvp("./silentlogging", argv) ;
                perror("TT exec silentlogging failed! ");
                exit(0);

            }
            break;
        default:
            //printf("start child pid for ap log,child_id=%d\n", child_id);
            ap_log_child_pid = child_id;
            //do we need it? yes
            while ( (return_id = wait(&status)) != child_id ) {
                printf("after wait return_id=%d,WEXITSTATUS(status)=%d\n",return_id , WEXITSTATUS(status));
            }
            break;
        }	
        // parent process
        //do we need it?
        //printf("thread_monitor_child_dlt_log_process,WEXITSTATUS(status)=%d\n", WEXITSTATUS(status));
        if ( WEXITSTATUS(status) == 0 ) {
            //printf("thread_monitor_child_dlt_log_process, before quit...\n");
            //exit(0);
        }

        printf("quiting, please wait...(%d,%d,%d)\n",status,WIFEXITED(status),WEXITSTATUS(status));

        //ap log has quitted.
        ap_log_child_pid = -2;
        //printf("thread_monitor_child_dlt_log_process, no quit! \n");
        //printf("thread_monitor_child_dlt_log_process, no log now! Press ctrl+c to stop it!\n");

        // once ap log exe quit unexpectedly
        if ( (0 == WIFEXITED(status))) {
            exit(1);
        }

		return 0;

//ERROR_OUT:
//	exit(1);

}

int ss_new_thread_start_dlt_log_process(void) {
	int result = 0;
    {
        pthread_t thread_id;
        pthread_attr_t attr;
        struct sched_param param;

        //printf("ss_new_thread_start_dlt_log_process\n");
        pthread_attr_init(&attr);
        pthread_attr_getschedparam(&attr, &param);
        param.sched_priority=98;
        pthread_attr_setschedparam(&attr, &param);
        pthread_attr_setschedpolicy(&attr, SCHED_RR);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

        if (pthread_create(&thread_id, &attr, thread_monitor_child_dlt_log_process, NULL))
        {
            printf("pthread_create log process thread create failed, errno:%d(%s)\n", errno, strerror(errno));
            pthread_attr_destroy(&attr);
			result = -1;
            goto ERROR;
        } else {
            //printf("create dlt log thread ok\n");
        }
    }


ERROR:
	return result;

}


size_t slog_tftp_logfile_save(int logfd, const void *buf, size_t size)
{
	int retval = write(logfd, (void *)buf, size);

	return retval;
}

int slog_tftp_logfile_close(int logfd)
{
    return close(logfd);
}

int slog_tftp_logfile_create_fullname(int file_type, const char *fullname, long tftp_size, int is_dump)
{
    int fd = -1;

    if (!strncmp(fullname, "/dev/null", strlen("/dev/null"))) {
        fd = open("/dev/null", O_CREAT | O_RDWR | O_TRUNC, 0444);
    }
    else if (s_tftp_server_ip != NULL)
    {
        const char *filename = fullname;
        const char *p = strchr(filename, '/');
        while (p)
        {
            p++;
            filename = p;
            p = strchr(filename, '/');
        }

        fd = samsung_tftp_write_request(filename, tftp_size);
    }
    else if(is_ftp())
    {
        const char *filename = fullname;
        const char *p = strchr(filename, '/');
        while(p)
        {
            p++;
            filename = p;
            p = strchr(filename, '/');
        }
        LogInfo("%s  filename:%s  s_ftp_server_pass:%s\n",__func__,filename, s_ftp_server_pass);
        fd = s_ftp_write_request(file_type, s_ftp_server_ip, s_ftp_server_usr, s_ftp_server_pass, filename);

    }
    else
    {
        if (is_dump) {
            fd = open(fullname, O_CREAT | O_RDWR | O_TRUNC, 0666);
        }
    }
    return fd;
}

int delete_ap_log_file(const char *filename) {
    int result = 0;

    result = remove(filename);

    if (result != 0) {
        printf("delete_message_file failed(%s): %s,%d\n", filename, strerror(errno),errno);
    } else {
        //printf("delete_message_file ok(%s)\n", filename);
    }

//OUT:
    return result;
}

/*caller need to free result if it is valid*/
char *join_path(const char *path, const char *name) {
    int lenp = strlen(path);
    int lenn = strlen(name);
    int len;
    int needslash = 1;
    char *str;

    len = lenp + lenn + 2;
    if ((lenp > 0) && (path[lenp-1] == '/')) {
        needslash = 0;
        len--;
    }

    str = (char*)malloc(len);
    if (str != NULL) {
        memcpy(str, path, lenp);
        if (needslash) {
            str[lenp] = '/';
            lenp++;
        }
        memcpy(str+lenp, name, lenn+1);
    }
    return str;
}

/* the path:  there are two / at least*/
char * ss_fibo_create_log_folder_path(char*log_dir) {
    int i;
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);

    sprintf(s_logpath, "%s/fibolog_%02d%02d%02d%02d%02d%02d/", log_dir,
            1900+tm->tm_year, tm->tm_mon+1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);

    LogInfo("s_logpath: %s\n", s_logpath);

    for (i=1; i<sizeof(s_logpath) && s_logpath[i] != 0; i++) {
        if (s_logpath[i] == '\\'  || s_logpath[i] == '/' ) {
            char str_dir[FIBO_BUF_SIZE] = {0};
            strcpy(str_dir, s_logpath);
            str_dir[i] = '\0';
            if (access(str_dir, 0)) {
                mkdir(str_dir, 0777);
                LogInfo("mkdir:%s\n",  str_dir);
            }
        }
    }

    return s_logpath;

}


fibo_usbdev_t *samsung_find_pcie_log_devices()
{
    return &samsung_devices_table[PCIE_LOG_DEVICES_AP_NO];
}

int slog_avail_space_for_dump(const char *dir, long need_MB) {
    long free_space = 0;
    struct statfs stat;

    if (!statfs(dir, &stat)) {
        free_space = stat.f_bavail*(stat.f_bsize/512)/2; //KBytes
    }
    else {
        printf("statfs %s, errno : %d (%s)\n", dir, errno, strerror(errno));
    }

    free_space = (free_space/1024);
    if (free_space < need_MB) {
        printf("free space is %ldMBytes, need %ldMB\n", free_space, need_MB);
        return 0;
    }

    return 1;
}
static void dumpusage(char *arg)
{
    printf("========================================\n");
    printf("Usage:\n");
    printf("example: %s\n", arg);
    printf("example(usb): %s  -i tftp:IP\n", arg);
    printf("example(pcie): %s -e tcp -i tftp:IP\n", arg);
    printf("========================================\n");
}

int samsung_dumplog_main(int argc, char **argv)
{
    int i;
    char dump_dir[BUF_SIZE] = "dumplogfiles";
    char tftp_ip[BUF_SIZE] = "";
    bool whole_ramdump=false;
    fibo_usbdev_t *pdev = NULL;

    for (i = 1; i < argc ;)
    {
        if (!strcmp(argv[i], "-s") && (argc - i > 1))
        {
            strcpy(dump_dir, argv[i + 1]);
            i += 2;
        }
        else if(!strcmp(argv[i], "-i") && (argc - i > 1))
        {
            strcpy(tftp_ip, argv[i + 1]);
            if (!strncmp(tftp_ip, TFTP_F, strlen(TFTP_F)))
            {
                s_tftp_server_ip = tftp_ip+strlen(TFTP_F);
                if (samsung_tftp_test_server(s_tftp_server_ip))
                {
                    printf("save dump to tftp server %s\n", s_tftp_server_ip);
                }
                else
                {
                    exit(1);
                }
            }
            i += 2;
        }  else if (!strcmp(argv[i], "-h"))
        {
            dumpusage(argv[0]);
            i++;
            return 0;
        }
        else
        {
            dumpusage(argv[0]);
            return 0;
        }
    }

    pdev = fibo_get_fibocom_device(samsung_find_devices_in_table, "", 0);
    if (pdev == NULL)
    {
        goto END;
    }
    if (fibo_usb_open(pdev, 0, 1)) {
        goto END;
    }

    if (is_tftp())
    {
        samsung_catch_dump(pdev, whole_ramdump, dump_dir, 1);
    }
    else
    {
#ifdef CONFIG_SCOM_DUMP_LOCAL
        if(access(dump_dir, 0))
        {
            mkdir(dump_dir, 0777);
            LogInfo("[%s] mkdir:%s\n", __func__, dump_dir);
        }

        if(!slog_avail_space_for_dump(dump_dir, SDUMP_LOG_NEED_SPACE))
        {
            LogInfo("no enough disk to save dump\n");
            LogInfo("Using tftp mode: ./logtool -i tftp:IP\n");
            return -1;
        }
        else {
            LogInfo("Cache ramdump in local host.\n");
            samsung_catch_dump(pdev, whole_ramdump, dump_dir, 1);
            return 0;
        }
#else
        LogInfo("The local grab function of dump log is not supported\n");
#endif
    }

END:
    fibo_usb_close(pdev, 0);
    return 0;
}

extern int g_samsung_fastboot_type;
int samsung_dumplog_tcp_main(int argc, char **argv)
{
    char dump_dir[BUF_SIZE] = "dumplogfiles";
    char tftp_ip[BUF_SIZE] = "";
    bool whole_ramdump=false;
    fibo_usbdev_t *pdev = NULL;
    char ifname[16] = "mgif_raw";
    char server_ip[16] = "192.168.0.98"; // fixed IP of server
    int server_port = 5554; // fixed port of server
    int opt = -1;

    optind = 1;//must set to 1
    while ((opt = getopt(argc, argv, "e:i:s:h")) != -1) {
        //LogInfo("main line=%d,opt=0x%x, optarg=0x%x\n", __LINE__ , opt, optarg);
        switch (opt) {
            case 'e':
                if (!strstr(optarg, "tcp")) {
                    LogInfo("fastboot '%s' not support!!\n", optarg);
                    dumpusage(argv[0]);
                    return 0;
                }

                break;

            case 'i':
                strcpy(tftp_ip, optarg);
                if (!strncmp(tftp_ip, TFTP_F, strlen(TFTP_F))) {
                    s_tftp_server_ip = tftp_ip+strlen(TFTP_F);
                    if (samsung_tftp_test_server(s_tftp_server_ip)) {
                        LogInfo("save dump to tftp server %s\n", s_tftp_server_ip);
                    } else {
                        exit(1);
                    }
                }
                break;

            case 's':
                strcpy(dump_dir, optarg);
                break;

            case 'h':
            default:
                dumpusage(argv[0]);
                return 0;
        }
    }

    pdev = samsung_find_devices_in_table(0xFFFF, 0xFF00);
    if (pdev == NULL) {
        LogInfo("cannot find devices\n");
        return -1;
    }

    if (samsung_open_tcp(pdev, ifname, server_ip, server_port)) {
        LogInfo("tcp create fail!\n");
        return -1;
    }

    g_samsung_fastboot_type = SS_FASTBOOT_TYPE_NET;
    if (is_tftp()) {
        samsung_catch_dump(pdev, whole_ramdump, dump_dir, 1);
    } else {
#ifdef CONFIG_SCOM_DUMP_LOCAL
        if(access(dump_dir, 0))
        {
            mkdir(dump_dir, 0777);
            LogInfo("[%s] mkdir:%s\n", __func__, dump_dir);
        }

        if(!slog_avail_space_for_dump(dump_dir, SDUMP_LOG_NEED_SPACE))
        {
            LogInfo("no enouth disk to save dump\n");
            LogInfo("Using tftp mode: ./logtool -i tftp:IP\n");
            return -1;
        }
        else {
            LogInfo("Cache ramdump in local host.\n");
            samsung_catch_dump(pdev, whole_ramdump, dump_dir, 1);
            return 0;
        }
#else
        LogInfo("The local grab function of dump log is disabled\n");
        LogInfo("Need to change the Makefile:config_scom_localdump = no ---> config_scom_localdump = yes\n");
#endif
    }

    samsung_close_tcp(pdev);

    return 0;
}

