#include <vitasdkkern.h>
#include <taihen.h>
#include <stdbool.h>
#include "main.h"
#include "common.h"
#include "profile.h"
#include "ini.h"
#include "log.h"

#define PATH "ux0:/data/remaPSV2"
#define NAME_GLOBAL "GLOBAL"
#define NAME_HOME "HOME"
#define NAME_SETTINGS "SETTINGS"
#define EXT "bin"
#define EXT_INI "INI"
#define BUFFER_SIZE (1000 * sizeof(char)+ 0xfff) & ~0xfff
#define BUFFER_SIZE_SETTINGS (200 * sizeof(char)+ 0xfff) & ~0xfff

enum SECTION{
	SECTION_PROFILE = 0,
	SECTION_ANALOG,
	SECTION_TOUCH,
	SECTION_GYRO,
	SECTION_CONTROLLER,
	SECTION_RULE,
	SECTION_SETTINGS,
	SECTION__NUM
};
static const char* SECTION_STR[SECTION__NUM] = {
	"PROFILE",
	"ANALOG",
	"TOUCH",
	"GYRO",
	"CONTROLLER",
	"RULE",
	"SETTINGS"
};
enum SECTION getSectionId(char* n){
	for (int i = 0; i < SECTION__NUM; i++)
		if (!strcmp(SECTION_STR[i], n)) 
			return i;
	return -1;
};

const char* ANALOG_STR[PROFILE_ANALOG__NUM] = {
	"LEFT_DEADZONE_X",
	"LEFT_DEADZONE_Y",
	"RIGHT_DEADZONE_X",
	"RIGHT_DEADZONE_Y"
};
enum PROFILE_ANALOG_ID getAnalogId(char* n){
	for (int i = 0; i < PROFILE_ANALOG__NUM; i++)
		if (!strcmp(ANALOG_STR[i], n)) 
			return i;
	return -1;
}

const char* TOUCH_STR[PROFILE_TOUCH__NUM] = {
	"SWAP",
	"SWIPE_DURATION",
	"SWIPE_SENSIVITY"
};
enum PROFILE_TOUCH_ID getTouchId(char* n){
	for (int i = 0; i < PROFILE_TOUCH__NUM; i++)
		if (!strcmp(TOUCH_STR[i], n)) 
			return i;
	return -1;
}

const char* GYRO_STR[PROFILE_GYRO__NUM] = {
	"SENSIVITY_X",
	"SENSIVITY_Y",
	"SENSIVITY_Z",
	"DEADZONE_X",
	"DEADZONE_Y",
	"DEADZONE_Z",
	"DEADBAND",
	"WHEEL",
	"RESET_BTN1",
	"RESET_BTN2"
};
enum PROFILE_GYRO_ID getGyroId(char* n){
	for (int i = 0; i < PROFILE_GYRO__NUM; i++)
		if (!strcmp(GYRO_STR[i], n)) 
			return i;
	return -1;
}

const char* CONTROLLER_STR[PROFILE_CONTROLLER__NUM] = {
	"ENABLED",
	"PORT",
	"SWAP_BUTTONS"
};
enum PROFILE_CONTROLLER_ID getControllerId(char* n){
	for (int i = 0; i < PROFILE_CONTROLLER__NUM; i++)
		if (!strcmp(CONTROLLER_STR[i], n)) 
			return i;
	return -1;
}

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
    "GYRO_ROLL_RIGHT"
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

const char* SETTINGS_STR[PROFILE_SETTINGS__NUM] = {
	"PROFILE_SETTINGS_KEY1",
	"PROFILE_SETTINGS_KEY2",
	"PROFILE_SETTINGS_AUTOSAVE",
	"PROFILE_SETTINGS_DELAY"
};
enum PROFILE_SETTINGS_ID getSettingsId(char* n){
	for (int i = 0; i < PROFILE_SETTINGS__NUM; i++)
		if (!strcmp(SETTINGS_STR[i], n)) 
			return i;
	return -1;
}

Profile profile;
Profile profile_def;
Profile profile_global;
Profile profile_home;

int32_t profile_settings[PROFILE_SETTINGS__NUM];
int32_t profile_settings_def[PROFILE_SETTINGS__NUM];

void clone(Profile* pd, Profile* ps){
	pd->remapsNum = ps->remapsNum;
	pd->titleid[0] = '\0';
	strclone(pd->titleid, ps->titleid);
	memcpy(pd->analog, ps->analog, sizeof(ps->analog[0]) * PROFILE_ANALOG__NUM);
	memcpy(pd->touch, ps->touch, sizeof(ps->touch[0]) * PROFILE_TOUCH__NUM);
	memcpy(pd->gyro, ps->gyro, sizeof(ps->gyro[0]) * PROFILE_GYRO__NUM);
	memcpy(pd->controller, ps->controller, sizeof(ps->controller[0]) * PROFILE_CONTROLLER__NUM);
	memcpy(pd->remaps, ps->remaps, sizeof(struct RemapRule) * ps->remapsNum);
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
	for (int i = 0; i < PROFILE_ANALOG__NUM; i++)
		profile.analog[i] = profile_def.analog[i];
}
void profile_resetTouch(){
	for (int i = 0; i < PROFILE_TOUCH__NUM; i++)
		profile.touch[i] = profile_def.touch[i];
}
void profile_resetGyro() {
	for (int i = 0; i < PROFILE_GYRO__NUM; i++)
		profile.gyro[i] = profile_def.gyro[i];
}
void profile_resetController(){
	for (int i = 0; i < PROFILE_CONTROLLER__NUM; i++)
		profile.controller[i] = profile_def.controller[i];
}
void profile_resetSettings(){
	for (int i = 0; i < PROFILE_SETTINGS__NUM; i++)
		profile_settings[i] = profile_settings_def[i];
}

bool readFile(char* buff, int size, char* path, char* name, char* ext){
	char fname[128];
	sprintf(fname, "%s/%s.%s", path, name, ext);
	SceUID fd;
	fd = ksceIoOpen(fname, SCE_O_RDONLY, 0777);
	if (fd < 0)
		return false;
	ksceIoRead(fd, buff, size);
	if (ksceIoClose(fd) < 0)
		return false;
	
	return true;
}
bool writeFile(char* buff, int size, char* path, char* name, char* ext){
	//Create dir if not exists
	ksceIoMkdir(PATH, 0777); 

    char fname[128];
	sprintf(fname, "%s/%s.%s", PATH, name, EXT_INI);
	SceUID fd = ksceIoOpen(fname, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);
	if (fd < 0)
		return false;
	ksceIoWrite(fd, buff, size);
	if (ksceIoClose(fd) < 0)
		return false;

	return true;
}
bool deleteFile(char* path, char* name, char* ext){
	char fname[128];
	sprintf(fname, "%s/%s.%s", PATH, name, EXT_INI);
	if (ksceIoRemove(fname) >= 0)
		return true;
	return false;
}

bool generateINISettings(char* buff){
	INI _ini = ini_create(buff, 99);
	INI* ini = &_ini;

	ini_addSection(ini, SECTION_STR[SECTION_SETTINGS]);
	for (int i = 0; i < PROFILE_SETTINGS__NUM; i++)
		ini_addInt(ini, SETTINGS_STR[i], profile_settings[i]);
	return true;
}
bool generateINIProfile(Profile* p, char* buff){
	INI _ini = ini_create(buff, 99);
	INI* ini = &_ini;

	//Profile 
	ini_addSection(ini, SECTION_STR[SECTION_PROFILE]);
	ini_addStr(ini, "NAME", p->titleid);
	ini_addNL(ini);

	//Analog
	ini_addSection(ini, SECTION_STR[SECTION_ANALOG]);
	for (int i = 0; i < PROFILE_ANALOG__NUM; i++)
		ini_addInt(ini, ANALOG_STR[i], p->analog[i]);
	ini_addNL(ini);

	//Touch
	ini_addSection(ini, SECTION_STR[SECTION_TOUCH]);
	ini_addBool(ini, TOUCH_STR[PROFILE_TOUCH_SWAP], p->touch[PROFILE_TOUCH_SWAP]);
	ini_addInt(ini, TOUCH_STR[PROFILE_TOUCH_SWIPE_DURATION], p->touch[PROFILE_TOUCH_SWIPE_DURATION]);
	ini_addInt(ini, TOUCH_STR[PROFILE_TOUCH_SWIPE_SMART_SENSIVITY], p->touch[PROFILE_TOUCH_SWIPE_SMART_SENSIVITY]);
	ini_addNL(ini);

	//Gyro
	ini_addSection(ini, SECTION_STR[SECTION_GYRO]);
	for (int i = 0; i < PROFILE_GYRO__NUM; i++)
		ini_addInt(ini, GYRO_STR[i], p->gyro[i]);
	ini_addNL(ini);

	//Controller
	ini_addSection(ini, SECTION_STR[SECTION_CONTROLLER]);
	for (int i = 0; i < PROFILE_CONTROLLER__NUM; i++)
		ini_addInt(ini, CONTROLLER_STR[i], p->controller[i]);
	ini_addNL(ini);

	//Remaps
	for (int i = 0; i < p->remapsNum; i++){
		struct RemapRule* r =  &p->remaps[i];
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
			} else if (r->emu.action == REMAP_TOUCH_SWIPE_SMART_L || r->emu.action == REMAP_TOUCH_SWIPE_SMART_R || r->emu.action == REMAP_TOUCH_SWIPE_SMART_DPAD){
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
bool parseINISettings(char* buff){
	for (int i = 0; i < PROFILE_SETTINGS__NUM; i++)
		profile_settings[i] = profile_settings_def[i];

	INI_READER _ini = ini_read(buff);
	INI_READER* ini = &_ini;

	while(ini_nextEntry(ini)){
		if (getSectionId(ini->section) != SECTION_SETTINGS)
			continue;
		int id = getSettingsId(ini->name);
		if (id < 0)
			continue;
		profile_settings[id] = parseInt(ini->val);
	}
	return true;
}
bool parseINIProfile(Profile* p, char* buff){
	clone(p, &profile_def);
	INI_READER _ini = ini_read(buff);
	INI_READER* ini = &_ini;
	while(ini_nextEntry(ini)){
		int id = 0;
		int ruleId = 0;
		switch(getSectionId(ini->section)){
			case SECTION_PROFILE:
				//Nothing to read here
				break;
			case SECTION_ANALOG:
				id = getAnalogId(ini->name);
				if (id >= 0)
					p->analog[id] = parseInt(ini->val);
				break;
			case SECTION_TOUCH:
				switch(getTouchId(ini->name)){
					case PROFILE_TOUCH_SWAP: 
						p->touch[PROFILE_TOUCH_SWAP] = parseBool(ini->val); 
						break;
					case PROFILE_TOUCH_SWIPE_DURATION: 
						p->touch[PROFILE_TOUCH_SWIPE_DURATION] = parseInt(ini->val); 
						break;
					case PROFILE_TOUCH_SWIPE_SMART_SENSIVITY: 
						p->touch[PROFILE_TOUCH_SWIPE_SMART_SENSIVITY] = parseInt(ini->val); 
						break;
					default: break;
				}
				break;
			case SECTION_GYRO:
				id = getGyroId(ini->name);
				if (id >= 0)
					p->gyro[id] = parseInt(ini->val);
				break;
			case SECTION_CONTROLLER:
				id = getControllerId(ini->name);
				if (id >= 0)
					p->controller[id] = parseInt(ini->val);
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
						rr->trigger.param.tPoint.x = parseInt(ini_nextListVal(ini));
						rr->trigger.param.tPoint.y = parseInt(ini_nextListVal(ini));
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
	if (!readFile(buff, BUFFER_SIZE, PATH, name, EXT_INI))
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
	ret = writeFile(buff, strlen(buff), PATH, name, EXT_INI);

ERROR: //Free allocated memory
	ksceKernelFreeMemBlock(buff_uid);
	return ret;
}

bool profile_loadSettings(){
	char* buff;
	bool ret = false;
	profile_resetSettings();

    //Mem allocation for buffer
	SceUID buff_uid  = ksceKernelAllocMemBlock("RemaPSV2_INI_R", 
		SCE_KERNEL_MEMBLOCK_TYPE_KERNEL_RW, BUFFER_SIZE_SETTINGS, NULL);
	if (buff_uid < 0) 
		return false;
    if (ksceKernelGetMemBlockBase(buff_uid, (void**)&buff) != 0)
		goto ERROR;
	//Read file to buffer
	if (!readFile(buff, BUFFER_SIZE, PATH, NAME_SETTINGS, EXT_INI))
		goto ERROR;

	// Parse INI
	ret = parseINISettings(buff);
	
ERROR: //Free mem and quit
	ksceKernelFreeMemBlock(buff_uid);
	return ret;
}
bool profile_saveSettings(){
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
	if (!generateINISettings(buff))
		goto ERROR;
	
	//Write to file
	ret = writeFile(buff, strlen(buff), PATH, NAME_SETTINGS, EXT_INI);

ERROR: //Free allocated memory
	ksceKernelFreeMemBlock(buff_uid);
	return ret;
}

bool profile_save(char* titleId) {
	return writeProfile(&profile, titleId);
}
bool profile_load(char* titleId) {
	if (strcmp(profile.titleid, HOME) == 0){  //If used home profile previously
		clone(&profile_home, &profile);      //copy it back to its cache
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
	deleteFile(PATH, profile.titleid, EXT_INI);
}

void profile_saveAsGlobal(){
	clone(&profile_global, &profile);
	writeProfile(&profile_global, NAME_GLOBAL);
}
void profile_loadFromGlobal(){
	clone(&profile, &profile_global);
}
void profile_resetGlobal(){
	clone(&profile_global, &profile_def);
	writeProfile(&profile_global, NAME_GLOBAL);
}

void setDefProfile(){
	strclone(profile_def.titleid, HOME);

	profile_def.analog[PROFILE_ANALOG_LEFT_DEADZONE_X] = 30;
	profile_def.analog[PROFILE_ANALOG_LEFT_DEADZONE_Y] = 30;
	profile_def.analog[PROFILE_ANALOG_RIGHT_DEADZONE_X] = 30;
	profile_def.analog[PROFILE_ANALOG_RIGHT_DEADZONE_Y] = 30;

	profile_def.touch[PROFILE_TOUCH_SWAP] = 0;
	profile_def.touch[PROFILE_TOUCH_SWIPE_DURATION] = 50;
	profile_def.touch[PROFILE_TOUCH_SWIPE_SMART_SENSIVITY] = 10;

	profile_def.gyro[PROFILE_GYRO_SENSIVITY_X] = 127;
	profile_def.gyro[PROFILE_GYRO_SENSIVITY_Y] = 127;
	profile_def.gyro[PROFILE_GYRO_SENSIVITY_Z] = 127;
	profile_def.gyro[PROFILE_GYRO_DEADZONE_X] = 0;
	profile_def.gyro[PROFILE_GYRO_DEADZONE_Y] = 0;
	profile_def.gyro[PROFILE_GYRO_DEADZONE_Z] = 0;
	profile_def.gyro[PROFILE_GYRO_DEADBAND] = 0;
	profile_def.gyro[PROFILE_GYRO_WHEEL] = 0;
	profile_def.gyro[PROFILE_GYRO_RESET_BTN1] = 6;
	profile_def.gyro[PROFILE_GYRO_RESET_BTN2] = 7;

	profile_def.controller[PROFILE_CONTROLLER_ENABLED] = 0;
	profile_def.controller[PROFILE_CONTROLLER_PORT] = 0;
	profile_def.controller[PROFILE_CONTROLLER_SWAP_BUTTONS] = 0;

	profile_settings_def[PROFILE_SETTINGS_KEY1] = 4;
	profile_settings_def[PROFILE_SETTINGS_KEY2] = 3;
	profile_settings_def[PROFILE_SETTINGS_AUTOSAVE] = 1;
	profile_settings_def[PROFILE_SETTINGS_DELAY] = 10;
}
void profile_init(){
	//Set default profile
	setDefProfile();

	//Init global profile
	if (!readProfile(&profile_global, NAME_GLOBAL)){
		clone(&profile_global, &profile_def);
		writeProfile(&profile_global, NAME_GLOBAL);
	}

	//Init home profile
	if (!readProfile(&profile_home, NAME_HOME)){
		clone(&profile_home, &profile_global);
		writeProfile(&profile_home, NAME_HOME);
	}

	//Set home as active profile
	clone(&profile, &profile_home);

	//Load settings
	profile_loadSettings();
}
void profile_destroy(){
}