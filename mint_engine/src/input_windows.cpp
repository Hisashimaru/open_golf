#pragma once
#include <string>
#include <unordered_map>

#ifdef _WIN32
#include "app.h"
#include "input.h"
#include <windows.h>
#include <Xinput.h>
#pragma comment (lib, "xinput.lib")

struct InputAction
{
	int keys[4]; // {x_positive(main), x_negative, y_positive, y_negative}
};

#define MAX_PLAYERS 4
#define MAX_KEYS 512
static struct input_ctx
{
	float key_state[MAX_KEYS];
	float key_state_last[MAX_KEYS];

	struct {
		vec2 position;
		vec2 delta;
		int wheel;
		bool lock;
	} mouse;

	int gamepad_id[MAX_PLAYERS];
	InputDeviceType active_device[MAX_PLAYERS];

	std::unordered_map<std::string, std::vector<InputAction>> actions[MAX_PLAYERS];
} ctx = {};


void input_init()
{
	POINT point;
	GetCursorPos(&point);
	HWND *hwnd = (HWND*)app_get_context();
	ScreenToClient(*hwnd, &point);
	ctx.mouse.position = vec2((float)point.x, (float)point.y);
	for(int i=0; i<MAX_PLAYERS; i++)
	{
		ctx.gamepad_id[i] = -1;
	}
}

static bool _is_using_gamepad(int index)
{
	bool use = false;
	int key = KEY_PAD_UP + MAX_PAD_BUTTONS * index;
	for(int i=0; i<MAX_PAD_BUTTONS; i++)
	{
		use |= ctx.key_state[key+i] > 0.2f;
	}
	return use;
}

static void _update_gamepad(int device_index)
{
	XINPUT_STATE state;
	if(XInputGetState(device_index, &state) == ERROR_SUCCESS)
	{
		int player_index = -1;
		for(int i=0; i<MAX_PLAYERS; i++)
		{
			if(ctx.gamepad_id[i] == device_index)
			{
				player_index = i;
				break;
			}
		}
		if(player_index == -1 && state.Gamepad.wButtons)
		{
			for(int i=0; i<MAX_PLAYERS; i++)
			{
				if(ctx.gamepad_id[i] == -1)
				{
					// gamepad connected
					//printf("gamepad connected![%d]\n", index);
					ctx.gamepad_id[i] = device_index;
					player_index = i;
					break;
				}
			}
		}
		if(player_index == -1)
		{
			return;
		}

		WORD buttons = state.Gamepad.wButtons;
		int n = (MAX_PAD_BUTTONS * player_index);
		ctx.key_state[n + KEY_PAD_UP] = (buttons & XINPUT_GAMEPAD_DPAD_UP) ? 1.0f : 0.0f;
		ctx.key_state[n + KEY_PAD_DOWN] = (buttons & XINPUT_GAMEPAD_DPAD_DOWN) ? 1.0f : 0.0f;
		ctx.key_state[n + KEY_PAD_LEFT] = (buttons & XINPUT_GAMEPAD_DPAD_LEFT) ? 1.0f : 0.0f;
		ctx.key_state[n + KEY_PAD_RIGHT] = (buttons & XINPUT_GAMEPAD_DPAD_RIGHT) ? 1.0f : 0.0f;
		ctx.key_state[n + KEY_PAD_START] = (buttons & XINPUT_GAMEPAD_START) ? 1.0f : 0.0f;
		ctx.key_state[n + KEY_PAD_BACK] = (buttons & XINPUT_GAMEPAD_BACK) ? 1.0f : 0.0f;
		ctx.key_state[n + KEY_PAD_L3] = (buttons & XINPUT_GAMEPAD_LEFT_THUMB) ? 1.0f : 0.0f;
		ctx.key_state[n + KEY_PAD_R3] = (buttons & XINPUT_GAMEPAD_RIGHT_THUMB) ? 1.0f : 0.0f;
		ctx.key_state[n + KEY_PAD_LB] = (buttons & XINPUT_GAMEPAD_LEFT_SHOULDER) ? 1.0f : 0.0f;
		ctx.key_state[n + KEY_PAD_RB] = (buttons & XINPUT_GAMEPAD_RIGHT_SHOULDER) ? 1.0f : 0.0f;
		ctx.key_state[n + KEY_PAD_A] = (buttons & XINPUT_GAMEPAD_A) ? 1.0f : 0.0f;
		ctx.key_state[n + KEY_PAD_B] = (buttons & XINPUT_GAMEPAD_B) ? 1.0f : 0.0f;
		ctx.key_state[n + KEY_PAD_X] = (buttons & XINPUT_GAMEPAD_X) ? 1.0f : 0.0f;
		ctx.key_state[n + KEY_PAD_Y] = (buttons & XINPUT_GAMEPAD_Y) ? 1.0f : 0.0f;

		// left stick
		const float dead_zone = 0.2f;
		const float I16_MAX = 32767;
		const float I16_MIN = 32768;
		float left_stick_x = state.Gamepad.sThumbLX / (float)SHRT_MAX;
		float left_stick_y = state.Gamepad.sThumbLY / (float)SHRT_MAX;
		ctx.key_state[n + KEY_PAD_LS_RIGHT] = (left_stick_x >= dead_zone) ? left_stick_x : 0.0f;
		ctx.key_state[n + KEY_PAD_LS_LEFT] = (left_stick_x <= -dead_zone) ? -left_stick_x : 0.0f;
		ctx.key_state[n + KEY_PAD_LS_UP] = (left_stick_y >= dead_zone) ? left_stick_y : 0.0f;
		ctx.key_state[n + KEY_PAD_LS_DOWN] = (left_stick_y <= -dead_zone) ? -left_stick_y : 0.0f;

		// right stick
		float right_stick_x = state.Gamepad.sThumbRX / (float)SHRT_MAX;
		float right_stick_y = state.Gamepad.sThumbRY / (float)SHRT_MAX;
		ctx.key_state[n + KEY_PAD_RS_RIGHT] = (right_stick_x >= dead_zone) ? right_stick_x : 0.0f;
		ctx.key_state[n + KEY_PAD_RS_LEFT] = (right_stick_x <= -dead_zone) ? -right_stick_x : 0.0f;
		ctx.key_state[n + KEY_PAD_RS_UP] = (right_stick_y >= dead_zone) ? right_stick_y : 0.0f;
		ctx.key_state[n + KEY_PAD_RS_DOWN] = (right_stick_y <= -dead_zone) ? -right_stick_y : 0.0f;

		// Trigger
		float left_trigger = state.Gamepad.bLeftTrigger / 255.0f;
		float right_trigger = state.Gamepad.bRightTrigger / 255.0f;
		ctx.key_state[n + KEY_PAD_LT] = left_trigger >= dead_zone ? left_trigger : 0.0f;
		ctx.key_state[n + KEY_PAD_RT] = right_trigger >= dead_zone ? right_trigger : 0.0f;

		if(_is_using_gamepad(player_index))
		{
			ctx.active_device[player_index] = INPUT_DEVICE_GAMEPAD;
		}
	}
	else
	{
		for(int i=0; i<MAX_PLAYERS; i++)
		{
			if(ctx.gamepad_id[i] == device_index)
			{
				// disconnected
				ctx.gamepad_id[i] = -1;
				ctx.active_device[i] = INPUT_DEVICE_NONE;
				break;
			}
		}
	}
}

void input_update()
{
	if(!app_is_active())
	{
		for(int i=0; i<MAX_KEYS; i++)
		{
			ctx.key_state[i] = 0.0f;
		}
	}

	// Keyboard
	for(int i=0; i<MAX_KEYS; i++)
	{
		ctx.key_state_last[i] = ctx.key_state[i];
	}

	// Keyboard mouse check
	bool use_keyboard_mouse = false;
	for(int i=0; i<MAX_PC_KEYS; i++)
	{
		use_keyboard_mouse |= ctx.key_state_last[i] > 0.2f;
	}
	if(use_keyboard_mouse)
	{
		ctx.active_device[0] = INPUT_DEVICE_KEYBOARD;
	}

	// Mouse Cursor
	POINT point;
	GetCursorPos(&point);
	HWND *hwnd = (HWND*)app_get_context();
	ScreenToClient(*hwnd, &point);
	vec2 new_mouse_pos =  vec2((float)point.x, (float)point.y);
	ctx.mouse.delta = new_mouse_pos - ctx.mouse.position;
	ctx.mouse.position = new_mouse_pos;
	vec2 app_window_size = app_get_size();
	if(ctx.mouse.lock && app_is_active())
	{
		int x = (int)app_window_size.x / 2;
		int y = (int)app_window_size.y / 2;
		input_set_mouse_pos(x, y);
		ctx.mouse.position = vec2((float)x, (float)y);
	}
	ctx.key_state[KEY_MOUSE_MOVE_UP] = (ctx.mouse.delta.y > 0.0f) ? ctx.mouse.delta.y : 0.0f;
	ctx.key_state[KEY_MOUSE_MOVE_DOWN] = (ctx.mouse.delta.y < 0.0f) ? -ctx.mouse.delta.y : 0.0f;
	ctx.key_state[KEY_MOUSE_MOVE_RIGHT] = (ctx.mouse.delta.x > 0.0f) ? ctx.mouse.delta.x : 0.0f;
	ctx.key_state[KEY_MOUSE_MOVE_LEFT] = (ctx.mouse.delta.x < 0.0f) ? -ctx.mouse.delta.x : 0.0f;
	ctx.mouse.wheel = 0;
	ctx.key_state[KEY_MOUSE_WHEEL_UP] = 0.0f;
	ctx.key_state[KEY_MOUSE_WHEEL_DOWN] = 0.0f;


	// Game pad
	for(int i=0; i<4; i++)
	{
		_update_gamepad(i);
	}
}

bool input_down(int key)
{
	return ctx.key_state[key] > 0.0f;
}

bool input_hit(int key)
{
	return ctx.key_state[key] > 0.0f && ctx.key_state_last[key] == 0.0f;
}

bool input_up(int key)
{
	return ctx.key_state[key] == 0.0f && ctx.key_state_last[key] > 0.0f;
}

float input_axis(int key)
{
	return ctx.key_state[key];
}

float input_axis(int positive_key, int negative_key)
{
	return ctx.key_state[positive_key] - ctx.key_state[negative_key];
}

vec2 input_axis(int x_positive_key, int x_negative_key, int y_positive_key, int y_negative_key)
{
	return vec2(ctx.key_state[x_positive_key] - ctx.key_state[x_negative_key],
				ctx.key_state[y_positive_key] - ctx.key_state[y_negative_key]);
}

void input_set_key_state(int key, float state)
{
	ctx.key_state[key] = state;
}

void input_set_mouse_wheel(int delta)
{
	ctx.mouse.wheel = delta;
	ctx.key_state[KEY_MOUSE_WHEEL_UP] = (delta > 0) ? (float)delta : 0.0f;
	ctx.key_state[KEY_MOUSE_WHEEL_DOWN] = (delta < 0) ? (float)-delta : 0.0f;
}

vec2 input_get_mouse_pos(){	return ctx.mouse.position;}
vec2 input_get_mouse_delta(){return ctx.mouse.delta;}
float input_get_mouse_wheel(){return (float)ctx.mouse.wheel;}

void input_set_mouse_pos(int x, int y)
{
	POINT point = {x, y};
	HWND *hwnd = (HWND*)app_get_context();
	ClientToScreen(*hwnd, &point);
	SetCursorPos(point.x, point.y);
	ctx.mouse.position = vec2((float)x, (float)y);
}
void input_set_mouse_pos(vec2 pos){input_set_mouse_pos((int)pos.x, (int)pos.y);}


void input_mouse_lock(bool lock)
{
	lock = (lock == 1 && app_is_active()) ? 1 : 0;
	if(ctx.mouse.lock == lock) return;
	ctx.mouse.lock = lock;
	ShowCursor(!lock);
}



// input action system //////////////////////////////////////////////////////////
void input_register(const char *action, int key, int player_index)
{
	if(player_index >= MAX_PLAYERS) return;
	ctx.actions[player_index][action].push_back({key});
}

void input_register_1d(const char *action, int positive_key, int negative_key, int player_index)
{
	if(player_index >= MAX_PLAYERS) return;
	ctx.actions[player_index][action].push_back({positive_key, negative_key});
}

void input_register_2d(const char *action, int x_positive_key, int x_negative_key, int y_positive_key, int y_negative_key, int player_index)
{
	if(player_index >= MAX_PLAYERS) return;
	ctx.actions[player_index][action].push_back({x_positive_key, x_negative_key, y_positive_key, y_negative_key});
}

void input_unregister_all()
{
	for(int i=0; i<MAX_PLAYERS; i++)
	{
		ctx.actions[i].clear();
	}
}

bool input_down(const char *action, int player_index)
{
	if(player_index >= MAX_PLAYERS) return false;
	if(ctx.actions[player_index].count(action) == 0) return false;

	bool ret = false;
	for(int i=0; i<ctx.actions[player_index][action].size(); i++)
	{
		ret |= input_down(ctx.actions[player_index][action][i].keys[0]);
	}
	return ret;
}

bool input_up(const char *action, int player_index)
{
	if(player_index >= MAX_PLAYERS) return false;
	if(ctx.actions[player_index].count(action) == 0) return false;

	bool ret = false;
	for(int i=0; i<ctx.actions[player_index][action].size(); i++)
	{
		ret |= input_up(ctx.actions[player_index][action][i].keys[0]);
	}
	return ret;
}

bool input_hit(const char *action, int player_index)
{
	if(player_index >= MAX_PLAYERS) return false;
	if(ctx.actions[player_index].count(action) == 0) return false;

	bool ret = false;
	for(int i=0; i<ctx.actions[player_index][action].size(); i++)
	{
		ret |= input_hit(ctx.actions[player_index][action][i].keys[0]);
	}
	return ret;
}

float input_axis(const char *action, int player_index)
{
	if(player_index >= MAX_PLAYERS) return 0.0f;
	if(ctx.actions[player_index].count(action) == 0) return 0.0f;

	float ret = 0;
	for(int i=0; i<ctx.actions[player_index][action].size(); i++)
	{
		float value = input_axis(ctx.actions[player_index][action][i].keys[0]);
		ret = value > ret ? value : ret;
	}
	return ret;
}

vec2 input_axis(const char *x_positive_action, const char *x_negative_action, const char *y_positive_action, const char *y_negative_action, int player_index)
{
	if(player_index >= MAX_PLAYERS) return vec2();

	float x_pos = input_axis(x_positive_action, player_index);
	float x_neg = input_axis(x_negative_action, player_index);
	float y_pos = input_axis(y_positive_action, player_index);
	float y_neg = input_axis(y_negative_action, player_index);

	return vec2(x_pos - x_neg, y_pos - y_neg);
}

float input_axis_1d(const char *action, int player_index)
{
	if(player_index >= MAX_PLAYERS) return 0.0f;
	if(ctx.actions[player_index].count(action) == 0) return 0.0f;

	float ret = 0;
	for(int i=0; i<ctx.actions[player_index][action].size(); i++)
	{
		InputAction act = ctx.actions[player_index][action][i];
		float tmp = input_axis(act.keys[0], act.keys[1]);
		float neg = fminf(0, fminf(ret, tmp));
		float pos = fmaxf(0, fmaxf(ret, tmp));
		ret = neg + pos;
	}
	return ret;
}

vec2 input_axis_2d(const char *action, int player_index)
{
	if(player_index >= MAX_PLAYERS) return vec2();
	if(ctx.actions[player_index].count(action) == 0) return vec2();

	vec2 ret = vec2();
	for(int i=0; i<ctx.actions[player_index][action].size(); i++)
	{
		InputAction act = ctx.actions[player_index][action][i];
		vec2 tmp = input_axis(act.keys[0], act.keys[1], act.keys[2], act.keys[3]);
		vec2 neg = vec2(fminf(0, fminf(ret.x, tmp.x)), fminf(0, fminf(ret.y, tmp.y)));
		vec2 pos = vec2(fmaxf(0, fmaxf(ret.x, tmp.x)), fmaxf(0, fmaxf(ret.y, tmp.y)));
		ret = neg + pos;
	}

	return ret;
}

InputDeviceType input_get_active_device(int player_index)
{
	if(player_index >= MAX_PLAYERS) return INPUT_DEVICE_NONE;
	return ctx.active_device[player_index];
}


#endif // _WIN32

