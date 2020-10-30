#ifndef _RENDERER_H_
#define _RENDERER_H_

#include <stdbool.h>

#define CHA_W  12		//Character size in pexels
#define CHA_H  20

extern uint32_t fbWidth, fbHeight, fbPitch;

void renderer_drawChar(char character, int x, int y);
void renderer_drawCharIcon(char character, int x, int y);
void renderer_drawImage(uint32_t x, uint32_t y, uint32_t w, uint32_t h, const unsigned char* img);
void renderer_drawString(int x, int y, const char *str);
void renderer_drawStringF(int x, int y, const char *format, ...);

void renderer_drawLine(int32_t x1, int32_t y1, int32_t x2, int32_t y2);
void renderer_drawLineThick(int32_t x1, int32_t y1, int32_t x2, int32_t y2, uint8_t thickness);
void renderer_drawRectangle(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t clr);

void renderer_setFB(const SceDisplayFrameBuf *param);
void renderer_writeFromVFB(int64_t timeFromOpene);

void renderer_setColor(uint32_t clr);
void renderer_setStripped(bool flag);

void renderer_init();
void renderer_destroy();

#endif