#include "animation.h"
#include "app.h"


int AnimationClip::get_frame_size()
{
	int max_frame=0;
	for(auto it = animation_keys.begin(); it != animation_keys.end(); it++)
	{
		if(it->second->keys.size() > max_frame)
		{
			max_frame = (int)it->second->keys.size();
		}
	}

	return max_frame;
}


void Animator::update()
{
	if(current_animation == nullptr) return;
	if(!playing) return;

	elapsed += time_dt();
	int f = (int)(current_animation->frame_count * (elapsed/current_animation->get_length()));
	frame = f;

	if(frame >= current_animation->get_frame_size())
	{
		if(loop)
		{
			frame = 0;
			elapsed = 0;
		}
		else
		{
			frame = current_animation->get_frame_size()-1;
			playing = false;
		}
	}
}

void Animator::play(const char *name, bool loop)
{
	auto it = animations.find(std::string(name));
	if(it != animations.end())
	{
		current_animation_name = name;
		current_animation = it->second;
		this->loop = loop;
		elapsed = 0;
		frame = 0;
		playing = true;
	}
}

std::string Animator::get_current_animation_name()
{
	return current_animation_name;
}