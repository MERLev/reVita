#include <vitasdkkern.h>
#include <taihen.h>
#include <stdbool.h>
#include <string.h>
#include "../common.h"
#include "../log.h"
#include "theme.h"
#include "settings.h"
#include "fio.h"
#include "ini.h"

#define SECTION "SETTINGS"
#define BUFFER_SIZE_SETTINGS (200 * sizeof(char)+ 0xfff) & ~0xfff
#define PATH "ux0:/data/remaPSV2"
#define NAME_SETTINGS "SETTINGS"
#define EXT "INI"

struct ProfileEntry settings[SETT__NUM];

struct ProfileEntry* findByKey(char* n){
	for (int i = 0; i < SETT__NUM; i++)
		if (streq(settings[i].key, n)) 
			return &settings[i];
	return NULL;
}

void settings_reset(enum SETT_ID id){
	settings[id].v = settings[id].def;
}
void settings_resetAll(){
	for (int i = 0; i < SETT__NUM; i++)
		settings_reset(i);
}

bool generateINISettings(char* buff){
	INI _ini = ini_create(buff, 99);
	INI* ini = &_ini;

	ini_addSection(ini, SECTION);
	for (int i = 0; i < SETT__NUM; i++){
		switch(i){
			case SETT_THEME: 
				ini_addStr(ini, settings[i].key, THEME_STR[settings[i].v.u]);
				break;
			case SETT_DELAY_INIT:
				ini_addInt(ini, settings[i].key, settings[i].v.u);
				break;
			case SETT_AUTOSAVE:
			case SETT_REMAP_ENABLED:
				ini_addBool(ini, settings[i].key, settings[i].v.b);
				break;
			default: break;
		}
	}
	return true;
}

bool parseINISettings(char* buff){
	settings_resetAll();

	INI_READER _ini = ini_read(buff);
	INI_READER* ini = &_ini;

	while(ini_nextEntry(ini)){
		ProfileEntry* e = findByKey(ini->name);
		if (e == NULL) continue;
		switch(e->id){
			case SETT_THEME: 
				e->v.u = theme_findIdByKey(ini->val);
				break;
			case SETT_DELAY_INIT:
				e->v.u = parseInt(ini->val);
				break;
			case SETT_AUTOSAVE:
			case SETT_REMAP_ENABLED:
				e->v.u = parseBool(ini->val);
				break;
			default: break;
		}
	}
	return true;
}

bool settings_load(){
	char* buff;
	bool ret = false;
	settings_resetAll();

    //Mem allocation for buffer
	SceUID buff_uid  = ksceKernelAllocMemBlock("RemaPSV2_INI_R", 
		SCE_KERNEL_MEMBLOCK_TYPE_KERNEL_RW, BUFFER_SIZE_SETTINGS, NULL);
	if (buff_uid < 0) 
		return false;
    if (ksceKernelGetMemBlockBase(buff_uid, (void**)&buff) != 0)
		goto ERROR;
	//Read file to buffer
	if (!fio_readFile(buff, BUFFER_SIZE_SETTINGS, PATH, NAME_SETTINGS, EXT))
		goto ERROR;

	// Parse INI
	ret = parseINISettings(buff);
	
ERROR: //Free mem and quit
	ksceKernelFreeMemBlock(buff_uid);
	return ret;
}

bool settings_save(){
	bool ret = false;
	char* buff;
	
    //Mem allocation
	SceUID buff_uid  = ksceKernelAllocMemBlock("RemaPSV2_INI_W", 
		SCE_KERNEL_MEMBLOCK_TYPE_KERNEL_RW, BUFFER_SIZE_SETTINGS, NULL);
	if (buff_uid < 0) 
		return false;
    if (ksceKernelGetMemBlockBase(buff_uid, (void**)&buff) != 0)
		goto ERROR;

	//Generate INI into buffer
	if (!generateINISettings(buff))
		goto ERROR;
	
	//Write to file
	ret = fio_writeFile(buff, strlen(buff), PATH, NAME_SETTINGS, EXT);

ERROR: //Free allocated memory
	ksceKernelFreeMemBlock(buff_uid);
	return ret;
}

bool settings_isDef(enum SETT_ID id){
	switch(settings[id].type){
		case TYPE_UINT32: return settings[id].v.u == settings[id].def.u; break;
		case TYPE_INT32: return settings[id].v.i == settings[id].def.i; break;
		case TYPE_BOOL: return settings[id].v.b == settings[id].def.b; break;
		default: break;
	}
	return false;
}

void set(ProfileEntry entry){
	settings[entry.id] = entry; 
}

void setDefSettings(){
	set((ProfileEntry){
		.id = SETT_REMAP_ENABLED,
		.type = TYPE_BOOL,
		.def.b = true, 
		.key = "REMAP_ENABLED"});
	set((ProfileEntry){
		.id = SETT_AUTOSAVE,
		.type = TYPE_BOOL,
		.def.b = true, 
		.key = "AUTOSAVE"});
	set((ProfileEntry){
		.id = SETT_DELAY_INIT,
		.type = TYPE_UINT32,
		.def.u = 10, 
		.min.u = 0,
		.max.u = 60,
		.key = "DELAY_INIT"});
	set((ProfileEntry){
		.id = SETT_THEME,
		.def.u = THEME_DARK, 
		.type = TYPE_UINT32,
		.min.u = 0,
		.max.u = THEME__NUM - 1,
		.key = "THEME"});
}

void settings_init(){
	setDefSettings();
	settings_load();
}
void settings_destroy(){
}