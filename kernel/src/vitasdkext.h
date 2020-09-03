#ifndef _VITASDKEXT_H_
#define _VITASDKEXT_H_
#include <psp2/touch.h>
typedef enum SceCtrlButtonsExt {
    SCE_CTRL_TOUCHPAD    = 0x04000000,             //!< Dualshock 4 Touchpad button
    SCE_CTRL_L2          = SCE_CTRL_LTRIGGER,
    SCE_CTRL_R2          = SCE_CTRL_RTRIGGER,
} SceCtrlButtonsExt;

typedef void* SceClibMspace;
int ksceTouchPeek (SceUInt32 port, SceTouchData *pData, SceUInt32 nBufs);
int ksceTouchRead (SceUInt32 port, SceTouchData *pData, SceUInt32 nBufs);
int ksceTouchSetSamplingState(SceUInt32 port, SceTouchSamplingState state);
int ksceTouchGetPanelInfo(SceUInt32 port, SceTouchPanelInfo *pPanelInfo);
int ksceTouchPeekRegion(SceUInt32 port, SceTouchData *pData, SceUInt32 nBufs, int region);
int ksceTouchReadRegion(SceUInt32 port, SceTouchData *pData, SceUInt32 nBufs, int region);

#endif