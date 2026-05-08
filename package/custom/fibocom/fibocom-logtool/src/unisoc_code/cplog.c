#include <time.h>
#include "unisoc_log_main.h"
#include "misc.h"
#include "list.h"

extern ulog_ops_t ulog_ops;
extern int logfile_fd; // Use as cplogfile_fd in unisoc ecxxxu
extern unsigned ulog_exit_requested;
extern unsigned s_logfile_seq;
extern int logfile_maxsize;
extern char log_dir[];
extern int is_tftp();

int print_count = 1000;

void *ulog_logfile_save_thread(void *arg)
{
    ssize_t wc = 0;
    struct Head *head = arg;
    struct Node *p = NULL;
    int savelog_size_total = 0;
    char logel_name[256] = "";
    static int i = 0;
    const char *logfile_suffix = "logel";

    if (NULL == head)
    {
        LogInfo("head is NULL!\n");
        return NULL;
    }

    while (1)
    {
        if (head->count > print_count)
        {
            LogInfo("head->count is %d, ulog_exit_requested is %d\n", head->count, ulog_exit_requested);
            print_count += 100;
        }

        if (head->count > 0)
        {
            // save log to file
            p = head->first;
            wc = ulog_ops.logfile_save(logfile_fd, p->rbuf, p->size, p->logtype);

            /* if log too large, save to a nother file */
            savelog_size_total += p->size;
            if (savelog_size_total > logfile_maxsize)
            {
                LogInfo("create new log\n");
                ulog_ops.logfile_close(logfile_fd);
                savelog_size_total = 0;
                time_t now = time(NULL);
                struct tm *tm_info = localtime(&now);
                sprintf(logel_name, "cp%d_", ++i);
                strftime(logel_name + strlen(logel_name), sizeof(logel_name) - strlen(logel_name), "%Y%m%d_%H%M%S.logel", tm_info);
                if (is_tftp())
                {
                    logfile_fd = ulog_logfile_create_fullname(0, logel_name, 0, 1);
                }
                else
                {
                    logfile_fd = ulog_ops.logfile_create(log_dir, logfile_suffix, s_logfile_seq);
                }

                if (ulog_ops.logfile_init)
                {
                    ulog_ops.logfile_init(logfile_fd, s_logfile_seq);
                    s_logfile_seq++;
                }
            }
            /* delete list note */
            deleteNode(head);
        }
        else
        {
            if (1 == ulog_exit_requested)
            {
                /* close file */
                ulog_ops.logfile_close(logfile_fd);
                break;
            }
            usleep(5);
        }
    }

    return NULL;
}
