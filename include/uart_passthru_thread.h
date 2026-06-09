#ifndef UART_PASSTHRU_THREAD_H
#define UART_PASSTHRU_THREAD_H

#include <zephyr/kernel.h>
#include <zephyr/drivers/uart.h>
#include "cmd_parser.h"

extern struct k_sem  uart_pass_sem;
extern cmd_t         uart_pass_cmd;
extern struct k_msgq result_q;

void uart_passthru_thread(void *a, void *b, void *c);
#endif
