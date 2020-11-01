#include <vitasdkkern.h>
#include <stdio.h>
#include "../../main.h"
#include "../../vitasdkext.h"
#include "../../common.h"
#include "../../fio/settings.h"
#include "../../fio/theme.h"
#include "../gui.h"
#include "../renderer.h"
#include "../rendererv.h"

const char* getBtnName(uint32_t btn){
	static char str[3];
	for (uint i = 0; i < HW_BUTTONS_NUM; i++)
		if (HW_BUTTONS[i] == btn)
			return STR_BTN_S[i];
	for (uint i = 0; i < 32; i++)
		if (1U << i == btn)
			sprintf(str, "%i", i);
	return &str[0];
}

static int port = 0;
static SceCtrlData scdPrev;
void onDraw_debugButtons(uint menuY){
	SceCtrlData ctrl;
	int ret = ksceCtrlPeekBufferPositive2_internal(port, &ctrl, 1);
    int y = menuY;
    int x = L_1;
	uint buttons = ctrl.buttons;
	renderer_setColor(theme[COLOR_HEADER]);
	rendererv_drawStringF(L_1, y += CHA_H, "             Port: [%i]", port);
	if (ret < 1){
		renderer_setColor(theme[COLOR_DANGER]);
		rendererv_drawString(L_1, y += CHA_H, "ERROR READING INPUT");
		return;
	}
	for (int i = 0; i < 32; i++){
		if (i % 8 == 0){
			y += CHA_H;
			x = L_1;
		}
		gui_setColor(0, !btn_has(buttons, 1 << i));
		rendererv_drawString(x += CHA_W*4, y, getBtnName(1 << i));
	}
	renderer_setColor(theme[COLOR_DEFAULT]);
	rendererv_drawStringF(L_1, y += CHA_H, "$[: %i, $]: %i", 
		ctrl.lt, ctrl.rt);
	rendererv_drawStringF(L_1, y += CHA_H, "$U: [%i, %i], $u[%i, %i]", 
		ctrl.lx, ctrl.ly, ctrl.rx, ctrl.ry);
	rendererv_drawStringF(L_1, y += CHA_H, "reserved0 : [%i, %i, %i, %i]", 
		ctrl.reserved0[0], ctrl.reserved0[1], ctrl.reserved0[2], ctrl.reserved0[3]);
	rendererv_drawStringF(L_1, y += CHA_H, "reserved1 : [%i, %i, %i, %i, %i,", 
		ctrl.reserved1[0], ctrl.reserved1[1], ctrl.reserved1[2], ctrl.reserved1[3], ctrl.reserved1[4]);
	rendererv_drawStringF(L_1, y += CHA_H, "             %i, %i, %i, %i, %i]", 
		ctrl.reserved1[5], ctrl.reserved1[6], ctrl.reserved1[7], ctrl.reserved1[8], ctrl.reserved1[9]);
}

bool isBtnClicked(uint data, uint dataPrev, uint btn){
	return btn_has(data, btn) && !btn_has(dataPrev, btn);
}

void onInput_debugButtons(SceCtrlData *ctrl){
	gui_menu->dataPtr = &ctrl[0];
	if (isBtnClicked(ctrl->buttons, scdPrev.buttons, SCE_CTRL_SQUARE | SCE_CTRL_DOWN))
		gui_openMenuPrev();
	if (isBtnClicked(ctrl->buttons, scdPrev.buttons, SCE_CTRL_SQUARE | SCE_CTRL_LEFT))
		port = clamp(port - 1, 0, 4);
	if (isBtnClicked(ctrl->buttons, scdPrev.buttons, SCE_CTRL_SQUARE | SCE_CTRL_RIGHT))
		port = clamp(port + 1, 0, 4);
	scdPrev = ctrl[0];
}

#define MENU_DEBUG_BUTTONS_NUM			4
static struct MenuEntry menu_debug_buttons_entries[MENU_DEBUG_BUTTONS_NUM];
static struct Menu menu_debug_buttons = (Menu){
	.id = MENU_DEBUG_BUTTONS_ID, 
	.parent = MENU_MAIN_DEVELOPER_ID,
	.name = "$b BUTTONS INFO", 
	.footer = "$S+$<$> CHANGE PORT          $S+$v BACK",
	.noIndent = true,
	.onDraw = onDraw_debugButtons,
	.onInput = onInput_debugButtons,
	.onButton = onButton_null,
	.num = MENU_DEBUG_BUTTONS_NUM, 
	.entries = menu_debug_buttons_entries};

void menu_initDebugButtons(){
	gui_registerMenu(&menu_debug_buttons);
}