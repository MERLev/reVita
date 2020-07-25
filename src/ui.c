#include <vitasdkkern.h>
#include <psp2/motion.h> 
#include <psp2/touch.h> 
#include <psp2kern/ctrl.h> 
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "vitasdkext.h"
#include "main.h"
#include "renderer.h"
#include "icons.h"
#include "ui.h"
#include "profile.h"
#include "common.h"
#include "log.h"

#define VERSION				2
#define SUBVERSION			1
#define SUBSUBVERSION		0

#define UI_WIDTH 480
#define UI_HEIGHT 272

#define COLOR_DEFAULT     0x00C2C0BD
#define COLOR_CURSOR      0x00FFFFFF
#define COLOR_HEADER      0x00FF6600
#define COLOR_ACTIVE      0x0000FFFF
#define COLOR_DANGER     0x000000FF
#define L_0    5		//Left margin for text
#define L_1    18		
#define L_2    36

static const char* str_menus[MENU_MODES] = {
	"MAIN MENU", 
	"REMAP RULES", 
	"ANALOG STICKS", 
	"TOUCH", 
	"GYROSCOPE", 
	"CONNECTED CONTROLLERS", 
	"HOOKS",
	"SETTINGS",
	"CREDITS"
};
static const char* str_footer[MENU_MODES] = {
	"",
	"(L)(R):section ([])/(start):reset/all", 
	"([]):reset  (start):reset all", 
	"[TOUCH](RS):change  ([])/(start):reset", 
	"([]):reset  (start):reset all", 
	"([]):reset  (start):reset all", 
	"",
	"([]):reset  (start):reset all",
	""
};
static const char* str_main_menu[MENU_MODES-1] = {
	"Remap rules",
	"Analog sticks",
	"Touch",
	"Gyroscope",
	"External gamepads",
	"[DEBUG] Show hooks",
	"Settings",
	"Credits"
};

static const char* str_credits[CREDITS_NUM] = {
	"                     updated by Mer1e ",
	"               with the help of S1ngy ",
	"       original author Rinnegatamante ",
	"",
	"Thanks to",
	"  Cassie, W0lfwang, TheIronUniverse,",
	"  Kiiro Yakumo and mantixero",
	"     for enduring endless crashes",
	"         while testing this thing",
	"  Vita Nuova community",
	"    for all the help I got there",
	"",
	"Original Rinnegatamante's thanks to", 
	"  Tain Sueiras, nobodywasishere and",
	"  RaveHeart for their awesome",
	"    support on Patreon"
};
static const char* str_yes_no[] = {
	"No", "Yes"
};
static char* str_settings[] = {
	"Save as Game profile", 
	"Load Game profile", 
	"Save as Global profile", 
	"Load Global profile"
};
static char* str_funcs[HOOKS_NUM-1] = {
	"sceCtrlPeekBufferPositive",
	"sceCtrlPeekBufferPositive2",
	"sceCtrlReadBufferPositive",
	"sceCtrlReadBufferPositive2",
	"sceCtrlPeekBufferPositiveExt",
	"sceCtrlPeekBufferPositiveExt2",
	"sceCtrlReadBufferPositiveExt",
	"sceCtrlReadBufferPositiveExt2",
	"sceCtrlPeekBufferNegative",
	"sceCtrlPeekBufferNegative2",
	"sceCtrlReadBufferNegative",
	"sceCtrlReadBufferNegative2",
	"sceTouchRead",
	"sceTouchRead2",
	"sceTouchPeek",
	"sceTouchPeek2"
};
static char* str_btns[PHYS_BUTTONS_NUM] = {
	"Cross", "Circle", "Triangle", "Square",
	"Start", "Select", 
	"LT/L2", "RT/R2",
	"Up", "Right", "Left", "Down", 
	"L1", "R1", "L3", "R3"
};
static char* str_sections[] = {
	"Buttons", "LStick", "RStick", 
	"FrontTouch", "RearTouch", "Gyroscope",
	"","Disabled"
};
static char* str_analog_directions[] = {
	"Left", "Right", "Up", "Down"
};
static char* str_touch_zones[] = {
	"TopLeft", "TopRight", "BotLeft", "BotRight"
};
static char* str_gyro_directions[] = {
	"Left", "Right", "Up", "Down",
	"Roll Left","Roll Right"
};
static char* str_touch_points[] = {
	"Point A", "Point B", "Point C", "Point D"
};
static char* str_deadband[] = {
	"Game default", "Enable", "Disable"
};
char* getControllerName(int id){
	if 		(id == 	SCE_CTRL_TYPE_UNPAIRED) return "Unpaired controller";
	else if (id == 	SCE_CTRL_TYPE_PHY) 		return "Physical controller for VITA";
	else if (id == 	SCE_CTRL_TYPE_VIRT) 	return "Virtual controller for PSTV";
	else if (id == 	SCE_CTRL_TYPE_DS3) 		return "DualShock 3";
	else if (id == 	SCE_CTRL_TYPE_DS4) 		return "DualShock 4";
	else 									return "Unknown controller";
}

uint8_t ui_opened = 0;

uint8_t new_frame = 1;
static uint32_t ticker;
int cfg_i = 0;
int menu_i = MAIN_MENU;

//Calculate starting index for scroll menu
int calcStartingIndex(int idx, int entriesNum, int screenEntries){
	int bottom_l = 3;
	int ret = max(0, idx - (screenEntries - bottom_l));
	while (ret > 0 && (entriesNum - ret - 2) < screenEntries) ret--;
	return ret;
}

void drawHeader(){
	renderer_setColor(COLOR_HEADER);
	if (menu_i == MAIN_MENU)
		renderer_drawStringF(L_0, 2, "remaPSV2 v.%hhu.%hhu.%hhu", VERSION, SUBVERSION, SUBSUBVERSION);
	else	
		renderer_drawString(L_0, 2, str_menus[menu_i]);
	
	renderer_drawString(UI_WIDTH - CHA_W * strlen(titleid) - 10, 2, titleid);
	renderer_drawRectangle(5, CHA_H + 3, UI_WIDTH - 10, 1, COLOR_HEADER);
}

void drawFooter(){
	renderer_setColor(COLOR_HEADER);                                                               
	renderer_drawStringF(L_0, UI_HEIGHT-CHA_H - 2, str_footer[menu_i]);
	renderer_drawRectangle(5, UI_HEIGHT-CHA_H - 5, UI_WIDTH - 10, 1, COLOR_HEADER);
}

void drawMenu_main(int avaliableEntries, int y){
	for (int i = calcStartingIndex(cfg_i, MENU_MODES - 1, avaliableEntries); i < MENU_MODES - 1; i++) {
		if (cfg_i == i){//Draw cursor
			renderer_setColor(COLOR_CURSOR);
			renderer_drawString(L_0, y + CHA_H, (ticker % 16 < 8) ? "x" : "X");
		}
		
		renderer_setColor((i == cfg_i) ? COLOR_CURSOR : COLOR_DEFAULT);
		renderer_drawStringF(L_1, y += CHA_H, "%s", str_main_menu[i]);
		if (y + 40 > UI_HEIGHT) break;
	}
}

void drawMenu_remap(int avaliableEntries, int y){
	for (int i = calcStartingIndex(cfg_i, PROFILE_REMAP_NUM, avaliableEntries); i < PROFILE_REMAP_NUM; i++) {
		if (cfg_i == i){//Draw cursor
			renderer_setColor(COLOR_CURSOR);
			renderer_drawString(L_0 + CHA_W*10, y + CHA_H, (ticker % 16 < 8) ? "<" : ">");
		}
		
		char *srcSection = "", *srcAction = "", *targetSection = "", *targetAction = "";
		//Source section
		if (i == 0) srcSection = str_sections[0];
		else if (i == PHYS_BUTTONS_NUM)  srcSection = str_sections[3];
		else if (i == PHYS_BUTTONS_NUM + 4)  srcSection = str_sections[4];
		else if (i == PHYS_BUTTONS_NUM + 8) srcSection = str_sections[1];
		else if (i == PHYS_BUTTONS_NUM + 12) srcSection = str_sections[2];
		else if (i == PHYS_BUTTONS_NUM + 16) srcSection = str_sections[5];
		//Source  Action
		if (i < PHYS_BUTTONS_NUM) srcAction = str_btns[i];
		else if (i < PHYS_BUTTONS_NUM + 4)  srcAction = str_touch_zones[i - PHYS_BUTTONS_NUM];
		else if (i < PHYS_BUTTONS_NUM + 8)  srcAction = str_touch_zones[i - PHYS_BUTTONS_NUM-4];
		else if (i < PHYS_BUTTONS_NUM + 12) srcAction = str_analog_directions[i - PHYS_BUTTONS_NUM-8];
		else if (i < PHYS_BUTTONS_NUM + 16) srcAction = str_analog_directions[i - PHYS_BUTTONS_NUM-12];
		else if (i < PHYS_BUTTONS_NUM + 22) srcAction = str_gyro_directions[i - PHYS_BUTTONS_NUM-16];
		//Target Section
		if (profile_remap[i] < PHYS_BUTTONS_NUM) targetSection = str_sections[0];
		else if (profile_remap[i] == PHYS_BUTTONS_NUM)  targetSection = str_sections[6];
		else if (profile_remap[i] == PHYS_BUTTONS_NUM + 1)  targetSection = str_sections[7];
		else if (profile_remap[i] < PHYS_BUTTONS_NUM + 6)  targetSection = str_sections[1];
		else if (profile_remap[i] < PHYS_BUTTONS_NUM + 10)  targetSection = str_sections[2];
		else if (profile_remap[i] < PHYS_BUTTONS_NUM + 18)  targetSection = str_sections[3];
		else if (profile_remap[i] < PHYS_BUTTONS_NUM + 26)  targetSection = str_sections[4];
		//Target  Action
		if (profile_remap[i] < PHYS_BUTTONS_NUM) targetAction = str_btns[profile_remap[i]];
		else if (profile_remap[i] < PHYS_BUTTONS_NUM + 2)  targetAction = "";
		else if (profile_remap[i] < PHYS_BUTTONS_NUM + 6) targetAction = str_analog_directions[profile_remap[i] - PHYS_BUTTONS_NUM-2];
		else if (profile_remap[i] < PHYS_BUTTONS_NUM + 10) targetAction = str_analog_directions[profile_remap[i] - PHYS_BUTTONS_NUM-6];
		else if (profile_remap[i] < PHYS_BUTTONS_NUM + 14) targetAction = str_touch_zones[profile_remap[i] - PHYS_BUTTONS_NUM-10];
		else if (profile_remap[i] < PHYS_BUTTONS_NUM + 18) targetAction = str_touch_points[profile_remap[i] - PHYS_BUTTONS_NUM-14];
		else if (profile_remap[i] < PHYS_BUTTONS_NUM + 22) targetAction = str_touch_zones[profile_remap[i] - PHYS_BUTTONS_NUM-18];
		else if (profile_remap[i] < PHYS_BUTTONS_NUM + 26) targetAction = str_touch_points[profile_remap[i] - PHYS_BUTTONS_NUM-22];

		renderer_setColor(COLOR_HEADER);
		renderer_drawString(L_0, y += CHA_H, srcSection);
		
		if (i == cfg_i) renderer_setColor(COLOR_CURSOR);
		else if (profile_remap[i] == PHYS_BUTTONS_NUM) renderer_setColor(COLOR_DEFAULT);
		else if (profile_remap[i] == PHYS_BUTTONS_NUM + 1) renderer_setColor(COLOR_DANGER);
		else renderer_setColor(COLOR_ACTIVE);
		renderer_drawString(L_0 + CHA_W*11, y, srcAction);
		
		if (profile_remap[i] == PHYS_BUTTONS_NUM) renderer_setColor(COLOR_DEFAULT);
		else if (profile_remap[i] == PHYS_BUTTONS_NUM + 1) renderer_setColor(COLOR_DANGER);
		else renderer_setColor(COLOR_ACTIVE);
		if (profile_remap[i] != PHYS_BUTTONS_NUM)
			renderer_drawString(L_0 + CHA_W*21, y, " -> ");
		
		renderer_drawString(L_0 + CHA_W*25, y, targetSection);
		renderer_drawString(L_0 + CHA_W*36, y, targetAction);
		if (y + 60 > UI_HEIGHT) break;
	}
}

void drawMenu_analog(int avaliableEntries, int y){
	for (int i = calcStartingIndex(cfg_i, PROFILE_ANALOG_NUM, avaliableEntries); i < PROFILE_ANALOG_NUM; i++) {				
		if (y + 60 > UI_HEIGHT) break;
		
		if (!(i % 4)){	//Headers
			renderer_setColor(COLOR_HEADER);
			renderer_drawString(L_0, y+CHA_H, (i == 0) ? "Deadzone" : "Force digital");
		}
		
		if (i == cfg_i) renderer_setColor(COLOR_CURSOR);
		else if (profile_analog[i] != PROFILE_ANALOG_DEF[i]) renderer_setColor(COLOR_ACTIVE);
		else renderer_setColor(COLOR_DEFAULT);
		renderer_drawStringF(L_0+14*CHA_W, y+=CHA_H, "%s", 
			!(i % 2) ? (((i / 2) % 2 ) ? "Right Analog" : "Left Analog "): "");
		if (i < 4)
			renderer_drawStringF(L_0+27*CHA_W, y, "[%s axis]: %hhu", 
				(i % 2) ? "Y" : "X",
				profile_analog[i]);
		else 
			renderer_drawStringF(L_0+27*CHA_W, y, "[%s axis]: %s", 
				(i % 2) ? "Y" : "X",
				str_yes_no[profile_analog[i]]);	
				
		if (cfg_i == i){//Draw cursor
			renderer_setColor(COLOR_CURSOR);
			renderer_drawString(L_0 + 36*CHA_W, y, (ticker % 16 < 8) ? "<" : ">");
		}
	}
}

void drawMenu_touch(int avaliableEntries, int y){
	for (int i = calcStartingIndex(cfg_i, PROFILE_TOUCH_NUM, avaliableEntries); i < PROFILE_TOUCH_NUM; i++) {				
		if (y + 60 > UI_HEIGHT) break;
		
		if (cfg_i == i){//Draw cursor
			renderer_setColor(COLOR_CURSOR);
			renderer_drawString(L_0+ ((i<16) ? 16*CHA_W : 32*CHA_W), y + CHA_H, (ticker % 16 < 8) ? "<" : ">");
		}
		
		if (i == 0 || i == 8){	//Headers
			renderer_setColor(COLOR_HEADER);
			renderer_drawString(L_0, y+CHA_H, (i == 0) ? "Front" : "Rear");
		}
		
		if (i < 16){ //Points
			if (i == cfg_i) renderer_setColor(COLOR_CURSOR);
			else if (profile_touch[i] != PROFILE_TOUCH_DEF[i]) 
				renderer_setColor(COLOR_ACTIVE);
			else renderer_setColor(COLOR_DEFAULT);
			if (i < 16){
				if (!(i % 2)) 
					renderer_drawString(L_0+6*CHA_W, y+CHA_H, str_touch_points[(i % 8)/2]);
				renderer_drawStringF(L_0+14*CHA_W, y+=CHA_H, "%s:", !(i % 2) ? "x" : "y");
				renderer_drawStringF(L_0+17*CHA_W, y, "%hu", profile_touch[i]);
			}
			if (y + 60 > UI_HEIGHT) break;
		}
		
		if (i == 16){ //Front touch mode
			if (16 == cfg_i) renderer_setColor(COLOR_CURSOR);
			else if (profile_touch[16] == PROFILE_TOUCH_DEF[i]) renderer_setColor(COLOR_DEFAULT);
			else renderer_setColor(COLOR_ACTIVE);
			renderer_drawString(L_0, y+=CHA_H, "Disable Front touch if remapped:");
			renderer_drawString(L_0+33*CHA_W, y, str_yes_no[profile_touch[16]]);
			if (y + 60 > UI_HEIGHT) break;
		}
		
		if (i==17){ //Rear touch mode
			if (17 == cfg_i) renderer_setColor(COLOR_CURSOR);
			else if (profile_touch[17] == PROFILE_TOUCH_DEF[i]) renderer_setColor(COLOR_DEFAULT);
			else renderer_setColor(COLOR_ACTIVE);
			renderer_drawString(L_0, y+=CHA_H, "Disable Rear touch  if remapped:");
			renderer_drawString(L_0+33*CHA_W, y, str_yes_no[profile_touch[17]]);
			if (y + 60 > UI_HEIGHT) break;
		}
	}
}

void drawMenu_gyro(int avaliableEntries, int y){
	for (int i = calcStartingIndex(cfg_i, PROFILE_GYRO_NUM, avaliableEntries); i < PROFILE_GYRO_NUM; i++) {
		if (y + 60 > UI_HEIGHT) break;

		if (i < 6 && !(i % 3)) {	//Draw Headers
			renderer_setColor(COLOR_HEADER);
			renderer_drawString(L_1, y + CHA_H, (i == 0) ? "Sensivity" : (i == 3) ? "Deadzone" : "Mode");
		}

		if (i == cfg_i) renderer_setColor(COLOR_CURSOR);
		else if (profile_gyro[i] != PROFILE_GYRO_DEF[i]) renderer_setColor(COLOR_ACTIVE);
		else renderer_setColor(COLOR_DEFAULT);
		if (i < 6){ 		//Draw sens and deadzone option
			renderer_drawStringF(L_1 + 17 * CHA_W, y += CHA_H, "%s axis: %hhu",
				((i % 3) == 2) ? "Z" : ((i % 3) ? "Y" : "X"),
				profile_gyro[i]);
		
		} else if (i == 6) { //Draw deadband option
			renderer_drawStringF(L_1, y += CHA_H, "Deadband mode          : %s", 
				str_deadband[profile_gyro[i]]);
			
		} else if (i == 7) { //Draw wheel mode option
			renderer_drawStringF(L_1, y += CHA_H, "Wheel mode [WIP]       : %s", 
				str_yes_no[profile_gyro[i]]);
		
		} else if (i < 10) { //Draw reset button options
			renderer_drawStringF(L_1, y += CHA_H, "Wheel reset %s key : %s", 
				(i == 8) ? "first " : "second", 
				str_btns[profile_gyro[i]]);
		
		} else if (i == 10) { //Draw manual reset option
			renderer_drawString(L_1, y += CHA_H, "Manual wheel reset");
		}
		
		if (cfg_i == i) {//Draw cursor
			renderer_setColor(COLOR_CURSOR);
			if (cfg_i == 10)
				renderer_drawString(L_0, y, (ticker % 16 < 8) ? "x" : "X");
			else
				renderer_drawString(L_1 + 24 * CHA_W, y, (ticker % 16 < 8) ? "<" : ">");
		}
	}
}

void drawMenu_controller(int avaliableEntries, int y){
	SceCtrlPortInfo pi;
	int res = ksceCtrlGetControllerPortInfo(&pi);
	if (res != 0){//Should not ever trigger
		renderer_setColor(COLOR_DANGER);
		renderer_drawString(L_1, y+= CHA_H, "Error getting controllers info");
	} else {
		//Cursor
		renderer_setColor(COLOR_CURSOR);
		renderer_drawString(L_0, y + CHA_H + CHA_H * cfg_i, (ticker % 16 < 8) ? "<" : ">");
		
		//Use external controller
		renderer_setColor(cfg_i == 0 ? COLOR_CURSOR : 
			(profile_controller[0] == PROFILE_CONTROLLER_DEF[0] ? COLOR_DEFAULT : COLOR_ACTIVE));
		renderer_drawStringF(L_1, y += CHA_H, "Use external controller: %s", str_yes_no[profile_controller[0]]);
		
		//Port selection
		renderer_setColor(cfg_i == 1 ? COLOR_CURSOR : 
			(profile_controller[1] == PROFILE_CONTROLLER_DEF[1] ? COLOR_DEFAULT : COLOR_ACTIVE));
		renderer_drawStringF(L_1, y += CHA_H, "Selected port: {%i} %s %s", 
			profile_controller[1],
			getControllerName(pi.port[profile_controller[1]]), 
			profile_controller[1] ? "" : "[DEFAULT]");
		
		//Button swap
		renderer_setColor(cfg_i == 2 ? COLOR_CURSOR : 
			(profile_controller[2] == PROFILE_CONTROLLER_DEF[2] ? COLOR_DEFAULT : COLOR_ACTIVE));
		renderer_drawStringF(L_1, y += CHA_H, "Swap L1<>LT R1<>RT     : %s", str_yes_no[profile_controller[2]]);
		
		//Ports stats
		y+=CHA_H;
		renderer_setColor(COLOR_DEFAULT);
		renderer_drawString(L_1, y+= CHA_H, "Detected controllers:");
		for (int i = max(0, cfg_i - (avaliableEntries + 1)); i < 5; i++){
			renderer_setColor((L_1 == cfg_i) ? COLOR_CURSOR : ((pi.port[i] != SCE_CTRL_TYPE_UNPAIRED) ? COLOR_ACTIVE : COLOR_DEFAULT));
			renderer_drawStringF(L_1, y += CHA_H, "Port %i: %s", i, getControllerName(pi.port[i]));
			if (y + 40 > UI_HEIGHT) break;
		}	
	}
}

void drawMenu_hooks(int avaliableEntries, int y){
	for (int i = calcStartingIndex(cfg_i, HOOKS_NUM - 1, avaliableEntries); i < HOOKS_NUM - 1; i++) {
		if (cfg_i == i){//Draw cursor
			renderer_setColor(COLOR_CURSOR);
			renderer_drawString(L_0, y + CHA_H, "-");
		}
		renderer_setColor((i == cfg_i) ? COLOR_CURSOR : (used_funcs[i] ? COLOR_ACTIVE : COLOR_DEFAULT));
		renderer_drawStringF(L_1, y += CHA_H, "%s : %s", str_funcs[i], used_funcs[i] ? "Yes" : "No");
		if (y + 40 > UI_HEIGHT) break;
	}
}

void drawMenu_settings(int avaliableEntries, int y){
	//Cursor
	renderer_setColor(COLOR_CURSOR);
	if (cfg_i <= 2)
		renderer_drawString(L_0, y + CHA_H + CHA_H * cfg_i, (ticker % 16 < 8) ? "<" : ">");
	else
		renderer_drawString(L_0, y + CHA_H + CHA_H * cfg_i, (ticker % 16 < 8) ? "x" : "X");
	//Menu trigger keys
	renderer_setColor(cfg_i == 0 ? COLOR_CURSOR : 
		(profile_settings[0] == PROFILE_SETTINGS_DEF[0] ? COLOR_DEFAULT : COLOR_ACTIVE));
	renderer_drawStringF(L_1, y += CHA_H, "Menu trigger first key    : %s", 
		str_btns[profile_settings[0]]);
	renderer_setColor(cfg_i == 1 ? COLOR_CURSOR : 
		(profile_settings[1] == PROFILE_SETTINGS_DEF[1] ? COLOR_DEFAULT : COLOR_ACTIVE));
	renderer_drawStringF(L_1, y += CHA_H, "            second key    : %s", 
		str_btns[profile_settings[1]]);
	
	//Save game profile on close
	renderer_setColor(cfg_i == 2 ? COLOR_CURSOR : 
		(profile_settings[2] == PROFILE_SETTINGS_DEF[2] ? COLOR_DEFAULT : COLOR_ACTIVE));
	renderer_drawStringF(L_1, y += CHA_H, "Save Game profile on close: %s", str_yes_no[profile_settings[2]]);
	
	//Startup delay
	renderer_setColor(cfg_i == 3 ? COLOR_CURSOR : 
		(profile_settings[3] == PROFILE_SETTINGS_DEF[3] ? COLOR_DEFAULT : COLOR_ACTIVE));
	renderer_drawStringF(L_1, y += CHA_H, "Startup delay             : %hhu seconds", profile_settings[3]);
	
	//Profile management
	for (int i = 0; i <	sizeof(str_settings)/sizeof(char*); i++){
		renderer_setColor((cfg_i == (4 + i)) ? COLOR_CURSOR : COLOR_DEFAULT);
		renderer_drawString(L_1, y += CHA_H, str_settings[i]);
	}
}

void drawMenu_credits(int avaliableEntries, int y){
	for (int i = calcStartingIndex(cfg_i, CREDITS_NUM, avaliableEntries); i < CREDITS_NUM; i++) {	
		if (cfg_i == i){//Draw cursor
			renderer_setColor(COLOR_CURSOR);
			renderer_drawString(L_0, y + CHA_H, "-");
		}
		
		renderer_setColor(COLOR_DEFAULT);
		renderer_drawStringF(L_1, y += CHA_H, "%s", str_credits[i]);
		if (y + 40 > UI_HEIGHT) break;
	}
}

void drawTouchPointer(){
	if (menu_i != TOUCH_MENU || cfg_i >= 16)
		return;
	int8_t ic_hsize = ICN_SIZE_LARGE / 2;
	int left = profile_touch[cfg_i - (cfg_i % 2)] - 8;
	left *= (float)fbWidth / ((cfg_i < 8) ? TOUCH_SIZE[0] : TOUCH_SIZE[2]);
	left = min((max(ic_hsize, left)), fbWidth - ic_hsize);
	int top = profile_touch[cfg_i - (cfg_i % 2) + 1] - 10;
	top *= (float)fbHeight / ((cfg_i < 8) ? TOUCH_SIZE[1] : TOUCH_SIZE[3]); //Scale to framebuffer size
	top = min((max(ic_hsize, top)), fbHeight - ic_hsize);//limit into screen
	renderer_setColor((ticker % 8 < 4) ? COLOR_DANGER : COLOR_HEADER);
	renderer_drawImageDirectlyToFB(left - ic_hsize, top - ic_hsize, 64, 64, ICN_TOUCH);
}

void drawMenu() {
	drawHeader();
	//Draw menu
	int y = 6;
	int screen_entries = ((float)UI_HEIGHT - 10) / CHA_H - 1;
	int avaliableEntries = screen_entries - 4;
	switch (menu_i){
		case MAIN_MENU: drawMenu_main(avaliableEntries, y); break;
		case REMAP_MENU: drawMenu_remap(avaliableEntries, y); break;
		case ANALOG_MENU: drawMenu_analog(avaliableEntries, y); break;
		case TOUCH_MENU: drawMenu_touch(avaliableEntries, y); break;
		case GYRO_MENU: drawMenu_gyro(avaliableEntries, y); break;
		case CNTRL_MENU: drawMenu_controller(avaliableEntries, y); break;
		case FUNCS_LIST: drawMenu_hooks(avaliableEntries, y); break;  
		case SETTINGS_MENU: drawMenu_settings(avaliableEntries, y); break; 
		case CREDITS_MENU: drawMenu_credits(avaliableEntries, y); break;                                                             
		default: break;
	}
	drawFooter();
}

//Draw directly to FB to overlay over everything else;
void drawDirectlyToFB(){
	drawTouchPointer();
}

void ui_draw(const SceDisplayFrameBuf *pParam){
	LOG("ui_draw() %u\n", ui_opened);
	if (ui_opened) {
		new_frame = 1;
		ticker++;
		renderer_setFB(pParam);
		drawMenu();
		renderer_writeToFB();
		drawDirectlyToFB();	
	}
}

void ui_open(const SceDisplayFrameBuf *pParam){
	LOG("ui_open()\n");
	ui_opened = 1;
	cfg_i = 0;
}

void ui_init(){
    renderer_init(UI_WIDTH, UI_HEIGHT);
}

void ui_destroy(){
	renderer_destroy();
}