#include "tree.h"
#include "tree_log.h"
#include "app.h"
#include "particle.h"
#include "sound.h"

static ParticleEmitterRef damage_particle;
static ParticleEmitterRef break_particle;
static SoundRef break_sound;
static SoundRef leaves_sound;

void Tree::init()
{
	model = load_model("tree");
	if(break_particle == nullptr)
	{
		break_particle = load_particle("data/particles/pinetree.fx");
		damage_particle = load_particle("data/particles/woodchips.fx");

		break_sound = sound_load("data/sounds/tree_cracking.wav");
		leaves_sound = sound_load("data/sounds/leaves2.wav");
	}
	break_sound->emit();
}

void Tree::update()
{
	rotation *= quat::euler(angular_velocity * time_dt());
	angular_velocity += angular_velocity * 0.8f * time_dt();

	if(quat::angle(quat(), rotation) >= 45.0f)
	{
		vec3 up = rotation.up();
		for(int i=0; i<8; i++)
		{
			auto ent = add_entity<TreeLog>();
			ent->position = position + (up * 2.0f * (float)i) + up;
			ent->rotation = rotation;
			ent->velocity = vec3(0, -((2.0f * (float)i) * 0.7f), 0);
		}
		break_particle->emit(position + rotation.up()*10, quat::identity());
		leaves_sound->emit();
		destroy();
		return;
	}

	if(vec3::dot(vec3(0,1,0), rotation.up()) < 0.0f)
	{
		angular_velocity = vec3();
	}
}

void Tree::take_damage(float value)
{
	damage_particle->emit(position + rotation.up(), quat::identity());
}