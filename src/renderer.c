#include <psp2kern/display.h> 
#include <psp2kern/kernel/sysmem.h> 
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "font.h"
#include "icons.h"
#include "renderer.h"
#include "common.h"
#include "log.h"

uint32_t color;
uint8_t stripped;

SceUID fbuf_uid;
uint32_t* fb_base;
uint32_t* fbfbBase_user;
uint32_t fbWidth, fbHeight, fbPitch;
uint32_t uiWidth, uiHeight;

#define UI_CORNER_RADIUS 9
static const unsigned char UI_CORNER_OFF[UI_CORNER_RADIUS] = {9, 7, 5, 4, 3, 2, 2, 1, 1};

//Drawn character in 2x scaling
void renderer_drawIcon(char character, int x, int y){
	uint32_t idx = 0;
	switch (character){
		case 'X': idx = ICON_BTN_CROSS; break;
		case 'S': idx = ICON_BTN_SQUARE; break;
		case 'T': idx = ICON_BTN_TRIANGLE; break;
		case 'C': idx = ICON_BTN_CIRCLE; break;
		case 'P': idx = ICON_BTN_PS; break;
		case ';': idx = ICON_BTN_SELECT; break;
		case ':': idx = ICON_BTN_START; break;
		case '^': idx = ICON_BTN_UP; break;
		case 'v': idx = ICON_BTN_DONW; break;
		case '<': idx = ICON_BTN_LEFT; break;
		case '>': idx = ICON_BTN_RIGHT; break;
		case '[': idx = ICON_BTN_LT; break;
		case '{': idx = ICON_BTN_L1; break;
		case '(': idx = ICON_BTN_L2; break;
		case ',': idx = ICON_BTN_L3; break;
		case ']': idx = ICON_BTN_RT; break;
		case '}': idx = ICON_BTN_R1; break;
		case ')': idx = ICON_BTN_R2; break;
		case '.': idx = ICON_BTN_R3; break;
		case 'p': idx = ICON_BTNPOWER; break;
		case '+': idx = ICON_BTN_VOLUP; break;
		case '-': idx = ICON_BTN_VOLDOWN; break;
		case 'l': idx = ICON_RS_LEFT; break;
		case 'r': idx = ICON_RS_RIGHT; break;
		case 'u': idx = ICON_RS_UP; break;
		case 'd': idx = ICON_RS_DOWN; break;
		case 'L': idx = ICON_LS_LEFT; break;
		case 'R': idx = ICON_LS_RIGHT; break;
		case 'U': idx = ICON_LS_UP; break;
		case 'D': idx = ICON_LS_DOWN; break;
		case 'F': idx = ICON_FT; break;
		case '1': idx = ICON_FT_L; break;
		case '2': idx = ICON_FT_R; break;
		case '3': idx = ICON_FT_TL; break;
		case '4': idx = ICON_FT_TR; break;
		case '5': idx = ICON_FT_BL; break;
		case '6': idx = ICON_FT_BR; break;
		case 'B': idx = ICON_BT; break;
		case '7': idx = ICON_BT_L; break;
		case '8': idx = ICON_BT_R; break;
		case '9': idx = ICON_BT_TL; break;
		case '0': idx = ICON_BT_TR; break;
		case '_': idx = ICON_BT_BL; break;
		case '=': idx = ICON_BT_BR; break;
		case 'q': idx = ICON_GY_LEFT; break;
		case 'e': idx = ICON_GY_RIGHT; break;
		case 'w': idx = ICON_GY_UP; break;
		case 's': idx = ICON_GY_DOWN; break;
		case 'Q': idx = ICON_GY_ROLLLEFT; break;
		case 'E': idx = ICON_GY_ROLLRIGHT; break;
		case 'o': idx = ICON_PSV_LEFT; break;
		case 'O': idx = ICON_PSV_RIGHT; break;
		case 't': idx = ICON_BTN_DS4TOUCH; break;
		default: break;
	}
	renderer_drawImage(x + 1, y, ICON_W, ICON_H, &ICON[idx * 54]);
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

void renderer_drawImageDirectlyToFB(uint32_t x, uint32_t y, uint32_t w, uint32_t h, const unsigned char* img){
	uint32_t idx = 0;
	uint8_t bitN = 0;
	uint32_t cache[w];
	for (int j = 0; j < h; j++){
		ksceKernelMemcpyUserToKernel(&cache[0],
			(uintptr_t)&fbfbBase_user[(j + y) * fbPitch + x],
			sizeof(uint32_t) * w);
		for (int i = 0; i < w; i++){
			if (bitN >= 8){
				idx++;			
				bitN = 0;
			}
			if (READ(img[idx], (7 - bitN)))
				cache[i] = color; 
			bitN++;
		}
		ksceKernelMemcpyKernelToUser((uintptr_t)&fbfbBase_user[(j + y) * fbPitch + x],
			&cache[0],
			sizeof(uint32_t) * w);
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

void renderer_writeToFB(){
	uint32_t ui_x = (max(fbWidth - uiWidth, 0)) / 2;
	uint32_t ui_y = (max(fbHeight - uiHeight, 0)) / 2;
	for (int i = 0; i < uiHeight; i++){
		uint32_t off = 0;
		if (i < UI_CORNER_RADIUS){
			off = UI_CORNER_OFF[i];
		} else if (i > uiHeight - UI_CORNER_RADIUS - 1) {
			off = UI_CORNER_OFF[uiHeight - i];
		}
		ksceKernelMemcpyKernelToUser(
			(uintptr_t)&fbfbBase_user[(ui_y + i) * fbPitch + ui_x + off],
			&fb_base[i * uiWidth + off],
			sizeof(uint32_t) * (uiWidth - 2 * off));		
	}
}

void renderer_setColor(uint32_t clr){
	color = clr;
}

//Drawn character in 2x scaling
void renderer_drawCharacter(int character, int x, int y){
    for (int yy = 0; yy < 10; yy++) {
        int xDisplacement = x;
        int yDisplacement = (y + (yy<<1)) * uiWidth;
        uint32_t* screenPos = (uint32_t*)(fb_base + xDisplacement + yDisplacement);
        uint8_t charPos = font[character * 10 + yy];
        for (int xx = 7; xx >= 2; xx--) {
			if (screenPos > (uint32_t*)(fb_base + uiHeight * uiWidth)) break;
			if ((charPos >> xx) & 1) {
				*(screenPos) = color;
				*(screenPos+1) = color;
				*(screenPos+uiWidth) = color;
				*(screenPos+uiWidth+1) = color;	
			}	
			screenPos += 2;
        }
    }
}

void renderer_stripped(uint8_t flag){
	stripped = flag;
}

void renderer_drawString(int x, int y, const char *str){
    for (size_t i = 0; i < strlen(str); i++){
		if (str[i] == ' ') continue;
		if (str[i] == '$'){
			renderer_drawIcon(str[i+1], x + i * 12, y);
			i++;
		} else {
        	renderer_drawCharacter(str[i], x + i * 12, y);
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
	LOG("ui SceUID %i\n", fbuf_uid);
	log_flush();
    ksceKernelGetMemBlockBase(fbuf_uid, (void**)&fb_base);
}

void renderer_destroy(){
	ksceKernelFreeMemBlock(fbuf_uid);
}
