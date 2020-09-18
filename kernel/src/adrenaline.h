#ifndef _ADRENALINE_H_
#define _ADRENALINE_H_

// Taken from Adrenaline
typedef struct {
	int savestate_mode;
	int num;
	unsigned int sp;
	unsigned int ra;

	int pops_mode;
	int draw_psp_screen_in_pops;
	char title[128];
	char titleid[12];
	char filename[256];

	int psp_cmd;
	int vita_cmd;
	int psp_response;
	int vita_response;
} SceAdrenaline;

#endif