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
#include "http.h"

struct influx_config influx_init(
    struct http_config *hconfig,
    char *organization, char *bucket, char *token)
{
    return (struct influx_config){
        .httpConfig = *hconfig,
        .organization = organization,
        .bucket = bucket,
        .token = token,
    };
}

/**
 * Connects to InfluxDB
 * @returns 1 (true) in case we successfulyl connected or else 0
 */
int influx_connect(struct influx_config *config)
{
    if (http_connect(&(config->httpConfig)) == -1)
        return 0;
    return 1;
}

/**
 * Performs HTTP GET Query with token and check if we get a 200 response
 * @returns boolean, 0 in case an error occured, 1 in case the connection was authenticated
 */
int influx_authenticate(struct influx_config *config)
{
    int ret = http_get(&(config->httpConfig), "/api/v2/buckets", config->token);
    if (ret == 200)
        return 1;

    return 0;
}

/**
 * Performs HTTP POST Query with token and Line protocol data
 * @returns 0 if unsuccessfull HTTP response, 1 if HTTP 200 OK
 */
int influx_write_DSMR(influx_config_t *config, char *line, int lineLength)
{
    // Prepary URIQuery
    char *query = calloc(128, 1);
    sprintf(query, "bucket=%s&org=%s&precision=s",
            config->bucket, config->organization);

    char *measurement = "meter";
    time_t t = convertTimestamp(line);

    // Prepare Influx body
    char *body = calloc(lineLength * 2, 1); // Allocate enough space
    // +23 to remove the timestamp=,
    sprintf(body, "%s %s %ld", measurement, line + 23, t);
    int body_len = strlen(body);

#if DEBUG
    printLog(__func__, "body_length: %db\n", body_len);
#endif
    int ret = http_post(&(config->httpConfig), "/api/v2/write", query, config->token,
                        body, body_len);

    free(query);
    free(body);

    if (ret >= 200 && ret < 300)
        return 1;

    return -1;
}
/**
 * Converts meter timestamp=YYMMDDhhmmssX to Unix timestamp
 * Assuming the first element is timestamp
 * //250914143330S
 * //25Y 09M 14d 14h 33m 30s
 */
time_t convertTimestamp(char *line)
{
    struct tm t;

    char year[3], month[3], day[3], hour[3], minute[3], second[3];

    // Extract components from the timestamp
    char *ts = line + 10;
    strncpy(year, ts, 2);
    year[2] = '\0';
    strncpy(month, ts + 2, 2);
    month[2] = '\0';
    strncpy(day, ts + 4, 2);
    day[2] = '\0';
    strncpy(hour, ts + 6, 2);
    hour[2] = '\0';
    strncpy(minute, ts + 8, 2);
    minute[2] = '\0';
    strncpy(second, ts + 10, 2);
    second[2] = '\0';

    // Convert year to 2000s
    int yearInt = atoi(year) + 2000 - 1900;

    // Fill timestruct
    t.tm_year = yearInt;
    t.tm_mon = atoi(month) - 1;
    t.tm_mday = atoi(day);
    t.tm_hour = atoi(hour);
    t.tm_min = atoi(minute);
    t.tm_sec = atoi(second);
    t.tm_isdst = 1; // timestamps from meter are never DST
#if DEBUG
    printLog(__func__, "Year %d\tMonth %d\tDay %d\n", t.tm_year, t.tm_mon, t.tm_mday);
    printLog(__func__, "Hour %d\tMinute %d\tSecond %d\n", t.tm_hour, t.tm_min, t.tm_sec);

    printLog(__func__, "time(NULL)=%ld\n", time(NULL));
    printLog(__func__, "mktime(&t)=%ld\n", mktime(&t));
#endif
    return mktime(&t);
}
