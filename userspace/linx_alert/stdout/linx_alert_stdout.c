#include <stdio.h>

#include "linx_alert.h"

int linx_alert_output_stdout(linx_alert_message_t *message, linx_alert_config_t *config)
{
    (void)config;

    fprintf(stdout, "%s", message->message);

    fflush(stdout);

    return 0;
}
