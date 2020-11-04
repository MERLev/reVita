#include <vitasdkkern.h>
#include <taihen.h>

#include "common.h"
#include "vitasdkext.h"
#include "main.h"
#include "log.h"

int importF(const char *module, uint32_t library_nid_360, uint32_t func_nid_360, 
        uint32_t library_nid_365, uint32_t func_nid_365, const void *func){
    int ret;
    if ((ret = module_get_export_func(KERNEL_PID, module, library_nid_360, func_nid_360, (uintptr_t*)func)) < 0)    // 3.60
        ret = module_get_export_func(KERNEL_PID, module, library_nid_365, func_nid_365, (uintptr_t*)func);          // 3.65
    return ret;
}

int importFByOffset(char* modname, size_t offset, uintptr_t *addr){
    STRUCTS(tai_module_info_t, modInfo);
    int ret = taiGetModuleInfoForKernel(KERNEL_PID, modname, &modInfo);
    if (ret < 0)
        return ret;
    return module_get_offset(KERNEL_PID, modInfo.modid, 0, offset | 1, addr);
}

#define IMPORT(modname, lnid, fnid, name)\
    if (module_get_export_func(KERNEL_PID, #modname, lnid, fnid, (uintptr_t*)&_##name))\
        LOG("ERROR: import "#modname">"#name"\n");
        
#define IMPORT2(modname, lnid360, fnid360, lnid365, fnid365, name)\
    if (importF(#modname, lnid360, fnid360, lnid365, fnid365, (uintptr_t*)&_##name) < 0)\
        LOG("ERROR: import "#modname">"#name"\n");

#define IMPORT_OFFSET(modname, offset, name)\
    if (importFByOffset(#modname, offset, (uintptr_t*)&_##name) < 0)\
        LOG("ERROR: import by offset "#modname">"#name">'%08X'\n", offset);\

SceUID (*_ksceKernelGetProcessMainModule)(SceUID pid);
SceUID ksceKernelGetProcessMainModule(SceUID pid){
    return _ksceKernelGetProcessMainModule(pid);
}

int (*_ksceKernelGetModuleInfo)(SceUID pid, SceUID modid, SceKernelModuleInfo *info);
int ksceKernelGetModuleInfo(SceUID pid, SceUID modid, SceKernelModuleInfo *info){
    return _ksceKernelGetModuleInfo(pid, modid, info);
}

int (*_ksceCtrlSetSamplingModeExt)(int mode);
int ksceCtrlSetSamplingModeExt(int mode){
    return _ksceCtrlSetSamplingModeExt(mode);
}

int (*_ksceCtrlPeekBufferPositive2)(int port, SceCtrlData *ctrl, int nBufs);
int ksceCtrlPeekBufferPositive2(int port, SceCtrlData *ctrl, int nBufs){
    return _ksceCtrlPeekBufferPositive2(port, ctrl, nBufs);
}

int (*_ksceCtrlPeekBufferPositiveExt)(int port, SceCtrlData *ctrl, int nBufs);
int ksceCtrlPeekBufferPositiveExt(int port, SceCtrlData *ctrl, int nBufs){
    return _ksceCtrlPeekBufferPositiveExt(port, ctrl, nBufs);
}

int (*_ksceCtrlPeekBufferPositiveExt2)(int port, SceCtrlData *ctrl, int nBufs);
int ksceCtrlPeekBufferPositiveExt2(int port, SceCtrlData *ctrl, int nBufs){
    return _ksceCtrlPeekBufferPositiveExt2(port, ctrl, nBufs);
}

void vitasdkext_init(){
    IMPORT_OFFSET(SceCtrl, 0x3EF8, ksceCtrlPeekBufferPositive2);
    IMPORT_OFFSET(SceCtrl, 0x3928, ksceCtrlPeekBufferPositiveExt);
    IMPORT_OFFSET(SceCtrl, 0x4B48, ksceCtrlPeekBufferPositiveExt2);
    IMPORT_OFFSET(SceCtrl, 0x2B98, ksceCtrlSetSamplingModeExt);

    IMPORT2(SceKernelModulemgr, 0xC445FA63, 0x20A27FA9, 0x92C9FFC2, 0x679F5144, ksceKernelGetProcessMainModule);
    IMPORT2(SceKernelModulemgr, 0xC445FA63, 0xD269F915, 0x92C9FFC2, 0xDAA90093, ksceKernelGetModuleInfo);
}
void vitasdkext_destroy(){

}