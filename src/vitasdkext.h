#ifndef _VITASDKEXT_H_
#define _VITASDKEXT_H_
typedef enum SceCtrlButtonsExt {
    SCE_CTRL_TOUCHPAD    = 0x04000000             //!< Dualshock 4 Touchpad button
} SceCtrlButtonsExt;

typedef void* SceClibMspace;
int ksceTouchPeek (SceUInt32 port, SceTouchData *pData, SceUInt32 nBufs);
int ksceTouchSetSamplingState(SceUInt32 port, SceTouchSamplingState state);
int ksceTouchGetPanelInfo(SceUInt32 port, SceTouchPanelInfo *pPanelInfo);

#endif