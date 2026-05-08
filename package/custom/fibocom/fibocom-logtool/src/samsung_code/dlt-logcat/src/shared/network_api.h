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

typedef unsigned int uint32_t;
#define TFTP_F "tftp:"
#define FTP_F  "ftp:"
#define SLOG_BUF_SIZE (4*1024)



extern size_t kfifo_write(int idx, const void *buf, size_t size);
extern void kfifo_free(int idx);
extern int kfifo_alloc(int fd);
extern int kfifo_idx(int fd);


extern int slog_logfile_close(int logfd);
extern size_t slog_logfile_save(int logfd, const void *buf, size_t size);
extern int slog_logfile_create_fullname(int file_type, const char *fullname, long tftp_size, int is_dump);
extern int tftp_write_request(const char *filename, long tsize);
extern int tftp_test_server(const char *serv_ip);
extern int s_ftp_write_request(int index, const char *ftp_server, const char *user, const char *pass, const char *filename);
extern int is_tftp(void);
extern int is_ftp(void);
extern char * target_folder_get_parse_for_ftp(char * target_filename);
extern int tftp_ftp_set_transfer_status(int status);
extern int is_transfer_ongoing(void);
extern void s_ftp_quit(void);


