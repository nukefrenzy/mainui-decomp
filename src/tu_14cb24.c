// tu_14cb24: a small zip reader (0x14cb24-0x14da50): central directory lookup, stored and
// deflated entries (inflate lives at 0x131a68). Used by render_COPYING_2d574.
// Compiled as C (call results are stored straight from r0), so there is no template tail.
// The C units 0x14acfc-0x14f730 are split where .rodata proves a new file: GCC merges identical
// string literals within one file even at -O0, and "w", "r" and "%d" each appear more than once.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdarg.h>
#include <sys/stat.h>

// Byte readers for the zip/gzip loader below.
int readByte(FILE *f)           // 0x14cb24
{
    if (f) {
        int c = fgetc(f);
        if (c != EOF)
            return c;
        return 0;
    }
    return 0;
}

unsigned char readU8(FILE *f)   // 0x14cb78
{
    return readByte(f);
}

void skipBytes(FILE *f, int n)  // 0x14cba4
{
    unsigned char i;
    size_t r;
    char c;
    if (f == NULL)
        return;
    for (i = 0; i < n; i++)
        r = fread(&c, 1, 1, f);
}

unsigned short readU16BE(FILE *f)   // 0x14cc14
{
    int hi = readByte(f);
    return (unsigned short)(hi << 8) + (unsigned short)readByte(f);
}

unsigned int readU32BE(FILE *f)     // 0x14cc64
{
    unsigned int hi = readU16BE(f);
    return (hi << 16) + readU16BE(f);
}

unsigned short readU16LE(FILE *f)   // 0x14cca8
{
    int lo = readByte(f);
    return (unsigned short)(readByte(f) << 8) + (unsigned short)lo;
}

unsigned int readU32LE(FILE *f)     // 0x14ccf8
{
    unsigned int lo = readU16LE(f);
    return (readU16LE(f) << 16) + lo;
}

// Reads little-endian fields described by fmt ('1', '2', '4' bytes; ' ' is ignored) into the
// pointers that follow; a NULL pointer skips the field.
void readFields(FILE *f, const char *fmt, ...)  // 0x14cd40
{
    va_list ap;
    va_start(ap, fmt);
    while (*fmt) {
        switch (*fmt++) {
        case ' ':
            break;
        case '1': {
            unsigned char *p = va_arg(ap, unsigned char *);
            if (p)
                *p = readByte(f);
            else
                skipBytes(f, 1);
            break;
        }
        case '2': {
            unsigned short *p = va_arg(ap, unsigned short *);
            if (p)
                *p = readU16LE(f);
            else
                skipBytes(f, 2);
            break;
        }
        case '4': {
            unsigned int *p = va_arg(ap, unsigned int *);
            if (p)
                *p = readU32LE(f);
            else
                skipBytes(f, 4);
            break;
        }
        default:
            return;
        }
    }
    va_end(ap);
}

typedef struct ZipFile {
    FILE *fp;
    unsigned int cdOffset;      // +0x04 central directory offset
    unsigned int cdSize;        // +0x08
    unsigned int eocdPos;       // +0x0c end-of-central-directory record
    unsigned short entries;     // +0x10
} ZipFile;

// Scans forward for the 4-byte signature sig; returns its file offset or -1.
int findSignature(ZipFile *zip, const unsigned char *sig)  // 0x14cedc
{
    int matched = 0;
    int count = 0;
    size_t total = 0;
    if (zip == NULL || zip->fp == NULL)
        return -1;
    while (!feof(zip->fp)) {
        unsigned char c;
        total += fread(&c, 1, 1, zip->fp);
        if (sig[matched] == c) {
            if (matched == 3) {
                int pos = ftell(zip->fp) - 4;
                return pos;
            }
            matched++;
        } else {
            matched = 0;
        }
        count++;
    }
    return -1;
}

int zipReadCentralDir(ZipFile *zip)     // 0x14cff0
{
    int i = 0;
    unsigned char eocdSig[4] = {'P', 'K', 5, 6};
    unsigned char cdSig[4] = {'P', 'K', 1, 2};
    unsigned short disk;
    if (zip == NULL || zip->fp == NULL)
        return -1;
    zip->eocdPos = 0;
    for (i = 0; i < 0xffff; i += 256) {
        fseek(zip->fp, -22 - i, SEEK_END);
        zip->eocdPos = findSignature(zip, eocdSig);
        if (zip->eocdPos != -1)
            break;
    }
    if (zip->eocdPos == -1) {
        puts("Couldn't find central dir, Corrupted zip");
        return -1;
    }
    readFields(zip->fp, "22 22 44", &disk, NULL, NULL, &zip->entries, &zip->cdSize, &zip->cdOffset);
    if (disk) {
        printf("Multi disk not supported (%d)\n", disk);
        return -1;
    }
    fseek(zip->fp, zip->cdOffset, SEEK_SET);
    if (findSignature(zip, cdSig) != zip->cdOffset) {
        puts("Corrupted zip");
        return -1;
    }
    return 0;
}

// Walks the central directory to the entry called name (or with CRC crc) and seeks to its
// local header.
int zipFindEntry(ZipFile *zip, const char *name, unsigned int crc)  // 0x14d1dc
{
    char *entryName = NULL;
    size_t total = 0;
    fseek(zip->fp, zip->cdOffset, SEEK_SET);
    long start = ftell(zip->fp);
    unsigned int entryCrc;
    unsigned int offset;
    unsigned short extraLen, commentLen, nameLen;
    if (crc == 0)
        crc = -1;
    do {
        unsigned int sig = readU32LE(zip->fp);
        if (sig != 0x02014b50)
            goto fail;
        readFields(zip->fp, "222222 4 44 222 224 4 ", NULL, NULL, NULL, NULL, NULL, NULL, &entryCrc, NULL,
                   NULL, &nameLen, &extraLen, &commentLen, NULL, NULL, NULL, &offset);
        if (entryName)
            free(entryName);
        entryName = (char *)calloc(nameLen + 1, 1);
        total += fread(entryName, nameLen, 1, zip->fp);
        skipBytes(zip->fp, extraLen + commentLen);
    } while ((strcmp(entryName, name) != 0 || strlen(entryName) != strlen(name)) && crc != entryCrc);
    free(entryName);
    fseek(zip->fp, offset, SEEK_SET);
    return 0;
fail:
    if (entryName)
        free(entryName);
    return -1;
}

typedef struct Inflater {       // the inflate stream at 0x131a68
    char raw[0x2c];
    void *window;               // +0x2c
} Inflater;
Inflater *inflaterOpen(void *unused, FILE *fp, int compSize);  // at 0x131a68
int inflaterRead(Inflater *z, void *buf, int len);              // at 0x131b64

typedef struct ZipEntry {       // 0x1c bytes
    long pos;                   // +0x00
    Inflater *z;                // +0x04 (method 8 only)
    FILE *fp;                   // +0x08
    int compSize;               // +0x0c
    int size;                   // +0x10
    int method;                 // +0x14 0 = stored, 8 = deflate
    int read;                   // +0x18
} ZipEntry;

void zipCloseEntry(ZipEntry *e);

ZipEntry *zipOpenEntry(ZipFile *zip, const char *name, unsigned int crc)   // 0x14d418
{
    if (zipFindEntry(zip, name, crc) == 0) {
        unsigned int sig = readU32LE(zip->fp);
        if (sig != 0x04034b50) {
            puts("Error");
            return NULL;
        }
        skipBytes(zip->fp, 4);
        int method = readU16LE(zip->fp);
        if (method != 0 && method != 8) {
            puts("Error: Unsupported compression method");
            return NULL;
        }
        skipBytes(zip->fp, 8);
        int compSize = readU32LE(zip->fp);
        int size = readU32LE(zip->fp);
        int nameLen = readU16LE(zip->fp);
        int extraLen = readU16LE(zip->fp);
        skipBytes(zip->fp, extraLen + nameLen);
        ZipEntry *e = (ZipEntry *)malloc(28);
        if (e == NULL)
            return NULL;
        e->pos = ftell(zip->fp);
        e->compSize = compSize;
        e->size = size;
        if (method == 8)
            e->z = inflaterOpen(NULL, zip->fp, compSize);
        e->method = method;
        e->read = 0;
        e->fp = zip->fp;
        return e;
    }
    return NULL;
}

int zipRead(ZipEntry *e, void *buf, unsigned int len)  // 0x14d638
{
    int n;
    int unused = 0;
    if (e == NULL)
        return -1;
    if (e->method == 8) {
        n = inflaterRead(e->z, buf, len);
    } else {
        int remain = e->read - e->size;
        if (len > remain)
            remain = len;
        n = fread(buf, 1, len, e->fp);
        e->read += n;
    }
    e->pos = ftell(e->fp);
    return n;
}

// Reads the whole entry name (or with CRC crc) into a new buffer; *size gets its length.
void *zipReadFile(ZipFile *zip, const char *name, unsigned int crc, unsigned int *size)   // 0x14d730
{
    int n;
    ZipEntry *e = zipOpenEntry(zip, name, crc);
    if (e == NULL)
        return NULL;
    char *buf = (char *)malloc(e->size);
    if (buf == NULL)
        return NULL;
    if (e->method == 8)
        n = inflaterRead(e->z, buf, e->size);
    else
        n = fread(buf, 1, e->size, e->fp);
    if (n != e->size)
        printf("GNZIP: Readed data size different from uncompressed size %d!=%d \n", n, e->size);
    *size = e->size;
    zipCloseEntry(e);
    return buf;
}

void zipCloseEntry(ZipEntry *e)     // 0x14d858
{
    if (e == NULL)
        return;
    if (e->method == 8)
        free(e->z->window);
    free(e);
}

ZipFile *zipOpen(const char *path)  // 0x14d8b0
{
    ZipFile *zip = (ZipFile *)malloc(24);
    struct stat st;
    if (stat(path, &st) == -1) {
        printf("Couldn't open %s , %d:%s\n", path, errno, strerror(errno));
        free(zip);
        return NULL;
    }
    if (!S_ISREG(st.st_mode) && !S_ISLNK(st.st_mode)) {
        printf("%s is not a regular file\n", path);
        free(zip);
        return NULL;
    }
    zip->fp = fopen(path, "rb");
    if (zip->fp == NULL) {
        printf("Ho no! Couldn't open %s\n", path);
        free(zip);
        return NULL;
    }
    int err = zipReadCentralDir(zip);
    if (err) {
        printf("Strange %d %s\n", err, path);
        fclose(zip->fp);
        free(zip);
        return NULL;
    }
    return zip;
}

void zipClose(ZipFile *zip)     // 0x14da1c
{
    fclose(zip->fp);
    free(zip);
}
