#ifndef _UI_H_
#define _UI_H_

extern uint8_t show_menu;
extern void ui_inputHandler(SceCtrlData *ctrl);
extern void ui_draw(const SceDisplayFrameBuf *pParam);
extern void ui_show();

#endif