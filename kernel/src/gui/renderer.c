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
#include "renderer.h"
#include "../common.h"
#include "../log.h"

uint32_t color;
uint8_t stripped;

SceUID fbuf_uid;
uint32_t* fb_base;
uint32_t* fbfbBase_user;
uint32_t fbWidth, fbHeight, fbPitch;
uint32_t uiWidth, uiHeight;

#define UI_CORNER_RADIUS 9
#define ANIMATION_TIME  120000

static const unsigned char UI_CORNER_OFF[UI_CORNER_RADIUS] = {9, 7, 5, 4, 3, 2, 2, 1, 1};

void renderer_drawChar(char character, int x, int y){
	character = character % ICON_ID__NUM;
	renderer_drawImage(x, y, FONT_WIDTH, FONT_HEIGHT, &font[character * 40]);
}

void renderer_drawCharIcon(char character, int x, int y){
	renderer_drawImage(x, y - 1, ICON_W, ICON_H, &ICON[character * 60]);
}

void renderer_drawImage(uint32_t x, uint32_t y, uint32_t w, uint32_t h, const unsigned char* img){
	uint32_t idx = 0;
	uint8_t bitN = 0;
	for (int j = 0; j < h; j++){
		for (int i = 0; i < w; i++){
			if (bitN >= 8){
				idx++;			
				bitN = 0;
			}
			if (READ(img[idx], (7 - bitN)))
				fb_base[(y + j) * uiWidth + x + i] = color;
			bitN++;
		}
		if (bitN != 0){
			idx++;			
			bitN = 0;
		}
	}
}

bool readPixel(uint32_t x, uint32_t y, uint32_t w, uint32_t h, const unsigned char* img){
	uint32_t realW = ((w - 1) / 8 + 1) * 8;
	uint32_t idx = (realW * y + x) / 8;
	uint8_t bitN = 7 - ((realW * y + x) % 8);
	return READ(img[idx], bitN);
}

void drawPixelToFB(int32_t x, int32_t y, uint32_t color){
	if (x >= 0 && x < fbWidth && y >= 0 && y < fbHeight)
		ksceKernelMemcpyKernelToUser((uintptr_t)&fbfbBase_user[y * fbPitch + x], &color, sizeof(color));
}

void renderer_drawLine(int32_t x1, int32_t y1, int32_t x2, int32_t y2) {
	int32_t xinc, yinc, x, y;
	int32_t dx, dy, e;
	dx = abs(x2 - x1);
	dy = abs(y2 - y1);
	if (x1 < x2)
		xinc = 1;
	else
		xinc = -1;
	if (y1 < y2)
		yinc = 1;
	else
		yinc = -1;
	x = x1;
	y = y1;
	drawPixelToFB(x, y, color);
	if (dx >= dy) {
		e = (2 * dy) - dx;
		while (x != x2) {
			if (e < 0) {
				e += (2 * dy);
			} else {
				e += (2 * (dy - dx));
				y += yinc;
			}
			x += xinc;
			drawPixelToFB(x, y, color);
		}
	} else {
		e = (2 * dx) - dy;
		while (y != y2) {
			if (e < 0) {
				e += (2 * dx);
			} else {
				e += (2 * (dx - dy));
				x += xinc;
			}
			y += yinc;
			drawPixelToFB(x, y, color);
		}
	}
}

void renderer_drawLineThick(int32_t x1, int32_t y1, int32_t x2, int32_t y2, uint8_t thickness){
	int32_t wy, wx;
	renderer_drawLine(x1, y1, x2, y2);
	if (abs(x1 - x2) > abs(y1 - y2)){
		wy = thickness + floorSqrt(thickness * (((float) abs(y1 - y2)) / abs(x1 - x2)));
		for (int i = 0; i < wy; i++) {
			renderer_drawLine(x1, y1 - i, x2, y2 - i);
			renderer_drawLine(x1, y1 + i, x2, y2 + i);
		}
	} else {
		wx = thickness + floorSqrt(thickness * (((float) abs(x1 - x2)) / abs(y1 - y2)));
		for (int i = 0; i < wx; i++) {
			renderer_drawLine(x1 - i, y1, x2 - i, y2);
			renderer_drawLine(x1 + i, y1, x2 + i, y2);
		}
	}
}

void renderer_drawImageDirectlyToFB(int32_t x, int32_t y, uint32_t w, uint32_t h, const unsigned char* img){
	for (int j = 0; j < h; j++){
		for (int i = 0; i < w; i++)
			if (readPixel(i, j, w, h, img))
				drawPixelToFB(x + i, y + j, color);
	}
}

void clear() { 
    for (int i = 0; i < uiWidth * (CHA_H + 4); i++)
        fb_base[i] = 0x00000000;
    for (int i = uiWidth * (CHA_H + 4); i < uiWidth * uiHeight - uiWidth * (CHA_H + 4); i++)
        fb_base[i] = 0x00171717;
    for (int i = uiWidth * uiHeight - uiWidth * (CHA_H + 4); i < uiWidth * uiHeight; i++)
        fb_base[i] = 0x00000000;
}

void renderer_drawRectangle(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t clr) {
	if ((x + w) > uiWidth || (y + h) > uiHeight)
		return;
	for (int i = x; i < x + w; i++)
		for (int j = y; j < y + h; j++)
			fb_base[j * uiWidth + i] = clr;
}

uint32_t blendColor(uint32_t fg, uint32_t bg) {
	uint8_t* colorMain= (uint8_t*)&fg;
	uint8_t* colorBg= (uint8_t*)&bg;
    uint8_t inv_alpha = 255 - colorMain[0];

    uint32_t result;
	uint8_t* resP = (uint8_t*)&result;
    resP[1] = ((colorMain[0] * colorMain[1] + inv_alpha * colorBg[1]) >> 8); // B
    resP[2] = ((colorMain[0] * colorMain[2] + inv_alpha * colorBg[2]) >> 8); // G
    resP[3] = ((colorMain[0] * colorMain[3] + inv_alpha * colorBg[3]) >> 8); // R
    resP[0] = 0xFF;                                         // A
    return result;
}

void blendAlphaOverlay(){
    for (int i = 0; i < uiWidth * uiHeight; i++)
        fb_base[i] = blendColor(fb_base[i], 0xFF000000);
}

void makeAlphaBackground(){
	uint32_t ui_x = (max(fbWidth - uiWidth, 0)) / 2;
	uint32_t ui_y = (max(fbHeight - uiHeight, 0)) / 2;
	for (int i = 0; i < uiHeight; i++){
		uint32_t off = 0;
		if (i < UI_CORNER_RADIUS){
			off = UI_CORNER_OFF[i];
		} else if (i > uiHeight - UI_CORNER_RADIUS - 1) {
			off = UI_CORNER_OFF[uiHeight - i];
		}
		ksceKernelMemcpyUserToKernel(
			&fb_base[i * uiWidth + off],
			(uintptr_t)&fbfbBase_user[(ui_y + i) * fbWidth + ui_x + off],
			sizeof(uint32_t) * (uiWidth - 2 * off));		
	}
	blendAlphaOverlay();
}

void renderer_setFB(const SceDisplayFrameBuf *param){
	fbWidth = param->width;
	fbHeight = param->height;
	fbfbBase_user = param->base;
	fbPitch = param->pitch;
}

void renderer_writeToFB(int64_t tickOpened){
	int64_t tick = ksceKernelGetSystemTimeWide();

	uint32_t ui_x = (max(fbWidth - uiWidth, 0)) / 2;
	uint32_t ui_y = (max(fbHeight - uiHeight, 0)) / 2;

	float multiplyer = 0;
	if (ANIMATION_TIME >= tick - tickOpened)
		multiplyer = ((float)(ANIMATION_TIME - (int)(tick - tickOpened))) / ANIMATION_TIME;

	int32_t ui_yAnimated = ui_y - (uiHeight + ui_y) * multiplyer;
	uint32_t ui_yCalculated = max(ui_yAnimated, 0);
	uint32_t ui_cutout = ui_yAnimated >= 0 ? 0 : -ui_yAnimated;
		
	for (int i = 0; i < uiHeight - ui_cutout; i++){
		uint32_t off = 0;
		if (i < UI_CORNER_RADIUS){
			off = UI_CORNER_OFF[i];
		} else if (i > uiHeight - UI_CORNER_RADIUS - 1) {
			off = UI_CORNER_OFF[uiHeight - i];
		}
		ksceKernelMemcpyKernelToUser(
			(uintptr_t)&fbfbBase_user[(ui_yCalculated + i) * fbPitch + ui_x + off],
			&fb_base[(i + ui_cutout) * uiWidth + off],
			sizeof(uint32_t) * (uiWidth - 2 * off));		
	}
}

void renderer_setColor(uint32_t c){
	color = c;
}

void renderer_stripped(uint8_t flag){
	stripped = flag;
}

void renderer_drawString(int x, int y, const char *str){
    for (size_t i = 0; i < strlen(str); i++){
		if (str[i] == ' ') continue;
		if (str[i] == '$'){
			renderer_drawCharIcon(str[i+1], x + i * 12, y);
			i++;
		} else {
			renderer_drawChar(str[i], x + i * 12, y);
			// renderer_drawImage(x + i * 12, y, FONT_WIDTH, FONT_HEIGHT, &font[(unsigned char)str[i]*40]);
		}
	}
	if (stripped)
		renderer_drawRectangle(x, y + CHA_H / 2, (strlen(str)) * CHA_W, 2, color);
}

void renderer_drawStringF(int x, int y, const char *format, ...){
	char str[512] = { 0 };
	va_list va;

	va_start(va, format);
	vsnprintf(str, 512, format, va);
	va_end(va);

	renderer_drawString(x, y, str);
}

void renderer_init(uint32_t w, uint32_t h){
	uiWidth = w;
	uiHeight = h;
    fbuf_uid = ksceKernelAllocMemBlock("fb_base", SCE_KERNEL_MEMBLOCK_TYPE_KERNEL_RW, (uiWidth*uiHeight*sizeof(uint32_t)  + 0xfff) & ~0xfff, NULL);
    ksceKernelGetMemBlockBase(fbuf_uid, (void**)&fb_base);
    LOG("MEMORY ALLOC renderer %i : %i\n", (int)fb_base, (uiWidth*uiHeight*sizeof(uint32_t) + 0xfff) & ~0xfff);
}

void renderer_destroy(){
	ksceKernelFreeMemBlock(fbuf_uid);
}
