#include <stdio.h>  // For printf
#include <stdlib.h> // For exit
#include <errno.h>

#include "common.h"
#include "tty.h"
#include "DSMR.h"
#include "influx.h"

int influxSetup(influx_config_t *config);
void influxWriteDSMR(influx_config_t *influxConfig, char *line, int lineLength);
void clearBuffer(char *buf, int n);

int main(const int argc, char *argv[])
{
    setupLogs();

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

    // Serial Buffer
    size_t bufferLength = 128;
    char lineBuffer[bufferLength];

    // Influx
    printLog(__func__, "Setting up Influx connection");
    influx_config_t config;
    int ret = influxSetup(&config);
    if (!ret)
    {
        printError(__func__, "Connection to Influx failed");
        goto cleanup;
    }
    printLog(__func__, "Connected to Influx!");

    //
    int readBytes;

    // line-protocol buffer
    char influxBuffer[2048];
    char *bufferOffset = influxBuffer;

    // clear influxBuffer
    clearBuffer(influxBuffer, sizeof(influxBuffer));

    int totalOffset = 0, offset = 0;

    for (;;)
    {
        int readBytes = readTTY(ttyfd, lineBuffer, bufferLength);
        if (readBytes < 0)
        {
            // Fatal
            break;
        }
        // TODO:  Maybe a function that resets the DSMR if detecting '/FLU5'
        // If it's not the !CRC, decode line
        offset = decodeLine(influxBuffer + totalOffset, lineBuffer, readBytes);
        totalOffset += offset;

        if (lineBuffer[0] == '!')
        {
            // If line contains the !CRC -> send to Influx
            influxWriteDSMR(&config, influxBuffer, totalOffset);

            // clear influxBuffer
            clearBuffer(influxBuffer, sizeof(influxBuffer));
            totalOffset = 0;
        }
    }

cleanup:
    // Cleanup
    closeTTY(ttyfd);

    return EXIT_FAILURE;
}

/**
 * influxSetup does all the nasty stuff
 * @returns boolean
 *
 * Should probably do producer-consumer structure.
 * But for now as a prototype.....
 * I'm not going to do that yet
 */
int influxSetup(influx_config_t *config)
{
    unsigned short port = 8086;
    int ret;

    char *token = getenv("INFLUX_TOKEN");
    if (token == NULL)
        return 0;
    char *db = getenv("INFLUX_DB");
    if (db == NULL)
        return 0;
    char *host = getenv("INFLUX_HOST");
    if (host == NULL)
        return 0;
    char *bucket = getenv("INFLUX_BUCKET");
    if (bucket == NULL)
        return 0;

    influxInit(config, host, port, db, bucket, token);
    ret = influxConnect(config);
    if (ret == -1)
        return 0;

    return influxAuthenticateAndValidate(config);
}

void influxWriteDSMR(influx_config_t *influxConfig, char *line, int lineLength)
{
    // printLog(__func__, "Sending to Influx \nVoltages: L1: %s\tL2: %s\tL3: %s\nCurrents: L1: %s\tL2: %s\tL3: %s\nPEAK: %s\n",
    //  dsmr->InstantaneousVoltageL1, dsmr->InstantaneousVoltageL2, dsmr->InstantaneousVoltageL3,
    //  dsmr->InstantaneousCurrentL1, dsmr->InstantaneousCurrentL2, dsmr->InstantaneousCurrentL3,
    //  dsmr->MaximumDemandRunningMonth);
    printLog(__func__, "influxBuffer (%d) '%s'\n\n\n", lineLength, line);
}

void clearBuffer(char *buf, int n)
{
    for (int i = 0; i < n; i++)
        buf[i] = 0;
}