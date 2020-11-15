#include <vitasdkkern.h>
#include <stdbool.h>
#include <string.h>
#include "common.h"

#define HBASE 16

char* ullx(uint64_t val){
    static char buf[HBASE + 2] = { [0 ... HBASE + 1] = 0 };
    char* out = &buf[HBASE + 1];
    uint64_t hval = val;

    do {
        *out = "0123456789ABCDEF"[hval % HBASE];
        --out;
        hval /= HBASE;
    } while(hval);

    while (out != &buf[2]){
        *out = '0';
        out--;
    }
    *out-- = 'x', *out = '0';

    return out;
}

int32_t clamp(int32_t value, int32_t mini, int32_t maxi) {
	if (value < mini) return mini; 
	if (value > maxi) return maxi;
	return value;
}

//min and max could be swapped
int32_t clampSmart(int32_t val, int32_t min, int32_t max) {
	if ((max > min && val < min) || (max <= min && val > min)) return min;
	if ((max > min && val > max) || (max <= min && val < max)) return max;
	return val;
}

bool btn_has(uint32_t btns, uint32_t btn){
	return (btns & btn) == btn;
}

void btn_add(uint32_t* btns, uint32_t btn){
	*btns = *btns | btn;
}

void btn_del(uint32_t* btns, uint32_t btn){
	*btns = *btns & ~btn;
}

void btn_toggle(uint32_t* btns, uint32_t btn){
	*btns = *btns ^ btn;
}

char* strclone(char* dst, char* src){
	strcpy(dst, src);
	dst[strlen(src)] = '\0';
	return dst;
}

char* strnclone(char* dst, char* src, int num){
	strncpy(dst, src, num);
	dst[num] = '\0';
	return dst;
}

bool streq(char* str1, char* str2){
	return strcmp(str1, str2) == 0;
}

bool strStartsWith(char* s, char* p){
	return strncmp(s, p, strlen(p)) == 0;
}

bool strEndsWith(char* s, char* p)
{
    size_t ls = strlen(s);
    size_t lp = strlen(p);
    if (ls >= lp) {
        return (0 == memcmp(p, s + (ls - lp), lp));
    }
    return false;
}

bool streqall(char *first, char* second, ...){
    int ret = true;
	va_list argp;
	char *p;

	if(first == NULL || second == NULL || !streq(first, second))
		return false;

	va_start(argp, second);

	while((p = va_arg(argp, char *)) != NULL)
		ret = ret && streq(first, p);

	va_end(argp);

	return ret;
}

bool streqany(char *first, ...){
	va_list argp;
	char *p;

	if(first == NULL)
		return false;
		
	va_start(argp, first);

	while((p = va_arg(argp, char *)) != NULL)
	    if (streq(first, p))
	        return true;

	va_end(argp);

	return false;
}

int floorSqrt(int x) { 
    if (x == 0 || x == 1) 
    	return x; 
  
    int i = 1, result = 1; 
    while (result <= x) { 
      i++; 
      result = i * i; 
    } 
    return i - 1; 
} 

char* removeSecondarySuffix(char* titleid) {
	if (strEndsWith(titleid, SECONDARY_PROFILE_SUFFIX))
		titleid[strlen(titleid) - strlen(SECONDARY_PROFILE_SUFFIX)] = 0;
	return titleid;
}