/*******************************************************************
 *  CopyRight(C) 2022-2026  Fibocom Wireless Inc
 *******************************************************************
 * FileName : sahara_protocal.c
 * DESCRIPTION : upgrade_tool for USB and PCIE of Fibocom modules
 * Author   : Frank.zhou
 * Date     : 2022.08.22
 *******************************************************************/
#include "misc_usb.h"
#include "sahara_protocol.h"

#define SaharaLogInfo(fmt, args...)  \
    do {    \
        fprintf(stdout, "" fmt, ##args);    \
        fflush(stdout); \
        if (g_dl_logfile_fp) { \
            fprintf(g_dl_logfile_fp, "" fmt, ##args); \
        } \
    } while (0)

#define BSWAP_32(x) (uint32_t)((((x)&0xff000000) >> 24) | (((x)&0x00ff0000) >> 8) | (((x)&0x0000ff00) << 8) | (((x)&0x000000ff) << 24))

#define BSWAP_64(x) ((uint64_t)(((x) & 0x00000000000000FF) << 56) | \
                    ((uint64_t)(((x) & 0x000000000000FF00) << 40) | \
                     ((uint64_t)(((x) & 0x0000000000FF0000) << 24) | \
                      ((uint64_t)(((x) & 0x00000000FF000000) << 8) | \
                       ((uint64_t)(((x) & 0x000000FF00000000) >> 8) | \
                        ((uint64_t)(((x) & 0x0000FF0000000000) >> 24) | \
                         ((uint64_t)(((x) & 0x00FF000000000000) >> 40) | \
                          ((uint64_t)(((x) & 0xFF00000000000000) >> 56)))))))))


#define dbg(log_level, fmt, arg... ) do {LogInfo(fmt "\n", ## arg);} while (0)
#define SAHARA_PRINT_LENGTH 48

extern int fibo_find_file_in_dir(const char *dir, const char *prefix, char *xmlfile);

static int usb_use_usbfs_interface(const void *usb_handle)
{
    return (((fibo_usbdev_t *)usb_handle)->usbdev >= 0);
}

static uint32_t le_uint32(uint32_t v32)
{
    const int is_bigendian = 1;

    if ((*(char *)&is_bigendian) == 0) {
        return BSWAP_32(v32);
    }

    return v32;
}

static uint64_t le_uint64(uint64_t v64)
{
    const int is_bigendian = 1;

    if ((*(char *)&is_bigendian) == 0) {
        return BSWAP_64(v64);
    }

    return v64;
}



void LogInfoHexArray(unsigned char* array, size_t length) {
    size_t i;
    for (i = 0; i < length; ++i) 
    {
        SaharaLogInfo("0x%02X ", array[i]);
    }
    SaharaLogInfo("\r\n");
}

static int sahara_tx_data(void *usb_handle, void *tx_buffer, size_t bytes_to_send)
{
    fibo_usbdev_t *pdev = (fibo_usbdev_t *)usb_handle;
#ifdef SAHARA_DEBUG
    unsigned char tx_buffer_print[SAHARA_PRINT_LENGTH]={0};
    memcpy(tx_buffer_print, (char *)tx_buffer, SAHARA_PRINT_LENGTH);
    if(bytes_to_send <= SAHARA_PRINT_LENGTH)
    {
        LogInfoHexArray(tx_buffer_print, bytes_to_send);
    }
#endif
    return pdev->write(usb_handle, tx_buffer, bytes_to_send, bytes_to_send, 3000);
}


static int sahara_rx_data(void *usb_handle, void *rx_buffer, size_t bytes_to_read)
{
    size_t retval = 0;
    sahara_packet_header *command_packet_header = (sahara_packet_header *)rx_buffer;
    fibo_usbdev_t *pdev = (fibo_usbdev_t *)usb_handle;
#ifdef SAHARA_DEBUG
    uint32_t print_length=0;
    unsigned char rx_buffer_print[SAHARA_PRINT_LENGTH]={0};
    const char *boot_sahara_cmd_id_str[SAHARA_LAST_CMD_ID] = {
        "SAHARA_NO_CMD_ID", //           = 0x00,
        " SAHARA_HELLO_ID", //           = 0x01, // sent from target to host
        "SAHARA_HELLO_RESP_ID", //       = 0x02, // sent from host to target
        "SAHARA_READ_DATA_ID", //        = 0x03, // sent from target to host
        "SAHARA_END_IMAGE_TX_ID", //     = 0x04, // sent from target to host
        "SAHARA_DONE_ID", //             = 0x05, // sent from host to target
        "SAHARA_DONE_RESP_ID", //        = 0x06, // sent from target to host
        "SAHARA_RESET_ID", //            = 0x07, // sent from host to target
        "SAHARA_RESET_RESP_ID", //       = 0x08, // sent from target to host
        "SAHARA_MEMORY_DEBUG_ID", //     = 0x09, // sent from target to host
        "SAHARA_MEMORY_READ_ID", //      = 0x0A, // sent from host to target
        "SAHARA_CMD_READY_ID", //        = 0x0B, // sent from target to host
        "SAHARA_CMD_SWITCH_MODE_ID", //  = 0x0C, // sent from host to target
        "SAHARA_CMD_EXEC_ID", //         = 0x0D, // sent from host to target
        "SAHARA_CMD_EXEC_RESP_ID", //    = 0x0E, // sent from target to host
        "SAHARA_CMD_EXEC_DATA_ID", //    = 0x0F, // sent from host to target
        "SAHARA_64_BITS_MEMORY_DEBUG_ID", //    = 0x10, // sent from target to host
        "SAHARA_64_BITS_MEMORY_READ_ID", //     = 0x11, // sent from host to target
        "SAHARA_64_BITS_READ_DATA_ID", //       = 0x12,
    };
#endif
    if (0 == bytes_to_read) {
        if (usb_use_usbfs_interface(usb_handle)) {
            retval = pdev->read(usb_handle, rx_buffer, SAHARA_RAW_BUFFER_SIZE, 0, 5000);
            if (retval < sizeof(sahara_packet_header)) {
                return 0;
            }
        }
        else {
            retval = pdev->read(usb_handle, rx_buffer, sizeof(sahara_packet_header), 0, 5000);
            if (retval != sizeof(sahara_packet_header)) {
                return 0;
            }
        }

        if (le_uint32(command_packet_header->command) < SAHARA_LAST_CMD_ID) {
#ifdef SAHARA_DEBUG
            dbg(LOG_EVENT, "RECEIVED <-- %s", boot_sahara_cmd_id_str[le_uint32(command_packet_header->command)]);
            memcpy(rx_buffer_print, (char *)rx_buffer, 8);
            dbg(LOG_EVENT, "rx_buffer <--%d, %d\r\n", command_packet_header->command, command_packet_header->length);
            LogInfoHexArray(rx_buffer_print, 8);
#endif
            if (!usb_use_usbfs_interface(usb_handle)) {
                retval += pdev->read(usb_handle, (uint8_t *)rx_buffer + sizeof(sahara_packet_header),
                    le_uint32(command_packet_header->length) - sizeof(sahara_packet_header), 0, 5000);
#ifdef SAHARA_DEBUG
                print_length = (le_uint32(command_packet_header->length) > SAHARA_PRINT_LENGTH) ? SAHARA_PRINT_LENGTH : le_uint32(command_packet_header->length);
                memset(rx_buffer_print, 0, sizeof(rx_buffer_print));
                memcpy(rx_buffer_print, (char *)rx_buffer, print_length);
                LogInfoHexArray(rx_buffer_print, print_length);
#endif
            }

            if (retval != le_uint32(command_packet_header->length)) {
                dbg(LOG_INFO, "Read %zd bytes, Header indicates command %d and packet length %d bytes",
                    retval, le_uint32(command_packet_header->command), le_uint32(command_packet_header->length));
                return 0;
            }
        } else {
            dbg(LOG_EVENT, "RECEIVED <-- SAHARA_CMD_UNKONOW_%d", le_uint32(command_packet_header->command));
            return 0;
        }
    }
    else {
        retval = pdev->read(usb_handle, rx_buffer, bytes_to_read, bytes_to_read, 5000);
    }

    return 1;
}

static int send_reset_command(void *usb_handle, void *tx_buffer)
{
    sahara_packet_reset *sahara_reset = (sahara_packet_reset *)tx_buffer;
    sahara_reset->header.command = le_uint32(SAHARA_RESET_STATE_MACHINE_ID);
    sahara_reset->header.length = le_uint32(sizeof(sahara_packet_reset));

    /* Send the Reset Request */
    dbg(LOG_EVENT, "SENDING --> SAHARA_RESET");
    if (0 == sahara_tx_data(usb_handle, tx_buffer, sizeof(sahara_packet_reset))) {
        dbg(LOG_ERROR, "Sending RESET packet failed");
        return 0;
    }

    return 1;
}

static int send_done_packet(void *usb_handle, void *tx_buffer)
{
    sahara_packet_done *sahara_done = (sahara_packet_done *)tx_buffer;

    sahara_done->header.command = le_uint32(SAHARA_DONE_ID);
    sahara_done->header.length = le_uint32(sizeof(sahara_packet_done));
    /* Send the image data packet */
    dbg(LOG_EVENT, "SENDING --> SAHARA_DONE");
    if (0 == sahara_tx_data (usb_handle, tx_buffer, sizeof(sahara_packet_done))) {
        dbg(LOG_ERROR, "Sending DONE packet failed");
        return 0;
    }

    return 1;
}

static int start_image_transfer(void *usb_handle, void *tx_buffer, const sahara_packet_read_data *sahara_read_data, FILE *file_handle)
{
    int retval = 0;
    uint32_t cur_size = 0, bytes_to_read_next = 0;
    uint32_t DataOffset = le_uint32(sahara_read_data->data_offset);
    uint32_t DataLength = le_uint32(sahara_read_data->data_length);
#ifdef SAHARA_DEBUG
    dbg(LOG_INFO, "image_id:0x%02x, DataOffset:0x%02x, DataLength:0x%02x", le_uint32(sahara_read_data->image_id), DataOffset, DataLength);
#endif
    if (fseek(file_handle, (long)DataOffset, SEEK_SET)) {
        dbg(LOG_INFO, "fseek failed, errno: %d(%s)", errno, strerror(errno));
        return 0;
    }

    while (cur_size < DataLength) {
        bytes_to_read_next = MIN((uint32_t)DataLength - cur_size, SAHARA_RAW_BUFFER_SIZE);
        retval = fread(tx_buffer, 1, bytes_to_read_next, file_handle);
        if (retval < 0) {
            dbg(LOG_ERROR, "read failed: %d(%s)", errno, strerror(errno));
            return 0;
        }

        if ((uint32_t) retval != bytes_to_read_next) {
            dbg(LOG_ERROR, "Read %d bytes, but was asked for 0x%08x bytes", retval, DataLength);
            return 0;
        }

        /*send the image data*/
        if (0 == sahara_tx_data(usb_handle, tx_buffer, bytes_to_read_next)) {
            dbg(LOG_ERROR, "Tx Sahara Image Failed");
            return 0;
        }

        cur_size += bytes_to_read_next;
    }

    return 1;
}

static int start_image_64bit_transfer(void *usb_handle, void *tx_buffer, const sahara_packet_read_data_64bit *sahara_read_64bit_data, FILE *file_handle)
{
    int retval = 0;
    uint64_t cur_size = 0, bytes_to_read_next = 0;
    uint64_t DataOffset = le_uint64(sahara_read_64bit_data->data_offset);
    uint64_t DataLength = le_uint64(sahara_read_64bit_data->data_length);
#ifdef SAHARA_DEBUG
    dbg(LOG_INFO, "image_id:0x%02x, DataOffset:0x%02x, DataLength:0x%02x", le_uint64(sahara_read_64bit_data->image_id), DataOffset, DataLength);
#endif
    if (fseek(file_handle, (uint64_t)DataOffset, SEEK_SET)) {
        dbg(LOG_INFO, "fseek failed, errno: %d(%s)", errno, strerror(errno));
        return 0;
    }

    while (cur_size < DataLength) {
        bytes_to_read_next = MIN((uint64_t)DataLength - cur_size, SAHARA_RAW_BUFFER_SIZE);
        retval = fread(tx_buffer, 1, bytes_to_read_next, file_handle);
        if (retval < 0) {
            dbg(LOG_ERROR, "read failed: %d(%s)", errno, strerror(errno));
            return 0;
        }

        if ((uint64_t) retval != bytes_to_read_next) {
            dbg(LOG_ERROR, "Read %d bytes, but was asked for 0x%08lx bytes", retval, DataLength);
            return 0;
        }

        /*send the image data*/
        if (0 == sahara_tx_data(usb_handle, tx_buffer, bytes_to_read_next)) {
            dbg(LOG_ERROR, "Tx Sahara Image Failed");
            return 0;
        }

        cur_size += bytes_to_read_next;
    }

    return 1;
}


static int sahara_start(void *usb_handle, void *tx_buffer, void *rx_buffer, FILE *file_handle)
{
    uint32_t image_id = 0;

    sahara_packet_header* sahara_cmd = (sahara_packet_header *)rx_buffer;
    sahara_packet_hello *sahara_hello = (sahara_packet_hello *)rx_buffer;
    sahara_packet_read_data *sahara_read_data = (sahara_packet_read_data *)rx_buffer;
    sahara_packet_read_data_64bit *sahara_packet_64bit_read_data = (sahara_packet_read_data_64bit *)rx_buffer;
    sahara_packet_done_resp *sahara_done_resp = (sahara_packet_done_resp *)rx_buffer;
    sahara_packet_end_image_tx *sahara_end_image_tx = (sahara_packet_end_image_tx *)rx_buffer;
    sahara_packet_hello_resp *sahara_hello_resp = (sahara_packet_hello_resp *)tx_buffer;

    dbg(LOG_EVENT, "STATE <-- SAHARA_WAIT_HELLO");
    if (0 == sahara_rx_data(usb_handle, rx_buffer, 0)){
        if (0 == sahara_rx_data(usb_handle, rx_buffer, 0)) {
            sahara_tx_data(usb_handle, tx_buffer, 1);
            if (0 == sahara_rx_data(usb_handle, rx_buffer, 0)) {
                dbg(LOG_ERROR, "wait for hello packet timeout\n");
                return -1;
            }
        }
    }

    //Check if the received command is a hello command
    if (le_uint32(sahara_cmd->command) != SAHARA_HELLO_ID) {
        dbg(LOG_ERROR, "Received a different command: %x while waiting for hello packet", sahara_hello->header.command);
        send_reset_command(usb_handle, rx_buffer);
        return -1;
    }

    // Recieved hello, send the hello response
    sahara_hello_resp->header.command = le_uint32(SAHARA_HELLO_RESP_ID);
    sahara_hello_resp->header.length = le_uint32(sizeof(sahara_packet_hello_resp));
    sahara_hello_resp->version = sahara_hello->version; //SAHARA_VERSION;
    sahara_hello_resp->version_supported = sahara_hello->version_supported; //SAHARA_VERSION_SUPPORTED;
    sahara_hello_resp->status = le_uint32(SAHARA_STATUS_SUCCESS);
    sahara_hello_resp->mode = sahara_hello->mode;
    sahara_hello_resp->reserved0 = le_uint32(1);
    sahara_hello_resp->reserved1 = le_uint32(2);
    sahara_hello_resp->reserved2 = le_uint32(3);
    sahara_hello_resp->reserved3 = le_uint32(4);
    sahara_hello_resp->reserved4 = le_uint32(5);
    sahara_hello_resp->reserved5 = le_uint32(6);

    switch (le_uint32(sahara_hello->mode)) {
        case SAHARA_MODE_IMAGE_TX_PENDING:
            dbg(LOG_EVENT, "RECEIVED <-- SAHARA_MODE_IMAGE_TX_PENDING");
        break;
        case SAHARA_MODE_IMAGE_TX_COMPLETE:
            dbg(LOG_EVENT, "RECEIVED <-- SAHARA_MODE_IMAGE_TX_COMPLETE");
        break;
        case SAHARA_MODE_MEMORY_DEBUG:
            dbg(LOG_EVENT, "RECEIVED <-- SAHARA_MODE_MEMORY_DEBUG");
        break;
        case SAHARA_MODE_COMMAND:
            dbg(LOG_EVENT, "RECEIVED <-- SAHARA_MODE_COMMAND");
        break;
        default:
            dbg(LOG_EVENT, "RECEIVED <-- SAHARA_MODE_0x%x", le_uint32(sahara_hello->mode));
        break;
    }

    if (le_uint32(sahara_hello->mode) != SAHARA_MODE_IMAGE_TX_PENDING) {
        dbg(LOG_ERROR, "ERROR NOT SAHARA_MODE_IMAGE_TX_PENDING");
        sahara_hello_resp->mode = SAHARA_MODE_IMAGE_TX_PENDING;
    }

    /*Send the Hello  Resonse Request*/
    dbg(LOG_EVENT, "SENDING --> SAHARA_HELLO_RESPONSE");
    if (0 == sahara_tx_data(usb_handle, tx_buffer, sizeof(sahara_packet_hello_resp))) {
        dbg(LOG_ERROR, "Tx Sahara Data Failed ");
        return -1;
    }

    while (1) {
#ifdef SAHARA_DEBUG
        dbg(LOG_INFO, "STATE <-- SAHARA_WAIT_COMMAND");
#endif
        if (0 == sahara_rx_data(usb_handle, rx_buffer, 0))
            return -1;

        if (le_uint32(sahara_cmd->command) == SAHARA_READ_DATA_ID) {
            if (0 == start_image_transfer(usb_handle, tx_buffer, sahara_read_data, file_handle)) {
                return -1;
            }
        }
        else if (le_uint32(sahara_cmd->command) == SAHARA_64_BITS_READ_DATA_ID) {
            if (0 == start_image_64bit_transfer(usb_handle, tx_buffer, sahara_packet_64bit_read_data, file_handle)) {
                return -1;
            }
        }
        else if (le_uint32(sahara_cmd->command) == SAHARA_END_IMAGE_TX_ID) {
            dbg(LOG_EVENT, "image_id: %d, status: %d", le_uint32(sahara_end_image_tx->image_id), le_uint32(sahara_end_image_tx->status));
            if (le_uint32(sahara_end_image_tx->status) == SAHARA_STATUS_SUCCESS) {
                image_id = le_uint32(sahara_end_image_tx->image_id);
                send_done_packet (usb_handle, tx_buffer);
                break;
            } else {
                return -1;
            }
        }
        else if (le_uint32(sahara_cmd->command) == SAHARA_HELLO_ID) {
             continue;
        }
        else {
            dbg(LOG_ERROR, "Received an unknown command: %d ", le_uint32(sahara_cmd->command));
            send_reset_command (usb_handle, tx_buffer);
            return -1;
        }
    }

    dbg(LOG_EVENT, "STATE <-- SAHARA_WAIT_DONE_RESP");
    if (0 == sahara_rx_data(usb_handle, rx_buffer, 0)) {
        return -1;
    }

    dbg(LOG_INFO, "image_tx_status: %d", le_uint32(sahara_done_resp->image_tx_status));

    if (SAHARA_MODE_IMAGE_TX_PENDING == le_uint32(sahara_done_resp->image_tx_status)) {
        if (image_id == 13) //prog_nand_firehose_9x07.mbn
            return 0;
        if (image_id == 7) //NPRG9x55.mbn
            return 0;
      }
      else if (SAHARA_MODE_IMAGE_TX_COMPLETE == le_uint32(sahara_done_resp->image_tx_status)) {
        dbg(LOG_EVENT,"Successfully uploaded all images");
        return 0;
    }

    dbg(LOG_ERROR, "Received unrecognized status: %d at SAHARA_WAIT_DONE_RESP state",
                            le_uint32(sahara_done_resp->image_tx_status));

    return -1;
}

int sahara_download_main(const char *image_dir, void *usb_handle, int edl_mode_05c69008)
{
    int retval = -1;
    int count = 0;
    char prog_nand_firehose_filename[MAX_PATH_LEN] = {0};
    char full_path[MAX_PATH_LEN*2] = {0};
    FILE *file_handle = NULL;
    void *tx_buffer = NULL;
    void *rx_buffer = NULL;

    if (edl_mode_05c69008) {
        if(fibo_find_file_in_dir(image_dir, "prog_nand_firehose", prog_nand_firehose_filename) != 0
            && fibo_find_file_in_dir(image_dir, "prog_firehose", prog_nand_firehose_filename) != 0) {
            dbg(LOG_ERROR, "can not find prog_nand_firehose.");
            goto END;
        }
        dbg(LOG_INFO, "prog_nand_firehose_filename: %s", prog_nand_firehose_filename);
        snprintf(full_path, sizeof(full_path), "%s/%s", image_dir, prog_nand_firehose_filename);
    }
    else {
        snprintf(full_path, sizeof(full_path), "%s/..", image_dir);
        if(fibo_find_file_in_dir(full_path, "NPRG9x", prog_nand_firehose_filename) != 0
            && fibo_find_file_in_dir(full_path, "NPRG9x", prog_nand_firehose_filename) != 0) {
            dbg(LOG_ERROR, "can not find NPRG9x mbn.");
            goto END;
        }
        dbg(LOG_INFO, "prog_nand_firehose_filename: %s", prog_nand_firehose_filename);

        snprintf(full_path, sizeof(full_path), "%s/../%s", image_dir, prog_nand_firehose_filename);
    }

sahara_begin:
    count++;
    file_handle = fopen(full_path, "rb");
    if (file_handle == NULL) {
        dbg(LOG_INFO, "%s %d %s errno: %d(%s)", __func__, __LINE__, full_path, errno, strerror(errno));
        goto END;
    }

    rx_buffer = malloc(SAHARA_RAW_BUFFER_SIZE);
    if (NULL == rx_buffer) {
        dbg(LOG_ERROR, "Failed to allocate rx_buffer.");
        goto END;
    }

    tx_buffer = malloc(SAHARA_RAW_BUFFER_SIZE);
    if (NULL == tx_buffer) {
        dbg(LOG_ERROR, "Failed to allocate tx_buffer.");
        goto END;
    }

    memset(rx_buffer, 0, SAHARA_RAW_BUFFER_SIZE);
    memset(tx_buffer, 0, SAHARA_RAW_BUFFER_SIZE);
    if (sahara_start(usb_handle, tx_buffer, rx_buffer, file_handle)) {
        goto END;
    }

    retval = 0;
END:
    if (rx_buffer) {
        free(rx_buffer);
        rx_buffer = NULL;
    }

    if (tx_buffer) {
        free(tx_buffer);
        tx_buffer = NULL;
    }

    if (file_handle) {
        fclose(file_handle);
        file_handle = NULL;
    }

    if (retval) {
        dbg(LOG_ERROR, "sahara send prog_nand_firehose failed.");
        sleep(1);
        if(count < 10)
        {
            dbg(LOG_ERROR, "sahara protocol interaction try again.");
            goto sahara_begin;
        }
    }
    else {
        dbg(LOG_STATUS, "sahara send prog_nand_firehose OK");
    }

    return retval;
}
