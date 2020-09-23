#include <vitasdkkern.h>
#include <stdarg.h>
#include <string.h>
#include <stdbool.h>
#include "ini.h"
#include "../log.h"

#define TRUE_STR "TRUE"
#define FALSE_STR "FALSE"
void ini_append(struct INI* ini, const char *fmt, ...) {
    va_list va;
    va_start (va, fmt);
    vsprintf(ini->idx, fmt, va);
    va_end (va);
	ini->idx = &ini->idx[strlen(ini->idx)];
}

void ini_addNL(struct INI* ini){
	ini_append(ini, "\n");
}

void ini_addSection(struct INI* ini, const char* name) {
	ini_append(ini, "\n[%s]", name);
}
void ini_addInt(struct INI* ini, const char* name, int val) {
	ini_append(ini, "\n%s=%i", name, val);
}
void ini_addStr(struct INI* ini, const char* name, const char* val) {
	ini_append(ini, "\n%s=%s", name, val);
}
void ini_addBool(struct INI* ini, const char* name, bool val) {
	ini_append(ini, "\n%s=%s", name, val ? TRUE_STR : FALSE_STR);
}
void ini_addList(struct INI* ini, const char* name) {
    ini_append(ini, "\n%s=", name);
}
void ini_addListInt(struct INI* ini, int val){
	ini_append(ini, "%i,", val);
}
void ini_addListStr(struct INI* ini, const char* val){
	ini_append(ini, "%s,", val);
}
void ini_addBGR(struct INI* ini, const char* name, uint val){
    ini_append(ini, "\n%s=#%02X%02X%02X", name, 
            val & 0xFF, (val >> 8) & 0xFF, (val >> 16) & 0xFF);
}

char* ini_nextLine(INI_READER* ini){
    if (ini->_.EOL == ini->_.EOS)           //End of buffer reached
        return NULL;                    //No more lines
    if (ini->_.EOL == NULL)               //if first call
        ini->_.line = ini->_.buff;          //Set line start at buff start
    else
        ini->_.line = &ini->_.EOL[1];       
    ini->_.EOL = strchr(ini->_.line, '\n'); 
    if (ini->_.EOL == NULL)
        ini->_.EOL = ini->_.EOS;
    ini->_.EOL[0] = '\0';
    return ini->_.line;
}
char* ini_nextEntry(INI_READER* ini){
    if (ini_nextLine(ini) == NULL)
        return NULL;
    if (!strlen(ini->_.line)) 
        return ini_nextEntry(ini);
    if (ini->_.line[0] == '[') {//Looks like section
        char section[SECTION_SIZE];
        char sectionAttr[SECTION_ATTR_SIZE];
        if (sscanf(ini->_.line, "[%127[^:]:%127[^]]", section, sectionAttr) == 2){ //Section with attr
            strcpy(ini->section, section);
            strcpy(ini->sectionAttr, sectionAttr);
            return ini_nextEntry(ini);
        } else if (sscanf (ini->_.line, "[%[^]]", section) == 1){ //Section without attr
            strcpy(ini->section, section);
            return ini_nextEntry(ini);
        }// Error parsing
    } else if (strchr(ini->_.line, '=')){ //Looks like Entry
        char key[ENTRY_NAME_SIZE];
        char value[ENTRY_VALUE_SIZE];
        if (sscanf (ini->_.line, "%[^=]=%s", key, value) == 2){
            strcpy(ini->name, key);
            strcpy(ini->val, value);
            strcpy(ini->listVal, "");
            ini->_.EOE = NULL;
            return ini->name;
        } //Error parsing
    }
    return ini_nextEntry(ini);
}
char* ini_nextListVal(INI_READER* ini){
    if (ini->_.EOE == ini->_.EOL)          //End of list reached
        return NULL;                       //No more list entries to return
    if (ini->_.EOE == NULL){               //if first func call
        // char* eq = strchr(ini->_.line, '=');
        // if (eq[1] == '\n')                  //if list value is empty
        //     return NULL;
        // ini->_.listEntry = &eq[1];
        ini->_.listEntry = &strchr(ini->_.line, '=')[1];
    } else
        ini->_.listEntry = &ini->_.EOE[1];  
    ini->_.EOE = strchr(ini->_.listEntry, ',');  
    if (ini->_.EOE == NULL)
        ini->_.EOE = ini->_.EOL;
    int len = (ini->_.EOE - ini->_.listEntry)/sizeof(char);
    strncpy(ini->listVal, ini->_.listEntry, len);
    ini->listVal[len] = '\0';
    return &ini->listVal[0];
}

struct INI ini_create(char* buff, int max){
    struct INI ini;
    ini.buff = buff;
	ini.buff[0] = '\0';
	ini.idx = ini.buff;
    return ini;
}
struct INI_READER ini_read(char* buff){
    static struct INI_READER ini;
    ini._.buff = buff;
    ini._.EOS = &buff[strlen(buff)];
    return ini;
}

int parseInt(char* c){
    if (c == NULL) 
        return 0;
    int result;
    sscanf(c, "%d", &result);
    return result;
}
bool parseBool(char* c){
    return !strcmp(c, TRUE_STR) ? true : false;
}
void addNumber(int* val, uint8_t number, uint8_t pos){
    uint8_t alignedPos = pos % 2 ? pos - 1 : pos + 1;
    *val += number << ((alignedPos) * 4);
}
int parseBGR(char* c){
    //sscnf cannot parse HEX unfortunatly - had to do it manually
    int ret = 0;
    for (int i = 0; i < strlen(c) - 1; i++){
        switch(c[i + 1]){
            case '1': addNumber(&ret, 0x1, i); break;
            case '2': addNumber(&ret, 0x2, i); break;
            case '3': addNumber(&ret, 0x3, i); break;
            case '4': addNumber(&ret, 0x4, i); break;
            case '5': addNumber(&ret, 0x5, i); break;
            case '6': addNumber(&ret, 0x6, i); break;
            case '7': addNumber(&ret, 0x7, i); break;
            case '8': addNumber(&ret, 0x8, i); break;
            case '9': addNumber(&ret, 0x9, i); break;
            case 'A': addNumber(&ret, 0xA, i); break;
            case 'B': addNumber(&ret, 0xB, i); break;
            case 'C': addNumber(&ret, 0xC, i); break;
            case 'D': addNumber(&ret, 0xD, i); break;
            case 'E': addNumber(&ret, 0xE, i); break;
            case 'F': addNumber(&ret, 0xF, i); break;
            default: break;
        }
    }
    return ret;
}