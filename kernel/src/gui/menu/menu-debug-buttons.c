#include <vitasdkkern.h>
#include "../../main.h"
#include "../../vitasdkext.h"
#include "../../common.h"
#include "../../fio/settings.h"
#include "../../fio/theme.h"
#include "../gui.h"
#include "../renderer.h"

void onDraw_debugButtons(unsigned int menuY){
	uint32_t arr[16] = {
		0x00010000,
		0x00020000,
		0x00040000,
		0x00080000,
		0x00100000,
		0x00200000,
		0x00400000,
		0x00800000,
		0x01000000,
		0x02000000,
		0x04000000,
		0x08000000,
		0x10000000,
		0x20000000,
		0x40000000,
		0x80000000
	};
	SceCtrlData ctrl;
	int ret = ksceCtrlPeekBufferPositive(0, &ctrl, 1);
    int y = menuY;
    int x = L_1;
	// SceCtrlData* ctrlP = gui_menu->dataPtr;
	// if (ctrlP != NULL)
	// 	ctrl = *ctrlP;
	unsigned int buttons = ctrl.buttons;
	if (ret < 1){
		renderer_setColor(theme[COLOR_DANGER]);
		renderer_drawString(L_1, y += CHA_H, "ERROR READING INPUT");
		return;
	}
	renderer_setColor(theme[COLOR_DEFAULT]);
	renderer_drawStringF(L_1, y += CHA_H, "Port: {%i}", 0);
	y += CHA_H;
	for(int i = 0; i < 16; i++){
		if (i == 8){
			y += CHA_H;
			x = L_1;
		}
		gui_setColor(0, !btn_has(buttons, HW_BUTTONS[i]));
		renderer_drawString(x += CHA_W*4, y, STR_BTN_S[i]);
	}
    x = L_1;
	y+= CHA_H;
	for(int i = 0; i < 16; i++){
		if (i == 8){
			y += CHA_H;
			x = L_1;
		}
		gui_setColor(0, !btn_has(buttons, arr[i]));
		switch (arr[i]){
			case SCE_CTRL_PSBUTTON: renderer_drawString(x += CHA_W*4, y, "$P");break;
			case SCE_CTRL_POWER: renderer_drawStringF(x += CHA_W*4, y, "$p");break;
			case SCE_CTRL_VOLUP: renderer_drawStringF(x += CHA_W*4, y, "$+");break;
			case SCE_CTRL_VOLDOWN: renderer_drawStringF(x += CHA_W*4, y, "$-");break;
			case SCE_CTRL_TOUCHPAD: renderer_drawStringF(x += CHA_W*4, y, "$t");break;
			default: renderer_drawStringF(x += CHA_W*4, y, "%i", i + 16); break;
		}
	}
	renderer_setColor(theme[COLOR_DEFAULT]);
	renderer_drawStringF(L_1, y += CHA_H, "LT: %i, RT: %i, reserved0 : [%i, %i, %i, %i]", 
		ctrl.lt, ctrl.rt,
		ctrl.reserved0[0], ctrl.reserved0[1], ctrl.reserved0[2], ctrl.reserved0[3]);
	renderer_drawStringF(L_1, y += CHA_H, "reserved1 : [%i, %i, %i, %i, %i,", 
		ctrl.reserved1[0], ctrl.reserved1[1], ctrl.reserved1[2], ctrl.reserved1[3], ctrl.reserved1[4]);
	renderer_drawStringF(L_1, y += CHA_H, "             %i, %i, %i, %i, %i]", 
		ctrl.reserved1[5], ctrl.reserved1[6], ctrl.reserved1[7], ctrl.reserved1[8], ctrl.reserved1[9]);
	renderer_drawStringF(L_1, y += CHA_H, "Analogs : [%i, %i] [%i, %i]", 
		ctrl.lx, ctrl.ly, ctrl.rx, ctrl.ry);
}

void onInput_debugButtons(SceCtrlData *ctrl){
	gui_menu->dataPtr = &ctrl[0];
}

#define MENU_DEBUG_BUTTONS_NUM			4
static struct MenuEntry menu_debug_buttons_entries[MENU_DEBUG_BUTTONS_NUM];
static struct Menu menu_debug_buttons = (Menu){
	.id = MENU_DEBUG_BUTTONS_ID, 
	.parent = MENU_MAIN_ID,
	.num = MENU_DEBUG_BUTTONS_NUM, 
	.name = "$b BUTTONS INFO", 
	.noIndent = true,
	.onDraw = onDraw_debugButtons,
	.onInput = onInput_debugButtons,
	.entries = menu_debug_buttons_entries};

void menu_initDebugButtons(){
	gui_registerMenu(&menu_debug_buttons);
}