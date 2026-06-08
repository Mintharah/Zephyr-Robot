#include <zephyr/kernel.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/drivers/pwm.h>

/* drivers */
static const struct device *uart = DEVICE_DT_GET(DT_ALIAS(dut_uart));

/* dio pins */
#define DIO_COUNT 4
static const struct gpio_dt_spec dios[DIO_COUNT] = {
    GPIO_DT_SPEC_GET_BY_IDX(DT_PATH(zephyr_user), dio_gpios, 0),
    GPIO_DT_SPEC_GET_BY_IDX(DT_PATH(zephyr_user), dio_gpios, 1),
    GPIO_DT_SPEC_GET_BY_IDX(DT_PATH(zephyr_user), dio_gpios, 2),
    GPIO_DT_SPEC_GET_BY_IDX(DT_PATH(zephyr_user), dio_gpios, 3),
};

/* pwm output */
static const struct pwm_dt_spec pwm_out = PWM_DT_SPEC_GET(DT_ALIAS(dut_pwm));

/* uart loopback */
static void uart_cb(const struct device *dev, void *user_data){
    uint8_t c;
    if(!uart_irq_update(dev) || !uart_irq_rx_ready(dev)) return;
    while(uart_fifo_read(dev, &c, 1) == 1){
        uart_poll_out(dev, c);
    }
}

/* gpio loopback thread */
/* dio 0, 2: inputs, dio1,3: outputs */
static void gpio_loopback_thread(void *a, void *b, void *c){
    gpio_pin_configure_dt(&dios[0], GPIO_INPUT);
    gpio_pin_configure_dt(&dios[1], GPIO_INPUT);
    gpio_pin_configure_dt(&dios[2], GPIO_OUTPUT_INACTIVE);
    gpio_pin_configure_dt(&dios[3], GPIO_OUTPUT_INACTIVE);

    while(1){
        gpio_pin_set_dt(&dios[1], gpio_pin_get_dt(&dios[0]));
        gpio_pin_set_dt(&dios[3], gpio_pin_get_dt(&dios[2]));
        k_sleep(K_MSEC(1));
    }
}

K_THREAD_STACK_DEFINE(gpio_stack, 512);
static struct k_thread gpio_tid;

int main(void){
    /*uart loopback*/
    uart_irq_callback_set(uart, uart_cb);
    uart_irq_rx_enable(uart);

    /* pwm output 1kHz 50%, testbench sees ~1.65v */
    pwm_set_dt(&pwm_out, 1000000, 500000);

    /* gpio loopback */
    k_thread_create(&gpio_tid, gpio_stack, 512, gpio_loopback_thread, NULL, NULL, NULL, 4, 0, K_NO_WAIT);

    return 0;
}