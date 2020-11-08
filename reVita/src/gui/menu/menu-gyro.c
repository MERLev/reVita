#include <vitasdkkern.h>
#include "../../main.h"
#include "../../common.h"
#include "../../sysactions.h"
#include "../../fio/settings.h"
#include "../gui.h"
#include "../renderer.h"
#include "../rendererv.h"

#define COMMAND_TYPE        -4

const char* STR_DEADBAND[3] = {
	"Disable",
	"Enable", 
	"Game default" 
};

void onButton_gyro(uint32_t btn){
	switch (btn) {
		case SCE_CTRL_SELECT: profile_resetGyro(); break;
		case SCE_CTRL_CROSS:
			if (gui_getEntry()->type == COMMAND_TYPE 
					&& gui_getEntry()->dataUint == REMAP_SYS_CALIBRATE_MOTION){
				sysactions_calibrateMotion();
			}
			break;
		default: 
			if (gui_getEntry()->type == COMMAND_TYPE)
				onButton_generic(btn);
			else
				onButton_genericEntries(btn);
			break;
	}
}

void onDraw_gyro(uint menuY){
    int y = menuY;
	int ii = gui_calcStartingIndex(gui_menu->idx, gui_menu->num , gui_lines, BOTTOM_OFFSET);
	for (int i = ii; i < min(ii + gui_lines, gui_menu->num); i++) {		
		
		if (gui_menu->entries[i].type == HEADER_TYPE){
			gui_drawEntry(L_1, y+= CHA_H, &gui_menu->entries[i], gui_menu->idx == i); 
			continue;
		}

		if (gui_menu->entries[i].type == COMMAND_TYPE){
			if (gui_menu->entries[i].dataUint == REMAP_SYS_CALIBRATE_MOTION){
				gui_setColor(i == gui_menu->idx, true);
				rendererv_drawString(L_1, y += CHA_H, gui_menu->entries[i].name);
				continue;
			}
		}

		ProfileEntry* pe = gui_menu->entries[i].dataPE;
		switch(pe->id){
			case PR_GY_DEADBAND:
				gui_setColor(i == gui_menu->idx, profile_isDef(pe));
				rendererv_drawString(L_1, y += CHA_H, gui_menu->entries[i].name);
				gui_drawStringFRight(0, y, "%s", STR_DEADBAND[pe->v.u]);
				break;
			default: gui_drawEntry(L_1, y+= CHA_H, &gui_menu->entries[i], gui_menu->idx == i); break;
		}
	}
	gui_drawFullScroll(ii > 0, ii + gui_lines < gui_menu->num, ((float)gui_menu->idx)/(gui_menu->num-1));
}

static struct MenuEntry menu_gyro_entries[] = {
	(MenuEntry){.name = "General", .type = HEADER_TYPE},
	(MenuEntry){.name = "$t Use DS34Motion", .dataPE = &profile.entries[PR_GY_DS4_MOTION]},
	(MenuEntry){.name = "$Q Deadband", .dataPE = &profile.entries[PR_GY_DEADBAND]},
	(MenuEntry){.name = "Sensitivity", .type = HEADER_TYPE},
	(MenuEntry){.name = "$q X Axis", .dataPE = &profile.entries[PR_GY_SENSITIVITY_X]},
	(MenuEntry){.name = "$w Y Axis", .dataPE = &profile.entries[PR_GY_SENSITIVITY_Y]},
	(MenuEntry){.name = "$E Z Axis", .dataPE = &profile.entries[PR_GY_SENSITIVITY_Z]},
	(MenuEntry){.name = "Deadzone", .type = HEADER_TYPE},
	(MenuEntry){.name = "$q X Axis", .dataPE = &profile.entries[PR_GY_DEADZONE_X]},
	(MenuEntry){.name = "$w Y Axis", .dataPE = &profile.entries[PR_GY_DEADZONE_Y]},
	(MenuEntry){.name = "$E Z Axis", .dataPE = &profile.entries[PR_GY_DEADZONE_Z]},
	(MenuEntry){.name = "Anti-Deadzone (for $UAnalogs)", .type = HEADER_TYPE},
	(MenuEntry){.name = "$q X Axis", .dataPE = &profile.entries[PR_GY_ANTIDEADZONE_X]},
	(MenuEntry){.name = "$w Y Axis", .dataPE = &profile.entries[PR_GY_ANTIDEADZONE_Y]},
	(MenuEntry){.name = "$E Z Axis", .dataPE = &profile.entries[PR_GY_ANTIDEADZONE_Z]},
	(MenuEntry){.name = "Calibration (for $YFlight/Racing)", .type = HEADER_TYPE},
	(MenuEntry){.name = "$E Calibrate now", .type = COMMAND_TYPE, .dataUint = REMAP_SYS_CALIBRATE_MOTION},
	(MenuEntry){.name = "$q X Axis", .dataPE = &profile.entries[PR_GY_CALIBRATION_X]},
	(MenuEntry){.name = "$w Y Axis", .dataPE = &profile.entries[PR_GY_CALIBRATION_Y]},
	(MenuEntry){.name = "$E Z Axis", .dataPE = &profile.entries[PR_GY_CALIBRATION_Z]}};
static struct Menu menu_gyro = (Menu){
	.id = MENU_GYRO_ID, 
	.parent = MENU_MAIN_PROFILE_ID,
	.name = "$Q PROFILE > GYROSCOPE", 
	.footer = 	"$<$>${$}CHANGE $SRESET $;RESET ALL     "
				"$CBACK                          $:CLOSE",
	.onButton = onButton_gyro,
	.onDraw = onDraw_gyro,
	.num = SIZE(menu_gyro_entries), 
	.entries = menu_gyro_entries};

void menu_initGyro(){
	gui_registerMenu(&menu_gyro);
}