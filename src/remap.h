#ifndef _REMAP_H_
#define _REMAP_H_

extern int retouch(SceUInt32 port, SceTouchData *pData, SceUInt32 nBufs, uint8_t hookId);
extern int remap(SceCtrlData *ctrl, int nBufs, int hookId);
extern void patchToExt(SceCtrlData *ctrl);
extern void remap_init();
extern void remap_resetBuffers(uint8_t hookId);
extern void remap_resetTouchBuffers(uint8_t hookId);

#endif