/**
 * influx.c - Handles TCP/IP connection to Influx DB
 * and writes the given data using the line protocol.
 *
 * Consists of connecting to Influx
 *
 * Note: Basic HTTP implementation
 * Note: This wouldn't be implemented on microcontrollers.
 */
#include <string.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h> // getaddrinfo()
#include <arpa/inet.h>
#include <unistd.h> // for write close and read

#include <sys/select.h>
#include <time.h> // select

#include "common.h"
#include "influx.h"

void influxInit(struct influx_config *config, char *host, unsigned short port,
                char *db, char *bucket, char *token)
{
    config->remote_port = port;

    size_t slen = strlen(host);
    config->remote_host = malloc(slen + 1);
    strncpy(config->remote_host, host, slen);
    config->remote_host[slen] = 0;

    slen = strlen(db);
    config->db = malloc(slen);
    strncpy(config->db, db, slen + 1);
    config->db[slen] = 0;

    slen = strlen(bucket);
    config->bucket = malloc(slen + 1);
    strncpy(config->bucket, bucket, slen);
    config->bucket[slen] = 0;

    slen = strlen(token);
    config->token = malloc(slen + 1);
    strncpy(config->token, token, slen);
    config->token[slen] = 0;
}

/**
 * influxConnect connects with the first possible socket to InfluxDB
 * @returns the socket fd of the connection
 */
int influxConnect(struct influx_config *config)
{
    // Find suitable socket
    char service[6];
    sprintf(service, "%d", config->remote_port); // I hate this

    struct addrinfo hints = {
                        .ai_family = AF_UNSPEC, // IPv4 or IPv6, whatever
                        .ai_socktype = SOCK_STREAM},
                    *servinfo, *sip;

    int ret;
    if ((ret = getaddrinfo(config->remote_host, service, &hints, &servinfo)) == -1)
    {
        printErrno(__func__, "getaddrinfo failed");
        return -1;
    }

    // Fetch first possible socket from servinfo
    int sockfd = -1;
    for (sip = servinfo; sip != NULL; sip = sip->ai_next)
    {
        if ((sockfd = socket(sip->ai_family, sip->ai_socktype, sip->ai_protocol)) == -1)
        {
            sockfd = -1;
            continue;
        }

        // Now try to connect
        if (connect(sockfd, sip->ai_addr, sip->ai_addrlen) == -1)
        {
            // Convert for debug
            char ip[INET6_ADDRSTRLEN];
            struct sockaddr_in *adr4 = (struct sockaddr_in *)sip->ai_addr;
            void *adr = &(adr4->sin_addr);
            inet_ntop(sip->ai_family, adr, ip, INET6_ADDRSTRLEN);

            printErrno(__func__, "Connection failed to %s:%d\n", ip, ntohs(adr4->sin_port));
            close(sockfd);
            sockfd = -1;
            continue;
        }
        break;
    }

    freeaddrinfo(servinfo);

    if (sockfd == -1)
    {
        printErrno(__func__, "couldn't connect to server");
        return -1;
    }

    config->sockfd = sockfd;

    return sockfd;
}

int influxAuthenticateAndValidate(struct influx_config *config)
{
    // HTTP / 1.1 200 OK
    char HTTPBuffer[2048];
    int len = constructHTTPGetHeader(config, HTTPBuffer, sizeof(HTTPBuffer), "/api/v2/buckets");
    if ((write(config->sockfd, HTTPBuffer, len)) <= 0)
    {
        printErrno(__func__, "couldn't send HTTP GET request");
        return 0;
    }

    // Wait for reply
    int nread = sread(config->sockfd, HTTPBuffer, len, 2);
    if (nread <= 0)
    {
        printErrno(__func__, "An error occured reading the reply");
        return 0;
    }

    // Check if reply is OK or not
    int ret = checkHTTPCode(HTTPBuffer);
    if (ret == 200)
        return 1;
    else
        return 0;
}

int influxWrite(struct influx_config *config, char *line, int lineLength)
{

    char endpoint[512];
    sprintf(endpoint, "/api/v2/write?bucket=%s&org=%s&precision=s", config->bucket, config->organization);

    char HTTPBuffer[2048];
    int len = constructHTTPPostHeader(config, HTTPBuffer, sizeof(HTTPBuffer), endpoint, lineLength);
    if ((write(config->sockfd, HTTPBuffer, len)) <= 0)
    {
        printErrno(__func__, "couldn't send HTTP GET request");
        return 0;
    }

    // Wait for reply
    int nread = sread(config->sockfd, HTTPBuffer, len, 2);
    if (nread <= 0)
    {
        printErrno(__func__, "An error occured reading the reply (%d)", nread);
        return 0;
    }

    // Check if reply is OK or not
    int ret = checkHTTPCode(HTTPBuffer);
    if (ret == 200)
        return 1;
    else
        return 0;
}

size_t constructHTTPGetHeader(struct influx_config *config, char *buffer, size_t bufferlen, char *endpoint)
{
    return sprintf(buffer, "GET %s HTTP/1.1\r\n"
                           "Host: %s:%d\r\n"
                           "User-Agent: DSMR/1.0\r\n"
                           "Authorization: Token %s\r\n\r\n",
                   endpoint, config->remote_host, config->remote_port,
                   config->token);
}

size_t constructHTTPPostHeader(struct influx_config *config, char *buffer, size_t bufferlen, char *endpoint, size_t contentLength)
{
    return sprintf(buffer, "POST %s HTTP/1.1\r\n"
                           "Host: %s:%d\r\n"
                           "User-Agent: influxdb-client-cheader\r\n"
                           "Content-Length: %d\r\n"
                           "Authorization: Token %s\r\n\r\n",
                   endpoint, config->remote_host, config->remote_port,
                   contentLength, config->token);
}

// Helper functions
int sread(int fd, void *buf, size_t nbytes, int timeout)
{
    fd_set set;
    FD_ZERO(&set);
    FD_SET(fd, &set);
    struct timeval tv = {.tv_sec = timeout};

    int ret = select(fd + 1, &set, NULL, NULL, &tv);
    if (ret == -1)
    {
        printErrno(__func__, "select error");
        return 0;
    }
    if (ret == 0)
    {
        printError(__func__, "Timeout!");
        return 0;
    }

    // Read the reply
    int nread = read(fd, buf, nbytes);
    return nread;
}

/**
 * checkHTTPCode parses the given HTTP response string and extracts the HTTP code
 * @returns HTTP code
 */
int checkHTTPCode(char *__restrict__ s)
{
    // HTTP/1.1 401 Unauthorized or HTTP/1.1 200 OK
    int httpcode;
    char *token;
    token = strtok(s, " ");
    token = strtok(NULL, " ");
    httpcode = atoi(token);

    return httpcode;
}