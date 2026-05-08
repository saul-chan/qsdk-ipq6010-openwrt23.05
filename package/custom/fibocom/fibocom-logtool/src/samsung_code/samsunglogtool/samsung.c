#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdint.h>
#include <termios.h>
#include <dirent.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <unistd.h>
#include <getopt.h>
#include <ctype.h>
#include <malloc.h>
#include <stdbool.h>
#include <pthread.h>
#include <inttypes.h>
#include <sys/socket.h>


#include "samsung_protocol.h"
#include "slog.h"


#include "samsung_log_main.h"


#include <sys/types.h>
#include <sys/wait.h>


int g_samsung_fastboot_type = SS_FASTBOOT_TYPE_USB;

#define smr_exec(_c) do{if (_c) {LogInfo("failed: %s (%s: %s: %d)\n", #_c, __FILE__, __func__, __LINE__); return -1;}} while(0)
/* modify macro MIN
* usually we difine it as: (a) < (b) ? (a) : (b)
* but it will cause some problems, here is a case:
* MIN(i++, j++), when calling the macro above, i++ will be run two times, which is wrong.
* so we can modify it as following.
*  (void)(&_a == &_b); is use to check wether the type of 'a' and 'b' is same or not.
* (void) is used to eliminated warnning.
*/
#define SAMSUNG_MIN(a, b) ({ \
            typeof(a) _a = a; \
            typeof(b) _b = b; \
            (void)(&_a == &_b); \
            _a < _b ? _a : _b; \
        })


samsung_data_t samsung_data = {
    NULL,              // rx_buffer
    NULL,              // tx_buffer
    NULL,              // misc_buffer
    0,                 // timed_data_size
    -1,                // fd
    -1,                // ram_dump_image
    5,                 // max_ram_dump_retries
    SAMSUNG_RAW_BUFFER_SIZE,           // max_ram_dump_read
    0,                 // command
    false              // ram_dump_64bit
};

typedef struct {
    const char *path_to_save_files;
    int verbose;
    int do_reset;
} kickstart_options_t;

static kickstart_options_t kickstart_options = {
    NULL,   // path_to_save_files
    1,     // verbose
    1,
};

enum LOG_LEVEL {
LOG_DEBUG = 1,
LOG_EVENT,
LOG_INFO,
LOG_STATUS,
LOG_WARN,
LOG_ERROR
};

#define dbg( log_level, fmt, arg... ) do {if (kickstart_options.verbose || LOG_ERROR == log_level) { unsigned msec = slog_msecs();  printf("[%03d.%03d] " fmt "\n",  msec/1000, msec%1000, ## arg);}} while (0)
#define MAX_RESPONSE_SIZE 256
dload_type exynos_memory_table[32];

static void delay(unsigned int timeout) {
    struct timespec timev;
    struct timespec timer;
    int rv;

    timer.tv_sec = timeout / 1000UL;
    timer.tv_nsec = (timeout * 1000000UL) % 1000000000UL;
    
    do
    {
        timev = timer;        /* remaining time */
        rv = nanosleep(&timev, &timer);
        
    } while (rv < 0 && errno == EINTR);
}

static int timeval_subtract (struct timeval *result, struct timeval *x, struct timeval *y) {
    // Perform the carry for the later subtraction by updating y.
    if (x->tv_usec < y->tv_usec) {
        int nsec = (y->tv_usec - x->tv_usec) / 1000000 + 1;
        y->tv_usec -= 1000000 * nsec;
        y->tv_sec += nsec;
    }
    if (x->tv_usec - y->tv_usec > 1000000) {
        int nsec = (x->tv_usec - y->tv_usec) / 1000000;
        y->tv_usec += 1000000 * nsec;
        y->tv_sec -= nsec;
    }

    // Compute the time remaining to wait. tv_usec is certainly positive. */
    result->tv_sec = x->tv_sec - y->tv_sec;
    result->tv_usec = x->tv_usec - y->tv_usec;

    // Return 1 if result is negative.
    return x->tv_sec < y->tv_sec;
}

static void time_throughput_calculate(struct timeval *start_time, struct timeval *end_time, size_t size_bytes)
{
    struct timeval result;
	double TP = 0.0;

    if (size_bytes == 0) {
        dbg(LOG_INFO, "Cannot calculate throughput, size is 0");
        return;
    }
    timeval_subtract(&result, end_time, start_time);

	TP  = (double)result.tv_usec/1000000.0;
	TP += (double)result.tv_sec;

	if(TP>0.0)
	{
		TP = (double)((double)size_bytes/TP)/(1024.0*1024.0);
		dbg(LOG_STATUS, "%zd bytes transferred in %ld.%06ld seconds (%.4fMBps)", size_bytes, result.tv_sec, result.tv_usec,TP);
	}
	else
		dbg(LOG_STATUS, "%zd bytes transferred in %ld.%06ld seconds", size_bytes, result.tv_sec, result.tv_usec);
}

static int dl_send_cmd(fibo_usbdev_t *pdev, char *pdl_cmd)
{
    int ret;

    ret = pdev->write(pdev,pdl_cmd, strlen(pdl_cmd), 0);
    return ret;
}

static int dl_read_data(fibo_usbdev_t *pdev, char *buffer, int size)
{
    int ret;

    ret = pdev->read(pdev,buffer, size, 0);
    return ret;
}

int dl_send_cmd_wait_ack(fibo_usbdev_t *pdev, char *dl_req,  unsigned timeout)
{
    int ret;

    ret = dl_send_cmd(pdev, dl_req);
    if (ret <= 0) return -1;
    memset(samsung_data.rx_buffer,0,MAX_RESPONSE_SIZE);
    ret = dl_read_data(pdev, samsung_data.rx_buffer,MAX_RESPONSE_SIZE);
    if (ret == 0) return -1;

    LogInfo("%s -->> %s\n", dl_req, g_samsung_fastboot_type == SS_FASTBOOT_TYPE_NET? 
                (char *)samsung_data.rx_buffer + 8 : (char *)samsung_data.rx_buffer);
    return 0;
}

#define DELIMITER ","
#define MAX_LINE_LENGTH 1024
int parse_txt(void) {
    char *ramdump_list = "ramdump.list";
    FILE *file = fopen(ramdump_list, "r");
    char line[MAX_LINE_LENGTH];
    int i = 0;
    int j = 0;

    memset(exynos_memory_table, 0, sizeof(exynos_memory_table));
    if (file == NULL) {
        //LogInfo("Error opening file %s\n",ramdump_list);
        exynos_memory_table[0].mem_base = 0x40000000;
        exynos_memory_table[0].length = 0x40000000;
        strncpy(exynos_memory_table[0].filename, 
	        "ap_0x40000000--0x7FFFFFFF.lst",
		sizeof(exynos_memory_table[0].filename));
        return -1;
    }
    while (fgets(line, sizeof(line), file) != NULL) {
        char *token = strtok(line, DELIMITER);
        j = 0;
        while (token != NULL) {
	        if (j == 0)
			exynos_memory_table[i].mem_base = strtol(token,NULL,16);
	        if (j == 1)
			exynos_memory_table[i].length = strtol(token,NULL,16);
	        if (j == 2)
			strncpy(exynos_memory_table[i].filename, token,sizeof(exynos_memory_table[i].filename));
	        token = strtok(NULL, DELIMITER);
	        j++;
        }
	i++;
	if (i >=  sizeof(exynos_memory_table)/sizeof(exynos_memory_table[0])) {
		dbg(LOG_INFO, "dump item more than exynos_memory_table size\n");
		break;
	}
    }
    fclose(file);
    return 0;
}

static bool samsung_start(fibo_usbdev_t *pdev, bool whole_ramdump) {
	int              retval = 0;
	int              i = 0;
	char strtmp[128] = { 0, };
	struct timeval time_start, time_end;

	kickstart_options.verbose = 1;
	parse_txt();
	dbg(LOG_INFO, ">>>>>>>>>>>>> SAMSUNG RAMDUMP START >>>>>>>>>>>>>\n");
	for(i = 0; i < sizeof(exynos_memory_table)/sizeof(exynos_memory_table[0]); i++) {
		int fd = -1;
		char full_filename[256] = {0};
		int  cur = 0;

		 if (!exynos_memory_table[i].mem_base)
	            break;

        if (kickstart_options.path_to_save_files) {
            strcpy(full_filename, kickstart_options.path_to_save_files);
            strcat(full_filename, "/");
        }

		strcat(full_filename, exynos_memory_table[i].filename);
		fd = slog_tftp_logfile_create_fullname(0, full_filename, exynos_memory_table[i].length, 1);
		if (fd==-1)  {
			dbg(LOG_ERROR, "ERROR: Your file '%s' does not exist or cannot be created\n\n",exynos_memory_table[i].filename);
			exit(0);
		}

		gettimeofday(&time_start, NULL);
        if (g_samsung_fastboot_type == SS_FASTBOOT_TYPE_NET) {
            //[data_size][data]
            sprintf(strtmp, "oem ramdump 0x%x 0x%x",  exynos_memory_table[i].mem_base, exynos_memory_table[i].length); 
            sprintf(samsung_data.misc_buffer, "%08ld%s", strlen(strtmp), strtmp);
        } else {
            sprintf(samsung_data.misc_buffer, "oem ramdump 0x%x 0x%x",  exynos_memory_table[i].mem_base, exynos_memory_table[i].length);
        }		
		smr_exec(dl_send_cmd_wait_ack(pdev, samsung_data.misc_buffer,  1000));

        if (g_samsung_fastboot_type == SS_FASTBOOT_TYPE_NET) {
            sprintf(strtmp, "%s", "upload");
            sprintf(samsung_data.misc_buffer, "%08ld%s", strlen(strtmp), strtmp);
        } else {
		    sprintf(samsung_data.misc_buffer, "upload");
        }
		smr_exec(dl_send_cmd_wait_ack(pdev, samsung_data.misc_buffer,  1000));

        dbg(LOG_INFO, "STATE ----> Start Recv %s Size 0x%x\n", full_filename, exynos_memory_table[i].length);
        kickstart_options.verbose = 0;
		while (cur < exynos_memory_table[i].length) {
			int len = SAMSUNG_MIN((uint32_t)(exynos_memory_table[i].length - cur), samsung_data.max_ram_dump_read);
			retval = dl_read_data(pdev,samsung_data.rx_buffer, len);
			cur += retval;
			retval = slog_tftp_logfile_save(fd, samsung_data.rx_buffer, retval);
			if (retval < 0) {
				dbg(LOG_ERROR, "file write failed: %s", strerror(errno));
				slog_tftp_logfile_close(fd);
				return false;
			}

			if ((uint32_t) retval != len) {
				dbg(LOG_WARN, "Wrote only %d of 0x%08x bytes", retval, len);
			}
			dbg(LOG_WARN, "Wrote  %d of 0x%08x bytes", retval, len);
		}

		kickstart_options.verbose = 1;
        dbg(LOG_INFO, "STATE <---- End Recv %s Recv_Size 0x%x\n", full_filename, cur);
		gettimeofday(&time_end, NULL);
		time_throughput_calculate(&time_start, &time_end, exynos_memory_table[i].length);
		delay(4000);//wait for data finish transfer
		slog_tftp_logfile_close(fd);
	}

	dbg(LOG_INFO, "<<<<<<<<<<<<< SAMSUNG RAMDUMP END <<<<<<<<<<<<<\n");
	return true;
}

 int samsung_catch_dump(fibo_usbdev_t *pdev, bool whole_ramdump, const char *path_to_save_files, int do_reset) {
    int retval;

    //samsung_data.mode = SAMSUNG_MODE_MEMORY_DEBUG;
    kickstart_options.path_to_save_files = path_to_save_files;
    kickstart_options.do_reset = do_reset;

    samsung_data.rx_buffer = malloc (SAMSUNG_RAW_BUFFER_SIZE);
    samsung_data.tx_buffer = malloc (2048);
    samsung_data.misc_buffer = malloc (SAMSUNG_RAW_BUFFER_SIZE); 

    if (NULL == samsung_data.rx_buffer || NULL == samsung_data.tx_buffer || NULL == samsung_data.misc_buffer) {
        dbg(LOG_ERROR, "Failed to allocate samsung buffers");
        return false;
    }

    retval = samsung_start(pdev, whole_ramdump);
    if (false == retval) {
        dbg(LOG_ERROR, "Samsung protocol error");
    }
    else {
        dbg(LOG_ERROR, "Samsung protocol completed");
    }

    free(samsung_data.rx_buffer);
    free(samsung_data.tx_buffer);
    free(samsung_data.misc_buffer);

    samsung_data.rx_buffer = samsung_data.tx_buffer = samsung_data.misc_buffer = NULL;

    if (retval == false)
        dbg(LOG_INFO, "Catch DUMP using Samsung protocol failed\n\n");
    else
        dbg(LOG_INFO, "Catch DUMP using Samsung protocol successful\n\n");

    return retval;
}
