#include <vitasdkkern.h>
#include <string.h>
#include "../../common.h"
#include "../../remap.h"
#include "../../fio/settings.h"
#include "../../fio/theme.h"
#include "../renderer.h"
#include "../gui.h"

struct RemapRule ui_ruleEdited; //Rule currently edited
int ui_ruleEditedIdx; //Rule currently edited

void onBuild_remap(Menu* m);

void generateRemapActionName(char* str, struct RemapAction* ra){
	bool front  = false;
	switch (ra->type){
		case REMAP_TYPE_BUTTON: gui_generateBtnComboName(str, ra->param.btn, 6);
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
			break;
		case REMAP_TYPE_SYSACTIONS: strcat(str, "$!");
			switch (ra->action){
				case REMAP_SYS_RESET_SOFT:  strcat(str, "$!Soft reset"); break;
				case REMAP_SYS_RESET_COLD: 	strcat(str, "$!Reboot"); break;
				case REMAP_SYS_STANDBY:    	strcat(str, "$!Power Off"); break;
				case REMAP_SYS_SUSPEND:  	strcat(str, "$!Suspend"); break;
				case REMAP_SYS_DISPLAY_OFF: strcat(str, "$!Display Off"); break;
				case REMAP_SYS_KILL: 		strcat(str, "$!Kill App"); break;
				default: break;
			}
			break;
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

void onButton_remap(uint32_t btn){
	switch (btn) {
		case SCE_CTRL_CROSS: 
			if (gui_getEntry()->data != NEW_RULE_IDX){
				ui_ruleEdited = profile.remaps[gui_getEntry()->data];
				ui_ruleEditedIdx = gui_getEntry()->data;
			} else {
				ui_ruleEditedIdx = -1;
			}
			gui_openMenu(MENU_REMAP_TRIGGER_TYPE_ID); 
			break;
		case SCE_CTRL_SQUARE:
			if (gui_getEntry()->data != NEW_RULE_IDX)
				profile.remaps[gui_getEntry()->data].disabled = !profile.remaps[gui_getEntry()->data].disabled;
			break;
		case SCE_CTRL_TRIANGLE:
			if (gui_getEntry()->data != NEW_RULE_IDX){
				profile.remaps[gui_getEntry()->data].propagate = !profile.remaps[gui_getEntry()->data].propagate;
				gui_menu->onBuild(gui_menu);
			}
			break;
		case SCE_CTRL_SELECT:
			if (gui_getEntry()->data != NEW_RULE_IDX){
				profile.remaps[gui_getEntry()->data].turbo = !profile.remaps[gui_getEntry()->data].turbo;
				gui_menu->onBuild(gui_menu);
			}
			break;
		case SCE_CTRL_START:
			if (gui_getEntry()->data != NEW_RULE_IDX){
				profile_removeRemapRule(gui_getEntry()->data);
				gui_menu->onBuild(gui_menu);
			}
			break;
		default: onButton_generic(btn);
	}
}
void onButton_remapTriggerType(uint32_t btn){
	switch (btn) {
		case SCE_CTRL_CROSS:
			ui_ruleEdited.trigger.type = gui_getEntry()->data;
			switch(gui_getEntry()->data){
				case REMAP_TYPE_BUTTON: gui_openMenuSmart(MENU_PICK_BUTTON_ID, 
					gui_menu->id, MENU_REMAP_EMU_TYPE_ID, (uint32_t)&ui_ruleEdited.trigger.param.btn); break;
				case REMAP_TYPE_LEFT_ANALOG: gui_openMenuSmart(MENU_PICK_ANALOG_LEFT_ID,
					gui_menu->id, MENU_REMAP_EMU_TYPE_ID, (uint32_t)&ui_ruleEdited.trigger.action); break; 
				case REMAP_TYPE_RIGHT_ANALOG: gui_openMenuSmart(MENU_PICK_ANALOG_RIGHT_ID,
					gui_menu->id, MENU_REMAP_EMU_TYPE_ID, (uint32_t)&ui_ruleEdited.trigger.action); break;
				case REMAP_TYPE_FRONT_TOUCH_ZONE: gui_openMenu(MENU_REMAP_TRIGGER_TOUCH_FRONT_ID); break;
				case REMAP_TYPE_BACK_TOUCH_ZONE: gui_openMenu(MENU_REMAP_TRIGGER_TOUCH_BACK_ID); break;
				case REMAP_TYPE_GYROSCOPE: gui_openMenu(MENU_REMAP_TRIGGER_GYRO_ID); break;
			};
			break;
		default: onButton_generic(btn);
	}
}
void onButton_remapTriggerTouch(uint32_t btn){
	switch (btn) {
		case SCE_CTRL_CROSS:
			ui_ruleEdited.trigger.action = gui_getEntry()->data;
			if (gui_getEntry()->data == REMAP_TOUCH_CUSTOM){
				gui_openMenuSmart(MENU_PICK_TOUCH_ZONE_ID, gui_menu->id, MENU_REMAP_EMU_TYPE_ID, 
					(uint32_t)&ui_ruleEdited.trigger);
				break;
			}
			//ToDo Set custom touch zones here
			gui_openMenu(MENU_REMAP_EMU_TYPE_ID);
			break;
		default: onButton_generic(btn);
	}
}
void onButton_remapTriggerGyro(uint32_t btn){
	switch (btn) {
		case SCE_CTRL_CROSS:
			ui_ruleEdited.trigger.action = gui_getEntry()->data;
			gui_openMenu(MENU_REMAP_EMU_TYPE_ID);
			break;
		default: onButton_generic(btn);
	}
}
void onButton_remapEmuType(uint32_t btn){
	switch (btn) {
		case SCE_CTRL_CROSS:
			ui_ruleEdited.emu.type = gui_getEntry()->data;
			switch(gui_getEntry()->data){
				case REMAP_TYPE_BUTTON: 
					gui_openMenuSmart(MENU_PICK_BUTTON_ID, 
						gui_menu->id, MENU_REMAP_ID, (uint32_t)&ui_ruleEdited.emu.param.btn); 
					break;
				case REMAP_TYPE_LEFT_ANALOG:
				case REMAP_TYPE_LEFT_ANALOG_DIGITAL:
					gui_openMenuSmart(MENU_PICK_ANALOG_LEFT_ID,
						gui_menu->id, MENU_REMAP_ID, (uint32_t)&ui_ruleEdited.emu.action); 
					break;
				case REMAP_TYPE_RIGHT_ANALOG:
				case REMAP_TYPE_RIGHT_ANALOG_DIGITAL: 
					gui_openMenuSmart(MENU_PICK_ANALOG_RIGHT_ID,
						gui_menu->id, MENU_REMAP_ID, (uint32_t)&ui_ruleEdited.emu.action); 
					break;
				case REMAP_TYPE_FRONT_TOUCH_POINT: gui_openMenu(MENU_REMAP_EMU_TOUCH_FRONT_ID); break;
				case REMAP_TYPE_BACK_TOUCH_POINT: gui_openMenu(MENU_REMAP_EMU_TOUCH_BACK_ID); break;
				case REMAP_TYPE_SYSACTIONS: gui_openMenu(MENU_REMAP_EMU_SYSACTIONS_ID); break;
			};
			break;
		default: onButton_generic(btn);
	}
}
void onButton_remapEmuTouch(uint32_t btn){
	switch (btn) {
		case SCE_CTRL_CROSS:
			ui_ruleEdited.emu.action = gui_getEntry()->data;
			if (gui_getEntry()->data == REMAP_TOUCH_CUSTOM || 
					gui_getEntry()->data == REMAP_TOUCH_SWIPE_SMART_DPAD || 
					gui_getEntry()->data == REMAP_TOUCH_SWIPE_SMART_L || 
					gui_getEntry()->data == REMAP_TOUCH_SWIPE_SMART_R){
				gui_openMenuSmart(MENU_PICK_TOUCH_POINT_ID, gui_menu->id, MENU_REMAP_ID, 
					(uint32_t)&ui_ruleEdited.emu);
				break;
			} else if (gui_getEntry()->data == REMAP_TOUCH_SWIPE){
				gui_openMenuSmart(MENU_PICK_TOUCH_SWIPE_ID, gui_menu->id, MENU_REMAP_ID, 
					(uint32_t)&ui_ruleEdited.emu);
				break;
			}
			if (ui_ruleEditedIdx >= 0) 
				profile.remaps[ui_ruleEditedIdx] = ui_ruleEdited;
			else
				profile_addRemapRule(ui_ruleEdited);
			gui_openMenu(MENU_REMAP_ID);
			break;
		default: onButton_generic(btn);
	}
}

void onButton_remapEmuActions(uint32_t btn){
	switch (btn) {
		case SCE_CTRL_CROSS:
			ui_ruleEdited.emu.action = gui_getEntry()->data;
			if (ui_ruleEditedIdx >= 0) 
				profile.remaps[ui_ruleEditedIdx] = ui_ruleEdited;
			else
				profile_addRemapRule(ui_ruleEdited);
			gui_openMenu(MENU_REMAP_ID);
			break;
		default: onButton_generic(btn);
	}
}

void onDraw_remap(unsigned int menuY){
    int y = menuY;
	int ii = gui_calcStartingIndex(gui_menu->idx, gui_menu->num, gui_lines, BOTTOM_OFFSET);
	for (int i = ii; i < min(ii + gui_lines, gui_menu->num); i++) {
		if (gui_menu->entries[i].data == NEW_RULE_IDX){
			gui_setColor(i == gui_menu->idx, 1);
			renderer_drawString(L_1, y += CHA_H, gui_menu->entries[i].name);
		} else {
			struct RemapRule* rr = &profile.remaps[gui_menu->entries[i].data];
			gui_setColorExt(i == gui_menu->idx, true, !rr->propagate || rr->disabled);
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
			gui_setColorExt(i == gui_menu->idx, true, rr->disabled);
			renderer_drawString(L_1 + len + 3*CHA_W, y, str);
			if (rr->turbo)
				renderer_drawString(UI_WIDTH - 4*CHA_W, y, "$M");
		}
	}
	gui_drawFullScroll(ii > 0, ii + gui_lines < gui_menu->num, ((float)gui_menu->idx) / (gui_menu->num - 1));
}

#define MENU_REMAP_NUM 1
static struct MenuEntry menu_remap_entries_def[MENU_REMAP_NUM] = {
	(MenuEntry){.name = "<new remap rule>", .data = NEW_RULE_IDX}};
static struct MenuEntry menu_remap_entries[REMAP_NUM + MENU_REMAP_NUM];
static struct Menu menu_remap = (Menu){
	.id = MENU_REMAP_ID, 
	.parent = MENU_MAIN_ID,
	.num = 0, 
	.name = "REMAP RULES", 
	.footer = "$SDISABLE $TPROPAGATE $;TURBO $:REMOVE",
	.onButton = onButton_remap,
	.onDraw = onDraw_remap,
	.onBuild = onBuild_remap,
	.entries = menu_remap_entries};

#define MENU_REMAP_TRIGGER_TYPE_NUM 6
static struct MenuEntry menu_remap_trigger_type_entries[MENU_REMAP_TRIGGER_TYPE_NUM] = {
	(MenuEntry){.name = "$X Buttons", .data = REMAP_TYPE_BUTTON},
	(MenuEntry){.name = "$U Analog Stick Left", .data = REMAP_TYPE_LEFT_ANALOG},
	(MenuEntry){.name = "$u Analog Stick Right", .data = REMAP_TYPE_RIGHT_ANALOG},
	(MenuEntry){.name = "$F Touch - Front Panel", .data = REMAP_TYPE_FRONT_TOUCH_ZONE},
	(MenuEntry){.name = "$B Touch - Back Panel", .data = REMAP_TYPE_BACK_TOUCH_ZONE},
	(MenuEntry){.name = "$Q Gyroscope", .data = REMAP_TYPE_GYROSCOPE}
};
static struct Menu menu_remap_trigger_type = (Menu){
	.id = MENU_REMAP_TRIGGER_TYPE_ID, 
	.parent = MENU_REMAP_ID,
	.num = MENU_REMAP_TRIGGER_TYPE_NUM, 
	.name = "SELECT TRIGGER", 
	.onButton = onButton_remapTriggerType,
	.entries = menu_remap_trigger_type_entries};

#define MENU_REMAP_TRIGGER_TOUCH_FRONT_NUM 7
static struct MenuEntry menu_remap_trigger_front_touch_entries[MENU_REMAP_TRIGGER_TOUCH_FRONT_NUM] = {
	(MenuEntry){.name = "$1 Left Zone", .data = REMAP_TOUCH_ZONE_L},
	(MenuEntry){.name = "$2 Right Zone", .data = REMAP_TOUCH_ZONE_R},
	(MenuEntry){.name = "$3 Top Left Zone", .data = REMAP_TOUCH_ZONE_TL},
	(MenuEntry){.name = "$4 Top Right Zone", .data = REMAP_TOUCH_ZONE_TR},
	(MenuEntry){.name = "$5 Bottom Left Zone", .data = REMAP_TOUCH_ZONE_BL},
	(MenuEntry){.name = "$6 Bottom Right Zone", .data = REMAP_TOUCH_ZONE_BR},
	(MenuEntry){.name = "$F Custom Zone", .data = REMAP_TOUCH_CUSTOM}
};
static struct Menu menu_remap_trigger_front_touch = (Menu){
	.id = MENU_REMAP_TRIGGER_TOUCH_FRONT_ID, 
	.parent = MENU_REMAP_TRIGGER_TYPE_ID,
	.num = MENU_REMAP_TRIGGER_TOUCH_FRONT_NUM, 
	.name = "$F SELECT TOUCH POINT", 
	.onButton = onButton_remapTriggerTouch,
	.entries = menu_remap_trigger_front_touch_entries};

#define MENU_REMAP_TRIGGER_TOUCH_BACK_NUM 7
static struct MenuEntry menu_remap_trigger_back_touch_entries[MENU_REMAP_TRIGGER_TOUCH_BACK_NUM] = {
	(MenuEntry){.name = "$1 Left Zone", .data = REMAP_TOUCH_ZONE_L},
	(MenuEntry){.name = "$2 Right Zone", .data = REMAP_TOUCH_ZONE_R},
	(MenuEntry){.name = "$3 Top Left Zone", .data = REMAP_TOUCH_ZONE_TL},
	(MenuEntry){.name = "$4 Top Right Zone", .data = REMAP_TOUCH_ZONE_TR},
	(MenuEntry){.name = "$5 Bottom Left Zone", .data = REMAP_TOUCH_ZONE_BL},
	(MenuEntry){.name = "$6 Bottom Right Zone", .data = REMAP_TOUCH_ZONE_BR},
	(MenuEntry){.name = "$B Custom Zone", .data = REMAP_TOUCH_CUSTOM}
};
static struct Menu menu_remap_trigger_back_touch = (Menu){
	.id = MENU_REMAP_TRIGGER_TOUCH_BACK_ID, 
	.parent = MENU_REMAP_TRIGGER_TYPE_ID,
	.num = MENU_REMAP_TRIGGER_TOUCH_BACK_NUM, 
	.name = "$B SELECT TOUCH POINT", 
	.onButton = onButton_remapTriggerTouch,
	.entries = menu_remap_trigger_back_touch_entries};

#define MENU_REMAP_TRIGGER_GYRO_NUM 6
static struct MenuEntry menu_remap_trigger_gyro_entries[MENU_REMAP_TRIGGER_GYRO_NUM] = {
	(MenuEntry){.name = "$q Move left", .data = REMAP_GYRO_LEFT},
	(MenuEntry){.name = "$e Move right", .data = REMAP_GYRO_RIGHT},
	(MenuEntry){.name = "$w Move up", .data = REMAP_GYRO_UP},
	(MenuEntry){.name = "$s Move down", .data = REMAP_GYRO_DOWN},
	(MenuEntry){.name = "$Q Roll left", .data = REMAP_GYRO_ROLL_LEFT},
	(MenuEntry){.name = "$E Roll right", .data = REMAP_GYRO_ROLL_RIGHT}
};
static struct Menu menu_remap_trigger_gyro = (Menu){
	.id = MENU_REMAP_TRIGGER_GYRO_ID, 
	.parent = MENU_REMAP_TRIGGER_TYPE_ID,
	.num = MENU_REMAP_TRIGGER_GYRO_NUM, 
	.name = "$E SELECT GYRO MOVEMENT", 
	.onButton = onButton_remapTriggerGyro,
	.entries = menu_remap_trigger_gyro_entries};

#define MENU_REMAP_EMU_TYPE_NUM 8
static struct MenuEntry menu_remap_emu_type_entries[MENU_REMAP_EMU_TYPE_NUM] = {
	(MenuEntry){.name = "$X Buttons", .data = REMAP_TYPE_BUTTON},
	(MenuEntry){.name = "$U Analog Stick Left", .data = REMAP_TYPE_LEFT_ANALOG},
	(MenuEntry){.name = "$U Analog Stick Left [DIGITAL]", .data = REMAP_TYPE_LEFT_ANALOG_DIGITAL},
	(MenuEntry){.name = "$u Analog Stick Right", .data = REMAP_TYPE_RIGHT_ANALOG},
	(MenuEntry){.name = "$u Analog Stick Right [DIGITAL]", .data = REMAP_TYPE_RIGHT_ANALOG_DIGITAL},
	(MenuEntry){.name = "$F Front Touch", .data = REMAP_TYPE_FRONT_TOUCH_POINT},
	(MenuEntry){.name = "$B Back Touch", .data = REMAP_TYPE_BACK_TOUCH_POINT},
	(MenuEntry){.name = "$! Actions", .data = REMAP_TYPE_SYSACTIONS}
};
static struct Menu menu_remap_emu_type = (Menu){
	.id = MENU_REMAP_EMU_TYPE_ID, 
	.parent = MENU_REMAP_ID,
	.num = MENU_REMAP_EMU_TYPE_NUM, 
	.name = "SELECT EMU", 
	.onButton = onButton_remapEmuType,
	.entries = menu_remap_emu_type_entries};

#define MENU_REMAP_EMU_TOUCH_FRONT_NUM 11
static struct MenuEntry menu_remap_emu_touch_front_entries[MENU_REMAP_EMU_TOUCH_FRONT_NUM] = {
	(MenuEntry){.name = "$1 Left Touch", .data = REMAP_TOUCH_ZONE_L},
	(MenuEntry){.name = "$2 Right Touch", .data = REMAP_TOUCH_ZONE_R},
	(MenuEntry){.name = "$3 Top Left Touch", .data = REMAP_TOUCH_ZONE_TL},
	(MenuEntry){.name = "$4 Top Right Touch", .data = REMAP_TOUCH_ZONE_TR},
	(MenuEntry){.name = "$5 Bottom Left Touch", .data = REMAP_TOUCH_ZONE_BL},
	(MenuEntry){.name = "$6 Bottom Right Touch", .data = REMAP_TOUCH_ZONE_BR},
	(MenuEntry){.name = "$i Custom Touch", .data = REMAP_TOUCH_CUSTOM},
	(MenuEntry){.name = "$j Swipe", .data = REMAP_TOUCH_SWIPE},
	(MenuEntry){.name = "$j Swipe controlled with $U", .data = REMAP_TOUCH_SWIPE_SMART_L},
	(MenuEntry){.name = "$j Swipe controlled with $u", .data = REMAP_TOUCH_SWIPE_SMART_R},
	(MenuEntry){.name = "$j Swipe controlled with $x", .data = REMAP_TOUCH_SWIPE_SMART_DPAD}};
static struct Menu menu_remap_emu_touch_front = (Menu){
	.id = MENU_REMAP_EMU_TOUCH_FRONT_ID, 
	.parent = MENU_REMAP_EMU_TYPE_ID,
	.num = MENU_REMAP_EMU_TOUCH_FRONT_NUM, 
	.name = "$F SELECT TOUCH POINT", 
	.onButton = onButton_remapEmuTouch,
	.entries = menu_remap_emu_touch_front_entries};

#define MENU_REMAP_EMU_TOUCH_BACK_NUM 11
static struct MenuEntry menu_remap_emu_touch_back_entries[MENU_REMAP_EMU_TOUCH_FRONT_NUM] = {
	(MenuEntry){.name = "$1 Left Touch", .data = REMAP_TOUCH_ZONE_L},
	(MenuEntry){.name = "$2 Right Touch", .data = REMAP_TOUCH_ZONE_R},
	(MenuEntry){.name = "$3 Top Left Touch", .data = REMAP_TOUCH_ZONE_TL},
	(MenuEntry){.name = "$4 Top Right Touch", .data = REMAP_TOUCH_ZONE_TR},
	(MenuEntry){.name = "$5 Bottom Left Touch", .data = REMAP_TOUCH_ZONE_BL},
	(MenuEntry){.name = "$6 Bottom Right Touch", .data = REMAP_TOUCH_ZONE_BR},
	(MenuEntry){.name = "$i Custom Touch", .data = REMAP_TOUCH_CUSTOM},
	(MenuEntry){.name = "$j Swipe", .data = REMAP_TOUCH_SWIPE},
	(MenuEntry){.name = "$j Swipe controlled with $U", .data = REMAP_TOUCH_SWIPE_SMART_L},
	(MenuEntry){.name = "$j Swipe controlled with $u", .data = REMAP_TOUCH_SWIPE_SMART_R},
	(MenuEntry){.name = "$j Swipe controlled with $x", .data = REMAP_TOUCH_SWIPE_SMART_DPAD}};
static struct Menu menu_remap_emu_touch_back = (Menu){
	.id = MENU_REMAP_EMU_TOUCH_BACK_ID, 
	.parent = MENU_REMAP_EMU_TYPE_ID,
	.num = MENU_REMAP_EMU_TOUCH_BACK_NUM, 
	.name = "$B SELECT TOUCH POINT", 
	.onButton = onButton_remapEmuTouch,
	.entries = menu_remap_emu_touch_back_entries};

#define MENU_REMAP_EMU_SYSACTIONS_NUM 6
static struct MenuEntry menu_remap_emu_sysactions_entries[MENU_REMAP_EMU_SYSACTIONS_NUM] = {
	(MenuEntry){.name = "Soft reset", .data = REMAP_SYS_RESET_SOFT},
	(MenuEntry){.name = "Reboot", .data = REMAP_SYS_RESET_COLD},
	(MenuEntry){.name = "Power Off", .data = REMAP_SYS_STANDBY},
	(MenuEntry){.name = "Suspend", .data = REMAP_SYS_SUSPEND},
	(MenuEntry){.name = "Display Off", .data = REMAP_SYS_DISPLAY_OFF},
	(MenuEntry){.name = "Kill App", .data = REMAP_SYS_KILL}};
static struct Menu menu_remap_emu_sysactions = (Menu){
	.id = MENU_REMAP_EMU_SYSACTIONS_ID, 
	.num = MENU_REMAP_EMU_SYSACTIONS_NUM, 
	.name = "$! SYSTEM ACTIONS",
	.onButton = onButton_remapEmuActions,
	.entries = menu_remap_emu_sysactions_entries};

void onBuild_remap(Menu* m){
	ui_ruleEdited = remap_createRemapRule();
	m->num = profile.remapsNum + MENU_REMAP_NUM;
	for (int i = 0; i < profile.remapsNum; i++){
		m->entries[i].data = i;
		m->entries[i].name = "";
	}
	for (int i = 0; i < MENU_REMAP_NUM; i++){
		int idx = i + profile.remapsNum;
		menu_remap_entries[idx].data = menu_remap_entries_def[i].data;
		menu_remap_entries[idx].name = menu_remap_entries_def[i].name;
	}
}
void menu_initRemap(){
	gui_registerMenu(&menu_remap);
	gui_registerMenu(&menu_remap_trigger_type);
	gui_registerMenu(&menu_remap_trigger_front_touch);
	gui_registerMenu(&menu_remap_trigger_back_touch);
	gui_registerMenu(&menu_remap_trigger_gyro);
	gui_registerMenu(&menu_remap_emu_type);
	gui_registerMenu(&menu_remap_emu_touch_front);
	gui_registerMenu(&menu_remap_emu_touch_back);
	gui_registerMenu(&menu_remap_emu_sysactions);
}