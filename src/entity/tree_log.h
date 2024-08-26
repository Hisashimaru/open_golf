#pragma once

#include "entity.h"

class TreeLog : public Entity
{
public:
	void init() override;
	void update() override;

	void take_damage(float value) override;

	vec3 velocity = vec3();
	vec3 angular_velocity = vec3();
	bool grounded = false;
};