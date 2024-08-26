#pragma once

#include "gpu.h"

enum class ClubType {
	Wood,
	Iron,
	Wedge,
	Putter,
	MaxTypes
};

struct GolfClub
{
	char name[32];
	ClubType type;
	TextureRef image;
	float power;
	float angle;
	float inaccurate;
};

enum class ColliderUserDataType
{
	Entity,
	FoliageObject,
};

enum class GameMode
{
	Single,
	Versus,
	Coop,
	Edit,
};

#define MAX_GOLF_CLUBS 4
extern GolfClub golf_clubs[MAX_GOLF_CLUBS];

struct GameResult
{
	bool count_time;
	float time;
	int shots;
	int score;

	float total_time;
	int total_shots;
	int total_score;
};

struct Global
{
	bool splitscreen;
	GameMode game_mode;
	
	struct {
		GameResult p1;
		GameResult p2;
	} result;

	// Test
	int mouse_sensitivity_idx;
	int pad_sensitivity_idx[2];
};
extern Global global;

void init_common();

void increase_mouse_sensitivity();
void increase_pad_sensitivity(int player_index);
float get_mouse_sensitivity();
float get_pad_sensitivity(int player_index);
void reset_sensitivities();