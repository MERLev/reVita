#include <vitasdkkern.h>
#include <taihen.h>
#include <stdio.h>
#include <string.h>
#include <psp2/motion.h> 
#include <psp2/touch.h>
#include <psp2kern/kernel/sysmem.h> 

#include "vitasdkext.h"
#include "main.h"
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
#include "ds4vita.h"
#include "remapsv.h"

#define INVALID_PID    -1

#define SPACE_KERNEL    1
#define SPACE_USER      0
#define LOGIC_POSITIVE  1
#define LOGIC_NEGATIVE  0
#define TRIGGERS_EXT    1
#define TRIGGERS_NONEXT 0

static tai_hook_ref_t refs[HOOKS_NUM];
static SceUID         hooks[HOOKS_NUM];
static SceUID mutex_procevent_uid = -1;
static SceUID thread_uid = -1;
static bool   thread_run = true;

SceUID bufsMemId;
uint8_t* bufsMemBase;
SceTouchData* bufsStd[TOUCH_HOOKS_NUM];

char titleid[32] = "";
int processid = -1;

bool used_funcs[HOOKS_NUM];
bool isInternalTouchCall = false;
bool isInternalCtrlCall = false;

static uint64_t startTick;
static bool delayedStartDone = false;

SceUID (*_ksceKernelGetProcessMainModule)(SceUID pid);
int (*_ksceKernelGetModuleInfo)(SceUID pid, SceUID modid, SceKernelModuleInfo *info);

int nullButtonsUSpace(SceCtrlData *bufs, SceUInt32 nBufs, bool isPositiveLogic){
    SceCtrlData scd;
	memset(&scd, 0, sizeof(scd));
    scd.buttons = isPositiveLogic ? 0 : 0xFFFFFFFF;
    scd.lx = scd.ly = scd.rx = scd.ry = 127;
    for (int i = 0; i < nBufs; i++)
        ksceKernelMemcpyKernelToUser((uintptr_t)&bufs[i], &scd, sizeof(SceCtrlData));
    return nBufs;
}

int onInput(int port, SceCtrlData *ctrl, int nBufs, int hookId, int isKernelSpace, int isPositiveLogic, int isExt){ 
    int ret = nBufs;

    if (ret < 1 || ret > BUFFERS_NUM) 
        return ret;
    if (gui_isOpen) 
        return nullButtonsUSpace(ctrl, nBufs, isPositiveLogic);
    if (!settings[SETT_REMAP_ENABLED].v.b) 
        return ret;
    if (!isExt && profile.entries[PR_CO_ENABLED].v.b) 
        return ret;

    used_funcs[hookId] = true;

    SceCtrlData scd;
    ksceKernelMemcpyUserToKernel(&scd, (uintptr_t)&ctrl[ret-1], sizeof(SceCtrlData));
    SceCtrlData* remappedBuffers;
    ret = remap_controls(port, &scd, ret, hookId, &remappedBuffers, isPositiveLogic, isExt);
    ksceKernelMemcpyKernelToUser((uintptr_t)&ctrl[0], remappedBuffers, ret * sizeof(SceCtrlData)); 

    return ret;
}

#define DECL_FUNC_HOOK_PATCH_CTRL(id, name, space, logic, triggers) \
    static int name##_patched(int port, SceCtrlData *ctrl, int nBufs) { \
		int ret = TAI_CONTINUE(int, refs[(id)], \
            (port == 1 && profile.entries[PR_CO_EMULATE_DS4].v.b) ? 0 : port, ctrl, nBufs); \
        return onInput(port, ctrl, ret, (id), (space), (logic), (triggers)); \
    }

DECL_FUNC_HOOK_PATCH_CTRL(H_CT_PEEK_P,      sceCtrlPeekBufferPositive,     SPACE_USER,   LOGIC_POSITIVE, TRIGGERS_NONEXT)
DECL_FUNC_HOOK_PATCH_CTRL(H_CT_READ_P,      sceCtrlReadBufferPositive,     SPACE_USER,   LOGIC_POSITIVE, TRIGGERS_NONEXT)
DECL_FUNC_HOOK_PATCH_CTRL(H_CT_PEEK_N,      sceCtrlPeekBufferNegative,     SPACE_USER,   LOGIC_NEGATIVE, TRIGGERS_NONEXT)
DECL_FUNC_HOOK_PATCH_CTRL(H_CT_READ_N,      sceCtrlReadBufferNegative,     SPACE_USER,   LOGIC_NEGATIVE, TRIGGERS_NONEXT)
DECL_FUNC_HOOK_PATCH_CTRL(H_CT_PEEK_P_EXT,  sceCtrlPeekBufferPositiveExt,  SPACE_USER,   LOGIC_POSITIVE, TRIGGERS_NONEXT)
DECL_FUNC_HOOK_PATCH_CTRL(H_CT_READ_P_EXT,  sceCtrlReadBufferPositiveExt,  SPACE_USER,   LOGIC_POSITIVE, TRIGGERS_NONEXT)

DECL_FUNC_HOOK_PATCH_CTRL(H_CT_PEEK_P_2,    sceCtrlPeekBufferPositive2,    SPACE_USER,   LOGIC_POSITIVE, TRIGGERS_EXT)
DECL_FUNC_HOOK_PATCH_CTRL(H_CT_READ_P_2,    sceCtrlReadBufferPositive2,    SPACE_USER,   LOGIC_POSITIVE, TRIGGERS_EXT)
DECL_FUNC_HOOK_PATCH_CTRL(H_CT_PEEK_N_2,    sceCtrlPeekBufferNegative2,    SPACE_USER,   LOGIC_NEGATIVE, TRIGGERS_EXT)
DECL_FUNC_HOOK_PATCH_CTRL(H_CT_READ_N_2,    sceCtrlReadBufferNegative2,    SPACE_USER,   LOGIC_NEGATIVE, TRIGGERS_EXT)
DECL_FUNC_HOOK_PATCH_CTRL(H_CT_PEEK_P_EXT2, sceCtrlPeekBufferPositiveExt2, SPACE_USER,   LOGIC_POSITIVE, TRIGGERS_EXT)
DECL_FUNC_HOOK_PATCH_CTRL(H_CT_READ_P_EXT2, sceCtrlReadBufferPositiveExt2, SPACE_USER,   LOGIC_POSITIVE, TRIGGERS_EXT)

// DECL_FUNC_HOOK_PATCH_CTRL(H_K_CT_PEEK_P,    ksceCtrlPeekBufferPositive,    SPACE_KERNEL, LOGIC_POSITIVE, TRIGGERS_NONEXT)
// DECL_FUNC_HOOK_PATCH_CTRL(H_K_CT_READ_P,    ksceCtrlReadBufferPositive,    SPACE_KERNEL, LOGIC_POSITIVE, TRIGGERS_NONEXT)
// DECL_FUNC_HOOK_PATCH_CTRL(H_K_CT_PEEK_N,    ksceCtrlPeekBufferNegative,    SPACE_KERNEL, LOGIC_NEGATIVE, TRIGGERS_NONEXT)
// DECL_FUNC_HOOK_PATCH_CTRL(H_K_CT_READ_N,    ksceCtrlReadBufferNegative,    SPACE_KERNEL, LOGIC_NEGATIVE, TRIGGERS_NONEXT)

int onTouch(SceUInt32 port, SceTouchData *pData, SceUInt32 nBufs, uint8_t hookId){
    if (profile.entries[PR_TO_PSTV_MODE].v.b) 
        return nBufs;
    if (nBufs < 1 || nBufs > BUFFERS_NUM) 
        return nBufs;

	//Disable in menu
    if (isInternalTouchCall) return nBufs;

    //Nullify input calls when UI is open
	if (gui_isOpen) {
		pData[0] = pData[nBufs - 1];
		pData[0].reportNum = 0;
		return 1;
	}
    
	if (!settings[SETT_REMAP_ENABLED].v.b) return nBufs;

    return remap_touch(port, pData, nBufs, hookId - H_K_TO_PEEK);
}

/*export*/ int remaPSV2k_onTouch(SceUInt32 port, SceTouchData *pData, SceUInt32 nBufs, uint8_t hookId){
    if (nBufs < 1 || nBufs > BUFFERS_NUM) return nBufs;
	if (!profile.entries[PR_TO_PSTV_MODE].v.b) return nBufs;	
    ksceKernelMemcpyUserToKernel(bufsStd[hookId], (uintptr_t)&pData[0], nBufs * sizeof(SceTouchData)); 
    int ret = onTouch(port, bufsStd[hookId], nBufs, hookId); 
    ksceKernelMemcpyKernelToUser((uintptr_t)&pData[0], bufsStd[hookId], ret * sizeof(SceTouchData)); 
    return ret;
}

#define DECL_FUNC_HOOK_PATCH_TOUCH(index, name) \
    static int name##_patched(SceUInt32 port, SceTouchData *pData, SceUInt32 nBufs) { \
        if (profile.entries[PR_TO_SWAP].v.b) port = !port; \
		int ret = TAI_CONTINUE(int, refs[(index)], port, pData, nBufs);\
        return onTouch(port, pData, ret, (index)); \
    }
DECL_FUNC_HOOK_PATCH_TOUCH(H_K_TO_PEEK, ksceTouchPeek)
DECL_FUNC_HOOK_PATCH_TOUCH(H_K_TO_READ, ksceTouchRead)

#define DECL_FUNC_HOOK_PATCH_TOUCH_REGION(index, name) \
    static int name##_patched(SceUInt32 port, SceTouchData *pData, SceUInt32 nBufs, int region) { \
        if (profile.entries[PR_TO_SWAP].v.b) port = !port; \
		int ret = TAI_CONTINUE(int, refs[(index)], port, pData, nBufs, region); \
        return onTouch(port, pData, ret, (index)); \
    }
DECL_FUNC_HOOK_PATCH_TOUCH_REGION(H_K_TO_PEEK_R, ksceTouchPeekRegion)
DECL_FUNC_HOOK_PATCH_TOUCH_REGION(H_K_TO_READ_R, ksceTouchReadRegion)

int ksceCtrlGetControllerPortInfo_patched(SceCtrlPortInfo* info){
    int ret = TAI_CONTINUE(int, refs[H_K_CT_PORT_INFO], info);
    if (profile.entries[PR_CO_EMULATE_DS4].v.b){
        info->port[1] = SCE_CTRL_TYPE_DS4;
    }
	return ret;
}

int ksceDisplaySetFrameBufInternal_patched(int head, int index, const SceDisplayFrameBuf *pParam, int sync) {
    used_funcs[H_K_DISP_SET_FB] = 1;

    if (index && ksceAppMgrIsExclusiveProcessRunning())
        goto DISPLAY_HOOK_RET; // Do not draw over SceShell overlay

    if (pParam)  
        gui_draw(pParam);

    if (!head || !pParam)
        goto DISPLAY_HOOK_RET;

    if (gui_isOpen) {
        SceCtrlData ctrl;
        if(ksceCtrlPeekBufferPositive(0, &ctrl, 1) == 1)
            gui_onInput(&ctrl);
    }

DISPLAY_HOOK_RET:
    return TAI_CONTINUE(int, refs[H_K_DISP_SET_FB], head, index, pParam, sync);
}

int ksceKernelInvokeProcEventHandler_patched(int pid, int ev, int a3, int a4, int *a5, int a6) {
    used_funcs[H_K_INV_PROC_EV_HANDLER] = 1;
    char titleidLocal[sizeof(titleid)];
    int ret = ksceKernelLockMutex(mutex_procevent_uid, 1, NULL);
    if (ret < 0)
        goto PROCEVENT_EXIT;
    ksceKernelGetProcessTitleId(pid, titleidLocal, sizeof(titleidLocal));
    if (!strncmp(titleidLocal, "main", sizeof(titleidLocal)))
        strnclone(titleidLocal, HOME, sizeof(titleidLocal));
    switch (ev) {
        case 1: //Start
        case 5: //Resume
            // if (streq(titleidLocal, "TSTCTRL00") ||
            //     streq(titleidLocal, "TSTCTRL20") ||
            //     streq(titleidLocal, "TSTCTRLE0") ||
            //     streq(titleidLocal, "TSTCTRLE2") ||
            //     streq(titleidLocal, "TSTCTRLN0") ||
            //     streq(titleidLocal, "TSTCTRLN2"))
            //     break;
            if (!strncmp(titleidLocal, "NPXS", 4)){ //If system app
                SceKernelModuleInfo info;
                info.size = sizeof(SceKernelModuleInfo);
                _ksceKernelGetModuleInfo(pid, _ksceKernelGetProcessMainModule(pid), &info);
                if(strncmp(info.module_name, "ScePspemu", 9) &&
			        strcmp(titleidLocal, "NPXS10012") && //not PS3Link
			        strcmp(titleidLocal, "NPXS10013")) //not PS4Link
                        break; //Use MAIN profile
            }
            
            if (strncmp(titleid, titleidLocal, sizeof(titleid))) {
                strnclone(titleid, titleidLocal, sizeof(titleid));
                processid = pid;
                gui_close();
                for (int i = 0; i < HOOKS_NUM; i++)
                    used_funcs[i] = false;
                profile_load(titleid);
                remap_resetBuffers();
                delayedStartDone = false;
                LOG("\nPROFILE LOAD: %s\n", titleid);
            }
            break;
        case 3: //Close
        case 4: //Suspend
            if (!strcmp(titleid, titleidLocal)){ //If current app suspended
                if (strncmp(titleid, HOME, sizeof(titleid))) {
                    strnclone(titleid, HOME, sizeof(titleid));
                    processid = -1;
                    gui_close();
                    for (int i = 0; i < HOOKS_NUM; i++)
                        used_funcs[i] = false;
                    profile_load(HOME);
                    remap_resetBuffers();
                    delayedStartDone = false;
                    LOG("\nPROFILE LOAD: %s\n", titleid);
                }
            }
            break;
        default:
            break;
    }

    ksceKernelUnlockMutex(mutex_procevent_uid, 1);

PROCEVENT_EXIT:
    return TAI_CONTINUE(int, refs[H_K_INV_PROC_EV_HANDLER], pid, ev, a3, a4, a5, a6);
}

static int main_thread(SceSize args, void *argp) {
    uint32_t oldBtns = 0;
    while (thread_run) {
        //Activate delayed start
        if (!delayedStartDone 
            && startTick + settings[SETT_DELAY_INIT].v.u * 1000000 < ksceKernelGetSystemTimeWide()){
            remap_setup();
	        delayedStartDone = true;
        }

        SceCtrlData ctrl;
        if(ksceCtrlPeekBufferPositive(0, &ctrl, 1) < 1){
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
                            gui_open();
                            remap_resetBuffers();
                            break;
                        case HOTKEY_REMAPS_TOOGLE: FLIP(settings[SETT_REMAP_ENABLED].v.b); break;
                        case HOTKEY_RESET_SOFT: sysactions_softReset();  break;
                        case HOTKEY_RESET_COLD: sysactions_coldReset();  break;
                        case HOTKEY_STANDBY: sysactions_standby();  break;
                        case HOTKEY_SUSPEND: sysactions_suspend();  break;
                        case HOTKEY_DISPLAY_OFF: sysactions_displayOff();  break;
                        case HOTKEY_KILL_APP: sysactions_killCurrentApp();  break;
                    }
                }
            }
        }

        oldBtns = ctrl.buttons;
        
        ksceKernelDelayThread(30 * 1000);
    }
    return 0;
}

//Sync ds4vita config
void syncDS4Vita(){
    if (profile.entries[PR_CO_EMULATE_DS4].v.b){
        ds4vita_setPort(2);
        ds4vita_setPort0MergeMode(DS4_MERGE_DISABLED);
        LOG("configDS4Vita Port 2, no merge\n");
    } else {
        ds4vita_setPort(1);
        LOG("configDS4Vita Port 1, merge\n");
        ds4vita_setPort0MergeMode(DS4_MERGE_ENABLED);
    }
}
//Sync configurations across other plugins
void sync(){
    syncDS4Vita();
}

void hookE(uint8_t hookId, const char *module, uint32_t library_nid, uint32_t func_nid, const void *func) {
    hooks[hookId] = taiHookFunctionExportForKernel(KERNEL_PID, &refs[hookId], module, library_nid, func_nid, func);
}
void hookI(uint8_t hookId, const char *module, uint32_t library_nid, uint32_t func_nid, const void *func) {
    hooks[hookId] = taiHookFunctionImportForKernel(KERNEL_PID, &refs[hookId], module, library_nid, func_nid, func);
}
void exportF(const char *module, uint32_t library_nid_360, uint32_t func_nid_360, 
        uint32_t library_nid_365, uint32_t func_nid_365, const void *func){
    if (module_get_export_func(KERNEL_PID, module, library_nid_360, func_nid_360, (uintptr_t*)func) < 0) // 3.60
        module_get_export_func(KERNEL_PID, module, library_nid_365, func_nid_365, (uintptr_t*)func);     // 3.65
}

void _start() __attribute__ ((weak, alias ("module_start")));
int module_start(SceSize argc, const void *args) {
    
    LOG("Plugin started\n");
    snprintf(titleid, sizeof(titleid), HOME);
    settings_init();
    hotkeys_init();
    theme_init();
    profile_init();
    gui_init();
    remap_init();
    userspace_init();
    startTick = ksceKernelGetSystemTimeWide();
    sync();
	theme_load(settings[SETT_THEME].v.u);

    mutex_procevent_uid = ksceKernelCreateMutex("remaPSV2_mutex_procevent", 0, 0, NULL);

	//Allocate memory for buffers
    bufsMemId = ksceKernelAllocMemBlock("remapsv2_bufs_main", 
        SCE_KERNEL_MEMBLOCK_TYPE_KERNEL_RW, 
        (sizeof(SceTouchData) * BUFFERS_NUM * TOUCH_HOOKS_NUM + 0xfff) & ~0xfff, 
        NULL);
    ksceKernelGetMemBlockBase(bufsMemId, (void**)&bufsMemBase);
    LOG("MEMORY ALLOC main[SceTouchData] %i : %i\n", (int)bufsMemBase, (sizeof(SceTouchData) * BUFFERS_NUM * TOUCH_HOOKS_NUM  + sizeof(SceCtrlData) * BUFFERS_NUM * CTRL_HOOKS_NUM + 0xfff) & ~0xfff);
    for (int i = 0; i < TOUCH_HOOKS_NUM; i++)
        bufsStd[i] = (SceTouchData*)(bufsMemBase + sizeof(SceTouchData) * i * BUFFERS_NUM);

    // Hooking functions
    for (int i = 0; i < HOOKS_NUM; i++)
        hooks[i] = 0;
    hookE(H_CT_PEEK_P,     "SceCtrl", 0xD197E3C7, 0xA9C3CED6, sceCtrlPeekBufferPositive_patched);
    hookE(H_CT_READ_P,     "SceCtrl", 0xD197E3C7, 0x67E7AB83, sceCtrlReadBufferPositive_patched);
    hookE(H_CT_PEEK_N,     "SceCtrl", 0xD197E3C7, 0x104ED1A7, sceCtrlPeekBufferNegative_patched);
    hookE(H_CT_READ_N,     "SceCtrl", 0xD197E3C7, 0x15F96FB0, sceCtrlReadBufferNegative_patched);
	hookE(H_CT_PEEK_P_EXT, "SceCtrl", 0xD197E3C7, 0xA59454D3, sceCtrlPeekBufferPositiveExt_patched);
    hookE(H_CT_READ_P_EXT, "SceCtrl", 0xD197E3C7, 0xE2D99296, sceCtrlReadBufferPositiveExt_patched);

    hookE(H_CT_PEEK_P_2,   "SceCtrl", 0xD197E3C7, 0x15F81E8C, sceCtrlPeekBufferPositive2_patched);
    hookE(H_CT_READ_P_2,   "SceCtrl", 0xD197E3C7, 0xC4226A3E, sceCtrlReadBufferPositive2_patched);
    hookE(H_CT_PEEK_N_2,   "SceCtrl", 0xD197E3C7, 0x81A89660, sceCtrlPeekBufferNegative2_patched);
    hookE(H_CT_READ_N_2,   "SceCtrl", 0xD197E3C7, 0x27A0C5FB, sceCtrlReadBufferNegative2_patched);
    hookE(H_CT_PEEK_P_EXT2,"SceCtrl", 0xD197E3C7, 0x860BF292, sceCtrlPeekBufferPositiveExt2_patched);
    hookE(H_CT_READ_P_EXT2,"SceCtrl", 0xD197E3C7, 0xA7178860, sceCtrlReadBufferPositiveExt2_patched);
    
    // hookE(H_K_CT_PEEK_P,   "SceCtrl", 0x7823A5D1, 0xEA1D3A34, ksceCtrlPeekBufferPositive_patched);
    // hookE(H_K_CT_READ_P,   "SceCtrl", 0x7823A5D1, 0x9B96A1AA, ksceCtrlReadBufferPositive_patched);
    // hookE(H_K_CT_PEEK_N,   "SceCtrl", 0x7823A5D1, 0x19895843, ksceCtrlPeekBufferNegative_patched);
    // hookE(H_K_CT_READ_N,   "SceCtrl", 0x7823A5D1, 0x8D4E0DD1, ksceCtrlReadBufferNegative_patched);

    hookE(H_K_TO_PEEK,     "SceTouch", TAI_ANY_LIBRARY, 0xBAD1960B, ksceTouchPeek_patched);
    hookE(H_K_TO_READ,     "SceTouch", TAI_ANY_LIBRARY, 0x70C8AACE, ksceTouchRead_patched);
    hookE(H_K_TO_PEEK_R,   "SceTouch", TAI_ANY_LIBRARY, 0x9B3F7207, ksceTouchPeekRegion_patched);
    hookE(H_K_TO_READ_R,   "SceTouch", TAI_ANY_LIBRARY, 0x9A91F624, ksceTouchReadRegion_patched);

	hookE(H_K_CT_PORT_INFO,        "SceCtrl",       TAI_ANY_LIBRARY, 0xF11D0D30, ksceCtrlGetControllerPortInfo_patched);
	hookE(H_K_DISP_SET_FB,         "SceDisplay",    0x9FED47AC,      0x16466675, ksceDisplaySetFrameBufInternal_patched);
	hookI(H_K_INV_PROC_EV_HANDLER, "SceProcessmgr", 0x887F19D0,      0x414CC813, ksceKernelInvokeProcEventHandler_patched);

    exportF("SceKernelModulemgr", 0xC445FA63, 0x20A27FA9, 0x92C9FFC2, 0x679F5144, &_ksceKernelGetProcessMainModule);
    exportF("SceKernelModulemgr", 0xC445FA63, 0xD269F915, 0x92C9FFC2, 0xDAA90093, &_ksceKernelGetModuleInfo);

    thread_uid = ksceKernelCreateThread("remaPSV2_thread", main_thread, 0x3C, 0x3000, 0, 0x10000, 0);
    ksceKernelStartThread(thread_uid, 0, NULL);

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

    //Free mem
	ksceKernelFreeMemBlock(bufsMemId);

    settings_destroy();
    hotkeys_destroy();
    theme_destroy();
    profile_destroy();
    gui_destroy();
    remap_destroy();
    userspace_destroy();
    return SCE_KERNEL_STOP_SUCCESS;
}
