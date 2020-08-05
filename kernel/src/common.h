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

int32_t clamp(int32_t value, int32_t mini, int32_t maxi);
bool btn_has(uint32_t btns, uint32_t btn);
void btn_add(uint32_t* btns, uint32_t btn);
void btn_del(uint32_t* btns, uint32_t btn);
void btn_toggle(uint32_t* btns, uint32_t btn);

char* strclone(char* dst, char* src);
char* strnclone(char* dst, char* , int num);
#endif