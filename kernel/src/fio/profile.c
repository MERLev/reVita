#include <vitasdkkern.h>
#include <taihen.h>
#include <stdbool.h>
#include "profile.h"
#include "fio.h"
#include "settings.h"
#include "ini.h"
#include "../main.h"
#include "../common.h"
#include "../log.h"

#define PATH "ux0:/data/remaPSV2"
#define NAME_GLOBAL "GLOBAL"
#define NAME_HOME "HOME"
#define EXT "bin"
#define EXT_INI "INI"
#define BUFFER_SIZE (1000 * sizeof(char)+ 0xfff) & ~0xfff

enum PROF_ID profile_findIdByKey(char* n){
	for (int i = 0; i < PROF__NUM; i++)
		if (!strcmp(profile.entries[i].key, n)) 
			return i;
	return -1;
}

enum SECTION{
	SECTION_PROFILE = 0,
	SECTION_RULE,
	SECTION__NUM
};
static const char* SECTION_STR[SECTION__NUM] = {
	"PROFILE",
	"RULE"
};
enum SECTION getSectionId(char* n){
	for (int i = 0; i < SECTION__NUM; i++)
		if (!strcmp(SECTION_STR[i], n)) 
			return i;
	return -1;
};

const char* REMAP_ACTION_TYPE_STR[REMAP_ACTION_TYPE_NUM] = {
    "BUTTON",
    "LEFT_ANALOG",
    "LEFT_ANALOG_DIGITAL",
    "RIGHT_ANALOG",
    "RIGHT_ANALOG_DIGITAL",
    "FRONT_TOUCH_ZONE",
    "BACK_TOUCH_ZONE",
    "FRONT_TOUCH_POINT",
    "BACK_TOUCH_POINT",
    "GYROSCOPE",
    "REMAPSV",
    "USYSACTIONS"
};
enum REMAP_ACTION_TYPE getActionTypeId(char* n){
	for (int i = 0; i < REMAP_ACTION_TYPE_NUM; i++)
		if (!strcmp(REMAP_ACTION_TYPE_STR[i], n)) 
			return i;
	return -1;
}

const char* REMAP_ACTION_STR[REMAP_ACTION_NUM] = {
    "ANALOG_UP",
    "ANALOG_DOWN",
    "ANALOG_LEFT",
    "ANALOG_RIGHT",
    "TOUCH_ZONE_L",
    "TOUCH_ZONE_R",
    "TOUCH_ZONE_TL",
    "TOUCH_ZONE_TR",
    "TOUCH_ZONE_BL",
    "TOUCH_ZONE_BR",
    "TOUCH_CUSTOM",
    "TOUCH_SWIPE",
    "TOUCH_SWIPE_SMART_L",
    "TOUCH_SWIPE_SMART_R",
    "TOUCH_SWIPE_SMART_DPAD",
    "GYRO_UP",
    "GYRO_DOWN",
    "GYRO_LEFT",
    "GYRO_RIGHT",
    "GYRO_ROLL_LEFT",
    "GYRO_ROLL_RIGHT",
	"SYS_RESET_SOFT",
	"SYS_RESET_COLD",
	"SYS_STANDBY",
	"SYS_SUSPEND",
	"SYS_DISPLAY_OFF",
	"SYS_KILL"
};
enum REMAP_ACTION getActionId(char* n){
	for (int i = 0; i < REMAP_ACTION_NUM; i++)
		if (!strcmp(REMAP_ACTION_STR[i], n)) 
			return i;
	return -1;
}

enum REMAP_KEY{
	REMAP_KEY_PROPAGATE = 0,
	REMAP_KEY_TURBO,
	REMAP_KEY_DISABLED,
	REMAP_KEY_TRIGGER_ACTION_TYPE,
	REMAP_KEY_TRIGGER_ACTION,
	REMAP_KEY_TRIGGER_BUTTONS,
	REMAP_KEY_TRIGGER_TOUCH_ZONE,
	REMAP_KEY_EMU_ACTION_TYPE,
	REMAP_KEY_EMU_ACTION,
	REMAP_KEY_EMU_BUTTONS,
	REMAP_KEY_EMU_TOUCH_POINT,
	REMAP_KEY_EMU_TOUCH_SWIPE,
	REMAP_KEY_EMU_TOUCH_SWIPE_SMART,
	REMAP_KEY__NUM
};
const char* REMAP_KEY_STR[REMAP_KEY__NUM] = {
	"PROPAGATE",
	"TURBO",
	"DISABLED",
	"TRIGGER_ACTION_TYPE",
	"TRIGGER_ACTION",
	"TRIGGER_BUTTONS",
	"TRIGGER_TOCUH_ZONE",
	"EMU_ACTION_TYPE",
	"EMU_ACTION",
	"EMU_BUTTONS",
	"EMU_TOUCH_POINT",
	"EMU_TOUCH_SWIPE",
	"EMU_TOUCH_SWIPE_SMART"
};
enum REMAP_KEY getRemapKeyId(char* n){
	for (int i = 0; i < REMAP_KEY__NUM; i++)
		if (!strcmp(REMAP_KEY_STR[i], n)) 
			return i;
	return -1;
}

const char*  HW_BUTTONS_STR[HW_BUTTONS_NUM] = {
	"CROSS", 
	"CIRCLE", 
	"TRIANGLE", 
	"SQUARE",
	"START", 
	"SELECT", 
	"LTRIGGER", 
	"RTRIGGER",
	"UP", 
	"RIGHT", 
	"LEFT", 
	"DOWN", 
	"L1",
	"R1", 
	"L3", 
	"R3", 
	"VOLUP", 
	"VOLDOWN", 
	"POWER", 
	"PS",
	"TOUCHPAD"
};
int getButtonId(char* n){
	for (int i = 0; i < HW_BUTTONS_NUM; i++)
		if (!strcmp(HW_BUTTONS_STR[i], n)) 
			return i;
	return -1;
}

Profile profile;
Profile profile_global;
Profile profile_home;

void profile_inc(ProfileEntry* pe, unsigned int val){
	switch(pe->type){
		case TYPE_UINT32: pe->v.u = 
			pe->min.u + (pe->v.u - pe->min.u + val) % (pe->max.u - pe->min.u + 1); break;
		case TYPE_INT32: pe->v.i = 
			pe->min.i + (pe->v.i - pe->min.i + val) % (pe->max.i - pe->min.i + 1 ); break;
		case TYPE_BOOL: FLIP(pe->v.b); break;
	}
}
void profile_dec(ProfileEntry* pe, unsigned int val){
	switch(pe->type){
		case TYPE_UINT32: pe->v.u = 
			pe->min.u + (pe->v.u - pe->min.u - val + pe->max.u - pe->min.u + 1) % (pe->max.u - pe->min.u + 1); break;
		case TYPE_INT32: pe->v.i = 
			pe->min.i + (pe->v.i - pe->min.i - val + pe->max.i - pe->min.i + 1) % (pe->max.i - pe->min.i + 1); break;
		case TYPE_BOOL: FLIP(pe->v.b); break;
	}
}

void profile_resetEntry(ProfileEntry* entry){
	entry->v = entry->def;
}
void profile_resetEntryById(enum PROF_ID id){
	profile.entries[id].v = profile.entries[id].def;
}
void profile_resetProfile(Profile* p){
	for (int i = 0; i < PROF__NUM; i++){
		profile_resetEntry(&p->entries[i]);
	}
}
void profile_reset(){
	profile_resetProfile(&profile);
}

void clone(Profile* pd, Profile* ps){
	pd->titleid[0] = '\0';
	strclone(pd->titleid, ps->titleid);
	for (int i = 0; i < PROF__NUM; i++)
		pd->entries[i].v = ps->entries[i].v;
	pd->remapsNum = ps->remapsNum;
	memcpy(pd->remaps, ps->remaps, sizeof(struct RemapRule) * REMAP_NUM);
}

void profile_addRemapRule(struct RemapRule ui_ruleEdited){
	if (profile.remapsNum < (REMAP_NUM - 1)){
		profile.remaps[profile.remapsNum] = ui_ruleEdited;
		profile.remapsNum++;
	}
}

void profile_removeRemapRule(uint8_t idx){
	if (idx < 0 || idx >= profile.remapsNum)
		return;
	for (int i = idx; i < profile.remapsNum - 1; i++)
		profile.remaps[i] = profile.remaps[i + 1];
	profile.remapsNum--;
}

void profile_resetRemapRules(){
	profile.remapsNum = 0;
}

void profile_resetAnalog(){
	profile_resetEntryById(PR_AN_LEFT_DEADZONE_X);
	profile_resetEntryById(PR_AN_LEFT_DEADZONE_Y);
	profile_resetEntryById(PR_AN_RIGHT_DEADZONE_X);
	profile_resetEntryById(PR_AN_RIGHT_DEADZONE_Y);
}
void profile_resetTouch(){
	profile_resetEntryById(PR_TO_PSTV_MODE);
	profile_resetEntryById(PR_TO_SWAP);
	profile_resetEntryById(PR_TO_SWIPE_DURATION);
	profile_resetEntryById(PR_TO_SWIPE_SMART_SENS);
	profile_resetEntryById(PR_TO_DRAW_POINT);
	profile_resetEntryById(PR_TO_DRAW_SWIPE);
	profile_resetEntryById(PR_TO_DRAW_SMART_SWIPE);
}
void profile_resetGyro() {
	profile_resetEntryById(PR_GY_SENSIVITY_X);
	profile_resetEntryById(PR_GY_SENSIVITY_Y);
	profile_resetEntryById(PR_GY_SENSIVITY_Z);
	profile_resetEntryById(PR_GY_DEADZONE_X);
	profile_resetEntryById(PR_GY_DEADZONE_Y);
	profile_resetEntryById(PR_GY_DEADZONE_Z);
	profile_resetEntryById(PR_GY_DEADBAND);
	profile_resetEntryById(PR_GY_WHEEL);
}
void profile_resetController(){
	profile_resetEntryById(PR_CO_ENABLED);
	profile_resetEntryById(PR_CO_PORT);
	profile_resetEntryById(PR_CO_SWAP_BUTTONS);
}
bool generateINIProfile(Profile* p, char* buff){
	INI _ini = ini_create(buff, 99);
	INI* ini = &_ini;

	//Profile 
	ini_addSection(ini, SECTION_STR[SECTION_PROFILE]);
	ini_addStr(ini, "NAME", p->titleid);

	for (int i = 0; i < PROF__NUM; i++){
		switch (p->entries[i].type){
			case TYPE_UINT32:
				ini_addInt(ini, p->entries[i].key, p->entries[i].v.u);
				break;
			case TYPE_INT32:
				ini_addInt(ini, p->entries[i].key, p->entries[i].v.i);
				break;
			case TYPE_BOOL:
				ini_addBool(ini, p->entries[i].key, p->entries[i].v.b);
				break;
			default: break;
		}
	}

	//Remaps
	for (int i = 0; i < p->remapsNum; i++){
		struct RemapRule* r =  &p->remaps[i];
		ini_addNL(ini);
		ini_append(ini, "\n[%s:%i]", SECTION_STR[SECTION_RULE],i);
		ini_addBool(ini, REMAP_KEY_STR[REMAP_KEY_PROPAGATE], r->propagate);
		ini_addBool(ini, REMAP_KEY_STR[REMAP_KEY_TURBO], r->turbo);
		ini_addBool(ini, REMAP_KEY_STR[REMAP_KEY_DISABLED], r->disabled);
		
		//Trigger
		ini_addStr(ini, REMAP_KEY_STR[REMAP_KEY_TRIGGER_ACTION_TYPE], REMAP_ACTION_TYPE_STR[r->trigger.type]);
		if (r->trigger.type == REMAP_TYPE_BUTTON){
			ini_addList(ini, REMAP_KEY_STR[REMAP_KEY_TRIGGER_BUTTONS]);
			for (int i = 0; i < HW_BUTTONS_NUM; i++)
				if (btn_has(r->trigger.param.btn, HW_BUTTONS[i]))
					ini_addListStr(ini, HW_BUTTONS_STR[i]);
		} else {
			ini_addStr(ini, REMAP_KEY_STR[REMAP_KEY_TRIGGER_ACTION], REMAP_ACTION_STR[r->trigger.action]);
			if (r->trigger.action == REMAP_TOUCH_CUSTOM){
				ini_addList(ini, REMAP_KEY_STR[REMAP_KEY_TRIGGER_TOUCH_ZONE]);
				ini_addListInt(ini, r->trigger.param.tPoints.a.x);
				ini_addListInt(ini, r->trigger.param.tPoints.a.y);
				ini_addListInt(ini, r->trigger.param.tPoints.b.x);
				ini_addListInt(ini, r->trigger.param.tPoints.b.y);
			}
		} 

		//Emu
		ini_addStr(ini, REMAP_KEY_STR[REMAP_KEY_EMU_ACTION_TYPE], REMAP_ACTION_TYPE_STR[r->emu.type]);
		if (r->emu.type == REMAP_TYPE_BUTTON) {
			ini_addList(ini, REMAP_KEY_STR[REMAP_KEY_EMU_BUTTONS]);
			for (int i = 0; i < HW_BUTTONS_NUM; i++)
				if (btn_has(r->emu.param.btn, HW_BUTTONS[i]))
					ini_addListStr(ini, HW_BUTTONS_STR[i]);
		} else {
			ini_addStr(ini, REMAP_KEY_STR[REMAP_KEY_EMU_ACTION], REMAP_ACTION_STR[r->emu.action]);
			if (r->emu.action == REMAP_TOUCH_CUSTOM){
				ini_addList(ini, REMAP_KEY_STR[REMAP_KEY_EMU_TOUCH_POINT]);
				ini_addListInt(ini, r->emu.param.tPoint.x);
				ini_addListInt(ini, r->emu.param.tPoint.y);
			} else if (r->emu.action == REMAP_TOUCH_SWIPE_SMART_L || 
					r->emu.action == REMAP_TOUCH_SWIPE_SMART_R || 
					r->emu.action == REMAP_TOUCH_SWIPE_SMART_DPAD){
				ini_addList(ini, REMAP_KEY_STR[REMAP_KEY_EMU_TOUCH_SWIPE_SMART]);
				ini_addListInt(ini, r->emu.param.tPoint.x);
				ini_addListInt(ini, r->emu.param.tPoint.y);
			} else if (r->emu.action == REMAP_TOUCH_SWIPE){
				ini_addList(ini, REMAP_KEY_STR[REMAP_KEY_EMU_TOUCH_SWIPE]);
				ini_addListInt(ini, r->emu.param.tPoints.a.x);
				ini_addListInt(ini, r->emu.param.tPoints.a.y);
				ini_addListInt(ini, r->emu.param.tPoints.b.x);
				ini_addListInt(ini, r->emu.param.tPoints.b.y);
			}
		}
	}
	return true;
}
bool parseINIProfile(Profile* p, char* buff){
	profile_resetProfile(p);
	p->version = ksceKernelGetSystemTimeWide();
	INI_READER _ini = ini_read(buff);
	INI_READER* ini = &_ini;
	while(ini_nextEntry(ini)){
		uint32_t id = 0;
		int ruleId = 0;
		switch(getSectionId(ini->section)){
			case SECTION_PROFILE:
				//Nothing to read here
				id = profile_findIdByKey(ini->name);
				if (id < 0) break;
				switch(p->entries[id].type){
					case TYPE_UINT32: p->entries[id].v.u = parseInt(ini->val); break;
					case TYPE_INT32: p->entries[id].v.i = parseInt(ini->val); break;
					case TYPE_BOOL: p->entries[id].v.b = parseBool(ini->val); break;
					default: break;
				}
				break;
			case SECTION_RULE:
				ruleId = parseInt(ini->sectionAttr);
				if (ruleId >= REMAP_NUM)
					continue;
				if (ruleId + 1 > p->remapsNum)
					p->remapsNum = ruleId + 1;
				struct RemapRule* rr = &p->remaps[ruleId];
				switch(getRemapKeyId(ini->name)){
					case REMAP_KEY_PROPAGATE: rr->propagate = parseBool(ini->val); break;
					case REMAP_KEY_TURBO: rr->turbo = parseBool(ini->val); break;
					case REMAP_KEY_DISABLED:  rr->disabled = parseBool(ini->val); break;
					case REMAP_KEY_TRIGGER_ACTION_TYPE: 
						id = getActionTypeId(ini->val);
						if (id >= 0)
							rr->trigger.type = id;
						break;
					case REMAP_KEY_TRIGGER_ACTION: 
						id = getActionId(ini->val);
						if (id >= 0)
							rr->trigger.action = id;
						break;
					case REMAP_KEY_TRIGGER_BUTTONS: 
						while(ini_nextListVal(ini)){
							int btnId = getButtonId(ini->listVal);
							if (btnId >= 0)
								btn_add(&rr->trigger.param.btn, HW_BUTTONS[btnId]);
						}
						break;
					case REMAP_KEY_TRIGGER_TOUCH_ZONE: 
						rr->trigger.param.tPoints.a.x = parseInt(ini_nextListVal(ini));
						rr->trigger.param.tPoints.a.y = parseInt(ini_nextListVal(ini));
						rr->trigger.param.tPoints.b.x = parseInt(ini_nextListVal(ini));
						rr->trigger.param.tPoints.b.y = parseInt(ini_nextListVal(ini));
						break;
					case REMAP_KEY_EMU_ACTION_TYPE: 
						id = getActionTypeId(ini->val);
						if (id >= 0)
							rr->emu.type = id;
						break;
					case REMAP_KEY_EMU_ACTION: 
						id = getActionId(ini->val);
						if (id >= 0)
							rr->emu.action = id;
						break;
					case REMAP_KEY_EMU_BUTTONS:
						while(ini_nextListVal(ini)){
							int btnId = getButtonId(ini->listVal);
							if (btnId >= 0)
								btn_add(&rr->emu.param.btn, HW_BUTTONS[btnId]);
						}
						break;
					case REMAP_KEY_EMU_TOUCH_POINT:
					case REMAP_KEY_EMU_TOUCH_SWIPE_SMART: 
						rr->emu.param.tPoint.x = parseInt(ini_nextListVal(ini));
						rr->emu.param.tPoint.y = parseInt(ini_nextListVal(ini));
						break;
					case REMAP_KEY_EMU_TOUCH_SWIPE: 
						rr->emu.param.tPoints.a.x = parseInt(ini_nextListVal(ini));
						rr->emu.param.tPoints.a.y = parseInt(ini_nextListVal(ini));
						rr->emu.param.tPoints.b.x = parseInt(ini_nextListVal(ini));
						rr->emu.param.tPoints.b.y = parseInt(ini_nextListVal(ini));
						break;
					default: break;
				}
				break;
			default: break;
		}
	}
	return true;
}
bool readProfile(Profile* p, char* name){
	char* buff;
	bool ret = false;

    //Mem allocation for buffer
	SceUID buff_uid  = ksceKernelAllocMemBlock("RemaPSV2_INI_R", 
		SCE_KERNEL_MEMBLOCK_TYPE_KERNEL_RW, BUFFER_SIZE, NULL);
	if (buff_uid < 0) 
		return false;
    if (ksceKernelGetMemBlockBase(buff_uid, (void**)&buff) != 0)
		goto ERROR;

	//Read file to buffer
	if (!fio_readFile(buff, BUFFER_SIZE, PATH, name, EXT_INI))
		goto ERROR;

	// Parse INI
	ret = parseINIProfile(p, buff);

	if (p->titleid != name){
		strclone(p->titleid, name);
	}
	
ERROR: //Free mem and quit
	ksceKernelFreeMemBlock(buff_uid);
	return ret;
}
bool writeProfile(Profile* p, char* name){
	bool ret = false;
	char* buff;
	
    //Mem allocation
	SceUID buff_uid  = ksceKernelAllocMemBlock("RemaPSV2_INI_W", 
		SCE_KERNEL_MEMBLOCK_TYPE_KERNEL_RW, BUFFER_SIZE, NULL);
	if (buff_uid < 0) 
		return false;
    if (ksceKernelGetMemBlockBase(buff_uid, (void**)&buff) != 0)
		goto ERROR;

	//Generate INI into buffer
	if (!generateINIProfile(p, buff))
		goto ERROR;

	//Write to file
	ret = fio_writeFile(buff, strlen(buff), PATH, name, EXT_INI);

ERROR: //Free allocated memory
	ksceKernelFreeMemBlock(buff_uid);
	return ret;
}

bool profile_save(char* titleId) {
	return writeProfile(&profile, titleId);
}
bool profile_load(char* titleId) {
	if (strcmp(profile.titleid, HOME) == 0){  //If used home profile previously
		clone(&profile_home, &profile);       //copy it back to its cache
	}
	
	if (strcmp(titleid, HOME) == 0){ //If home profile requested
		clone(&profile, &profile_home);		 //Restore it from cache
		return true;
	}

	if (readProfile(&profile, titleId))      
		return true;

	clone(&profile, &profile_global);        // If no profile for title - use global
	return false;
}

void profile_localSave(){
	writeProfile(&profile, profile.titleid);
}
void profile_localLoad(){
	readProfile(&profile, profile.titleid);
}
void profile_localReset(){
	profile_resetAnalog();
	profile_resetController();
	profile_resetGyro();
	profile_resetTouch();
	profile_resetRemapRules();
}
void profile_localDelete(){
	fio_deleteFile(PATH, profile.titleid, EXT_INI);
}

void profile_saveAsGlobal(){
	clone(&profile_global, &profile);
	writeProfile(&profile_global, NAME_GLOBAL);
}
void profile_loadFromGlobal(){
	clone(&profile, &profile_global);
}
void profile_resetGlobal(){
	profile_resetProfile(&profile_global);
	writeProfile(&profile_global, NAME_GLOBAL);
}

bool profile_isDef(enum PROF_ID id){
	switch(profile.entries[id].type){
		case TYPE_UINT32: return profile.entries[id].v.u == profile.entries[id].def.u; 
		case TYPE_INT32:  return profile.entries[id].v.i == profile.entries[id].def.i; 
		case TYPE_BOOL:   return profile.entries[id].v.b == profile.entries[id].def.b; 
		default: return false;
	}
}

void setPE(ProfileEntry entry){
	profile.entries[entry.id] = entry; 
}

void setDefProfile(){
	strclone(profile.titleid, HOME);
	profile.version = ksceKernelGetSystemTimeWide();
	// Analog
	setPE((ProfileEntry){
		.id = PR_AN_LEFT_DEADZONE_X,
		.type = TYPE_UINT32,
		.def.u = 30,
		.min.u = 0,
		.max.u = 127,
		.key = "LEFT_DEADZONE_X"});
	setPE((ProfileEntry){
		.id = PR_AN_LEFT_DEADZONE_Y,
		.type = TYPE_UINT32,
		.def.u = 30,
		.min.u = 0,
		.max.u = 127,
		.key = "LEFT_DEADZONE_Y"});
	setPE((ProfileEntry){
		.id = PR_AN_RIGHT_DEADZONE_X,
		.type = TYPE_UINT32,
		.def.u = 30,
		.min.u = 0,
		.max.u = 127,
		.key = "RIGHT_DEADZONE_X"});
	setPE((ProfileEntry){
		.id = PR_AN_RIGHT_DEADZONE_Y,
		.type = TYPE_UINT32,
		.def.u = 30,
		.min.u = 0,
		.max.u = 127,
		.key = "RIGHT_DEADZONE_Y"});

	// Touch
	setPE((ProfileEntry){
		.id = PR_TO_PSTV_MODE,
		.type = TYPE_BOOL,
		.def.b = false,
		.key = "PSTV_MODE"});
	setPE((ProfileEntry){
		.id = PR_TO_SWAP,
		.type = TYPE_BOOL,
		.def.b = false,
		.key = "SWAP"});
	setPE((ProfileEntry){
		.id = PR_TO_SWIPE_DURATION,
		.type = TYPE_UINT32,
		.def.u = 50,
		.min.u = 1,
		.max.u = 1000,
		.key = "SWIPE_DURATION"});
	setPE((ProfileEntry){
		.id = PR_TO_SWIPE_SMART_SENS,
		.type = TYPE_UINT32,
		.def.u = 10,
		.min.u = 1,
		.max.u =100,
		.key = "SWIPE_SENSIVITY"});
	setPE((ProfileEntry){
		.id = PR_TO_DRAW_POINT,
		.type = TYPE_BOOL,
		.def.b = true,
		.key = "DRAW_POINTER_POINT"});
	setPE((ProfileEntry){
		.id = PR_TO_DRAW_SWIPE,
		.type = TYPE_BOOL,
		.def.b = true,
		.key = "DRAW_POINTER_SWIPE"});
	setPE((ProfileEntry){
		.id = PR_TO_DRAW_SMART_SWIPE,
		.type = TYPE_BOOL,
		.def.b = true,
		.key = "DRAW_POINTER_SMART_SWIPE"});

	// Gyro
	setPE((ProfileEntry){
		.id = PR_GY_SENSIVITY_X,
		.type = TYPE_UINT32,
		.def.u = 127,
		.min.u = 1,
		.max.u = 200,
		.key = "SENSIVITY_X"});
	setPE((ProfileEntry){
		.id = PR_GY_SENSIVITY_Y,
		.type = TYPE_UINT32,
		.def.u = 127,
		.min.u = 1,
		.max.u = 200,
		.key = "SENSIVITY_Y"});
	setPE((ProfileEntry){
		.id = PR_GY_SENSIVITY_Z,
		.type = TYPE_UINT32,
		.def.u = 127,
		.min.u = 1,
		.max.u = 200,
		.key = "SENSIVITY_Z"});
	setPE((ProfileEntry){
		.id = PR_GY_DEADZONE_X,
		.type = TYPE_UINT32,
		.def.u = 0,
		.min.u = 1,
		.max.u = 200,
		.key = "DEADZONE_X"});
	setPE((ProfileEntry){
		.id = PR_GY_DEADZONE_Y,
		.type = TYPE_UINT32,
		.def.u = 0,
		.min.u = 1,
		.max.u = 200,
		.key = "DEADZONE_Y"});
	setPE((ProfileEntry){
		.id = PR_GY_DEADZONE_Z,
		.type = TYPE_UINT32,
		.def.u = 0,
		.min.u = 1,
		.max.u = 200,
		.key = "DEADZONE_Z"});
	setPE((ProfileEntry){
		.id = PR_GY_DEADBAND,
		.type = TYPE_UINT32,
		.def.u = 0,
		.min.u = 0,
		.max.u = 2,
		.key = "DEADBAND"});
	setPE((ProfileEntry){
		.id = PR_GY_WHEEL,
		.type = TYPE_BOOL,
		.def.u = false,
		.key = "WHEEL"});

	// External controllers
	setPE((ProfileEntry){
		.id = PR_CO_ENABLED,
		.type = TYPE_BOOL,
		.def.u = false,
		.key = "ENABLED"});
	setPE((ProfileEntry){
		.id = PR_CO_PORT,
		.type = TYPE_UINT32,
		.def.u = 0,
		.min.u = 0,
		.max.u = 4,
		.key = "PORT"});
	setPE((ProfileEntry){
		.id = PR_CO_SWAP_BUTTONS,
		.type = TYPE_BOOL,
		.def.u = false,
		.key = "SWAP_BUTTONS"});

	strclone(profile_home.titleid, HOME);
	memcpy(profile_home.entries, profile.entries, sizeof(ProfileEntry) * PROF__NUM);
	strclone(profile_global.titleid, GLOBAL);
	memcpy(profile_global.entries, profile.entries, sizeof(ProfileEntry) * PROF__NUM);
}

void profile_init(){
	//Set default profile
	setDefProfile();

	//Init global profile
	if (!readProfile(&profile_global, NAME_GLOBAL)){
		profile_resetProfile(&profile_global);
		writeProfile(&profile_global, NAME_GLOBAL);
	}

	//Init home profile
	if (!readProfile(&profile_home, NAME_HOME)){
		clone(&profile_home, &profile_global);
		writeProfile(&profile_home, NAME_HOME);
	}

	//Set home as active profile
	clone(&profile, &profile_home);
}
void profile_destroy(){
}