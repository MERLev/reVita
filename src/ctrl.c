#include <vitasdkkern.h>
#include <psp2/touch.h> 
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "vitasdkext.h"
#include "main.h"
#include "ui.h"
#include "profile.h"
#include "common.h"

static uint32_t curr_buttons;
static uint32_t old_buttons;
static uint64_t tick;
static uint64_t pressedTicks[PHYS_BUTTONS_NUM];

#define LONG_PRESS_TIME   350000	//0.35sec

uint8_t isBtnActive(uint8_t btnNum){
	return ((curr_buttons & HW_BUTTONS[btnNum]) && !(old_buttons & HW_BUTTONS[btnNum])) 
		|| (pressedTicks[btnNum] != 0 && tick - pressedTicks[btnNum] > LONG_PRESS_TIME);
}

//Set custom touch point xy using RS
void analogTouchPicker(SceCtrlData *ctrl){
	if (cfg_i >= 16)
		return;
	int o_idx1 = cfg_i - (cfg_i % 2);
	int shiftX = ((float)(ctrl->rx - 127)) / 8;
	int shiftY = ((float)(ctrl->ry - 127)) / 8;
	if (abs(shiftX) > 30 / 8)
		profile_touch[o_idx1] = clamp(profile_touch[o_idx1] + shiftX, 
			0, TOUCH_SIZE[(o_idx1 < 8) ? 0 : 2]);
	if (abs(shiftY) > 30 / 8)
		profile_touch[o_idx1+1] = clamp(profile_touch[o_idx1+1] + shiftY, 
			0, TOUCH_SIZE[((o_idx1+1) < 8) ? 1 : 3]);
}
//Set custom touch point xy using touch
void touchPicker(int padType){
	if ((padType == SCE_TOUCH_PORT_FRONT && cfg_i >= 8) ||
		(padType == SCE_TOUCH_PORT_BACK && (cfg_i < 8 || cfg_i >= 16)))
		return;
	SceTouchData std;
	internal_touch_call = 1;
	int ret = ksceTouchPeek(padType, &std, 1);
	internal_touch_call = 0;
	if (ret && std.reportNum){
		profile_touch[cfg_i - (cfg_i % 2)] = std.report[0].x;
		profile_touch[cfg_i - (cfg_i % 2) + 1] = std.report[0].y;
	}
}

void inputHandler_main(uint32_t btn){
	uint8_t menu_entries = MENU_MODES-1;
	switch (btn) {
	case SCE_CTRL_DOWN:
		cfg_i = (cfg_i + 1) % menu_entries;
		break;
	case SCE_CTRL_UP:
		if (--cfg_i < 0) cfg_i = menu_entries  -1;
		break;
	case SCE_CTRL_CROSS:
		menu_i = cfg_i + 1;
		cfg_i = 0;
		break;
	case SCE_CTRL_CIRCLE:
		if (menu_i == MAIN_MENU) {
			ui_opened = 0;
			profile_saveSettings();
			if (profile_settings[0])
				profile_saveLocal();
			delayedStart();
		} else {
			menu_i = MAIN_MENU;
			cfg_i = 0;
		}
		break;
	}
}

void inputHandler_remap(uint32_t btn){
	int menu_entries = PROFILE_REMAP_NUM;
	switch (btn) {
		case SCE_CTRL_DOWN:
			cfg_i = (cfg_i + 1) % menu_entries;
			break;
		case SCE_CTRL_UP:
			if (--cfg_i < 0) cfg_i = menu_entries  -1;
			break;
		case SCE_CTRL_RIGHT:
			profile_remap[cfg_i] = (profile_remap[cfg_i] + 1) % TARGET_REMAPS;
			break;
		case SCE_CTRL_LEFT:
			if (profile_remap[cfg_i]) 	
				profile_remap[cfg_i]--;
			else
				profile_remap[cfg_i] = TARGET_REMAPS - 1;
			break;
		case SCE_CTRL_LTRIGGER:
		case SCE_CTRL_L1: //Sections navigation
			if (profile_remap[cfg_i] < 16)
				profile_remap[cfg_i] = 38;	//Rear touch custom
			else if (profile_remap[cfg_i] < 17)
				profile_remap[cfg_i] = 0;	//HW Buttons
			else if (profile_remap[cfg_i] < 18)
				profile_remap[cfg_i] = 16;	//Original
			else if (profile_remap[cfg_i] < 22)
				profile_remap[cfg_i] = 17;	//Disabled
			else if (profile_remap[cfg_i] < 26)
				profile_remap[cfg_i] = 18;	//Left stick
			else if (profile_remap[cfg_i] < 30)
				profile_remap[cfg_i] = 22;	//Right stick
			else if (profile_remap[cfg_i] < 34)
				profile_remap[cfg_i] = 26;	//Front touch default
			else if (profile_remap[cfg_i] < 38)
				profile_remap[cfg_i] = 30;	//Front touch custom
			else 
				profile_remap[cfg_i] = 34;	//Rear touch default
			break;
		case SCE_CTRL_RTRIGGER:
		case SCE_CTRL_R1://Sections navigation
			if (profile_remap[cfg_i] < 16)
				profile_remap[cfg_i] = 16;	//Original
			else if (profile_remap[cfg_i] < 17)
				profile_remap[cfg_i] = 17;	//Disabled
			else if (profile_remap[cfg_i] < 18)
				profile_remap[cfg_i] = 18;	//Left stick
			else if (profile_remap[cfg_i] < 22)
				profile_remap[cfg_i] = 22;	//Right stick
			else if (profile_remap[cfg_i] < 26)
				profile_remap[cfg_i] = 26;	//Front touch default
			else if (profile_remap[cfg_i] < 30)
				profile_remap[cfg_i] = 30;	//Front touch custom
			else if (profile_remap[cfg_i] < 34)
				profile_remap[cfg_i] = 34;	//Rear touch default
			else if (profile_remap[cfg_i] < 38)
				profile_remap[cfg_i] = 38;	//Rear touch custom
			else 
				profile_remap[cfg_i] = 0;	//HW Buttons
			break;
		case SCE_CTRL_SQUARE:
			profile_remap[cfg_i] = PHYS_BUTTONS_NUM;
			break;
		case SCE_CTRL_START:
			profile_resetRemap();
			break;
		case SCE_CTRL_CIRCLE:
			menu_i = MAIN_MENU;
			cfg_i = 0;
			break;
	}
}

void inputHandler_analog(uint32_t btn){
	uint8_t menu_entries = PROFILE_ANALOG_NUM;
	switch (btn) {
		case SCE_CTRL_DOWN:
			cfg_i = (cfg_i + 1) % menu_entries;
			break;
		case SCE_CTRL_UP:
			if (--cfg_i < 0) cfg_i = menu_entries  -1;
			break;
		case SCE_CTRL_RIGHT:
			if (cfg_i < 4) profile_analog[cfg_i] = (profile_analog[cfg_i] + 1) % 128;
			else profile_analog[cfg_i] = !profile_analog[cfg_i];
			break;
		case SCE_CTRL_LEFT:
			if (profile_analog[cfg_i]) 	
				profile_analog[cfg_i]--;
			else
				profile_analog[cfg_i] = cfg_i < 4 ? 127 : 1;
			break;
		case SCE_CTRL_SQUARE:
			profile_analog[cfg_i] = PROFILE_ANALOG_DEF[cfg_i];
			break;
		case SCE_CTRL_CIRCLE:
			menu_i = MAIN_MENU;
			cfg_i = 0;
			break;
	}
}

void inputHandler_touch(uint32_t btn){
	uint8_t menu_entries = PROFILE_TOUCH_NUM;
	switch (btn) {
		case SCE_CTRL_DOWN:
			cfg_i = (cfg_i + 1) % menu_entries;
			break;
		case SCE_CTRL_UP:
			if (--cfg_i < 0) cfg_i = menu_entries  -1;
			break;
		case SCE_CTRL_RIGHT:
			if (cfg_i < 8)//Front Points xy
				profile_touch[cfg_i] = (profile_touch[cfg_i] + 1) 
					% ((cfg_i % 2) ? TOUCH_SIZE[1] : TOUCH_SIZE[0]);
			else if (cfg_i < 16)//Rear Points xy
				profile_touch[cfg_i] = (profile_touch[cfg_i] + 1)
					% ((cfg_i % 2) ? TOUCH_SIZE[3] : TOUCH_SIZE[2]);
			else 			//yes/no otion
				profile_touch[cfg_i] = !profile_touch[cfg_i];
			break;
		case SCE_CTRL_LEFT:
			if (profile_touch[cfg_i]) 	
				profile_touch[cfg_i]--;
			else {
				if (cfg_i < 8)//front points xy
					profile_touch[cfg_i] = ((cfg_i % 2) ? TOUCH_SIZE[1] - 1 : TOUCH_SIZE[0] - 1);
				if (cfg_i < 16)//rear points xy
					profile_touch[cfg_i] = ((cfg_i % 2) ? TOUCH_SIZE[3] - 1 : TOUCH_SIZE[2] - 1);
				else //yes/no options
					profile_touch[cfg_i] = !profile_touch[cfg_i];
			}
			break;
		case SCE_CTRL_SQUARE:
			profile_touch[cfg_i] = PROFILE_TOUCH_DEF[cfg_i];
			break;
		case SCE_CTRL_START:
			profile_resetTouch();
			break;
		case SCE_CTRL_CIRCLE:
			menu_i = MAIN_MENU;
			cfg_i = 0;
			break;
	}
}

void inputHandler_gyro(uint32_t btn){
	uint8_t menu_entries = PROFILE_GYRO_NUM;
	switch (btn) {
		case SCE_CTRL_DOWN:
			cfg_i = (cfg_i + 1) % menu_entries;
			break;
		case SCE_CTRL_UP:
			if (--cfg_i < 0) cfg_i = menu_entries  -1;
			break;
		case SCE_CTRL_RIGHT:
			if (cfg_i < 6) //Sens & deadzone
				profile_gyro[cfg_i] = (profile_gyro[cfg_i] + 1) % 200;
			else if (cfg_i == 6) // Deadband
				profile_gyro[cfg_i] = min(2, profile_gyro[cfg_i] + 1);
			else if (cfg_i == 7) // Wheel mode
				profile_gyro[cfg_i] = (profile_gyro[cfg_i] + 1) % 2;
			else if (cfg_i < 10) // Reset wheel buttons
				profile_gyro[cfg_i] 
					= min(PHYS_BUTTONS_NUM - 1, profile_gyro[cfg_i] + 1);
			break;
		case SCE_CTRL_LEFT:
			if (profile_gyro[cfg_i]) 	
				profile_gyro[cfg_i]--;
			else {
				if (cfg_i < 6) //Sens & deadzone
					profile_gyro[cfg_i] = 199;
				else if (cfg_i == 6) // deadband
					profile_gyro[cfg_i] = max(0, profile_gyro[cfg_i] - 1);
				else if (cfg_i == 7) //Wheel mode
					profile_gyro[cfg_i] = 1;
				else if (cfg_i < 10)  // Reset wheel btns
					profile_gyro[cfg_i] = max(0, profile_gyro[cfg_i] - 1);
			}
			break;
		case SCE_CTRL_SQUARE:
			profile_gyro[cfg_i] = PROFILE_GYRO_DEF[cfg_i];
			break;
		case SCE_CTRL_START:
			profile_resetGyro();
			break;
		case SCE_CTRL_CROSS:
			//if (cfg_i == 10){
				//sceMotionReset();
			break;
		case SCE_CTRL_CIRCLE:
			menu_i = MAIN_MENU;
			cfg_i = 0;
			break;
	}
}

void inputHandler_controller(uint32_t btn){
	uint8_t menu_entries = PROFILE_CONTROLLER_NUM;
	switch (btn) {
		case SCE_CTRL_DOWN:
			cfg_i = (cfg_i + 1) % menu_entries;
			break;
		case SCE_CTRL_UP:
			if (--cfg_i < 0) cfg_i = menu_entries  -1;
			break;
		case SCE_CTRL_RIGHT:
			if (cfg_i == 1)
				profile_controller[cfg_i] = min(5, profile_controller[cfg_i] + 1);
			else
				profile_controller[cfg_i] = !profile_controller[cfg_i];
			break;
		case SCE_CTRL_LEFT:
			if (cfg_i == 1)
				profile_controller[cfg_i] = max(0, profile_controller[cfg_i] - 1);
			else
				profile_controller[cfg_i] = !profile_controller[cfg_i];
			break;
		case SCE_CTRL_SQUARE:
			profile_controller[cfg_i] = PROFILE_CONTROLLER_DEF[cfg_i];
			break;
		case SCE_CTRL_START:
			profile_resetController();
			break;
		case SCE_CTRL_CIRCLE:
			menu_i = MAIN_MENU;
			cfg_i = 0;
			break;
	}
}

void inputHandler_hooks(uint32_t btn){
	uint8_t menu_entries = HOOKS_NUM - 1;
	switch (btn) {
		case SCE_CTRL_DOWN:
			cfg_i = (cfg_i + 1) % menu_entries;
			break;
		case SCE_CTRL_UP:
			if (--cfg_i < 0) cfg_i = menu_entries  -1;
			break;
		case SCE_CTRL_CIRCLE:
			menu_i = MAIN_MENU;
			cfg_i = 0;
			break;
	}
}

void inputHandler_credits(uint32_t btn){
	uint8_t menu_entries = CREDITS_NUM;
	switch (btn) {
		case SCE_CTRL_DOWN:
			cfg_i = (cfg_i + 1) % menu_entries;
			break;
		case SCE_CTRL_UP:
			if (--cfg_i < 0) cfg_i = menu_entries  -1;
			break;
		case SCE_CTRL_CIRCLE:
			menu_i = MAIN_MENU;
			cfg_i = 0;
			break;
	}
}

void inputHandler_settings(uint32_t btn){
	uint8_t menu_entries = PROFILE_SETTINGS_NUM + 4;
	switch (btn) {
		case SCE_CTRL_DOWN:
			cfg_i = (cfg_i + 1) % menu_entries;
			break;
		case SCE_CTRL_UP:
			if (--cfg_i < 0) cfg_i = menu_entries  -1;
			break;
		case SCE_CTRL_RIGHT:
			if (cfg_i < 2)
				profile_settings[cfg_i] 
					= min(PHYS_BUTTONS_NUM - 1, profile_settings[cfg_i] + 1);
			else if (cfg_i == 2)
				profile_settings[cfg_i] = !profile_settings[cfg_i];
			else if (cfg_i == 3)
				profile_settings[cfg_i] 
					= min(60, profile_settings[cfg_i] + 1);
			break;
		case SCE_CTRL_LEFT:
			if (cfg_i < 2)
				profile_settings[cfg_i] 
					= max(0, profile_settings[cfg_i] - 1);
			else if (cfg_i == 2)
				profile_settings[cfg_i] = !profile_settings[cfg_i];
			else if (cfg_i == 3)
				profile_settings[cfg_i] 
					= max(0, profile_settings[cfg_i] - 1);
			break;
		case SCE_CTRL_SQUARE:
			if (cfg_i <= 2)
				profile_settings[cfg_i] = PROFILE_SETTINGS_DEF[cfg_i];
			break;
		case SCE_CTRL_START:
			profile_resetSettings();
			break;
		case SCE_CTRL_CROSS:
			if      (cfg_i == PROFILE_SETTINGS_NUM)     profile_saveLocal();	
			else if (cfg_i == PROFILE_SETTINGS_NUM + 1) profile_loadLocal();			
			else if (cfg_i == PROFILE_SETTINGS_NUM + 2) profile_saveGlobal();			
			else if (cfg_i == PROFILE_SETTINGS_NUM + 3) profile_loadGlobal();
			break;
		case SCE_CTRL_CIRCLE:
			menu_i = MAIN_MENU;
			cfg_i = 0;
			break;
	}
}

void ctrl_inputHandler(SceCtrlData *ctrl) {
	if ((ctrl->buttons & HW_BUTTONS[profile_settings[0]]) 
			&& (ctrl->buttons & HW_BUTTONS[profile_settings[1]]))
		return; //Menu trigger butoons should not trigger any menu actions on menu open
	if (new_frame) {
		new_frame = 0;
		
		if (menu_i == TOUCH_MENU){					
			touchPicker(SCE_TOUCH_PORT_FRONT);
			touchPicker(SCE_TOUCH_PORT_BACK);
			analogTouchPicker(ctrl);
		}

		tick = ctrl->timeStamp;
		curr_buttons = ctrl->buttons;
		for (int i = 0; i < PHYS_BUTTONS_NUM; i++){
			if ((curr_buttons & HW_BUTTONS[i]) && !(old_buttons & HW_BUTTONS[i]))
				pressedTicks[i] = tick;
			else if (!(curr_buttons & HW_BUTTONS[i]) && (old_buttons & HW_BUTTONS[i]))
				pressedTicks[i] = 0;
			
			if (!isBtnActive(i))
				continue;

			switch (menu_i) {
				case MAIN_MENU: inputHandler_main(HW_BUTTONS[i]); break;
				case REMAP_MENU: inputHandler_remap(HW_BUTTONS[i]); break;
				case ANALOG_MENU: inputHandler_analog(HW_BUTTONS[i]); break;
				case TOUCH_MENU: inputHandler_touch(HW_BUTTONS[i]); break;
				case GYRO_MENU: inputHandler_gyro(HW_BUTTONS[i]); break;
				case CNTRL_MENU: inputHandler_controller(HW_BUTTONS[i]); break;
				case FUNCS_LIST: inputHandler_hooks(HW_BUTTONS[i]); break;
				case CREDITS_MENU: inputHandler_credits(HW_BUTTONS[i]); break;
				case SETTINGS_MENU: inputHandler_settings(HW_BUTTONS[i]); break;
				default: break;
			}
		}
		old_buttons = curr_buttons;
	}
}

void ctrl_init(){
    
}

void ctrl_destroy(){
	
}