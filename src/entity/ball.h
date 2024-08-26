#pragma once

#include "entity.h"


class Ball : public Entity
{
public:
	
	void init() override;
	void update() override;
	void set_color(int c); // 0:white 1:red 2:blue

	vec3 velocity = vec3(0,0,0);
	bool grounded = false;
	int team_id = 0;

	bool active = true;
};
typedef std::shared_ptr<Ball> BallRef;
extern std::vector<BallRef> g_balls;
