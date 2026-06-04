#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include "../include/cmd_parser.h"
#include "../include/dio_thread.h"
#include "../include/adc_thread.h"
#include "../include/pwm_thread.h"
#include "../include/uart_passthru_thread.h"
#include "../include/spi_thread.h"

/* Pending commands, one per thread, single-slot mailbox pattern */
cmd_t dio_cmd;
cmd_t adc_cmd;
cmd_t pwm_cmd;
cmd_t uart_pass_cmd;
cmd_t spi_cmd;

/* Message queues */
K_MSGQ_DEFINE(cmd_q, sizeof(cmd_t), 16, 4); /* 16 commands max, 4 bytes align */
K_MSGQ_DEFINE(result_q, sizeof(char[64]), 16, 4);

/* Semaphores, one per peripheral thread */
K_SEM_DEFINE(dio_sem, 0, 1);
K_SEM_DEFINE(adc_sem, 0, 1);
K_SEM_DEFINE(pwm_sem, 0, 1);
K_SEM_DEFINE(uart_pass_sem, 0, 1);
K_SEM_DEFINE(spi_sem, 0, 1);

/* Stack sizes */
#define STACK_SIZE 1024
K_THREAD_STACK_DEFINE(rx_stack, STACK_SIZE);
K_THREAD_STACK_DEFINE(tx_stack, STACK_SIZE);
K_THREAD_STACK_DEFINE(dio_stack, STACK_SIZE);
K_THREAD_STACK_DEFINE(adc_stack, STACK_SIZE);
K_THREAD_STACK_DEFINE(pwm_stack, STACK_SIZE);
K_THREAD_STACK_DEFINE(spi_stack, STACK_SIZE);
K_THREAD_STACK_DEFINE(dispatch_stack, STACK_SIZE);
K_THREAD_STACK_DEFINE(uart_pass_stack, STACK_SIZE);

static struct k_thread rx_tid, dispatch_tid, dio_tid, adc_tid, pwm_tid, uart_pass_tid, spi_tid, tx_tid;

//forward declaration
void cmd_rx_thread(void *a, void *b, void *c);
void result_tx_thread(void *a, void *b, void *c);
void cmd_dispatch_thread(void *a, void *b, void *c);

int main(void)
{
    k_thread_create(&rx_tid, rx_stack, STACK_SIZE, cmd_rx_thread, NULL, NULL, NULL, 3, 0, K_NO_WAIT);
    k_thread_create(&dispatch_tid, dispatch_stack, STACK_SIZE, cmd_dispatch_thread, NULL, NULL, NULL, 3, 0, K_NO_WAIT);
    k_thread_create(&dio_tid, dio_stack, STACK_SIZE, dio_thread, NULL, NULL, NULL, 4, 0, K_NO_WAIT);
    k_thread_create(&adc_tid, adc_stack, STACK_SIZE, adc_thread, NULL, NULL, NULL, 4, 0, K_NO_WAIT);
    k_thread_create(&pwm_tid, pwm_stack, STACK_SIZE, pwm_thread, NULL, NULL, NULL, 4, 0, K_NO_WAIT);
    k_thread_create(&uart_pass_tid, uart_pass_stack, STACK_SIZE, uart_passthru_thread, NULL, NULL, NULL, 4, 0, K_NO_WAIT);
    k_thread_create(&spi_tid, spi_stack, STACK_SIZE, spi_thread, NULL, NULL, NULL, 4, 0, K_NO_WAIT);
    k_thread_create(&tx_tid, tx_stack, STACK_SIZE, result_tx_thread, NULL, NULL, NULL, 3, 0, K_NO_WAIT);
    return 0;
}

/* UART RX interrupt driven */
static const struct device *host_uart = DEVICE_DT_GET(DT_ALIAS(tb_uart));

static char rx_line[128];
static int rx_pos;

static void uart_cb(const struct device *dev, void *user_data)
{
    uint8_t c;
    if (!uart_irq_update(dev) || !uart_irq_rx_ready(dev))
        return;
    while (uart_fifo_read(dev, &c, 1) == 1)
    {
        if (c == '\n')
        {
            rx_line[rx_pos] = '\0';
            /*hand off to rx thread */
            cmd_t cmd;
            if (cmd_parse(rx_line, &cmd) == 0)
                k_msgq_put(&cmd_q, &cmd, K_NO_WAIT);
            rx_pos = 0;
        }
        else if (rx_pos < sizeof(rx_line) - 1)
        {
            rx_line[rx_pos++] = c;
        }
    }
}

void result_tx_thread(void *a, void *b, void *c)
{
    char r[64];
    while (1)
    {
        k_msgq_get(&result_q, &r, K_FOREVER);
        for (char *p = r; *p; p++)
            uart_poll_out(host_uart, *p);
    }
}

void cmd_rx_thread(void *a, void *b, void *c)
{
    uart_irq_callback_set(host_uart, uart_cb);
    uart_irq_rx_enable(host_uart);
    while (1)
        k_sleep(K_FOREVER);
}

/* Dispatch thread */
void cmd_dispatch_thread(void *a, void *b, void *c)
{
    cmd_t cmd;
    while (1)
    {
        k_msgq_get(&cmd_q, &cmd, K_FOREVER);
        switch (cmd.type)
        {
        case CMD_DIO_SET:
        case CMD_DIO_GET:
            dio_cmd = cmd;
            k_sem_give(&dio_sem);
            break;
        case CMD_ADC_READ:
            adc_cmd = cmd;
            k_sem_give(&adc_sem);
            break;
        case CMD_PWM_SET:
        case CMD_PWM_GET:
            pwm_cmd = cmd;
            k_sem_give(&pwm_sem);
            break;
        case CMD_UART_SEND:
        case CMD_UART_RECV:
            uart_pass_cmd = cmd;
            k_sem_give(&uart_pass_sem);
            break;
        case CMD_SPI_XFER:
            spi_cmd = cmd;
            k_sem_give(&spi_sem);
            break;
        default:
        {
            char r[64];
            snprintf(r, sizeof(r), "ERR:UNKNOWN\n");
            k_msgq_put(&result_q, r, K_NO_WAIT);
        }
        }
    }
}