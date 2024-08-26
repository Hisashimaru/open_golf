#pragma once
#include "mathf.h"

enum InputDeviceType
{
	INPUT_DEVICE_NONE,
	INPUT_DEVICE_KEYBOARD,
	INPUT_DEVICE_GAMEPAD,
};

// Input
void input_init();
void input_update();
bool input_down(int key);
bool input_hit(int key);
bool input_up(int key);
float input_axis(int key);
float input_axis(int positive_key, int negative_key);
vec2 input_axis(int x_positive_key, int x_negative_key, int y_positive_key, int y_negative_key);

void input_set_key_state(int key, float state);
void input_set_mouse_wheel(int delta);
vec2 input_get_mouse_pos();
vec2 input_get_mouse_delta();
float input_get_mouse_wheel();
void input_set_mouse_pos(int x, int y);
void input_set_mouse_pos(vec2 pos);
void input_mouse_lock(bool lock);

// input action system
void input_register(const char *action, int key, int player_index=0);
void input_register_1d(const char *action, int positive_key, int negative_key, int player_index=0);
void input_register_2d(const char *action, int x_positive_key, int x_negative_key, int y_positive_key, int y_negative_key, int player_index=0);
void input_unregister_all();
bool input_down(const char *action, int player_index=0);
bool input_up(const char *action, int player_index=0);
bool input_hit(const char *action, int player_index=0);
float input_axis(const char *action, int player_index=0);
float input_axis(const char *positive_action, const char *negative_action, int player_index=0);
float input_axis_1d(const char *action, int player_index=0);
vec2 input_axis(const char *x_positive_action, const char *x_negative_action, const char *y_positive_action, const char *y_negative_action, int player_index=0);

InputDeviceType input_get_active_device(int player_index=0);


enum key_code
{
	KEY_BACKSPACE = 0x08,
	KEY_ENTER = 0x0D,
	KEY_ESCAPE = 0x1B,
	KEY_SHIFT = 0x10,
	KEY_CNTROL = 0x11,
	KEY_ALT = 0x12,
	KEY_SPACE = 0x20,

	KEY_LEFT = 0x25,
	KEY_UP,
	KEY_RIGHT,
	KEY_DOWN,

	KEY_0 = 0x03,
	KEY_1,
	KEY_2,
	KEY_3,
	KEY_4,
	KEY_5,
	KEY_6,
	KEY_7,
	KEY_8,
	KEY_9,

	KEY_A = 0x41,
	KEY_B,
	KEY_C,
	KEY_D,
	KEY_E,
	KEY_F,
	KEY_G,
	KEY_H,
	KEY_I,
	KEY_J,
	KEY_K,
	KEY_L,
	KEY_M,
	KEY_N,
	KEY_O,
	KEY_P,
	KEY_Q,
	KEY_R,
	KEY_S,
	KEY_T,
	KEY_U,
	KEY_V,
	KEY_W,
	KEY_X,
	KEY_Y,
	KEY_Z,
	KEY_BACKQUOTE = 0x60,

	KEY_F1 = 0x70,
	KEY_F2,
	KEY_F3,
	KEY_F4,
	KEY_F5,
	KEY_F6,
	KEY_F7,
	KEY_F8,
	KEY_F9,
	KEY_F10,
	KEY_F11,
	KEY_F12,

	// Mouse
	KEY_MOUSE0 = 0x15E,
	KEY_MOUSE1,
	KEY_MOUSE2,
	KEY_MOUSE3,
	KEY_MOUSE4,
	KEY_MB0 = KEY_MOUSE0,
	KEY_MB1,
	KEY_MB2,
	KEY_MB3,
	KEY_MB4,
	KEY_LMB = KEY_MOUSE0,
	KEY_RMB,
	KEY_MMB,
	KEY_MOUSE_WHEEL_UP,
	KEY_MOUSE_WHEEL_DOWN,
	KEY_MOUSE_MOVE_UP,
	KEY_MOUSE_MOVE_DOWN,
	KEY_MOUSE_MOVE_LEFT,
	KEY_MOUSE_MOVE_RIGHT,
	MAX_PC_KEYS,

	// GamePad 1
	KEY_PAD_UP,
	KEY_PAD_DOWN,
	KEY_PAD_LEFT,
	KEY_PAD_RIGHT,
	KEY_PAD_START,
	KEY_PAD_BACK,
	KEY_PAD_L3,	// Left Thumb
	KEY_PAD_R3,	// Right Thubm
	KEY_PAD_LB,	// Left Shoulder
	KEY_PAD_RB,	// Right Shoulder
	KEY_PAD_A,
	KEY_PAD_B,
	KEY_PAD_X,
	KEY_PAD_Y,
	KEY_PAD_LS_UP,
	KEY_PAD_LS_DOWN,
	KEY_PAD_LS_LEFT,
	KEY_PAD_LS_RIGHT,
	KEY_PAD_RS_UP,
	KEY_PAD_RS_DOWN,
	KEY_PAD_RS_LEFT,
	KEY_PAD_RS_RIGHT,
	KEY_PAD_LT,	// Left Trigger
	KEY_PAD_RT, // Right Trigger

	// GamePad 2
	KEY_PAD1_UP,
	KEY_PAD1_DOWN,
	KEY_PAD1_LEFT,
	KEY_PAD1_RIGHT,
	KEY_PAD1_START,
	KEY_PAD1_BACK,
	KEY_PAD1_L3,	// Left Thumb
	KEY_PAD1_R3,	// Right Thubm
	KEY_PAD1_LB,	// Left Shoulder
	KEY_PAD1_RB,	// Right Shoulder
	KEY_PAD1_A,
	KEY_PAD1_B,
	KEY_PAD1_X,
	KEY_PAD1_Y,
	KEY_PAD1_LS_UP,
	KEY_PAD1_LS_DOWN,
	KEY_PAD1_LS_LEFT,
	KEY_PAD1_LS_RIGHT,
	KEY_PAD1_RS_UP,
	KEY_PAD1_RS_DOWN,
	KEY_PAD1_RS_LEFT,
	KEY_PAD1_RS_RIGHT,
	KEY_PAD1_LT,	// Left Trigger
	KEY_PAD1_RT,	// Right Trigger
};

#define MAX_PAD_BUTTONS 24	// Check specific gamepad buttons  ex: input_down(MAX_PAD_BUTTONS * player_index + KEY_PAD_A)