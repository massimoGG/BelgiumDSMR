#include <stdio.h>  // For printf
#include <stdlib.h> // For exit
#include <string.h>
#include <errno.h>

#include "common.h"
#include "tty.h"
#include "DSMR.h"
#include "http.h"
#include "influx.h"

void clearBuffer(char *buf, int n);
int run(int ttyfd, struct influx_config *iconfig);

int main(const int argc, char *argv[])
{
    setupLogs();

    /**
     * TTY Setup
     */
    printLog(__func__, "Finding available TTY");
    int ttyfd = findAndOpenTTYUSB();
    if (ttyfd == -1)
        exit(EXIT_FAILURE);

    // At this point, we found a suitable TTYUSB* and opened it
    // Now setup termios attributes
    printLog(__func__, "Setting up TTY");
    ttyfd = setupTTY(ttyfd);
    if (ttyfd == -1)
        exit(EXIT_FAILURE);

    /**
     * InfluxDB connection setup
     */
    // HTTP setup
    printLog(__func__, "Setting up Influx HTTP connection");
    char *host = getenv("INFLUX_HOST");
    if (host == NULL)
        goto cleanup;

    struct http_config hconfig = http_init(host, 8086);
    int ret = http_connect(&hconfig);
    if (ret == -1)
    {
        printError(__func__, "HTTP connection to Influx failed");
        goto cleanup;
    }

    printLog(__func__, "Connection established");

    // At this point we've got an established HTTP connection
    char *token = getenv("INFLUX_TOKEN");
    char *organisation = getenv("INFLUX_ORG");
    char *bucket = getenv("INFLUX_BUCKET");
    if (token == NULL || organisation == NULL || bucket == NULL)
        goto cleanup;

    struct influx_config iconfig = influx_init(&hconfig, organisation, bucket, token);
    // Now validate connection
    if (!influx_connect(&iconfig))
    {
        printError(__func__, "Couldn't connect to server");
        goto cleanup;
    }

    if (!influx_authenticate(&iconfig))
    {
        printError(__func__, "Couldn't authenticate Influx connection");
        goto cleanup;
    }

    run(ttyfd, &iconfig);

cleanup:
    // Cleanup
    closeTTY(ttyfd);

    return EXIT_FAILURE;
}

int run(int ttyfd, struct influx_config *iconfig)
{
    int ret = 0;

    /**
     * Line protocol handling
     */
    size_t bufferLength = 128;
    char lineBuffer[bufferLength];

    int readBytes;

// line-protocol buffer
#define LINE_BUFFER_SIZE 2048
    char *influxBuffer = malloc(LINE_BUFFER_SIZE);
    char *bufferOffset = influxBuffer;

    // clear influxBuffer
    clearBuffer(influxBuffer, LINE_BUFFER_SIZE);

    int totalOffset = 0, offset = 0;

    for (;;)
    {
        readBytes = readTTY(ttyfd, lineBuffer, bufferLength);
        if (readBytes < 0)
        {
            printErrno(__func__, "readTTY returned a fatal response!");
            // Fatal
            return -1;
        }
        // TODO:  Maybe a function that resets the DSMR if detecting '/FLU5'
        // If it's not the !CRC, decode line
        offset = decodeLine(influxBuffer + totalOffset, lineBuffer, readBytes);
        totalOffset += offset;

        if (lineBuffer[0] == '!')
        {
            // If line contains the !CRC -> send to Influx
            // Remove last comma
            influxBuffer[totalOffset - 1] = 0;

            // printLog(__func__, "Encoded DSMR: '%s'", influxBuffer);

            ret = influx_write_DSMR(iconfig, influxBuffer, totalOffset);
            if (!ret)
                printError(__func__, "Writing data to InfluxDB failed: (%dbytes) '%s'", totalOffset, influxBuffer);
            // TODO: After x amount of failures, exit with failure?

            // clear influxBuffer
            clearBuffer(influxBuffer, LINE_BUFFER_SIZE);
            totalOffset = 0;
        }
    }
}

void clearBuffer(char *buf, int n)
{
    for (int i = 0; i < n; i++)
        buf[i] = 0;
}