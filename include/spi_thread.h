#ifndef SPI_THREAD_H
#define SPI_THREAD_H

#include <zephyr/kernel.h>
#include "cmd_parser.h"

extern struct k_sem  spi_sem;
extern cmd_t         spi_cmd;
extern struct k_msgq result_q;

void spi_thread(void *a, void *b, void *c);

#endif
