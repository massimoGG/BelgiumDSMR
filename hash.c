
/**
 * A dirty simple hash function I quickly made
 */
unsigned char hash(char *line, int lineLength)
{
    unsigned char hash = 0;

    for (int i = 0; i < lineLength; i++)
    {
        hash = line[i] * i - hash;
    }

    return hash;
}