// PIC I2C device authentication (0x14f730-0x150ad8).
// Arm GNU 8.2-2018.08, -O0 -fsigned-char -fPIC. Byte arrays are unsigned.
// Recovered storage addresses are listed below; the library/vendor is unknown.
typedef unsigned char byte;
extern byte i2cRead(byte, byte, byte *, int);
extern byte i2cWrite(byte, byte, byte *, int);
extern void msleep(int);
extern byte randomByte();
extern byte deviceKeyA[8];       // 0x188a24
extern byte deviceTx[16];        // 0x188a2c
extern byte deviceKeyB[8];       // 0x188a3c
extern byte deviceCounter[2];    // 0x188a44
extern byte deviceRx[16];        // 0x188a48
extern byte deviceInitialized;   // 0x18267c
extern byte roundTableA[2][8];    // 0x17ff0c
extern byte roundTableB[2][8];    // 0x17ff1c

byte readDeviceKeys()            // 0x14f730
{
    int i;
    byte status;
    byte buf[8];
    status = i2cRead(0x7a, 0x73, buf, 8);
    if (status) return status;
    for (i = 0; i < 4; i++) deviceKeyA[i] = buf[i + 4];
    status = i2cRead(0x7a, 0x74, buf, 8);
    if (status) return status;
    for (i = 4; i < 8; i++) deviceKeyA[i] = buf[i];
    status = i2cRead(0x7a, 0x75, buf, 8);
    if (status) return status;
    for (i = 0; i < 4; i++) deviceKeyB[i] = buf[i + 4];
    status = i2cRead(0x7a, 0x76, buf, 8);
    if (status) return status;
    for (i = 4; i < 8; i++) deviceKeyB[i] = buf[i];
    return 0;
}

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
            for (i = 0; i < 8; i++) key[i] = roundTableA[round][i];
        } else {
            for (i = 0; i < 8; i++)
                key[i] = roundTableB[round][i] ^ deviceKeyA[i] ^ deviceKeyB[i];
        }
        for (i = 0; i < 2; i++) key[i] = key[i] ^ seed[i];
        for (i = 0; i < 2; i++) key[i + 2] = key[i + 2] ^ deviceCounter[i];
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
            if (op == 0) invertSwapHalves(output);
            else if (op == 1) xorSwapHalves(output, key);
            else if (mode != 0 && op == 3) rotateBytesRight(output);
        }
    }
}

byte beginDeviceAuth()           // 0x1500b4
{
    int i;
    byte status;
    byte output[8];
    byte input[8];
    byte seed[2];
    status = i2cRead(0x7a, 0x30, deviceRx, 16);
    if (status) return status;
    for (i = 0; i < 8; i += 2) {
        input[i] = deviceRx[i * 2];
        input[i + 1] = deviceRx[i * 2 + 1];
    }
    seed[0] = deviceRx[11];
    seed[1] = deviceRx[2];
    deviceCounter[0] = 0;
    deviceCounter[1] = 1;
    transformDeviceBlock(input, output, seed, 2, 32, 0);
    for (i = 0; i < 8; i++) deviceTx[i * 2] = output[i];
    deviceTx[0] = ~(deviceTx[0] ^ 0x5f);
    deviceTx[1] = 0;
    deviceTx[3] = 0x52;
    deviceTx[5] = 0x47;
    deviceTx[7] = 0x4c;
    deviceTx[9] = 0x29;
    deviceTx[11] = 0x22;
    deviceTx[13] = 0x3a;
    deviceTx[15] = 0xc6;
    status = i2cWrite(0x7a, 0x30, deviceTx, 16);
    if (status) return status;
    msleep(42);
    return 0;
}

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
    deviceCounter[1] += 2;
    for (i = 1; i < 8; i += 2) deviceTx[i * 2] = randomByte();
    for (i = 0; i < 8; i++) deviceTx[i * 2] = key[i];
    status = i2cWrite(0x7a, 0x40, deviceTx, 16);
    if (status) return status;
    return 0;
}

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
    if (deviceInitialized == 0) {
        for (i = 0; i < 16; i++) deviceTx[i] = 0;
        i2cWrite(0x7a, 0x80, deviceTx, 8);
        msleep(1);
        i2cWrite(0x7a, 0x20, deviceTx, 1);
        msleep(1);
        deviceTx[0] = 0x83;
        i2cWrite(0x7a, 0x22, deviceTx, 1);
        msleep(1);
        status = beginDeviceAuth();
        if (status) return status;
        msleep(1);
        status = readDeviceKeys();
        if (status) return status;
        status = sendDeviceKey();
        if (status) return status;
        deviceInitialized = 1;
    }
    for (attempt = 0; attempt < 2; attempt++) {
        if (attempt == 0) reg = firstReg;
        else reg = 0x87;
        for (i = 0; i < 8; i++) {
            deviceTx[i * 2] = challenge[i];
            deviceTx[i * 2 + 1] = randomByte();
        }
        if (attempt == 0) {
            status = i2cRead(0x7a, reg, deviceRx, 16);
            if (status) return status;
            for (i = 0; i < 8; i += 2) {
                input[i] = deviceRx[i * 2];
                input[i + 1] = deviceRx[i * 2 + 1];
            }
            seed[0] = deviceRx[11];
            seed[1] = deviceRx[2];
            deviceTx[0] = ~seed[0];
            deviceTx[14] = seed[1];
            challenge[0] = deviceTx[0];
            challenge[7] = deviceTx[14];
        }
        status = i2cWrite(0x7a, reg, deviceTx, 16);
        if (status) return status;
        msleep(5);
        status = i2cRead(0x7a, reg, deviceRx, 16);
        if (status) return status;
        for (i = 0; i < 8; i += 2) {
            input[i] = deviceRx[i * 2];
            input[i + 1] = deviceRx[i * 2 + 1];
        }
        seed[0] = deviceRx[11];
        seed[1] = deviceRx[2];
        if (deviceCounter[1] == 255) {
            deviceCounter[0]++;
            deviceCounter[1] = 0;
        } else deviceCounter[1]++;
        iterations = 1;
        for (i = 0; i < iterations; i++) {
            transformDeviceBlock(input, response, seed, 2, 32, 2);
            for (j = 0; j < 8; j++) input[j] = response[j];
        }
        for (i = 0; i < 8; i++) {
            if (challenge[i] != response[i]) {
                status = 50;
                break;
            }
        }
        if (status == 0) return 0;
    }
    return status;
}
