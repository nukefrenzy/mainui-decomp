// Streaming zlib decoder used by the launcher's compressed assets.
#pragma once
#include <stdio.h>
typedef struct zhuffman {
    unsigned short fast[512], firstcode[16];
    int maxcode[17];
    unsigned short firstsymbol[16];
    unsigned char size[288];
    unsigned short value[288];
} zhuffman;
typedef struct zbuf {
    unsigned char *input, *inputEnd;
    FILE *file;
    int inputPos, inputLength, num_bits;
    unsigned int code_buffer;
    signed char *output, *outputStart;
    int unused24, unused28;
    unsigned char *history;
    unsigned int historyPos;
    int final, remaining, blockType, distance;
    zhuffman lengthTable, distanceTable;
} zbuf;
#ifdef __cplusplus
extern "C" {
#endif
zbuf *openZlib(unsigned char *, FILE *, int);
int readZlib(zbuf *, void *, int);
#ifdef __cplusplus
}
int parse_zlib_header(zbuf *);
void *stbi_zlib_decode_malloc(unsigned char *, int, int *);
#endif
