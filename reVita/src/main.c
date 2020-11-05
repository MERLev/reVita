#include <vitasdkkern.h>
#include <taihen.h>
#include <stdio.h>
#include <string.h>
#include <psp2/appmgr.h> 
#include <psp2/motion.h> 
#include <psp2/touch.h>
#include <psp2kern/kernel/sysmem.h> 
#include <psp2kern/registrymgr.h> 

#include "vitasdkext.h"
#include "main.h"
#include "motion-kernel.h"
#include "gui/gui.h"
#include "remap.h"
#include "fio/profile.h"
#include "fio/theme.h"
#include "fio/settings.h"
#include "fio/hotkeys.h"
#include "common.h"
#include "sysactions.h"
#include "log.h"
#include "userspace.h"
#include "ds34vita.h"
#include "revita.h"

#define INVALID_PID    -1

#define SPACE_KERNEL    1
#define SPACE_USER      0
#define LOGIC_POSITIVE  1
#define LOGIC_NEGATIVE  0
#define TRIGGERS_EXT    1
#define TRIGGERS_NONEXT 0

#define INTERNAL        (666*666)

#define HOOK_EXPORT(module, libnid, funcnid, name) \
    hooks[name##_id] = taiHookFunctionExportForKernel(\
            KERNEL_PID, &refs[name##_id], #module, libnid, funcnid, name##_patched);\
    if (hooks[name##_id] < 0)\
        LOG("ERROR: Hook export "#module">"#name" : %08X", hooks[name##_id]);

#define HOOK_IMPORT(module, libnid, funcnid, name) \
    hooks[name##_id] = taiHookFunctionImportForKernel(\
            KERNEL_PID, &refs[name##_id], #module, libnid, funcnid, name##_patched);\
    if (hooks[name##_id] < 0)\
        LOG("ERROR: Hook import "#module">"#name" : %08X", hooks[name##_id]);

#define HOOK_OFFSET(modid, offset, name) \
    hooks[name##_id] = taiHookFunctionOffsetForKernel(\
            KERNEL_PID, &refs[name##_id], (modid), 0, (offset) | 1, 1, name##_patched);\
    if (hooks[name##_id] < 0)\
        LOG("ERROR: Hook offset "#name" : %08X", hooks[name##_id]);

static tai_hook_ref_t refs[HOOKS_NUM];
static SceUID         hooks[HOOKS_NUM];

static SceUID mutex_procevent_uid = -1;
static SceUID mutexCtrlHook[5];
static SceUID mutexTouchHook[TOUCH_HOOKS_NUM][2];

static SceUID thread_uid = -1;
static bool   thread_run = true;

char titleid[32] = "";
int processid = -1;
static SceUID shellPid = -1;
static SceUID kernelPid = -1;
bool isPspemu = false;
bool isPSTV = false;
bool isPSTVTouchEmulation = false;

bool used_funcs[HOOKS_NUM];
bool ds34vitaRunning;

static uint64_t startTick;
static bool delayedStartDone = false;

bool isCallKernel(){
    return ksceKernelGetProcessId() == kernelPid;
}

bool isCallShell(){
    return ksceKernelGetProcessId() == shellPid;
}

bool isCallActive(){
    return ksceKernelGetProcessId() == processid;
}

int ksceCtrlPeekBufferPositive_internal(int port, SceCtrlData *pad_data, int count){
    pad_data->timeStamp = INTERNAL;
    int ret = ksceCtrlPeekBufferPositive(port, pad_data, count);
    return ret;
}

int ksceCtrlPeekBufferPositive2_internal(int port, SceCtrlData *pad_data, int count){
    pad_data->timeStamp = INTERNAL;
    int ret = ksceCtrlPeekBufferPositive2(port, pad_data, count);
    return ret;
}

int ksceTouchPeek_internal(SceUInt32 port, SceTouchData *pData, SceUInt32 nBufs){
    pData->status = INTERNAL;
    int ret = ksceTouchPeek(port, pData, nBufs);
    return ret;
}

void scheduleDelayedStart(){
    LOG("delayedStartDone =  0\n");
    delayedStartDone = false;
    startTick = ksceKernelGetSystemTimeWide();
}

void changeActiveApp(char* tId, int pid){
    if (!streq(titleid, tId)) {
        strnclone(titleid, tId, sizeof(titleid));
        processid = pid;
        for (int i = 0; i < HOOKS_NUM; i++)
            used_funcs[i] = false;
        profile_load(titleid);
        remap_resetBuffers();
        gui_close();
    }
}

static void updatePspemuTitle() {
    char id[12];
    if (ksceKernelMemcpyUserToKernelForPid(processid, &id[0], (uintptr_t)0x73CDE000 + 0x98, sizeof(id)) != 0)
        return;

    if (streq(id, ""))
        strclone(id, "XMB");
    if (isPspemu && !streq(titleid, id))
        changeActiveApp(id, processid);
}

int nullButtons_user(SceCtrlData *bufs, SceUInt32 nBufs, bool isPositiveLogic){
    SceCtrlData scd;
	memset(&scd, 0, sizeof(scd));
    scd.buttons = isPositiveLogic ? 0 : 0xFFFFFFFF;
    scd.lx = scd.ly = scd.rx = scd.ry = 127;
    for (int i = 0; i < nBufs; i++)
        ksceKernelMemcpyKernelToUser((uintptr_t)&bufs[i], &scd, sizeof(SceCtrlData));
    return nBufs;
}

int nullButtons_kernel(SceCtrlData *bufs, SceUInt32 nBufs, bool isPositiveLogic){
    for (int i = 0; i < nBufs; i++){
        bufs[i].buttons = isPositiveLogic ? 0 : 0xFFFFFFFF;
        bufs[i].lx = bufs[i].ly = bufs[i].rx = bufs[i].ry = 127;
    }
    return nBufs;
}

int onInput(int port, SceCtrlData *ctrl, int nBufs, int isKernelSpace, int isPositiveLogic, int isExt){ 
    int ret = nBufs;
    if (ret < 1 || ret > BUFFERS_NUM) 
        return ret;
    if (gui_isOpen) {
        if (isKernelSpace)
            return nullButtons_kernel(ctrl, nBufs, isPositiveLogic);
        else
            return nullButtons_user(ctrl, nBufs, isPositiveLogic);
    }
    if (!settings[SETT_REMAP_ENABLED].v.b) 
        return ret;

    if (!delayedStartDone){
        if (settings[POP_LOADING].v.b){
            char str[20];
            sprintf(str, "Loading... %isec", 
                settings[SETT_DELAY_INIT].v.u - (int)((ksceKernelGetSystemTimeWide() - startTick) / 1000000));
            gui_popupShow("reVita", str, 2*1000*1000);
        }

        if (startTick + settings[SETT_DELAY_INIT].v.u * 1000000 > ksceKernelGetSystemTimeWide())
            return ret;

        // Activate delayed start
        remap_setup();
        delayedStartDone = true;
        if (settings[POP_READY].v.b)
            gui_popupShow("reVita", "Ready", 2*1000*1000);
        LOG("delayedStartDone = 1\n");
    }

    ksceKernelLockMutex(mutexCtrlHook[port], 1, NULL);
    if (isKernelSpace) {
        // Update internal cache with latest buffers
        remap_ctrl_updateBuffers(port, &ctrl[ret-1], isPositiveLogic, isExt);

        // Replace returned buffers with those from internal cache
        ret = min(ret, remap_ctrl_getBufferNum(port));
        for (int i = 0; i < ret; i++){
            remap_ctrl_readBuffer(port, &ctrl[i], ret - i, isPositiveLogic, isExt);
        }
    } else {
        // Update internal cache with latest buffers
        SceCtrlData scd;
        ksceKernelMemcpyUserToKernel(&scd, (uintptr_t)&ctrl[ret-1], sizeof(SceCtrlData));
        remap_ctrl_updateBuffers(port, &scd, isPositiveLogic, isExt);

        // Replace returned buffers with those from internal cache
        ret = min(ret, remap_ctrl_getBufferNum(port));
        for (int i = 0; i < ret; i++){
            ksceKernelMemcpyUserToKernel(&scd, (uintptr_t)&ctrl[i], sizeof(SceCtrlData));
            remap_ctrl_readBuffer(port, &scd, ret - i, isPositiveLogic, isExt);
            ksceKernelMemcpyKernelToUser((uintptr_t)&ctrl[i], &scd, sizeof(SceCtrlData)); 
        }
    }
    ksceKernelUnlockMutex(mutexCtrlHook[port], 1);
    return ret;
}

#define DECL_FUNC_HOOK_PATCH_CTRL(name, space, logic, triggers) \
    static int name##_patched(int port, SceCtrlData *ctrl, int nBufs) { \
        if (((space) == SPACE_KERNEL && ctrl->timeStamp == INTERNAL) \
                || shellPid <= 0 \
                /*|| !isCallActive() \
                || (isPspemu && ksceKernelGetProcessId() != processid))*/ \
                || (!isCallShell() && !isCallActive()))\
            return TAI_CONTINUE(int, refs[name##_id], port, ctrl, nBufs); \
		int ret = TAI_CONTINUE(int, refs[name##_id], \
            (port == 1 && profile.entries[PR_CO_EMULATE_DS4].v.b) ? 0 : port, ctrl, nBufs); \
        used_funcs[name##_id] = true; \
        /*char titleidLocal[20]; \
        ksceKernelGetProcessTitleId(ksceKernelGetProcessId(), titleidLocal, 20); \
        if (ret > 0 && ret <= BUFFERS_NUM) \
            LOG("   "#name"_patched(%i,...,%i):%i |%i|%s| pid:%i\n", port, nBufs, ret, ksceKernelGetProcessId(), titleidLocal, processid);*/\
        return onInput(port, ctrl, ret, (space), (logic), (triggers));\
    }

DECL_FUNC_HOOK_PATCH_CTRL(ksceCtrlPeekBufferPositive,     SPACE_KERNEL,   LOGIC_POSITIVE, TRIGGERS_NONEXT)
DECL_FUNC_HOOK_PATCH_CTRL(ksceCtrlReadBufferPositive,     SPACE_KERNEL,   LOGIC_POSITIVE, TRIGGERS_NONEXT)
DECL_FUNC_HOOK_PATCH_CTRL(ksceCtrlPeekBufferNegative,     SPACE_KERNEL,   LOGIC_NEGATIVE, TRIGGERS_NONEXT)
DECL_FUNC_HOOK_PATCH_CTRL(ksceCtrlReadBufferNegative,     SPACE_KERNEL,   LOGIC_NEGATIVE, TRIGGERS_NONEXT)
DECL_FUNC_HOOK_PATCH_CTRL(ksceCtrlPeekBufferPositiveExt,  SPACE_KERNEL,   LOGIC_POSITIVE, TRIGGERS_NONEXT)
DECL_FUNC_HOOK_PATCH_CTRL(ksceCtrlReadBufferPositiveExt,  SPACE_KERNEL,   LOGIC_POSITIVE, TRIGGERS_NONEXT)

DECL_FUNC_HOOK_PATCH_CTRL(ksceCtrlPeekBufferPositive2,    SPACE_KERNEL,   LOGIC_POSITIVE, TRIGGERS_EXT)
DECL_FUNC_HOOK_PATCH_CTRL(ksceCtrlReadBufferPositive2,    SPACE_KERNEL,   LOGIC_POSITIVE, TRIGGERS_EXT)
DECL_FUNC_HOOK_PATCH_CTRL(ksceCtrlPeekBufferNegative2,    SPACE_KERNEL,   LOGIC_NEGATIVE, TRIGGERS_EXT)
DECL_FUNC_HOOK_PATCH_CTRL(ksceCtrlReadBufferNegative2,    SPACE_KERNEL,   LOGIC_NEGATIVE, TRIGGERS_EXT)
DECL_FUNC_HOOK_PATCH_CTRL(ksceCtrlPeekBufferPositiveExt2, SPACE_KERNEL,   LOGIC_POSITIVE, TRIGGERS_EXT)
DECL_FUNC_HOOK_PATCH_CTRL(ksceCtrlReadBufferPositiveExt2, SPACE_KERNEL,   LOGIC_POSITIVE, TRIGGERS_EXT)

int nullTouch_kernel(SceTouchData *pData, SceUInt32 nBufs){
    pData[0] = pData[nBufs - 1];
    pData[0].reportNum = 0;
    return 1;
}

int nullTouch_user(SceTouchData *pData, SceUInt32 nBufs){
    SceUInt32 reportsNum = 0;
    ksceKernelMemcpyKernelToUser((uintptr_t)&pData[0].reportNum, &reportsNum, sizeof(SceUInt32));
    return 1;
}

int onTouch(SceUInt32 port, SceTouchData *pData, SceUInt32 nBufs, uint8_t hookIdx, int isKernelSpace){
    int ret = nBufs;
    
    if (nBufs < 1 || nBufs > BUFFERS_NUM) 
        return nBufs;

    //Nullify input calls when UI is open
	if (gui_isOpen) {
        if (isKernelSpace)
            return nullTouch_kernel(pData, nBufs);
        else 
            return nullTouch_user(pData, nBufs);
	}
    
	if (!settings[SETT_REMAP_ENABLED].v.b) return nBufs;

    ksceKernelLockMutex(mutexTouchHook[hookIdx][port], 1, NULL);
    SceTouchData* remappedBuffers; 
    if (isKernelSpace){
        ret = remap_touch(port, &pData[nBufs - 1], nBufs, hookIdx, &remappedBuffers);
        memcpy(&pData[0], remappedBuffers, ret * sizeof(SceTouchData));
    } else {
        SceTouchData std;                // Last buffer
        ksceKernelMemcpyUserToKernel(&std, (uintptr_t)&pData[ret-1], sizeof(SceTouchData));
        ret = remap_touch(port, &std, nBufs, hookIdx, &remappedBuffers);
        ksceKernelMemcpyKernelToUser((uintptr_t)&pData[0], remappedBuffers, ret * sizeof(SceTouchData)); 
    }
    ksceKernelUnlockMutex(mutexTouchHook[hookIdx][port], 1);
    return ret;
}

void scaleTouchData(int port, SceTouchData *pData){
    for (int idx = 0; idx < pData->reportNum; idx++)
        pData->report[idx].y = 
            (pData->report[idx].y - T_SIZE[!port].a.y) 
            * (T_SIZE[port].b.y - T_SIZE[port].a.y) 
            / (T_SIZE[!port].b.y - T_SIZE[!port].a.y) 
            + T_SIZE[port].a.y;
}

#define DECL_FUNC_HOOK_PATCH_TOUCH(name, space) \
    static int name##_patched(SceUInt32 port, SceTouchData *pData, SceUInt32 nBufs) { \
        if (pData->status == INTERNAL || shellPid <= 0 || !delayedStartDone) \
            return TAI_CONTINUE(int, refs[name##_id], port, pData, nBufs); \
        if (profile.entries[PR_TO_SWAP].v.b) \
            port = !port; \
		int ret = TAI_CONTINUE(int, refs[name##_id], port, pData, nBufs); \
        if (profile.entries[PR_TO_SWAP].v.b && ret > 0 && ret < 64) \
            scaleTouchData(!port, &pData[ret - 1]); \
        used_funcs[name##_id] = true; \
        return onTouch(port, pData, ret, name##_id - ksceTouchPeek_id, (space)); \
    }
/*#define DECL_FUNC_HOOK_PATCH_TOUCH_REGION(index, name, space) \
    static int name##_patched(SceUInt32 port, SceTouchData *pData, SceUInt32 nBufs, int region) { \
        if (isInternalTouchCall) \
            return TAI_CONTINUE(int, refs[(index)], port, pData, nBufs, region); \
        if (profile.entries[PR_TO_SWAP].v.b) \
            port = !port; \
		int ret = TAI_CONTINUE(int, refs[(index)], port, pData, nBufs, region); \
        if (region != 1) return ret; \
        used_funcs[(index)] = true; \
        return onTouch(port, pData, ret, (index) - ksceTouchPeek_id, (space)); \
    }*/
DECL_FUNC_HOOK_PATCH_TOUCH(ksceTouchPeek, SPACE_KERNEL)
DECL_FUNC_HOOK_PATCH_TOUCH(ksceTouchRead, SPACE_KERNEL)
// DECL_FUNC_HOOK_PATCH_TOUCH_REGION(ksceTouchPeekRegion, SPACE_KERNEL)
// DECL_FUNC_HOOK_PATCH_TOUCH_REGION(ksceTouchReadRegion, SPACE_KERNEL)

int ksceCtrlGetControllerPortInfo_patched(SceCtrlPortInfo* info){
    int ret = TAI_CONTINUE(int, refs[ksceCtrlGetControllerPortInfo_id], info);
    if (profile.entries[PR_CO_EMULATE_DS4].v.b){
        info->port[1] = SCE_CTRL_TYPE_DS4;
    }
	return ret;
}

int ksceDisplaySetFrameBufInternal_patched(int head, int index, const SceDisplayFrameBuf *pParam, int sync) {
    used_funcs[ksceDisplaySetFrameBufInternal_id] = 1;

    if (index && ksceAppMgrIsExclusiveProcessRunning())
        goto DISPLAY_HOOK_RET; // Do not draw over SceShell overlay

    if (pParam)  
        gui_draw(pParam);

    if (!head || !pParam)
        goto DISPLAY_HOOK_RET;

    if (gui_isOpen) {
        SceCtrlData ctrl;
        
        if(ksceCtrlPeekBufferPositive2_internal(0, &ctrl, 1) == 1)
            gui_onInput(&ctrl);
    }

DISPLAY_HOOK_RET:
    return TAI_CONTINUE(int, refs[ksceDisplaySetFrameBufInternal_id], head, index, pParam, sync);
}

bool isAppAllowed(char* titleId){
    // if (STREQANY(titleId,  // If test app
    //         "TSTCTRL00", "TSTCTRL20", "TSTCTRLE0", "TSTCTRLE2", "TSTCTRLN0", "TSTCTRLN2", "VSDK00019"))
    //     return false;                  // Use MAIN profile
    if (strStartsWith(titleId, "NPXS") && !STREQANY(titleId, 
            "NPXS10012",     // PS3Link
            "NPXS10013"))    // PS4Link)
        return false;               // Use MAIN profile
    return true;
}

int ksceKernelInvokeProcEventHandler_patched(int pid, int ev, int a3, int a4, int *a5, int a6) {
    used_funcs[ksceKernelInvokeProcEventHandler_id] = 1;
    char titleidLocal[sizeof(titleid)];
    ksceKernelGetProcessTitleId(pid, titleidLocal, sizeof(titleidLocal));
    if (!isAppAllowed(titleidLocal))
        goto PROCEVENT_EXIT;
    int ret = ksceKernelLockMutex(mutex_procevent_uid, 1, NULL);
    if (ret < 0)
        goto PROCEVENT_EXIT;
    if (streq(titleidLocal, "main"))
        strnclone(titleidLocal, HOME, sizeof(titleidLocal));
    switch (ev) {
        case 1: //Start
            scheduleDelayedStart();
        case 5: //Resume
            if (streq(titleidLocal, "PSPEMUCFW")){
                isPspemu = true;
                processid = pid;
                break;
            }
            changeActiveApp(titleidLocal, pid);
            break;
        case 3: //Close
        case 4: //Suspend
            if (!streq(titleid, HOME)){
                isPspemu = false;
                changeActiveApp(HOME, pid);
                processid = shellPid;
            }
            break;
        default:
            break;
    }

    ksceKernelUnlockMutex(mutex_procevent_uid, 1);

PROCEVENT_EXIT:
    return TAI_CONTINUE(int, refs[ksceKernelInvokeProcEventHandler_id], pid, ev, a3, a4, a5, a6);
}

// Always return SceShell pid to read all buttons
SceUID ksceKernelGetProcessId_patched(void){
    int ret = TAI_CONTINUE(SceUID, refs[ksceKernelGetProcessId_id]);
    if (shellPid > 0 
            && delayedStartDone 
            && settings[SETT_REMAP_ENABLED].v.b//) 
            && !isPspemu)
        return shellPid;
    return ret;
}

int ksceRegMgrSetKeyInt_patched(char *category, char *name, int buf){
    int ret = TAI_CONTINUE(int, refs[ksceRegMgrSetKeyInt_id], category, name, buf);
	if (streq(category, "/CONFIG/SHELL") && streq(name, "touch_emulation"))
		isPSTVTouchEmulation = buf;
	if (streq(category, "/CONFIG/DISPLAY") && streq(name, "brightness"))
		brightnessLevel = buf;
	return ret;
}

static int main_thread(SceSize args, void *argp) {
    uint32_t oldBtns = 0;
    while (thread_run) {
        // Set shell pid
        if (shellPid < 0){
            processid = shellPid = ksceKernelSysrootGetShellPid();
            ksceCtrlSetSamplingModeExt(SCE_CTRL_MODE_ANALOG_WIDE);
            remap_resetBuffers();
        }

        // Update PSM title
        if (isPspemu) 
            updatePspemuTitle();

        SceCtrlData ctrl;
        if(ksceCtrlPeekBufferPositive2_internal(0, &ctrl, 1) != 1){
            ksceKernelDelayThread(30 * 1000);
            continue;
        }

        if (!gui_isOpen){
            for (int i = 0; i < HOTKEY__NUM; i++){
                if (hotkeys[i].v.u != 0 && 
                        btn_has(ctrl.buttons, hotkeys[i].v.u) && 
                        !btn_has(oldBtns, hotkeys[i].v.u)){
                    switch(i){
                        case HOTKEY_MENU:
                            if (!delayedStartDone)
                                break;
                            gui_open();
                            remap_resetBuffers();
                            break;
                        case HOTKEY_REMAPS_TOOGLE: 
                            FLIP(settings[SETT_REMAP_ENABLED].v.b); 
	                        if (settings[POP_REVITA].v.b)
                                gui_popupShow("reVita", settings[SETT_REMAP_ENABLED].v.b ? "On" : "Off", 2*1000*1000);
                            break;
                        case HOTKEY_RESET_SOFT: sysactions_softReset();  break;
                        case HOTKEY_RESET_COLD: sysactions_coldReset();  break;
                        case HOTKEY_STANDBY: sysactions_standby();  break;
                        case HOTKEY_SUSPEND: sysactions_suspend();  break;
                        case HOTKEY_DISPLAY_OFF: sysactions_displayOff();  break;
                        case HOTKEY_KILL_APP: sysactions_killCurrentApp();  break;
                        case HOTKEY_BRIGHTNESS_INC: sysactions_brightnessInc();  break;
                        case HOTKEY_BRIGHTNESS_DEC: sysactions_brightnessDec();  break;
                        case HOTKEY_SAVE_BACKUP: sysactions_saveBackup();  break;
                        case HOTKEY_SAVE_RESTORE: sysactions_saveRestore();  break;
                        case HOTKEY_SAVE_DELETE: sysactions_saveDelete();  break;
                        case HOTKEY_MOTION_CALIBRATE: sysactions_calibrateMotion();  break;
                    }
                }
            }
        }

        oldBtns = ctrl.buttons;

        ksceKernelDelayThread(30 * 1000);
    }
    return 0;
}

// Check if ds34vita running and set appropriate functions
void initDs34vita(){
    if (!ds34vitaRunning){
        LOG("initDs34vita();\n");
        tai_module_info_t info;
        info.size = sizeof(tai_module_info_t);
        ds34vitaRunning = taiGetModuleInfoForKernel(KERNEL_PID, "ds34vita", &info) == 0;
    }
}

//Sync ds4vita config
void syncDS34Vita(){
    LOG("syncDS34Vita()\n");
    initDs34vita();
    if (!ds34vitaRunning)
        return;
    ds34vita_setIsPort1Allowed(!profile.entries[PR_CO_EMULATE_DS4].v.b);
    LOG("ds34vita_setIsPort1Allowed(%i)", !profile.entries[PR_CO_EMULATE_DS4].v.b);
}

//Sync configurations across other plugins
void sync(){
    syncDS34Vita();
}

void _start() __attribute__ ((weak, alias ("module_start")));
int module_start(SceSize argc, const void *args) {
    LOG("Plugin started\n");

    // Create mutexes for ctrl and touch hooks
    mutex_procevent_uid = ksceKernelCreateMutex("reVita_mutex_procevent", 0, 0, NULL);

    kernelPid = ksceKernelGetProcessId();

    char fname[128];
    for (int j = 0; j < 5; j++){
        sprintf(fname, "reVita_mutex_ctrl_%i", j);
        mutexCtrlHook[j] = ksceKernelCreateMutex(&fname[0], 0, 0, NULL);
    }
    for (int i = 0; i < TOUCH_HOOKS_NUM; i++){
        for (int j = 0; j < 2; j++){
	        sprintf(fname, "reVita_mutex_touch_%i_%i", i, j);
            mutexTouchHook[i][j] = ksceKernelCreateMutex(&fname[0], 0, 0, NULL);
        }
    }

    // Import all needed functions
    vitasdkext_init();

	// PS TV uses SceTouchDummy instead of SceTouch
    STRUCTS(tai_module_info_t, modInfo);
	isPSTV = taiGetModuleInfoForKernel(KERNEL_PID, "SceTouch", &modInfo) < 0;

    // Set home profile titleId
    snprintf(titleid, sizeof(titleid), HOME);

    // Init all components
    // motion_init();
    settings_init();
    hotkeys_init();
    theme_init();
    profile_init();
    gui_init();
    remap_init();
    userspace_init();
    sysactions_init();

	ksceRegMgrGetKeyInt("/CONFIG/SHELL", "touch_emulation", (int *)&isPSTVTouchEmulation);
    startTick = ksceKernelGetSystemTimeWide();
	theme_load(settings[SETT_THEME].v.u);

    // Hooking functions
    memset(hooks, 0, sizeof(hooks));

	if (taiGetModuleInfoForKernel(KERNEL_PID, "SceCtrl", &modInfo) < 0) {
		LOG("Error finding SceBt module\n");
		return SCE_KERNEL_START_FAILED;
	}

    HOOK_EXPORT(SceCtrl, TAI_ANY_LIBRARY, 0xEA1D3A34, ksceCtrlPeekBufferPositive);
    HOOK_EXPORT(SceCtrl, TAI_ANY_LIBRARY, 0x9B96A1AA, ksceCtrlReadBufferPositive);
    HOOK_EXPORT(SceCtrl, TAI_ANY_LIBRARY, 0x19895843, ksceCtrlPeekBufferNegative);
    HOOK_EXPORT(SceCtrl, TAI_ANY_LIBRARY, 0x8D4E0DD1, ksceCtrlReadBufferNegative);
	HOOK_OFFSET(modInfo.modid, 0x3928, ksceCtrlPeekBufferPositiveExt);
    HOOK_OFFSET(modInfo.modid, 0x3BCC, ksceCtrlReadBufferPositiveExt);

    HOOK_OFFSET(modInfo.modid, 0x3EF8, ksceCtrlPeekBufferPositive2);
    HOOK_OFFSET(modInfo.modid, 0x449C, ksceCtrlReadBufferPositive2);
    HOOK_OFFSET(modInfo.modid, 0x41C8, ksceCtrlPeekBufferNegative2);
    HOOK_OFFSET(modInfo.modid, 0x47F0, ksceCtrlReadBufferNegative2);
    HOOK_OFFSET(modInfo.modid, 0x4B48, ksceCtrlPeekBufferPositiveExt2);
    HOOK_OFFSET(modInfo.modid, 0x4E14, ksceCtrlReadBufferPositiveExt2);

    if (!isPSTV) {
        HOOK_EXPORT(SceTouch, TAI_ANY_LIBRARY, 0xBAD1960B, ksceTouchPeek);
        HOOK_EXPORT(SceTouch, TAI_ANY_LIBRARY, 0x70C8AACE, ksceTouchRead);
        // HOOK_EXPORT(SceTouch, TAI_ANY_LIBRARY, 0x9B3F7207, ksceTouchPeekRegion);
        // HOOK_EXPORT(SceTouch, TAI_ANY_LIBRARY, 0x9A91F624, ksceTouchReadRegion);
    } else {
        HOOK_EXPORT(SceTouchDummy, TAI_ANY_LIBRARY, 0xBAD1960B, ksceTouchPeek);
        HOOK_EXPORT(SceTouchDummy, TAI_ANY_LIBRARY, 0x70C8AACE, ksceTouchRead);
        // HOOK_EXPORT(SceTouchDummy, TAI_ANY_LIBRARY, 0x9B3F7207, ksceTouchPeekRegion);
        // HOOK_EXPORT(SceTouchDummy, TAI_ANY_LIBRARY, 0x9A91F624, ksceTouchReadRegion);
    }

	HOOK_EXPORT(SceCtrl,       TAI_ANY_LIBRARY, 0xF11D0D30, ksceCtrlGetControllerPortInfo);
	HOOK_IMPORT(SceCtrl,       0xE2C40624,      0x9DCB4B7A, ksceKernelGetProcessId);
	HOOK_EXPORT(SceDisplay,    0x9FED47AC,      0x16466675, ksceDisplaySetFrameBufInternal);
	HOOK_IMPORT(SceProcessmgr, 0x887F19D0,      0x414CC813, ksceKernelInvokeProcEventHandler);
	HOOK_EXPORT(SceRegistryMgr, 0xB2223AEB, 0xD72EA399, ksceRegMgrSetKeyInt);

    // Create threads
    thread_uid = ksceKernelCreateThread("reVita_thread", main_thread, 0x3C, 0x3000, 0, 0x10000, 0);
    ksceKernelStartThread(thread_uid, 0, NULL);
    
    // remap_setup();
    // scheduleDelayedStart();

    return SCE_KERNEL_START_SUCCESS;
}

int module_stop(SceSize argc, const void *args) {
    if (thread_uid >= 0) {
        thread_run = 0;
        ksceKernelWaitThreadEnd(thread_uid, NULL, NULL);
        ksceKernelDeleteThread(thread_uid);
    }

    for (int i = 0; i < HOOKS_NUM; i++) {
        if (hooks[i] >= 0)
            taiHookReleaseForKernel(hooks[i], refs[i]);
    }

    if (mutex_procevent_uid >= 0)
        ksceKernelDeleteMutex(mutex_procevent_uid);
    for (int j = 0; j < 5; j++)
        if (mutexCtrlHook[j] >= 0)
            ksceKernelDeleteMutex(mutexCtrlHook[j]);
    for (int i = 0; i < TOUCH_HOOKS_NUM; i++)
        for (int j = 0; j < 2; j++)
            if (mutexTouchHook[i][j] >= 0)
                ksceKernelDeleteMutex(mutexTouchHook[i][j]);

    vitasdkext_destroy();
    // motion_destroy();
    settings_destroy();
    hotkeys_destroy();
    theme_destroy();
    profile_destroy();
    gui_destroy();
    remap_destroy();
    userspace_destroy();
    return SCE_KERNEL_STOP_SUCCESS;
}
