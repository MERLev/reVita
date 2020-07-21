#ifndef _COMMON_H_
#define _COMMON_H_

# define max(a,b) (((a)>(b))?(a):(b))
# define min(a,b) (((a)<(b))?(a):(b))
# define lim(a,b,c) (((a)>(b))?(((a)<(c))?(a):(c)):(b))

extern int32_t clamp(int32_t value, int32_t mini, int32_t maxi);

#endif