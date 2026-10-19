// Recovered streaming adaptation of stb's zlib decoder (original file: stb_zlib.cpp).
// Proven 0x1302cc-0x131e34: 19 functions, 7,016 bytes, no template tail.
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cassert>
#include "ZlibDecoder.h"
#define CHECK(test, line, signature) ((test) ? (void)0 : __assert_fail(#test, "stb_zlib.cpp", line, signature))
static_assert(sizeof(zhuffman) == 0x7e4 && sizeof(zbuf) == 0x100c, "decoder layouts");
extern int g_lengthBase[31], g_lengthExtra[31], g_distanceBase[32], g_distanceExtra[32];
extern unsigned char g_codeLengthOrder[19];
unsigned char g_defaultLength[288], g_defaultDistance[32];

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
    CHECK(bits <= 16, 77, "int bit_reverse(int, int)");
    return bit_reverse16(v) >> (16 - bits);
}
int zbuild_huffman(zhuffman *z, unsigned char *sizelist, int num)
{
    int i, k = 0;
    int code, next_code[16], sizes[17];
    memset(sizes, 0, sizeof(sizes));
    memset(z->fast, 255, sizeof(z->fast));
    for (i = 0; i < num; ++i) ++sizes[sizelist[i]];
    sizes[0] = 0;
    for (i = 1; i < 16; ++i) CHECK(sizes[i] <= (1 << i), 95, "int zbuild_huffman(zhuffman*, uint8*, int)");
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
        CHECK(z->code_buffer < (1U << z->num_bits), 153, "void fill_bits(zbuf*)");
        z->code_buffer |= zget8(z) << z->num_bits;
        z->num_bits += 8;
    } while (z->num_bits <= 24);
}
unsigned int zreceive(zbuf *z, int n)
{
    unsigned int k;
    if (z->num_bits < n) fill_bits(z);
    k = z->code_buffer & ((1 << n) - 1);
    z->code_buffer >>= n;
    z->num_bits -= n;
    return k;
}
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
    CHECK(z->size[b] == s, 190, "int zhuffman_decode(zbuf*, zhuffman*)");
    a->code_buffer >>= s;
    a->num_bits -= s;
    return z->value[b];
}
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
        CHECK(c >= 0 && c < 19, 232, "int compute_huffman_codes(zbuf*)");
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
            CHECK(c == 18, 244, "int compute_huffman_codes(zbuf*)");
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
        CHECK(a->num_bits == 0, 415, "int parse_header(zbuf*)");
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
