#ifndef HTTP_H
#define HTTP_H

struct http_config
{
    int sockfd;
    char *remote_host;
    unsigned short remote_port;
};

struct http_response
{
    char *request_uri;
    char *body;
    char status_code;
    char *status_text;
};

struct http_config http_init(char *host, unsigned short port);
int http_connect(struct http_config *config);
int http_get(struct http_config *config, char *uri, char *token);
int http_post(struct http_config *config, char *uri, char *query, char *token,
              char *post_data, int post_length);

#endif