#include <vitasdkkern.h>
#include <taihen.h>
#include <stdbool.h>
#include <string.h>
#include "theme.h"
#include "fio.h"
#include "profile.h"
#include "ini.h"

#define SECTION "THEME"
#define PATH "ux0:/data/remaPSV2"
#define PATH_THEME "ux0:/data/remaPSV2/Theme"
#define EXT_INI "INI"
#define BUFFER_SIZE_THEME (1000 * sizeof(char)+ 0xfff) & ~0xfff

const char* THEME_COLOR_STR[THEME_COLOR__NUM] = {
	"COLOR_DEFAULT",
	"COLOR_HEADER",
	"COLOR_ACTIVE",
	"COLOR_DANGER",
	"COLOR_CURSOR_DEFAULT",
	"COLOR_CURSOR_HEADER",
	"COLOR_CURSOR_ACTIVE",
	"COLOR_CURSOR_DANGER",
	"COLOR_BG_HEADER",
	"COLOR_BG_BODY",
	"COLOR_TOUCH_FRONT",
	"COLOR_TOUCH_BOTOM",
	"COLOR_TOUCH_SHADOW"
};
enum THEME_COLOR_ID getThemeColorId(char* n){
	for (int i = 0; i < THEME_COLOR__NUM; i++)
		if (!strcmp(THEME_COLOR_STR[i], n)) 
			return i;
	return -1;
}

char* THEME_STR[THEME__NUM] = {
	"DARK",
	"LIGHT"
};
enum THEME_ID theme_findIdByKey(char* n){
	for (int i = 0; i < THEME__NUM; i++)
		if (!strcmp(THEME_STR[i], n)) 
			return i;
	return -1;
}

uint32_t theme[THEME_COLOR__NUM];

bool generateINITheme(char* buff){
	INI _ini = ini_create(buff, 99);
	INI* ini = &_ini;
	
	ini_addSection(ini, SECTION);
	for (int i = 0; i < THEME_COLOR__NUM; i++)
		ini_addBGR(ini, THEME_COLOR_STR[i], theme[i]);
	return true;
}
bool parseINITheme(char* buff){
	INI_READER _ini = ini_read(buff);
	INI_READER* ini = &_ini;

	while(ini_nextEntry(ini)){
		int id = getThemeColorId(ini->name);
		if (id < 0)
			continue;
		theme[id] = parseBGR(ini->val);
	}
	return true;
}
bool theme_load(int themeId){
	char* buff;
	bool ret = false;

    //Mem allocation for buffer
	SceUID buff_uid  = ksceKernelAllocMemBlock("RemaPSV2_INI_R", 
		SCE_KERNEL_MEMBLOCK_TYPE_KERNEL_RW, BUFFER_SIZE_THEME, NULL);
	if (buff_uid < 0) 
		return false;
    if (ksceKernelGetMemBlockBase(buff_uid, (void**)&buff) != 0)
		goto ERROR;
	//Read file to buffer
	if (!fio_readFile(buff, BUFFER_SIZE_THEME, PATH_THEME, THEME_STR[themeId], EXT_INI))
		goto ERROR;

	// Parse INI
	ret = parseINITheme(buff);
	
ERROR: //Free mem and quit
	ksceKernelFreeMemBlock(buff_uid);
	return ret;
}
bool theme_save(int themeId){
	bool ret = false;
	char* buff;
    //Mem allocation
	SceUID buff_uid  = ksceKernelAllocMemBlock("RemaPSV2_INI_W", 
		SCE_KERNEL_MEMBLOCK_TYPE_KERNEL_RW, BUFFER_SIZE_THEME, NULL);
	if (buff_uid < 0) 
		return false;
    if (ksceKernelGetMemBlockBase(buff_uid, (void**)&buff) != 0)
		goto ERROR;
	
	//Generate INI into buffer
	if (!generateINITheme(buff))
		goto ERROR;
	
	//Write to file
	ret = fio_writeFile(buff, strlen(buff), PATH_THEME, THEME_STR[themeId], EXT_INI);

ERROR: //Free allocated memory
	ksceKernelFreeMemBlock(buff_uid);
	return ret;
}
void setDefThemeDark(){
	theme[COLOR_DEFAULT] = 			0x00C2C0BD;
	theme[COLOR_HEADER] = 			0x00FF6600;
	theme[COLOR_ACTIVE] = 			0x0000B0B0;
	theme[COLOR_DANGER] = 			0x00000099;
	theme[COLOR_CURSOR_DEFAULT] = 	0x00FFFFFF;
	theme[COLOR_CURSOR_HEADER] = 	0x00FFAA22;
	theme[COLOR_CURSOR_ACTIVE] = 	0x0000DDDD;
	theme[COLOR_CURSOR_DANGER] = 	0x000000DD;
	theme[COLOR_BG_HEADER] = 		0x00000000;
	theme[COLOR_BG_BODY] = 			0x00171717;
	theme[COLOR_TOUCH_FRONT] = 		0x0000B0B0;
	theme[COLOR_TOUCH_BOTTOM] = 	0x00FF6600;
	theme[COLOR_TOUCH_SHADOW] = 	0x00000000;
}
void setDefThemeLight(){
	theme[COLOR_DEFAULT] = 			0x00000000;
	theme[COLOR_HEADER] = 			0x00FF6600;
	theme[COLOR_ACTIVE] = 			0x00008080;
	theme[COLOR_DANGER] = 			0x00000099;
	theme[COLOR_CURSOR_DEFAULT] = 	0x00000000;
	theme[COLOR_CURSOR_HEADER] = 	0x00FFAA22;
	theme[COLOR_CURSOR_ACTIVE] = 	0x0000A0A0;
	theme[COLOR_CURSOR_DANGER] = 	0x000000DD;
	theme[COLOR_BG_HEADER] = 		0x00DDDDDD;
	theme[COLOR_BG_BODY] = 			0x00FFFFFF;
	theme[COLOR_TOUCH_FRONT] = 		0x0000B0B0;
	theme[COLOR_TOUCH_BOTTOM] = 	0x00FF6600;
	theme[COLOR_TOUCH_SHADOW] = 	0x00000000;
}
void theme_init(){//Load theme
	setDefThemeDark();
	if (!theme_load(THEME_DARK)){
		setDefThemeDark();
	}
	setDefThemeLight();
	theme_save(THEME_DARK);
	if (!theme_load(THEME_LIGHT)){
		setDefThemeLight();
	}
	theme_save(THEME_LIGHT);
}
void theme_destroy(){
}