#ifndef INFLUX_H
#define INFLUX_H

#include "http.h"

typedef struct influx_config
{
    struct http_config httpConfig;

    char *bucket;
    char *organization;
    char *token;

} influx_config_t;

struct influx_config influx_init(
    struct http_config *hconfig,
    char *organization, char *bucket, char *token);

int influx_connect(struct influx_config *config);
int influx_authenticate(struct influx_config *config);
int influx_write_DSMR(influx_config_t *config, char *line, int lineLength);

time_t convertTimestamp(char *line);
#endif