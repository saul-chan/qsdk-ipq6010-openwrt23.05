#pragma once

#include <stdint.h>

#define MAX_READ_COUNT    (20) // for arm, this value should be 20
#define MAX_BUFFER_SIZE  (1024 * 80)
#define MAX_TRANSPORT_SIZE  (64)

int32_t open_port(char *devPath, uint32_t baud, uint32_t timeout_ms);
int32_t close_port();
int32_t write_data(uint8_t *data, uint64_t size);
int32_t read_data(uint8_t *buffer, uint64_t size);
int32_t clear_read_buffer();


int32_t init(int32_t fd);
int32_t set_baud(int32_t fd, uint32_t baud);
int32_t set_buffer(int32_t fd);
int32_t set_timeout(int32_t fd, uint32_t timeout_ms);
uint32_t get_baudVal(uint32_t baudrate);
uint32_t set_customer_def_baud(int32_t fd, uint32_t baud);