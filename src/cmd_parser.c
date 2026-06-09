#include "../include/cmd_parser.h"

static const struct
{
    const char *str;
    cmd_type_t type;
} map[] = {
    {"DIO_SET",   CMD_DIO_SET},
    {"DIO_GET",   CMD_DIO_GET},
    {"ADC_READ",  CMD_ADC_READ},
    {"PWM_SET",   CMD_PWM_SET},
    {"PWM_GET",   CMD_PWM_GET},
    {"UART_SEND", CMD_UART_SEND},
    {"UART_RECV", CMD_UART_RECV},
    {"SPI_XFER",  CMD_SPI_XFER},
};

int cmd_parse(const char *line, cmd_t *out)
{
    char buf[128];
    strncpy(buf, line, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';

    char *tok = strtok(buf, ":");
    if (!tok || strcmp(tok, "CMD") != 0)
        return -1;

    tok = strtok(NULL, ":");
    if (!tok)
        return -1;

    out->type = CMD_UNKNOWN;
    for (size_t i = 0; i < ARRAY_SIZE(map); i++)
    {
        if (strcmp(tok, map[i].str) == 0)
        {
            out->type = map[i].type;
            break;
        }
    }

    out->argc = 0;
    while ((tok = strtok(NULL, ":")) && out->argc < 5)
    {
        strncpy(out->args[out->argc], tok, 31);
        out->args[out->argc][31] = '\0';
        out->argc++;
    }

    return (out->type == CMD_UNKNOWN) ? -1 : 0;
}
