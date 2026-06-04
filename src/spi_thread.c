#include <zephyr/drivers/spi.h>
#include "../include/spi_thread.h"

static const struct device *spi_dev = DEVICE_DT_GET(DT_ALIAS(dut_spi));
static const struct spi_config spi_config = {
    .frequency = 1000000,
    .operation = SPI_WORD_SET(8) | SPI_TRANSFER_MSB,
    .cs = SPI_CS_CONTROL_INIT(DT_ALIAS(dut_spi))};

void spi_thread(void *a, void *b, void *c)
{
    char result[64];
    while (1)
    {
        k_sem_take(&spi_sem, K_FOREVER);
        /* hex bytes space separated, args[0] */
        uint8_t tx[16] = {0};
        uint8_t rx[16] = {0};
        int n = 0;
        char *tok = strtok(spi_cmd.args[0], " ");
        while (tok && n < 16)
        {
            tx[n++] = strtol(tok, NULL, 16);
            tok = strtok(NULL, " ");
        }
        struct spi_buf tb = {.buf = tx, .len = n};
        struct spi_buf rb = {.buf = rx, .len = n};
        struct spi_buf_set tbs = {.buffers = &tb, .count = 1};
        struct spi_buf_set rbs = {.buffers = &rb, .count = 1};
        spi_transceive(spi_dev, &spi_config, &tbs, &rbs);
        char hex[48] = {0};
        int pos = 0;
        for (int i = 0; i < n; i++)
        {
            pos += snprintf(hex + pos, 3, "%02X", rx[i]);
        }
        snprintf(result, sizeof(result), "OK:SPI_XFER:%s\n", hex);
        k_msgq_put(&result_q, result, K_NO_WAIT);
    }
}