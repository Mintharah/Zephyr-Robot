#include <zephyr/drivers/adc.h>
#include "../include/adc_thread.h"

static const struct adc_dt_spec adc_ch =
    ADC_DT_SPEC_GET_BY_IDX(DT_PATH(zephyr_user), 0);

void adc_thread(void *a, void *b, void *c)
{
    adc_channel_setup_dt(&adc_ch);

    int16_t buf;
    struct adc_sequence seq = {
        .buffer      = &buf,
        .buffer_size = sizeof(buf),
    };
    adc_sequence_init_dt(&adc_ch, &seq);

    char result[64];

    while (1) {
        k_sem_take(&adc_sem, K_FOREVER);
        adc_read(adc_ch.dev, &seq);
        int32_t mv = buf;
        adc_raw_to_millivolts_dt(&adc_ch, &mv);
        snprintf(result, sizeof(result), "OK:ADC_READ:%d\n", (int)mv);
        k_msgq_put(&result_q, result, K_NO_WAIT);
    }
}
