#pragma once

#include "entity.h"

class Tree : public Entity
{
public:
	
	void init() override;
	void update() override;

	void take_damage(float value) override;

	vec3 angular_velocity = vec3();
};