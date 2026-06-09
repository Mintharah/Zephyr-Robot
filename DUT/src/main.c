#include <zephyr/kernel.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/pwm.h>

/* UART for loopback from testbench passthrough */
static const struct device *uart = DEVICE_DT_GET(DT_ALIAS(dut_uart));

/* DIO pins: 0,2 are inputs driven by testbench; 1,3 are outputs that mirror them */
#define DIO_COUNT 4
static const struct gpio_dt_spec dios[DIO_COUNT] = {
    GPIO_DT_SPEC_GET_BY_IDX(DT_PATH(zephyr_user), dio_gpios, 0),
    GPIO_DT_SPEC_GET_BY_IDX(DT_PATH(zephyr_user), dio_gpios, 1),
    GPIO_DT_SPEC_GET_BY_IDX(DT_PATH(zephyr_user), dio_gpios, 2),
    GPIO_DT_SPEC_GET_BY_IDX(DT_PATH(zephyr_user), dio_gpios, 3),
};

/* PWM output: 1 kHz 50%, testbench ADC sees ~1.65 V */
static const struct pwm_dt_spec pwm_out = PWM_DT_SPEC_GET(DT_ALIAS(dut_pwm));

/* SPI slave device */
static const struct device *spi_slave = DEVICE_DT_GET(DT_ALIAS(dut_spi));

/* UART loopback: echo every received byte back to the sender */
static void uart_cb(const struct device *dev, void *user_data)
{
    uint8_t c;
    if (!uart_irq_update(dev) || !uart_irq_rx_ready(dev))
        return;
    while (uart_fifo_read(dev, &c, 1) == 1) {
        uart_poll_out(dev, c);
    }
}

/* GPIO loopback thread: DIO0→DIO1, DIO2→DIO3 */
static void gpio_loopback_thread(void *a, void *b, void *c)
{
    gpio_pin_configure_dt(&dios[0], GPIO_INPUT);
    gpio_pin_configure_dt(&dios[1], GPIO_OUTPUT_INACTIVE);
    gpio_pin_configure_dt(&dios[2], GPIO_INPUT);
    gpio_pin_configure_dt(&dios[3], GPIO_OUTPUT_INACTIVE);

    while (1) {
        gpio_pin_set_dt(&dios[1], gpio_pin_get_dt(&dios[0]));
        gpio_pin_set_dt(&dios[3], gpio_pin_get_dt(&dios[2]));
        k_sleep(K_MSEC(1));
    }
}

/*
 * SPI slave loopback thread.
 * Waits for the testbench master to clock in bytes, echoes them back
 * on MISO during the *next* transfer (classic slave echo pattern).
 *
 * NOTE: Zephyr's STM32 SPI driver supports slave mode — enable it
 * in your DUT overlay with  slave;  inside the spi node.
 */
#define SPI_SLAVE_BUF_LEN 16

static uint8_t spi_rx_buf[SPI_SLAVE_BUF_LEN];
static uint8_t spi_tx_buf[SPI_SLAVE_BUF_LEN];

static const struct spi_config spi_slave_cfg = {
    .frequency = 1000000,
    .operation = SPI_WORD_SET(8)      |
                 SPI_TRANSFER_MSB     |
                 SPI_OP_MODE_SLAVE,
};

static void spi_slave_thread(void *a, void *b, void *c)
{
    if (!device_is_ready(spi_slave)) {
        return;
    }

    struct spi_buf rx_buf = { .buf = spi_rx_buf, .len = SPI_SLAVE_BUF_LEN };
    struct spi_buf tx_buf = { .buf = spi_tx_buf, .len = SPI_SLAVE_BUF_LEN };
    struct spi_buf_set rx_set = { .buffers = &rx_buf, .count = 1 };
    struct spi_buf_set tx_set = { .buffers = &tx_buf, .count = 1 };

    while (1) {
        /* Pre-load TX with whatever we last received (echo) */
        memcpy(spi_tx_buf, spi_rx_buf, SPI_SLAVE_BUF_LEN);

        /*
         * spi_transceive() blocks until CS is asserted and
         * SPI_SLAVE_BUF_LEN bytes have been clocked in/out.
         */
        int ret = spi_transceive(spi_slave, &spi_slave_cfg, &tx_set, &rx_set);
        if (ret < 0) {
            /* transceive error — just retry */
            k_sleep(K_MSEC(1));
        }
    }
}

K_THREAD_STACK_DEFINE(gpio_stack,      512);
K_THREAD_STACK_DEFINE(spi_slave_stack, 1024);

static struct k_thread gpio_tid;
static struct k_thread spi_slave_tid;

int main(void)
{
    /* UART loopback */
    uart_irq_callback_set(uart, uart_cb);
    uart_irq_rx_enable(uart);

    /* PWM output 1 kHz 50% */
    pwm_set_dt(&pwm_out, 1000000, 500000);

    /* GPIO loopback thread */
    k_thread_create(&gpio_tid, gpio_stack, K_THREAD_STACK_SIZEOF(gpio_stack),
                    gpio_loopback_thread, NULL, NULL, NULL,
                    4, 0, K_NO_WAIT);

    /* SPI slave loopback thread */
    k_thread_create(&spi_slave_tid, spi_slave_stack,
                    K_THREAD_STACK_SIZEOF(spi_slave_stack),
                    spi_slave_thread, NULL, NULL, NULL,
                    4, 0, K_NO_WAIT);

    return 0;
}