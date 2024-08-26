#pragma once
#include <memory>
#include "external/miniaudio.h"
#include "mathf.h"

struct SoundAttenuation
{
	float min_distance;
	float max_distance;
	float rolloff_factor;
};

const SoundAttenuation sound_attenuation_faint = {5.0f, FLOAT_MAX, 2.0f};
const SoundAttenuation sound_attenuation_normal = {6.0f, FLOAT_MAX, 1.8f};
const SoundAttenuation sound_attenuation_loud = {15.0f, FLOAT_MAX, 0.8f};
const SoundAttenuation sound_attenuation_loudest = {200.0f, FLOAT_MAX, 0.5f};

struct Sound {
	Sound() : sound({}), onetime_flag(false){}
	~Sound(){unload();}
	bool load(const char *filename);
	void unload();

	void play();
	void play(const vec3 &position);
	std::shared_ptr<Sound> emit();
	std::shared_ptr<Sound> emit(float volume);
	std::shared_ptr<Sound> emit(const vec3 &position);
	void set_volume(float volume);
	void set_attenuation(const SoundAttenuation &attenuation);

	bool is_playing();
	bool is_onetime(){return onetime_flag;}

	ma_sound sound = {};
	bool onetime_flag = false;
	float volume = 1.0f;
};
typedef std::shared_ptr<Sound> SoundRef;

void sound_init();
void sound_uninit();
void sound_update();

// listener
void sound_listener_set_position(const vec3 &position);
vec3 sound_listener_get_position();
void sound_listener_set_rotation(const quat &rotation);
quat sound_listener_get_rotation();

// sounds
SoundRef sound_load(const char *filepath);