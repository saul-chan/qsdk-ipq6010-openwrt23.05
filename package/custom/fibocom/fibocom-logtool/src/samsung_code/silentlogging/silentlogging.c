#include "silentlogging.h"
#include "DMFileManager.h"
#include "network_api.h"

void TouchSdmHeader(bool remove) {
    LOG_PRINT_D("-----------TouchSdmHeader-------------\n");
    char buf[256] = { 0, };
    //property_get(PROP_SLOG_PATH, buf, DEF_SLOG_PATH);
    char mLogPath[PATH_MAX] = {};
    strncpy(mLogPath, gSlogPath, PATH_MAX);
    char* suffix = ".sbuff_header.sdm";
    strcat(mLogPath, suffix);

    if (remove) {
        if (unlink(mLogPath) < 0) {
            LOG_PRINT_W("Failed to remove header.\n");
        }
    }
    else {
        size_t size = sizeof(DM_HEADER) / sizeof(DM_HEADER[0]);
        FILE *out = fopen(mLogPath, "w");
        if (out != NULL) {
            size_t ret = fwrite(DM_HEADER, 1, size, out);
            fclose(out);
            if (ret != size) {
                LOG_PRINT_W("header may not be valid. Remove.\n");
                if (unlink(mLogPath) < 0) {
                    LOG_PRINT_W("Failed to remove header.\n");
                }
            }
        }
    }
}

int createHeaderFile() {
    char strDmLogFile[256];
    snprintf(strDmLogFile, sizeof(strDmLogFile), "%s/.%s_header.%s", gSlogPath, "sbuff", "sdm");
    //snprintf(strDmLogFile, sizeof(strDmLogFile), "%s.sbuff_header.sdm", gSlogPath);
    
    char *timeZoneId = getenv("TZ");
    LOG_PRINT_D("getenv timeZoneId = %s \n", timeZoneId);
    if (timeZoneId == NULL) {
        timeZoneId = (char *)malloc(PATH_MAX);
        if (timeZoneId != NULL) {
            strncpy(timeZoneId, "Asia/Shanghai", PATH_MAX - 1);
            timeZoneId[PATH_MAX - 1] = '\0';
        } else {
            perror("malloc error. \n");
            return 1;
        }
    }
    LOG_PRINT_D("timeZoneId = %s \n", timeZoneId);
    size_t timeZbyteLen = strlen(timeZoneId);
    unsigned char *timeZbyte = (unsigned char *)timeZoneId;

    unsigned char bTimeStmp[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    unsigned char tmpSdmHead[] = {
        0x00, 0x00, 0x39, 0xFD, 0x04, 0x00, 0x02, 0x00, 0x00, 0x00, 0x1E, 0x00,
        0x53, 0x69, 0x6c, 0x65, 0x6e, 0x74, 0x20, 0x4c, 0x6f, 0x67, 0x20, 0x56,
        0x65, 0x72, 0x2e, 0x20, 0x32, 0x20, 0x53, 0x65, 0x70, 0x2e, 0x20, 0x30,
        0x36, 0x2c, 0x32, 0x30, 0x31, 0x32, 0x08, 0x00,
        bTimeStmp[0], bTimeStmp[1], bTimeStmp[2], bTimeStmp[3], bTimeStmp[4], bTimeStmp[5],
        0x00, 0x00, (unsigned char)timeZbyteLen, 0x00
    };

    sdmLogVersionInfoSize = sizeof(tmpSdmHead) + timeZbyteLen;
    SdmLogVersionInfo = (unsigned char *)malloc(sdmLogVersionInfoSize);
    if (SdmLogVersionInfo == NULL) {
        LOG_PRINT_E("IOException: Memory allocation failed");
        return 0;
    }

    memcpy(SdmLogVersionInfo, tmpSdmHead, sizeof(tmpSdmHead));
    memcpy(SdmLogVersionInfo + sizeof(tmpSdmHead), timeZbyte, timeZbyteLen);
    SdmLogVersionInfo[0] = (unsigned char)(sdmLogVersionInfoSize - 2);

    if (!is_tftp() && !is_ftp())
    {
        LOG_PRINT_I("not tftp/ftp mode createHeaderFile into path ----------\n");
        if (access(strDmLogFile, F_OK) != -1) {
            remove(strDmLogFile);
            LOG_PRINT_D("header file exists, deleting Headerfile");
        }

        FILE *fileOut = fopen(strDmLogFile, "wb");
        if (fileOut == NULL) {
            LOG_PRINT_E("IOException: Unable to open file");
            return 0;
        }

        if (fwrite(SdmLogVersionInfo, 1, sdmLogVersionInfoSize, fileOut) != sdmLogVersionInfoSize) {
            LOG_PRINT_E("IOException: Write error");
            free(SdmLogVersionInfo);
            fclose(fileOut);
            return 0;
        }
        fclose(fileOut);
    }

    LOG_PRINT_D("Header Operation finished");
    //free(SdmLogVersionInfo);
    return 1;
}

void printCharArray(const unsigned char* array, size_t size) {
    size_t i;
    for (i = 0; i < size; i++) {
        LOG_PRINT_D("0x%02x ", array[i]);
    }
    LOG_PRINT_D("\n");
}

void SendProfileDirect() {
    LOG_PRINT_D("Send Profile data to CP \n");

    LOG_PRINT_I("User select profile content: level = %d \n", logLevel);
    printCharArray(DM_STARTS[logLevel], DM_START_LENGTH[logLevel]);
    LOG_PRINT_I("\n");
    dmd_modem_write(g_modem_fd, (char *)DM_STARTS[logLevel], DM_START_LENGTH[logLevel]); //write to modem
    if (enableTCP) {
        dmd_modem_write(g_modem_fd, (char *)START_TCP_LOGS, sizeof(START_TCP_LOGS)); //write TCP enable to modem
    }
    dmd_modem_write(g_modem_fd, (char *)DEFAULT_PROFILE, sizeof(DEFAULT_PROFILE)); //write to modem
    dmd_modem_write(g_modem_fd, (char *)DEFAULT_PROFILE1, sizeof(DEFAULT_PROFILE1)); //write to modem
    DM_start = true;
}

int32_t dmd_modem_write(int32_t fd, char* buffer, int32_t buf_len)
{
    LOG_PRINT_I("dmd_modem_write buffer: \n");
    //printCharArray(buffer, DM_START_0);
    int32_t t = 0, w;
    uint32_t to_send, msg_len;
    uint32_t msg_lennwf;
    char *bufnfm = buffer;
    int32_t wholeSize = buf_len;

    while(buf_len > 0) {
        if (buffer[0] != 0x7F) {
            break;
        }

        msg_len = (buffer[1] | (buffer[2] << 8)) & 0xFFFF;

        if ((int32_t)msg_len + 1 >= buf_len) {
            break;
        }

        if (buffer[msg_len + 1] != 0x7E) {
            break;
        }

        to_send = msg_len + 2;

        { // socket monitor call this func, when silent mode.
            while(1)
            {
                if(silentbufthrdlock == true)    // Is bufferthread Locked?
                {
                    //ALOGD("Buffer Thread Locked  = %d", silentbufthrdlock);
                    usleep(100000);
                    continue;
                }
                else
                {
                    //ALOGD("Buffer Thread Un-Locked ");
                    silentbufthrdlock = true;    // Make Thread Locked
                    break;
                }
            }

            msg_lennwf = (buffer[4] | (buffer[5] << 8)) & 0xFFFF;
            if (msg_lennwf < 2) {
                break;
            }
            msg_lennwf =msg_lennwf -2;
            memset(&hdr[0],0,2);
            *dmlen=(msg_lennwf +8);    //new format
            bufnfm=buffer+6;

            CalculateTimeStamp();

            if(buff2use==1)
            {
               memcpy(silent_buf1+rcvoffset, hdr, sizeof(hdr));
               rcvoffset+=sizeof(hdr);

               memcpy(silent_buf1+rcvoffset, bufnfm, msg_lennwf); // write byte to buffer
               rcvoffset=rcvoffset+msg_lennwf;        // as 7E skipped
            }

            else if (buff2use==2)
            {
               memcpy(silent_buf2+rcvoffset, hdr, sizeof(hdr));
               rcvoffset+=sizeof(hdr);

               memcpy(silent_buf2+rcvoffset, bufnfm,msg_lennwf); // write byte to buffer
               rcvoffset=rcvoffset+msg_lennwf;        // as 7E skipped
            }

            silentbufthrdlock = false;    // Make Buffer Thread Un-Locked

        }
        LOG_PRINT_D("write %u byte(s)... \n", to_send);
        printCharArray(buffer, to_send);
        /*willa.liu@20241024 JIRA:MBB0678-423 begin*/
        int retries = 0;
        while (retries < MAX_RETRIES) {
            w = write(fd, buffer, to_send);
            if (w < 0) {
                LOG_PRINT_D("fd %d, Write Error= %d, errno=%d \n", fd, w, errno);
                if (errno == EINTR || errno == EAGAIN) {
                    retries++;
                    //usleep(10);
                    continue;
                } else {
                    LOG_PRINT_E("fd %d, Write Error not EINTR or EAGAIN! \n", fd);
                    return -1;
                }
            }
            break;
        }
        if (retries == MAX_RETRIES) {
            LOG_PRINT_D("fd %d, All %d attempts to write failed.\n", fd, MAX_RETRIES);
        /*willa.liu@20241024 JIRA:MBB0678-423 end*/
        }
        //LOG_PRINT("after write .. wrote to modem = %d \n", w);
        if( w > (int32_t)to_send)
            w = to_send;

        if (!w)
            return 0;
        buf_len -= w;
        buffer = (char *)(buffer + w);
        t += w;
        //LOG_PRINT("Written %d/%d \n", t, wholeSize);
    }
    return t;
}

bool modem_init(const char *deviceID)
{
    //int n = 10;
    LOG_PRINT_E("modem_init+++ \n");
    while(1) {
        g_modem_fd = open(deviceID, O_RDWR | O_NONBLOCK | O_NOCTTY);
        if (g_modem_fd < 0) {
            LOG_PRINT_E("modem_init:Fail to open %s \n", deviceID);
            sleep(1);
            DM_start_retry = false;
            continue;
        } else {
            DM_start_retry = true;
            DM_start_retry_need = true;
            if (is_tftp() || is_ftp())
            {
                if (need_create_tftp_file_again)
                {
                    LOG_PRINT_I("modem_init:need_create_tftp_file_again %s \n", deviceID);
                    gZipOperation = 1;
                    filesizeoffset = 0;
                    need_create_tftp_file_again = false;
                }
            }
            break;
        }
    }
    // LOG_PRINT("%s, g_modem_fd=%d \n", deviceID, g_modem_fd);
    // g_modem_fd = open(deviceID, O_RDWR | O_NONBLOCK | O_NOCTTY);
    // if (g_modem_fd < 0) {
    //     LOG_PRINT_E("modem_init:Fail to open %s \n", deviceID);
    //     return 0;
    // }

    /*willa.liu@20241108 JIRA:MBB0678-501 begin*/
    struct termios usb_termios;
    memset((char*)&usb_termios, 0, sizeof(struct termios));

    if (tcgetattr(g_modem_fd, &usb_termios) < 0) {
        return false;
    }

    cfmakeraw(&usb_termios);
    usb_termios.c_iflag &= ~ICRNL;
    usb_termios.c_iflag &= ~INLCR;
    usb_termios.c_oflag &= ~OCRNL;
    usb_termios.c_oflag &= ~ONLCR;
    usb_termios.c_lflag &= ~ICANON;
    usb_termios.c_lflag &= ~ECHO;

    if (tcsetattr(g_modem_fd, TCSANOW, &usb_termios) <0) {
        return false;
    }
    /*willa.liu@20241108 JIRA:MBB0678-501 end*/
    LOG_PRINT_I("%s, g_modem_fd=%d \n", deviceID, g_modem_fd);
    return (g_modem_fd > 0);
}

bool init_monitor(void)
{
    if (!modem_init(deviceID)) {
        return false;
    }
    return true;
}

void modem_close(void)
{
    LOG_PRINT_W("try modem_close. \n");
    if (g_modem_fd != -1) {
        close(g_modem_fd);
        g_modem_fd = -1;
    }
}

void exit_handle(int sig)
 {
    LOG_PRINT_W("Exit SIG(%d) received \n", sig);
    if (DM_start)
    {
        dmd_modem_write(g_modem_fd, (char *)DM_STOP, sizeof(DM_STOP)); //write to modem
        DM_start = false;
    }
    if (enableTCP)
    {
        dmd_modem_write(g_modem_fd, (char *)STOP_TCP_LOGS, sizeof(STOP_TCP_LOGS)); //write disable tcp to modem
    }
    g_quit_flag = 1;
    free(SdmLogVersionInfo);
    pthread_cancel(thread_id_modem);

    /*give it chance to tftp thread for last ack.*/
    if (is_tftp() || is_ftp()) {
        LOG_PRINT_I("exit_handle close server file. slog_logfile_close ftp_fd  = %d \n", ftp_fd);
        slog_logfile_close(ftp_fd);
        ftp_fd = -1;
        silentlogging_sync_network_stop();
    }

    pthread_mutex_destroy(&mutex);
    exit(0);
 }

 void exitSignalHandle()
 {
    signal(SIGINT, exit_handle);
    signal(SIGTSTP, exit_handle);
    signal(SIGTERM, exit_handle);
 }

 void log_hexdump(const char *format, int lSize, ...)
{
    char str[80], octet[10];
    int ofs, i, l;

    if (lSize < 0) {
        //ALOGD("+log_hexdump() invalid size\n");
        return ;
    }

    const int MAX_DUMP_SIZE = (1024 * 2);
    if (lSize >= MAX_DUMP_SIZE) {
        //ALOGD("+log_hexdump() dump data over %d KB\n", (MAX_DUMP_SIZE / 1024));
        lSize = MAX_DUMP_SIZE;
    }

    //ALOGD("+log_hexdump()\n");

    for (ofs = 0; ofs < lSize; ofs += 16) {
        sprintf( str, "%03d: ", ofs );

        for (i = 0; i < 16; i++) {
            if ((i + ofs) < lSize)
                sprintf( octet, "%02x ", format[ofs + i] );
            else
                strcpy( octet, "   " );

                strcat( str, octet );
            }
            strcat( str, "  " );
            l = strlen( str );

            for (i = 0; (i < 16) && ((i + ofs) < lSize); i++)
                str[l++] = isprint( format[ofs + i] ) ? format[ofs + i] : '.';

            str[l] = '\0';
            //ALOGD("%s\n", str);
    }

    //ALOGD("-log_hexdump()\n");

}

 void DoSendProfile(int32_t fd)
{
    FILE *sprofile_file = NULL;
    char filestr[100];
    int read_num = 0;
    struct stat ProfileFileSt;
    static char *read_buf = NULL;

    if (read_buf == NULL)
    {
        read_buf = (char *)malloc(SOCKET_MAX_BUF);
        memset(read_buf, 0, SOCKET_MAX_BUF);

        if (read_buf == NULL)
        {
            //ALOGE("%s : memory allocation fail", __FUNCTION__);
            LOG_PRINT_E("DoSendProfile : memory allocation fail");
            return;
        }
    }

    sprintf(filestr, "%ssbuff_profile.sdm", gSlogPath);
    //ALOGD("%s----------File String is  : %s  ----------\n", __func__, filestr);

    sprofile_file = fopen(filestr, "rb");

    if (NULL != sprofile_file)
    {
        if (stat(filestr, &ProfileFileSt) != 0)
        {
            //ALOGE("%s : stat error(%s)", __FUNCTION__, strerror(errno));
            LOG_PRINT_E("stat error\n");
        }
        if (0 < (read_num = fread(read_buf, sizeof(read_buf[0]), ProfileFileSt.st_size, sprofile_file)))
        {
            log_hexdump((const char *)read_buf, read_num);
            dmd_modem_write(fd, read_buf, read_num); //write to modem
        }
        else
            //ALOGE("%s : fread error(%s)", __FUNCTION__, strerror(errno));
            LOG_PRINT_E("fread error\n");

        fclose(sprofile_file);
        sprofile_file = NULL;
    }
    else
    {
        //ALOGD("----------Opening sprofile_file Fail-----\n");
        LOG_PRINT_E("----------Opening sprofile_file Fail-----\n");
    }
}

 void *modem_status_monitor(void *arg)
{
    int32_t fd;
    struct pollfd pollfd;
    int ret = 0;
    int state = -1;

    fd = open(PATH_BOOT, O_RDWR);
    pollfd.fd = fd;
    pollfd.events = POLLHUP | POLLIN | POLLRDNORM;
    pollfd.revents = 0;

    //ALOGD("%s : start!!!", __FUNCTION__);
    LOG_PRINT_D("modem_status_monitor : start!!!");
    while(1)
    {
        pollfd.revents = 0;
        ret = poll(&pollfd, 1, -1);

        if((pollfd.revents & POLLHUP) || (pollfd.revents & POLLIN) || (pollfd.revents & POLLRDNORM))
        {
            //ALOGD("%s : receive poll event!!!", __FUNCTION__);
            const unsigned int interval = 2;
            bool restartDm = false;
            while (true) {
                state = ioctl(fd, IOCTL_MODEM_STATUS);
                //ALOGD("%s : modem status [%d]", __FUNCTION__, state);
                LOG_PRINT_D("modem_status_monitor : modem status [%d]\n", __FUNCTION__, state);
                if (state == STATE_OFFLINE || state == STATE_BOOTING) {
                    restartDm = true;
                    int spin = 300;
                    while (spin--) {
                        state = ioctl(fd, IOCTL_MODEM_STATUS);
                        if (state == STATE_ONLINE) {
                            break;
                        }
                        usleep(100 * 1000);
                    }

                    if (spin < 0) {
                        //ALOGE("%s : Modem boot timeout", __FUNCTION__);
                        restartDm = false;
                    }
                }
                else if (state == STATE_CRASH_EXIT || state == STATE_CRASH_RESET) {
                    //ALOGD("%s : Modem  is STATE_CRASH_EXIT or STATE_CRASH_RESET", __FUNCTION__);
                    //ALOGD("%s : Check status after %d seconds again.", __FUNCTION__, interval);
                    restartDm = true;
                    sleep(interval);
                    continue;
                }

                if (state == STATE_ONLINE) {
                    //ALOGD("%s : Modem is ONLINE", __FUNCTION__);
                    LOG_PRINT_D("modem_status_monitor : Modem is ONLINE\n", __FUNCTION__, state);
                    if (restartDm) {
                        DoSendProfile(g_modem_fd);
                    }
                }
                break;
            }
        }
        else {
            //ALOGE("%s : unknown poll event!", __FUNCTION__);
            LOG_PRINT_E("modem_status_monitor : unknown poll event!");
            usleep(200000);
        }
    }
    close(fd);
    return NULL;
}

// Read property for DMD log dump
int EnableLogDump()
{
    int printlog = 0;
    char logstr[100];
    //property_get(PROPERTY_DMD_ENABLE_LOG, logstr , "0");      // 0 to disable
    printlog = atoi(logstr);
    return printlog;
}

void CalculateTimeStamp(void)
{
    //Start Enter Time Based on Packet Time Stamp
    //ALOGD("%s++", __FUNCTION__);

    time(&tmSec);
    gettimeofday(&stmMilli, NULL);
    ptmLocal = localtime(&tmSec);
    tmSec = mktime(ptmLocal);

    llTimestamp = tmSec;
    llTimestamp *= 1000;
    llTimestamp += (stmMilli.tv_usec / 1000);

    //unsigned char acBuffer[16];
    memset(&hdr[4], 0, 6);
    memcpy(&hdr[4], &llTimestamp, 6);

    //END Enter Time Based on Packet Time Stamp
    //ALOGD("%s--", __FUNCTION__);
}

void CheckVersionInfo(char *buff, int32_t buff_len)
{
    if(buff != NULL && buff_len > 0)
    {
        if((buff[2]==0xA0 || buff[2]==0xA1) && buff[3]==0x00 && buff[4]==0x01)
        {
            //ALOGD( "%s : Find version information packet.", __FUNCTION__);
            if (gVersionInfo.hdr) {
                //delete[] gVersionInfo.hdr;
                free(gVersionInfo.hdr);
                gVersionInfo.hdr = NULL;
                gVersionInfo.hdrSize = 0;
            }

            //gVersionInfo.hdr = new char[sizeof(hdr)] {};
            gVersionInfo.hdr = (char*)malloc(buff_len * sizeof(hdr));
            if (gVersionInfo.hdr) {
                gVersionInfo.hdrSize = sizeof(hdr);
                memcpy(gVersionInfo.hdr, hdr, sizeof(char)*gVersionInfo.hdrSize);
            }

            if (gVersionInfo.buff) {
                //delete[] gVersionInfo.buff;
                free(gVersionInfo.buff);
                gVersionInfo.buff = NULL;
                gVersionInfo.size = 0;
            }

            //gVersionInfo.buff = new char[buff_len] {};
            gVersionInfo.buff = (char*)malloc(buff_len * sizeof(char));
            if (gVersionInfo.buff) {
                gVersionInfo.size = buff_len;
                memcpy(gVersionInfo.buff, buff, sizeof(char)*gVersionInfo.size);
            }
            g_have_versioninfo = gVersionInfo.hdr && gVersionInfo.hdrSize > 0 &&
                                 gVersionInfo.buff && gVersionInfo.size > 0;
        }
    } else {
        if (buff == NULL) {
            //ALOGW("%s : buff is null.", __FUNCTION__);
            LOG_PRINT_W("buff is null.\n");
        } else if (buff_len <= 0) {
            //ALOGW("%s : buff_len is abnormal(%d).", __FUNCTION__, buff_len);
            LOG_PRINT_W("buff_len is abnormal(%d)\n", buff_len);
        }
    }
    return;
}

uint64_t CalcTime()
{
    uint64_t Timestamp = 0UL;
    time_t timeSec;
    struct timeval tmMilli;
    struct tm *pTmLocal;

    time(&timeSec);
    gettimeofday(&tmMilli, NULL);
    pTmLocal = localtime(&timeSec);

    Timestamp = pTmLocal->tm_sec + (uint64_t)pTmLocal->tm_min*100 + (uint64_t)pTmLocal->tm_hour*100*100 + (uint64_t)pTmLocal->tm_mday*100*100*100 + (uint64_t)(pTmLocal->tm_mon+1)*100*100*100*100 + (uint64_t)(pTmLocal->tm_year+1900)*100*100*100*100*100;

    return Timestamp;
}

// Merging all files .. function moved from framework for speed
void MergeHeader(uint64_t timeStamp)
{
    if (is_tftp() || is_ftp())
    {
        char ftpFilestr[MAX_FILE_NAME];
        sprintf(ftpFilestr, "sbuff_%ld.sdm", timeStamp);

        LOG_PRINT_I("MergeHeader slog_logfile_create_fullname \n");
        ftp_fd = slog_logfile_create_fullname(0, ftpFilestr, 0, 0);
        LOG_PRINT_I("MergeHeader ftp_fd = %d \n", ftp_fd);
        size_t written_len = 0;
        written_len = slog_logfile_save(ftp_fd, SdmLogVersionInfo, sdmLogVersionInfoSize);
        LOG_PRINT_I("----------tftp/ftp MergeHeader to DM %s---------sount=%d\n", ftpFilestr, written_len);
        return;
    }
    char filestr[100], strHeaderFile[100];
    unsigned char merge_buf[100]={0,};
    FILE *sbuff_file=NULL;
    FILE *HeaderFile=NULL;
    struct stat HeadFileSt;
    int i = 0;
    int nread = 0;

    sprintf(filestr, "%ssbuff_%ld.sdm", gSlogPath, timeStamp);
    if (useLogFileName)
    {
        sprintf(filestr, "%s%s", gSlogPath, userLogFileName);
    }
    //ALOGD( "----------MergeHeader : %s  ----------\n" , filestr);
    LOG_PRINT_D("----------MergeHeader : %s  ----------\n" , filestr);

    sbuff_file = fopen(filestr, "ab");

    //merge header file first
    sprintf(strHeaderFile,"%s.sbuff_header.sdm", gSlogPath);
    HeaderFile = fopen(strHeaderFile, "rb");
    if(HeaderFile!=NULL)
    {
        //ALOGD( "Header exists-----merging header\n");
        LOG_PRINT_D("MergeHeader : Header exists-----merging header\n");
        if (stat(strHeaderFile, &HeadFileSt) != 0)
        {
            //ALOGE("%s : stat error(%s)", __FUNCTION__, strerror(errno));
            LOG_PRINT_E("stat error(%s)\n", strerror(errno));
        }
        //ALOGD( "HeadFileSt.st_size = %ld\n",  HeadFileSt.st_size);
        LOG_PRINT_D("HeadFileSt.st_size = %ld\n",  HeadFileSt.st_size);
        while(i < HeadFileSt.st_size)
        {
            memset(merge_buf,0,sizeof(merge_buf));
            nread = fread(merge_buf, sizeof(merge_buf[0]) , sizeof(merge_buf), HeaderFile);
            fwrite(merge_buf, sizeof(merge_buf[0]), nread, sbuff_file);
            i += nread;
        }
        LOG_PRINT_I("Merge Header into %s \n",  filestr);
        fclose(HeaderFile);
        HeaderFile=NULL;
    } else {
        LOG_PRINT_E("MergeHeader : HeaderFile(%s) open fial \n" , strHeaderFile);
    }

    if(NULL!=sbuff_file)
    {
        fclose(sbuff_file);
        sbuff_file=NULL;
    }

     return ;
}

void MergeVersionInfo(uint64_t timeStamp)
{
    char filestr[100];
    FILE *sbuff_file = NULL;

    if (!g_have_versioninfo ||
        gVersionInfo.hdrSize == 0 || gVersionInfo.hdr == NULL ||
        gVersionInfo.buff == NULL || gVersionInfo.size == 0) {
        //ALOGW("%s : Invalid version info.", __FUNCTION__);
        LOG_PRINT_D("MergeVersionInfo : Invalid version info. \n");/*willa.liu@20241024 JIRA:MBB0678-423*/
        return;
    }

    sprintf(filestr, "%ssbuff_%ld.sdm", gSlogPath, timeStamp);
    if (useLogFileName)
    {
        sprintf(filestr, "%s%s", gSlogPath, userLogFileName);
    }
    //ALOGD("%s() : %s", __FUNCTION__, filestr);
    LOG_PRINT_D("MergeVersionInfo : %s. \n", filestr);
    sbuff_file = fopen(filestr, "ab");
    fwrite(gVersionInfo.hdr, sizeof(char), gVersionInfo.hdrSize, sbuff_file);
    fwrite(gVersionInfo.buff, sizeof(char), gVersionInfo.size, sbuff_file);

    if(NULL != sbuff_file)
    {
        fclose(sbuff_file);
        sbuff_file = NULL;
    }
    return;
}

int AddFileToList(uint64_t timeStamp) {
    struct DMFileManager *fileManager = DMFileManager_getInstance();
    //TODO SET DMFile basePath as gSlogPath
    DMFileManager_setBaseDir(fileManager, gSlogPath);
    if (fileMaxCounts != MANAGED_FILE_COUNT_THRESHHOLD) {
        LOG_PRINT_D("SetLimit fileMaxCounts=%d\n", fileMaxCounts);
        DMFileManager_setLimit(fileManager, 0, fileMaxCounts);
    }
    //TODO END
    if (fileManager != NULL) {
        char fileName[100] = {0};
        sprintf(fileName, "sbuff_%ld", timeStamp);
        char fileFullName[256];
        strcpy(fileFullName, fileName);
        strcat(fileFullName, ".sdm");
        DMFileManager_add(fileManager, fileFullName);
    } else {
        LOG_PRINT_E("%s : Failed to add file list.\n", __FUNCTION__);
        return -1;
    }
    return 1;
}

void *DoFileOperationThread(void *arg)
{
    pthread_mutex_lock(&mutex);
    unsigned char *write_buf;
    uint64_t privTime = gTimeForFile;

    write_buf = (unsigned char *)malloc(DR_DM_SILENT_BUF_SIZE * sizeof(unsigned char));

    // Write to file
    //ALOGD( "----------DoFileOperationThread-----\n");
    LOG_PRINT_D( "----------DoFileOperationThread-----\n");
    FILE *sbuff_file=NULL;

    char filestr[MAX_FILE_NAME];
    sprintf(filestr, "%ssbuff_%ld.sdm", gSlogPath, privTime);
    if (useLogFileName)
    {
        sprintf(filestr, "%s%s", gSlogPath, userLogFileName);
    }

    LOG_PRINT_D( "----------gZipOperation = %d-----\n", gZipOperation);
    if(gZipOperation)
    {
        if (useLogFileName)
        {
            char newfilestr[MAX_FILE_NAME];
            sprintf(newfilestr, "%ssbuff_%ld.sdm", gSlogPath, privTime);
            if (rename(filestr, newfilestr) != 0) {
                perror("Rename fail!");
                LOG_PRINT_E("Rename fail!");
            }
            LOG_PRINT_I("Rename to %s \n", newfilestr);
        } else {
            LOG_PRINT_I("Save DM log to %s \n", filestr);
        }

        privTime = gTimeForFile;
        gTimeForFile = CalcTime();
        if (is_tftp() || is_ftp())
        {
            LOG_PRINT_I("close server file. slog_logfile_close ftp_fd  = %d \n", ftp_fd);
            if (ftp_fd != -1)
            {
                slog_logfile_close(ftp_fd);
                ftp_fd = -1;
            }
        }
        MergeHeader(gTimeForFile);
        MergeVersionInfo(gTimeForFile);
        sendzipintent = 1;       // set zip intent
    }

    memset(write_buf, 0, DR_DM_SILENT_BUF_SIZE * sizeof(unsigned char));
    if(gsvbuffno==1)
    {
        memcpy(write_buf, silent_buf1, DR_DM_SILENT_BUF_SIZE * sizeof(unsigned char));
    }
    else if(gsvbuffno==2)
    {
        memcpy(write_buf, silent_buf2, DR_DM_SILENT_BUF_SIZE * sizeof(unsigned char));
    }

    //ALOGD("----------File String is gsvfileno : %s  ----------\n" ,filestr);
    LOG_PRINT_D("----------File String is gsvfileno : %s  ----------\n" ,filestr);

    if (is_tftp() || is_ftp())
    {
        char ftpFilestr[MAX_FILE_NAME];
        sprintf(ftpFilestr, "sbuff_%ld.sdm", gTimeForFile);

        size_t written_len = 0;
        written_len = slog_logfile_save(ftp_fd, write_buf, gsvoffset);
        LOG_PRINT_I("slog_logfile_save into %s, ftp_fd = %d, scount = %d\n", ftpFilestr, ftp_fd, written_len);
    } else {
        sbuff_file = fopen(filestr, "ab");

        if(NULL!=sbuff_file)
        {
            fwrite(write_buf, sizeof(write_buf[0]), gsvoffset, sbuff_file);
            //ALOGD("----------Wrote to DM /data/log/SBUFF_FILE---------sount=%d\n",scount );
            LOG_PRINT_D("----------Wrote to DM %s---------sount=%d\n", gSlogPath,scount );
            fclose(sbuff_file);
            sbuff_file=NULL;
        } else
        {
            //ALOGD("----------Opening tmpsbuff_file2 Fail-----\n");
            LOG_PRINT_D("----------Opening tmpsbuff_file2 Fail-----\n");
        }
    }

    if(sendzipintent==1){
        LOG_PRINT_I("sendzipintent = 1.\n");
        sendzipintent = 0;
        gZipOperation = false;

        char propZip[MAX_PROP_LEN] = {0, };
        //property_get(PROP_ZIP_OPERATION, propZip, "1");
        int isZip = atoi(propZip);
        if (isZip) {
            //DoZipOperation(privTime);
            LOG_PRINT_D("DoZipOperation do nothing.\n");
        } else {
            //TODO
            LOG_PRINT_D("AddFileToList.\n");
            if (!is_tftp() && !is_ftp())
            {
                AddFileToList(privTime);
            } else {
                LOG_PRINT_I("DoFileOperationThread sendzipintent=%d ftp_fd = %d \n", sendzipintent, ftp_fd);
                //slog_logfile_close(ftp_fd);
            }
        }
    }
    free(write_buf);
    pthread_mutex_unlock(&mutex);
    return NULL;
}

int CheckModemStatus(int32_t fd)
{
    //check modem status start
    int status = 0;
    int spin = 100;

    while (spin--)
    {
        status = ioctl(fd, IOCTL_MODEM_STATUS);
        if (status == STATE_ONLINE)
        {
            //ALOGD("%s : Modem  is ONLINE", __func__);
            LOG_PRINT_D("Modem  is ONLINE\n");
            gTimeForFile = CalcTime();
            MergeHeader(gTimeForFile);     // insert header in first file for first time after boot
            DoSendProfile(fd);
            return 1;
        }
        usleep(500000);
    }

    if (spin < 0)
    {
        //ALOGE("%s : Modem boot timeout", __func__);
        LOG_PRINT_E("Modem boot timeout\n");
    }
    return 0;
}

int writeBuff2File(char* buffer, int n) {
    int out_fd = open("output_file.txt", O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR); // 目标文件名
    if (out_fd < 0) {
        perror("open output file error");
        LOG_PRINT_E("writeBuff2File open output file error");
        return 1;
    }

    ssize_t written = write(out_fd, buffer, n);
    if (written < 0) {
        perror("write error");
        LOG_PRINT_E("writeBuff2File write error");
        close(out_fd);
        return 1;
    }

    // const char* tag = "TAG_TEST\n"; // TAG_TEST字符串，添加换行符
    // ssize_t tag_written = write(out_fd, tag, strlen(tag));
    // if (tag_written < 0) {
    //     perror("write tag error");
    //     close(out_fd);
    //     return 1;
    // }
    close(out_fd);
    return 0;
}

void writeSilentloggingBuff(char *buffer, int n)
{
    LOG_PRINT_D("writeSilentloggingBuff----\n");
    if (g_dmd_mode == MODE_SILENT) { // silent mode
                //ALOGD("%s : MODE_SILENT SilentLog write", __func__);
                // Calculate Time Stamp for DM packets
                CalculateTimeStamp();
                while(1)
                {
                    if(silentbufthrdlock == true)    // Is bufferthread Locked?
                    {
                        //ALOGD("Buffer Thread Locked  = %d", silentbufthrdlock);
                        usleep(100000);
                        continue;
                    }
                    else
                    {
                        //ALOGD("Buffer Thread Un-Locked ");
                        silentbufthrdlock=true;    // Make Thread Locked
                        break;
                    }
                }
                // Start Convert to sdm format
                p = buffer;
                bufnfm = buffer;        //new format data buffer
                pktlen=n;

                while (pktlen > 0) {
                    //LOG_PRINT("----------inside  while (pktlen(%d) > 0)-----\n", pktlen);
                    //ALOGD( "----------inside  while (pktlen > 0)-----\n");
                    
                            // for (int i = 0; i < n && i < n; i++) {
                            //     LOG_PRINT("0x%02X ", (unsigned char)p[i]); 
                            // }
                            // LOG_PRINT("\n"); 
                    if(p[0]  != DM_MSG_START)
                    {
                        //ALOGD( "SAVESILENTLOG-- Convert to Sdm--DM_MSG_START_FLAG(0x7F) is not found. \n");
                        LOG_PRINT_D( "SAVESILENTLOG error-- Convert to Sdm--DM_MSG_START_FLAG(0x7F) is not found. Data stize n= %d \n", n);
                        //printf( "SAVESILENTLOG error-- Convert to Sdm--DM_MSG_START_FLAG(0x7F) is not found. Data stize n= %d \n", n);
                        // if (n = 4095) {
                        //     for (int i = 0; i < n && i < 100; i++) {
                        //         LOG_PRINT("0x%02X ", (unsigned char)p[i]); 
                        //     }
                        // }
                        //writeBuff2File(p, n);
                        break;
                    }

                    msg_len = (unsigned char)p[1] + ((unsigned char)p[2] << 8);
                    msg_lennwf = (unsigned char)p[4] + ((unsigned char)p[5] << 8);
                    msg_lennwf = msg_lennwf - 2;
                    bufnfm = p + 6;

                    // msg_len doens't include '7F' and '7E' (minus 2 bytes).
                    if (msg_len <= 0 || msg_len > pktlen - 2) {
                        //ALOGE("msg_len > %d - 2 or invalid (%d)", pktlen, msg_len);
                        LOG_PRINT_D("SAVESILENTLOG error msg_len > %d - 2 or invalid (%d)", pktlen, msg_len);
                        //printf("SAVESILENTLOG error msg_len > %d - 2 or invalid (%d)", pktlen, msg_len);
                        if(EnableLogDump()) {
                            //ALOGD("*****hexdump all data this buffer*****");
                            LOG_PRINT_D("*****hexdump all data this buffer*****\n");
                            //log_hexdump((const char *)buffer, n);
                        }
                        break;
                    }

                    if(p[msg_len+1]  != DM_MSG_END) {
                        LOG_PRINT_D( "SAVESILENTLOG error-- Convert to Sdm-- DM_MSG_END_FLAG(0x7E) is not found. \n");
                        //printf( "SAVESILENTLOG error-- Convert to Sdm-- DM_MSG_END_FLAG(0x7E) is not found. \n");
                        if(EnableLogDump()) {
                            //ALOGD("*****hexdump all data this buffer*****");
                            LOG_PRINT_D("*****hexdump all data this buffer*****\n");
                            //log_hexdump((const char *)buffer, n);
                        }
                        break;
                    }

                    // msg_lennwf is a payload length.
                    // MUST not be a negative value.
                    if (msg_lennwf <= 0 || msg_lennwf > (pktlen - (6 + 1))) {
                        LOG_PRINT_D("SAVESILENTLOG error msg_lennwf > %d-(6 + 1) or invalid (%d)", pktlen, msg_lennwf);
                        //printf("SAVESILENTLOG erro msg_lennwf > %d-(6 + 1) or invalid (%d)", pktlen, msg_lennwf);
                        if(EnableLogDump()) {
                            //ALOGD("*****hexdump all data this buffer*****");
                            //log_hexdump((const char *)buffer, n);
                        }
                        break;
                    }

                    if(g_silent_logging_started == 0 && g_silent_logging_asked == 1)
                    {
                        //property_set(PROPERTY_SILENTLOG_MODE, "On");
                        LOG_PRINT_D( "received silent modem log : property set on\n");
                        g_silent_logging_started = 1;
                    }

                    if (p[10] == 1)
                    {
                        DM_start_retry = false;
                        DM_start_retry_need = true;
                        LOG_PRINT_E( "received DM start rsp from cp. \n");
                    }

                    to_send = msg_len +1;
                    totalWriteBytes += to_send;
                    double percentage = 0.0;
                    percentage = (double)totalWriteBytes / totalBytes * 100;
                    //LOG_PRINT_D( "----------totalBytes = %d, totalWriteBytes = %d  --------\n", totalBytes, totalWriteBytes);
                    //LOG_PRINT_D( "----------percentage= %.2f%%\n", percentage);
                    memset(&hdr[0],0,2);
                    *dmlen=(msg_lennwf +8);

                    // check DM start response in header file for version information
                    if(!g_have_versioninfo)
                    {
                        if(EnableLogDump())
                        {
                            log_hexdump((const char *)bufnfm, msg_lennwf);
                        }
                        CheckVersionInfo(bufnfm, msg_lennwf);
                    }

                    // select buffer to use
                    if(buff2use==1)
                    {
                        memcpy(silent_buf1+rcvoffset, hdr, sizeof(hdr));
                        rcvoffset+=sizeof(hdr);
                        memcpy(silent_buf1+rcvoffset, bufnfm, msg_lennwf); // write byte to buffer
                        rcvoffset=rcvoffset+msg_lennwf;        // as 7E skipped
                    }
                    else if (buff2use==2)
                    {
                        memcpy(silent_buf2+rcvoffset, hdr, sizeof(hdr));
                        rcvoffset+=sizeof(hdr);
                        memcpy(silent_buf2+rcvoffset, bufnfm, msg_lennwf); // write byte to buffer
                        rcvoffset=rcvoffset+msg_lennwf;        // as 7E skipped
                    }

                    LOG_PRINT_D( "----------rcvoffset = %d , buff2use=%d   -----\n",rcvoffset,buff2use);
                    p =p+to_send+1;    //skip 7E
                    pktlen = pktlen- (to_send+1);
                }
                //End convet to sdm

                //rcvoffset+=n;
                scount =scount-(n+4);    // 4 bytes added while converting to sdm format
                LOG_PRINT_D( "---------------scount= %d    rcvoffset= %d DM_start=%d \n",scount,rcvoffset,DM_start);
                if (scount<1000 ||rcvoffset>(DR_DM_SILENT_BUF_SIZE-4576))    // rcvoffset margin to avoid buffer over flow
                {
                    // Write to file
                    //ALOGD( "----------DoFileOperation-----scount= %d\n",scount);
                    LOG_PRINT_D( "----------DoFileOperation-----scount= %d\n",scount);
                    gsvbuffno=buff2use;
                    gsvoffset=rcvoffset;

                    pthread_attr_t attr;
                    pthread_attr_init(&attr);
                    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

                    pthread_create(&thread1, &attr, DoFileOperationThread, NULL);

                    //ALOGD( "    sount = [%d] , rcvoffset = [%d]  \n",scount ,rcvoffset);
                    LOG_PRINT_D("    sount = [%d] , rcvoffset = [%d]  \n",scount ,rcvoffset);
                    scount=DR_DM_SILENT_BUF_SIZE;
                    filesizeoffset += rcvoffset;
                    // Switch Buffer
                    if(buff2use==1)
                    {
                        switchclearbuff=1; // ON
                        buff2use =2;
                        rcvoffset=0;
                        // check file size max reached
                        if(filesizeoffset > gDmFileMaxSize) {
                            gZipOperation = true;
                            //ALOGD( " filesizeoffset = [%d]  \n", filesizeoffset);
                            filesizeoffset =0;
                        }
                        //ALOGD( "----------Buffer Swicthed to Second OnE --buff2use==2-----\n");
                        LOG_PRINT_D("----------Buffer Swicthed to Second OnE --buff2use==2-----\n");
                    }
                    else if(buff2use==2)
                    {
                        switchclearbuff=1; // ON
                        buff2use=1;
                        rcvoffset=0;
                        // check file size max reached
                        if(filesizeoffset > gDmFileMaxSize) {
                            gZipOperation = true;
                            //ALOGD( " filesizeoffset = [%d]  \n", filesizeoffset);
                            filesizeoffset =0;
                        }
                        //ALOGD( "----------Buffer Swicthed to First OnE --buff2use==1-----\n");
                        LOG_PRINT_D("----------Buffer Swicthed to First OnE --buff2use==1-----\n");
                    }
                }
                // clearing buffered delayed due to Thread operation writing to file from previous buffer
                if(((scount<(DR_DM_SILENT_BUF_SIZE/2))&&switchclearbuff==1))
                {
                    if(buff2use==1)
                    {
//                        memset(silent_buf2,0,sizeof(silent_buf2));
                        switchclearbuff=0; // OFF , buffer cleared
//                        ALOGD( "----------current buffer 1, silent_buf2 cleared-----\n");
                    }
                    else if(buff2use==2)
                    {
//                        memset(silent_buf1,0,sizeof(silent_buf1));
                        switchclearbuff=0; // OFF , buffer cleared
//                        ALOGD( "----------current buffer 2, silent_buf1 cleared-----\n");
                    }
                }
                silentbufthrdlock=false;    // Make Buffer Thread Un-Locked


    }
}

void *run_modem_monitor(void *arg)
{
    int32_t n;
    fd_set rfds;
    char buffer[MAX_BUF];
    int32_t fd = g_modem_fd;

    char profilePath[PATH_MAX] = {};
    strncpy(profilePath, gSlogPath, PATH_MAX);
    char* suffix = ".sbuff_profile.sdm";
    strcat(profilePath, suffix);
    //TODO
    //Cause sbuff_profile.sdm no exist in current app, CheckModemStatus never been call. So init gTimeForFile, And inser header. 
    gTimeForFile = CalcTime();
    MergeHeader(gTimeForFile);     // insert header in first file for first time after boot
    MergeVersionInfo(gTimeForFile);
    //TODO END
    LOG_PRINT_D("run_modem_monitor : gTimeForFile = %ld. \n", gTimeForFile);
    //Check Profile file exists --i.e. silent mode
    if( access( profilePath, F_OK ) != -1 ){
        //ALOGD("Profile File Exists");
        CheckModemStatus(fd);
        g_silent_logging_asked=1;
    }
    //cicle buffer
    CircularBuffer ringBuffer;
    initBuffer(&ringBuffer);
    //cicle buffer

    while(1)
    {
        if (g_modem_fd < 0) {
            //ALOGE("%s : Invalid g_modem_fd. thread exit", __FUNCTION__);
            break;
        }
        fd = g_modem_fd;

        FD_ZERO(&rfds);
        FD_SET(fd, &rfds);
        //Add timer to check modem normal or not.
        struct timeval tv;
        tv.tv_sec = detect_cp_status_timer;
        tv.tv_usec = 0;
        if (EnableLogDump()){
            //ALOGD("before select\n");
            LOG_PRINT_D("before select\n");
        }
        n = select(fd + 1, &rfds, NULL, NULL, &tv);
        //ALOGD("%s : after select", __func__);

        if (n < 0) {
            if (errno == EINTR)
                continue;
            //ALOGE("%s : select err = %d", __func__, errno);
            LOG_PRINT_E("select err = %d \n", errno);
            continue;
        } else if (n == 0)
        {
            LOG_PRINT_E("Timeout: No DM data read within %d seconds. DM_start_retry_need = %d \n", detect_cp_status_timer, DM_start_retry_need);
            if (DM_start_retry_need) {
                /*give it chance to tftp thread for last ack.*/
                if (is_tftp() || is_ftp()) {
                    LOG_PRINT_I("run_modem_monitor close server file. slog_logfile_close ftp_fd  = %d \n", ftp_fd);
                    slog_logfile_close(ftp_fd);
                    ftp_fd = -1;
                    need_create_tftp_file_again = true;
                    silentlogging_sync_network_stop();
                }
                modem_close();
                modem_init(deviceID);
                pthread_create(&thread_retry_DM_start, NULL, retry_DM_start, NULL);
            } else
                modem_close();
            continue;
        }

        if (FD_ISSET(fd, &rfds))
        {
            n = read(fd, &buffer, MAX_BUF - 1);
            //uint64_t tempT = CalcTime();
            //printf("time=%ld", tempT);
            //printf("read from fd len =%d. \n", n); 
            totalBytes += n;
            LOG_PRINT_D("totalBytes =%d. \n", totalBytes); 
            //LOG_PRINT("Read data form fd n=%d. \n", n); 
            //writeBuff2File(buffer, n);
            if (n < 0) {
                if (errno == EINTR)
                    continue;
                LOG_PRINT_D("read err \n");
                continue;
            }
            else if (n == 0) // status unpluged
            {
                LOG_PRINT_E("%d EOF, try restart DM log! DM_start_retry_need = %d \n", fd, DM_start_retry_need);
                if (DM_start_retry_need) {
                    /*give it chance to tftp thread for last ack.*/
                    if (is_tftp() || is_ftp()) {
                        LOG_PRINT_I("run_modem_monitor close server file. slog_logfile_close ftp_fd  = %d \n", ftp_fd);
                        slog_logfile_close(ftp_fd);
                        ftp_fd = -1;
                        need_create_tftp_file_again = true;
                        silentlogging_sync_network_stop();
                    }

                    modem_close();
                    modem_init(deviceID);
                    pthread_create(&thread_retry_DM_start, NULL, retry_DM_start, NULL);
                } else
                    modem_close();
                continue;
            }
            // if (LOG_ENABLE)
            // {
            //     LOG_PRINT_D("Print buffer from fd(%s) \n", deviceID);
            //     for (int i = 0; i < n && i < n; i++) {
            //         LOG_PRINT_D("0x%02X ", (unsigned char)buffer[i]); 
            //     }
            //     LOG_PRINT_D("\n");
            // }
            
            // write fd data into ringBuffer
            writeBuffer(&ringBuffer, buffer, n);
            //printBuffer(&ringBuffer);

            while (ringBuffer.size >= 3) { // make sure size enough to parse
                //LOG_PRINT("++++ringBuffer.tail=%d, ringBuffer.data=%p \n", ringBuffer.tail, ringBuffer.data);
                unsigned char *p_tial = ringBuffer.data + ringBuffer.tail;
                if (*p_tial == 0x7F) {
                    size_t len1 = ringBuffer.data + (ringBuffer.tail +1)%TEMP_BUFFER_SIZE;
                    size_t len2 = ringBuffer.data + (ringBuffer.tail +2)%TEMP_BUFFER_SIZE;
                    LOG_PRINT_D("0x%02X, 0x%02X, 0x%02X \n", (unsigned char)(*p_tial), (unsigned char)*((unsigned char *)len1), (unsigned char)*((unsigned char *)len2)); 
                    size_t msg_len = *((unsigned char *)len1) + (*((unsigned char *)len2) << 8);
                    //LOG_PRINT("0x%02X, 0x%02X \n", (unsigned char)(p_tial + 1), (unsigned char)(p_tial + 2)); 
                    LOG_PRINT_D(" ++++msg_len=%d \n", msg_len);
                    size_t total_length = msg_len + 2; // include 7F and 7E
                    if (total_length > DM_PACKAGE_MAX_SIZE || msg_len < 0)
                    {
                        ringBuffer.tail = (ringBuffer.tail + 1) % TEMP_BUFFER_SIZE;
                        ringBuffer.size--;
                        LOG_PRINT_W("total_length > DM_PACKAGE_MAX_SIZE or msg_len < 0, Skill this package! \n");
                        continue;
                    }

                    if (ringBuffer.size >= total_length) {
                        unsigned char tempBuffer[TEMP_BUFFER_SIZE];
                        readBuffer(&ringBuffer, tempBuffer, total_length);
                        // LOG_PRINT_D("Print tempBuffer:\n"); 
                        // if (LOG_ENABLE)
                        // {
                        //      for (int i = 0; i < n && i < total_length; i++) {
                        //          LOG_PRINT("0x%02X ", (unsigned char)tempBuffer[i]); 
                        //      }
                        //      LOG_PRINT("\n"); 
                        // }
                        //Deal with dm log data 
                        writeSilentloggingBuff(tempBuffer, total_length);
                        continue;
                    } else {
                        // data length not enough, break
                        //LOG_PRINT("ringBuffer.size =%d, total_length=%d \n", ringBuffer.size, total_length); 
                        //LOG_PRINT("ringBuffer.size < 3, break --- \n"); 
                        break;
                    }
                         
                }
                if (ringBuffer.size <= 0) {
                    break;
                }else{
                    // move to next index
                    //LOG_PRINT("ringBuffer.tail(%d) +1, ringBuffer.size(%d) -1 \n", ringBuffer.tail, ringBuffer.size); 
                    ringBuffer.tail = (ringBuffer.tail + 1) % TEMP_BUFFER_SIZE;
                    ringBuffer.size--;
                }
                
                //LOG_PRINT("Still in while ......, ringBuffer.size = %d \n", ringBuffer.size); 
                
            }
            //LOG_PRINT("Exit while ......, ringBuffer.size = %d \n", ringBuffer.size);
        }
    }
            
}

void start_monitor_thread(void)
{
    //run thread
    pthread_create(&thread_id_modem, NULL, run_modem_monitor, NULL);
}

void retry_DM_start () {
    //Send start request until START_RSP read from fd.
    while(DM_start_retry) {
        LOG_PRINT_W(" ++++DM_start_retry send ++++ \n");
        SendProfileDirect();
        LOG_PRINT_W(" ++++DM_start_retry send ---- \n");
        sleep(1);
    }
}

bool SetLogPath(const char *path)
{
    //strcpy(gSlogPath, path);
    size_t pathLen = strlen(gSlogPath);
    DIR *targetDir = NULL;

    if (pathLen > PATH_MAX - 2 || pathLen == 0)
        goto InvaildPath;

    if (gSlogPath[pathLen - 1] != '/') {
        gSlogPath[pathLen] = '/';
        gSlogPath[pathLen + 1] = '\0';
    }

    targetDir = opendir(gSlogPath);
    if (targetDir)
        closedir(targetDir);
    else {
        //ALOGW("[DMProperty::%s] Fail to open %s: %d", __FUNCTION__, targetDir, errno);
        LOG_PRINT_W("SetLogPath: Fail to open %s \n", targetDir);
        goto InvaildPath;
    }

    return true;

InvaildPath:
    strncpy(gSlogPath, SLOG_PATH, PATH_MAX);
    return false;
}

bool mkdir_slog(void)
{
    if (chdir(gSlogPath) < 0) {
        if (mkdir(gSlogPath, S_IRWXU | S_IRWXG | S_IRWXO) < 0) {
            //ALOGE("mkdir %s create fail", gSlogPath);
            LOG_PRINT_E("mkdir %s create fail\n", gSlogPath);
            return false;
        }
        else
            //ALOGD("%s : %s", __func__, gSlogPath);
            LOG_PRINT_D("mkdir_slog : %s \n", gSlogPath);
    }

    LOG_PRINT_D("mkdir_slog : success mkdir!\n");
    return true;
}

void print_help() {
    printf("Usage: silentlogging [options]\n");
    printf("Options:\n");
    printf("  -l, --logpath <value>           Set the log path.\n");
    printf("  -p, --profileFilter <value>     Specify the log level profile.\n");
    printf("  -d, --deviceID <value>          Specify the device.\n");
    printf("  -s, --size <value>              Specify the single log file max size .\n");
    printf("  -c, --count <value>             Specify the log file max counts.\n");
    printf("  -o, --name <value>              Specify the log file name.\n");
    printf("  -h, --help                      Display this help message.\n");
}

void gerUserOpiton(int argc, char **argv) {
    int opt = -1;
    // char cwd[PATH_MAX];
    // if (getcwd(cwd, sizeof(cwd)) != NULL) {
    //     LOG_PRINT("Current path: %s\n", cwd);
    //     snprintf(gSlogPath, sizeof(gSlogPath), "%s/slog/", cwd);
    //     LOG_PRINT("Log path: %s\n", gSlogPath);
    // } else {
    //     perror("getcwd() error!");
    //     return 1;
    // }
    strcpy(gSlogPath, SLOG_PATH);
    strcpy(deviceID, PATH_MODEM);
    if (argc < 2 || argv == NULL)
    {
        return;
    } else {
        struct option long_options[] = {
            {"logPath", required_argument, NULL, 'l'},
            {"profile", required_argument, NULL, 'p'},
            {"deviceID", required_argument, NULL, 'd'},
            {"size", required_argument, NULL, 's'},
            {"count", required_argument, NULL, 'c'},
            {"name", required_argument, NULL, 'o'},
            {"tftpip", required_argument, NULL, 't'},
            {"ftpip", required_argument, NULL, 'q'},
            {"ftpAcc", required_argument, NULL, 'f'},
            {"ftpPsw", required_argument, NULL, 'w'},
            {"tcp", required_argument, NULL, 'g'},
            {"help", no_argument, NULL, 'h'},
            {0, 0, 0, 0, 0, 0, 0}
        };
        int option_index = 0;
        while ((opt = getopt_long(argc, argv, "l:p:d:s:c:o:t:q:f:w:g:h", long_options, &option_index)) != -1) {
            switch (opt) {
                case 'l':
                    LOG_PRINT_I("Get user log path: %s \n", optarg);
                    strcpy(gSlogPath, optarg);
                    if (gSlogPath[0] != '/') 
                    {
                        char cwd[PATH_MAX];
                        if (getcwd(cwd, sizeof(cwd)) != NULL) {
                            LOG_PRINT_I("Current path: %s\n", gSlogPath);
                            snprintf(gSlogPath, sizeof(gSlogPath), "%s/%s", cwd, optarg);
                            LOG_PRINT_I("Abs Log path: %s\n", gSlogPath);
                        } else {
                            perror("getcwd() error!");
                            LOG_PRINT_E("getcwd() error!");
                            return 1;
                        }
                    }
                    LOG_PRINT_I("set target folder for ftp: %s \n", gSlogPath);
                    target_folder_get_parse_for_ftp(gSlogPath);
                    break;
                case 'p':
                    //profilePath = optarg;
                    logLevel = atoi(optarg);
                    LOG_PRINT_I("Get user log level profile: %d \n", logLevel);
                    if (logLevel < 0 || logLevel > 3) {
                        LOG_PRINT_W("Input log level profile invalid, Use Defaul log level profile = 0. \n");
                        logLevel = 0;
                    }
                    //TODO 
                    break;
                case 'd':
                    LOG_PRINT_I("Get user deviceID is %s \n", optarg);
                    strcpy(deviceID, optarg);
                    break;
                case 's':
                    gDmFileMaxSize = atoi(optarg);
                    LOG_PRINT_I("Get user single log file max size: %s \n", optarg);
                    if (gDmFileMaxSize < 0 || gDmFileMaxSize > DR_DM_MAX_FILE_SIZE) {
                        LOG_PRINT_W("Input user single log file max size is invalid, Use Defaul size = 100M. \n");
                        gDmFileMaxSize = DR_DM_DEF_FILE_SIZE;
                    } else {
                        gDmFileMaxSize = gDmFileMaxSize*1024*1024;
                        LOG_PRINT_D("Input user single log file max size is %d \n", gDmFileMaxSize);
                    }
                    break;
                case 'c':
                    fileMaxCounts = atoi(optarg) - 1;/*willa.liu@20241108 JIRA:MBB0678-499*/
                    LOG_PRINT_I("Get user log file max counts: %s \n", optarg);
                    if (fileMaxCounts < 1 || fileMaxCounts > 1000) { /*willa.liu@20241108 JIRA:MBB0678-499*/
                        LOG_PRINT_W("Input user log file max counts should be:2~1000, Use Defaul count = 1000. \n");/*willa.liu@20241108 JIRA:MBB0678-499*/
                        fileMaxCounts = MANAGED_FILE_COUNT_THRESHHOLD;
                    } 
                    break;
                case 'o':
                    strcpy(userLogFileName, optarg);
                    LOG_PRINT_I("Get user set temp log file name: %s \n", userLogFileName);
                    if (userLogFileName != NULL && strlen(userLogFileName) > 0)
                    {
                        useLogFileName = true;
                    } 
                    break;
                case 't':/*tftp ip*/
                {
                    g_tftp_server_ip = optarg;
                    LOG_PRINT_I("tftp got %s\n",g_tftp_server_ip);
                    useLogFileName = false;
                    break;
                }
                case 'q':/*ftp ip*/
                {
                    g_ftp_server_ip = optarg;
                    LOG_PRINT_I("g_ftp_server_ip got %s\n",g_ftp_server_ip);
                    useLogFileName = false;
                    break;
                }
                case 'f':/*ftp account*/
                {
                    g_ftp_server_usr = optarg;
                    LOG_PRINT_I("g_ftp_server_usr got %s\n",g_ftp_server_usr);
                    break;
                }
                case 'w':/*ftp password*/
                {
                    g_ftp_server_pass = optarg;
                    LOG_PRINT_I("g_ftp_server_pass got %s\n",g_ftp_server_pass);
                    break;
                }
                case 'g': /*tcp option*/
                {
                    enableTCP = atoi(optarg) == 1;
                    if (enableTCP) {
                        LOG_PRINT_I("TCP log enabled! \n");
                    } else {
                        LOG_PRINT_I("TCP log disabled! \n");
                    }
                    break;
                }
                case 'h':
                    print_help();
                    exit(0);
                default:
                    fprintf(stderr, "See 'silentlogging -h' Usage: \n");
                    exit(0);
            }
        }
    }
    
}

int silentlogging_sync_network_stop(void) {
    int result = 0;
    /*give it chance to tftp thread for last ack.*/
    if (is_tftp() || is_ftp())
    {
        int wait_times = 0;
        #define WAIT_TFTP_TIMES (6)
        while(is_transfer_ongoing() && (wait_times++ < WAIT_TFTP_TIMES)) {
            sleep(1);
        }
        if (wait_times > WAIT_TFTP_TIMES) {
            LOG_PRINT_E("time out for waiting for tftp thread quit...\n");
            result = -1;
        }

        /*ftp connection finish*/
        s_ftp_quit();
    }

    return result;
}


int32_t main(int argc, char **argv) {
    LOG_PRINT_I("################  Silentlogging start  ################\n");
    pthread_mutex_init(&mutex, NULL);

    gerUserOpiton(argc, argv);
    if (!mkdir_slog())
	        return 0;

    if (!SetLogPath(gSlogPath)){
        return 0;
    }

    if (!init_monitor()) {
        LOG_PRINT_E("init monitor fail\n");
        return 0;
    }

    exitSignalHandle();
    // 1. Touch header
    // 2. send profile including start command
    //TouchSdmHeader(false);
    LOG_PRINT_D("createHeaderFile start\n");
    createHeaderFile();
    start_monitor_thread();

    //SetCurrentMode(MODE_SILENT_LOGGING);
    SendProfileDirect();

    while(1){
        sleep(1000);
    }
    return 0;
}
