#ifndef ADC_THREAD_H
#define ADC_THREAD_H

#include <zephyr/kernel.h>
#include "cmd_parser.h"

extern struct k_sem  adc_sem;
extern cmd_t         adc_cmd;
extern struct k_msgq result_q;

void adc_thread(void *a, void *b, void *c);

#endif
