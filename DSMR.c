#include "DSMR.h"
#include "common.h"
#include "hash.h"

/**
 * HashMap to functionpointer
 */
struct hashkeyval
{
    unsigned char hash;
    unsigned char tag;
    unsigned char digitWidth; // Total number of digits
    unsigned char digitPoint; // number of digits after decimal point

    void (*f)(char *line, int lineLength);
};

#define HASHMAPSIZE 26
struct hashkeyval hashmap[HASHMAPSIZE] = {
    {.hash = 193},                                   // 0-0:1.0.0
    {.hash = 49},                                    // 0-0:96.1.1
    {.hash = 249, .digitWidth = 9, .digitPoint = 3}, // 1-0:1.8.1
    {.hash = 1, .digitWidth = 9, .digitPoint = 3},   // 1-0:1.8.2
    {.hash = 253, .digitWidth = 9, .digitPoint = 3}, // 1-0:2.8.1
    {.hash = 5, .digitWidth = 9, .digitPoint = 3},   // 1-0:2.8.2
    {.hash = 250},                                   // 0-0:96.14.0
    {.hash = 235, .digitWidth = 5, .digitPoint = 3}, // 1-0:1.7.0
    {.hash = 239, .digitWidth = 5, .digitPoint = 3}, // 1-0:2.7.0
    {.hash = 242},                                   // 0-0:96.13.0
    {.hash = 86, .digitWidth = 4, .digitPoint = 1},  // 1-0:32.7.0
    {.hash = 78, .digitWidth = 4, .digitPoint = 1},  // 1-0:52.7.0
    {.hash = 70, .digitWidth = 4, .digitPoint = 1},  // 1-0:72.7.0
    {.hash = 81, .digitWidth = 5, .digitPoint = 1},  // 1-0:31.7.0
    {.hash = 73, .digitWidth = 5, .digitPoint = 1},  // 1-0:51.7.0
    {.hash = 65, .digitWidth = 5, .digitPoint = 1},  // 1-0:71.7.0
    {.hash = 85, .digitWidth = 5, .digitPoint = 3},  // 1-0:21.7.0
    {.hash = 77, .digitWidth = 5, .digitPoint = 3},  // 1-0:41.7.0
    {.hash = 69, .digitWidth = 5, .digitPoint = 3},  // 1-0:61.7.0
    {.hash = 90, .digitWidth = 5, .digitPoint = 3},  // 1-0:22.7.0
    {.hash = 82, .digitWidth = 5, .digitPoint = 3},  // 1-0:42.7.0
    {.hash = 74, .digitWidth = 5, .digitPoint = 3},  // 1-0:62.7.0
    {.hash = 56, .digitWidth = 3, .digitPoint = 0},  // 0-1:24.1.0
    {.hash = 217},                                   // 1-0:1.4.0
    {.hash = 229, .digitWidth = 5, .digitPoint = 3}, // 1-0:1.6.0
    {.hash = 50},                                    // 0-0:98.1.0
};

/**
 * processLine parses a given line from DSMR Serial TTY and fills the
 * given DSMR_T
 */
void decodeLine(char *line, int lineLength, struct DSMR *dsmr)
{
    printLog(__func__, "Processing line '%s'", line);

    int OIDKeyIndexEnd = -1, OIDValueIndexEnd = -1;
    unsigned char keyHash = 0;

    decodeOBISHashKeyValue(line, lineLength, &OIDKeyIndexEnd, &OIDValueIndexEnd, &keyHash);
    if (OIDKeyIndexEnd == -1 || OIDValueIndexEnd == -1)
    {
        printError(__func__, "\tCouldn't identify line %s\n", line);
        return;
    }

    int hashIndex = findOBISOIDByHash(keyHash);
    if (hashIndex == -1)
    {
        printError(__func__, "OBIS OID not found!");
        return;
    }

    // Let's print it for debug purposes
    line[OIDKeyIndexEnd + 1] = 0;
    line[OIDValueIndexEnd + 1] = 0;
    printLog(__func__, "OBIS OID Index %d - Key: %s Value: %s", hashIndex, line, line + OIDKeyIndexEnd + 2);

    // Now execute the function pointer to convert the value

    /**
     * Logical names of COSEM objects uses OBIS (object identification system)
     *
     * Raw packet encoding:
     *      / xxx 5 Identification = /FLU 5 253769484_A
     *
     *      Data
     *      !CRC
     * The CRC = CRC16 calculated over the preceding characters
     *  in the data message from / to ! using polynomial,
     *  computed with least significant bit first,
     *  result is a 4 hexadecimal character (MSB first)
     */
}

/**
 * decodeOBISHaskKeyvalue iterates over the line and finds
 *  the indices of the end of the Key and Value;
 *  and calculates the hash of the key
 */
void decodeOBISHashKeyValue(
    char *line,
    int lineLength,
    int *OIDIndexEnd, int *OBISEndValue,
    unsigned char *OIDKeyHash)
{
    // Key
    for (int i = 0; i < lineLength; i++)
    {
        if (line[i] == '(')
        {

            // Key ended one character before
            *OIDIndexEnd = i - 1;
            break;
        }

        // If we're still inside the OID key -> Calculate hash
        *OIDKeyHash = line[i] * i - *OIDKeyHash;
    }

    // Value: Start from 2 characters after 'K(V' = 012
    for (int i = *OIDIndexEnd + 2; i < lineLength; i++)
    {
        // Rather than ')' to remove the unit
        if (line[i] == '*')
        {
            *OBISEndValue = i - 1;
            break;
        }
    }
}

/**
 * findOBISOIDByHash returns
 * @returns the index of the OID if the hash was found or -1
 */
int findOBISOIDByHash(unsigned char hash)
{
    for (int i = 0; i < HASHMAPSIZE; i++)
    {
        if (hashmap[i].hash == hash)
        {
            return i;
        }
    }
    return -1;
}
