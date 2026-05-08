/*******************************************************************
 *  CopyRight(C) 2022-2026  Fibocom Wireless Inc
 *******************************************************************
 * FileName : firehose_download.c
 * DESCRIPTION : upgrade_tool for USB and PCIE of Fibocom modules
 * Author   : Frank.zhou
 * Date     : 2022.08.22
 *******************************************************************/
#include "firehose_download.h"

#define STORAGE_TYPE_NAND 1  //nand
#define STORAGE_TYPE_EMMC 2  //emmc
int storage_type = STORAGE_TYPE_NAND;
int skip_program_gptmain0 = 1;

static int64_t s_total_filesize = 0;
static int64_t s_transfer_bytes = 0;
static void set_transfer_allbytes(int64_t bytes)
{
    s_transfer_bytes = 0;
    s_total_filesize = bytes;
}

static int update_transfer_bytes(int64_t bytes_cur)
{
    static int last_percent = -1;
    int percent = 0;

    if (bytes_cur == -1 || bytes_cur == 0) {
        percent = bytes_cur;
    }
    else {
        s_transfer_bytes += bytes_cur;
        percent = (s_transfer_bytes * 100) / s_total_filesize;
    }

    if (percent != last_percent) {
        last_percent = percent;
    }

    return percent;
}

static void show_progress()
{
    static int percent = 0;

    if (s_total_filesize > 0) {
        percent = (s_transfer_bytes * 100) / s_total_filesize;
    }
    LogInfo("\nupgrade progress %d%% %lu/%lu\n", percent, s_transfer_bytes, s_total_filesize);
}

static const char * fh_xml_find_value(const char *xml_line, const char *key, char **ppend)
{
    char *pchar = strstr(xml_line, key);
    char *pend;

    if (!pchar)
    {
        LogInfo("%s: no key %s in %s\n", __func__, key, xml_line);
        return NULL;
    }

    pchar += strlen(key);
    if (pchar[0] != '=' && pchar[1] != '"')
    {
        LogInfo("%s: no start %s in %s\n", __func__, "=\"", xml_line);
        return NULL;
    }

    pchar += strlen("=\"");
    pend = strstr(pchar, "\"");
    if (!pend)
    {
        LogInfo("%s: no end %s in %s\n", __func__, "\"", xml_line);
        return NULL;
    }

    *ppend = pend;
    return pchar;
}

static const char *fh_xml_get_value(const char *xml_line, const char *key)
{
    static char value[64];

    char *pchar = strstr(xml_line, key);
    char *pend;

    if (!pchar) {
        LogInfo("no key %s in %s\n", key, xml_line);
        return NULL;
    }

    pchar += strlen(key);
    if (pchar[0] != '=' && pchar[1] != '"') {
        LogInfo("no start %s in %s\n", "=\"", xml_line);
        return NULL;
    }

    pchar += strlen("=\"");
    pend = strstr(pchar, "\"");
    if (!pend) {
        LogInfo("no end %s in %s\n", "\"", xml_line);
        return NULL;
    }

    strncpy(value, pchar, pend - pchar);
    value[pend - pchar] = '\0';

    //LogInfo("%s=%s\n", key, value);

    return value;
}

static void fh_xml_set_value(char *xml_line, const char *key, unsigned value)
{
    char *pend;
    const char *pchar = fh_xml_find_value(xml_line, key, &pend);
    char value_str[32];
    char *tmp_line = malloc(strlen(xml_line) + 1 + sizeof(value_str));

    if (!pchar || !tmp_line)
    {
        return;
    }

    strcpy(tmp_line, xml_line);
    snprintf(value_str, sizeof(value_str), "%u", value);
    tmp_line[pchar - xml_line] = '\0';
    strcat(tmp_line, value_str);
    strcat(tmp_line, pend);
    strcpy(xml_line, tmp_line);
    free(tmp_line);
}

static int fh_parse_xml_line(const char *xml_line, struct fh_cmd *fh_cmd)
{
    const char *pchar = NULL;
    size_t len = strlen(xml_line);

    memset(fh_cmd, 0, sizeof(struct fh_cmd));
    strcpy(fh_cmd->xml_original_data, xml_line);
    if (fh_cmd->xml_original_data[len - 1] == '\n')
    {
        fh_cmd->xml_original_data[len - 1] = '\0';

    }

    if (strstr(xml_line, "vendor=\"fibocom\""))
    {
        fh_cmd->vdef.type = "vendor";
        snprintf(fh_cmd->vdef.buffer, sizeof(fh_cmd->vdef.buffer), "%s", xml_line);
        return 0;
    }
    else if (!strncmp(xml_line, "<erase ", strlen("<erase ")))
    {
        fh_cmd->erase.type = "erase";
        if ((pchar = fh_xml_get_value(xml_line, "PAGES_PER_BLOCK")))
            fh_cmd->erase.PAGES_PER_BLOCK = strtoul(pchar, NULL, 10);
        if ((pchar = fh_xml_get_value(xml_line, "SECTOR_SIZE_IN_BYTES")))
            fh_cmd->erase.SECTOR_SIZE_IN_BYTES = strtoul(pchar, NULL, 10);
        if (strstr(xml_line, "label") && strstr(xml_line, "last_sector"))
        {
            if ((pchar = fh_xml_get_value(xml_line, "label")))
                strcpy(fh_cmd->erase.label, pchar);
            if ((pchar = fh_xml_get_value(xml_line, "last_sector")))
                fh_cmd->erase.last_sector = strtoul(pchar, NULL, 10);
        }
        if ((pchar = fh_xml_get_value(xml_line, "num_partition_sectors")))
            fh_cmd->erase.num_partition_sectors = strtoul(pchar, NULL, 10);
        if ((pchar = fh_xml_get_value(xml_line, "physical_partition_number")))
            fh_cmd->erase.physical_partition_number = strtoul(pchar, NULL, 10);
        if ((pchar = fh_xml_get_value(xml_line, "start_sector")))
            fh_cmd->erase.start_sector = strtoul(pchar, NULL, 10);
        return 0;
    }
    else if (!strncmp(xml_line, "<program ", strlen("<program ")))
    {
        if ((pchar = fh_xml_get_value(xml_line, "filename")))
        {
            if (strlen(pchar) == 0) { //The filename is empty.
                LogInfo("finename is empty\n");
                return -1;
            }
            strncpy(fh_cmd->program.filename, pchar, MAX_PATH_LEN);
        }
        fh_cmd->program.type = "program";
        if ((pchar = fh_xml_get_value(xml_line, "PAGES_PER_BLOCK")))
            fh_cmd->program.PAGES_PER_BLOCK = strtoul(pchar, NULL, 10);
        if ((pchar = fh_xml_get_value(xml_line, "SECTOR_SIZE_IN_BYTES")))
            fh_cmd->program.SECTOR_SIZE_IN_BYTES = strtoul(pchar, NULL, 10);
        if (strstr(xml_line, "label") && strstr(xml_line, "last_sector"))
        {
            if ((pchar = fh_xml_get_value(xml_line, "label")))
                strcpy(fh_cmd->program.label, pchar);
            if ((pchar = fh_xml_get_value(xml_line, "last_sector")))
                fh_cmd->program.last_sector = strtoul(pchar, NULL, 10);
        }
        if ((pchar = fh_xml_get_value(xml_line, "num_partition_sectors"))){
            fh_cmd->program.num_partition_sectors = strtoul(pchar, NULL, 10);
            fh_cmd->program.erase_num_partition_sectors = fh_cmd->program.num_partition_sectors;
        }
        if ((pchar = fh_xml_get_value(xml_line, "physical_partition_number")))
            fh_cmd->program.physical_partition_number = strtoul(pchar, NULL, 10);
        if ((pchar = fh_xml_get_value(xml_line, "start_sector")))
            fh_cmd->program.start_sector = strtoul(pchar, NULL, 10);

        return 0;
    }
    else if (!strncmp(xml_line, "<response ", strlen("<response ")))
    {
        fh_cmd->response.type = "response";
        pchar = fh_xml_get_value(xml_line, "value");
        if (pchar) {
            if (!strcmp(pchar, "ACK"))
                fh_cmd->response.value =  "ACK";
            else if(!strcmp(pchar, "NAK"))
                fh_cmd->response.value =  "NAK";
            else
                 fh_cmd->response.value =  "OTHER";
        }
        if (strstr(xml_line, "rawmode"))
        {
            pchar = fh_xml_get_value(xml_line, "rawmode");
            if (pchar)
            {
                fh_cmd->response.rawmode = !strcmp(pchar, "true");
            }
        }
        else if (strstr(xml_line, "MaxPayloadSizeToTargetInBytes"))
        {
            pchar = fh_xml_get_value(xml_line, "MaxPayloadSizeToTargetInBytes");
            if (pchar)
            {
                fh_cmd->response.MaxPayloadSizeToTargetInBytes = strtoul(pchar, NULL, 10);
            }
        }
        return 0;
    }
    else if (!strncmp(xml_line, "<log ", strlen("<log ")))
    {
        fh_cmd->program.type = "log";
        return 0;
    }

    error_return();
}

static int fh_parse_xml_file(struct fh_data *fh_data, const char *xml_file)
{
    FILE *fp = fopen(xml_file, "rb");

    LogInfo("start\n");
    if (fp == NULL)
    {
        LogInfo("fopen (%s) failed, errno: %d (%s)\n", xml_file, errno, strerror(errno));
        error_return();
    }

    while (fgets(fh_data->xml_buf, fh_data->xml_size, fp))
    {
        char *xml_line = strstr(fh_data->xml_buf, "<");

        if (xml_line && strstr(xml_line, "<!--"))
        {
            if (strstr(xml_line, "-->"))
            {
                if (strstr(xml_line, "/>") < strstr(xml_line, "<!--"))
                    goto __fh_parse_xml_line;

                continue;
            }
            else
            {
                do {
                    fgets(fh_data->xml_buf, fh_data->xml_size, fp);
                    xml_line = fh_data->xml_buf;
                } while(!strstr(xml_line, "-->") && strstr(xml_line, "<!--"));

                continue;
            }
        }

__fh_parse_xml_line:
        if (xml_line && (strstr(xml_line, "<erase ")
            || strstr(xml_line, "<program ") || strstr(xml_line, "vendor=\"fibocom\"")))
        {
            if (!fh_parse_xml_line(xml_line, &fh_data->fh_cmd_table[fh_data->fh_cmd_count]))
                fh_data->fh_cmd_count++;
        }
    }

    fclose(fp);

    if (fh_data->fh_cmd_count == 0) {
        error_return();
    }

    return 0;
}

static int fh_fixup_program_cmd(struct fh_data *fh_data, struct fh_cmd *fh_cmd)
{
    char full_path[MAX_PATH_LEN*2] = {0};
    char *ptmp = NULL;
    FILE *fp = NULL;
    long filesize = 0;
    uint32_t num_partition_sectors = fh_cmd->program.num_partition_sectors;

    // LogInfo("start\n");
    while((ptmp = strchr(fh_cmd->program.filename, '\\')))
    {
        *ptmp = '/';
    }

    snprintf(full_path, sizeof(full_path), "%s/%s", fh_data->image_dir, fh_cmd->program.filename);

    if (access(full_path, R_OK))
    {
        fh_cmd->program.num_partition_sectors = 0;
        LogInfo("fail to access %s, errno: %d (%s)\n", full_path, errno, strerror(errno));
        error_return();
    }

    fp = fopen(full_path, "rb");
    if (fp == NULL)
    {
        fh_cmd->program.num_partition_sectors = 0;
        LogInfo("fail to fopen %s, errno: %d (%s)\n", full_path, errno, strerror(errno));
        error_return();
    }

    fseek(fp, 0, SEEK_END);
    filesize = ftell(fp);
    fclose(fp);

    if (filesize <= 0)
    {
        LogInfo("fail to ftell %s, errno: %d (%s)\n", full_path, errno, strerror(errno));
        fh_cmd->program.num_partition_sectors = 0;
        fh_cmd->program.filesz = 0;
        error_return();
    }
    fh_cmd->program.filesz = filesize;

    fh_cmd->program.num_partition_sectors = filesize/fh_cmd->program.SECTOR_SIZE_IN_BYTES;
    if (filesize%fh_cmd->program.SECTOR_SIZE_IN_BYTES)
        fh_cmd->program.num_partition_sectors += 1;

    if (num_partition_sectors != fh_cmd->program.num_partition_sectors)
    {
        fh_xml_set_value(fh_cmd->xml_original_data, "num_partition_sectors",
            fh_cmd->program.num_partition_sectors);
    }

    return 0;
}

static int fh_recv_cmd(struct fh_data *fh_data, struct fh_cmd *fh_cmd, unsigned timeout)
{
    int ret = 0;
    char *xml_line = NULL;
    char *pend = NULL;

    LogInfo("start\n");
    memset(fh_cmd, 0, sizeof(struct fh_cmd));
    ret = fh_data->pdev->read(fh_data->pdev, fh_data->xml_buf, fh_data->xml_size, 1, timeout);
    if (ret <= 0)
    {
        //error_return();
        LogInfo("No response cmd received\n");
        return -1;
    }
    fh_data->xml_buf[ret] = '\0';

    xml_line = fh_data->xml_buf;
    while (*xml_line)
    {
        xml_line = strstr(xml_line, "<?xml version=");
        if (xml_line == NULL)
        {
            if (fh_cmd->cmd.type == NULL)
            {
                LogInfo("{{{%s}}}", fh_data->xml_buf);
                error_return();
            }
            else
            {
                break;
            }
        }
        xml_line += strlen("<?xml version=");

        xml_line = strstr(xml_line, "<data>\n");
        if (xml_line == NULL)
        {
            LogInfo("{{{%s}}}", fh_data->xml_buf);
            error_return();
        }
        xml_line += strlen("<data>\n");

        if (!strncmp(xml_line, "<response ", strlen("<response ")))
        {
            fh_parse_xml_line(xml_line, fh_cmd);
            pend = strstr(xml_line, "/>");
            if (pend == NULL) {
                error_return();
            }
            pend += 2;
            LogInfo("%.*s\n", (int)(pend -xml_line),  xml_line);
            xml_line = pend + 1;
        }
        else if (!strncmp(xml_line, "<log ", strlen("<log ")))
        {
            if (fh_cmd->cmd.type && strcmp(fh_cmd->cmd.type, "log"))
            {
                LogInfo("{{{%s}}}", fh_data->xml_buf);
                break;
            }
            fh_parse_xml_line(xml_line, fh_cmd);
            pend = strstr(xml_line, "/>");
            if (pend == NULL) {
                error_return();
            }
            pend += 2;
            {
                char *prn = xml_line;
                while (prn < pend)
                {
                    if (*prn == '\r' || *prn == '\n')
                        *prn = '.';
                    prn++;
                }
            }
            LogInfo("%.*s\n", (int)(pend -xml_line), xml_line);
            xml_line = pend + 1;
        }
        else
        {
            LogInfo("unkonw %s", xml_line);
            error_return();
        }
    }

    if (fh_cmd->cmd.type)
        return 0;

    error_return();
}

static int fh_wait_response_cmd(struct fh_data *fh_data, struct fh_cmd *fh_cmd, unsigned timeout)
{
    LogInfo("start\n");
    while (1)
    {
        if (0 != fh_recv_cmd(fh_data, fh_cmd, timeout))
            error_return();

        if (strstr(fh_cmd->cmd.type, "log"))
            continue;

        return 0;
    }

    error_return();
}

static int fh_send_cmd(struct fh_data *fh_data, const struct fh_cmd *fh_cmd)
{
    size_t tx_len = 0;
    char *pstart, *pend;
    char *xml_buf = fh_data->xml_buf;
    unsigned xml_size = fh_data->xml_size;

    LogInfo("start\n");
    memset(xml_buf, 0, xml_size);
    snprintf(xml_buf, xml_size, "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n");
    snprintf(xml_buf + strlen(xml_buf), xml_size, "<data>\n");

    pstart = xml_buf + strlen(xml_buf);

    if (strstr(fh_cmd->cmd.type, "vendor"))
    {
        snprintf(xml_buf + strlen(xml_buf), xml_size, "%s", fh_cmd->vdef.buffer);
    }
    else if (strstr(fh_cmd->cmd.type, "erase")) {
        if (fh_cmd->erase.label[0] && fh_cmd->erase.last_sector)
        {
            snprintf(xml_buf + strlen(xml_buf), xml_size,
                "<erase PAGES_PER_BLOCK=\"%u\" SECTOR_SIZE_IN_BYTES=\"%u\" label=\"%s\" last_sector=\"%u\" num_partition_sectors=\"%u\" physical_partition_number=\"%u\" start_sector=\"%u\" />",
                fh_cmd->erase.PAGES_PER_BLOCK, fh_cmd->erase.SECTOR_SIZE_IN_BYTES,
                fh_cmd->erase.label, fh_cmd->erase.last_sector,
                fh_cmd->erase.num_partition_sectors, fh_cmd->erase.physical_partition_number, fh_cmd->erase.start_sector);
        }
        else if (fh_cmd->erase.PAGES_PER_BLOCK && fh_cmd->erase.SECTOR_SIZE_IN_BYTES)
        {
            snprintf(xml_buf + strlen(xml_buf), xml_size,
                "<erase PAGES_PER_BLOCK=\"%u\" SECTOR_SIZE_IN_BYTES=\"%u\" num_partition_sectors=\"%u\" physical_partition_number=\"%u\" start_sector=\"%u\"    />",
                fh_cmd->erase.PAGES_PER_BLOCK, fh_cmd->erase.SECTOR_SIZE_IN_BYTES,
                fh_cmd->erase.num_partition_sectors, fh_cmd->erase.physical_partition_number, fh_cmd->erase.start_sector);
        }
        else
        {
            snprintf(xml_buf + strlen(xml_buf), xml_size,
               "<erase num_partition_sectors=\"%u\" start_sector=\"%u\"    />",
               fh_cmd->erase.num_partition_sectors, fh_cmd->erase.start_sector);
        }
    }
    else if (strstr(fh_cmd->cmd.type, "program")) {
        if (fh_cmd->cmd.erase_first)
        {
            if (fh_cmd->program.label[0] && fh_cmd->program.last_sector)
            {
                snprintf(xml_buf + strlen(xml_buf), xml_size,
                    "<erase PAGES_PER_BLOCK=\"%u\" SECTOR_SIZE_IN_BYTES=\"%u\" filename=\"%s\" label=\"%s\" last_sector=\"%u\" num_partition_sectors=\"%u\"  physical_partition_number=\"%u\" start_sector=\"%u\" />",
                    fh_cmd->program.PAGES_PER_BLOCK,  fh_cmd->program.SECTOR_SIZE_IN_BYTES,  fh_cmd->program.filename,
                    fh_cmd->program.label, fh_cmd->program.last_sector,
                    fh_cmd->program.erase_num_partition_sectors, fh_cmd->program.physical_partition_number, fh_cmd->program.start_sector);
            }
            else if (fh_cmd->erase.PAGES_PER_BLOCK && fh_cmd->erase.SECTOR_SIZE_IN_BYTES)
            {
                snprintf(xml_buf + strlen(xml_buf), xml_size,
                    "<erase PAGES_PER_BLOCK=\"%u\" SECTOR_SIZE_IN_BYTES=\"%u\" filename=\"%s\" num_partition_sectors=\"%u\"  physical_partition_number=\"%u\" start_sector=\"%u\" />",
                    fh_cmd->program.PAGES_PER_BLOCK,  fh_cmd->program.SECTOR_SIZE_IN_BYTES,  fh_cmd->program.filename,
                    fh_cmd->program.erase_num_partition_sectors, fh_cmd->program.physical_partition_number,  fh_cmd->program.start_sector);
            }
            else
            {
                snprintf(xml_buf + strlen(xml_buf), xml_size,
                    "<erase num_partition_sectors=\"%u\" start_sector=\"%u\"    />",
                    fh_cmd->program.erase_num_partition_sectors, fh_cmd->program.start_sector);
            }
        }
        else if (fh_cmd->program.label[0] && fh_cmd->program.last_sector)
        {
            snprintf(xml_buf + strlen(xml_buf), xml_size,
            "<program PAGES_PER_BLOCK=\"%u\" SECTOR_SIZE_IN_BYTES=\"%u\" filename=\"%s\" label=\"%s\" last_sector=\"%u\" num_partition_sectors=\"%u\"  physical_partition_number=\"%u\" start_sector=\"%u\" />",
            fh_cmd->program.PAGES_PER_BLOCK,  fh_cmd->program.SECTOR_SIZE_IN_BYTES,  fh_cmd->program.filename,
            fh_cmd->program.label, fh_cmd->program.last_sector,
            fh_cmd->program.num_partition_sectors, fh_cmd->program.physical_partition_number,  fh_cmd->program.start_sector);
        }
        else
        {
            snprintf(xml_buf + strlen(xml_buf), xml_size,
            "<program PAGES_PER_BLOCK=\"%u\" SECTOR_SIZE_IN_BYTES=\"%u\" filename=\"%s\" num_partition_sectors=\"%u\"  physical_partition_number=\"%u\" start_sector=\"%u\" />",
            fh_cmd->program.PAGES_PER_BLOCK,  fh_cmd->program.SECTOR_SIZE_IN_BYTES,  fh_cmd->program.filename,
            fh_cmd->program.num_partition_sectors, fh_cmd->program.physical_partition_number,  fh_cmd->program.start_sector);
        }
    }
    else if (strstr(fh_cmd->cmd.type, "configure"))
    {
        snprintf(xml_buf + strlen(xml_buf), xml_size,
            "<configure MemoryName=\"%s\" Verbose=\"%u\" AlwaysValidate=\"%u\" MaxDigestTableSizeInBytes=\"%u\" MaxPayloadSizeToTargetInBytes=\"%u\"  ZlpAwareHost=\"%u\" SkipStorageInit=\"%u\" />",
            fh_cmd->cfg.MemoryName, fh_cmd->cfg.Verbose, fh_cmd->cfg.AlwaysValidate,
            fh_cmd->cfg.MaxDigestTableSizeInBytes,
            fh_cmd->cfg.MaxPayloadSizeToTargetInBytes,
            fh_cmd->cfg.ZlpAwareHost, fh_cmd->cfg.SkipStorageInit);
    }
    else if (strstr(fh_cmd->cmd.type, "reset"))
    {
        snprintf(xml_buf + strlen(xml_buf), xml_size, "<power DelayInSeconds=\"5\" value=\"reset\" />");
    }
    else
    {
        LogInfo("unknown fh_cmd->cmd.type:%s\n", fh_cmd->cmd.type);
        error_return();
    }

    pend = xml_buf + strlen(xml_buf);
    LogInfo("%.*s\n", (int)(pend - pstart),  pstart);

    snprintf(xml_buf + strlen(xml_buf), xml_size, "\n</data>");

    tx_len = fh_data->pdev->write(fh_data->pdev, xml_buf, strlen(xml_buf), strlen(xml_buf), 3000);

    if (tx_len == strlen(xml_buf))
        return 0;

    error_return();
}

static int fh_send_eraseall_cmd(struct fh_data *fh_data)
{
    size_t tx_len = 0;
    char *pstart, *pend;
    char *xml_buf = fh_data->xml_buf;
    unsigned xml_size = fh_data->xml_size;

    LogInfo("start\n");
    memset(xml_buf, 0, xml_size);
    snprintf(xml_buf, xml_size, "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n");
    snprintf(xml_buf + strlen(xml_buf), xml_size, "<data>\n");

    pstart = xml_buf + strlen(xml_buf);
    snprintf(xml_buf + strlen(xml_buf), xml_size, "<erase physical_partition_number=\"0\" start_sector=\"0\" num_partition_sectors=\"4294967295\" />");
    pend = xml_buf + strlen(xml_buf);
    LogInfo("%.*s\n", (int)(pend - pstart),  pstart);

    snprintf(xml_buf + strlen(xml_buf), xml_size, "\n</data>");

    tx_len = fh_data->pdev->write(fh_data->pdev, xml_buf, strlen(xml_buf), strlen(xml_buf), 3000);

    if (tx_len == strlen(xml_buf))
        return 0;

    error_return();
}

static int fh_send_reset_cmd(struct fh_data *fh_data)
{
    struct fh_cmd fh_reset_cmd;

    LogInfo("start\n");
    fh_reset_cmd.cmd.type = "reset";
    return fh_send_cmd(fh_data, &fh_reset_cmd);
}

static int fh_send_cfg_cmd(struct fh_data *fh_data)
{
    struct fh_cmd fh_cfg_cmd;
    struct fh_cmd fh_rx_cmd;

    LogInfo("start\n");

    memset(&fh_cfg_cmd, 0x00, sizeof(fh_cfg_cmd));
    fh_cfg_cmd.cfg.type = "configure";
    fh_cfg_cmd.cfg.Verbose = 0;
    fh_cfg_cmd.cfg.AlwaysValidate = 0;
    fh_cfg_cmd.cfg.SkipStorageInit = 0;
    fh_cfg_cmd.cfg.ZlpAwareHost = fh_data->ZlpAwareHost;

    if(storage_type == STORAGE_TYPE_EMMC)
    {
        fh_cfg_cmd.cfg.MemoryName = "emmc";
        fh_cfg_cmd.cfg.MaxDigestTableSizeInBytes = 8192;
        fh_cfg_cmd.cfg.MaxPayloadSizeToTargetInBytes = 1048576;
        fh_cfg_cmd.cfg.MaxPayloadSizeFromTargetInBytes = 8192;
        fh_cfg_cmd.cfg.MaxPayloadSizeToTargetInByteSupported = 1048576;
    }
    else
    {
        fh_cfg_cmd.cfg.MemoryName = "nand";
        fh_cfg_cmd.cfg.MaxDigestTableSizeInBytes = 2048;
        fh_cfg_cmd.cfg.MaxPayloadSizeToTargetInBytes = 8*1024;
        fh_cfg_cmd.cfg.MaxPayloadSizeFromTargetInBytes = 2*1024;
        fh_cfg_cmd.cfg.MaxPayloadSizeToTargetInByteSupported = 8*1024;
    }

    fh_send_cmd(fh_data, &fh_cfg_cmd);
    if (fh_wait_response_cmd(fh_data, &fh_rx_cmd, 3000) != 0)
        error_return();

    if (!strcmp(fh_rx_cmd.response.value, "NAK") && fh_rx_cmd.response.MaxPayloadSizeToTargetInBytes)
    {
        fh_cfg_cmd.cfg.MaxPayloadSizeToTargetInBytes = fh_rx_cmd.response.MaxPayloadSizeToTargetInBytes;
        fh_cfg_cmd.cfg.MaxPayloadSizeToTargetInByteSupported = fh_rx_cmd.response.MaxPayloadSizeToTargetInBytes;

        fh_send_cmd(fh_data, &fh_cfg_cmd);
        if (fh_wait_response_cmd(fh_data, &fh_rx_cmd, 3000) != 0)
            error_return();
    }

    if (strcmp(fh_rx_cmd.response.value, "ACK") != 0)
        error_return();

    fh_data->MaxPayloadSizeToTargetInBytes = fh_cfg_cmd.cfg.MaxPayloadSizeToTargetInBytes;

    return 0;
}

static int fh_send_rawmode_image(struct fh_data *fh_data, const struct fh_cmd *fh_cmd, unsigned timeout)
{
    char full_path[MAX_PATH_LEN*2] = {0};
    FILE *fp = NULL;
    size_t filesize = 0, cur_size = 0;
    size_t readbytes = 0, writebytes = 0;
    int tick_cnt = -1;
    char *pbuf = NULL, *ptmp = NULL;

    LogInfo("start\n");
    while((ptmp = strchr(fh_cmd->program.filename, '\\'))) {
        *ptmp = '/';
    }

    snprintf(full_path, sizeof(full_path), "%s/%s", fh_data->image_dir, fh_cmd->program.filename);
    fp = fopen(full_path, "rb");
    if (fp == NULL) {
        LogInfo("fail to fopen %s, errno: %d (%s)\n", full_path, errno, strerror(errno));
        error_return();
    }

    fseek(fp, 0, SEEK_END);
    filesize = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    LogInfo("program.filename: %s, filesize:%zd\n", fh_cmd->program.filename, filesize);
    pbuf = malloc(fh_data->MaxPayloadSizeToTargetInBytes * 2);
    if (pbuf == NULL) {
        LogInfo("allocate pbuf failed.\n");
        goto END;
    }

    while (cur_size < filesize) {
        readbytes = fread(pbuf, 1, fh_data->MaxPayloadSizeToTargetInBytes, fp);
        //LogInfo("readbytes: %d\n", readbytes);        
        if (readbytes <= 0) {
            break;
        }

        if (readbytes % fh_cmd->program.SECTOR_SIZE_IN_BYTES)
        {
            memset(pbuf + readbytes, 0, fh_cmd->program.SECTOR_SIZE_IN_BYTES - (readbytes % fh_cmd->program.SECTOR_SIZE_IN_BYTES));
            readbytes += fh_cmd->program.SECTOR_SIZE_IN_BYTES - (readbytes % fh_cmd->program.SECTOR_SIZE_IN_BYTES);
        }

        writebytes = fh_data->pdev->write(fh_data->pdev, pbuf, readbytes, readbytes, timeout);
        if (writebytes != readbytes) {
            LogInfo("qcom_usb_write failed, readbytes:%zd, writebytes:%zd\n", readbytes, writebytes);
            break;
        }
        update_transfer_bytes(writebytes);
        cur_size += readbytes;
        if (!((++tick_cnt) % 10)) {
            printf(".");
            fflush(stdout);
        }
    }

    show_progress();
END:
    if (fp != NULL) {
        fclose(fp);
        fp = NULL;
    }
    if (pbuf != NULL) {
        free(pbuf);
        pbuf = NULL;
    }

    if (cur_size < filesize) {
        LogInfo("cur_size:%zd, filesize:%zd\n", cur_size, filesize);
        return -1;
    }

    LogInfo("send %s finished\n", fh_cmd->program.filename);

    return 0;
}

static int fh_process_erase(struct fh_data *fh_data, const struct fh_cmd *fh_cmd)
{
    struct fh_cmd fh_rx_cmd;
    unsigned timeout = 15000;

    fh_send_cmd(fh_data, fh_cmd);
    if (fh_wait_response_cmd(fh_data, &fh_rx_cmd, timeout) != 0)
    {
        error_return();
    }

    if (strcmp(fh_rx_cmd.response.value, "ACK"))
    {
        error_return();
    }

    return 0;
}

int do_erase_or_program(struct fh_data *fh_data, struct fh_cmd *fh_cmd, int skip_program_sbl)
{
    struct fh_cmd fh_rx_cmd;

    LogInfo("-----------------------------\n");

    // when -r 1 is set, skip to erase or program efs2
    if (fh_data->efs_download_flag
        && (strstr(fh_cmd->program.filename, "cefs.mbn") || strstr(fh_cmd->program.label, "efs2"))) {
        LogInfo("skip cefs.mbn ...\n");
        return 0;
    }

    //erase
    if (strstr(fh_cmd->cmd.type, "erase")) {
        fh_send_cmd(fh_data, fh_cmd);
        if (fh_wait_response_cmd(fh_data, &fh_rx_cmd, 6000) != 0)
            error_return();
        if (strcmp(fh_rx_cmd.response.value, "ACK"))
            error_return();
        return 0;
    }
    else if (strstr(fh_cmd->cmd.type, "program"))
    {
        //erase first
         if (fh_cmd->cmd.erase_first) {
            fh_send_cmd(fh_data, fh_cmd);
            if (fh_wait_response_cmd(fh_data, &fh_rx_cmd, 6000) != 0)
            {
                LogInfo("fh_wait_response_cmd fail\n");
                error_return();
            }
            if (strcmp(fh_rx_cmd.response.value, "ACK"))
            {
                LogInfo("response should be ACK\n");
                error_return();
            }
            fh_cmd->cmd.erase_first = 0;
            LogInfo("-----------------------------\n");
        }

        if (storage_type == STORAGE_TYPE_NAND)
        {
            // skip program sbl
            if (skip_program_sbl
                && (strstr(fh_cmd->program.filename, "sbl") || strstr(fh_cmd->program.label, "sbl"))) {
                return 0;
            }
        }

        if (storage_type == STORAGE_TYPE_EMMC)
        {
            // skip program gpt_main0.bin
            if (skip_program_gptmain0
                && (strstr(fh_cmd->program.filename, "gpt_main0.bin") || strstr(fh_cmd->program.label, "gpt_main0.bin"))) {
                return 0;
            }
        }

        //program
        fh_send_cmd(fh_data, fh_cmd);
        if (fh_wait_response_cmd(fh_data, &fh_rx_cmd, 3000) != 0)
        {
            LogInfo("fh_wait_response_cmd fail\n");
            error_return();
        }
        if (strcmp(fh_rx_cmd.response.value, "ACK"))
        {
            LogInfo("response should be ACK\n");
            error_return();
        }

        if (fh_rx_cmd.response.rawmode != 1)
        {
            LogInfo("response should be rawmode true\n");
            error_return();
        }
        if (fh_send_rawmode_image(fh_data, fh_cmd, 15000))
        {
            LogInfo("fh_send_rawmode_image fail\n");
            error_return();
        }
        if (fh_wait_response_cmd(fh_data, &fh_rx_cmd, 6000) != 0)
        {
            LogInfo("fh_wait_response_cmd fail\n");
            error_return();
        }
        if (strcmp(fh_rx_cmd.response.value, "ACK"))
        {
            LogInfo("response should be ACK\n");
            error_return();
        }
        if (fh_rx_cmd.response.rawmode != 0)
        {
            LogInfo("response should be rawmode false\n");
            error_return();
        }
        return 0;
    }
    else if (strstr(fh_cmd->cmd.type, "vendor"))
    {
        fh_send_cmd(fh_data, fh_cmd);
        if (fh_wait_response_cmd(fh_data, &fh_rx_cmd, 6000) != 0) {
            error_return();
        }
        if (strcmp(fh_rx_cmd.response.value, "ACK")) {
            error_return();
        }
        return 0;
    }

    error_return();
}

int firehose_download_main(const char *image_dir, void *usb_handle, int efs_download_flag, const char *xml_dir)
{
    int ret = -1;
    unsigned x = 0;
    char rawprogram_full_path[MAX_PATH_LEN*2] = {0}, rawprogram_file[MAX_PATH_LEN] = {0};
    struct fh_cmd fh_rx_cmd;
    uint64_t total_filesize = 0;
    unsigned erase_partition_count = 0;
    unsigned max_num_partition_sectors = 0;
    struct fh_data *fh_data = malloc(sizeof(struct fh_data));

    LogInfo("-------------------------------\n");
    LogInfo("start");

    if (fh_data == NULL) {
        LogInfo("malloc fh_data failed.\n");
        error_return();
    }

    memset(fh_data, 0, sizeof(struct fh_data));
    fh_data->image_dir = image_dir;
    fh_data->pdev = (fibo_usbdev_t *)usb_handle;
    fh_data->xml_size = sizeof(fh_data->xml_buf);
    fh_data->ZlpAwareHost = fh_data->pdev->usb_need_zero_package;
    fh_data->efs_download_flag = efs_download_flag; // when -r 1 is set, skip to erase or program efs2

    if(xml_dir[0] == 0)
    {
        if((fibo_find_file_in_dir(image_dir, "rawprogram_nand_p2K_b128K_for_upgrade_l", rawprogram_file) == 0) ||
            (fibo_find_file_in_dir(image_dir, "rawprogram_nand_p4K_b256K_for_upgrade_l", rawprogram_file) == 0))
        {
            LogInfo("find FG132 rawprogram_file.\n");
        }

        else if((fibo_find_file_in_dir(image_dir, "rawprogram_nand_p2K_b128K_for_upgrade_dl", rawprogram_file) == 0) ||
            (fibo_find_file_in_dir(image_dir, "rawprogram_nand_p4K_b256K_for_upgrade_dl", rawprogram_file) == 0))
        {
            LogInfo("find FG131 rawprogram_file.\n");
        }

        else if(fibo_find_file_in_dir(image_dir, "rawprogram_nand_p", rawprogram_file) == 0)
        {
            LogInfo("find rawprogram_file.\n");
        }

        else if(fibo_find_file_in_dir(image_dir, "rawprogram_unsparse0", rawprogram_file) == 0)
        {
            LogInfo("find x72 unsparse xml rawprogram_file.\n");

            if(strcmp(rawprogram_file,"rawprogram_unsparse0.xml") != 0)
            {
                strcpy(rawprogram_file,"rawprogram_unsparse0.xml");
            }
        }

        else if(fibo_find_file_in_dir(image_dir, "rawprogram0", rawprogram_file) == 0)
        {
            LogInfo("find x72 rawprogram_file.\n");

            if(strcmp(rawprogram_file,"rawprogram0.xml") != 0)
            {
                strcpy(rawprogram_file,"rawprogram0.xml");
            }
        }

        else
        {
            LogInfo("can not find right rawprogram_file.\n");
            error_dbg();
            goto END;
        }


        snprintf(rawprogram_full_path, sizeof(rawprogram_full_path), "%s/%s", image_dir, rawprogram_file);
    }
    else
    {
        memcpy(rawprogram_full_path, xml_dir, MAX_PATH_LEN*2);
    }

    //check whether file is exited
    if (access(rawprogram_full_path, R_OK))
    {
        LogInfo("access %s failed, errno: %d (%s)\n", rawprogram_full_path, errno, strerror(errno));
        error_dbg();
        goto END;
    }
    LogInfo("The xml file is %s\n", rawprogram_full_path);

    if(strstr(rawprogram_full_path,"rawprogram_unsparse0") != NULL || strstr(rawprogram_full_path,"rawprogram0") != NULL )
    {
        storage_type = STORAGE_TYPE_EMMC; //emmc
    }

    if (fh_parse_xml_file(fh_data, rawprogram_full_path)) {
        error_dbg();
        goto END;
    }

    for (x=0; x<fh_data->fh_cmd_count; x++)
    {
        struct fh_cmd *fh_cmd = &fh_data->fh_cmd_table[x];

        if (strstr(fh_cmd->cmd.type, "program"))
        {
            fh_fixup_program_cmd(fh_data, fh_cmd);
            if (fh_cmd->program.num_partition_sectors == 0) {
                error_dbg();
                goto END;
            }

            //calc files size
            total_filesize += fh_cmd->program.filesz;
        }
        else if (strstr(fh_cmd->cmd.type, "erase"))
        {
            if(storage_type == STORAGE_TYPE_EMMC)
            {
                if ((fh_cmd->erase.num_partition_sectors + fh_cmd->erase.start_sector) > max_num_partition_sectors)
                {
                    max_num_partition_sectors = (fh_cmd->erase.num_partition_sectors + fh_cmd->erase.start_sector);
                }
                erase_partition_count++;
            }
            else
            {
                erase_partition_count++;
            }
        }
    }

    LogInfo("erase_partition_count: %d\n", erase_partition_count);

    set_transfer_allbytes(total_filesize);
    fh_recv_cmd(fh_data, &fh_rx_cmd, 3000);
    while (fh_recv_cmd(fh_data, &fh_rx_cmd, 1000) == 0);

    LogInfo("-----------------------------\n");
    if (fh_send_cfg_cmd(fh_data)) {
        error_dbg();
        goto END;
    }
    LogInfo("-----------------------------\n");

    //erase all
    if (fh_data->pdev->erase_all_before_download) {
        LogInfo("============================\n");
        LogInfo("erase_all_before_download\n");
        LogInfo("============================\n");
        if(storage_type == STORAGE_TYPE_NAND)
        {
            fh_send_eraseall_cmd(fh_data);
            if (fh_wait_response_cmd(fh_data, &fh_rx_cmd, 6000) != 0){
                error_dbg();
                goto END;
            }
            if (strcmp(fh_rx_cmd.response.value, "ACK")) {
                error_dbg();
                goto END;
            }
            erase_partition_count = 0xFFFFFFFF;
        }
        else
        {
            for (x=0; x<fh_data->fh_cmd_count; x++)
            {
                struct fh_cmd *fh_cmd = &fh_data->fh_cmd_table[x];
                if(!strstr(fh_cmd->cmd.type, "erase"))
                   continue;
                if(fh_cmd->erase.start_sector != 0)
                   continue;

                fh_xml_set_value(fh_cmd->xml_original_data, "num_partition_sectors", max_num_partition_sectors);
                if (fh_cmd->erase.last_sector)
                {
                    fh_xml_set_value(fh_cmd->xml_original_data, "last_sector", max_num_partition_sectors - 1);
                }

                if(fh_process_erase(fh_data, fh_cmd))
                {
                   error_return();
                }
            }
        }
    }

    //erase and program or vendor
    for (x=0; x<fh_data->fh_cmd_count; x++)
    {
        struct fh_cmd *fh_cmd = &fh_data->fh_cmd_table[x];

        if (strstr(fh_cmd->cmd.type, "erase"))
        {
            if (fh_data->pdev->erase_all_before_download) {
                continue;
            }

            if (do_erase_or_program(fh_data, fh_cmd, 0)) {
                error_dbg();
            }
        }
        else if (strstr(fh_cmd->cmd.type, "program"))
        {
            fh_cmd->cmd.erase_first = (erase_partition_count == 0)? 1:0;
            if (do_erase_or_program(fh_data, fh_cmd, 1)) {
                error_dbg();
                goto END;
            }
        }
        else if (strstr(fh_cmd->cmd.type, "vendor"))
        {
            if (do_erase_or_program(fh_data, fh_cmd, 0)) {
                error_dbg();
                goto END;
            }
        }
    }

    if (storage_type == STORAGE_TYPE_NAND)
    {
        //program sbl last
        for (x=0; x<fh_data->fh_cmd_count; x++)
        {
            struct fh_cmd *fh_cmd = &fh_data->fh_cmd_table[x];

            if (strstr(fh_cmd->cmd.type, "program")
                && (strstr(fh_cmd->program.label, "sbl") || strstr(fh_cmd->program.filename, "sbl"))) {
                if (do_erase_or_program(fh_data, fh_cmd, 0)) {
                    error_dbg();
                    goto END;
                }
                goto RESET;
            }
        }
        LogInfo("cannot find sbl for program.\n");
        goto END;
    }

    if (storage_type == STORAGE_TYPE_EMMC)
    {
        //program gpt_main0.bin last
        skip_program_gptmain0 = 0;
        for (x=0; x<fh_data->fh_cmd_count; x++)
        {
            struct fh_cmd *fh_cmd = &fh_data->fh_cmd_table[x];

            if (strstr(fh_cmd->cmd.type, "program")
                && (strstr(fh_cmd->program.label, "gpt_main0.bin") || strstr(fh_cmd->program.filename, "gpt_main0.bin"))) {
                if (do_erase_or_program(fh_data, fh_cmd, 0)) {
                    error_dbg();
                    goto END;
                }
                goto RESET;
            }
        }
        LogInfo("cannot find gpt_main0.bin for program.\n");
        goto END;
    }

RESET:
    //reset device
    fh_send_reset_cmd(fh_data);
    if (fh_wait_response_cmd(fh_data, &fh_rx_cmd, 3000) != 0)
    {
        LogInfo("fh_wait_response_cmd failed\n");
        error_dbg();
        goto END;
    }
    /* MBB0052-473 zhangboxing 2022/05/31 begin */
    while (fh_recv_cmd(fh_data, &fh_rx_cmd, 1000) == 0);
    /* MBB0052-473 zhangboxing 2022/05/31 end */
    ret = 0;
END:
    if (fh_data) {
        free(fh_data);
        fh_data = NULL;
    }

    return ret;
}
