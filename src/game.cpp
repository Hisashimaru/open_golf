#include <memory>
#include <string>
#include "app.h"
#include "input.h"
#include "game.h"
#include "map.h"
#include "game_ui.h"
#include "common.h"
#include "resource_manager.h"
#include "scenes/scene_base.h"
#include "scenes/scene_game.h"
#include "scenes/scene_edit.h"

#define IS_TIME_PASSED(t) (t <= time_now())
#define IS_TIME_CURRENT(t) (t <= time_now() && t > time_last())

#define RESULT_SHOW_TIME 0.7f
#define RESULT_INPUT_KEY_TIME 1.0f

static struct game_ctx
{
	std::unique_ptr<Scene> scene;
	std::vector<std::string> map_file_list;
	std::string map_file_path;
	float wait_fade_out_time;
	int current_level;

	// result
	float result_show_time;
	float result_key_input_time;
	bool showing_result;
	bool showing_final_result;
	bool showing_thanks;
} ctx = {};

static void _change_map();

void game_init()
{
	ui_init();
	if(global.game_mode == GameMode::Edit)
	{
		ctx.scene = std::make_unique<EditScene>();
	}
	else
	{
		ctx.scene = std::make_unique<GameScene>();
		ctx.map_file_list.clear();
		ctx.map_file_list.push_back("map1");
		ctx.map_file_list.push_back("map2");
		ctx.map_file_list.push_back("map3");
		ctx.map_file_path = ctx.map_file_list[0];
		ctx.current_level = 0;
	}

	ctx.scene->init();
	_change_map();

	ctx.result_show_time = 0;
	ctx.result_key_input_time = 0;
	ctx.showing_result = 0;
	ctx.showing_final_result = 0;
}

void game_update()
{
	ctx.scene->update();
	ui_update();

	// load map if need
	if(!ctx.map_file_path.empty())
	{
		// fade out
		if(ctx.wait_fade_out_time == 0.0f)
		{
			ctx.wait_fade_out_time = time_now() + 0.5f;
			ui_fade_out(0.5f);
		}
		else if(ctx.wait_fade_out_time <= time_now())
		{
			_change_map();
			ctx.wait_fade_out_time = 0.0f;
		}
	}


	// result screen
	if(ctx.showing_result)
	{
		if(IS_TIME_CURRENT(ctx.result_show_time))
		{
			ui_show_result();
		}

		if(IS_TIME_CURRENT(ctx.result_key_input_time))
		{
			ui_show_next_text();
		}

		if(IS_TIME_PASSED(ctx.result_key_input_time))
		{
			// show "press a key to the next course"

			if(input_hit(KEY_SPACE) || input_hit(KEY_MOUSE0) ||
				input_hit(KEY_PAD_A) || input_hit(KEY_PAD_RT) ||
				input_hit(KEY_PAD1_A) || input_hit(KEY_PAD1_RT))
			{
				if(game_has_next_map())
				{
					game_next_map();
				}
				else
				{
					ui_show_final_result();
					ctx.showing_final_result = true;
					ctx.result_show_time = time_now() + RESULT_SHOW_TIME;
					ctx.result_key_input_time = ctx.result_show_time + RESULT_INPUT_KEY_TIME;
				}
				ctx.showing_result = false;
			}
		}
	}

	// final result
	if(ctx.showing_final_result)
	{
		if(IS_TIME_CURRENT(ctx.result_show_time))
		{
			ui_show_final_result();
		}

		if(IS_TIME_CURRENT(ctx.result_key_input_time))
		{
			ui_show_next_text();
		}

		if(IS_TIME_PASSED(ctx.result_key_input_time))
		{
			// show "press a key to the next course"

			if(input_hit(KEY_SPACE) || input_hit(KEY_MOUSE0) ||
				input_hit(KEY_PAD_A) || input_hit(KEY_PAD_RT) ||
				input_hit(KEY_PAD1_A) || input_hit(KEY_PAD1_RT))
			{
				ui_fade_out(0.3f);
				ctx.result_show_time = time_now() + 0.3f;
				ctx.showing_final_result = false;
				ctx.showing_thanks = true;
			}
		}
	}

	// Thanks you for plaing
	if(ctx.showing_thanks)
	{
		if(IS_TIME_CURRENT(ctx.result_show_time))
		{
			ui_show_thanks();
			ui_fade_in(0.3f);
		}

		if(IS_TIME_CURRENT(ctx.result_show_time + 3.0f))
		{
			ui_fade_out(0.3f);
		}

		if(IS_TIME_CURRENT(ctx.result_show_time + 3.3f))
		{
			game_change_mode(GameMode::Single);
			ctx.showing_thanks = false;
		}
	}


	// change game mode
	if(input_hit(KEY_F1)) {
		game_change_mode(GameMode::Single);
	} else if(input_hit(KEY_F2)) {
		game_change_mode(GameMode::Versus);
	} else if(input_hit(KEY_F3)) {
		game_change_mode(GameMode::Coop);
	}
}

void game_draw()
{
	ctx.scene->draw();
}

void game_uninit()
{
	ctx.scene->uninit();
	ctx.scene = nullptr;
	ctx.map_file_list.clear();
	ui_uninit();
}

void game_cupin()
{
	// calculate score
	// player 1
	global.result.p1.total_shots += global.result.p1.shots;
	global.result.p1.total_time += global.result.p1.time;
	global.result.p1.score = (int)(500.0f - ((float)global.result.p1.shots + (global.result.p1.time / 60.0f)) * 10.0f);
	global.result.p1.total_score += global.result.p1.score;

	// playe 2
	global.result.p2.total_shots += global.result.p2.shots;
	global.result.p2.total_time += global.result.p2.time;
	global.result.p2.score = (int)(500.0f - ((float)global.result.p2.shots + (global.result.p2.time / 60.0f)) * 10.0f);
	global.result.p2.total_score += global.result.p2.score;

	ctx.result_show_time = time_now() + RESULT_SHOW_TIME;
	ctx.result_key_input_time = ctx.result_show_time + RESULT_INPUT_KEY_TIME;
	ctx.showing_result = true;
}

void game_change_map(const char *filename)
{
	// load map after scene update
	ctx.map_file_path = filename;
}

void game_next_map()
{
	if(game_has_next_map())
	{
		ctx.current_level++;
	}
	else
	{
		ctx.current_level = 0;	// loop map list
	}
	ctx.map_file_path = ctx.map_file_list[ctx.current_level];
}

bool game_has_next_map()
{
	int next_level = ctx.current_level + 1;
	if(ctx.map_file_list.size() <= next_level || ctx.map_file_list[next_level].empty()) {
		return false;
	}

	return true;
}

static void _change_map()
{
	if(ctx.map_file_path.empty()) return;
	ctx.scene->uninit();
	map_load(ctx.map_file_path.c_str());
	ctx.scene->init();
	ctx.map_file_path = "";

	ui_reset();
	ui_fade_in(0.5f);
}

void game_change_mode(GameMode mode)
{
	global.game_mode = mode;
	global.splitscreen = (global.game_mode == GameMode::Versus || global.game_mode == GameMode::Coop);
	game_uninit();
	game_init();
	reset_sensitivities();
}