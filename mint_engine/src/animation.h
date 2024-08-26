#pragma once

#include <vector>
#include <unordered_map>
#include <memory>
#include "mathf.h"

struct AnimationKey
{
	std::vector<transform_t> keys;
};

struct AnimationClip
{
	int get_frame_size();
	float get_length(){return (float)frame_count/frame_rate;};

	int frame_rate=0;
	int frame_count=0;
	std::unordered_map<std::string, std::unique_ptr<AnimationKey>> animation_keys;
};
typedef std::shared_ptr<AnimationClip> AnimationClipRef;


struct Animator
{
	void update();
	void play(const char *name, bool loop);
	bool is_playing(){return playing;}
	std::string get_current_animation_name();

	std::unordered_map<std::string, AnimationClipRef> animations;
	AnimationClipRef current_animation;
	std::string current_animation_name;
	float elapsed = 0;
	int frame = 0;
	bool playing = false;
	bool loop = false;
};