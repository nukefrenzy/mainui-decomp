// Streaming DEFLATE decoder (ZlibDecoder.cpp), adapted from stb_image's zlib code. The zip reader
// (ZipReader.c) uses it, reading straight from the archive file.
#pragma once
#include <stdio.h>
typedef struct zhuffman {
    unsigned short fast[512], firstcode[16];
    int maxcode[17];
    unsigned short firstsymbol[16];
    unsigned char size[288];
    unsigned short value[288];
} zhuffman;
// Decoder state. Input comes from memory (input) or a file; output goes to the caller's buffer,
// with the last 32 KiB kept in history for back-references, so a stream can be decoded in parts.
typedef struct zbuf {
    unsigned char *input, *inputEnd;
    FILE *file;
    int inputPos, inputLength, num_bits;    // bytes consumed / available; bits in code_buffer
    unsigned int code_buffer;
    signed char *output, *outputStart;
    int unused24, unused28;
    unsigned char *history;                 // 32 KiB ring of recent output
    unsigned int historyPos;
    int final, remaining, blockType, distance;  // last block seen; bytes left in the current
                                                // copy or stored block; 0-2 or -1 between blocks
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
