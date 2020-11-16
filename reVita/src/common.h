#ifndef _COMMON_H_
#define _COMMON_H_
#include <stdbool.h>

#define max(a,b) (((a)>(b))?(a):(b))
#define min(a,b) (((a)<(b))?(a):(b))
#define lim(a,b,c) (((a)>(b))?(((a)<(c))?(a):(c)):(b))

#define SET(x,y) x |= (1 << y)
#define CLEAR(x,y) x &= ~(1<< y)
#define READ(x,y) ((0u == (x & (1<<y)))?0u:1u)
#define TOGGLE(x,y) (x ^= (1<<y))
#define SIZE(x)  (sizeof(x) / sizeof((x)[0]))

#define FLIP(B) (B=!B)
#define STRUCTS(type, name) \
    type name;\
    memset(&name, 0, sizeof(type));\
    name.size = sizeof(type);\

#define STREQALL(...) streqall(__VA_ARGS__, NULL)
#define STREQANY(...) streqany(__VA_ARGS__, NULL)

#define SECONDARY_PROFILE_SUFFIX "-SECONDARY"

char* ullx(uint64_t val);
int32_t clamp(int32_t value, int32_t mini, int32_t maxi);
int32_t clampSmart(int32_t val, int32_t min, int32_t max);
bool btn_has(uint32_t btns, uint32_t btn);
void btn_add(uint32_t* btns, uint32_t btn);
void btn_del(uint32_t* btns, uint32_t btn);
void btn_toggle(uint32_t* btns, uint32_t btn);

char* strclone(char* dst, char* src);
char* strnclone(char* dst, char* , int num);
bool streq(char* str1, char* str2);
bool streqall(char *first, char* second, ...);
bool streqany(char *first, ...);
bool strStartsWith(char* s, char* p);
bool strEndsWith(char* s, char* p);

int floorSqrt(int x);
#endif