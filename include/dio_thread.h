#ifndef DIO_THREAD_H
#define DIO_THREAD_H

#include <zephyr/kernel.h>
#include "cmd_parser.h"

extern struct k_sem  dio_sem;
extern cmd_t         dio_cmd;
extern struct k_msgq result_q;

void dio_thread(void *a, void *b, void *c);

#endif
