#include "../include/uart_passthru_thread.h"

static const struct device *dut_uart = DEVICE_DT_GET(DT_ALIAS(dut_uart));

void uart_passthru_thread(void *a, void *b, void *c){
    char result[64];
    while(1){
        k_sem_take(&uart_pass_sem, K_FOREVER);
        if(uart_pass_cmd.type == CMD_UART_SEND){
            for(char *p = uart_pass_cmd.args[0]; *p; p++){
                uart_poll_out(dut_uart, *p);
            }
            uart_poll_out(dut_uart, '\n');
            snprintf(result, sizeof(result), "OK:UART_SEND\n");
        }
        else {
            char rbuf[50] = {0};
            int i = 0;
            uint8_t ch;
            /* poll up to 50 chars with a 200 ms timeout */
            int64_t deadline = k_uptime_get() + 200;
            while(i < 49 && k_uptime_get() < deadline)
                if(uart_poll_in(dut_uart, &ch) == 0)
                    rbuf[i++] = ch;
            snprintf(result, sizeof(result), "OK:UART_RECV:%s\n", rbuf);
        }
        k_msgq_put(&result_q, result, K_NO_WAIT);
    }
}