/**
 * http.c - Handles everything on HTTP layer
 *
 * Usage:
 * struct http_config config = http_init("ADDRESS", 8086);
 * int ret = http_connect(&config);
 *
 */
#include <stdio.h>
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
#include "http.h"

#define HTTP_TIMEOUT 1
#define MAXHEADERSIZE 512
#define MAXBODYSIZE 2048

int checkHTTPCode(char *__restrict__ s);
int sread(int fd, void *buf, size_t nbytes, int timeout);

struct http_config http_init(char *host, unsigned short port)
{
    // size_t slen = strlen(host);
    // config->remote_host = malloc(slen + 1);
    // strncpy(config->remote_host, host, slen);
    // config->remote_host[slen] = 0;

    return (struct http_config){
        .remote_host = host,
        .remote_port = port,
        .sockfd = -1,
    };
}

/**
 * influxConnect connects with the first possible socket to InfluxDB
 * @returns the socket fd of the connection
 */
int http_connect(struct http_config *config)
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

/**
 * @returns int status code
 */
int http_get(struct http_config *config, char *uri, char *token)
{
    char *headers = calloc(MAXHEADERSIZE, 1);

    // Construct the headers
    sprintf(headers, "GET %s HTTP/1.1\r\n"
                     "Host: %s:%d\r\n"
                     "Connection: keep-alive\r\n"
                     "Authorization: Token %s\r\n\r\n",
            uri, config->remote_host, config->remote_port, token);

    // Write
    int nsent = write(config->sockfd, headers, strlen(headers));
    if (nsent <= 0)
        return 0; // error or closed connection

    free(headers);
    char *response = calloc(MAXBODYSIZE, 1);

    // read response
    int read_len = 0;
    do
    {
        read_len = sread(config->sockfd, response + read_len, MAXBODYSIZE, HTTP_TIMEOUT);
        printLog(__func__, "read %d bytes from HTTP", read_len);

        // If response is bigger than buffer
        if (read_len == MAXBODYSIZE)
            response = realloc(response, MAXBODYSIZE * 2); // I dont care about this
    } while (read_len >= MAXBODYSIZE);

    if (read_len <= 0)
        return 0;

    int status_code = checkHTTPCode(response);

    free(response);
    return status_code;
}

/**
 * @returns int status code
 */
int http_post(
    struct http_config *config, char *uri, char *query, char *token,
    char *post_data, int post_length)
{
    char *body = malloc(MAXBODYSIZE);

    // Construct the headers
    sprintf(body, "POST %s?%s HTTP/1.1\r\n"
                  "Host: %s:%d\r\n"
                  "Connection: keep-alive\r\n"
                  "Content-Length: %d\r\n"
                  "Authorization: Token %s\r\n\r\n",
            uri, query, config->remote_host,
            config->remote_port, strlen(post_data), token);

    int body_offset = strlen(body);
    // copy post_data at body_offset
    strncpy(body + body_offset, post_data, post_length);

    // Write
    int nsent = write(config->sockfd, body, body_offset + post_length);
    if (nsent <= 0)
        return 0; // error or closed connection

    // Clear response
    memset(body, 0, MAXBODYSIZE);

    // read response
    int read_len = 0;
    do
    {
        read_len = sread(config->sockfd, body + read_len, MAXBODYSIZE, HTTP_TIMEOUT);

        // If response is bigger than buffer
        if (read_len == MAXBODYSIZE)
            body = realloc(body, MAXBODYSIZE * 2); // I dont care about this
    } while (read_len >= MAXBODYSIZE);

    if (read_len <= 0)
        return 0;

    int status_code = checkHTTPCode(body);

    free(body);
    return status_code;
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

/// @brief Block reads from fd for timeout seconds
/// @param fd
/// @param buf
/// @param nbytes
/// @param timeout
/// @return bytes read
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
        return -1;
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
