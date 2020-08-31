#include <vitasdkkern.h>
#include "../../main.h"
#include "../../common.h"
#include "../../fio/settings.h"
#include "../gui.h"
#include "../renderer.h"

const char* STR_DEADBAND[3] = {
	"Game default", 
	"Enable", 
	"Disable"
};

void onButton_gyro(uint32_t btn){
	switch (btn) {
		case SCE_CTRL_START: profile_resetGyro(); break;
		default: onButton_genericEntries(btn);break;
	}
}

void onDraw_gyro(unsigned int menuY){
    int y = menuY;
	int ii = gui_calcStartingIndex(gui_menu->idx, gui_menu->num , gui_lines, BOTTOM_OFFSET);
	for (int i = ii; i < min(ii + gui_lines, gui_menu->num); i++) {		
		
		if (gui_menu->entries[i].type == HEADER_TYPE){
			gui_drawEntry(L_1, y+= CHA_H, &gui_menu->entries[i], gui_menu->idx == i); 
			continue;
		}
		
		int32_t id = gui_menu->entries[i].dataPE->id;
		switch(id){
			case PR_GY_DEADBAND:
				gui_setColor(i == gui_menu->idx, profile_isDef(id));
				renderer_drawString(L_1, y += CHA_H, gui_menu->entries[i].name);
				gui_drawStringFRight(0, y, "%s", STR_DEADBAND[profile.entries[id].v.u]);
				break;
			default: gui_drawEntry(L_1, y+= CHA_H, &gui_menu->entries[i], gui_menu->idx == i); break;
		}
	}
	gui_drawFullScroll(ii > 0, ii + gui_lines < gui_menu->num, ((float)gui_menu->idx)/(gui_menu->num-1));
}

#define MENU_GYRO_NUM 11
static struct MenuEntry menu_gyro_entries[MENU_GYRO_NUM] = {
	(MenuEntry){.name = "Sensivity", .type = HEADER_TYPE},
	(MenuEntry){.name = "$q X AXIS", .dataPE = &profile.entries[PR_GY_SENSIVITY_X]},
	(MenuEntry){.name = "$w Y Axis", .dataPE = &profile.entries[PR_GY_SENSIVITY_Y]},
	(MenuEntry){.name = "$E Z Axis", .dataPE = &profile.entries[PR_GY_SENSIVITY_Z]},
	(MenuEntry){.name = "Deadzone", .type = HEADER_TYPE},
	(MenuEntry){.name = "$q X Axis", .dataPE = &profile.entries[PR_GY_DEADZONE_X]},
	(MenuEntry){.name = "$w Y Axis", .dataPE = &profile.entries[PR_GY_DEADZONE_Y]},
	(MenuEntry){.name = "$E Z Axis", .dataPE = &profile.entries[PR_GY_DEADZONE_Z]},
	(MenuEntry){.name = "More", .type = HEADER_TYPE},
	(MenuEntry){.name = "Deadband  ", .dataPE = &profile.entries[PR_GY_DEADBAND]},
	(MenuEntry){.name = "Wheel mode", .dataPE = &profile.entries[PR_GY_WHEEL]}};
static struct Menu menu_gyro = (Menu){
	.id = MENU_GYRO_ID, 
	.parent = MENU_MAIN_ID,
	.num = MENU_GYRO_NUM, 
	.name = "$Q GYROSCOPE", 
	.footer = "$SRESET  $:RESET ALL",
	.onButton = onButton_gyro,
	.onDraw = onDraw_gyro,
	.entries = menu_gyro_entries};

void menu_initGyro(){
	gui_registerMenu(&menu_gyro);
}