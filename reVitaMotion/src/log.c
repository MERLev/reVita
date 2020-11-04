#include <vitasdk.h>
#include <taihen.h>
#include <stdio.h>
#include <psp2/io/fcntl.h> 
#include "log.h"

static unsigned int log_buf_ptr = 0;
static char log_buf[16 * 1024];

void log_reset(){
	SceUID fd = sceIoOpen(LOG_FILE,
		SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 6);
	if (fd < 0)
		return;

	sceIoClose(fd);

	memset(log_buf, 0, sizeof(log_buf));
}

void log_write(const char *buffer, size_t length){
	if ((log_buf_ptr + length) >= sizeof(log_buf))
		log_flush();

	memcpy(log_buf + log_buf_ptr, buffer, length);

	log_buf_ptr = log_buf_ptr + length;
}

void log_flush(){
	sceIoMkdir(LOG_PATH, 6);

	SceUID fd = sceIoOpen(LOG_FILE,
		SCE_O_WRONLY | SCE_O_CREAT | SCE_O_APPEND, 6);
	if (fd < 0)
		return;

	sceIoWrite(fd, log_buf, strlen(log_buf));
	sceIoClose(fd);
	memset(log_buf, 0, sizeof(log_buf));
	log_buf_ptr = 0;
}
