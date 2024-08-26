#pragma once

#include "entity.h"
#include "mathf.h"
#include "renderer.h"


class Player : public Entity
{
public:
	Player(int player_index) : player_index(player_index){}
	void init() override;
	void update() override;
	void draw() override;
	void collide_and_slide();

	int player_index = 0;
	int team_id = 0;
	vec3 velocity;
	bool on_ground = false;
	vec3 camera_angle;
	CameraRef camera;

	float next_club_change_time;
	int current_club = 0;

	// shot
	LineData line_data;
	float shot_time = 0.0f;
	vec3 shot_velocity = vec3();
	float shot_power = 40.0f;
	bool shot_mode = false;

	bool ball_founded = false;

	float next_footstep_time = 0.0f;
};
typedef std::shared_ptr<Player> PlayerRef;