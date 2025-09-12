#include <stdio.h>  // For printf
#include <stdlib.h> // For exit
#include <errno.h>

#include "common.h"
#include "tty.h"
#include "DSMR.h"

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
    }

    // Cleanup
    closeTTY(ttyfd);

    return EXIT_FAILURE;
}