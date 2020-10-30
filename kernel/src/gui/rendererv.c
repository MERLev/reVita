#include <vitasdkkern.h>
#include <psp2kern/display.h> 
#include <psp2kern/kernel/sysmem.h> 
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "img/font.h"
#include "img/icons.h"
#include "img/icons-font.h"
#include "rendererv.h"
#include "../common.h"
#include "../log.h"

static uint32_t color;
static bool isStripped;

SceUID vfb_uid;
uint32_t* vfb_base;
uint32_t uiWidth, uiHeight;

#define ANIMATION_TIME  120000

bool vreadPixel(uint32_t x, uint32_t y, uint32_t w, uint32_t h, const unsigned char* img){
	uint32_t realW = ((w - 1) / 8 + 1) * 8;
	uint32_t idx = (realW * y + x) / 8;
	uint8_t bitN = 7 - ((realW * y + x) % 8);
	return READ(img[idx], bitN);
}

void clear() { 
    for (int i = 0; i < uiWidth * (CHA_H + 4); i++)
        vfb_base[i] = 0x00000000;
    for (int i = uiWidth * (CHA_H + 4); i < uiWidth * uiHeight - uiWidth * (CHA_H + 4); i++)
        vfb_base[i] = 0x00171717;
    for (int i = uiWidth * uiHeight - uiWidth * (CHA_H + 4); i < uiWidth * uiHeight; i++)
        vfb_base[i] = 0x00000000;
}

void rendererv_drawChar(char character, int x, int y){
	character = character % ICON_ID__NUM;
	rendererv_drawImage(x, y, FONT_WIDTH, FONT_HEIGHT, &font[character * 40]);
}

void rendererv_drawCharIcon(char character, int x, int y){
	rendererv_drawImage(x, y - 1, ICON_W, ICON_H, &ICON[character * 60]);
}

void rendererv_drawImage(uint32_t x, uint32_t y, uint32_t w, uint32_t h, const unsigned char* img){
	uint32_t idx = 0;
	uint8_t bitN = 0;
	for (int j = 0; j < h; j++){
		for (int i = 0; i < w; i++){
			if (bitN >= 8){
				idx++;			
				bitN = 0;
			}
			if (READ(img[idx], (7 - bitN)))
				vfb_base[(y + j) * uiWidth + x + i] = color;
			bitN++;
		}
		if (bitN != 0){
			idx++;			
			bitN = 0;
		}
	}
}

void rendererv_drawString(int x, int y, const char *str){
    for (size_t i = 0; i < strlen(str); i++){
		if (str[i] == ' ') continue;
		if (str[i] == '$'){
			rendererv_drawCharIcon(str[i+1], x + i * 12, y);
			i++;
		} else {
			rendererv_drawChar(str[i], x + i * 12, y);
			// rendererv_drawImage(x + i * 12, y, FONT_WIDTH, FONT_HEIGHT, &font[(unsigned char)str[i]*40]);
		}
	}
	if (isStripped)
		rendererv_drawRectangle(x, y + CHA_H / 2, (strlen(str)) * CHA_W, 2, color);
}

void rendererv_drawStringF(int x, int y, const char *format, ...){
	char str[512] = { 0 };
	va_list va;

	va_start(va, format);
	vsnprintf(str, 512, format, va);
	va_end(va);

	rendererv_drawString(x, y, str);
}

void rendererv_drawRectangle(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t clr) {
	if ((x + w) > uiWidth || (y + h) > uiHeight)
		return;
	for (int i = x; i < x + w; i++)
		for (int j = y; j < y + h; j++)
			vfb_base[j * uiWidth + i] = clr;
}

void rendererv_setColor(uint32_t c){
	color = c;
}

void rendererv_setStripped(bool flag){
	isStripped = flag;
}

int rendererv_allocVirtualFB(){
    int ret = ksceKernelAllocMemBlock("vfb_base", SCE_KERNEL_MEMBLOCK_TYPE_KERNEL_RW, (uiWidth*uiHeight*sizeof(uint32_t)  + 0xfff) & ~0xfff, NULL);
    if (ret > 0){
		vfb_uid = ret;
		ksceKernelGetMemBlockBase(vfb_uid, (void**)&vfb_base);
	}
    LOG("MEMORY ALLOC renderer %i : %i\n", (int)vfb_base, (uiWidth*uiHeight*sizeof(uint32_t) + 0xfff) & ~0xfff);
	return ret;
}

int rendererv_freeVirtualFB(){
	return ksceKernelFreeMemBlock(vfb_uid);
}

void rendererv_init(uint32_t w, uint32_t h){
	uiWidth = w;
	uiHeight = h;
}

void rendererv_destroy(){
}
