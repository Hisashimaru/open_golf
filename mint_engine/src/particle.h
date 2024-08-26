#pragma once
#include <vector>
#include <memory>
#include "mathf.h"
#include "material.h"


struct Particle
{
	vec3 position;
	vec3 velocity;
	vec4 color;
	float rotation;
	float rotation_speed;
	float size;
	float life_time;
};

enum EmitterShape
{
	EMITTER_SHAPE_POINT,
	EMITTER_SHAPE_SPHERE,
	EMITTER_SHAPE_HEMISPHERE,
	EMITTER_SHAPE_CONE,
};

struct EmitterDesc
{
	int max_particles;

	// emit params
	vec2 start_size;			// min max
	vec2 start_speed;			// min max
	vec2 start_rotation;		// min max
	vec2 start_rotation_speed;	// min max
	vec2 life_time;				// min max
	vec4 color;
	vec3 gravity;

	//float emission_interval;	// second per emit
	float emission_rate;		// emit per second
	int emission_count;

	// shape
	struct {
		EmitterShape type;
		float radius;
		float angle;
	} shape;
};

struct ParticleEmitter
{
	ParticleEmitter() : emitter_desc(){};
	ParticleEmitter(const EmitterDesc &desc) : emitter_desc(desc){};
	void emit(int count=0);
	void emit(const vec3 &position, const quat &rotation, int count=0);
	void emit(const Particle &desc);
	void update();
	void draw();
	bool load(const char *filename);

	EmitterDesc emitter_desc;
	vec3 position = vec3();
	quat rotation = quat();
	bool is_active = true;
	float elapsed_time = 0;
	MaterialRef material; // custom particle material
	std::vector<Particle> particles;
};
typedef std::shared_ptr<ParticleEmitter> ParticleEmitterRef;


void particle_init();
void particle_uninit();
void particle_update();
void particle_draw();
ParticleEmitterRef particle_emitter_load(const char *filename);
ParticleEmitterRef particle_emitter_create(const EmitterDesc &desc);

// resource manager
ParticleEmitterRef load_particle(const char *name);