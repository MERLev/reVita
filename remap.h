#ifndef _REMAP_H_
#define _REMAP_H_

extern int retouch(SceUInt32 port, SceTouchData *pData, SceUInt32 nBufs, uint8_t hookId);
extern int remap(SceCtrlData *ctrl, int count, int hookId, int logic);
extern void patchToExt(SceCtrlData *ctrl);
extern void remapInit();

#endif