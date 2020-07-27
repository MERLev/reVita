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
	if (ui_entry->id >= 16)
		return;
	int o_idx1 = ui_entry->id - (ui_entry->id % 2);
	int shiftX = ((float)(ctrl->rx - 127)) / 8;
	int shiftY = ((float)(ctrl->ry - 127)) / 8;
	if (abs(shiftX) > 30 / 8)
		profile.touch[o_idx1] = clamp(profile.touch[o_idx1] + shiftX, 
			0, TOUCH_SIZE[(o_idx1 < 8) ? 0 : 2]);
	if (abs(shiftY) > 30 / 8)
		profile.touch[o_idx1+1] = clamp(profile.touch[o_idx1+1] + shiftY, 
			0, TOUCH_SIZE[((o_idx1+1) < 8) ? 1 : 3]);
}
//Set custom touch point xy using touch
void touchPicker(int padType){
	if ((padType == SCE_TOUCH_PORT_FRONT && ui_entry->id >= 8) ||
		(padType == SCE_TOUCH_PORT_BACK && (ui_entry->id < 8 || ui_entry->id >= 16)))
		return;
	SceTouchData std;
	internal_touch_call = 1;
	int ret = ksceTouchPeek(padType, &std, 1);
	internal_touch_call = 0;
	if (ret && std.reportNum){
		profile.touch[ui_entry->id - (ui_entry->id % 2)] = std.report[0].x;
		profile.touch[ui_entry->id - (ui_entry->id % 2) + 1] = std.report[0].y;
	}
}

void inputHandler_main(uint32_t btn){
	switch (btn) {
		case SCE_CTRL_DOWN:
			ui_nextEntry();
			break;
		case SCE_CTRL_UP:
			ui_prevEntry();
			break;
		case SCE_CTRL_CROSS:
			ui_setMenu(ui_menu->entries[ui_menu->idx].id);
			ui_setIdx(0);
			break;
		case SCE_CTRL_CIRCLE:
			ui_opened = 0;
			profile_saveSettings();
			if (profile_settings[0])
				profile_saveLocal();
			delayedStart();
			break;
	}
}

void inputHandler_remap(uint32_t btn){
	switch (btn) {
		case SCE_CTRL_DOWN:
			ui_nextEntry();
			break;
		case SCE_CTRL_UP:
			ui_prevEntry();
			break;
		case SCE_CTRL_RIGHT:
			profile.remap[ui_menu->idx] = (profile.remap[ui_menu->idx] + 1) % TARGET_REMAPS;
			break;
		case SCE_CTRL_LEFT:
			if (profile.remap[ui_menu->idx]) 	
				profile.remap[ui_menu->idx]--;
			else
				profile.remap[ui_menu->idx] = TARGET_REMAPS - 1;
			break;
		case SCE_CTRL_LTRIGGER:
		case SCE_CTRL_L1: //Sections navigation
			if (profile.remap[ui_menu->idx] < 16)
				profile.remap[ui_menu->idx] = 38;	//Rear touch custom
			else if (profile.remap[ui_menu->idx] < 17)
				profile.remap[ui_menu->idx] = 0;	//HW Buttons
			else if (profile.remap[ui_menu->idx] < 18)
				profile.remap[ui_menu->idx] = 16;	//Original
			else if (profile.remap[ui_menu->idx] < 22)
				profile.remap[ui_menu->idx] = 17;	//Disabled
			else if (profile.remap[ui_menu->idx] < 26)
				profile.remap[ui_menu->idx] = 18;	//Left stick
			else if (profile.remap[ui_menu->idx] < 30)
				profile.remap[ui_menu->idx] = 22;	//Right stick
			else if (profile.remap[ui_menu->idx] < 34)
				profile.remap[ui_menu->idx] = 26;	//Front touch default
			else if (profile.remap[ui_menu->idx] < 38)
				profile.remap[ui_menu->idx] = 30;	//Front touch custom
			else 
				profile.remap[ui_menu->idx] = 34;	//Rear touch default
			break;
		case SCE_CTRL_RTRIGGER:
		case SCE_CTRL_R1://Sections navigation
			if (profile.remap[ui_menu->idx] < 16)
				profile.remap[ui_menu->idx] = 16;	//Original
			else if (profile.remap[ui_menu->idx] < 17)
				profile.remap[ui_menu->idx] = 17;	//Disabled
			else if (profile.remap[ui_menu->idx] < 18)
				profile.remap[ui_menu->idx] = 18;	//Left stick
			else if (profile.remap[ui_menu->idx] < 22)
				profile.remap[ui_menu->idx] = 22;	//Right stick
			else if (profile.remap[ui_menu->idx] < 26)
				profile.remap[ui_menu->idx] = 26;	//Front touch default
			else if (profile.remap[ui_menu->idx] < 30)
				profile.remap[ui_menu->idx] = 30;	//Front touch custom
			else if (profile.remap[ui_menu->idx] < 34)
				profile.remap[ui_menu->idx] = 34;	//Rear touch default
			else if (profile.remap[ui_menu->idx] < 38)
				profile.remap[ui_menu->idx] = 38;	//Rear touch custom
			else 
				profile.remap[ui_menu->idx] = 0;	//HW Buttons
			break;
		case SCE_CTRL_SQUARE:
			profile.remap[ui_menu->idx] = PHYS_BUTTONS_NUM;
			break;
		case SCE_CTRL_START:
			profile_resetRemap();
			break;
		case SCE_CTRL_CIRCLE:
			ui_setMenu(MAIN_MENU);
			ui_setIdx(0);
			break;
	}
}

void inputHandler_analog(uint32_t btn){
	int8_t id = ui_entry->id;
	switch (btn) {
		case SCE_CTRL_DOWN:
			ui_nextEntry();
			break;
		case SCE_CTRL_UP:
			ui_prevEntry();
			break;
		case SCE_CTRL_RIGHT:
			if (id == HEADER_IDX) break;
			if (id < 4) profile.analog[id] = (profile.analog[id] + 1) % 128;
			else profile.analog[id] = !profile.analog[id];
			break;
		case SCE_CTRL_LEFT:
			if (id == HEADER_IDX) break;
			if (profile.analog[id]) 	
				profile.analog[id]--;
			else
				profile.analog[id] = id < 4 ? 127 : 1;
			break;
		case SCE_CTRL_SQUARE:
			profile.analog[id] = profile_def.analog[id];
			break;
		case SCE_CTRL_CIRCLE:
			ui_setMenu(MAIN_MENU);
			ui_setIdx(0);
			break;
	}
}

void inputHandler_touch(uint32_t btn){
	int8_t idx = ui_entry->id;
	switch (btn) {
		case SCE_CTRL_DOWN:
			ui_nextEntry();
			break;
		case SCE_CTRL_UP:
			ui_prevEntry();
			break;
		case SCE_CTRL_RIGHT:
			if (idx == HEADER_IDX) break;
			else if (idx < 8)//Front Points xy
				profile.touch[idx] = (profile.touch[idx] + 1) 
					% ((idx % 2) ? TOUCH_SIZE[1] : TOUCH_SIZE[0]);
			else if (idx < 16)//Rear Points xy
				profile.touch[idx] = (profile.touch[idx] + 1)
					% ((idx % 2) ? TOUCH_SIZE[3] : TOUCH_SIZE[2]);
			else 			//yes/no otion
				profile.touch[idx] = !profile.touch[idx];
			break;
		case SCE_CTRL_LEFT:
			if (idx == HEADER_IDX) break;
			else if (profile.touch[idx]) 	
				profile.touch[idx]--;
			else {
				if (idx < 8)//front points xy
					profile.touch[idx] = ((idx % 2) ? TOUCH_SIZE[1] - 1 : TOUCH_SIZE[0] - 1);
				if (idx < 16)//rear points xy
					profile.touch[idx] = ((idx % 2) ? TOUCH_SIZE[3] - 1 : TOUCH_SIZE[2] - 1);
				else //yes/no options
					profile.touch[idx] = !profile.touch[idx];
			}
			break;
		case SCE_CTRL_SQUARE:
			profile.touch[idx] = profile_def.touch[idx];
			break;
		case SCE_CTRL_START:
			profile_resetTouch();
			break;
		case SCE_CTRL_CIRCLE:
			ui_setMenu(MAIN_MENU);
			ui_setIdx(0);
			break;
	}
}

void inputHandler_gyro(uint32_t btn){
	int8_t id = ui_entry->id;
	switch (btn) {
		case SCE_CTRL_DOWN:
			ui_nextEntry();
			break;
		case SCE_CTRL_UP:
			ui_prevEntry();
			break;
		case SCE_CTRL_RIGHT:
			if (id < 6) //Sens & deadzone
				profile.gyro[id] = (profile.gyro[id] + 1) % 200;
			else if (id == 6) // Deadband
				profile.gyro[id] = min(2, profile.gyro[id] + 1);
			else if (id == 7) // Wheel mode
				profile.gyro[id] = (profile.gyro[id] + 1) % 2;
			/*else if (cfg_i < 10) // Reset wheel buttons
				profile.gyro[cfg_i] 
					= min(PHYS_BUTTONS_NUM - 1, profile.gyro[cfg_i] + 1);*/
			break;
		case SCE_CTRL_LEFT:
			if (profile.gyro[id]) 	
				profile.gyro[id]--;
			else {
				if (id < 6) //Sens & deadzone
					profile.gyro[id] = 199;
				else if (id == 6) // deadband
					profile.gyro[id] = max(0, profile.gyro[id] - 1);
				else if (id == 7) //Wheel mode
					profile.gyro[id] = 1;
				/*else if (idx < 10)  // Reset wheel btns
					profile.gyro[idx] = max(0, profile.gyro[idx] - 1);*/
			}
			break;
		case SCE_CTRL_SQUARE:
			profile.gyro[id] = profile_def.gyro[id];
			break;
		case SCE_CTRL_START:
			profile_resetGyro();
			break;
		case SCE_CTRL_CROSS:
			//if (idx == 10){
				//sceMotionReset();
			break;
		case SCE_CTRL_CIRCLE:
			ui_setMenu(MAIN_MENU);
			ui_setIdx(0);
			break;
	}
}

void inputHandler_controller(uint32_t btn){
	int8_t id = ui_entry->id;
	switch (btn) {
		case SCE_CTRL_DOWN:
			ui_nextEntry();
			break;
		case SCE_CTRL_UP:
			ui_prevEntry();
			break;
		case SCE_CTRL_RIGHT:
			if (id == 1)
				profile.controller[id] = min(5, profile.controller[id] + 1);
			else
				profile.controller[id] = !profile.controller[id];
			break;
		case SCE_CTRL_LEFT:
			if (id == 1)
				profile.controller[id] = max(0, profile.controller[id] - 1);
			else
				profile.controller[id] = !profile.controller[id];
			break;
		case SCE_CTRL_SQUARE:
			profile.controller[id] = profile_def.controller[id];
			break;
		case SCE_CTRL_START:
			profile_resetController();
			break;
		case SCE_CTRL_CIRCLE:
			ui_setMenu(MAIN_MENU);
			ui_setIdx(0);
			break;
	}
}

void inputHandler_hooks(uint32_t btn){
	switch (btn) {
		case SCE_CTRL_DOWN:
			ui_nextEntry();
			break;
		case SCE_CTRL_UP:
			ui_prevEntry();
			break;
		case SCE_CTRL_CIRCLE:
			ui_setMenu(MAIN_MENU);
			ui_setIdx(0);
			break;
	}
}

void inputHandler_credits(uint32_t btn){
	switch (btn) {
		case SCE_CTRL_DOWN:
			ui_nextEntry();
			break;
		case SCE_CTRL_UP:
			ui_prevEntry();
			break;
		case SCE_CTRL_CIRCLE:
			ui_setMenu(MAIN_MENU);
			ui_setIdx(0);
			break;
	}
}

void inputHandler_settings(uint32_t btn){
	int8_t id = ui_entry->id;
	switch (btn) {
		case SCE_CTRL_DOWN:
			ui_nextEntry();
			break;
		case SCE_CTRL_UP:
			ui_prevEntry();
			break;
		case SCE_CTRL_RIGHT:
			if (id < 2)
				profile_settings[id] 
					= min(PHYS_BUTTONS_NUM - 1, profile_settings[id] + 1);
			else if (id == 2)
				profile_settings[id] = !profile_settings[id];
			else if (id == 3)
				profile_settings[id] 
					= min(60, profile_settings[id] + 1);
			break;
		case SCE_CTRL_LEFT:
			if (id < 2)
				profile_settings[id] 
					= max(0, profile_settings[id] - 1);
			else if (id == 2)
				profile_settings[id] = !profile_settings[id];
			else if (id == 3)
				profile_settings[id] 
					= max(0, profile_settings[id] - 1);
			break;
		case SCE_CTRL_SQUARE:
			if (id <= 2)
				profile_settings[id] = profile_settings_def[id];
			break;
		case SCE_CTRL_START:
			profile_resetSettings();
			break;
		case SCE_CTRL_CIRCLE:
			ui_setMenu(MAIN_MENU);
			ui_setIdx(0);
			break;
	}
}

void inputHandler_profiles(uint32_t btn){
	switch (btn) {
		case SCE_CTRL_DOWN:
			ui_nextEntry();
			break;
		case SCE_CTRL_UP:
			ui_prevEntry();
			break;
		case SCE_CTRL_CROSS:
			switch (ui_entry->id){
				case PROFILE_GLOBAL_SAVE: profile_saveGlobal(); break;
				case PROFILE_GLOABL_LOAD: profile_loadGlobal(); break;
				case PROFILE_GLOBAL_DELETE: profile_deleteGlobal(); break;
				case PROFILE_LOCAL_SAVE: profile_saveLocal(); break;
				case PROFILE_LOCAL_LOAD: profile_loadLocal(); break;
				case PROFILE_LOCAL_DELETE: profile_deleteLocal(); break;
				default: break;
			}
			break;
		case SCE_CTRL_CIRCLE:
			ui_setMenu(MAIN_MENU);
			ui_setIdx(0);
			break;
	}
}

void ctrl_inputHandler(SceCtrlData *ctrl) {
	if ((ctrl->buttons & HW_BUTTONS[profile_settings[0]]) 
			&& (ctrl->buttons & HW_BUTTONS[profile_settings[1]]))
		return; //Menu trigger butoons should not trigger any menu actions on menu open
	if (new_frame) {
		new_frame = 0;
		
		if (ui_menu->id  == TOUCH_MENU){					
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

			switch (ui_menu->id) {
				case MAIN_MENU: inputHandler_main(HW_BUTTONS[i]); break;
				case REMAP_MENU: inputHandler_remap(HW_BUTTONS[i]); break;
				case ANALOG_MENU: inputHandler_analog(HW_BUTTONS[i]); break;
				case TOUCH_MENU: inputHandler_touch(HW_BUTTONS[i]); break;
				case GYRO_MENU: inputHandler_gyro(HW_BUTTONS[i]); break;
				case CNTRL_MENU: inputHandler_controller(HW_BUTTONS[i]); break;
				case HOOKS_MENU: inputHandler_hooks(HW_BUTTONS[i]); break;
				case SETTINGS_MENU: inputHandler_settings(HW_BUTTONS[i]); break;
				case PROFILES_MENU: inputHandler_profiles(HW_BUTTONS[i]); break;
				case CREDITS_MENU: inputHandler_credits(HW_BUTTONS[i]); break;
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