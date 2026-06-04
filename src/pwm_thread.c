#include <zephyr/drivers/pwm.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/pwm.h>
#include "../include/pwm_thread.h"

/* Alias defined in overlay: dut-pwm → pwm_out node → first pwm spec */
static const struct pwm_dt_spec pwm_out =
    PWM_DT_SPEC_GET(DT_ALIAS(dut_pwm));

void pwm_thread(void *a, void *b, void *c)
{
    if (!pwm_is_ready_dt(&pwm_out)) {
        /* hang — misconfigured overlay */
        k_sleep(K_FOREVER);
    }

    char result[64];

    while (1) {
        k_sem_take(&pwm_sem, K_FOREVER);

        if (pwm_cmd.type == CMD_PWM_SET) {
            /* Robot sends: CMD:PWM_SET:<period_ns>:<pulse_ns>
               e.g. CMD:PWM_SET:1000000:500000  → 1 kHz 50% */
            uint32_t period_ns = (uint32_t)atoi(pwm_cmd.args[0]);
            uint32_t pulse_ns  = (uint32_t)atoi(pwm_cmd.args[1]);

            int rc = pwm_set_dt(&pwm_out, period_ns, pulse_ns);
            if (rc == 0) {
                snprintf(result, sizeof(result),
                         "OK:PWM_SET:%u:%u\n", period_ns, pulse_ns);
            } else {
                snprintf(result, sizeof(result), "ERR:PWM_SET:%d\n", rc);
            }
        } else {
            snprintf(result, sizeof(result), "ERR:PWM:UNSUPPORTED\n");
        }

        k_msgq_put(&result_q, result, K_NO_WAIT);
    }
}