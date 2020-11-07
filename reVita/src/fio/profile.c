#include <vitasdkkern.h>
#include <taihen.h>
#include <stdbool.h>
#include "profile.h"
#include "fio.h"
#include "settings.h"
#include "ini.h"
#include "../gui/gui.h"
#include "../main.h"
#include "../common.h"
#include "../log.h"

#define PATH "ux0:/data/reVita/Profile/"
#define NAME_GLOBAL "GLOBAL"
#define NAME_SHARED "SHARED"
#define NAME_HOME "HOME"
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
    "USYSACTIONS",
	"REMAPSV2",
	"DISABLED"
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
    "TOUCH_ZONE_FULL",
    "TOUCH_ZONE_CENTER",
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
    "GYRO_SIM_UP",
    "GYRO_SIM_DOWN",
    "GYRO_SIM_LEFT",
    "GYRO_SIM_RIGHT",
    "GYRO_SIM_ROLL_LEFT",
    "GYRO_SIM_ROLL_RIGHT",
	"SYS_RESET_SOFT",
	"SYS_RESET_COLD",
	"SYS_STANDBY",
	"SYS_SUSPEND",
	"SYS_DISPLAY_OFF",
	"SYS_KILL",
	"SYS_BRIGHTNESS_INC",
	"SYS_BRIGHTNESS_DEC",
    "REMAP_SYS_SAVE_BACKUP",
    "REMAP_SYS_SAVE_RESTORE",
	"REMAP_SYS_SAVE_DELETE",
    "REMAP_SYS_CALIBRATE_MOTION",
	"REM_SWAP_TOUCHPADS"
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
	REMAP_KEY_EMU_REMAPSV_ACTION,
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
	"EMU_TOUCH_SWIPE_SMART",
	"EMU_REMAPSV_ACTION"
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
	"UP", 
	"RIGHT", 
	"LEFT", 
	"DOWN", 
	"START", 
	"SELECT", 
	"L1",
	"R1", 
	"L2", 
	"R2",
	"L3", 
	"R3", 
	"VOLUP", 
	"VOLDOWN", 
	"POWER", 
	"PS",
	"TOUCHPAD",
	"HEADPHONE"
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

void profile_inc(ProfileEntry* pe, uint val){
	switch(pe->type){
		case TYPE_UINT32: pe->v.u = 
			pe->min.u + (pe->v.u - pe->min.u + val) % (pe->max.u - pe->min.u + 1); break;
		case TYPE_INT32: pe->v.i = 
			pe->min.i + (pe->v.i - pe->min.i + val) % (pe->max.i - pe->min.i + 1 ); break;
		case TYPE_BOOL: FLIP(pe->v.b); break;
	}
}
void profile_dec(ProfileEntry* pe, uint val){
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
	p->remapsNum = 0;
	for (int i = 0; i < REMAP_NUM; i++){
		p->remaps[i] = remap_createRemapRule();
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
	if (idx >= profile.remapsNum)
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
	profile_resetEntryById(PR_AN_MODE_WIDE);
}
void profile_resetTouch(){
	profile_resetEntryById(PR_TO_SWAP);
	profile_resetEntryById(PR_TO_SWIPE_DURATION);
	profile_resetEntryById(PR_TO_SWIPE_SMART_SENS);
	profile_resetEntryById(PR_TO_DRAW_POINT);
	profile_resetEntryById(PR_TO_DRAW_SWIPE);
	profile_resetEntryById(PR_TO_DRAW_SMART_SWIPE);
	profile_resetEntryById(PR_TO_DRAW_NATIVE);
}
void profile_resetGyro() {
	profile_resetEntryById(PR_GY_SENSITIVITY_X);
	profile_resetEntryById(PR_GY_SENSITIVITY_Y);
	profile_resetEntryById(PR_GY_SENSITIVITY_Z);
	profile_resetEntryById(PR_GY_DEADZONE_X);
	profile_resetEntryById(PR_GY_DEADZONE_Y);
	profile_resetEntryById(PR_GY_DEADZONE_Z);
	profile_resetEntryById(PR_GY_ANTIDEADZONE_X);
	profile_resetEntryById(PR_GY_ANTIDEADZONE_Y);
	profile_resetEntryById(PR_GY_ANTIDEADZONE_Z);
	profile_resetEntryById(PR_GY_CALIBRATION_X);
	profile_resetEntryById(PR_GY_CALIBRATION_Y);
	profile_resetEntryById(PR_GY_CALIBRATION_Z);
	profile_resetEntryById(PR_GY_DS4_MOTION);
	profile_resetEntryById(PR_GY_DEADBAND);
}
void profile_resetController(){
	profile_resetEntryById(PR_CO_SWAP_BUTTONS);
	profile_resetEntryById(PR_CO_EMULATE_DS4);
}
void profile_resetMore(){
	profile_resetEntryById(PR_MO_BLANK_FRAME);
	profile_resetEntryById(PR_MO_DELAY_START);
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
			for (int btnId = 0; btnId < HW_BUTTONS_NUM; btnId++)
				if (btn_has(r->trigger.param.btn, HW_BUTTONS[btnId]))
					ini_addListStr(ini, HW_BUTTONS_STR[btnId]);
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
		switch (r->emu.type){
			case REMAP_TYPE_BUTTON:
				ini_addList(ini, REMAP_KEY_STR[REMAP_KEY_EMU_BUTTONS]);
				for (int btnId = 0; btnId < HW_BUTTONS_NUM; btnId++)
					if (btn_has(r->emu.param.btn, HW_BUTTONS[btnId]))
						ini_addListStr(ini, HW_BUTTONS_STR[btnId]);
				break;
			case REMAP_TYPE_REMAPSV_ACTIONS:
				ini_addStr(ini, REMAP_KEY_STR[REMAP_KEY_EMU_REMAPSV_ACTION], profile.entries[r->emu.action].key);
				break;
			default: 
				ini_addStr(ini, REMAP_KEY_STR[REMAP_KEY_EMU_ACTION], REMAP_ACTION_STR[r->emu.action]);
				switch(r->emu.action){
					case REMAP_TOUCH_CUSTOM:
						ini_addList(ini, REMAP_KEY_STR[REMAP_KEY_EMU_TOUCH_POINT]);
						ini_addListInt(ini, r->emu.param.tPoint.x);
						ini_addListInt(ini, r->emu.param.tPoint.y);
						break;
					case REMAP_TOUCH_SWIPE:
						ini_addList(ini, REMAP_KEY_STR[REMAP_KEY_EMU_TOUCH_SWIPE]);
						ini_addListInt(ini, r->emu.param.tPoints.a.x);
						ini_addListInt(ini, r->emu.param.tPoints.a.y);
						ini_addListInt(ini, r->emu.param.tPoints.b.x);
						ini_addListInt(ini, r->emu.param.tPoints.b.y);
						break;
					case REMAP_TOUCH_SWIPE_SMART_L:
					case REMAP_TOUCH_SWIPE_SMART_R:
					case REMAP_TOUCH_SWIPE_SMART_DPAD:
						ini_addList(ini, REMAP_KEY_STR[REMAP_KEY_EMU_TOUCH_SWIPE_SMART]);
						ini_addListInt(ini, r->emu.param.tPoint.x);
						ini_addListInt(ini, r->emu.param.tPoint.y);
						break;
					default: break;
				}
				break;
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
		int32_t id = 0;
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
					case REMAP_KEY_EMU_REMAPSV_ACTION:
						for (int profId = 0; profId < PROF__NUM; profId++){
							if (streq(ini->val, profile.entries[profId].key)){
								rr->emu.action = profId;
								break;
							}
						}
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
    LOG("profile_save('%s')\n", titleid);
	return writeProfile(&profile, titleId);
}
bool profile_load(char* titleId) {
    LOG("profile_load('%s')\n", titleid);
	if (strcmp(profile.titleid, HOME) == 0){  //If used home profile previously
		clone(&profile_home, &profile);       //copy it back to its cache
	}
	
	if (strcmp(titleid, HOME) == 0){          //If home profile requested
		clone(&profile, &profile_home);		  //Restore it from cache
		return true;
	}

	if (readProfile(&profile, titleId)){
		return true;
	}

	clone(&profile, &profile_global);        // If no profile for title - use global
	return false;
}

void profile_saveLocal(){
	writeProfile(&profile, titleid);
}
void profile_loadLocal(){
	readProfile(&profile, titleid);
}
void profile_resetLocal(){
	profile_resetAnalog();
	profile_resetController();
	profile_resetGyro();
	profile_resetTouch();
	profile_resetRemapRules();
}
void profile_deleteLocal(){
	fio_deleteFile(PATH, profile.titleid, EXT_INI);
}

void profile_saveAsGlobal(){
	clone(&profile_global, &profile);
	writeProfile(&profile_global, NAME_GLOBAL);
}
void profile_loadFromGlobal(){
	char titleId_orig[32];
	strclone(titleId_orig, profile.titleid);
	clone(&profile, &profile_global);
	strclone(profile.titleid, titleId_orig);
}
void profile_resetGlobal(){
	profile_resetProfile(&profile_global);
	writeProfile(&profile_global, NAME_GLOBAL);
}

void profile_saveAsShared(){
	writeProfile(&profile, NAME_SHARED);
}
void profile_loadFromShared(){
	readProfile(&profile, NAME_SHARED);
	strclone(profile.titleid, titleid);
}
void profile_deleteShared(){
	fio_deleteFile(PATH, NAME_SHARED, EXT_INI);
}

bool profile_isDef(ProfileEntry* pe){
	switch(pe->type){
		case TYPE_UINT32: return pe->v.u == pe->def.u; 
		case TYPE_INT32:  return pe->v.i == pe->def.i; 
		case TYPE_BOOL:   return pe->v.b == pe->def.b; 
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
		.id = PR_AN_MODE_WIDE,
		.type = TYPE_BOOL,
		.def.u = 1,
		.key = "Analogs Wide mode"});
	setPE((ProfileEntry){
		.id = PR_AN_LEFT_DEADZONE_X,
		.type = TYPE_UINT32,
		.def.u = 30,
		.min.u = 0,
		.max.u = 127,
		.key = "Left Analog deadzone X"});
	setPE((ProfileEntry){
		.id = PR_AN_LEFT_DEADZONE_Y,
		.type = TYPE_UINT32,
		.def.u = 30,
		.min.u = 0,
		.max.u = 127,
		.key = "Left analog deadzone Y"});
	setPE((ProfileEntry){
		.id = PR_AN_RIGHT_DEADZONE_X,
		.type = TYPE_UINT32,
		.def.u = 30,
		.min.u = 0,
		.max.u = 127,
		.key = "Right analog deadzone X"});
	setPE((ProfileEntry){
		.id = PR_AN_RIGHT_DEADZONE_Y,
		.type = TYPE_UINT32,
		.def.u = 30,
		.min.u = 0,
		.max.u = 127,
		.key = "Right analog deadzone Y"});

	// Touch
	setPE((ProfileEntry){
		.id = PR_TO_SWAP,
		.type = TYPE_BOOL,
		.def.b = false,
		.key = "Swap touchpads"});
	setPE((ProfileEntry){
		.id = PR_TO_SWIPE_DURATION,
		.type = TYPE_UINT32,
		.def.u = 50,
		.min.u = 1,
		.max.u = 1000,
		.key = "Swipe duration"});
	setPE((ProfileEntry){
		.id = PR_TO_SWIPE_SMART_SENS,
		.type = TYPE_UINT32,
		.def.u = 10,
		.min.u = 1,
		.max.u =100,
		.key = "Swipe sensivity"});
	setPE((ProfileEntry){
		.id = PR_TO_DRAW_POINT,
		.type = TYPE_BOOL,
		.def.b = true,
		.key = "Show emulated touch pointer"});
	setPE((ProfileEntry){
		.id = PR_TO_DRAW_SWIPE,
		.type = TYPE_BOOL,
		.def.b = true,
		.key = "Show emulated swipe"});
	setPE((ProfileEntry){
		.id = PR_TO_DRAW_SMART_SWIPE,
		.type = TYPE_BOOL,
		.def.b = true,
		.key = "Show emulated controlled swipe"});
	setPE((ProfileEntry){
		.id = PR_TO_DRAW_NATIVE,
		.type = TYPE_BOOL,
		.def.b = false,
		.key = "Show native touch pointer"});

	// Gyro
	setPE((ProfileEntry){
		.id = PR_GY_SENSITIVITY_X,
		.type = TYPE_UINT32,
		.def.u = 25,
		.min.u = 1,
		.max.u = 100,
		.key = "Gyro sensivity X"});
	setPE((ProfileEntry){
		.id = PR_GY_SENSITIVITY_Y,
		.type = TYPE_UINT32,
		.def.u = 25,
		.min.u = 1,
		.max.u = 100,
		.key = "Gyro sensivity Y"});
	setPE((ProfileEntry){
		.id = PR_GY_SENSITIVITY_Z,
		.type = TYPE_UINT32,
		.def.u = 25,
		.min.u = 1,
		.max.u = 100,
		.key = "Gyro sensivity Z"});
	setPE((ProfileEntry){
		.id = PR_GY_DEADZONE_X,
		.type = TYPE_UINT32,
		.def.u = 0,
		.min.u = 0,
		.max.u = 100,
		.key = "Gyro deadzone X"});
	setPE((ProfileEntry){
		.id = PR_GY_DEADZONE_Y,
		.type = TYPE_UINT32,
		.def.u = 0,
		.min.u = 0,
		.max.u = 100,
		.key = "Gyro deadzone Y"});
	setPE((ProfileEntry){
		.id = PR_GY_DEADZONE_Z,
		.type = TYPE_UINT32,
		.def.u = 0,
		.min.u = 0,
		.max.u = 100,
		.key = "Gyro deadzone Z"});
	setPE((ProfileEntry){
		.id = PR_GY_ANTIDEADZONE_X,
		.type = TYPE_UINT32,
		.def.u = 5,
		.min.u = 0,
		.max.u = 100,
		.key = "Gyro antideadzone X"});
	setPE((ProfileEntry){
		.id = PR_GY_ANTIDEADZONE_Y,
		.type = TYPE_UINT32,
		.def.u = 5,
		.min.u = 0,
		.max.u = 100,
		.key = "Gyro antideadzone Y"});
	setPE((ProfileEntry){
		.id = PR_GY_ANTIDEADZONE_Z,
		.type = TYPE_UINT32,
		.def.u = 5,
		.min.u = 0,
		.max.u = 100,
		.key = "Gyro antideadzone Z"});
	setPE((ProfileEntry){
		.id = PR_GY_CALIBRATION_X,
		.type = TYPE_INT32,
		.def.i = 0,
		.min.i = -1000,
		.max.i = 1000,
		.key = "Gyro calibration X"});
	setPE((ProfileEntry){
		.id = PR_GY_CALIBRATION_Y,
		.type = TYPE_INT32,
		.def.i = 0,
		.min.i = -1000,
		.max.i = 1000,
		.key = "Gyro calibration Y"});
	setPE((ProfileEntry){
		.id = PR_GY_CALIBRATION_Z,
		.type = TYPE_INT32,
		.def.i = 0,
		.min.i = -1000,
		.max.i = 1000,
		.key = "Gyro calibration Z"});
	setPE((ProfileEntry){
		.id = PR_GY_DS4_MOTION,
		.type = TYPE_BOOL,
		.def.u = 1,
		.key = "Gyro DS4"});
	setPE((ProfileEntry){
		.id = PR_GY_DEADBAND,
		.type = TYPE_UINT32,
		.def.u = 2,
		.min.u = 0,
		.max.u = 2,
		.key = "Gyro deadband"});

	// External controllers
	setPE((ProfileEntry){
		.id = PR_CO_SWAP_BUTTONS,
		.type = TYPE_BOOL,
		.def.u = false,
		.key = "Swap side buttons"});
	// setPE((ProfileEntry){
	// 	.id = PR_CO_FIX_BUTTONS,
	// 	.type = TYPE_BOOL,
	// 	.def.u = true,
	// 	.key = "FIX_BUTTONS"});
	setPE((ProfileEntry){
		.id = PR_CO_EMULATE_DS4,
		.type = TYPE_BOOL,
		.def.u = false,
		.key = "Virtual DS4"});

	// More
	setPE((ProfileEntry){
		.id = PR_MO_BLANK_FRAME,
		.type = TYPE_BOOL,
		.def.u = 0,
		.key = "Clear screen"});
	setPE((ProfileEntry){
		.id = PR_MO_DELAY_START,
		.type = TYPE_BOOL,
		.type = TYPE_UINT32,
		.def.u = 10,
		.min.u = 0,
		.max.u = 60,
		.key = "Startup delay"});

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