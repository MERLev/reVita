#ifndef _REMAP_H_
#define _REMAP_H_

#define BUFFERS_NUM         64

extern int remap_touch(SceUInt32 port, SceTouchData *pData, SceUInt32 nBufs, uint8_t hookId);
extern int remap_controls(SceCtrlData *ctrl, int nBufs, int hookId);
extern void remap_patchToExt(SceCtrlData *ctrl);
extern void remap_init();
extern void remap_resetCtrlBuffers(uint8_t hookId);
extern void remap_resetTouchBuffers(uint8_t hookId);

#endif