#include <vitasdk.h>
#include "common.h"

const uint32_t HW_BUTTONS[PHYS_BUTTONS_NUM] = {
	SCE_CTRL_CROSS, SCE_CTRL_CIRCLE, SCE_CTRL_TRIANGLE, SCE_CTRL_SQUARE,
	SCE_CTRL_START, SCE_CTRL_SELECT, SCE_CTRL_LTRIGGER, SCE_CTRL_RTRIGGER,
	SCE_CTRL_UP, SCE_CTRL_RIGHT, SCE_CTRL_LEFT, SCE_CTRL_DOWN, SCE_CTRL_L1,
	SCE_CTRL_R1, SCE_CTRL_L3, SCE_CTRL_R3
};

// Generic clamp function
int32_t clamp(int32_t value, int32_t mini, int32_t maxi) {
	if (value < mini) { return mini; }
	if (value > maxi) { return maxi; }
	return value;
}