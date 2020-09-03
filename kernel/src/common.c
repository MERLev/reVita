#include <vitasdkkern.h>
#include <stdbool.h>
#include <string.h>

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