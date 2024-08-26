#include <vector>
#include "renderer.h"
#include "app.h"
#include "collision.h"
#include "sound.h"
#include "entity.h"
#include "ball.h"
#include "../common.h"
#include "../game.h"
#include "../map.h"


static const float BALL_RADIUS = 0.05f;
static SoundRef holein_sound;
static SoundRef hit_wood_sound;
static SoundRef hit_ground_sound;

std::vector<BallRef> g_balls;

void Ball::init()
{
	model = load_model("ball");

	if(holein_sound == nullptr)
	{
		holein_sound = sound_load("data/sounds/hole_in.wav");
		hit_wood_sound = sound_load("data/sounds/ball_hit_wood.wav");
		hit_ground_sound = sound_load("data/sounds/ball_hit_ground.wav");
	}
}

void Ball::update()
{
	if(active == false) return;

	bool on_ground = false;
	bool bounced = false;
	// chech the ground
	if(spherecast(ray_t(position, vec3(0,-0.01f,0)), BALL_RADIUS, nullptr))
	{
		on_ground = true;
	}

	// apply gravity
	vec3 gravity = vec3(0, -20, 0) * time_dt();
	if(!on_ground)
	{
		velocity += gravity;
	}

	RayHit hitinfo;
	if(spherecast(ray_t(position, velocity*time_dt()), BALL_RADIUS, &hitinfo))
	{
		position = position + velocity.normalized() * hitinfo.distance + hitinfo.normal * 0.001f;
		vec3 v = velocity * time_dt();
		float approach_angle = vec3::angle(v.normalized(), hitinfo.normal);
		if(approach_angle >= 95.0f && v.len() > 0.08f)
		{
			// Bounce
			//vec3 distination = position + v;
			velocity = vec3::reflect(velocity, hitinfo.normal);
			velocity -= velocity * 0.4f;
			bounced = true;
		}
		else
		{
			// sliding
			velocity = vec3::project_on_plane(velocity, hitinfo.normal);
		}

		// hit sound
		if(approach_angle >= 95.0f && v.len() > 0.04f)
		{
			u32 type = hitinfo.collider->user_data.type;
			if(type == (u32)ColliderUserDataType::Entity || type == (u32)ColliderUserDataType::FoliageObject)
			{
				hit_wood_sound->emit(0.6f);
			}
			else
			{
				hit_ground_sound->emit(0.5f);
			}
		}
	}
	else
	{
		position += velocity * time_dt();
	}

	// damping
	if(on_ground && !bounced)
	{
		velocity -= velocity * 1.8f * time_dt();
	}

	// spinning
	vec3 axis = vec3::cross(velocity.normalized(), vec3(0,1,0));
	rotation = quat::axis_angle(axis, -(velocity.len() / (BALL_RADIUS*PI*2.0f) * 360.0f * time_dt())) * rotation;

	// check cupin
	vec3 hole_pos = map_get_hole_position();
	hole_pos.y = position.y;
	if((hole_pos - position).len() <= BALL_RADIUS * 2 && velocity.len() <= 3)
	{
		// sink the ball
		position = map_get_hole_position() + vec3(0, -0.2f, 0) + (position - hole_pos).normalized() * 0.05f;
		holein_sound->play();
		active = false;

		// stop timer
		if(team_id == 0)
		{
			global.result.p1.count_time = false;
		}
		else
		{
			global.result.p2.count_time = false;
		}

		// show result
		if(global.game_mode == GameMode::Single || global.game_mode == GameMode::Coop)
		{
			game_cupin();
		}
		else
		{
			if(global.result.p1.count_time == false && global.result.p2.count_time == false)
			{
				game_cupin();
			}
		}
	}
}

void Ball::set_color(int c)
{
	if(c == 1) {
		model = load_model("ball_red");
	} else if(c == 2) {
		model = load_model("ball_blue");
	} else {
		model = load_model("ball");
	}
}