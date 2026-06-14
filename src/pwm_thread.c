#include "../include/pwm_thread.h"
#include <zephyr/drivers/pwm.h>
#include <zephyr/kernel.h>

static const struct pwm_dt_spec pwm_out = PWM_DT_SPEC_GET(DT_ALIAS(pwm_gen));

static const struct pwm_dt_spec pwm_in = PWM_DT_SPEC_GET(DT_ALIAS(pwm_cap));

void pwm_thread(void *a, void *b, void *c) {
  if (!pwm_is_ready_dt(&pwm_out)) {
    /* hang — misconfigured overlay */
    k_sleep(K_FOREVER);
  }

  char result[64];

  while (1) {
    k_sem_take(&pwm_sem, K_FOREVER);

    switch (pwm_cmd.type) {

    case CMD_PWM_SET: {
      /* CMD:PWM_SET:<period_ns>:<pulse_ns>
       * e.g. CMD:PWM_SET:1000000:500000  -> 1 kHz, 50 % */
      uint32_t period_ns = (uint32_t)atoi(pwm_cmd.args[0]);
      uint32_t pulse_ns = (uint32_t)atoi(pwm_cmd.args[1]);

      int rc = pwm_set_dt(&pwm_out, period_ns, pulse_ns);
      if (rc == 0) {
        snprintf(result, sizeof(result), "OK:PWM_SET:%u:%u\n", period_ns,
                 pulse_ns);
      } else {
        snprintf(result, sizeof(result), "ERR:PWM_SET:%d\n", rc);
      }
      break;
    }

    case CMD_PWM_GET: {
      /* CMD:PWM_GET -> OK:PWM_GET:<period_ns>:<pulse_ns> */
      uint64_t period = 0, pulse = 0;
      int rc = pwm_capture_nsec(pwm_in.dev, pwm_in.channel,
                                PWM_CAPTURE_TYPE_BOTH | PWM_CAPTURE_MODE_SINGLE,
                                &period, &pulse, K_MSEC(100));

      if (rc == 0) {
        snprintf(result, sizeof(result), "OK:PWM_GET:%u:%u\n", (uint32_t)period,
                 (uint32_t)pulse);
      } else {
        /* -EAGAIN on timeout (no edges seen), etc. */
        snprintf(result, sizeof(result), "ERR:PWM_GET:%d\n", rc);
      }
      break;
    }

    default:
      snprintf(result, sizeof(result), "ERR:PWM:UNSUPPORTED\n");
      break;
    }

    k_msgq_put(&result_q, result, K_NO_WAIT);
  }
}
