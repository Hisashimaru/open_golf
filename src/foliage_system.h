#pragma once

#include <stdio.h>
#include <vector>
#include "mathf.h"
#include "model.h"
#include "collision.h"

enum class FoliageType
{
	none = 0,
	grass1,
	bush1 = 20,
	tree1 = 40,
	max_types = 64
};

struct FoliageColliderDesc{
	FoliageColliderDesc(){}
	ColliderType type;
	vec3 center;
	union {
		struct Box{vec3 size;} box;
		struct Sphere{float radius;} sphere;
		struct Capsule{vec3 dir; float radius; float height;} capsule;
	};
};

struct FoliageDesc
{
	FoliageDesc(){}
	FoliageType type;
	char name[32];
	ModelRef model;
	float radius;
	float space;
	bool grass_object;
	float health;

	// collider
	FoliageColliderDesc collider;
};

void foliage_init();
void foliage_uninit();
void foliage_clear();
void foliage_add(FoliageType type, const vec3 &position);
void foliage_add(FoliageType type, const vec3 &position, float radius, float spacing_factor=1.0f);
void foliage_add_replace(FoliageType type, const vec3 &position, float radius);
void foliage_replace(FoliageType type, const vec3 &position);
void foliage_remove(const vec3 &position, float radius);
void foliage_take_damage(u32 id, vec3 dir, float damage);
void foliage_mowing(const vec3 &position, float radius);
void foliage_draw();
void foliage_save(FILE *fp);
void foliage_load(FILE *fp);
const FoliageDesc* foliage_get_descs();