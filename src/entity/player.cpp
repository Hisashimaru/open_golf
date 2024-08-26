#include "app.h"
#include "input.h"
#include "renderer.h"
#include "sound.h"
#include "resource_manager.h"
#include "player.h"
#include "ball.h"
#include "../common.h"
#include "../foliage_system.h"
#include "../hud.h"

static const int SIMCOUNT = 60;
static const float SHOT_DELAY = 0.2f;

static const float max_speed = 15.0f;

static vec3 debug_pos;

// footsteps
static SoundRef footstep_sound;
static float next_footstep_time_shared;

static SoundRef shot_sound;
static SoundRef putter_sound;
static SoundRef chopping_sound;


void Player::init()
{
	model = load_model("player");
	model->play("walk", true);
	if(player_index == 1) {	// player 2
		model->materials[0] = load_material("player2");
	}

	camera = renderer_create_camera();
	camera->fov = 80;
	hud_change_club((ClubType)current_club, player_index);

	if(footstep_sound == nullptr){
		footstep_sound = sound_load("data/sounds/footsteps_grass.wav");
		footstep_sound->set_volume(0.5f);

		shot_sound = sound_load("data/sounds/ball_hit_iron.wav");
		putter_sound = sound_load("data/sounds/ball_hit_putter.wav");
		chopping_sound = sound_load("data/sounds/chop_wood.wav");
	}
}

void Player::update()
{
	static bool mouse_lock = true;
	input_mouse_lock(mouse_lock);

	const float speed = 2.0f;
	vec3 forward = quat::euler(0, camera_angle.y, 0).forward();
	vec3 right = quat::euler(0, camera_angle.y, 0).right();
	// flymode
	// vec3 forward = rotation.forward();
	//vec3 right = rotation.forward();
	vec2 mv = input_axis("left", "right", "forward", "back", player_index);
	vec3 move = vec3();
	move += rotation.forward() * mv.y;
	move -= rotation.right() * mv.x;
	if(move.len() > 1.0f)
	{
		move = move.normalized();
	}

	// footstep sound
	if(move.len() > 0.5f && next_footstep_time <= time_now())
	{
		footstep_sound->emit();
		next_footstep_time = time_now() + 0.3f;
	}

	
	if(input_hit("mouse_sensitivity"))
	{
		printf("mouse\n");
		input_set_key_state(KEY_MOUSE2, 0);
		increase_mouse_sensitivity();
	}
	if(input_hit("pad_sensitivity"))
	{
		increase_pad_sensitivity(player_index);
	}

	// camera rotation
	vec2 mouse = input_axis("cam_right", "cam_left", "cam_up", "cam_down", player_index);
	if(player_index == 0 && input_get_active_device() == INPUT_DEVICE_KEYBOARD)
	{
		camera_angle += vec3(mouse.y, -mouse.x, 0) * 0.1f * get_mouse_sensitivity();
	}
	else
	{
		camera_angle += vec3(mouse.y, -mouse.x, 0) * 2.0f * get_pad_sensitivity(player_index);
	}
	camera_angle.x = clamp(camera_angle.x, -89, 89);
	quat cam_rotation = quat::euler(camera_angle);

	// camera viewport size
	vec2 screen_size = app_get_size();
	if(global.game_mode == GameMode::Single)
	{
		camera->viewport = {0, 0, screen_size.x, screen_size.y};
	}
	else
	{
		if(player_index == 0)
			camera->viewport = {0, 0, screen_size.x*0.5f, screen_size.y};
		else
			camera->viewport = {screen_size.x*0.5f, 0, screen_size.x*0.5f, screen_size.y};
	}


	// character movement
	const float player_radius = 1.0f;
	const vec3 player_offset = vec3(0, player_radius, 0);
	const vec3 player_eye = vec3(0, 1.8f, 0);
	bool last_frame_on_ground = on_ground;
	RayHit s_hitinfo;
	on_ground = spherecast(ray_t(position + player_offset, vec3(0, -0.01f, 0)), player_radius, &s_hitinfo);
	//on_ground = true;

	// landed
	if(!last_frame_on_ground && on_ground)
	{
		// cancel gravity velocity
		vec3 ground_normal = s_hitinfo.normal;
		velocity = vec3::project_on_plane(velocity, ground_normal);
	}

	if(on_ground)
	{
		velocity += move * speed;
		if(velocity.len() > max_speed) {
			velocity = velocity.normalized() * max_speed;
		}
		velocity -= velocity * 8.0f * time_dt();
	}
	else
	{
		// gravity
		velocity += vec3(0, -20.0f, 0) * time_dt();
	}

	//// collision check
	//if(spherecast(ray_t(position, velocity * time_dt()), player_radius, &s_hitinfo))
	//{
	//	// sliding
	//	position = position + velocity.normalized() * s_hitinfo.distance;
	//}
	//else
	//{
	//	position += velocity * time_dt();
	//}
	position = spherecast_slide(position + player_offset, velocity * time_dt(), player_radius) - player_offset;
	rotation = quat(0, camera_angle.y, 0);

	//if(velocity.y < 0.45f)
	//{
	//	printf("%f, %f, %f\n", velocity.x, velocity.y, velocity.z);
	//}

	camera->position = position + player_eye;
	camera->rotation = cam_rotation;


	// change golf club
	if(next_club_change_time <= time_now())
	{
		if(input_hit("prev_weapon", player_index))
		{
			current_club--;
			if(current_club < 0) current_club = MAX_GOLF_CLUBS-1;
			hud_change_club((ClubType)current_club, player_index);
			next_club_change_time = time_now() + 0.1f;
		}
		if(input_hit("next_weapon", player_index))
		{
			current_club++;
			if(current_club >= MAX_GOLF_CLUBS) current_club = 0;
			hud_change_club((ClubType)current_club, player_index);
			next_club_change_time = time_now() + 0.1f;
		}
	}



	// shot
	bool tool_action = false;
	BallRef ball = nullptr;
	for(int i=0; i<g_balls.size(); i++)
	{
		BallRef b = g_balls[i];
		if(b->team_id != team_id) continue;

		vec3 ball_vec = b->position - camera->position;
		ball_vec.y = 0.0f;
		float distance = ball_vec.len();
		ball_vec.normalized();
		float angle = vec3::angle(ball_vec, quat::euler(0.0f, camera_angle.y, 0.0f).forward());
		float camera_ball_angle = vec3::angle((b->position - camera->position).normalized(), camera->rotation.forward());

		if(distance < 5.0f && camera_ball_angle < 10)
		{
			ball_founded = true;
		}

		if(distance < 5.0f && angle <= 70.0f)
		{
			ball = b;
		}
	}

	// click to shot mode
	if(ball)
	{
		if(input_hit("action", player_index))
		{
			line_data.points.clear();
			shot_mode = true;
			shot_time = -SHOT_DELAY;
		}
	}
	else
	{
		shot_mode = false;
	}


	if(shot_mode)
	{
		if(shot_time >= 0.0f)
		{
			if(model->animator.get_current_animation_name() != "shot_ready")
			{
				model->play("shot_ready");
			}
			vec3 dir = quat::euler(vec3(0, camera_angle.y, 0)) * vec3(0,0,1);
			vec3 right = vec3::cross(dir, vec3(0,1,0));
			dir = quat::axis_angle(right, golf_clubs[current_club].angle) * dir;

			// ground slope
			if(golf_clubs[current_club].angle == 0.0f)
			{
				RayHit ground_hit_info;
				vec3 ground_normal(0,1,0);
				if(raycast(ray_t(ball->position, vec3(0,-1,0)), &ground_hit_info))
				{
					ground_normal = ground_hit_info.normal;
					dir = vec3::project_on_plane(dir, ground_normal).normalized();
				}
			}

			float min_shot_power = shot_time >= 1.0f ? 8.0f : 0.0f;
			if(golf_clubs[current_club].angle == 0.0f)
				min_shot_power = shot_time >= 1.0f ? 1.0f : 0.0f;

			shot_velocity = dir * lerp(min_shot_power, golf_clubs[current_club].power, (sinf(shot_time*2.5f - PI*0.5f) * 0.5f) + 0.5f);
			vec3 vel = shot_velocity;
			vec3 pos = ball->position;
			line_data.points.clear();
			LinePoint p;
			p.position = pos;
			p.scale = 1.0f;
			line_data.points.push_back(p);
			for(int i=0; i<SIMCOUNT; i++)
			{
				if(golf_clubs[current_club].angle == 0.0f)
				{
					// putter club simulates ball friction
					vel = vel - vel * 1.8f * 0.033f;
					pos += vel * 0.033f;

					// shot line over the ground
					RayHit ground_hit;
					if(raycast(ray_t(pos + vec3(0,1,0), vec3(0,-1,0)), &ground_hit))
					{
						pos = ground_hit.point + vec3(0, 0.05f, 0);
					}
				}
				else
				{
					// other clubs simulates gravity
					vel += vec3(0, -20, 0) * 0.033f;
					pos += vel * 0.033f;
				}

				p.position = pos;
				line_data.points.push_back(p);
			}
		}

		if(shot_time >= 0.0f)
		{
			hud_hold_action(0, player_index);
		}
		else
		{
			// draw hold ring
			hud_hold_action((-SHOT_DELAY-shot_time)/-SHOT_DELAY, player_index);
		}

		if(input_up("action", player_index))
		{
			if(shot_time >= 0.0f)
			{
				model->play("shot");
				if(golf_clubs[current_club].angle == 0.0f)
				{
					// putter
					putter_sound->emit();
					ball->velocity = shot_velocity;
				}
				else
				{
					// shot
					shot_sound->emit();
					ball->velocity = rand_cone(shot_velocity, golf_clubs[current_club].inaccurate);
					camera->shake(0.15f, 10.0f, 0.0f, 0.1f, 0.1f);
				}

				if(player_index == 0){ 
					global.result.p1.shots++;
				} else {
					if(global.game_mode == GameMode::Coop) {
						global.result.p1.shots++;
					} else {
						global.result.p2.shots++;
					}
				}
			}
			else
			{
				tool_action = true;
			}
			shot_mode = false;
		}

		shot_time += time_dt();
	}
	else
	{
		// reset shot_ready pose
		if(model->animator.get_current_animation_name() == "shot_ready")
		{
			model->play("idle", true);
		}

		hud_hold_action(0, player_index);
	}


	if(shot_mode == false && input_hit("action", player_index))
	{
		tool_action = true;
	}
	

	if(tool_action)
	{
		bool mowing = true;
		vec3 camray = cam_rotation.forward() * 3.0f;
		RayHit hitinfo;
		ray_t ray(camera->position, camray);

		if(spherecast(ray, 0.5f, &hitinfo))
		{
			debug_pos = ray.pos + ray.dir.normalized() * hitinfo.distance;
			//debug_pos += hitinfo.normal;
			//debug_pos = hitinfo.point;
			//printf("(%f, %f, %f)\n", hitinfo.normal.x, hitinfo.normal.y, hitinfo.normal.z);
			if(hitinfo.collider->user_data.type == (int)ColliderUserDataType::FoliageObject)
			{
				foliage_take_damage((u32)((u64)hitinfo.collider->user_data.data), forward, 1.0f);
				mowing = false;
			}
			else if(hitinfo.collider->user_data.type == (int)ColliderUserDataType::Entity)
			{
				EntityRef ent = get_entity((u32)(u64)hitinfo.collider->user_data.data);
				if(ent)
				{
					chopping_sound->emit();
					ent->take_damage(1.0f);
				}
			}
		}


		if(mowing)
		{
			// mowing the grasses
			vec3 forward = quat::euler(vec3(0, camera_angle.y, 0)) * vec3(0,0,1);
			foliage_mowing(position + forward, 2.0f);
		}
	}


	// animations
	if(model->animator.get_current_animation_name() == "shot")
	{
		if(!model->animator.is_playing())
		{
			model->play("idle", true);
		}
	}

	// walk animation
	if(model->animator.get_current_animation_name() != "shot_ready" && model->animator.get_current_animation_name() != "shot")
	{
		if(velocity.len() >= 1.0f) 
		{
			if(model->animator.get_current_animation_name() != "walk")
			{
				model->play("walk", true);
			}
		}
		else
		{
			if(model->animator.get_current_animation_name() != "idle")
			{
				model->play("idle", true);
			}
		}
	}
}

void Player::draw()
{
	if(renderer_get_current_camera() == camera)
	{
		if(shot_mode)
		{
			line_data.thickness = 0.05f;
			draw_line(line_data, vec4(1,1,1,0.5f));
		}
	}
	else
	{
		// skip rendering self player model
		draw_model(model, mat4(position, rotation, vec3(1,1,1)));
	}

	static ModelRef sphere_model = create_model(create_sphere_mesh(0.5f), create_material("unlit"));
	//draw_model(sphere_model, mat4(debug_pos, quat::identity(), vec3(1,1,1)));
}