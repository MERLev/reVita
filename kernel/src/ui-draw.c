#include <vitasdkkern.h>
#include "vitasdkext.h"
#include <inttypes.h>
#include "main.h"
#include "renderer.h"
#include "icons.h"
#include "ui.h"
#include "ui-draw.h"
#include "profile.h"
#include "log.h"

#define VERSION				"2.1.0"

#define UI_WIDTH            480
#define UI_HEIGHT           272
#define HEADER_HEIGHT		(CHA_H + 6)
#define BOTTOM_OFFSET		5
#define L_0    				5		//Left margin for menu
#define L_1    				18		
#define L_2    				36

static uint32_t ticker;
static uint32_t menuY = 0;

//Active emulated touchpoints
static EmulatedTouch etFront, etBack;

const char* str_btn_small[HW_BUTTONS_NUM] = {
	"$X", "$C", "$T", "$S", "$:", "$;", "$[", "$]", 
	"$^", "$>", "$<", "$v", "${", "$}", "$(", "$)", 
	"$+", "$-", "$p", "$P", "$t"
};
static const char* str_yes_no[] = {
	"No", "Yes"
};
char* str_btns[HW_BUTTONS_NUM] = {
	"$X Cross", "$C Circle", "$T Triangle", "$S Square",
	"$: Start", "$; Select", 
	"$[ LT/L2", "$] RT/R2",
	"$^ Up", "$> Right", "$< Left", "$v Down", 
	"${ L1", "$} R1", "$( L3", "$) R3",
	"$+ Volume UP", "$- Volume DOWN", "$p POWER", "$P PS",
	"$t DS4 Touchpad"
};
static char* str_deadband[] = {
	"Game default", "Enable", "Disable"
};
const char* str_theme[THEME__NUM] = {
	"DARK", "LIGHT"
};
char* getControllerName(int id){
	if 		(id == 	SCE_CTRL_TYPE_UNPAIRED) return "Unpaired";
	else if (id == 	SCE_CTRL_TYPE_PHY) 		return "Physical VITA";
	else if (id == 	SCE_CTRL_TYPE_VIRT) 	return "Virtual PSTV";
	else if (id == 	SCE_CTRL_TYPE_DS3) 		return "DualShock 3";
	else if (id == 	SCE_CTRL_TYPE_DS4) 		return "DualShock 4";
	else 									return "Unknown";
}
void generateBtnComboName(char* str, uint32_t btns, int max){
	int i = -1;
	int counter = 0;
	while (++i < HW_BUTTONS_NUM){
		if (btn_has(btns, HW_BUTTONS[i])) {
			if (counter >=  max){
				str[strlen(str) - 2] = '.';
				str[strlen(str) - 1] = '.';
				break;
			} else {
				counter++;
				strcat(str, str_btn_small[i]);
			}
		}
	}
}
void generateRemapActionName(char* str, struct RemapAction* ra){
	bool front  = false;
	switch (ra->type){
		case REMAP_TYPE_BUTTON: generateBtnComboName(str, ra->param.btn, 6);
			break;
		case REMAP_TYPE_LEFT_ANALOG:
		case REMAP_TYPE_LEFT_ANALOG_DIGITAL:
			switch (ra->action){
				case REMAP_ANALOG_UP:    strcat(str, "$U"); break;
				case REMAP_ANALOG_DOWN:  strcat(str, "$D"); break;
				case REMAP_ANALOG_LEFT:  strcat(str, "$L"); break;
				case REMAP_ANALOG_RIGHT: strcat(str, "$R"); break;
				default: break;
			}
			break;
		case REMAP_TYPE_RIGHT_ANALOG:
		case REMAP_TYPE_RIGHT_ANALOG_DIGITAL:
			switch (ra->action){
				case REMAP_ANALOG_UP:    strcat(str, "$u"); break;
				case REMAP_ANALOG_DOWN:  strcat(str, "$d"); break;
				case REMAP_ANALOG_LEFT:  strcat(str, "$l"); break;
				case REMAP_ANALOG_RIGHT: strcat(str, "$r"); break;
				default: break;
			}
			break;
		case REMAP_TYPE_FRONT_TOUCH_POINT:
		case REMAP_TYPE_FRONT_TOUCH_ZONE:
			front = true;
		case REMAP_TYPE_BACK_TOUCH_ZONE:
		case REMAP_TYPE_BACK_TOUCH_POINT:
			strcat(str, front ? "$F" : "$B");
			switch (ra->action){
				case REMAP_TOUCH_ZONE_L:           strcat(str, "$i$1"); break;
				case REMAP_TOUCH_ZONE_R:           strcat(str, "$i$2"); break;
				case REMAP_TOUCH_ZONE_TL:          strcat(str, "$i$3"); break;
				case REMAP_TOUCH_ZONE_TR:          strcat(str, "$i$4"); break;
				case REMAP_TOUCH_ZONE_BL:          strcat(str, "$i$5"); break;
				case REMAP_TOUCH_ZONE_BR:          strcat(str, "$i$6"); break;
				case REMAP_TOUCH_CUSTOM:           strcat(str, "$i"); break;
				case REMAP_TOUCH_SWIPE:            strcat(str, "$j"); break;
				case REMAP_TOUCH_SWIPE_SMART_DPAD: strcat(str, "$j$x"); break;
				case REMAP_TOUCH_SWIPE_SMART_L:    strcat(str, "$j$U"); break;
				case REMAP_TOUCH_SWIPE_SMART_R:    strcat(str, "$j$u"); break;
				default: break;
			}
			break;
		case REMAP_TYPE_GYROSCOPE:
			switch (ra->action){
				case REMAP_GYRO_LEFT:  		strcat(str, "$q"); break;
				case REMAP_GYRO_RIGHT: 		strcat(str, "$e"); break;
				case REMAP_GYRO_UP:    		strcat(str, "$w"); break;
				case REMAP_GYRO_DOWN:  		strcat(str, "$s"); break;
				case REMAP_GYRO_ROLL_LEFT:  strcat(str, "$Q"); break;
				case REMAP_GYRO_ROLL_RIGHT: strcat(str, "$E"); break;
				default: break;
			}
		case REMAP_TYPE_SYSACTIONS: strcat(str, "$!");
		default: break;
	}
}
void generateRemapRuleName(char* str, struct RemapRule* rule){
	strclone(str, "");
    strcat(str, rule->propagate ? "P " : "  ");
	generateRemapActionName(str, &rule->trigger);
	strcat(str, "->");
	generateRemapActionName(str, &rule->emu);
}

//Calculate starting index for scroll menu
int calcStartingIndex(int idx, int entriesNum, int screenEntries, int bottomOffset){
	int ret = max(0, idx - (screenEntries - bottomOffset - 1));
	while (ret > 0 && (entriesNum - ret) < screenEntries) ret--;
	return ret;
}
void setColorHeader(uint8_t isCursor){
	if (isCursor){
		renderer_setColor(theme[COLOR_CURSOR_HEADER]);
	} else {
		renderer_setColor(theme[COLOR_HEADER]);
	}
}
void setColor(uint8_t isCursor, uint8_t isDefault){
	if (isCursor){
		if (isDefault) renderer_setColor(theme[COLOR_CURSOR_DEFAULT]);
		else renderer_setColor(theme[COLOR_CURSOR_ACTIVE]);
	} else {
		if (isDefault) renderer_setColor(theme[COLOR_DEFAULT]);
		else renderer_setColor(theme[COLOR_ACTIVE]);
	}
}
void setColorExt(uint8_t isCursor, uint8_t isDefault, uint8_t isDisabled){
	if (isCursor){
		if     (isDisabled) renderer_setColor(theme[COLOR_CURSOR_DANGER]);
		else if (isDefault) renderer_setColor(theme[COLOR_CURSOR_DEFAULT]);
		else                renderer_setColor(theme[COLOR_CURSOR_ACTIVE]);
	} else {
		if     (isDisabled) renderer_setColor(theme[COLOR_DANGER]);
		else if (isDefault) renderer_setColor(theme[COLOR_DEFAULT]);
		else                renderer_setColor(theme[COLOR_ACTIVE]);
	}
}
void drawStringFRight(int x, int y, const char *format, ...){
	char str[512] = { 0 };
	va_list va;

	va_start(va, format);
	vsnprintf(str, 512, format, va);
	va_end(va);

	renderer_drawString(UI_WIDTH - (strlen(str) + 2) * CHA_W - x, y, str);
}
void drawScroll(int8_t up, int8_t down){
	renderer_setColor(theme[COLOR_HEADER]);
	if (up)
		renderer_drawImage(UI_WIDTH - ICN_ARROW_X - 3, HEADER_HEIGHT + 3, ICN_ARROW_X, ICN_ARROW_Y, ICN_ARROW_UP);
	if (down)
		renderer_drawImage(UI_WIDTH - ICN_ARROW_X - 3, UI_HEIGHT - HEADER_HEIGHT - ICN_ARROW_X - 3, ICN_ARROW_X, ICN_ARROW_Y, ICN_ARROW_DOWN);
}
void drawFullScroll(int8_t up, int8_t down, float pos){
	if (!up && !down) return;
	renderer_setColor(theme[COLOR_HEADER]);
	uint16_t calculatedPos = HEADER_HEIGHT + 4 + ICN_ARROW_Y + pos * (UI_HEIGHT - 2 * HEADER_HEIGHT - 2 * ICN_ARROW_Y - ICN_SLIDER_Y - 8);
	renderer_drawImage(UI_WIDTH - ICN_ARROW_X - 3, calculatedPos, ICN_SLIDER_X, ICN_SLIDER_Y, ICN_SLIDER);
	drawScroll(1,1);
}
void drawEditPointer(uint16_t x, uint16_t y){
	renderer_drawImage(x, y, ICN_ARROW_X, ICN_ARROW_Y, (ticker % 32 < 16) ? ICN_ARROW_LEFT : ICN_ARROW_RIGHT);
}

void drawHeader(){
	renderer_drawRectangle(0, 0, UI_WIDTH, HEADER_HEIGHT - 1, theme[COLOR_BG_HEADER]);//BG
	renderer_drawRectangle(0, HEADER_HEIGHT - 1, UI_WIDTH, 1, theme[COLOR_HEADER]);//Separator
	renderer_setColor(theme[COLOR_HEADER]);
	if (ui_menu->id == MENU_MAIN_ID){
		renderer_drawStringF(L_0, 3, "$P remaPSV2 v.%s", VERSION);
		renderer_drawString(UI_WIDTH - CHA_W * strlen(titleid) - 10, 2, titleid);
	} else	
		renderer_drawString(L_0, 3, ui_menu->name);
	
}
void drawFooter(){
	renderer_drawRectangle(0, UI_HEIGHT - HEADER_HEIGHT, UI_WIDTH, 1, theme[COLOR_HEADER]);//Separator
	renderer_drawRectangle(0, UI_HEIGHT - (HEADER_HEIGHT - 1), UI_WIDTH, HEADER_HEIGHT - 1, theme[COLOR_BG_HEADER]);//BG
	renderer_setColor(theme[COLOR_HEADER]);   
	if (ui_menu->footer)                                                            
		renderer_drawStringF(L_0, UI_HEIGHT-HEADER_HEIGHT + 4, ui_menu->footer);
}
void drawIndent(){
	int y = (ui_menu->idx - calcStartingIndex(ui_menu->idx, ui_menu->num, ui_lines, BOTTOM_OFFSET)) * CHA_H
		+ HEADER_HEIGHT + CHA_H / 2;
	renderer_drawRectangle(L_1 - 5, y - 1, UI_WIDTH - 2 * L_1 + 10, CHA_H + 2, theme[COLOR_BG_HEADER]);//BG
	renderer_setColor(theme[COLOR_HEADER]);   
}

void onDraw_generic(){
    int y = menuY;
	int ii = calcStartingIndex(ui_menu->idx, ui_menu->num, ui_lines, BOTTOM_OFFSET);
	for (int i = ii; i < min(ii + ui_lines, ui_menu->num); i++) {
		setColor(i == ui_menu->idx, 1);
		renderer_drawString(L_1, y += CHA_H, ui_menu->entries[i].name);
	}
}

void onDraw_remap(){
    int y = menuY;
	int ii = calcStartingIndex(ui_menu->idx, ui_menu->num, ui_lines, BOTTOM_OFFSET);
	for (int i = ii; i < min(ii + ui_lines, ui_menu->num); i++) {
		if (ui_menu->entries[i].data == NEW_RULE_IDX){
			setColor(i == ui_menu->idx, 1);
			renderer_drawString(L_1, y += CHA_H, ui_menu->entries[i].name);
		} else {
			struct RemapRule* rr = &profile.remaps[ui_menu->entries[i].data];
			setColorExt(i == ui_menu->idx, true, !rr->propagate || rr->disabled);
			char str[20] = "";
			generateRemapActionName(str, &rr->trigger);
			int len = strlen(str) * CHA_W;
			renderer_drawString(L_1, y += CHA_H, str);
			if (!rr->disabled){
				renderer_setColor(theme[COLOR_DEFAULT]);
				renderer_drawString(L_1 + len + CHA_W, y, ">");
			} else {
				renderer_setColor(theme[COLOR_DANGER]);
				renderer_drawString(L_1 + len + CHA_W, y, "X");
			}
			str[0] = '\0';
			generateRemapActionName(str, &rr->emu);
			setColorExt(i == ui_menu->idx, true, rr->disabled);
			renderer_drawString(L_1 + len + 3*CHA_W, y, str);
			if (rr->turbo)
				renderer_drawString(UI_WIDTH - 4*CHA_W, y, "$M");
		}
	}
	drawFullScroll(ii > 0, ii + ui_lines < ui_menu->num, ((float)ui_menu->idx) / (ui_menu->num - 1));
}
void onDraw_pickButton(){
    int y = menuY;
	int ii = calcStartingIndex(ui_menu->idx, ui_menu->num, ui_lines, BOTTOM_OFFSET);
	uint32_t btns = *(uint32_t*)ui_menu->data;
	for (int i = ii; i < min(ii + ui_lines, ui_menu->num); i++) {
		setColor(i == ui_menu->idx, !btn_has(btns, HW_BUTTONS[ui_menu->entries[i].data]));
		renderer_drawString(L_1, y += CHA_H, str_btns[ui_menu->entries[i].data]);
	}
}
void onDraw_analog(){
    int y = menuY;
	int ii = calcStartingIndex(ui_menu->idx, ui_menu->num , ui_lines, BOTTOM_OFFSET);
	for (int i = ii; i < min(ii + ui_lines, ui_menu->num); i++) {			
		int32_t id = ui_menu->entries[i].data;

		if (ui_menu->entries[i].type == HEADER_TYPE){
			setColorHeader(ui_menu->idx == i);
			renderer_drawString(L_1, y+=CHA_H, ui_menu->entries[i].name);
		} else if (id < 4){
			setColor(i == ui_menu->idx, profile.analog[id] == profile_def.analog[id]);
			renderer_drawString(L_1, y += CHA_H, ui_menu->entries[i].name);
			drawStringFRight(0, y, "%hhu", profile.analog[id]);
		}
	}
	drawFullScroll(ii > 0, ii + ui_lines < ui_menu->num, ((float)ui_menu->idx) / (ui_menu->num-1));
}
void onDraw_touch(){
    int y = menuY;
	int ii = calcStartingIndex(ui_menu->idx, ui_menu->num , ui_lines, BOTTOM_OFFSET);
	for (int i = ii; i < min(ii + ui_lines, ui_menu->num); i++) {			
		int32_t id = ui_menu->entries[i].data;

		if (ui_menu->entries[i].type == HEADER_TYPE){
			setColorHeader(ui_menu->idx == i);
			renderer_drawString(L_1, y+=CHA_H, ui_menu->entries[i].name);
		} else if (id == PROFILE_TOUCH_SWAP || id == PROFILE_TOUCH_PSTV_MODE || 
					id == PROFILE_TOUCH_DRAW_POINTER_POINT || 
					id == PROFILE_TOUCH_DRAW_POINTER_SWIPE || 
					id == PROFILE_TOUCH_DRAW_POINTER_SMART_SWIPE){
			setColor(i == ui_menu->idx, profile.touch[id] == profile_def.touch[id]);
			renderer_drawString(L_1, y += CHA_H, ui_menu->entries[i].name);
			drawStringFRight(0, y, "%s", str_yes_no[profile.touch[id]]);
		} else if (id == PROFILE_TOUCH_SWIPE_DURATION || id == PROFILE_TOUCH_SWIPE_SMART_SENSIVITY){
			setColor(i == ui_menu->idx, profile.touch[id] == profile_def.touch[id]);
			renderer_drawString(L_1, y += CHA_H, ui_menu->entries[i].name);
			drawStringFRight(0, y, "%i", profile.touch[id]);
		}
	}
	drawFullScroll(ii > 0, ii + ui_lines < ui_menu->num, ((float)ui_menu->idx) / (ui_menu->num - 1));
}
void onDraw_pickTouchPoint(){
    int y = menuY;
	int ii = calcStartingIndex(ui_menu->idx, ui_menu->num , ui_lines, BOTTOM_OFFSET);	
	RemapAction* ra = (RemapAction*)ui_menu->data;
	for (int i = ii; i < min(ii + ui_lines, ui_menu->num); i++) {	
		int32_t id = ui_menu->entries[i].data;
		int coord = (id == 0) ? ra->param.tPoint.x : ra->param.tPoint.y;
		setColor(i == ui_menu->idx, 1);
		renderer_drawStringF(L_2, y += CHA_H, "%s: %hu", ui_menu->entries[i].name, coord);
	}
	drawFullScroll(ii > 0, ii + ui_lines < ui_menu->num, ((float)ui_menu->idx) / (ui_menu->num - 1));
}
void onDraw_pickTouchZone(){
    int y = menuY;
	int ii = calcStartingIndex(ui_menu->idx, ui_menu->num , ui_lines, BOTTOM_OFFSET);	
	RemapAction* ra = (RemapAction*)ui_menu->data;
	for (int i = ii; i < min(ii + ui_lines, ui_menu->num); i++) {	
		int32_t id = ui_menu->entries[i].data;
		int coord = 0;
		switch (id){
			case 0: coord = ra->param.tPoints.a.x; break;
			case 1: coord = ra->param.tPoints.a.y; break;
			case 2: coord = ra->param.tPoints.b.x; break;
			case 3: coord = ra->param.tPoints.b.y; break;}
		setColor(i == ui_menu->idx, 1);
		renderer_drawStringF(L_2, y += CHA_H, "%s: %hu", ui_menu->entries[i].name, coord);
	}
	drawFullScroll(ii > 0, ii + ui_lines < ui_menu->num, ((float)ui_menu->idx) / (ui_menu->num - 1));
}
void onDraw_gyro(){
    int y = menuY;
	int ii = calcStartingIndex(ui_menu->idx, ui_menu->num , ui_lines, BOTTOM_OFFSET);
	for (int i = ii; i < min(ii + ui_lines, ui_menu->num); i++) {		
		int32_t id = ui_menu->entries[i].data;
		
		if (ui_menu->entries[i].type == HEADER_TYPE){
			setColorHeader(ui_menu->idx == i);
			renderer_drawString(L_1, y+=CHA_H, ui_menu->entries[i].name);
		} else if (id < 6){//Draw sens and deadzone option
			setColor(i == ui_menu->idx, profile.gyro[id] == profile_def.gyro[id]);
			renderer_drawString(L_1, y += CHA_H, ui_menu->entries[i].name);
			drawStringFRight(0, y, "%hhu", profile.gyro[id]);
		} else if (id == 6) {//Draw deadband option
			setColor(i == ui_menu->idx, profile.gyro[id] == profile_def.gyro[id]);
			renderer_drawString(L_1, y += CHA_H, ui_menu->entries[i].name);
			drawStringFRight(0, y, "%s", str_deadband[profile.gyro[id]]);
		} else if (id == 7) {//Draw wheel option
			setColor(i == ui_menu->idx, profile.gyro[id] == profile_def.gyro[id]);
			renderer_drawString(L_1, y += CHA_H, ui_menu->entries[i].name);
			drawStringFRight(0, y, "%s", str_yes_no[profile.gyro[id]]);
		}
	}
	drawFullScroll(ii > 0, ii + ui_lines < ui_menu->num, ((float)ui_menu->idx)/(ui_menu->num-1));
}
void onDraw_controller(){
    int y = menuY;
	SceCtrlPortInfo pi;
	int res = ksceCtrlGetControllerPortInfo(&pi);
	if (res != 0){//Should not ever trigger
		renderer_setColor(theme[COLOR_DANGER]);
		renderer_drawString(L_1, y+= CHA_H, "Error getting controllers info");
		return;
	}
	
	int ii = calcStartingIndex(ui_menu->idx, ui_menu->num , ui_lines, BOTTOM_OFFSET);
	for (int i = ii; i < min(ii + ui_lines, ui_menu->num); i++) {		
		int32_t id = ui_menu->entries[i].data;

		setColor(i == ui_menu->idx, profile.controller[id] == profile_def.controller[id]);
		renderer_drawString(L_1, y += CHA_H, ui_menu->entries[i].name);
		if (id == 0 || id == 2){//Use external controller / buttons swap
			drawStringFRight(0, y, "%s", str_yes_no[profile.controller[id]]);
		} else if (id == 1){//Port selection
			drawStringFRight(0, y, "%s {%i}", getControllerName(pi.port[profile.controller[1]]), profile.controller[id]);
		}
	}

	//Ports stats
	y+=CHA_H;
	renderer_setColor(theme[COLOR_HEADER]);
	renderer_drawString(L_1, y+= CHA_H, "Detected controllers:");
	for (int i = 0; i < 5; i++){
		renderer_setColor((pi.port[i] == SCE_CTRL_TYPE_UNPAIRED) ? theme[COLOR_DANGER] : theme[COLOR_ACTIVE]);
		renderer_drawStringF(L_2, y += CHA_H, "Port %i: %s", i, getControllerName(pi.port[i]));
	}
}
void onDraw_hooks(){
    int y = menuY;
	int ii = calcStartingIndex(ui_menu->idx, ui_menu->num, ui_lines, ui_lines - 1);
	for (int i = ii; i < min(ii + ui_lines, ui_menu->num); i++) {
		renderer_setColor((used_funcs[ui_menu->entries[i].data] ? theme[COLOR_ACTIVE] : theme[COLOR_DEFAULT]));
		renderer_drawStringF(L_1, y += CHA_H, ui_menu->entries[i].name);
		drawStringFRight(0, y, "%s", str_yes_no[used_funcs[ui_menu->entries[i].data]]);
	}
	drawFullScroll(ii > 0, ii + ui_lines < ui_menu->num, 
			((float)ui_menu->idx)/(ui_menu->num - (ui_lines - 1) - 1));
}
void onDraw_debugButtons(){
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
	isInternalExtCall = 1;
	int ret = ksceCtrlPeekBufferPositive(profile.controller[1], &ctrl, 1);
	isInternalExtCall = 0;
    int y = menuY;
    int x = L_1;
	SceCtrlData* ctrlP = (SceCtrlData*)ui_menu->data;
	if (ctrlP != NULL)
		ctrl = *ctrlP;
	unsigned int buttons = ctrl.buttons;
	if (ret < 1){
		renderer_setColor(theme[COLOR_DANGER]);
		renderer_drawString(L_1, y += CHA_H, "ERROR READING INPUT");
		return;
	}
	renderer_setColor(theme[COLOR_DEFAULT]);
	renderer_drawStringF(L_1, y += CHA_H, "Port: {%i}", profile.controller[1]);
	y += CHA_H;
	for(int i = 0; i < 16; i++){
		if (i == 8){
			y += CHA_H;
			x = L_1;
		}
		setColor(0, !btn_has(buttons, HW_BUTTONS[i]));
		renderer_drawString(x += CHA_W*4, y, str_btn_small[i]);
	}
    x = L_1;
	y+= CHA_H;
	for(int i = 0; i < 16; i++){
		if (i == 8){
			y += CHA_H;
			x = L_1;
		}
		setColor(0, !btn_has(buttons, arr[i]));
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
	renderer_drawStringF(L_1, y += CHA_H, "Timestamp: %"PRIu64, 
		ctrl.timeStamp);

}
void onDraw_settings(){
    int y = menuY;
	int ii = calcStartingIndex(ui_menu->idx, ui_menu->num , ui_lines, BOTTOM_OFFSET);
	for (int i = ii; i < min(ii + ui_lines, ui_menu->num); i++) {		
		int32_t id = ui_menu->entries[i].data;
		
		setColor(i == ui_menu->idx, profile_settings[id] == profile_settings_def[id]);
		renderer_drawString(L_1, y += CHA_H, ui_menu->entries[i].name);
		switch (id){
			case PROFILE_SETTINGS_KEY1:
			case PROFILE_SETTINGS_KEY2:
				drawStringFRight(0, y, "%s", str_btns[profile_settings[id]]);
				break;
			case PROFILE_SETTINGS_AUTOSAVE:
				drawStringFRight(0, y, "%s", str_yes_no[profile_settings[id]]);
				break;
			case PROFILE_SETTINGS_DELAY:
				drawStringFRight(0, y, "%hhu", profile_settings[id]);
				break;
			case PROFILE_SETTINGS_THEME:
				drawStringFRight(0, y, "%s", str_theme[profile_settings[id]]);
				break;
			default: break;
		}
	}
	drawFullScroll(ii > 0, ii + ui_lines < ui_menu->num, ((float)ui_menu->idx) / (ui_menu->num-1));
}
void onDraw_profiles(){
    int y = menuY;
	int ii = calcStartingIndex(ui_menu->idx, ui_menu->num, ui_lines, BOTTOM_OFFSET);
	for (int i = ii; i < min(ii + ui_lines, ui_menu->num); i++) {
		if (ui_menu->entries[i].type == HEADER_TYPE){
				setColorHeader(ui_menu->idx == i);
			if (i == 0){
				renderer_drawStringF(L_1, y+=CHA_H, "%s [%s]", 
					ui_menu->entries[i].name, profile.titleid);
			} else {
				renderer_drawString(L_1, y+=CHA_H, ui_menu->entries[i].name);
			}
		} else {
			setColor(i == ui_menu->idx, 1);
			renderer_drawString(L_2, y += CHA_H, ui_menu->entries[i].name);
		}
	}
}
void onDraw_credits(){
    int y = menuY;
	int ii = calcStartingIndex(ui_menu->idx, ui_menu->num, ui_lines, ui_lines - 1);
	for (int i = ii; i < min(ui_menu->num, ii + ui_lines); i++) {	
		renderer_setColor(theme[COLOR_DEFAULT]);
		renderer_drawString(L_0, y += CHA_H, ui_menu->entries[i].name);
	}
	drawFullScroll(ii > 0, ii + ui_lines < ui_menu->num, 
			((float)ui_menu->idx)/(ui_menu->num - (ui_lines - 1) - 1));
}

void drawTouchPointer(uint32_t panel, TouchPoint* tp){
	int left = tp->x - 8;
	left *= (float)fbWidth / ((panel == SCE_TOUCH_PORT_FRONT) ? T_FRONT_SIZE.x : T_BACK_SIZE.x);
	int top = tp->y - 10;
	top *= (float)fbHeight / ((panel == SCE_TOUCH_PORT_FRONT) ? T_FRONT_SIZE.y : T_BACK_SIZE.y); //Scale to framebuffer size
	renderer_setColor(theme[COLOR_TOUCH]);
	renderer_drawImageDirectlyToFB(left - 8, top - 1, ICN_TOUCH_X, ICN_TOUCH_Y, ICN_TOUCH);
}

void drawEmulatedPointersForPanel(uint32_t panel){
	EmulatedTouch* et = &etBack;
	if (panel == SCE_TOUCH_PORT_FRONT)
		et = &etFront;
	for (int i = 0; i < et->num; i++)
		if (et->reports[i].isSwipe){
			if (profile.touch[PROFILE_TOUCH_DRAW_POINTER_SWIPE])
				drawTouchPointer(SCE_TOUCH_PORT_FRONT, &et->reports[i].swipeCurrentPoint);
		} else if (et->reports[i].isSmartSwipe){
			if (profile.touch[PROFILE_TOUCH_DRAW_POINTER_SMART_SWIPE])
				drawTouchPointer(SCE_TOUCH_PORT_FRONT, &et->reports[i].swipeEndPoint);
		} else {
			if (profile.touch[PROFILE_TOUCH_DRAW_POINTER_POINT])
				drawTouchPointer(SCE_TOUCH_PORT_FRONT, &et->reports[i].point);
		}
}

void drawTouchZone(uint32_t panel, TouchPoints2* tz){
	//ToDo draw rectangle
	renderer_setColor(theme[COLOR_TOUCH]);
	renderer_drawLineThick(tz->a.x / 2, tz->a.y / 2, tz->a.x / 2, tz->b.y / 2, 3); //Vertical 1
	renderer_drawLineThick(tz->a.x / 2, tz->a.y / 2, tz->b.x / 2, tz->a.y / 2, 3); //Horizontal 1
	renderer_drawLineThick(tz->a.x / 2, tz->b.y / 2, tz->b.x / 2, tz->b.y / 2, 3); //Vertical 2
	renderer_drawLineThick(tz->b.x / 2, tz->a.y / 2, tz->b.x / 2, tz->b.y / 2, 3); //Horizontal 2
	drawTouchPointer(panel, &(TouchPoint){.x = tz->a.x, .y = tz->a.y});
	drawTouchPointer(panel, &(TouchPoint){.x = tz->b.x, .y = tz->b.y});
}
void drawTouchSwipe(uint32_t panel, TouchPoints2* tz){
	//ToDo draw rectangle
	renderer_setColor(theme[COLOR_TOUCH]);
	renderer_drawLineThick(tz->a.x / 2, tz->a.y / 2, tz->b.x / 2, tz->b.y / 2, 3);
	drawTouchPointer(panel, &(TouchPoint){.x = tz->a.x, .y = tz->a.y});
	drawTouchPointer(panel, &(TouchPoint){.x = tz->b.x, .y = tz->b.y});
}

void drawBody() {
	renderer_drawRectangle(0, HEADER_HEIGHT, UI_WIDTH, UI_HEIGHT -  2 * HEADER_HEIGHT, theme[COLOR_BG_BODY]);//BG
	if (!ui_menu->noIndent)
		drawIndent();
	//Draw menu
	menuY = HEADER_HEIGHT - CHA_H / 2;
	ui_lines = ((float)(UI_HEIGHT - 2 * HEADER_HEIGHT)) / CHA_H - 1;
    
    if (ui_menu->onDraw)
        ui_menu->onDraw();
    else
        onDraw_generic();
}

//Draw directly to FB to overlay over everything else;
void drawDirectlyToFB(){
	RemapAction* ra;
	switch (ui_menu->id){
		case MENU_PICK_TOUCH_POINT_ID: 
		case MENU_PICK_TOUCH_ZONE_ID: 
		case MENU_PICK_TOUCH_SWIPE_ID: 
			ra = (RemapAction*) ui_menu->data;
			uint32_t panel = (ra->type == REMAP_TYPE_FRONT_TOUCH_POINT || ra->type == REMAP_TYPE_FRONT_TOUCH_ZONE) ?
				SCE_TOUCH_PORT_FRONT : SCE_TOUCH_PORT_BACK;
			if (ui_menu->id == MENU_PICK_TOUCH_POINT_ID) 
				drawTouchPointer(panel, &ra->param.tPoint); 
			else if (ui_menu->id == MENU_PICK_TOUCH_SWIPE_ID) 
				drawTouchSwipe(panel, &ra->param.tPoints); 
			else if (ui_menu->id == MENU_PICK_TOUCH_ZONE_ID) 
				drawTouchZone(panel, &ra->param.tPoints); 
			break;
		default: break;
	}
}

void ui_draw(const SceDisplayFrameBuf *pParam){
	new_frame = 1;
	ticker++;
	if (ui_opened) {
		renderer_setFB(pParam);
		drawHeader();
		drawBody();
		drawFooter();
		renderer_writeToFB(tickUIOpen);
		drawDirectlyToFB();	
	} else {
		renderer_setFB(pParam);
		drawEmulatedPointersForPanel(SCE_TOUCH_PORT_FRONT);
		drawEmulatedPointersForPanel(SCE_TOUCH_PORT_BACK);
	}
}

void ui_updateEmulatedTouch(SceTouchPortType panel, EmulatedTouch et){
	if (panel == SCE_TOUCH_PORT_FRONT)
		etFront = et;
	else 
		etBack = et;
}

void ui_draw_init(){
    renderer_init(UI_WIDTH, UI_HEIGHT);
}
void ui_draw_destroy(){
	renderer_destroy();
}


