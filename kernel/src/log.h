#ifndef LOG_H
#define LOG_H

#include <stdio.h>
#include <string.h>

#define LOG_PREFIX " [remaPSV2] "
#define LOG_PATH "ux0:/log/"
#define LOG_FILE LOG_PATH "remaPSV2.txt"

void log_reset(); 
void log_write(const char *buffer, size_t length);
void log_flush();

#ifdef LOG_DEBUG
	extern int prefixFlag;
	#define LOG(...) \
	do { \
		if (prefixFlag){\
			ksceDebugPrintf(LOG_PREFIX); \
			prefixFlag = 0;\
		}\
		char buffer[256]; \
		snprintf(buffer, sizeof(buffer), ##__VA_ARGS__); \
		char *pch, *pchPrev = &buffer[0]; \
		pch = strchr(buffer,'\n'); \
		while (pch != NULL && pch[1] != '\0') { \
			*pch = '\0'; \
			ksceDebugPrintf(pchPrev); \
			ksceDebugPrintf("\n"); \
			ksceDebugPrintf(LOG_PREFIX); \
			pchPrev = pch + 1; \
			pch = strchr(pch + 1,'\n'); \
		} \
		ksceDebugPrintf(pchPrev); \
		if (pch != NULL && pch[1] == '\0') \
			prefixFlag = 1; \
	} while (0)
	#define LOGF LOG
#elif LOG_DISC
	#define LOG(...) \
	do { \
		char buffer[256]; \
		snprintf(buffer, sizeof(buffer), ##__VA_ARGS__); \
		log_write(buffer, strlen(buffer)); \
	} while (0)
	#define LOGF(...) \
	do { \
		char buffer[256]; \
		snprintf(buffer, sizeof(buffer), ##__VA_ARGS__); \
		log_write(buffer, strlen(buffer)); \
		log_flush(); \
	} while (0)
#else
	#define LOG(...) (void)0
	#define LOGF LOG
#endif

#endif
