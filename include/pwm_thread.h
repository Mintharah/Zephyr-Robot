#ifndef PWM_THREAD_H
#define PWM_THREAD_H

#include <zephyr/kernel.h>
#include "cmd_parser.h"

extern struct k_sem  pwm_sem;
extern cmd_t         pwm_cmd;
extern struct k_msgq result_q;

void pwm_thread(void *a, void *b, void *c);

#endif
