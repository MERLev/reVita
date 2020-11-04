#ifndef LOG_H
#define LOG_H

#include <stdio.h>
#include <string.h>

#define LOG_PREFIX " |[reVitaMotion]| "
#define LOG_PATH "ux0:/log/"
#define LOG_FILE LOG_PATH "reVitaMotion.txt"

void log_reset();
void log_write(const char *buffer, size_t length);
void log_flush();

#ifdef LOG_DEBUG
	#define LOG(...) \
	do { \
		char buffer[256]; \
		snprintf(buffer, sizeof(buffer), ##__VA_ARGS__); \
		sceClibPrintf(LOG_PREFIX); \
		sceClibPrintf(buffer); \
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
