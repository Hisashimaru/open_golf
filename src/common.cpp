#include "common.h"
#include "resource_manager.h"

GolfClub golf_clubs[MAX_GOLF_CLUBS];
Global global = {};

void init_common()
{
	// name, type, icon, power, angle, inaccurate
	golf_clubs[(int)ClubType::Wood] = {"Dirver", ClubType::Wood, load_texture("data/ui/club_wood.png"), 40.0f, 25.0f, 6};
	golf_clubs[(int)ClubType::Iron] = {"Iron", ClubType::Iron, load_texture("data/ui/club_iron.png"), 30.0f, 35.0f, 4};
	golf_clubs[(int)ClubType::Wedge] = {"Wedge", ClubType::Wedge, load_texture("data/ui/club_wedge.png"), 20.0f, 50.0f, 2};
	golf_clubs[(int)ClubType::Putter] = {"Putter", ClubType::Putter, load_texture("data/ui/club_putter.png"), 15.0f, 0.0f, 0};
}


#define MAX_SENSITIVITY_CYCLES 3
static float sensitivity[MAX_SENSITIVITY_CYCLES] = {1.0f, 1.25f, 0.75f};

void increase_mouse_sensitivity()
{
	global.mouse_sensitivity_idx++;
	if(global.mouse_sensitivity_idx >= MAX_SENSITIVITY_CYCLES)
	{
		global.mouse_sensitivity_idx = 0;
	}
}

void increase_pad_sensitivity(int player_index)
{
	global.pad_sensitivity_idx[player_index]++;
	if(global.pad_sensitivity_idx[player_index] >= MAX_SENSITIVITY_CYCLES)
	{
		global.pad_sensitivity_idx[player_index] = 0;
	}
}

float get_mouse_sensitivity(){return sensitivity[global.mouse_sensitivity_idx];}
float get_pad_sensitivity(int player_index){return sensitivity[global.pad_sensitivity_idx[player_index]];}

void reset_sensitivities()
{
	global.mouse_sensitivity_idx = 0;
	global.pad_sensitivity_idx[0] = 0;
	global.pad_sensitivity_idx[1] = 0;
}