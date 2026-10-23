// DeviceAuth.c (0x14f730-0x150ad8): the device check MainUI runs before anything else.
//
// A chip on I2C bus 1 (8-bit address 0x7a; probably a vendor authentication chip) must answer a
// challenge. The first call sets the chip up: it reads two 8-byte keys from it, exchanges a
// block scrambled with fixed keys, and sends one of its keys back disguised. Each call then
// writes the challenge (with random filler bytes), reads the chip's answer, unscrambles it with
// the keys and a counter, and compares the result with the challenge; it tries two registers.
// waitForProbe (I2c.c) retries this up to 20 times, and main exits if it fails.
//
// Separately built vendor code: compiled with GCC 7.3.0 (deps gcc73; its .comment string is
// MainUI's second one), -O0 -fsigned-char -fPIC. Byte arrays are unsigned. The library and
// vendor are unknown, and the storage names are reconstructions (see the comments).
#include "PlatformApi.h"
typedef unsigned char byte;
extern byte i2cRead(byte, byte, byte *, int);
extern byte i2cWrite(byte, byte, byte *, int);
extern void msleep(int);
extern byte randomByte();

#define AUTH_CHIP 0x7a          // I2C address of the chip (8-bit form; 7-bit 0x3d)
#define AUTH_MISMATCH 50        // authenticateDevice: the answer did not match
extern byte deviceKeyA[8];       // 0x188a24
extern byte deviceTxData[16];        // 0x188a2c
extern byte deviceKeyB[8];       // 0x188a3c
extern byte deviceCounterBuf[2];    // 0x188a44
extern byte deviceRecvBuf[16];        // 0x188a48
extern byte deviceHasInit;   // 0x18267c
extern byte cipherKeys1[2][8];    // 0x17ff0c
extern byte cipherKeys2[2][8];    // 0x17ff1c

byte deviceHasInit = 0; // 0x18267c, ordinary .bss: the explicit zero keeps it out of COMMON (public, so -fPIC GOT access is unchanged); name chosen so ld's GOT (hash) order matches the original; the original name is stripped
byte deviceKeyA[8]; // 0x188a24
byte deviceTxData[16]; // 0x188a2c, COMMON; name chosen so ld's within-file COMMON (hash) order matches the original; the original name is stripped
byte deviceKeyB[8]; // 0x188a3c
byte deviceCounterBuf[2]; // 0x188a44, COMMON; name chosen so ld's within-file COMMON (hash) order matches the original; the original name is stripped
byte deviceRecvBuf[16]; // 0x188a48, COMMON; name chosen so ld's within-file COMMON (hash) order matches the original; the original name is stripped
// 0x17ff0c: name chosen so ld's GOT (hash) order matches the original; the original name is stripped
byte cipherKeys1[2][8] = {
    {0xe8, 0xf6, 0x78, 0x6a, 0x7b, 0x04, 0xf8, 0xd2},
    {0xff, 0x1e, 0x6f, 0x82, 0x6c, 0xec, 0xef, 0x3a},
}; // 0x17ff0c
// 0x17ff1c: name chosen so ld's GOT (hash) order matches the original; the original name is stripped
byte cipherKeys2[2][8] = {
    {0x9f, 0xb7, 0xfb, 0x2c, 0xc0, 0x36, 0x0a, 0x1e},
    {0x09, 0x21, 0x6d, 0xba, 0x56, 0xa0, 0x9c, 0x88},
}; // 0x17ff1c

// Reads the chip's two 8-byte keys, four bytes from each of registers 0x73-0x76.
// Returns 0 or the I2C error.
byte readDeviceKeys()            // 0x14f730
{
    int i;
    byte status;
    byte buf[8];
    status = i2cRead(AUTH_CHIP, 0x73, buf, 8);
    if (status) return status;
    for (i = 0; i < 4; i++) deviceKeyA[i] = buf[i + 4];
    status = i2cRead(AUTH_CHIP, 0x74, buf, 8);
    if (status) return status;
    for (i = 4; i < 8; i++) deviceKeyA[i] = buf[i];
    status = i2cRead(AUTH_CHIP, 0x75, buf, 8);
    if (status) return status;
    for (i = 0; i < 4; i++) deviceKeyB[i] = buf[i + 4];
    status = i2cRead(AUTH_CHIP, 0x76, buf, 8);
    if (status) return status;
    for (i = 4; i < 8; i++) deviceKeyB[i] = buf[i];
    return 0;
}

// The three block operations of transformDeviceBlock, on 8 bytes in place.
void invertSwapHalves(byte *data) // 0x14f958
{
    int i;
    byte tmp[8];
    for (i = 0; i < 4; i++) {
        tmp[i] = ~data[i + 4];
        tmp[i + 4] = data[i];
    }
    for (i = 0; i < 8; i++) data[i] = tmp[i];
}

void xorSwapHalves(byte *data, byte *key) // 0x14fa34
{
    int i;
    byte tmp[8];
    for (i = 0; i < 4; i++) {
        tmp[i + 4] = data[i] ^ key[i];
        tmp[i] = data[i + 4] ^ key[i + 4];
    }
    for (i = 0; i < 8; i++) data[i] = tmp[i];
}

void rotateBytesRight(byte *data) // 0x14fb40
{
    int i;
    byte tmp[8];
    for (i = 7; i > 0; i--) tmp[i] = data[i - 1];
    tmp[0] = data[7];
    for (i = 0; i < 8; i++) data[i] = tmp[i];
}

// The scrambler: copies input to output, then for each round derives an 8-byte round key
// (fixed keys in mode 0; otherwise fixed keys mixed with the chip's keys), mixes in the seed
// and the counter, and applies bits-1 operations chosen by bits of the rotating key.
void transformDeviceBlock(byte *input, byte *output, byte *seed,
                          byte rounds, int bits, byte mode) // 0x14fbfc
{
    int i;
    byte round;
    byte step;
    byte saved;
    byte op;
    byte key[8];
    for (i = 0; i < 8; i++) output[i] = input[i];
    for (round = 0; round < rounds; round++) {
        if (mode == 0) {
            for (i = 0; i < 8; i++) key[i] = cipherKeys1[round][i];
        } else {
            for (i = 0; i < 8; i++)
                key[i] = cipherKeys2[round][i] ^ deviceKeyA[i] ^ deviceKeyB[i];
        }
        for (i = 0; i < 2; i++) key[i] = key[i] ^ seed[i];
        for (i = 0; i < 2; i++) key[i + 2] = key[i + 2] ^ deviceCounterBuf[i];
        for (step = 0; step < bits - 1; step++) {
            saved = key[0];
            for (i = 0; i < 7; i++) key[i] = (key[i] << 3) | (key[i + 1] >> 5);
            key[7] = (key[7] << 3) | (saved >> 5);
        }
        for (step = 0; step < bits - 1; step++) {
            saved = key[7];
            for (i = 7; i > 0; i--) key[i] = (key[i] >> 3) | (key[i - 1] << 5);
            key[0] = (key[0] >> 3) | (saved << 5);
            op = (key[0] >> 6) | ((key[0] >> 3) & 4);
            if (op == 0)
                invertSwapHalves(output);
            else if (op == 1)
                xorSwapHalves(output, key);
            else if (mode != 0 && op == 3)
                rotateBytesRight(output);
        }
    }
}

// Setup step 1: reads 16 bytes from register 0x30, scrambles eight of them (bytes 0-1, 4-5,
// 8-9, 12-13, seeded by bytes 11 and 2) with the fixed keys, and writes the result back in the
// even slots with fixed odd bytes. Starts the counter at 1.
byte beginDeviceAuth()           // 0x1500b4
{
    int i;
    byte status;
    byte output[8];
    byte input[8];
    byte seed[2];
    status = i2cRead(AUTH_CHIP, 0x30, deviceRecvBuf, 16);
    if (status) return status;
    for (i = 0; i < 8; i += 2) {
        input[i] = deviceRecvBuf[i * 2];
        input[i + 1] = deviceRecvBuf[i * 2 + 1];
    }
    seed[0] = deviceRecvBuf[11];
    seed[1] = deviceRecvBuf[2];
    deviceCounterBuf[0] = 0;
    deviceCounterBuf[1] = 1;
    transformDeviceBlock(input, output, seed, 2, 32, 0);
    for (i = 0; i < 8; i++) deviceTxData[i * 2] = output[i];
    deviceTxData[0] = ~(deviceTxData[0] ^ 0x5f);
    deviceTxData[1] = 0;
    deviceTxData[3] = 0x52;
    deviceTxData[5] = 0x47;
    deviceTxData[7] = 0x4c;
    deviceTxData[9] = 0x29;
    deviceTxData[11] = 0x22;
    deviceTxData[13] = 0x3a;
    deviceTxData[15] = 0xc6;
    status = i2cWrite(AUTH_CHIP, 0x30, deviceTxData, 16);
    if (status) return status;
    msleep(42);
    return 0;
}

// Setup step 3: writes the chip's second key, XOR-masked, to register 0x40.
// Original quirk, kept as-is: the random bytes go to the even slots, which the key then
// overwrites, so the odd slots keep what was there.
byte sendDeviceKey()             // 0x150320
{
    int i;
    byte status;
    byte key[8];
    key[0] = ~(deviceKeyB[0] ^ 0x24);
    key[1] = ~(deviceKeyB[1] ^ 0x3c);
    key[2] = deviceKeyB[2] ^ 0x4b;
    key[3] = deviceKeyB[3] ^ 0x5f;
    key[4] = deviceKeyB[4] ^ 0x48;
    key[5] = deviceKeyB[5] ^ 0x31;
    key[6] = ~(deviceKeyB[6] ^ 0x34);
    key[7] = ~(deviceKeyB[7] ^ 0x18);
    deviceCounterBuf[1] += 2;
    for (i = 1; i < 8; i += 2) deviceTxData[i * 2] = randomByte();
    for (i = 0; i < 8; i++) deviceTxData[i * 2] = key[i];
    status = i2cWrite(AUTH_CHIP, 0x40, deviceTxData, 16);
    if (status) return status;
    return 0;
}

// One check (see the file header): 0 if the chip's answer, unscrambled into response, equals
// the challenge (8 bytes, partly overwritten), else AUTH_MISMATCH or an I2C error.
byte authenticateDevice(byte *challenge, byte *response) // 0x150504
{
    int i;
    int j;
    int attempt;
    byte status;
    byte reg;
    byte firstReg;
    int iterations;
    byte input[8];
    byte seed[2];
    firstReg = 0xe9;
    if (deviceHasInit == 0) {   // once per run: reset the chip, then the setup steps
        for (i = 0; i < 16; i++) deviceTxData[i] = 0;
        i2cWrite(AUTH_CHIP, 0x80, deviceTxData, 8);
        msleep(1);
        i2cWrite(AUTH_CHIP, 0x20, deviceTxData, 1);
        msleep(1);
        deviceTxData[0] = 0x83;
        i2cWrite(AUTH_CHIP, 0x22, deviceTxData, 1);
        msleep(1);
        status = beginDeviceAuth();
        if (status) return status;
        msleep(1);
        status = readDeviceKeys();
        if (status) return status;
        status = sendDeviceKey();
        if (status) return status;
        deviceHasInit = 1;
    }
    for (attempt = 0; attempt < 2; attempt++) {
        if (attempt == 0)
            reg = firstReg;
        else
            reg = 0x87;
        for (i = 0; i < 8; i++) {
            deviceTxData[i * 2] = challenge[i];
            deviceTxData[i * 2 + 1] = randomByte();
        }
        if (attempt == 0) {
            status = i2cRead(AUTH_CHIP, reg, deviceRecvBuf, 16);
            if (status) return status;
            for (i = 0; i < 8; i += 2) {
                input[i] = deviceRecvBuf[i * 2];
                input[i + 1] = deviceRecvBuf[i * 2 + 1];
            }
            seed[0] = deviceRecvBuf[11];
            seed[1] = deviceRecvBuf[2];
            deviceTxData[0] = ~seed[0];
            deviceTxData[14] = seed[1];
            challenge[0] = deviceTxData[0];
            challenge[7] = deviceTxData[14];
        }
        status = i2cWrite(AUTH_CHIP, reg, deviceTxData, 16);
        if (status) return status;
        msleep(5);
        status = i2cRead(AUTH_CHIP, reg, deviceRecvBuf, 16);
        if (status) return status;
        for (i = 0; i < 8; i += 2) {
            input[i] = deviceRecvBuf[i * 2];
            input[i + 1] = deviceRecvBuf[i * 2 + 1];
        }
        seed[0] = deviceRecvBuf[11];
        seed[1] = deviceRecvBuf[2];
        if (deviceCounterBuf[1] == 255) {
            deviceCounterBuf[0]++;
            deviceCounterBuf[1] = 0;
        } else
            deviceCounterBuf[1]++;
        iterations = 1;
        for (i = 0; i < iterations; i++) {
            transformDeviceBlock(input, response, seed, 2, 32, 2);
            for (j = 0; j < 8; j++) input[j] = response[j];
        }
        for (i = 0; i < 8; i++) {
            if (challenge[i] != response[i]) {
                status = AUTH_MISMATCH;
                break;
            }
        }
        if (status == 0) return 0;
    }
    return status;
}
