#include <vitasdkkern.h>
#include <taihen.h>
#include <stdbool.h>
#include <string.h>
#include "../vitasdkext.h"
#include "../common.h"
#include "../log.h"
#include "theme.h"
#include "hotkeys.h"
#include "fio.h"
#include "ini.h"

#define SECTION "HOTKEYS"
#define BUFFER_SIZE_HOTKEYS (200 * sizeof(char)+ 0xfff) & ~0xfff
#define PATH "ux0:/data/remaPSV2"
#define NAME_SETTINGS "HOTKEYS"
#define EXT "INI"

struct ProfileEntry hotkeys[HOTKEY__NUM];

struct ProfileEntry* hotkeys_findByKey(char* n){
	for (int i = 0; i < HOTKEY__NUM; i++)
		if (streq(hotkeys[i].key, n)) 
			return &hotkeys[i];
	return NULL;
}

void hotkeys_reset(enum HOTKEY_ID id){
	hotkeys[id].v = hotkeys[id].def;
}
void hotkeys_resetAll(){
	for (int i = 0; i < HOTKEY__NUM; i++)
		hotkeys_reset(i);
}

void addBtnsList(INI* ini, ProfileEntry* pe){
	ini_addList(ini, pe->key);
	for (int i = 0; i < HW_BUTTONS_NUM; i++)
		if (btn_has(pe->v.u, HW_BUTTONS[i]))
			ini_addListStr(ini, HW_BUTTONS_STR[i]);
}

bool generateINIHotkeys(char* buff){
	INI _ini = ini_create(buff, 99);
	INI* ini = &_ini;

	ini_addSection(ini, SECTION);
	for (int i = 0; i < HOTKEY__NUM; i++)
		addBtnsList(ini, &hotkeys[i]);
	return true;
}

void profile_parseINIButtons(INI_READER* ini, uint32_t* btns){
	while(ini_nextListVal(ini)){
		int btnId = getButtonId(ini->listVal);
		if (btnId >= 0)
			btn_add(btns, HW_BUTTONS[btnId]);
	}
}

bool parseINIHotkeys(char* buff){
	hotkeys_resetAll();

	INI_READER _ini = ini_read(buff);
	INI_READER* ini = &_ini;

	while(ini_nextEntry(ini)){
		ProfileEntry* e = hotkeys_findByKey(ini->name);
		if (e == NULL) continue;
		e->v.u = 0;
		profile_parseINIButtons(ini, &e->v.u);
	}
	return true;
}

bool hotkeys_load(){
	char* buff;
	bool ret = false;
	hotkeys_resetAll();

    //Mem allocation for buffer
	SceUID buff_uid  = ksceKernelAllocMemBlock("RemaPSV2_INI_R", 
		SCE_KERNEL_MEMBLOCK_TYPE_KERNEL_RW, BUFFER_SIZE_HOTKEYS, NULL);
	if (buff_uid < 0) 
		return false;
    if (ksceKernelGetMemBlockBase(buff_uid, (void**)&buff) != 0)
		goto ERROR;
	//Read file to buffer
	if (!fio_readFile(buff, BUFFER_SIZE_HOTKEYS, PATH, NAME_SETTINGS, EXT))
		goto ERROR;

	// Parse INI
	ret = parseINIHotkeys(buff);
	
ERROR: //Free mem and quit
	ksceKernelFreeMemBlock(buff_uid);
	return ret;
}

bool hotkeys_save(){
	bool ret = false;
	char* buff;
	
    //Mem allocation
	SceUID buff_uid  = ksceKernelAllocMemBlock("RemaPSV2_INI_W", 
		SCE_KERNEL_MEMBLOCK_TYPE_KERNEL_RW, BUFFER_SIZE_HOTKEYS, NULL);
	if (buff_uid < 0) 
		return false;
    if (ksceKernelGetMemBlockBase(buff_uid, (void**)&buff) != 0)
		goto ERROR;

	//Generate INI into buffer
	if (!generateINIHotkeys(buff))
		goto ERROR;
	
	//Write to file
	ret = fio_writeFile(buff, strlen(buff), PATH, NAME_SETTINGS, EXT);

ERROR: //Free allocated memory
	ksceKernelFreeMemBlock(buff_uid);
	return ret;
}

bool hotkeys_isDef(enum HOTKEY_ID id){
	return hotkeys[id].v.u == hotkeys[id].def.u;
}

void hotkey_set(ProfileEntry entry){
	hotkeys[entry.id] = entry; 
}

void setDefHotkeys(){
	hotkey_set((ProfileEntry){
		.id = HOTKEY_MENU,
		.type = TYPE_UINT32,
		.def.u = SCE_CTRL_START + SCE_CTRL_SQUARE, 
		.key = "MENU"});
	hotkey_set((ProfileEntry){
		.id = HOTKEY_REMAPS_TOOGLE,
		.type = TYPE_UINT32,
		.def.u = SCE_CTRL_START + SCE_CTRL_TRIANGLE, 
		.key = "REMAPS_TOGGLE"});
	hotkey_set((ProfileEntry){
		.id = HOTKEY_RESET_SOFT,
		.type = TYPE_UINT32,
		.def.u = 0, 
		.key = "RESET_SOFT"});
	hotkey_set((ProfileEntry){
		.id = HOTKEY_RESET_COLD,
		.type = TYPE_UINT32,
		.def.u = SCE_CTRL_PSBUTTON + SCE_CTRL_POWER, 
		.key = "RESET_COLD"});
	hotkey_set((ProfileEntry){
		.id = HOTKEY_STANDBY,
		.type = TYPE_UINT32,
		.def.u = 0, 
		.key = "STANDBY"});
	hotkey_set((ProfileEntry){
		.id = HOTKEY_SUSPEND,
		.type = TYPE_UINT32,
		.def.u = 0, 
		.key = "SUSPEND"});
	hotkey_set((ProfileEntry){
		.id = HOTKEY_DISPLAY_OFF,
		.type = TYPE_UINT32,
		.def.u = 0, 
		.key = "DISPLAY_OFF"});
	hotkey_set((ProfileEntry){
		.id = HOTKEY_KILL_APP,
		.type = TYPE_UINT32,
		.def.u = SCE_CTRL_SELECT + SCE_CTRL_START + SCE_CTRL_L1 + SCE_CTRL_R1, 
		.key = "KILL_APP"});
	hotkey_set((ProfileEntry){
		.id = HOTKEY_BRIGHTNESS_INC,
		.type = TYPE_UINT32,
		.def.u = SCE_CTRL_START + SCE_CTRL_UP,  
		.key = "HOTKEY_BRIGHTNESS_INC"});
	hotkey_set((ProfileEntry){
		.id = HOTKEY_BRIGHTNESS_DEC,
		.type = TYPE_UINT32,
		.def.u = SCE_CTRL_START + SCE_CTRL_DOWN,  
		.key = "HOTKEY_BRIGHTNESS_DEC"});
	hotkey_set((ProfileEntry){
		.id = HOTKEY_SAVE_BACKUP,
		.type = TYPE_UINT32,
		.def.u = SCE_CTRL_START + SCE_CTRL_L1, 
		.key = "HOTKEY_SAVE_BACKUP"});
	hotkey_set((ProfileEntry){
		.id = HOTKEY_SAVE_RESTORE,
		.type = TYPE_UINT32,
		.def.u = SCE_CTRL_START + SCE_CTRL_R1, 
		.key = "HOTKEY_SAVE_RESTORE"});
	hotkey_set((ProfileEntry){
		.id = HOTKEY_MOTION_CALIBRATE,
		.type = TYPE_UINT32,
		.def.u = SCE_CTRL_START + SCE_CTRL_CIRCLE, 
		.key = "HOTKEY_MOTION_CALIBRATE"});
}

void hotkeys_init(){
	setDefHotkeys();
	hotkeys_load();
}
void hotkeys_destroy(){
}