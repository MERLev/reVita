#ifndef _UI_H_
#define _UI_H_

enum MENU_IDS{
	MAIN_MENU = 0,
	REMAP_MENU,
	ANALOG_MENU,
	TOUCH_MENU,
	GYRO_MENU,
	CNTRL_MENU,
	FUNCS_LIST,
	SETTINGS_MENU,
	CREDITS_MENU
}MENU_IDS;

#define MENU_MODES          9  // Menu modes num
#define CREDITS_NUM			16

extern uint8_t ui_opened;
extern uint8_t new_frame;
extern int cfg_i;
extern int menu_i;

extern void ui_draw(const SceDisplayFrameBuf *pParam);
extern void ui_open();
extern void ui_init();
extern void ui_destroy();

#endif