// cJSON 1.7.10 (compiled into MainUI at 0x51b60): the node layout, its type flags, and the
// functions MainUI calls, declared once. C and C++.
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef struct cJSON {
    struct cJSON *next, *prev, *child;
    int type;
    char *valuestring;
    int valueint;
    double valuedouble;
    char *string;
} cJSON;

#define cJSON_Number 8
#define cJSON_String 16

cJSON *cJSON_Parse(const char *text);
const char *cJSON_GetErrorPtr(void);
void cJSON_Delete(cJSON *item);
char *cJSON_Print(const cJSON *item);
char *cJSON_PrintUnformatted(const cJSON *item);
void cJSON_free(void *object);

cJSON *cJSON_GetObjectItem(const cJSON *object, const char *name);
int cJSON_GetArraySize(const cJSON *array);
cJSON *cJSON_GetArrayItem(const cJSON *array, int index);
char *cJSON_GetStringValue(const cJSON *item);
int cJSON_IsNumber(const cJSON *item);
int cJSON_IsString(const cJSON *item);

cJSON *cJSON_CreateObject(void);
cJSON *cJSON_CreateNumber(double number);
cJSON *cJSON_CreateString(const char *string);
cJSON *cJSON_CreateBool(int value);
void cJSON_AddItemToObject(cJSON *object, const char *name, cJSON *item);
void cJSON_AddItemToArray(cJSON *array, cJSON *item);
cJSON *cJSON_AddNumberToObject(cJSON *object, const char *name, double number);
cJSON *cJSON_AddStringToObject(cJSON *object, const char *name, const char *string);
cJSON *cJSON_AddArrayToObject(cJSON *object, const char *name);
void cJSON_DeleteItemFromObject(cJSON *object, const char *name);

#ifdef __cplusplus
}
#endif
