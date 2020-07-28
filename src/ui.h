#ifndef _UI_H_
#define _UI_H_

;enum MENU_ID{
	MAIN_MENU = 0,
	REMAP_MENU,
	ANALOG_MENU,
	TOUCH_MENU,
	GYRO_MENU,
	CNTRL_MENU,
	HOOKS_MENU,
	SETTINGS_MENU,
	CREDITS_MENU,
	PROFILES_MENU,
	REMAP_LIST,

	REMAP_NEW_TRIGGER_GROUP_SUB,
	REMAP_NEW_TRIGGER_BTN_SUB,
	REMAP_NEW_TRIGGER_COMBO_SUB,
	REMAP_NEW_TRIGGER_ANALOG_SUB,
	REMAP_NEW_TRIGGER_TOUCH_SUB,
	REMAP_NEW_TRIGGER_GYRO_SUB,
	
	REMAP_NEW_EMU_GROUP_SUB,
	REMAP_NEW_EMU_BTN_SUB,
	REMAP_NEW_EMU_COMBO_SUB,
	REMAP_NEW_EMU_ANALOG_SUB,
	REMAP_NEW_EMU_DIGITAL_ANALOG_SUB,
	REMAP_NEW_EMU_TOUCH_SUB,

	MENU_ID_NUM
}MENU_ID;

enum PROFILE_ACTIONS{
	PROFILE_GLOBAL_SAVE = 0,
	PROFILE_GLOABL_LOAD,
	PROFILE_GLOBAL_DELETE,
	PROFILE_LOCAL_SAVE,
	PROFILE_LOCAL_LOAD,
	PROFILE_LOCAL_DELETE
}PROFILE_ACTIONS;

#define HEADER_IDX          -1
#define TEXT_IDX            -2
#define NEW_RULE_IDX        -3

//typedef struct Menu Menu;
typedef struct Menu{
	enum MENU_ID id;
	uint8_t num;
	uint8_t idx;
	enum MENU_ID prev;
	enum MENU_ID next;
	enum MENU_ID parent;
	char* name;
	struct MenuEntry* entries;
}Menu;
typedef struct MenuEntry{
	char* name;
	int8_t id;
} MenuEntry;

extern uint8_t ui_opened;
extern uint8_t ui_lines;
extern uint8_t new_frame;
struct Menu* ui_menu;
MenuEntry* ui_entry;

struct RemapRule rule; //Rule currently edited

void ui_openMenu(enum MENU_ID id);
void ui_openMenuSmart(enum MENU_ID id, enum MENU_ID prevId, enum MENU_ID nextId);
void ui_prevMenu();
void ui_openMenuParent();
void ui_nextEntry();
void ui_prevEntry();
extern void ui_draw(const SceDisplayFrameBuf *pParam);
extern void ui_open();
extern void ui_init();
extern void ui_destroy();

void ui_setIdx(int i);

#endif