#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#include <time.h>
#include <signal.h>
#include <assert.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <poll.h>
#include <netinet/in.h>
#include <pthread.h>
#include <dirent.h>
#include <sys/ioctl.h>
#include <linux/version.h>
#if LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 20)
#include <linux/usb/ch9.h>
#else
#include <linux/usb_ch9.h>
#endif
#include <linux/usbdevice_fs.h>

//typedef unsigned int uint32_t;
//#define TFTP_F "tftp:"
//#define FTP_F  "ftp:"

enum FASTBOOT_TYPE {
  SS_FASTBOOT_TYPE_USB = 0,
  SS_FASTBOOT_TYPE_NET
};

extern unsigned slog_msecs(void);


extern int slog_tftp_logfile_create_fullname(int file_type, const char *fullname, long tftp_size, int is_dump);
extern int slog_tftp_logfile_close(int logfd);
extern size_t slog_tftp_logfile_save(int logfd, const void *buf, size_t size);

extern int samsung_tftp_write_request(const char *filename, long tsize);
extern int samsung_tftp_test_server(const char *serv_ip);
extern int s_ftp_write_request(int index, const char *ftp_server, const char *user, const char *pass, const char *filename);;

extern int samsung_catch_dump(fibo_usbdev_t *pdev, bool whole_ramdump, const char *path_to_save_files, int do_reset);

extern int samsung_open_tcp(fibo_usbdev_t *pdev, char *ifname, char *server_ip, int server_port);
extern int samsung_close_tcp(fibo_usbdev_t *pdev);
extern int samsung_tcp_write(const void *handle, void *pbuf, int size, int portnum);
extern int samsung_tcp_read(const void *handle, void *pbuf, int size, int portnum);

int ss_new_thread_start_dlt_log_process(void);
int slog_avail_space_for_dump(const char *dir, long need_MB);


