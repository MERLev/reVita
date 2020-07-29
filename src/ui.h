#ifndef _UI_H_
#define _UI_H_

#define VERSION				"2.1.0"

#define UI_WIDTH            480
#define UI_HEIGHT           272
#define HEADER_HEIGHT		(CHA_H + 6)
#define STR_SIZE			30

#define BOTTOM_OFFSET		5

#define COLOR_DEFAULT     	0x00C2C0BD
#define COLOR_CURSOR      	0x00FFFFFF
#define COLOR_HEADER      	0x00FF6600
#define COLOR_CURSOR_HEADER	0x00FFAA22
#define COLOR_ACTIVE      	0x0000B0B0
#define COLOR_CURSOR_ACTIVE	0x0000DDDD
#define COLOR_DANGER      	0x00000099
#define COLOR_CURSOR_DANGER	0x000000DD
#define COLOR_BG_HEADER   	0x00000000
#define COLOR_BG_BODY     	0x00171717
#define L_0    				5		//Left margin for menu
#define L_1    				18		
#define L_2    				36

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

struct Menu;
typedef void (*onButtonF)(uint32_t btn);
typedef void (*onInputF)(SceCtrlData *ctrl);
typedef void (*onDrawF)();
typedef void (*onBuildF)(struct Menu* m);
typedef struct Menu{
	enum MENU_ID id;
	enum MENU_ID prev;
	enum MENU_ID next;
	enum MENU_ID parent;

	uint8_t idx;
	char* name;
	char* footer;
	uint8_t num;
	struct MenuEntry* entries;

	onButtonF onButton;
	onInputF onInput;
	onDrawF onDraw;
	onBuildF onBuild;

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