#include "../include/uart_passthru_thread.h"

#include <zephyr/sys/ring_buffer.h>
#include <string.h>

static const struct device *loop_uart = DEVICE_DT_GET(DT_ALIAS(loop_uart));

#define LOOP_RX_BUF_SIZE 128
RING_BUF_DECLARE(loop_rx_rb, LOOP_RX_BUF_SIZE);

static void loop_uart_rx_cb(const struct device *dev, void *user_data)
{
    (void)user_data;

    if (!uart_irq_update(dev) || !uart_irq_rx_ready(dev))
        return;

    uint8_t c;
    while (uart_fifo_read(dev, &c, 1) == 1)
        ring_buf_put(&loop_rx_rb, &c, 1);
}

void uart_passthru_thread(void *a, void *b, void *c)
{
    /* Register ISR and enable RX interrupts on the loopback UART once at startup */
    uart_irq_callback_user_data_set(loop_uart, loop_uart_rx_cb, NULL);
    uart_irq_rx_enable(loop_uart);

    char result[64];

    while (1)
    {
        k_sem_take(&uart_pass_sem, K_FOREVER);

        if (uart_pass_cmd.type == CMD_UART_SEND)
        {
            /* Flush stale bytes from previous transactions */
            ring_buf_reset(&loop_rx_rb);

            const char *msg = uart_pass_cmd.args[0];
            for (const char *p = msg; *p; p++)
                uart_poll_out(loop_uart, *p);
            uart_poll_out(loop_uart, '\n');

            snprintf(result, sizeof(result), "OK:UART_SEND\n");
            k_msgq_put(&result_q, result, K_NO_WAIT);
        }
        else if (uart_pass_cmd.type == CMD_UART_RECV)
        {
            char rbuf[48] = {0};
            int i = 0;
            int64_t deadline = k_uptime_get() + 300; /* 300 ms window */

            while (i < (int)(sizeof(rbuf) - 1) && k_uptime_get() < deadline)
            {
                uint8_t ch;
                if (ring_buf_get(&loop_rx_rb, &ch, 1) == 1)
                {
                    if (ch == '\n' || ch == '\r')
                    {
                        if (i > 0)
                            break; /* complete line received */
                        continue;  /* skip leading newlines */
                    }
                    rbuf[i++] = (char)ch;
                }
                else
                {
                    k_sleep(K_MSEC(1));
                }
            }

            snprintf(result, sizeof(result), "OK:UART_RECV:%s\n", rbuf);
            k_msgq_put(&result_q, result, K_NO_WAIT);
        }
    }
}
