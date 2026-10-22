// C entry points shared by the launcher C and C++ translation units.
#pragma once
#ifdef __cplusplus
extern "C" {
#endif
int findProcess(const char *name, int debug);
const char *getLanguageFile(void);
void saveSystemJson(void *shm, const char *theme, int fontSize);
unsigned char waitForProbe(void);
unsigned char authenticateDevice(unsigned char *challenge, unsigned char *response);
#ifdef __cplusplus
}
#endif
