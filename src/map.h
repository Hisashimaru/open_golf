#pragma once
#include "model.h"

struct TeeingArea
{
	vec3 position;
	float angle;
	float width;
};

struct MapData
{
	u8 version;
	vec3 hole_position;
	TeeingArea teeing_area;

	char map_name[64];

	// map model
	char model_name[64];
};

void map_init();
void map_uninit();
void map_load(const char *filename=nullptr);
void map_save(const char *filename);
const MapData* map_get_data();
void map_draw();
//void map_set_model(ModelRef model);
void map_load_model(const char *filename);
void map_set_hole(const vec3 &pos);
void map_set_teeing_area(const vec3 &pos, float width, float angle);
vec3 map_get_hole_position();