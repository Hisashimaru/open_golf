#include <vector>
#include "sound.h"

static struct sound_ctx
{
	std::vector<SoundRef> sounds;
	ma_engine engine;
	ma_sound_group group;
} ctx = {};


void sound_init()
{
	ma_engine_config config = ma_engine_config_init();
	config.listenerCount = 1;
	ma_engine_init(&config, &ctx.engine);
}

void sound_uninit()
{
	for(int i=0; i<ctx.sounds.size(); i++)
	{
		ctx.sounds[i]->unload();
	}
	ctx.sounds.clear();

	ma_engine_uninit(&ctx.engine);
}

void sound_update()
{
	for(int i=0; i<ctx.sounds.size(); i++)
	{
		SoundRef s = ctx.sounds[i];
		if(s->is_onetime() && !s->is_playing())
		{
			ctx.sounds[i] = ctx.sounds.back();
			ctx.sounds.pop_back();
			i--;
		}
	}
}

SoundRef sound_load(const char *filename)
{
	SoundRef sound = std::make_shared<Sound>();
	sound->load(filename);
	ctx.sounds.push_back(sound);
	return sound;
}




void sound_listener_set_position(const vec3 &position)
{
	ma_engine_listener_set_position(&ctx.engine, 0, position.x, position.y, position.z);
}

vec3 sound_listener_get_position()
{
	ma_vec3f pos = ma_engine_listener_get_position(&ctx.engine, 0);
	return vec3(pos.x, pos.y, pos.z);
}

void sound_listener_set_rotation(const quat &rotation)
{
	vec3 dir = rotation.forward();
	ma_engine_listener_set_direction(&ctx.engine, 0, dir.x, dir.y, dir.z);
}

quat sound_listener_get_rotation()
{
	ma_vec3f dir = ma_engine_listener_get_direction(&ctx.engine, 0);
	return quat::look(vec3(dir.x, dir.y, dir.z), vec3(0,1,0));
}




bool Sound::load(const char *filename)
{
	ma_result res = ma_sound_init_from_file(&ctx.engine, filename, MA_SOUND_FLAG_DECODE, nullptr, nullptr, &sound);
	if(res == MA_ERROR)
	{
		return false;
	}

	set_attenuation(sound_attenuation_normal);
	return true;
}

void Sound::unload()
{
	ma_sound_uninit(&sound);
	sound = ma_sound{};
}

void Sound::play()
{
	ma_sound_set_spatialization_enabled(&sound, false);
	ma_sound_seek_to_pcm_frame(&sound, 0);
	ma_sound_start(&sound);
}

void Sound::play(const vec3 &position)
{
	ma_sound_set_spatialization_enabled(&sound, true);
	ma_sound_set_position(&sound, position.x, position.y, position.z);
	ma_sound_seek_to_pcm_frame(&sound, 0);
	ma_sound_start(&sound);
}

SoundRef Sound::emit()
{
	return emit(volume);
}

SoundRef Sound::emit(float volume)
{
	SoundRef dest = std::make_shared<Sound>();
	ctx.sounds.push_back(dest);
	ma_sound_init_copy(&ctx.engine, &sound, MA_SOUND_FLAG_DECODE, nullptr, &dest->sound);

	dest->play();
	dest->onetime_flag = true;
	dest->volume = volume;
	dest->set_volume(volume);
	return dest;
}

SoundRef Sound::emit(const vec3 &position)
{
	SoundRef dest = std::make_shared<Sound>();
	ctx.sounds.push_back(dest);
	ma_sound_init_copy(&ctx.engine, &sound, MA_SOUND_FLAG_DECODE, nullptr, &dest->sound);

	dest->play(position);
	dest->onetime_flag = true;
	return dest;
}

void Sound::set_volume(float volume)
{
	this->volume = volume;
	ma_sound_set_volume(&sound, volume);
}

void Sound::set_attenuation(const SoundAttenuation &attenuation)
{
	//ma_sound_set_attenuation_model(s, ma_attenuation_model_exponential);
	ma_sound_set_min_distance(&sound, attenuation.min_distance);
	ma_sound_set_max_distance(&sound, attenuation.max_distance);
	ma_sound_set_rolloff(&sound, attenuation.rolloff_factor);
}

bool Sound::is_playing()
{
	return ma_sound_is_playing(&sound);
}