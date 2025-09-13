#ifndef INFLUX_H
#define INFLUX_H

typedef struct influx_config
{
    int sockfd;
    char *remote_host;
    unsigned short remote_port;

    char *db;
    char *bucket;
    char *organization;
    char *token;
} influx_config_t;

void influxInit(struct influx_config *config, char *host, unsigned short port,
                char *db, char *bucket, char *token);
int influxConnect(struct influx_config *config);
int influxAuthenticateAndValidate(struct influx_config *config);
int influxWrite(struct influx_config *config, char *line, int lineLength);

// Helper functions
size_t constructHTTPGetHeader(struct influx_config *config, char *buffer, size_t bufferlen, char *endpoint);
size_t constructHTTPPostHeader(struct influx_config *config, char *buffer, size_t bufferlen, char *endpoint, size_t contentLength);
int sread(int fd, void *buf, size_t nbytes, int timeout);
int checkHTTPCode(char *__restrict__ s);
#endif