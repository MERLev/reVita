#ifndef _COMMON_H_
#define _COMMON_H_

# define max(a,b) (((a)>(b))?(a):(b))
# define min(a,b) (((a)<(b))?(a):(b))
# define lim(a,b,c) (((a)>(b))?(((a)<(c))?(a):(c)):(b))

#define PHYS_BUTTONS_NUM    16 // Supported physical buttons num
extern const uint32_t HW_BUTTONS[PHYS_BUTTONS_NUM];

extern int32_t clamp(int32_t value, int32_t mini, int32_t maxi);

#endif