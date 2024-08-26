#pragma once

#include "app.h"
#include "gpu.h"
#include "mathf.h"
#include "material.h"
#include "model.h"
#include "font.h"

// camera
struct Camera
{
	vec3 position;
	quat rotation;
	float fov;
	float near;
	float far;
	rect_t viewport;
	u32 clear_mode;
	vec4 clear_color;
	int priority;
	bool is_active;

	// camera shake
	float shake_amplitude;
	float shake_frequency;
	float shake_start_time;
	float shake_duration;
	float shake_in_time;
	float shake_out_time;

	Camera()
	{
		position = vec3();
		rotation = quat::identity();
		fov = 65.0f;
		near = 0.1f;
		far = 500.0f;
		vec2 size = app_get_size();
		viewport = rect_t(0, 0, size.x, size.y);
		clear_mode = CLEAR_BUFFER_COLOR | CLEAR_BUFFER_DEPTH;
		clear_color = vec4(0.4f, 0.4f, 0.8f, 1.0f);
		priority = 0;
		is_active = true;

		shake_amplitude = 0;
		shake_frequency = 0;
		shake_duration = 0;
		shake_in_time = 0;
		shake_out_time = 0;
		shake_start_time = 0;
	}

	bool operator<(const Camera &right) const {
		return priority < right.priority;
	}

	mat4 get_view_matrix() const;
	mat4 get_projection_matrix() const;
	ray_t get_ray(const vec2 &screen_position) const;
	void shake(float amplitude, float frequency, float duration, float in_time, float out_time);
	void get_shaked_transform(vec3 *position, quat *rotation);
};
typedef std::shared_ptr<Camera> CameraRef;


void renderer_init();
void renderer_draw(void(*draw_func)());
void renderer_commit(CameraRef camera);

// 2D
void draw_rect(const rect_t &rect, const vec4 &color);
void draw_ring(vec2 center, float start_angle, float angle, float inner_radius, float outer_radius, const vec4 &color=vec4(1,1,1,1));
void draw_texture(std::shared_ptr<Texture> texture, const vec2 &pos, const vec4 &color=vec4(1,1,1,1));
void draw_texture(std::shared_ptr<Texture> texture, const vec2 &pos, ShaderRef shader);
void draw_texture(std::shared_ptr<Texture> texture, const rect_t &src, const rect_t &dest, const vec4 &color=vec4(1,1,1,1));
void draw_texture(std::shared_ptr<Texture> texture, const rect_t &src, const rect_t &dest, ShaderRef shader);

void renderer_set_font(FontRef font);
FontRef renderer_get_font();
void draw_text(const vec2 &pos, float size, const char *str, ...);

struct Camera;
//void begin3d(const Camera *camera);
//void end3d();

CameraRef renderer_create_camera();
CameraRef renderer_get_current_camera();
void renderer_remove_camera(CameraRef camera);
void renderer_remove_cameras();

// rendering matrixies
mat4 renderer_get_model_matrix();
mat4 renderer_get_view_matrix();
mat4 renderer_get_projection_matrix();

// 3D
void draw_rect(const vec3 &pos, float width, float height, const vec4 &color);
void draw_mesh(MeshRef mesh, MaterialRef material, const mat4 &transform);
void draw_mesh(MeshRef mesh, MaterialRef material, u32 count, const mat4 *transforms);
void draw_mesh_lines(MeshRef mesh, MaterialRef material, const mat4 &transform);
void draw_model(ModelRef model, const mat4 &transform);
void draw_model(ModelRef model, u32 count, const mat4 *transforms);



// Line
struct LinePoint
{
	vec3 position = vec3();
	float scale = 1.0f;
};

struct LineData
{
	std::vector<LinePoint> points;
	float thickness = 0.1f;
};

// mesh line
void draw_line(const LineData &data, const vec4 &color=vec4(1,1,1,1));

// debug line
void draw_line(const vec3 &start, const vec3 &end);