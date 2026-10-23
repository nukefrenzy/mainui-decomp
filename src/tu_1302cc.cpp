// tu_1302cc (0x1302cc-0x131e34): a DEFLATE (zlib) decoder, adapted from stb_image's.
//
// The original file was stb_zlib.cpp (its assert messages name it). Unlike stb's, it decodes in
// parts: readZlib fills as much of the caller's buffer as asked and keeps its place, reading the
// compressed data from memory or straight from a file, so the zip reader (tu_14cb24.c) can
// decompress an entry without loading it whole. Function names are stb's where stb has them.
// 19 functions, 7,016 bytes, no template tail.
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cassert>
#include "ZlibDecoder.h"

// Original deferred assertion names, kept separate from ordinary pooled literals.
extern const char zlib_signature_zbuild_huffman[];
extern const char zlib_signature_bit_reverse[];
extern const char zlib_signature_fill_bits[];
extern const char zlib_signature_compute_huffman_codes[];
extern const char zlib_signature_zhuffman_decode[];
extern const char zlib_signature_parse_header[];
#define CHECK(test, line, signature) ((test) ? (void)0 : __assert_fail(#test, "stb_zlib.cpp", line, signature))
static_assert(sizeof(zhuffman) == 0x7e4 && sizeof(zbuf) == 0x100c, "decoder layouts");
extern int g_lengthBase[31], g_lengthExtra[31], g_distanceBase[32], g_distanceExtra[32];
extern unsigned char g_codeLengthOrder[19];
// The DEFLATE tables (RFC 1951): base and extra bits for length codes 257-285 and distance
// codes, and the order in which code-length code lengths are sent (including unused zeros).
int g_lengthBase[31] = {3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31, 35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258, 0, 0}; // 0x17f9c0
int g_lengthExtra[31] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0, 0, 0}; // 0x17fa3c
int g_distanceBase[32] = {1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193, 257, 385, 513, 769, 1025, 1537, 2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577, 0, 0}; // 0x17fab8
int g_distanceExtra[32] = {0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13, 0, 0}; // 0x17fb38
unsigned char g_codeLengthOrder[19] = {16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15}; // 0x17fbb8

// Code lengths of the fixed Huffman codes (block type 1), built on first use.
unsigned char g_defaultLength[288], g_defaultDistance[32];

// Corrupt data: print the reason and exit the program (stb returns an error instead).
int zerror(const char *message)
{
    printf("GNZLIB Failure %s\n", message);
    exit(1);
}
int bit_reverse16(int n)
{
    n = ((n & 0xaaaa) >> 1) | ((n & 0x5555) << 1);
    n = ((n & 0xcccc) >> 2) | ((n & 0x3333) << 2);
    n = ((n & 0xf0f0) >> 4) | ((n & 0x0f0f) << 4);
    n = ((n & 0xff00) >> 8) | ((n & 0x00ff) << 8);
    return n;
}
int bit_reverse(int v, int bits)
{
    CHECK(bits <= 16, 77, zlib_signature_bit_reverse);
    return bit_reverse16(v) >> (16 - bits);
}
// Builds a canonical Huffman decoding table from num code lengths, including a 9-bit fast
// lookup. Returns 1 (malformed lengths exit through zerror).
int zbuild_huffman(zhuffman *z, unsigned char *sizelist, int num)
{
    int i, k = 0;
    int code, next_code[16], sizes[17];
    memset(sizes, 0, sizeof(sizes));
    memset(z->fast, 255, sizeof(z->fast));
    for (i = 0; i < num; ++i) ++sizes[sizelist[i]];
    sizes[0] = 0;
    for (i = 1; i < 16; ++i) CHECK(sizes[i] <= (1 << i), 95, zlib_signature_zbuild_huffman);
    code = 0;
    for (i = 1; i < 16; ++i) {
        next_code[i] = code;
        z->firstcode[i] = (unsigned short)code;
        z->firstsymbol[i] = (unsigned short)k;
        code += sizes[i];
        if (sizes[i] && code - 1 >= (1 << i)) return zerror("bad codelengths");
        z->maxcode[i] = code << (16 - i);
        code <<= 1;
        k += sizes[i];
    }
    z->maxcode[16] = 0x10000;
    for (i = 0; i < num; ++i) {
        int s = sizelist[i];
        if (s) {
            int c = next_code[s] - z->firstcode[s] + z->firstsymbol[s];
            z->size[c] = (unsigned char)s;
            z->value[c] = (unsigned short)i;
            if (s <= 9) {
                for (int j = bit_reverse(next_code[s], s); j < 512; j += (1 << s)) z->fast[j] = (unsigned short)c;
            }
            ++next_code[s];
        }
    }
    return 1;
}
// Next input byte (0 past the end), from memory or the file.
int zget8(zbuf *z)
{
    if (z->inputPos >= z->inputLength) return 0;
    if (z->input) {
        z->inputPos++;
        return *z->input++;
    }
    unsigned char result;
    fread(&result, 1, 1, z->file);
    z->inputPos++;
    return result;
}
void fill_bits(zbuf *z)
{
    do {
        CHECK(z->code_buffer < (1U << z->num_bits), 153, zlib_signature_fill_bits);
        z->code_buffer |= zget8(z) << z->num_bits;
        z->num_bits += 8;
    } while (z->num_bits <= 24);
}
// Takes the next n bits (least significant first).
unsigned int zreceive(zbuf *z, int n)
{
    unsigned int k;
    if (z->num_bits < n) fill_bits(z);
    k = z->code_buffer & ((1 << n) - 1);
    z->code_buffer >>= n;
    z->num_bits -= n;
    return k;
}
// Decodes one Huffman symbol: fast table for short codes, else a search by length; -1 if invalid.
int zhuffman_decode(zbuf *a, zhuffman *z)
{
    int b, s, k;
    if (a->num_bits < 16) fill_bits(a);
    b = z->fast[a->code_buffer & 511];
    if (b < 65535) {
        s = z->size[b];
        a->code_buffer >>= s;
        a->num_bits -= s;
        return z->value[b];
    }
    k = bit_reverse(a->code_buffer, 16);
    for (s = 10; ; ++s) { if (k < z->maxcode[s]) break; }
    if (s == 16) return -1;
    b = (k >> (16 - s)) - z->firstcode[s] + z->firstsymbol[s];
    CHECK(z->size[b] == s, 190, zlib_signature_zhuffman_decode);
    a->code_buffer >>= s;
    a->num_bits -= s;
    return z->value[b];
}
// Reads a dynamic block's code tables (block type 2) into lengthTable and distanceTable.
int compute_huffman_codes(zbuf *a)
{
    zhuffman z;
    unsigned char lencodes[286 + 32 + 137];
    unsigned char codelength_sizes[19];
    int i, n, hlit, hdist, hclen;
    hlit = zreceive(a, 5) + 257;
    hdist = zreceive(a, 5) + 1;
    hclen = zreceive(a, 4) + 4;
    memset(codelength_sizes, 0, sizeof(codelength_sizes));
    for (i = 0; i < hclen; ++i) {
        int s = zreceive(a, 3);
        codelength_sizes[g_codeLengthOrder[i]] = (unsigned char)s;
    }
    if (!zbuild_huffman(&z, codelength_sizes, 19)) return 0;
    n = 0;
    while (n < hlit + hdist) {
        int c = zhuffman_decode(a, &z);
        CHECK(c >= 0 && c < 19, 232, zlib_signature_compute_huffman_codes);
        if (c < 16) lencodes[n++] = (unsigned char)c;
        else if (c == 16) {
            c = zreceive(a, 2) + 3;
            memset(lencodes + n, lencodes[n-1], c);
            n += c;
        } else if (c == 17) {
            c = zreceive(a, 3) + 3;
            memset(lencodes + n, 0, c);
            n += c;
        } else {
            CHECK(c == 18, 244, zlib_signature_compute_huffman_codes);
            c = zreceive(a, 7) + 11;
            memset(lencodes + n, 0, c);
            n += c;
        }
    }
    if (n != hlit + hdist) return zerror("bad codelengths");
    if (!zbuild_huffman(&a->lengthTable, lencodes, hlit)) return 0;
    if (!zbuild_huffman(&a->distanceTable, lencodes + hlit, hdist)) return 0;
    return 1;
}
void init_default_huffman()
{
    int i;
    for (i = 0; i < 144; ++i) g_defaultLength[i] = 8;
    for (; i < 256; ++i) g_defaultLength[i] = 9;
    for (; i < 280; ++i) g_defaultLength[i] = 7;
    for (; i < 288; ++i) g_defaultLength[i] = 8;
    for (i = 0; i < 32; ++i) g_defaultDistance[i] = 5;
}
// The 32 KiB history ring: append a byte; read the byte `distance` bytes back.
void historyWrite(zbuf *a, unsigned char value)
{
    a->history[a->historyPos++] = value;
    if (a->historyPos >= 32768) a->historyPos = 0;
}
unsigned char historyRead(zbuf *a, int distance)
{
    int index = a->historyPos - distance;
    if (index < 0) index += 32768;
    return a->history[index];
}
// Checks the 2-byte zlib header: valid checksum, no preset dictionary, method 8 (deflate).
// (Zip entries have no such header, so the zip reader does not call this.)
int parse_zlib_header(zbuf *a)
{
    int cmf = zget8(a);
    int compression = cmf & 15;
    int flg = zget8(a);
    if ((cmf * 256 + flg) % 31 != 0) return zerror("bad zlib header");
    if (flg & 32) return zerror("no preset dict");
    if (compression != 8) return zerror("bad compression");
    return 1;
}
// Decodes a compressed block until count bytes are written or the block ends (blockType is
// then -1). A back-reference cut short by count is finished on the next call.
int parse_huffman_block(zbuf *a, int count)
{
    int written = 0;
    if (a->remaining > 0) {
        while (a->remaining--) {
            unsigned char value = historyRead(a, a->distance);
            historyWrite(a, value);
            *a->output++ = value;
            written++;
            if (written == count) return count;
        }
    }
    while (true) {
        int z = zhuffman_decode(a, &a->lengthTable);
        if (z < 256) {
            if (z < 0) return zerror("bad huffman code");
            *a->output++ = (unsigned char)z;
            historyWrite(a, (unsigned char)z);
            ++written;
            if (written == count) return count;
        } else {
            int len, dist;
            if (z == 256) { a->blockType = -1; return written; }
            if (a->remaining < 1) {
                z -= 257;
                len = g_lengthBase[z];
                if (g_lengthExtra[z]) len += zreceive(a, g_lengthExtra[z]);
                z = zhuffman_decode(a, &a->distanceTable);
                if (z < 0) return zerror("bad huffman code");
                dist = g_distanceBase[z];
                if (g_distanceExtra[z]) dist += zreceive(a, g_distanceExtra[z]);
                if (dist > 32768) { printf("Bad dist\n"); return 0; }
                a->remaining = len;
                a->distance = dist;
            }
            while (a->remaining--) {
                unsigned char value = historyRead(a, a->distance);
                historyWrite(a, value);
                *a->output++ = value;
                ++written;
                if (written == count) return count;
            }
        }
    }
}
// Copies up to count bytes of a stored block.
int parse_uncompressed_block(zbuf *a, int count)
{
    int written = a->remaining > count ? count : a->remaining;
    int remaining = written;
    unsigned char value;
    while (remaining) {
        if (a->input) {
            historyWrite(a, *a->input);
            *a->output++ = (char)*a->input++;
        } else {
            fread(&value, 1, 1, a->file);
            historyWrite(a, value);
            *a->output++ = (char)value;
        }
        --remaining;
        ++a->inputPos;
    }
    a->remaining -= written;
    if (a->remaining < 1) a->blockType = -1;
    return written;
}
// Starts a block: reads a stored block's length, or sets up the fixed or dynamic code tables.
// The original int function falls through on successful blocks; its caller ignores the result.
// Preserve that behavior (and GCC's return-type warning) for the instruction match.
int parse_header(zbuf *a)
{
    int k, len, nlen;
    unsigned char header[4];
    switch (a->blockType) {
    case 0:
        if (a->num_bits & 7) zreceive(a, a->num_bits & 7);
        k = 0;
        while (a->num_bits > 0) {
            header[k++] = (unsigned char)a->code_buffer;
            a->code_buffer >>= 8;
            a->num_bits -= 8;
        }
        CHECK(a->num_bits == 0, 415, zlib_signature_parse_header);
        while (k < 4) header[k++] = zget8(a);
        len = header[1] * 256 + header[0];
        nlen = header[3] * 256 + header[2];
        printf("len %d nlen %d\n", len, nlen);
        if (nlen != (len ^ 0xffff)) return zerror("zlib corrupt (uncompressed block)");
        if (a->inputPos + len > a->inputLength) return zerror("read past buffer");
        a->remaining = len;
        break;
    case 1:
        // The last distance-code length doubles as the initialized flag.
        if (g_defaultDistance[31] == 0) init_default_huffman();
        if (!zbuild_huffman(&a->lengthTable, g_defaultLength, 288)) return 0;
        if (!zbuild_huffman(&a->distanceTable, g_defaultDistance, 32)) return 0;
        break;
    case 2:
        if (!compute_huffman_codes(a)) return 0;
        break;
    case 3: return 0;
    }
}
// A decoder for size bytes of raw DEFLATE data from memory (input) or the file's current
// position. Free it with free(z->history) and free(z) (zipCloseEntry does the first).
// Original quirk, kept as-is: with neither input nor file it leaks and returns NULL.
zbuf *openZlib(unsigned char *input, FILE *file, int size)
{
    zbuf *a = (zbuf *)malloc(sizeof(zbuf));
    a->history = (unsigned char *)malloc(32768);
    a->historyPos = 0;
    a->final = 0;
    a->blockType = -1;
    a->remaining = 0;
    if (!input && !file) return NULL;
    a->input = input;
    a->file = file;
    a->inputEnd = input + size;
    a->inputPos = 0;
    a->inputLength = size;
    a->num_bits = 0;
    a->code_buffer = 0;
    return a;
}
// Decodes up to size bytes into output, continuing where the last call stopped. Returns the
// count, or -1 once all input is consumed.
int readZlib(zbuf *a, void *output, int size)
{
    int count = 0;
    int written = 0;
    if (!a) return -1;
    if (a->inputPos >= a->inputLength) return -1;
    {
        a->outputStart = (signed char *)output;
        a->output = (signed char *)output;
        int left = size;
        do {
            if (a->blockType == -1) {
                a->final = zreceive(a, 1);
                a->blockType = zreceive(a, 2);
                parse_header(a);
            }
            switch (a->blockType) {
            case 0: count = parse_uncompressed_block(a, left); written += count; break;
            case 1:
            case 2: count = parse_huffman_block(a, left); written += count; break;
            case 3: break;
            }
            left -= count;
        } while (!a->final && written < size);
    }
    return written;
}
// stb's one-shot helper: decodes a zlib stream from memory into a new buffer. Unused here.
// Original bug, kept as-is: every readZlib call writes to the start of the buffer, so only
// the last part survives, although *outsize counts all of it.
void *stbi_zlib_decode_malloc(unsigned char *input, int size, int *outsize)
{
    zbuf *a = openZlib(input, NULL, size);
    int count, total = 0;
    int chunk = size * 20;
    printf("stbi_zlib_decode_malloc %p %d\n", input, chunk);
    void *output = malloc(chunk);
    if (!output || !a) return NULL;
    if (!parse_zlib_header(a)) return NULL;
    while ((count = readZlib(a, output, chunk)) != -1) {
        total += count;
        output = realloc(output, chunk + total);
    }
    output = realloc(output, total);
    *outsize = total;
    return output;
}

// The __PRETTY_FUNCTION__ strings of the asserting functions.
const char zlib_signature_zbuild_huffman[] __attribute__((section(".rodata.function_names"),aligned(4))) = "int zbuild_huffman(zhuffman*, uint8*, int)";
const char zlib_signature_bit_reverse[] __attribute__((section(".rodata.function_names"),aligned(4))) = "int bit_reverse(int, int)";
const char zlib_signature_fill_bits[] __attribute__((section(".rodata.function_names"),aligned(4))) = "void fill_bits(zbuf*)";
const char zlib_signature_compute_huffman_codes[] __attribute__((section(".rodata.function_names"),aligned(4))) = "int compute_huffman_codes(zbuf*)";
const char zlib_signature_zhuffman_decode[] __attribute__((section(".rodata.function_names"),aligned(4))) = "int zhuffman_decode(zbuf*, zhuffman*)";
const char zlib_signature_parse_header[] __attribute__((section(".rodata.function_names"),aligned(4))) = "int parse_header(zbuf*)";
