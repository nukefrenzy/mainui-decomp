// tu_14dc18: I2C register access on /dev/i2c-1, msleep and a random byte (0x14dc18-0x14e0cc);
// the PIC device-check code from 0x14f730 calls these.
// Compiled as C (call results are stored straight from r0), so there is no template tail.
// The C units 0x14acfc-0x14f730 are split where .rodata proves a new file: GCC merges identical
// string literals within one file even at -O0, and "w", "r" and "%d" each appear more than once.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <time.h>

// I2C register access on /dev/i2c-1 (linux/i2c-dev.h layouts).
struct i2c_msg {
    unsigned short addr;
    unsigned short flags;
    unsigned short len;
    unsigned char *buf;
};
struct i2c_rdwr_ioctl_data {
    struct i2c_msg *msgs;
    unsigned int nmsgs;
};

unsigned char i2cRead(unsigned char devAddr, unsigned char reg, unsigned char *buf, int len)  // 0x14dc18
{
    int i;
    unsigned int fd;
    int ret;
    struct i2c_rdwr_ioctl_data data;
    struct i2c_msg msgs[2];
    unsigned char wbuf[512];
    unsigned char rbuf[512];
    memset(wbuf, 0, 512);
    memset(rbuf, 0, 512);
    memset(buf, 0, len);
    fd = open("/dev/i2c-1", O_RDWR);
    if (fd == 0) {
        puts("error on opening the device file");
        exit(1);
    }
    ioctl(fd, 0x702, 5);
    ioctl(fd, 0x701, 1);
    data.msgs = msgs;
    data.nmsgs = 2;
    data.msgs[0].len = 1;
    data.msgs[0].flags = 0;
    data.msgs[0].addr = devAddr >> 1;
    data.msgs[0].buf = wbuf;
    data.msgs[0].buf[0] = reg;
    data.msgs[1].len = len;
    data.msgs[1].flags = 1;
    data.msgs[1].addr = devAddr >> 1;
    data.msgs[1].buf = rbuf;
    ret = ioctl(fd, 0x707, &data);
    if (ret < 0) {
        printf("error during I2C_RDWR ioctl with error code %d\n", ret);
        return ret;
    }
    memcpy(buf, data.msgs[1].buf, len);
    for (i = 0; i < len; i++)
        ;
    close(fd);
    return 0;
}

unsigned char i2cWrite(unsigned char devAddr, unsigned char reg, unsigned char *buf, int len) // 0x14de34
{
    unsigned int fd;
    int ret;
    struct i2c_rdwr_ioctl_data data;
    struct i2c_msg msgs[2];
    unsigned char wbuf[512];
    fd = open("/dev/i2c-1", O_RDWR);
    if (fd == 0) {
        puts("error on opening the device file");
        exit(1);
    }
    ioctl(fd, 0x702, 5);
    ioctl(fd, 0x701, 1);
    data.msgs = msgs;
    data.nmsgs = 1;
    data.msgs[0].len = len + 1;
    data.msgs[0].flags = 0;
    data.msgs[0].addr = devAddr >> 1;
    data.msgs[0].buf = wbuf;
    data.msgs[0].buf[0] = reg;
    memcpy(data.msgs[0].buf + 1, buf, len);
    ret = ioctl(fd, 0x707, &data);
    if (ret < 0) {
        printf("error during I2C_RDWR ioctl with error code %d\n", ret);
        return ret;
    }
    close(fd);
    return 0;
}

void msleep(int ms)             // 0x14df98
{
    usleep(ms * 1000);
}

static unsigned int g_randSeed; // 0x182640

unsigned char randomByte()      // 0x14dfc8
{
    srand(time(NULL));
    g_randSeed += rand();
    g_randSeed = g_randSeed * 1103515245 + 12345;
    return g_randSeed >> 16;
}

unsigned char probe_50504(unsigned char *a, unsigned char *b);  // at 0x150504

unsigned char waitForProbe()    // 0x14e054
{
    int i;
    unsigned char ok;
    unsigned char b[8];
    unsigned char a[8];
    for (i = 0; i < 20; i++) {
        ok = probe_50504(a, b);
        if (!ok)
            break;
        putchar('.');
    }
    return ok;
}
