#ifndef _RENDERERV_H_
#define _RENDERERV_H_

#include <stdbool.h>

#define CHA_W  12		//Character size in pexels
#define CHA_H  20

extern uint32_t* vfb_base;
extern uint32_t uiWidth, uiHeight;

void rendererv_drawChar(char character, int x, int y);
void rendererv_drawCharIcon(char character, int x, int y);
void rendererv_drawImage(uint32_t x, uint32_t y, uint32_t w, uint32_t h, const unsigned char* img);
void rendererv_drawString(int x, int y, const char *str);
void rendererv_drawStringF(int x, int y, const char *format, ...);
void rendererv_drawRectangle(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t clr);

void rendererv_setColor(uint32_t clr);
void rendererv_setStripped(bool flag);

int rendererv_allocVirtualFB();
int rendererv_freeVirtualFB();

void rendererv_init(uint32_t w, uint32_t h);
void rendererv_destroy();

#endif