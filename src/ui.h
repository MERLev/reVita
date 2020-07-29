#ifndef _UI_H_
#define _UI_H_

;enum MENU_ID{
	MENU_MAIN_ID = 0,
	MENU_ANALOG_ID,
	MENU_TOUCH_ID,
	MENU_GYRO_ID,
	MENU_CONTROLLER_ID,
	MENU_HOKS_ID,
	MENU_SETTINGS_ID,
	MENU_CREDITS_ID,
	MENU_PROFILE_ID,

	MENU_PICK_BUTTON_ID,
	MENU_PICK_ANALOG_ID,
	MENU_PICK_TOUCH_POINT_ID,
	MENU_PICK_TOUCH_ZONE_ID,

	MENU_REMAP_ID,
	MENU_REMAP_TRIGGER_TYPE_ID,
	MENU_REMAP_TRIGGER_TOUCH_ID,
	MENU_REMAP_TRIGGER_GYRO_ID,
	MENU_REMAP_EMU_TYPE_ID,
	MENU_REMAP_EMU_TOUCH_ID,
	MENU_ID__NUM
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

typedef   void (*onButtonF)(uint32_t btn);
typedef   void (*onInputF)(SceCtrlData *ctrl);
typedef struct Menu{
	enum MENU_ID id;
	uint8_t num;
	uint8_t idx;
	enum MENU_ID prev;
	enum MENU_ID next;
	enum MENU_ID parent;
	char* name;
	char* footer;
	onButtonF onButton;
	onInputF onInput;
	struct MenuEntry* entries;
	uint32_t data;
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

struct RemapRule ui_ruleEdited; //Rule currently edited

void ui_openMenu(enum MENU_ID id);
void ui_openMenuSmart(enum MENU_ID id, enum MENU_ID prevId, enum MENU_ID nextId, uint32_t data);
void ui_openMenuPrev();
void ui_openMenuNext();
void ui_openMenuParent();
void ui_nextEntry();
void ui_prevEntry();

void ui_onInput(SceCtrlData *ctrl);
extern void ui_draw(const SceDisplayFrameBuf *pParam);
extern void ui_open();
extern void ui_close();
extern void ui_init();
extern void ui_destroy();

void ui_setIdx(int i);

#endif