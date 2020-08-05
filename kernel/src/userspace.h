#ifndef _USERSPACE_H_
#define _USERSPACE_H_

int _sceMotionGetState(SceMotionState *pData);

void userspace_init();
void userspace_destroy();

#endif