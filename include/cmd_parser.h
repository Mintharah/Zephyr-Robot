#ifndef CMD_PARSER_H
#define CMD_PARSER_H

#include <zephyr/sys/util.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

typedef enum {
    CMD_DIO_SET,
    CMD_DIO_GET,
    CMD_ADC_READ,
    CMD_PWM_SET,
    CMD_PWM_GET,
    CMD_UART_SEND,
    CMD_UART_RECV,
    CMD_SPI_XFER,
    CMD_UNKNOWN
} cmd_type_t;

typedef struct {
    cmd_type_t type;
    char args[5][32];
    int argc;
} cmd_t;

int cmd_parse(const char *line, cmd_t *out);
#endif
