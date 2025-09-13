#include <stdio.h>  // For printf
#include <stdlib.h> // For exit
#include <errno.h>

#include "common.h"
#include "tty.h"
#include "DSMR.h"
#include "influx.h"

int main(const int argc, char *argv[])
{
    setupLogs();

    int ttyfd = findAndOpenTTYUSB();
    if (ttyfd == -1)
        exit(EXIT_FAILURE);

    // At this point, we found a suitable TTYUSB* and opened it
    // Now setup termios attributes
    ttyfd = setupTTY(ttyfd);
    if (ttyfd == -1)
        exit(EXIT_FAILURE);

    // Serial Buffer
    size_t bufferLength = 128;
    char lineBuffer[bufferLength];
    struct DSMR lastDSMR;

    /**
     * Should probably do producer-consumer structure.
     * But for now as a prototype.....
     * I'm not going to do that yet
     */
    char *token = getenv("INFLUX_TOKEN");
    char *db = getenv("INFLUX_DB");
    char *host = getenv("INFLUX_HOST");
    char *bucket = getenv("INFLUX_BUCKET");
    unsigned short port = 8086;
    int ret;

    influx_config_t config;
    influxInit(&config, host, port, db, bucket, token);
    ret = influxConnect(&config);

    ret = influxAuthenticateAndValidate(&config);
    if (ret)
        printf("Succesfully authenticated to Influx!\n");
    else
    {
        printf("Couldn't authenticated! >:c\n");
        goto cleanup;
    }

    for (;;)
    {
        int readBytes = readTTY(ttyfd, lineBuffer, bufferLength);
        if (readBytes < 0)
        {
            // Fatal
            break;
        }

        // printf("[%d] %s", readBytes, lineBuffer);
        decodeLine(lineBuffer, readBytes, &lastDSMR);

        // Now push to Influx
        influxWriteDSMR(&config, &lastDSMR);
    }

cleanup:
    // Cleanup
    closeTTY(ttyfd);

    return EXIT_FAILURE;
}