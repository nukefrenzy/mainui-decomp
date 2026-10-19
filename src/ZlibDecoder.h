// Streaming zlib decoder used by the launcher's compressed assets.
#pragma once
#include <cstdio>
struct zhuffman {
    unsigned short fast[512], firstcode[16];
    int maxcode[17];
    unsigned short firstsymbol[16];
    unsigned char size[288];
    unsigned short value[288];
};
struct zbuf {
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
};
zbuf *openZlib(unsigned char *, FILE *, int);
int parse_zlib_header(zbuf *);
int readZlib(zbuf *, void *, int);
void *stbi_zlib_decode_malloc(unsigned char *, int, int *);
