/* src/dio_thread.c */
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/devicetree.h>
#include "../include/dio_thread.h"

/* Pull the entire gpio array from the overlay in one macro */
#define ZEPHYR_USER_NODE DT_PATH(zephyr_user)

/* Build a compile-time array of gpio_dt_spec from the dio-gpios property */
static const struct gpio_dt_spec dios[] = {
    DT_FOREACH_PROP_ELEM_SEP(
        ZEPHYR_USER_NODE, dio_gpios,
        GPIO_DT_SPEC_GET_BY_IDX,   /* expands each element */
        (,)                         /* comma separator */
    )
};

#define DIO_PIN_COUNT ARRAY_SIZE(dios)

void dio_thread(void *a, void *b, void *c)
{
    /* Boot all as inputs */
    for (int i = 0; i < DIO_PIN_COUNT; i++) {
        gpio_pin_configure_dt(&dios[i], GPIO_INPUT);
    }

    char result[64];

    while (1) {
        k_sem_take(&dio_sem, K_FOREVER);

        int pin = atoi(dio_cmd.args[0]);
        if (pin < 0 || pin >= DIO_PIN_COUNT) {
            snprintf(result, sizeof(result), "ERR:DIO:BAD_PIN\n");
            k_msgq_put(&result_q, result, K_NO_WAIT);
            continue;
        }

        if (dio_cmd.type == CMD_DIO_SET) {
            int val = atoi(dio_cmd.args[1]);
            gpio_pin_configure_dt(&dios[pin], GPIO_OUTPUT_INACTIVE);
            gpio_pin_set_dt(&dios[pin], val);
            snprintf(result, sizeof(result), "OK:DIO_SET:%d:%d\n", pin, val);
        } else {
            gpio_pin_configure_dt(&dios[pin], GPIO_INPUT);
            int v = gpio_pin_get_dt(&dios[pin]);
            snprintf(result, sizeof(result), "OK:DIO_GET:%d:%d\n", pin, v);
        }

        k_msgq_put(&result_q, result, K_NO_WAIT);
    }
}