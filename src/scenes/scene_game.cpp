#include "app.h"
#include "input.h"
#include "gpu.h"
#include "renderer.h"
#include "ui.h"
#include "resource_manager.h"
#include "scene_game.h"
#include "../map.h"
#include "../entity/entity.h"
#include "../entity/player.h"
#include "../entity/ball.h"
#include "../entity//tree_log.h"
#include "../common.h"
#include "imgui/imgui.h"

static PlayerRef player;
static PlayerRef player2;

void GameScene::init()
{
	const MapData *data = map_get_data();
	RayHit hitinfo;

	// Player 1
	player = add_entity<Player>(0);
	player->rotation = quat::euler(vec3(0, data->teeing_area.angle, 0));
	player->camera_angle.y = data->teeing_area.angle;
	vec3 pos = data->teeing_area.position - player->rotation.forward();
	if(raycast(ray_t(pos + vec3(0,1,0), vec3(0,-2,0)), &hitinfo))
	{
		player->position = hitinfo.point + vec3(0,0.1f,0);
	}
	else
	{
		player->position = pos + vec3(0,0.1f,0);
	}

	// Log
	//EntityRef ent = add_entity<TreeLog>();
	//ent->position = player->position + vec3(0, 5, 10);

	// Ball
	BallRef ball1 = add_entity<Ball>();
	g_balls.push_back(ball1);
	vec3 ball_pos = player->position + player->rotation.forward() * 5.0f;
	if(raycast(ray_t(ball_pos + vec3(0,1,0), vec3(0,-2,0)), &hitinfo))
	{
		ball1->position = hitinfo.point + vec3(0,0.05f,0);
	}
	else
	{
		ball1->position = player->position + vec3(0,0.05f,0);
	}

	vec2 screen_size = app_get_size();
	CameraRef cam1 = player->camera;
	cam1->viewport = {0, 0, screen_size.x, screen_size.y};

	// local multiplayer
	if(global.splitscreen)
	{
		cam1->viewport = {0, 0, screen_size.x*0.5f, screen_size.y};

		// Player2
		player2 = add_entity<Player>(1);
		player2->player_index = 1;
		player2->rotation = quat::euler(vec3(0, data->teeing_area.angle, 0));
		player2->camera_angle.y = data->teeing_area.angle;
		pos = player->position + player->rotation.right() * 3.0f;
		if(raycast(ray_t(pos + vec3(0,1,0), vec3(0,-2,0)), &hitinfo))
		{
			player2->position = hitinfo.point + vec3(0,0.1f,0);
		}
		else
		{
			player2->position = pos + vec3(0,0.1f,0);
		}

		CameraRef cam2 = player2->camera;
		cam2->position = vec3(0,10,0);
		cam2->viewport = {screen_size.x*0.5f, 0, screen_size.x*0.5f, screen_size.y};
		cam2->is_active = true;
		cam2->priority = 1;
		cam2->clear_mode = CLEAR_BUFFER_DEPTH;

		// create player2's ball
		if(global.game_mode == GameMode::Versus)
		{
			BallRef ball2 = add_entity<Ball>();
			g_balls.push_back(ball2);

			vec3 ball_pos = player2->position + player2->rotation.forward() * 5.0f;
			RayHit hitinfo;
			if(raycast(ray_t(ball_pos + vec3(0,1,0), vec3(0,-2,0)), &hitinfo))
			{
				ball2->position = hitinfo.point + vec3(0,0.05f,0);
			}
			else
			{
				ball2->position = player2->position + vec3(0,0.05f,0);
			}

			ball2->team_id = 1;
			ball2->set_color(2);	// player 2 ball color is blue
			player2->team_id = 1;

			ball1->set_color(1);	// player 1 ball color is red
		}
	}


	// init input actions
	input_unregister_all();
	input_register("forward", KEY_W);
	input_register("back", KEY_S);
	input_register("left", KEY_A);
	input_register("right", KEY_D);
	input_register("cam_up", KEY_MOUSE_MOVE_UP);
	input_register("cam_down", KEY_MOUSE_MOVE_DOWN);
	input_register("cam_left", KEY_MOUSE_MOVE_LEFT);
	input_register("cam_right", KEY_MOUSE_MOVE_RIGHT);
	input_register("action", KEY_MOUSE0);
	input_register("next_weapon", KEY_MOUSE_WHEEL_DOWN);
	input_register("prev_weapon", KEY_MOUSE_WHEEL_UP);
	input_register("mouse_sensitivity", KEY_MOUSE2);

	input_register("forward", KEY_PAD_LS_UP);
	input_register("back", KEY_PAD_LS_DOWN);
	input_register("left", KEY_PAD_LS_LEFT);
	input_register("right", KEY_PAD_LS_RIGHT);
	input_register("cam_up", KEY_PAD_RS_DOWN);
	input_register("cam_down", KEY_PAD_RS_UP);
	input_register("cam_left", KEY_PAD_RS_LEFT);
	input_register("cam_right", KEY_PAD_RS_RIGHT);
	input_register("action", KEY_PAD_RT);
	input_register("next_weapon", KEY_PAD_RB);
	input_register("prev_weapon", KEY_PAD_LB);
	input_register("pad_sensitivity", KEY_PAD_R3);

	if(global.splitscreen)
	{
		input_register("forward", KEY_PAD1_LS_UP, 1);
		input_register("back", KEY_PAD1_LS_DOWN, 1);
		input_register("left", KEY_PAD1_LS_LEFT, 1);
		input_register("right", KEY_PAD1_LS_RIGHT, 1);
		input_register("cam_up", KEY_PAD1_RS_DOWN, 1);
		input_register("cam_down", KEY_PAD1_RS_UP, 1);
		input_register("cam_left", KEY_PAD1_RS_LEFT, 1);
		input_register("cam_right", KEY_PAD1_RS_RIGHT, 1);
		input_register("action", KEY_PAD1_RT, 1);
		input_register("next_weapon", KEY_PAD1_RB, 1);
		input_register("prev_weapon", KEY_PAD1_LB, 1);
		input_register("pad_sensitivity", KEY_PAD_R3, 1);
	}

	// reset round result data
	global.result.p1.count_time = true;
	global.result.p1.time = 0;
	global.result.p1.shots = 0;
	global.result.p2.count_time = true;
	global.result.p2.time = 0;
	global.result.p2.shots = 0;
}

void GameScene::uninit()
{
	player = nullptr;
	player2 = nullptr;
	g_balls.clear();
	free_entities();
	map_uninit();
	renderer_remove_cameras();
}

void game_cupin();
void GameScene::update()
{
	if(input_hit(KEY_ESCAPE))
	{
		app_quit();
	}

	update_entities();

	if(global.result.p1.count_time){ global.result.p1.time += time_dt(); }
	if(global.result.p2.count_time){ global.result.p2.time += time_dt(); }

	// debug
	if(input_hit(KEY_ENTER))
	{
		game_cupin();
	}
}

void GameScene::draw()
{
	draw_entities();
	map_draw();
}