#include "tree_log.h"
#include "app.h"
#include "collision.h"
#include "particle.h"
#include "sound.h"
#include "../common.h"

static ParticleEmitterRef break_particle;
static ParticleEmitterRef damage_particle;
static SoundRef break_sound;
static SoundRef drop_sound;

void TreeLog::init()
{
	model = load_model("pinetree_log");
	rotation = quat::axis_angle(rotation.right(), 45.0f);
	collider = create_capsule_collider(vec3(), vec3(0,0.5f,0), 0.4f, 0.4f);
	collider->user_data.data = (void*)(u64)id;
	collider->user_data.type = (u32)ColliderUserDataType::Entity;

	if(break_particle == nullptr)
	{
		damage_particle = load_particle("data/particles/woodchips.fx");
		break_particle = load_particle("data/particles/wood.fx");
		break_sound = sound_load("data/sounds/break_wood2.wav");
		drop_sound = sound_load("data/sounds/drop_log.wav");
	}

	health = 2.0f;
}

void TreeLog::update()
{
	if(grounded) return;

	velocity += vec3(0,-10,0) * time_dt();
	set_position(position + velocity * time_dt());

	RayHit hit_infos[8];
	int hits = raycast_all(ray_t(position, vec3(0, -0.4f, 0)), hit_infos, 8);
	for(int i=0; i<hits; i++)
	{
		RayHit hit = hit_infos[i];
		position = hit.point + vec3(0, 0.4f, 0);
		velocity = vec3();
		quat rot = rotation * quat::from_to(rotation.up(), vec3::project_on_plane(rotation.up(), hit.normal).normalized());
		set_rotation(rot);

		drop_sound->emit();
		grounded = true;
		break;
	}
}

void TreeLog::take_damage(float value)
{
	health -= value;
	damage_particle->emit(position, quat::identity());
	if(health <= 0)
	{
		break_particle->emit(position, quat::identity());
		break_sound->emit(0.6f);
		destroy();
	}
}