#include <stddef.h>

void *memcpy(void *destination, const void *source, size_t length)
{
    unsigned char *dst = destination;
    const unsigned char *src = source;

    while (length-- != 0) *dst++ = *src++;
    return destination;
}

void *memset(void *destination, int value, size_t length)
{
    unsigned char *dst = destination;

    while (length-- != 0) *dst++ = (unsigned char)value;
    return destination;
}

int memcmp(const void *left, const void *right, size_t length)
{
    const unsigned char *a = left;
    const unsigned char *b = right;

    while (length-- != 0) {
        if (*a != *b) return *a < *b ? -1 : 1;
        ++a;
        ++b;
    }
    return 0;
}
