#include <vitasdkkern.h>
#include <string.h>
#include "../../common.h"
#include "../../remap.h"
#include "../../fio/settings.h"
#include "../../fio/theme.h"
#include "../renderer.h"
#include "../gui.h"

struct RemapRule ui_ruleEdited; //Rule currently edited
int ui_ruleEditedIdx; //Index of Rule currently edited

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
				case REMAP_TOUCH_ZONE_CENTER:      strcat(str, "$i$7"); break;
				case REMAP_TOUCH_ZONE_FULL:        strcat(str, "$i$8"); break;
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
				case REMAP_SYS_RESET_SOFT:  strcat(str, "Soft reset"); break;
				case REMAP_SYS_RESET_COLD: 	strcat(str, "Reboot"); break;
				case REMAP_SYS_STANDBY:    	strcat(str, "Power Off"); break;
				case REMAP_SYS_SUSPEND:  	strcat(str, "Suspend"); break;
				case REMAP_SYS_DISPLAY_OFF: strcat(str, "Display Off"); break;
				case REMAP_SYS_KILL: 		strcat(str, "Kill App"); break;
				default: break;
			}
			break;
		case REMAP_TYPE_REMAPSV_ACTIONS: strcat(str, "$c");
			switch (ra->action){
				case REMAP_REM_SWAP_TOUCHPADS:  strcat(str, "Swap Touchpads"); break;
				default: break;
			}
			break;
		case REMAP_TYPE_DISABLED: strcat(str, "$%");
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
			if (gui_getEntry()->dataUint != NEW_RULE_IDX){
				ui_ruleEdited = profile.remaps[gui_getEntry()->dataUint];
				ui_ruleEditedIdx = gui_getEntry()->dataUint;
			} else {
				ui_ruleEditedIdx = -1;
			}
			gui_openMenu(MENU_REMAP_TRIGGER_TYPE_ID); 
			break;
		case SCE_CTRL_SQUARE:
			if (gui_getEntry()->dataUint != NEW_RULE_IDX)
				profile.remaps[gui_getEntry()->dataUint].disabled = !profile.remaps[gui_getEntry()->dataUint].disabled;
			break;
		case SCE_CTRL_TRIANGLE:
			if (gui_getEntry()->dataUint != NEW_RULE_IDX){
				profile.remaps[gui_getEntry()->dataUint].propagate = !profile.remaps[gui_getEntry()->dataUint].propagate;
				gui_menu->onBuild(gui_menu);
			}
			break;
		case SCE_CTRL_SELECT:
			if (gui_getEntry()->dataUint != NEW_RULE_IDX){
				profile.remaps[gui_getEntry()->dataUint].turbo = !profile.remaps[gui_getEntry()->dataUint].turbo;
				gui_menu->onBuild(gui_menu);
			}
			break;
		case SCE_CTRL_START:
			if (gui_getEntry()->dataUint != NEW_RULE_IDX){
				profile_removeRemapRule(gui_getEntry()->dataUint);
				gui_menu->onBuild(gui_menu);
			}
			break;
		default: onButton_generic(btn);
	}
}
void onButton_remapTriggerType(uint32_t btn){
	switch (btn) {
		case SCE_CTRL_CROSS:
			ui_ruleEdited.trigger.type = gui_getEntry()->dataUint;
			switch(gui_getEntry()->dataUint){
				case REMAP_TYPE_BUTTON: gui_openMenuSmartPtr(MENU_PICK_BUTTON_ID, 
					gui_menu->id, MENU_REMAP_EMU_TYPE_ID, &ui_ruleEdited.trigger.param.btn); break;
				case REMAP_TYPE_LEFT_ANALOG: gui_openMenuSmartPtr(MENU_PICK_ANALOG_LEFT_ID,
					gui_menu->id, MENU_REMAP_EMU_TYPE_ID, &ui_ruleEdited.trigger.action); break; 
				case REMAP_TYPE_RIGHT_ANALOG: gui_openMenuSmartPtr(MENU_PICK_ANALOG_RIGHT_ID,
					gui_menu->id, MENU_REMAP_EMU_TYPE_ID, &ui_ruleEdited.trigger.action); break;
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
			ui_ruleEdited.trigger.action = gui_getEntry()->dataUint;
			if (gui_getEntry()->dataUint == REMAP_TOUCH_CUSTOM){
				gui_openMenuSmartPtr(MENU_PICK_TOUCH_ZONE_ID, gui_menu->id, MENU_REMAP_EMU_TYPE_ID, 
					&ui_ruleEdited.trigger);
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
			ui_ruleEdited.trigger.action = gui_getEntry()->dataUint;
			gui_openMenu(MENU_REMAP_EMU_TYPE_ID);
			break;
		default: onButton_generic(btn);
	}
}
void onButton_remapEmuType(uint32_t btn){
	switch (btn) {
		case SCE_CTRL_CROSS:
			ui_ruleEdited.emu.type = gui_getEntry()->dataUint;
			switch(gui_getEntry()->dataUint){
				case REMAP_TYPE_BUTTON: 
					gui_openMenuSmartPtr(MENU_PICK_BUTTON_ID, 
						gui_menu->id, MENU_REMAP_ID, &ui_ruleEdited.emu.param.btn); 
					break;
				case REMAP_TYPE_LEFT_ANALOG:
				case REMAP_TYPE_LEFT_ANALOG_DIGITAL:
					gui_openMenuSmartPtr(MENU_PICK_ANALOG_LEFT_ID,
						gui_menu->id, MENU_REMAP_ID, &ui_ruleEdited.emu.action); 
					break;
				case REMAP_TYPE_RIGHT_ANALOG:
				case REMAP_TYPE_RIGHT_ANALOG_DIGITAL: 
					gui_openMenuSmartPtr(MENU_PICK_ANALOG_RIGHT_ID,
						gui_menu->id, MENU_REMAP_ID, &ui_ruleEdited.emu.action); 
					break;
				case REMAP_TYPE_FRONT_TOUCH_POINT: gui_openMenu(MENU_REMAP_EMU_TOUCH_FRONT_ID); break;
				case REMAP_TYPE_BACK_TOUCH_POINT: gui_openMenu(MENU_REMAP_EMU_TOUCH_BACK_ID); break;
				case REMAP_TYPE_SYSACTIONS: gui_openMenu(MENU_REMAP_EMU_SYSACTIONS_ID); break;
				case REMAP_TYPE_REMAPSV_ACTIONS: gui_openMenu(MENU_REMAP_EMU_REMAPSV_ID); break;
				case REMAP_TYPE_DISABLED: 
					profile_addRemapRule(ui_ruleEdited);
					gui_openMenu(MENU_REMAP_ID);
					break;
			};
			break;
		default: onButton_generic(btn);
	}
}
void onButton_remapEmuTouch(uint32_t btn){
	switch (btn) {
		case SCE_CTRL_CROSS:
			ui_ruleEdited.emu.action = gui_getEntry()->dataUint;
			if (gui_getEntry()->dataUint == REMAP_TOUCH_CUSTOM || 
					gui_getEntry()->dataUint == REMAP_TOUCH_SWIPE_SMART_DPAD || 
					gui_getEntry()->dataUint == REMAP_TOUCH_SWIPE_SMART_L || 
					gui_getEntry()->dataUint == REMAP_TOUCH_SWIPE_SMART_R){
				gui_openMenuSmartPtr(MENU_PICK_TOUCH_POINT_ID, gui_menu->id, MENU_REMAP_ID, &ui_ruleEdited.emu);
				break;
			} else if (gui_getEntry()->dataUint == REMAP_TOUCH_SWIPE){
				gui_openMenuSmartPtr(MENU_PICK_TOUCH_SWIPE_ID, gui_menu->id, MENU_REMAP_ID, &ui_ruleEdited.emu);
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
			ui_ruleEdited.emu.action = gui_getEntry()->dataUint;
			if (ui_ruleEditedIdx >= 0) 
				profile.remaps[ui_ruleEditedIdx] = ui_ruleEdited;
			else
				profile_addRemapRule(ui_ruleEdited);
			gui_openMenu(MENU_REMAP_ID);
			break;
		default: onButton_generic(btn);
	}
}

void onDraw_remap(uint menuY){
    int y = menuY;
	int ii = gui_calcStartingIndex(gui_menu->idx, gui_menu->num, gui_lines, BOTTOM_OFFSET);
	for (int i = ii; i < min(ii + gui_lines, gui_menu->num); i++) {
		if (gui_menu->entries[i].dataUint == NEW_RULE_IDX){
			gui_setColor(i == gui_menu->idx, 1);
			renderer_drawString(L_1, y += CHA_H, gui_menu->entries[i].name);
		} else {
			struct RemapRule* rr = &profile.remaps[gui_menu->entries[i].dataUint];
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
	(MenuEntry){.name = "<new remap rule>", .dataUint = NEW_RULE_IDX}};
static struct MenuEntry menu_remap_entries[REMAP_NUM + MENU_REMAP_NUM];
static struct Menu menu_remap = (Menu){
	.id = MENU_REMAP_ID, 
	.parent = MENU_MAIN_ID,
	.name = "REMAP RULES", 
	.footer = "$SDISABLE $TPROPAGATE $;TURBO $:REMOVE",
	.onButton = onButton_remap,
	.onDraw = onDraw_remap,
	.onBuild = onBuild_remap,
	.num = 0,
	.entries = menu_remap_entries};

static struct MenuEntry menu_remap_trigger_type_entries[] = {
	(MenuEntry){.name = "Buttons", 				.icn = ICON_BTN_CROSS,	.dataUint = REMAP_TYPE_BUTTON},
	(MenuEntry){.name = "Analog Stick Left", 	.icn = ICON_LS_UP, 		.dataUint = REMAP_TYPE_LEFT_ANALOG},
	(MenuEntry){.name = "Analog Stick Right", 	.icn = ICON_RS_UP, 		.dataUint = REMAP_TYPE_RIGHT_ANALOG},
	(MenuEntry){.name = "Touch - Front Panel", 	.icn = ICON_FT, 		.dataUint = REMAP_TYPE_FRONT_TOUCH_ZONE},
	(MenuEntry){.name = "Touch - Back Panel", 	.icn = ICON_BT, 		.dataUint = REMAP_TYPE_BACK_TOUCH_ZONE},
	(MenuEntry){.name = "Gyroscope", 			.icn = ICON_GY_ROLLLEFT,.dataUint = REMAP_TYPE_GYROSCOPE}};
static struct Menu menu_remap_trigger_type = (Menu){
	.id = MENU_REMAP_TRIGGER_TYPE_ID, 
	.parent = MENU_REMAP_ID,
	.name = "SELECT TRIGGER", 
	.onButton = onButton_remapTriggerType,
	.num = SIZE(menu_remap_trigger_type_entries),
	.entries = menu_remap_trigger_type_entries};

static struct MenuEntry menu_remap_trigger_front_touch_entries[] = {
	(MenuEntry){.name = "Whole touchpad",.icn = ICON_FULL, 	.dataUint = REMAP_TOUCH_ZONE_FULL},
	(MenuEntry){.name = "Center Zone", 		.icn = ICON_CENTER, .dataUint = REMAP_TOUCH_ZONE_CENTER},
	(MenuEntry){.name = "Left Zone", 		.icn = ICON_L, 		.dataUint = REMAP_TOUCH_ZONE_L},
	(MenuEntry){.name = "Right Zone", 		.icn = ICON_R, 		.dataUint = REMAP_TOUCH_ZONE_R},
	(MenuEntry){.name = "Top Left Zone", 	.icn = ICON_TL, 	.dataUint = REMAP_TOUCH_ZONE_TL},
	(MenuEntry){.name = "Top Right Zone", 	.icn = ICON_TR, 	.dataUint = REMAP_TOUCH_ZONE_TR},
	(MenuEntry){.name = "Bottom Left Zone", .icn = ICON_BL, 	.dataUint = REMAP_TOUCH_ZONE_BL},
	(MenuEntry){.name = "Bottom Right Zone",.icn = ICON_BR, 	.dataUint = REMAP_TOUCH_ZONE_BR},
	(MenuEntry){.name = "Custom Zone", 		.icn = ICON_FT, 	.dataUint = REMAP_TOUCH_CUSTOM}};
static struct Menu menu_remap_trigger_front_touch = (Menu){
	.id = MENU_REMAP_TRIGGER_TOUCH_FRONT_ID, 
	.parent = MENU_REMAP_TRIGGER_TYPE_ID,
	.name = "$F SELECT TOUCH POINT", 
	.onButton = onButton_remapTriggerTouch,
	.num = SIZE(menu_remap_trigger_front_touch_entries),
	.entries = menu_remap_trigger_front_touch_entries};

static struct MenuEntry menu_remap_trigger_back_touch_entries[] = {
	(MenuEntry){.name = "Whole touchpad",	.icn = ICON_FULL, 	.dataUint = REMAP_TOUCH_ZONE_FULL},
	(MenuEntry){.name = "Center Zone", 		.icn = ICON_CENTER, .dataUint = REMAP_TOUCH_ZONE_CENTER},
	(MenuEntry){.name = "Left Zone", 		.icn = ICON_L, 		.dataUint = REMAP_TOUCH_ZONE_L},
	(MenuEntry){.name = "Right Zone", 		.icn = ICON_R, 		.dataUint = REMAP_TOUCH_ZONE_R},
	(MenuEntry){.name = "Top Left Zone", 	.icn = ICON_TL, 	.dataUint = REMAP_TOUCH_ZONE_TL},
	(MenuEntry){.name = "Top Right Zone", 	.icn = ICON_TR, 	.dataUint = REMAP_TOUCH_ZONE_TR},
	(MenuEntry){.name = "Bottom Left Zone", .icn = ICON_BL, 	.dataUint = REMAP_TOUCH_ZONE_BL},
	(MenuEntry){.name = "Bottom Right Zone",.icn = ICON_BR, 	.dataUint = REMAP_TOUCH_ZONE_BR},
	(MenuEntry){.name = "Custom Zone", 		.icn = ICON_BT, 	.dataUint = REMAP_TOUCH_CUSTOM}};
static struct Menu menu_remap_trigger_back_touch = (Menu){
	.id = MENU_REMAP_TRIGGER_TOUCH_BACK_ID, 
	.parent = MENU_REMAP_TRIGGER_TYPE_ID,
	.name = "$B SELECT TOUCH POINT", 
	.onButton = onButton_remapTriggerTouch,
	.num = SIZE(menu_remap_trigger_back_touch_entries),
	.entries = menu_remap_trigger_back_touch_entries};

static struct MenuEntry menu_remap_trigger_gyro_entries[] = {
	(MenuEntry){.name = "Move left", 	.icn = ICON_GY_LEFT, 		.dataUint = REMAP_GYRO_LEFT},
	(MenuEntry){.name = "Move right", 	.icn = ICON_GY_RIGHT, 		.dataUint = REMAP_GYRO_RIGHT},
	(MenuEntry){.name = "Move up", 		.icn = ICON_GY_UP, 			.dataUint = REMAP_GYRO_UP},
	(MenuEntry){.name = "Move down", 	.icn = ICON_GY_DOWN, 		.dataUint = REMAP_GYRO_DOWN},
	(MenuEntry){.name = "Roll left", 	.icn = ICON_GY_ROLLLEFT, 	.dataUint = REMAP_GYRO_ROLL_LEFT},
	(MenuEntry){.name = "Roll right", 	.icn = ICON_GY_ROLLRIGHT, 	.dataUint = REMAP_GYRO_ROLL_RIGHT}};
static struct Menu menu_remap_trigger_gyro = (Menu){
	.id = MENU_REMAP_TRIGGER_GYRO_ID, 
	.parent = MENU_REMAP_TRIGGER_TYPE_ID,
	.name = "$E SELECT GYRO MOVEMENT", 
	.onButton = onButton_remapTriggerGyro,
	.num = SIZE(menu_remap_trigger_gyro_entries),
	.entries = menu_remap_trigger_gyro_entries};

static struct MenuEntry menu_remap_emu_type_entries[] = {
	(MenuEntry){.name = "Buttons", 						.icn = ICON_BTN_CROSS, 	.dataUint = REMAP_TYPE_BUTTON},
	(MenuEntry){.name = "Analog Stick Left", 			.icn = ICON_LS_UP, 		.dataUint = REMAP_TYPE_LEFT_ANALOG},
	(MenuEntry){.name = "Analog Stick Left [DIGITAL]", 	.icn = ICON_LS_UP,		.dataUint = REMAP_TYPE_LEFT_ANALOG_DIGITAL},
	(MenuEntry){.name = "Analog Stick Right", 			.icn = ICON_RS_UP,		.dataUint = REMAP_TYPE_RIGHT_ANALOG},
	(MenuEntry){.name = "Analog Stick Right [DIGITAL]", .icn = ICON_RS_UP,		.dataUint = REMAP_TYPE_RIGHT_ANALOG_DIGITAL},
	(MenuEntry){.name = "Front Touch", 					.icn = ICON_FT,			.dataUint = REMAP_TYPE_FRONT_TOUCH_POINT},
	(MenuEntry){.name = "Back Touch", 					.icn = ICON_BT, 		.dataUint = REMAP_TYPE_BACK_TOUCH_POINT},
	(MenuEntry){.name = "System Actions", 				.icn = ICON_DANGER,		.dataUint = REMAP_TYPE_SYSACTIONS},
	(MenuEntry){.name = "remaPSV2 settings", 			.icn = ICON_CONFIG,		.dataUint = REMAP_TYPE_REMAPSV_ACTIONS},
	(MenuEntry){.name = "Disabled", 					.icn = ICON_DISABLED, 	.dataUint = REMAP_TYPE_DISABLED}};
static struct Menu menu_remap_emu_type = (Menu){
	.id = MENU_REMAP_EMU_TYPE_ID, 
	.parent = MENU_REMAP_ID,
	.name = "SELECT EMU", 
	.onButton = onButton_remapEmuType,
	.num = SIZE(menu_remap_emu_type_entries), 
	.entries = menu_remap_emu_type_entries};

static struct MenuEntry menu_remap_emu_touch_front_entries[] = {
	(MenuEntry){.name = "Center Touch", 			.icn = ICON_CENTER, .dataUint = REMAP_TOUCH_ZONE_CENTER},
	(MenuEntry){.name = "Left Touch", 				.icn = ICON_L, 		.dataUint = REMAP_TOUCH_ZONE_L},
	(MenuEntry){.name = "Right Touch", 				.icn = ICON_R, 		.dataUint = REMAP_TOUCH_ZONE_R},
	(MenuEntry){.name = "Top Left Touch", 			.icn = ICON_TL, 	.dataUint = REMAP_TOUCH_ZONE_TL},
	(MenuEntry){.name = "Top Right Touch", 			.icn = ICON_TR, 	.dataUint = REMAP_TOUCH_ZONE_TR},
	(MenuEntry){.name = "Bottom Left Touch", 		.icn = ICON_BL, 	.dataUint = REMAP_TOUCH_ZONE_BL},
	(MenuEntry){.name = "Bottom Right Touch", 		.icn = ICON_BR, 	.dataUint = REMAP_TOUCH_ZONE_BR},
	(MenuEntry){.name = "Custom Touch", 			.icn = ICON_TOUCH, 	.dataUint = REMAP_TOUCH_CUSTOM},
	(MenuEntry){.name = "Swipe", 					.icn = ICON_SWIPE, 	.dataUint = REMAP_TOUCH_SWIPE},
	(MenuEntry){.name = "Swipe controlled with $U", .icn = ICON_SWIPE, 	.dataUint = REMAP_TOUCH_SWIPE_SMART_L},
	(MenuEntry){.name = "Swipe controlled with $u", .icn = ICON_SWIPE, 	.dataUint = REMAP_TOUCH_SWIPE_SMART_R},
	(MenuEntry){.name = "Swipe controlled with $x", .icn = ICON_SWIPE, 	.dataUint = REMAP_TOUCH_SWIPE_SMART_DPAD}};
static struct Menu menu_remap_emu_touch_front = (Menu){
	.id = MENU_REMAP_EMU_TOUCH_FRONT_ID, 
	.parent = MENU_REMAP_EMU_TYPE_ID,
	.name = "$F SELECT TOUCH POINT", 
	.onButton = onButton_remapEmuTouch,
	.num = SIZE(menu_remap_emu_touch_front_entries),
	.entries = menu_remap_emu_touch_front_entries};

static struct MenuEntry menu_remap_emu_touch_back_entries[] = {
	(MenuEntry){.name = "Center Touch", 			.icn = ICON_CENTER, .dataUint = REMAP_TOUCH_ZONE_CENTER},
	(MenuEntry){.name = "Left Touch", 				.icn = ICON_L, 		.dataUint = REMAP_TOUCH_ZONE_L},
	(MenuEntry){.name = "Right Touch", 				.icn = ICON_R, 		.dataUint = REMAP_TOUCH_ZONE_R},
	(MenuEntry){.name = "Top Left Touch", 			.icn = ICON_TL, 	.dataUint = REMAP_TOUCH_ZONE_TL},
	(MenuEntry){.name = "Top Right Touch", 			.icn = ICON_TR, 	.dataUint = REMAP_TOUCH_ZONE_TR},
	(MenuEntry){.name = "Bottom Left Touch", 		.icn = ICON_BL, 	.dataUint = REMAP_TOUCH_ZONE_BL},
	(MenuEntry){.name = "Bottom Right Touch", 		.icn = ICON_BR, 	.dataUint = REMAP_TOUCH_ZONE_BR},
	(MenuEntry){.name = "Custom Touch", 			.icn = ICON_TOUCH, 	.dataUint = REMAP_TOUCH_CUSTOM},
	(MenuEntry){.name = "Swipe", 					.icn = ICON_SWIPE, 	.dataUint = REMAP_TOUCH_SWIPE},
	(MenuEntry){.name = "Swipe controlled with $U", .icn = ICON_SWIPE, 	.dataUint = REMAP_TOUCH_SWIPE_SMART_L},
	(MenuEntry){.name = "Swipe controlled with $u", .icn = ICON_SWIPE, 	.dataUint = REMAP_TOUCH_SWIPE_SMART_R},
	(MenuEntry){.name = "Swipe controlled with $x", .icn = ICON_SWIPE, 	.dataUint = REMAP_TOUCH_SWIPE_SMART_DPAD}};
static struct Menu menu_remap_emu_touch_back = (Menu){
	.id = MENU_REMAP_EMU_TOUCH_BACK_ID, 
	.parent = MENU_REMAP_EMU_TYPE_ID,
	.name = "$B SELECT TOUCH POINT", 
	.onButton = onButton_remapEmuTouch,
	.num = SIZE(menu_remap_emu_touch_back_entries),
	.entries = menu_remap_emu_touch_back_entries};

static struct MenuEntry menu_remap_emu_sysactions_entries[] = {
	(MenuEntry){.name = "Soft reset", 	.dataUint = REMAP_SYS_RESET_SOFT},
	(MenuEntry){.name = "Reboot", 		.dataUint = REMAP_SYS_RESET_COLD},
	(MenuEntry){.name = "Power Off", 	.dataUint = REMAP_SYS_STANDBY},
	(MenuEntry){.name = "Suspend", 		.dataUint = REMAP_SYS_SUSPEND},
	(MenuEntry){.name = "Display Off", 	.dataUint = REMAP_SYS_DISPLAY_OFF},
	(MenuEntry){.name = "Kill App", 	.dataUint = REMAP_SYS_KILL}};
static struct Menu menu_remap_emu_sysactions = (Menu){
	.id = MENU_REMAP_EMU_SYSACTIONS_ID, 
	.name = "$! SYSTEM ACTIONS",
	.onButton = onButton_remapEmuActions,
	.num = SIZE(menu_remap_emu_sysactions_entries),
	.entries = menu_remap_emu_sysactions_entries};

static struct MenuEntry menu_remap_emu_remapsv_entries[] = {
	(MenuEntry){.name = "Touch : Swap touchpads", 	.dataUint = REMAP_REM_SWAP_TOUCHPADS}};
static struct Menu menu_remap_emu_remapsv = (Menu){
	.id = MENU_REMAP_EMU_REMAPSV_ID, 
	.name = "$! REMAPSV2 CONFIG OPTIONS",
	.onButton = onButton_remapEmuActions,
	.num = SIZE(menu_remap_emu_remapsv_entries),
	.entries = menu_remap_emu_remapsv_entries};

void onBuild_remap(Menu* m){
	ui_ruleEdited = remap_createRemapRule();
	m->num = profile.remapsNum + MENU_REMAP_NUM;
	for (int i = 0; i < profile.remapsNum; i++){
		m->entries[i].dataUint = i;
		m->entries[i].name = "";
	}
	for (int i = 0; i < MENU_REMAP_NUM; i++){
		int idx = i + profile.remapsNum;
		menu_remap_entries[idx].dataUint = menu_remap_entries_def[i].dataUint;
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
	gui_registerMenu(&menu_remap_emu_remapsv);
}