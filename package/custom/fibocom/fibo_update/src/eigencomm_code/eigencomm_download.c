#include <stdio.h>
#include <getopt.h>
#include <string.h>

#include "global_def.h"
#include "utils.h"
#include "action.h"
#include "eigencomm_devices_list.h"
#include "linux_comm.h"

#define BTOOL_VERSION "V1.0.0"
#define CUSTOM_PORT "/dev/ttyUSB1"

/** shijiaxing 2024-1016 rename old pdev to g_eigencomm_pdev for preventing potentianl namespace conflict to other files.*/
static fibo_usbdev_t *g_eigencomm_pdev = NULL;


// command line options
static const struct option options[] = {{ "port",       required_argument,  NULL, 'd' },
                                        { "cfgfile",    required_argument,  NULL, 'c' },
                                        { "single_burn",required_argument,  NULL, 'b' },
                                        { "burnlist",   required_argument,  NULL, 'B' },
                                        { "single_erase", required_argument,  NULL, 'e' },
                                        { "eraselist",  required_argument,  NULL, 'E' },
                                        { "file",       required_argument,  NULL, 'f' },
                                        { "readmem",    required_argument,  NULL, 'R' },
                                        { "split",      no_argument,        NULL, 'S' },
                                        { "reset",      no_argument,        NULL, 'r' },
                                        { "skip",       no_argument,        NULL, 's' },
                                        { "version",    no_argument,        NULL, 'v' },
                                        { "help",       no_argument,        NULL, 'h' },
                                        { "module_type",required_argument,  NULL, 't' }};

static const char *opt_string = "d:c:f:b:t:B:e:E:R:Srsvh"; // ':'means has a parameter,for exp: "d:" means -d param, "v" means -v

// action table
typedef int32_t(*ActionHandleFuncP)(stPrimCliCfg* actionCfg);
typedef struct {
    const char *actionName;
    const ActionHandleFuncP actionFunc;
}stAction;

static const stAction actions[] = {
    { "split", action_split_pkg },
    { "eraselist", action_erase_list },
    { "burnlist", action_burn_list },
    { "single_burn", action_burn },
    { "single_erase", action_erase },
    { "readmem", action_read_memory},
};

static void print_help() {
  // clang-format off
  fprintf(stdout, "upgrade_tool is a tool to burn image files\n\n"
          "USAGE:\n"
          "    upgrade_tool [options] <command> [<arguments...>]\n\n"
          "VERSION:\n"
          "    %s\n\n"
          "OPTIONS:\n"
          "    -d, --port              Set uart or usb port\n"
          "    -c, --cfgfile           Set configuretion file path\n"
          "    -b, --single_burn       Burn image,params: [BL|AP|CP|FF|OTHER1|OTHER2|OTHER3|OTHER4|OTHER5]\n"
          "    -B, --burnlist          Burn image list, params: [BL|AP|CP|FF] [BL|AP|CP|FF] [BL|AP|CP|FF|..]...\n"
          "    -e, --single_erase      Erase specified section: [addr] [length] [type]\n"
          "    -E, --eraselist         Erase flash type list, params: [all|nvm|cal] [all|nvm|cal] [all|nvm|cal]\n"
          "    -f, --file              Set burn image binpkg file\n"
          "    -R, --readmem,          Read memory from UE, params: [addr] [length] [path]\n"
          "    -S, --split             Split pkg file\n"
          "    -r, --reset             System reset after burn\n"
          "    -s, --skip              Skip agent boot download\n"
          "    -v, --version           Print tool version\n"
          "    -h, --help              Print help information\n"
          "    -t, --module_type       Change module type\n",
          BTOOL_VERSION
  );
  fflush(stdout);
  // clang-format on
}

fibo_usbdev_t *eigencomm_find_devices_in_table(int idvendor, int idproduct, int check_port_type)
{
    int i, size = 0;
    fibo_usbdev_t *pdev = NULL;

    // LogInfo("start\n");
    if (check_port_type) {
        size = sizeof(eigencomm_devices_table)/sizeof(eigencomm_devices_table[0]);
        for (i=0; i<size; i++)
        {
            pdev = &eigencomm_devices_table[i];

            if ((pdev->idVendor == idvendor) && (pdev->idProduct == idproduct)) {
                goto FIND;
            }
        }

        if (check_port_type == 1) {
            return NULL;
        }
    }

    size = sizeof(eigencomm_dl_devices_table)/sizeof(eigencomm_dl_devices_table[0]);
    for (i=0; i<size; i++)
    {
        pdev = &eigencomm_dl_devices_table[i];
        if ((pdev->idVendor == idvendor) && (pdev->idProduct == idproduct)) {
            goto FIND;
        }
    }

    return NULL;
FIND:
    pdev->ttyfd = -1;
    pdev->usbdev = -1;
    pdev->tcp_client_fd = -1;
    pdev->pcie_fd = -1;
    return pdev;
}

static int CheckATPort(char *portname, char *syspath, int times)
{
    int i = 0;

    if (portname == NULL)
    {
        LogInfo("portname null handler");
        return -1;
    }

    if (syspath == NULL)
    {
        LogInfo("syspath null handler");
        return -1;
    }

    LogInfo("wait for at port\r\n");
    for (i=0; i<times; i++) {
        g_eigencomm_pdev = fibo_get_fibocom_device(eigencomm_find_devices_in_table, portname, syspath, 1);
        if (g_eigencomm_pdev != NULL) {
            return 0;
        }
        usleep(5000);
    }

    return -1;
}

static int CheckDLPort(char *portname, char *syspath, int times)
{
    int i = 0;

    if (portname == NULL)
    {
        LogInfo("portname null handler");
        return -1;
    }

    if (syspath == NULL)
    {
        LogInfo("syspath null handler");
        return -1;
    }

    LogInfo("wait for dl port\r\n");
    for (i = 0; i < times; i++)
    {
        g_eigencomm_pdev = fibo_get_fibocom_device(eigencomm_find_devices_in_table, portname, syspath, 0);
        if (g_eigencomm_pdev != NULL)
        {
            return 0;
        }

        usleep(5000);
    }
    return -1;
}

int eigencomm_send_atcmd(fibo_usbdev_t *pdev, char * at_cmd)
{
    int len, i;
    int fd;
    char buf[256];

    if (pdev == NULL)
    {
        LogInfo("pdev");
        return -1;
    }

    if (at_cmd == NULL)
    {
        LogInfo("at_cmd");
        return -1;
    }

    LogInfo("open at port %s\n",pdev->at_port);
    fd = open(pdev->at_port, O_RDWR | O_NOCTTY);
    if (fd < 0)
    {
        LogInfo("open");
        return -1;
    }

    fcntl(fd, F_SETFL, fcntl(fd,F_GETFL) | O_NONBLOCK);

    /** shijiaxing 2024-1016 MTC0708-1012: fix atcmd comm issue.
     * in some user's sbc system, AT port without initialize as uart behaviers unexpectively.
     * it sends bad AT cmd request to device, and takes wrong response through and from AT port.
     * initialize AT port as uart device to fix this issue.
     * to prevent changing misc_usb code, just copy from misc_usb.c then add two init below.
    */
    init(fd);
    set_baud(fd, 115200);

    len = write(fd, at_cmd, strlen(at_cmd));
    LogInfo("write %d is %s\r\n", len, at_cmd);
    if (len <= 0)
    {
        LogInfo("write");
        close(fd);
        return -1;
    }

    LogInfo("write is ok\r\n");


    for(i=0; i< 2; i++)
    {
        memset(buf, 0, sizeof(buf));
        len = read(fd, buf, sizeof(buf) - 1);
        if (len < 0)
        {
            LogInfo("read fail\r\n");
        }else{
            buf[len] = '\0';
            LogInfo("NO %d read %d is %s\r\n", i, len, buf);
            break;
        }
        usleep(300000);
    }

    sleep(1);
    close(fd);
    return 0;
}

int32_t action_handle(stPrimCliCfg* actionCfg)
{
    int32_t ret = 0;
    int32_t i,act_len;
    ActionHandleFuncP pFunc = NULL;
    stPrimCliCfg* _pActCfg = actionCfg;
    printf("actionCfg is %s\r\n", actionCfg->action);
    // action
    act_len = sizeof(actions)/sizeof(stAction);
    for (i=0; i<act_len; i++)
    {
        lowercase(_pActCfg->action);
        if (strstr((const char*)_pActCfg->action, actions[i].actionName) != NULL)
        {
            pFunc = actions[i].actionFunc;
        }

        if (pFunc != NULL)
        {
            printf("supported action: %s\r\n", actions[i].actionName);
            ret = pFunc(_pActCfg);
        }

        if(ret !=0)
        {
            printf("action: %s error\r\n", actions[i].actionName);
            return -1;
        }

        pFunc = NULL;
    }
    printf("all supported action is end\r\n");
    return ret;
}


int popen_write(const char*cmd)
{
    printf("%s %d  cmd=%s\n", __func__, __LINE__,cmd);
    FILE *fp;
    //char buf[256] = {0};
    if((fp = popen(cmd, "r")) == NULL)
    {
        printf("Fail to popen\n");
        return -1;
    }
    pclose(fp);
    return 0;
}

int copy_image(char *imageFile)
{
    int ret=0;
    char cmd[256]={0};

    snprintf(cmd, sizeof(cmd), "rm %s/*.binpkg", PATH_PREFIX);
    popen_write(cmd);

    snprintf(cmd, sizeof(cmd), "rm %s/xpk*", PATH_PREFIX);
    popen_write(cmd);

    snprintf(cmd, sizeof(cmd), "rm %s/config_xpk*", PATH_PREFIX);
    popen_write(cmd);

    snprintf(cmd, sizeof(cmd), "rm %s/ap_bootloader.bin", PATH_PREFIX);
    popen_write(cmd);
    snprintf(cmd, sizeof(cmd), "rm %s/ap_fibo_soft.bin", PATH_PREFIX);
    popen_write(cmd);
    snprintf(cmd, sizeof(cmd), "rm %s/cp-demo-flash.bin", PATH_PREFIX);
    popen_write(cmd);

    sprintf(cmd,"cp %s %s/image.binpkg",imageFile, PATH_PREFIX);
    ret = popen_write(cmd);
    if(ret != 0)
    {
        printf("image file copy error\n");
        return -1;
    }
    return 0;
}

int32_t eigencomm_download_main(int32_t argc, char **argv)
{
    int32_t c, ret=0;
    int timeout = 3;
    stPrimCliCfg cliCfg = {0};
    char imageFile[256]={0};
    char portname[MAX_PATH_LEN] = {0}, syspath[MAX_PATH_LEN] = {0};
    char *action_str = "";

    memset(&cliCfg, 0, sizeof(stPrimCliCfg));
    umask(0);
    optind = 1;//must set to 1

    //2024-11-14, Shijiaxing, MTC0709-989, Begin. TPLink feature: upgrade using custom image.
    //use strncat() instead strncpy() to resolve conflict sequence of options.
    //note the leading ' ' to the action string, passed to strncat(). 
    while ((c = getopt_long(argc, argv, opt_string, options, NULL)) != -1)
    {
        switch(c) {
            case 'h':
                print_help();
                return 0;
            case 'v':
                LogInfo("BTool version %s\n", BTOOL_VERSION);
                return 0;
            case 'd':
                strncpy(cliCfg.port, optarg, sizeof(cliCfg.port));
                LogInfo("port is %s\r\n",cliCfg.port);
                break;
            case 'c':
                strncpy(cliCfg.cfgFile, optarg, sizeof(cliCfg.cfgFile));
                break;
            case 'b':
                strncpy(cliCfg.burnFileType, optarg, sizeof(cliCfg.burnFileType));
                action_str = " single_burn";
                strncat(cliCfg.action, action_str, strlen(action_str));
                break;
            case 'B':
                strncpy(cliCfg.burnFileList, optarg, sizeof(cliCfg.burnFileList));
                action_str = " burnlist";
                strncat(cliCfg.action, action_str, strlen(action_str));
                break;
            case 'e':
                strncpy(cliCfg.eraseSection, optarg, sizeof(cliCfg.eraseSection));
                action_str = " single_erase";
                strncat(cliCfg.action, action_str, strlen(action_str));
                break;
            case 'E':
                strncpy(cliCfg.eraseList, optarg, sizeof(cliCfg.eraseList));
                action_str = " eraselist";
                strncat(cliCfg.action, action_str, strlen(action_str));
                break;
            case 'f':
                strncpy(imageFile, optarg, sizeof(imageFile));
                LogInfo("imageFile is %s\r\n",imageFile);
                if (access(imageFile, F_OK) != 0)
                {
                    LogInfo("%s not exist\r\n", imageFile);
                    return -1;
                }
                copy_image(imageFile);

                snprintf(cliCfg.cfgFile, sizeof(cliCfg.cfgFile), "%s%s", PATH_PREFIX, INI_718S_PATH);
                action_str = " split";
                strncat(cliCfg.action, action_str, strlen(action_str));
                cliCfg.reset = 1;
                break;
            case 'R':
                strncpy(cliCfg.readSection, optarg, sizeof(cliCfg.readSection));
                action_str = " readmem";
                strncat(cliCfg.action, action_str, strlen(action_str));
                break;
            case 's':
                cliCfg.skipAgDl = 1;
                break;
            case 'r':
                cliCfg.reset = 1;
                break;
            case 'S':
                action_str = " split";
                strncat(cliCfg.action, action_str, strlen(action_str));
                break;
            case 't':
                LogInfo("LE370 module_type\r\n");
                break;
            default:
                print_help();
                return -1;
        }
    }

    if (strlen(cliCfg.action) == 0)
    {
        print_help();
        return -1;
    }
    else if (strlen(imageFile) != 0) //reserve origional use of option '-f'. set as default operation.
    {
        if (strlen(cliCfg.eraseList) == 0)
        {
            strncpy(cliCfg.eraseList, "ALL", sizeof(cliCfg.eraseList));
            action_str = " eraselist";
            strncat(cliCfg.action, action_str, strlen(action_str));
        }
        if (strlen(cliCfg.burnFileList) == 0)
        {
            strncpy(cliCfg.burnFileList, "BL AP CP", sizeof(cliCfg.burnFileList));
            action_str = " burnlist";
            strncat(cliCfg.action, action_str, strlen(action_str));
        }
    }
    //2024-11-14, Shijiaxing, MTC0709-989, End. TPLink feature: upgrade using custom image.

    /** shijiaxing 2024-1016 MTC0708-1012: fix crash dump.
     * in previous code, send atcmd first, when waiting for download port by CheckDLPort().
     * but when DL port not found, CheckDLPort() would set pdev = NULL,
     * which would cause retry in fibo_common_send_atcmd() crash without a pdev NULL check.
    */
    timeout = 3;
    while (!CheckATPort(portname, syspath, 2) && (timeout > 0))
    {
        if ((g_eigencomm_pdev->idVendor == eigencomm_devices_table[1].idVendor)
            && (g_eigencomm_pdev->idProduct == eigencomm_devices_table[1].idProduct)
            && (cliCfg.port[0] == 0))
        {
            strcpy(cliCfg.port, CUSTOM_PORT);
        }

        if (!eigencomm_send_atcmd(g_eigencomm_pdev, "AT\r\n"))
        {
            LogInfo("Device start to reset, please wait.\r\n");
            eigencomm_send_atcmd(g_eigencomm_pdev, "AT+GTDLMODE=\"autodloader\"\r\n");

            if (!CheckDLPort(portname, syspath, 1000))
            {
                break;  //found DL Port, break
            }
        }
        timeout--;
        usleep(10000);
    }

    if (timeout <= 0)
    {
        LogInfo("Send AT CMD timeout.\r\n");
    }

    if (CheckDLPort(portname, syspath, 5000)) {
        LogInfo("Device Not Found, please turn device into DL mode.\r\n");
        return -1;
    }

    if(cliCfg.port[0] == 0)
    {
        LogInfo("cliCfg.port is Null, use pdev port\r\n");
        strncpy(cliCfg.port, g_eigencomm_pdev->portname, sizeof(cliCfg.port));
    }

    ret = action_handle(&cliCfg);
    LogInfo("-----------------------\r\n");
    if(ret == 0)
    {
        LogInfo("Upgrade module successfully\r\n");
    }
    else
    {
        LogInfo("Upgrade module failed\r\n");
    }
    LogInfo("-----------------------\r\n");

    return ret;
}
