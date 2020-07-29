#include <vitasdkkern.h>

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

#define COLOR_DEFAULT     	0x00C2C0BD
#define COLOR_CURSOR      	0x00FFFFFF
#define COLOR_HEADER      	0x00FF6600
#define COLOR_CURSOR_HEADER	0x00FFAA22
#define COLOR_ACTIVE      	0x0000B0B0
#define COLOR_CURSOR_ACTIVE	0x0000DDDD
#define COLOR_DANGER      	0x00000099
#define COLOR_CURSOR_DANGER	0x000000DD
#define COLOR_BG_HEADER   	0x00000000
#define COLOR_BG_BODY     	0x00171717
#define L_0    				5		//Left margin for menu
#define L_1    				18		
#define L_2    				36

static uint32_t ticker;
static uint32_t menuY = 0;

const char* str_btn_small[PHYS_BUTTONS_NUM] = {
	"X", "O", "T", "S", "-", "+", "L", "R", "^", ">", "<", "v", "l", "r", "{", "}"
};
const char* str_remap_type[REMAP_ACTION_TYPE_NUM] = {
	"BTN", "LAN", "LAD", "RAN", "RAD", "FTZ", "BTZ", "FTP", "BTP", "GYR"
};
const char* str_remap_action[REMAP_ACTION_NUM] = {
	"UP", "DN", "LT", "RT", 
	"TL", "TR", "BL", "BR", "CU", 
	"UP", "DN", "LT", "RT", "RL", "RT"
};
static const char* str_yes_no[] = {
	"No", "Yes"
};
static const char* str_btns[PHYS_BUTTONS_NUM] = {
	"Cross", "Circle", "Triangle", "Square",
	"Start", "Select", 
	"LT/L2", "RT/R2",
	"Up", "Right", "Left", "Down", 
	"L1", "R1", "L3", "R3"
};
static char* str_deadband[] = {
	"Game default", "Enable", "Disable"
};
char* getControllerName(int id){
	if 		(id == 	SCE_CTRL_TYPE_UNPAIRED) return "Unpaired";
	else if (id == 	SCE_CTRL_TYPE_PHY) 		return "Physical VITA";
	else if (id == 	SCE_CTRL_TYPE_VIRT) 	return "Virtual PSTV";
	else if (id == 	SCE_CTRL_TYPE_DS3) 		return "DualShock 3";
	else if (id == 	SCE_CTRL_TYPE_DS4) 		return "DualShock 4";
	else 									return "Unknown";
}
void generateBtnComboName(char* str, uint32_t btns){
	int i = -1;
	char tmp[6] = "";
	while (++i < PHYS_BUTTONS_NUM && strlen(tmp) < 4)
		if (btn_has(btns, HW_BUTTONS[i])) 
			strcat(tmp, str_btn_small[i]);
	if (strlen(tmp) == 4)
		strcat(tmp, "~");
	strcat(str, tmp);
}
void generateRemapRuleName(char* str, struct RemapRule* rule){
	strcpy(str, "");
    strcat(str, rule->propagate ? "P " : "  ");
	switch (rule->trigger.type){
		case REMAP_TYPE_BUTTON :
			strcat(str, str_remap_type[rule->trigger.type]);
			strcat(str, "{");
			generateBtnComboName(str, rule->trigger.param.btn);
			strcat(str, "}");
			break;
		default:
			strcat(str, str_remap_type[rule->trigger.type]);
			strcat(str, "{");
			strcat(str, str_remap_action[rule->trigger.action]);
			strcat(str, "}");
			break;
		}
	strcat(str, " -> ");
	switch (rule->emu.type){
		case REMAP_TYPE_BUTTON :
			strcat(str, str_remap_type[rule->emu.type]);
			strcat(str, "{");
			generateBtnComboName(str, rule->emu.param.btn);
			strcat(str, "}");
			break;
		default:
			strcat(str, str_remap_type[rule->emu.type]);
			strcat(str, "{");
			strcat(str, str_remap_action[rule->emu.action]);
			strcat(str, "}");
			break;
		}
}

//Calculate starting index for scroll menu
int calcStartingIndex(int idx, int entriesNum, int screenEntries, int bottomOffset){
	int ret = max(0, idx - (screenEntries - bottomOffset - 1));
	while (ret > 0 && (entriesNum - ret) < screenEntries) ret--;
	return ret;
}
void setColorHeader(uint8_t isCursor){
	if (isCursor){
		renderer_setColor(COLOR_CURSOR_HEADER);
	} else {
		renderer_setColor(COLOR_HEADER);
	}
}
void setColor(uint8_t isCursor, uint8_t isDefault){
	if (isCursor){
		if (isDefault)
			renderer_setColor(COLOR_CURSOR);
		else 
			renderer_setColor(COLOR_CURSOR_ACTIVE);
	} else {
		if (isDefault)
			renderer_setColor(COLOR_DEFAULT);
		else 
			renderer_setColor(COLOR_ACTIVE);
	}
}
void drawScroll(int8_t up, int8_t down){
	renderer_setColor(COLOR_HEADER);
	if (up)
		renderer_drawImage(UI_WIDTH - ICN_ARROW_X - 3, HEADER_HEIGHT + 3, ICN_ARROW_X, ICN_ARROW_Y, ICN_ARROW_UP);
	if (down)
		renderer_drawImage(UI_WIDTH - ICN_ARROW_X - 3, UI_HEIGHT - HEADER_HEIGHT - ICN_ARROW_X - 3, ICN_ARROW_X, ICN_ARROW_Y, ICN_ARROW_DOWN);
}
void drawFullScroll(int8_t up, int8_t down, float pos){
	if (!up && !down) return;
	renderer_setColor(COLOR_HEADER);
	uint16_t calculatedPos = HEADER_HEIGHT + 4 + ICN_ARROW_Y + pos * (UI_HEIGHT - 2 * HEADER_HEIGHT - 2 * ICN_ARROW_Y - ICN_SLIDER_Y - 8);
	renderer_drawImage(UI_WIDTH - ICN_ARROW_X - 3, calculatedPos, ICN_SLIDER_X, ICN_SLIDER_Y, ICN_SLIDER);
	drawScroll(1,1);
}
void drawEditPointer(uint16_t x, uint16_t y){
	renderer_drawImage(x, y, ICN_ARROW_X, ICN_ARROW_Y, (ticker % 32 < 16) ? ICN_ARROW_LEFT : ICN_ARROW_RIGHT);
}

void drawHeader(){
	renderer_drawRectangle(0, 0, UI_WIDTH, HEADER_HEIGHT - 1, COLOR_BG_HEADER);//BG
	renderer_drawRectangle(0, HEADER_HEIGHT - 1, UI_WIDTH, 1, COLOR_HEADER);//Separator
	renderer_setColor(COLOR_HEADER);
	if (ui_menu->id == MENU_MAIN_ID){
		renderer_drawStringF(L_0, 3, "remaPSV2 v.%s", VERSION);
		renderer_drawString(UI_WIDTH - CHA_W * strlen(titleid) - 10, 2, titleid);
	} else	
		renderer_drawString(L_0, 3, ui_menu->name);
	
}
void drawFooter(){
	renderer_drawRectangle(0, UI_HEIGHT - HEADER_HEIGHT, UI_WIDTH, 1, COLOR_HEADER);//Separator
	renderer_drawRectangle(0, UI_HEIGHT - (HEADER_HEIGHT - 1), UI_WIDTH, HEADER_HEIGHT - 1, COLOR_BG_HEADER);//BG
	renderer_setColor(COLOR_HEADER);   
	if (ui_menu->footer)                                                            
		renderer_drawStringF(L_0, UI_HEIGHT-HEADER_HEIGHT + 4, ui_menu->footer);
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
			setColor(i == ui_menu->idx, profile.remaps[ui_menu->entries[i].data].disabled);
			renderer_stripped(profile.remaps[ui_menu->entries[i].data].disabled);
			renderer_drawString(L_1, y += CHA_H, ui_menu->entries[i].name);
			renderer_stripped(0);
		}
	}
	drawFullScroll(ii > 0, ii + ui_lines < ui_menu->num, ((float)ui_menu->idx) / (ui_menu->num - 1));
}
void onDraw_pickButton(){
    int y = menuY;
	int ii = calcStartingIndex(ui_menu->idx, ui_menu->num, ui_lines, BOTTOM_OFFSET);
	for (int i = ii; i < min(ii + ui_lines, ui_menu->num); i++) {
		uint32_t btns = *(uint32_t*)ui_menu->data;
		setColor(i == ui_menu->idx, !btn_has(btns, HW_BUTTONS[ui_menu->entries[i].data]));
		renderer_drawString(L_1, y += CHA_H, str_btns[ui_menu->entries[i].data]);
	}
}
void onDraw_analog(){
    int y = menuY;
	int ii = calcStartingIndex(ui_menu->idx, ui_menu->num , ui_lines, BOTTOM_OFFSET);
	for (int i = ii; i < min(ii + ui_lines, ui_menu->num); i++) {			
		int8_t id = ui_menu->entries[i].data;

		if (id == HEADER_IDX){
			setColorHeader(ui_menu->idx == i);
			renderer_drawString(L_1, y+=CHA_H, ui_menu->entries[i].name);
		} else if (id < 4){
			setColor(i == ui_menu->idx, profile.analog[id] == profile_def.analog[id]);
			renderer_drawStringF(L_2, y += CHA_H, "%s: %hhu", 
					ui_menu->entries[i].name, profile.analog[id]);
		} else if (id != HEADER_IDX) {
			setColor(i == ui_menu->idx, profile.analog[id] == profile_def.analog[id]);
			renderer_drawStringF(L_2, y += CHA_H, "%s: %s", 
					ui_menu->entries[i].name, str_yes_no[profile.analog[id]]);
		}

		if (ui_menu->idx == i && (id != HEADER_IDX)){//Draw cursor
			renderer_setColor(COLOR_CURSOR);
			renderer_drawString(L_2 + 17*CHA_W, y, (ticker % 16 < 8) ? "<" : ">");
		}
	}
	drawFullScroll(ii > 0, ii + ui_lines < ui_menu->num, ((float)ui_menu->idx) / (ui_menu->num-1));
}
void onDraw_touch(){
    int y = menuY;
	int ii = calcStartingIndex(ui_menu->idx, ui_menu->num , ui_lines, BOTTOM_OFFSET);
	for (int i = ii; i < min(ii + ui_lines, ui_menu->num); i++) {			
		int8_t id = ui_menu->entries[i].data;

		if (id == HEADER_IDX){
			setColorHeader(ui_menu->idx == i);
			renderer_drawString(L_1, y+=CHA_H, ui_menu->entries[i].name);
		} else if (id == 16 || id == 17){
			setColor(i == ui_menu->idx, profile.touch[id] == profile_def.touch[id]);
			renderer_drawStringF(L_2, y += CHA_H, "%s: %s", 
					ui_menu->entries[i].name, str_yes_no[profile.touch[id]]);
		} else if (id != HEADER_IDX) {
			setColor(i == ui_menu->idx, profile.touch[id] == profile_def.touch[id]);
			renderer_drawStringF(L_2, y += CHA_H, "%s: %hu", 
					ui_menu->entries[i].name, profile.touch[id]);
		}

		if (ui_menu->idx == i && (id != HEADER_IDX)){//Draw cursor
			renderer_setColor(COLOR_CURSOR);
			renderer_drawString(L_2+ 20*CHA_W, y, (ticker % 16 < 8) ? "<" : ">");
		}
	}
	drawFullScroll(ii > 0, ii + ui_lines < ui_menu->num, ((float)ui_menu->idx) / (ui_menu->num - 1));
}
void onDraw_pickTouchPoint(){
    int y = menuY;
	int ii = calcStartingIndex(ui_menu->idx, ui_menu->num , ui_lines, BOTTOM_OFFSET);	
	RemapAction* ra = (RemapAction*)ui_menu->data;
	for (int i = ii; i < min(ii + ui_lines, ui_menu->num); i++) {	
		int8_t id = ui_menu->entries[i].data;
		int coord = (id == 0) ? ra->param.touch.x : ra->param.touch.y;
		setColor(i == ui_menu->idx, 1);
		//LOG("%i", coord);
		renderer_drawStringF(L_2, y += CHA_H, "%s: %hu", ui_menu->entries[i].name, coord);
		if (ui_menu->idx == i && (id != HEADER_IDX)){//Draw cursor
			renderer_setColor(COLOR_CURSOR);
			renderer_drawString(L_2+ 20*CHA_W, y, (ticker % 16 < 8) ? "<" : ">");
		}
	}
	drawFullScroll(ii > 0, ii + ui_lines < ui_menu->num, ((float)ui_menu->idx) / (ui_menu->num - 1));
}
void onDraw_pickTouchZone(){
    int y = menuY;
	int ii = calcStartingIndex(ui_menu->idx, ui_menu->num , ui_lines, BOTTOM_OFFSET);	
	RemapAction* ra = (RemapAction*)ui_menu->data;
	for (int i = ii; i < min(ii + ui_lines, ui_menu->num); i++) {	
		int8_t id = ui_menu->entries[i].data;
		int coord = 0;
		switch (id){
			case 0: coord = ra->param.zone.a.x; break;
			case 1: coord = ra->param.zone.a.y; break;
			case 2: coord = ra->param.zone.b.x; break;
			case 3: coord = ra->param.zone.b.y; break;}
		setColor(i == ui_menu->idx, 1);
		renderer_drawStringF(L_2, y += CHA_H, "%s: %hu", ui_menu->entries[i].name, coord);

		if (ui_menu->idx == i && (id != HEADER_IDX)){//Draw cursor
			renderer_setColor(COLOR_CURSOR);
			renderer_drawString(L_2+ 20*CHA_W, y, (ticker % 16 < 8) ? "<" : ">");
		}
	}
	drawFullScroll(ii > 0, ii + ui_lines < ui_menu->num, ((float)ui_menu->idx) / (ui_menu->num - 1));
}
void onDraw_gyro(){
    int y = menuY;
	int ii = calcStartingIndex(ui_menu->idx, ui_menu->num , ui_lines, BOTTOM_OFFSET);
	for (int i = ii; i < min(ii + ui_lines, ui_menu->num); i++) {		
		int8_t id = ui_menu->entries[i].data;
		
		if (id == HEADER_IDX){
			setColorHeader(ui_menu->idx == i);
			renderer_drawString(L_1, y+=CHA_H, ui_menu->entries[i].name);
		} else if (id < 6){//Draw sens and deadzone option
			setColor(i == ui_menu->idx, profile.gyro[id] == profile_def.gyro[id]);
			renderer_drawStringF(L_2, y += CHA_H, "%s: %hhu", ui_menu->entries[i].name, profile.gyro[id]);
		} else if (id == 6) {//Draw deadband option
			setColor(i == ui_menu->idx, profile.gyro[id] == profile_def.gyro[id]);
			renderer_drawStringF(L_2, y += CHA_H, "%s: %s", ui_menu->entries[i].name, str_deadband[profile.gyro[id]]);
		} else if (id == 7) {//Draw wheel option
			setColor(i == ui_menu->idx, profile.gyro[id] == profile_def.gyro[id]);
			renderer_drawStringF(L_2, y += CHA_H, "%s: %s", ui_menu->entries[i].name, str_yes_no[profile.gyro[id]]);
		}

		if (ui_menu->idx == i && (id != HEADER_IDX)) {//Draw cursor
			renderer_setColor(COLOR_CURSOR);
			renderer_drawString(L_2 + 11 * CHA_W, y, (ticker % 16 < 8) ? "<" : ">");
		}
	}
	drawFullScroll(ii > 0, ii + ui_lines < ui_menu->num, ((float)ui_menu->idx)/(ui_menu->num-1));
}
void onDraw_controller(){
    int y = menuY;
	SceCtrlPortInfo pi;
	int res = ksceCtrlGetControllerPortInfo(&pi);
	if (res != 0){//Should not ever trigger
		renderer_setColor(COLOR_DANGER);
		renderer_drawString(L_1, y+= CHA_H, "Error getting controllers info");
		return;
	}
	
	int ii = calcStartingIndex(ui_menu->idx, ui_menu->num , ui_lines, BOTTOM_OFFSET);
	for (int i = ii; i < min(ii + ui_lines, ui_menu->num); i++) {		
		int8_t id = ui_menu->entries[i].data;

		setColor(i == ui_menu->idx, profile.controller[id] == profile_def.controller[id]);
		if (id == 0 || id == 2)//Use external controller / buttons swap
			renderer_drawStringF(L_1, y += CHA_H, "%s: %s", ui_menu->entries[i].name, str_yes_no[profile.controller[id]]);
		else if (id == 1){//Port selection
			renderer_drawStringF(L_1, y += CHA_H, "%s: {%i} %s", ui_menu->entries[i].name, profile.controller[id],
					getControllerName(pi.port[profile.controller[1]]));
		}

		if (ui_menu->idx == i && (id != HEADER_IDX)) {//Draw cursor
			renderer_setColor(COLOR_CURSOR);
			renderer_drawString(L_1 + 20 * CHA_W, y, (ticker % 16 < 8) ? "<" : ">");
		}
	}

	//Ports stats
	y+=CHA_H;
	renderer_setColor(COLOR_HEADER);
	renderer_drawString(L_1, y+= CHA_H, "Detected controllers:");
	for (int i = 0; i < 5; i++){
		renderer_setColor((pi.port[i] == SCE_CTRL_TYPE_UNPAIRED) ? COLOR_DANGER : COLOR_ACTIVE);
		renderer_drawStringF(L_2, y += CHA_H, "Port %i: %s", i, getControllerName(pi.port[i]));
	}
}
void onDraw_hooks(){
    int y = menuY;
	int ii = calcStartingIndex(ui_menu->idx, ui_menu->num, ui_lines, ui_lines - 1);
	for (int i = ii; i < min(ii + ui_lines, ui_menu->num); i++) {
		renderer_setColor((used_funcs[ui_menu->entries[i].data] ? COLOR_ACTIVE : COLOR_DEFAULT));
		renderer_drawStringF(L_1, y += CHA_H, "%s : %s", ui_menu->entries[i].name, str_yes_no[used_funcs[ui_menu->entries[i].data]]);
	}
	drawFullScroll(ii > 0, ii + ui_lines < ui_menu->num, 
			((float)ui_menu->idx)/(ui_menu->num - (ui_lines - 1) - 1));
}
void onDraw_settings(){
    int y = menuY;
	int ii = calcStartingIndex(ui_menu->idx, ui_menu->num , ui_lines, BOTTOM_OFFSET);
	for (int i = ii; i < min(ii + ui_lines, ui_menu->num); i++) {		
		int8_t id = ui_menu->entries[i].data;
		
		setColor(i == ui_menu->idx, profile_settings[id] == profile_settings_def[id]);
		if (id < 2){//Draw opening buttons
			renderer_drawStringF(L_1, y += CHA_H, "%s: %s", ui_menu->entries[i].name, str_btns[profile_settings[id]]);
		} else if (id == 2) {//Draw Save profile on close
			renderer_drawStringF(L_1, y += CHA_H, "%s: %s", ui_menu->entries[i].name, str_yes_no[profile_settings[id]]);
		} else if (id == 3) {//Startup delay
			renderer_drawStringF(L_1, y += CHA_H, "%s: %hhu", ui_menu->entries[i].name, profile_settings[id]);
		}

		if (ui_menu->idx == i && (id != HEADER_IDX)) {//Draw cursor
			renderer_setColor(COLOR_CURSOR);
			renderer_drawString(L_1 + 23 * CHA_W, y, (ticker % 16 < 8) ? "<" : ">");
		}
	}
	drawFullScroll(ii > 0, ii + ui_lines < ui_menu->num, ((float)ui_menu->idx) / (ui_menu->num-1));
}
void onDraw_profiles(){
    int y = menuY;
	int ii = calcStartingIndex(ui_menu->idx, ui_menu->num, ui_lines, BOTTOM_OFFSET);
	for (int i = ii; i < min(ii + ui_lines, ui_menu->num); i++) {
		if (ui_menu->entries[i].data == HEADER_IDX){
			setColorHeader(ui_menu->idx == i);
			renderer_drawString(L_1, y+=CHA_H, ui_menu->entries[i].name);
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
		renderer_setColor(COLOR_DEFAULT);
		renderer_drawString(L_0, y += CHA_H, ui_menu->entries[i].name);
	}
	drawFullScroll(ii > 0, ii + ui_lines < ui_menu->num, 
			((float)ui_menu->idx)/(ui_menu->num - (ui_lines - 1) - 1));
}

void drawTouchPointer(uint32_t panel, TouchPoint* tp){
	int8_t ic_halfsize = ICN_TOUCH_X / 2;
	int left = tp->x - 8;
	left *= (float)fbWidth / ((panel == SCE_TOUCH_PORT_FRONT) ? T_FRONT_SIZE.x : T_BACK_SIZE.x);
	left = min((max(ic_halfsize, left)), fbWidth - ic_halfsize);
	int top = tp->y - 10;
	top *= (float)fbHeight / ((panel == SCE_TOUCH_PORT_FRONT) ? T_FRONT_SIZE.y : T_BACK_SIZE.y); //Scale to framebuffer size
	top = min((max(ic_halfsize, top)), fbHeight - ic_halfsize);//limit into screen
	renderer_setColor((ticker % 8 < 4) ? COLOR_DANGER : COLOR_HEADER);
	renderer_drawImageDirectlyToFB(left - ic_halfsize, top - ic_halfsize, 64, 64, ICN_TOUCH);
}

void drawTouchZone(uint32_t panel, TouchZone* tz){
	//ToDo draw rectangle
	drawTouchPointer(panel, &(TouchPoint){.x = tz->a.x, .y = tz->a.y});
	drawTouchPointer(panel, &(TouchPoint){.x = tz->b.x, .y = tz->b.y});
}


void drawBody() {
	renderer_drawRectangle(0, HEADER_HEIGHT, UI_WIDTH, UI_HEIGHT -  2 * HEADER_HEIGHT, COLOR_BG_BODY);//BG
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
			ra = (RemapAction*) ui_menu->data;
			uint32_t panel = (ra->type == REMAP_TYPE_FRONT_TOUCH_POINT || ra->type == REMAP_TYPE_FRONT_TOUCH_ZONE) ?
				SCE_TOUCH_PORT_FRONT : SCE_TOUCH_PORT_BACK;
			if (ui_menu->id == MENU_PICK_TOUCH_POINT_ID) drawTouchPointer(panel, &ra->param.touch); 
			else drawTouchZone(panel, &ra->param.zone); 
			break;
		default: break;
	}
}

void ui_draw(const SceDisplayFrameBuf *pParam){
	if (ui_opened) {
		new_frame = 1;
		ticker++;
		renderer_setFB(pParam);
		drawHeader();
		drawBody();
		drawFooter();
		renderer_writeToFB();
		drawDirectlyToFB();	
	}
}

void ui_draw_init(){
    renderer_init(UI_WIDTH, UI_HEIGHT);
}
void ui_draw_destroy(){
	renderer_destroy();
}



