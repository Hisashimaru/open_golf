#include <string>
#include <algorithm>
#include <stdarg.h>
#include "app.h"
#include "gpu.h"
#include "renderer.h"
#include "builtin_font.h"
#include "builtin_postfx_shaders.h"


#define SSAO_MAX_SAMPLES 64

enum class RenderingCommandType
{
	DrawMesh,
	DrawMeshInstance,
	DrawMeshLines,
	DrawModel,
	DrawModelInstance,
};

struct RenderingCommand
{
	RenderingCommandType type;

	Material *material;
	Mesh *mesh;
	Model *model;
	mat4 *transforms;
	mat4 transform;
	vec4 color;
	u32 count;

	bool operator<(const RenderingCommand& a) const {
		return material->queue < a.material->queue;
	}
};

static struct renderer_ctx
{
	std::shared_ptr<Shader> sprite_shader;
	std::shared_ptr<Mesh> sprite_mesh;
	MeshRef shape_mesh;
	std::shared_ptr<Texture> tex_white;
	MaterialRef default_material;
	RenderTargetRef main_render_target;

	std::vector<CameraRef> cameras;
	CameraRef current_camera;
	CameraRef default_camera;

	mat4 model_matrix;
	mat4 view_matrix;
	mat4 projection_matrix;

	std::vector<RenderingCommand> rendering_commands;

	struct{
		std::vector<MeshRef> meshes;
		int count;
		MaterialRef material;
	} line;

	struct {
		struct {
			float start;
			float end;
			ShaderRef shader;
			vec4 color;
		} fog;

		struct {
			float radius;
			float bias;
			ShaderRef shader;
			ShaderRef blur_shader;
		} ssao;

		RenderTargetRef buffer;
		RenderTargetRef output_buffer;
	} postfx;

	FontRef font;
	FontRef default_font;

	char bone_matrix_uniform_names[128][64];
} ctx{};


void renderer_init()
{
	// Sprite Shader
	static char *sprite_shader_vs_src =R"(
		#version 330

		layout(location = 0) in vec3 _position;
		layout(location = 2) in vec2 _texcoord;
		layout(location = 3) in vec4 _color;

		out vec2 o_texcoord;
		out vec4 o_color;

		void main()
		{
			gl_Position = vec4(_position, 1.0);
			o_texcoord = _texcoord;
			o_color = _color;
		}
		)";

	static char *sprite_shader_fs_src =R"(
		#version 330

		out vec4 frag_color;
		in vec2 o_texcoord;
		in vec4 o_color;

		uniform vec4 _color;
		uniform sampler2D _main_tex;

		void main()
		{
			frag_color = texture(_main_tex, o_texcoord) * _color;
		}
		)";

	static char *sprite_inst_shader_vs_src =R"(
		#version 330

		layout(location = 0) in vec3 _position;
		layout(location = 2) in vec2 _texcoord;
		layout(location = 3) in vec4 _color;
		layout(location = 4) in vec3 _center;
		layout(location = 5) in vec4 _size;	// (x, y, w, h)

		uniform mat4 _projection;

		out vec2 o_texcoord;
		out vec4 o_color;

		void main()
		{
			gl_Position = _projection * vec4(_position, 1.0);
			//gl_Position = vec4(_position, 1.0);
			o_texcoord = _texcoord;
			o_color = _color;
		}
		)";

	ctx.sprite_shader = gpu_create_shader(sprite_shader_vs_src, sprite_shader_fs_src);

	Vertex v[4]{};
	u16 indices[6] = {0, 1, 3, 1, 2, 3};
	ctx.sprite_mesh = gpu_create_mesh(v, 4, indices, 6);
	ctx.shape_mesh = gpu_create_mesh(v, 4, indices, 6);

	u8 data[4] = {255, 255, 255, 255};
	ctx.tex_white = gpu_create_texture(data, 1, 1);

	vec2 screen_size = app_get_size();
	ctx.main_render_target= gpu_create_render_target((int)screen_size.x, (int)screen_size.y);

	ctx.default_material = create_material("unlit");

	// line rendering
	ctx.line.material = create_material("unlit");
	ctx.line.material->render_mode = Material::TRANSPARENT;

	ctx.default_font = std::make_shared<Font>(builtin_font, sizeof(builtin_font));
	ctx.font = ctx.default_font;

	ctx.default_camera = renderer_create_camera();

	for(int i=0; i<128; i++)
	{
		sprintf(ctx.bone_matrix_uniform_names[i], "_bone_matrix[%d]", i);
	}


	// poset processing
	// fog
	ctx.postfx.fog.shader = gpu_create_shader(postfx_vs_src, postfx_fog_src);
	ctx.postfx.fog.start = 1.0f;
	ctx.postfx.fog.end = 150.0f;
	ctx.postfx.fog.color = vec4(0.7f, 0.7f, 1.0f, 1.0);


	// SSAO
	ctx.postfx.ssao.shader = gpu_create_shader(postfx_vs_src, postfx_ssao_src);
	ctx.postfx.ssao.radius = 0.5f;
	ctx.postfx.ssao.bias = 0.025f;
	vec3 samples[SSAO_MAX_SAMPLES];
	for(int i=0; i<SSAO_MAX_SAMPLES; i++)
	{
		float scale = (float)i/SSAO_MAX_SAMPLES;
		scale = lerp(0.1f, 1.0f, scale * scale);
		samples[i] = rand_in_sphere().normalized() * scale;
		ctx.postfx.ssao.shader->set_value(("_samples[" + std::to_string(i) + "]").c_str(), samples[i]);
	}
	// SSAO blur
	ctx.postfx.ssao.blur_shader = gpu_create_shader(postfx_vs_src, postfx_ssao_blur_src);

	ctx.postfx.buffer = gpu_create_render_target((int)screen_size.x, (int)screen_size.y);
	ctx.postfx.output_buffer = gpu_create_render_target((int)screen_size.x, (int)screen_size.y);
}

void draw_rect(const rect_t &rect, const vec4 &color)
{
	Vertex v[4]{};
	vec3 pixel_offset = vec3(0.0f, -0.5f, 0.0f);
	// top_left
	v[0].position = vec3(rect.x, rect.y,  0) + pixel_offset;	
	// bottom_left
	v[1].position = vec3(rect.x, rect.y+rect.h,  0) + pixel_offset;
	// bottom_right
	v[2].position = vec3(rect.x+rect.w, rect.y+rect.h,  0) + pixel_offset;
	// top_right
	v[3].position = vec3(rect.x+rect.w,  rect.y,  0) + pixel_offset;
	v[0].color = v[1].color = v[2].color = v[3].color = color;

	vec2 viewport_size = app_get_size();
	mat4 proj = mat4::ortho(-0.5f, viewport_size.x-0.5f, viewport_size.y-0.5f, -0.5f, -1.0f, 1.0f);
	ctx.sprite_shader->set_value("_projection", proj);
	ctx.sprite_shader->set_value("_color", color);
	gpu_bind_texture(0, ctx.tex_white);

	gpu_enable_depth_test(false);
	gpu_set_blend_mode(BlendMode::Alpha);

	ctx.sprite_mesh->update(v, 4);
	ctx.sprite_mesh->draw(ctx.sprite_shader);

	gpu_enable_depth_test(true);
	gpu_set_blend_mode(BlendMode::None);

	gpu_bind_texture(0, nullptr);
}

void draw_ring(vec2 center, float start_angle, float angle, float inner_radius, float outer_radius, const vec4 &color)
{
	static std::vector<Vertex> vertices;
	static std::vector<u16> indices;

	int segments = 32;
	float segment_angle = angle / segments;
	for(int i=0; i<segments+1; i++)
	{
		Vertex v;
		v.color = vec4(1,1,1,1);
		float ang = (start_angle + segment_angle * i) * DEG2RAD;

		// inndier vertices;
		v.position = vec3(sinf(ang), -cosf(ang), 0) * inner_radius + vec3(center.x, center.y, 0);
		vertices.push_back(v);

		// outer vertices;
		v.position = vec3(sinf(ang), -cosf(ang), 0) * outer_radius + vec3(center.x, center.y, 0);
		vertices.push_back(v);
	}

	for(int i=0; i<segments; i++)
	{
		u16 base = (u16)(i*2);
		indices.push_back(base+1);
		indices.push_back(base);
		indices.push_back(base+3);
		indices.push_back(base);
		indices.push_back(base+2);
		indices.push_back(base+3);
	}

	vec2 viewport_size = app_get_size();
	mat4 proj = mat4::ortho(-0.5f, viewport_size.x-0.5f, viewport_size.y-0.5f, -0.5f, -1.0f, 1.0f);
	ctx.sprite_shader->set_value("_projection", proj);
	ctx.sprite_shader->set_value("_color", color);
	gpu_bind_texture(0, ctx.tex_white);

	gpu_enable_depth_test(false);
	gpu_set_blend_mode(BlendMode::Alpha);

	ctx.shape_mesh->update(vertices.data(), (u32)vertices.size(), indices.data(), (u32)indices.size());
	ctx.shape_mesh->draw(ctx.sprite_shader);

	gpu_enable_depth_test(true);
	gpu_set_blend_mode(BlendMode::None);

	gpu_bind_texture(0, nullptr);

	vertices.clear();
	indices.clear();
}


//void begin3d(const Camera *camera)
//{
//	ctx.active_camera = *camera;
//	ctx.view_matrix = mat4::lookat(camera->position, camera->position + camera->rotation.forward(), camera->rotation.up());
//	ctx.projection_matrix = mat4::perspective(camera->fov * DEG2RAD, camera->viewport.w / camera->viewport.h, camera->near, camera->far);
//	gpu_set_viewport(camera->viewport);
//}
//
//void end3d()
//{
//	vec2 size = app_get_size();
//	gpu_set_viewport(rect_t(0, 0, size.x, size.y));
//}

CameraRef renderer_create_camera()
{
	CameraRef cam = std::make_shared<Camera>();
	if(ctx.cameras.size() == 1 && ctx.cameras[0] == ctx.default_camera)
	{
		ctx.cameras.pop_back();
	}
	ctx.cameras.push_back(cam);
	return cam;
}

CameraRef renderer_get_current_camera()
{
	return ctx.current_camera;
}

void renderer_remove_camera(CameraRef camera)
{
	for(int i=0; i<ctx.cameras.size(); i++)
	{
		if(ctx.cameras[i] == camera)
		{
			ctx.cameras[i] = ctx.cameras.back();
			ctx.cameras.pop_back();
			if(ctx.cameras.size() == 0)
			{
				ctx.cameras.push_back(ctx.default_camera);
			}
			return;
		}
	}
}

void renderer_remove_cameras()
{
	ctx.cameras.clear();
	ctx.cameras.push_back(ctx.default_camera);
}



mat4 Camera::get_view_matrix() const
{
	return mat4::lookat(position, position + rotation.forward(), rotation.up());
}

mat4 Camera::get_projection_matrix() const
{
	return mat4::perspective(fov * DEG2RAD, viewport.w / viewport.h, near, far);
}

ray_t Camera::get_ray(const vec2 &screen_position) const
{
	// calculate normalized device coordinates
	vec3 p;
	p.x = (2.0f * screen_position.x) / viewport.w - 1.0f;
	p.y = 1.0f - (2.0f * screen_position.y) / viewport.h;
	p.z = 1.0f;

	mat4 view = mat4::lookat(position, position + rotation.forward(), rotation.up());

	mat4 projection = mat4::perspective(fov * DEG2RAD, viewport.w / viewport.h, near, far);

	// orthographic projection
	//float aspect = viewport_size.x / viewport_size.y;
	//float top = (ctx.camera.fov * DEG2RAD) * 0.5f;
	//float right = top * aspect;
	//proejction  = mat4::ortho(-right, right, -top, top, ctx.camera.near, ctx.camera.far);

	mat4 view_inv = view.inversed();
	mat4 projection_inv = projection.inversed();
	mat4 matinv = (view * projection).inversed();

	// unproject far/near points
	vec3 near_point = matinv.multiply_point(vec3(p.x, p.y, 0));
	vec3 far_point = matinv.multiply_point(vec3(p.x, p.y, 1));
	vec3 dir = (far_point - near_point).normalized();

	// orthographic projection
	//vec3 ray_origin = matinv.multiply_point(vec3(p.x, p.y, -1));

	ray_t ray(position, dir);
	return ray;
}

void Camera::shake(float amplitude, float frequency, float duration, float in_time, float out_time)
{
	shake_amplitude = amplitude;
	shake_frequency = frequency;
	shake_in_time = in_time;
	shake_out_time = out_time;
	shake_duration = duration;
	shake_start_time = 	time_now();
}

void Camera::get_shaked_transform(vec3 *position, quat *rotation)
{
	float k = 0;
	float elapsed = time_now() - shake_start_time;
	if(elapsed < shake_in_time)
	{
		float t = elapsed/shake_in_time;
		k = lerp(0, 1, t);
	}
	else if(elapsed < shake_in_time + shake_duration)
	{
		k = 1;
	}
	else if(elapsed < shake_in_time + shake_duration + shake_out_time)
	{
		elapsed = elapsed - (shake_in_time + shake_duration);
		float t = elapsed / shake_out_time;
		k = lerp(1, 0, t);
	}

	vec3 right = this->rotation.right() * cosf(time_now()*shake_frequency*1.1f)*shake_amplitude;
	vec3 up = vec3(0,1,0) * sinf(time_now()*shake_frequency)*shake_amplitude;
	*position = this->position + (right + up) * k;
	*rotation = this->rotation;
}

static float remap(float value, float inputMin, float inputMax, float outputMin, float outputMax)
{
	return (value - inputMin) * ((outputMax - outputMin) / (inputMax - inputMin)) + outputMin;
}

static float ease_in_quint( float t ) {
	float t2 = t * t;
	return t * t2 * t2;
}

static double ease_out_quint( float t ) {
	float t2 = (--t) * t;
	return 1 + t * t2 * t2;
}

mat4 renderer_get_model_matrix()
{
	return ctx.model_matrix;
}

mat4 renderer_get_view_matrix()
{
	return ctx.view_matrix;
}

mat4 renderer_get_projection_matrix()
{
	return ctx.projection_matrix;
}

void draw_texture(std::shared_ptr<Texture> texture, const vec2 &pos, const vec4 &color)
{
	if(!texture)
	{
		return;
	}

	float w = (float)texture->get_width();
	float h = (float)texture->get_height();
	draw_texture(texture, rect_t(0, 0, w, h), rect_t(pos.x, pos.y, w, h), color);
}

void draw_texture(std::shared_ptr<Texture> texture, const vec2 &pos, ShaderRef shader)
{
	if(!texture || !shader)
	{
		return;
	}

	float w = (float)texture->get_width();
	float h = (float)texture->get_height();
	draw_texture(texture, rect_t(0, 0, w, h), rect_t(pos.x, pos.y, w, h), shader);
}

void draw_texture(std::shared_ptr<Texture> texture, const rect_t &src, const rect_t &dest, const vec4 &color)
{
	if(!texture)
		return;

	ctx.sprite_shader->set_value("_color", color);
	draw_texture(texture, src, dest, ctx.sprite_shader);
}

void draw_texture(std::shared_ptr<Texture> texture, const rect_t &src, const rect_t &dest, ShaderRef shader)
{
	if(!texture || !shader)
	{
		return;
	}

	Vertex v[4]{};
	rect_t uv{};
	float tw = (float)texture->get_width();
	float th = (float)texture->get_height();
	uv.x = src.x / tw;
	uv.y = 1.0f - (src.y / th);
	uv.w = (src.x+src.w) /tw;
	uv.h = 1.0f - ((src.y+src.h) / th);

	vec2 viewport_size = app_get_size();
	float vw = viewport_size.x;
	float vh = viewport_size.y;

	// top_left
	v[0].position = vec3((dest.x/vw)*2-1, ((vh-dest.y)/vh)*2-1,  0);						v[0].uv = vec2(uv.x, uv.y);
	// bottom_left
	v[1].position = vec3((dest.x/vw)*2-1, ((vh-dest.y-dest.h)/vh)*2-1,  0);			v[1].uv = vec2(uv.x, uv.h);
	// bottom_right
	v[2].position = vec3(((dest.x+dest.w)/vw)*2-1, ((vh-dest.y-dest.h)/vh)*2-1,  0);	v[2].uv = vec2(uv.w, uv.h);
	// top_right
	v[3].position = vec3(((dest.x+dest.w)/vw)*2-1,  ((vh-dest.y)/vh)*2-1,  0);			v[3].uv = vec2(uv.w, uv.y);
	ctx.sprite_mesh->update(v, 4);

	gpu_bind_texture(0, texture);

	gpu_enable_depth_test(false);
	gpu_set_blend_mode(BlendMode::Alpha);
	ctx.sprite_mesh->draw(shader);
	gpu_enable_depth_test(true);
	gpu_set_blend_mode(BlendMode::None);

	gpu_bind_texture(0, nullptr);
}

void renderer_set_font(FontRef font)
{
	if(font)
	{
		ctx.font = font;
	}
	else
	{
		ctx.font = ctx.default_font;
	}
}

FontRef renderer_get_font()
{
	return ctx.font;
}

void draw_text(const vec2 &pos, float size, const char *str, ...)
{
	static std::string buffer;
	va_list arg_ptr;
	va_start(arg_ptr, str);
	//vsprintf(buffer, str, arg_ptr);
	int buff_size = vsnprintf(nullptr, 0, str, arg_ptr);
	buffer.resize(buff_size+1);
	vsnprintf(&buffer[0], buff_size+1, str, arg_ptr);
	va_end(arg_ptr);


	if(ctx.font == nullptr || ctx.font->get_glyph_count() == 0) return;
	float scale = size/ctx.font->get_size();
	float px = pos.x;
	float py = pos.y + ctx.font->get_ascent() * scale;
	//vec2 fsize = _font.texture->get_size();
	//ren_draw_texture_ex(_font.texture, Rect{ 0, 0, fsize.x, fsize.y }, Rect{ 0, 0, fsize.x, fsize.y });
	for(int i=0; str[i]!='\0'; i++)
	{
		if(str[i] == '\n')
		{
			py += ctx.font->get_line_space() * scale;
			px = pos.x;
			continue;
		}

		struct glyph_t g = ctx.font->get_glyph(str[i]);
		ctx.font->update_texture();
		draw_texture(ctx.font->texture, rect_t{(float)g.x, (float)g.y, (float)g.w, (float)g.h}, rect_t{(float)px + g.bearing_x * scale, (float)py + g.bearing_y * scale, (float)g.w*scale, (float)g.h*scale});
		px += g.advance*scale;
	}
}

static void draw_mesh_impl(Mesh *mesh, Material *material, const mat4 &transform)
{
	ctx.model_matrix = transform;
	MaterialShaderType type = (MaterialShaderType)((int)STATIC_OPAQUE + material->render_mode);

	if(material->render_mode == Material::TRANSPARENT) {
		gpu_set_blend_mode(BlendMode::Alpha);
	}
	else if(material->render_mode == Material::DEPTHE_MASK) {
		gpu_enable_colors(false);
		type = STATIC_OPAQUE;
	}
	material->apply(material->shaders[type]);
	gpu_enable_depth(material->zwrite);
	mesh->draw(material->shaders[type]);
	gpu_enable_depth(true);
	gpu_enable_colors(true);
	gpu_set_blend_mode(BlendMode::None);
}

static void draw_mesh_skinned_impl(Mesh *mesh, Material *material, const mat4 &transform)
{
	ctx.model_matrix = transform;
	MaterialShaderType type = (MaterialShaderType)((int)SKINNED_OPAQUE + material->render_mode);
	material->apply(material->shaders[type]);
	gpu_enable_depth(material->zwrite);
	mesh->draw(material->shaders[type]);
	gpu_enable_depth(true);
}

static void draw_mesh_instance_impl(Mesh *mesh, Material *material, u32 count, const mat4 *transforms)
{
	ctx.model_matrix = mat4::identity();
	MaterialShaderType type = (MaterialShaderType)((int)INSTANCING_OPAQUE + material->render_mode);
	material->apply(material->shaders[type]);
	gpu_enable_depth(material->zwrite);
	mesh->draw(material->shaders[type], count, transforms);
	gpu_enable_depth(true);
}

static void draw_mesh_lines_impl(Mesh *mesh, Material *material, const mat4 &transform)
{
	ctx.model_matrix = transform;
	MaterialShaderType type = (MaterialShaderType)((int)STATIC_OPAQUE + material->render_mode);
	material->apply(material->shaders[type]);
	gpu_enable_depth(material->zwrite);
	mesh->draw_lines(material->shaders[type]);
	gpu_enable_depth(true);
}

static void draw_model_impl(Model *model, const mat4 &transform)
{
	if(model->materials.size() > 0)
	{
		if(model->bones.size() > 0)
		{
			ctx.model_matrix = transform;
			MaterialRef material = model->materials[0];
			MaterialShaderType type = (MaterialShaderType)((int)SKINNED_OPAQUE + material->render_mode);
			material->apply(material->shaders[type]);

			// set bone matrix
			model->update();
			for(int i=0; i<model->bones.size(); i++)
			{
				material->shaders[type]->set_value(ctx.bone_matrix_uniform_names[i], model->bones[i].inv_matrix * model->bones[i].matrix);
			}
			gpu_enable_depth(material->zwrite);
			model->mesh->draw(material->shaders[type]);
			gpu_enable_depth(true);
		}
		else
		{
			draw_mesh_impl(model->mesh.get(), model->materials[0].get(), transform);
		}
	}
}

static void draw_model_instance_impl(Model *model, u32 count, const mat4 *transforms)
{
	if(model->materials.size() > 0)
	{
		draw_mesh_instance_impl(model->mesh.get(), model->materials[0].get(), count, transforms);
	}
}

static bool cameraref_sort(const CameraRef &left, const CameraRef &right)
{
	return left->priority < right->priority;
}

static void swap_post_process_buffer()
{
	RenderTargetRef tmp = ctx.postfx.buffer;
	ctx.postfx.buffer = ctx.postfx.output_buffer;
	ctx.postfx.output_buffer = tmp;
}

void renderer_draw(void(*draw_func)())
{
	gpu_clear(CLEAR_BUFFER_COLOR | CLEAR_BUFFER_DEPTH);
	gpu_bind_render_target(ctx.main_render_target);

	// sort and copy the camera list
	std::sort(ctx.cameras.begin(), ctx.cameras.end(), cameraref_sort);
	std::vector<CameraRef> sorted;
	for(int i=0; i<ctx.cameras.size(); i++)
	{
		sorted.push_back(ctx.cameras[i]);
	}

	// draw
	for(int i=0; i<sorted.size(); i++)
	{
		ctx.current_camera = sorted[i];
		if(draw_func) draw_func();
		renderer_commit(sorted[i]);
	}
	ctx.current_camera = nullptr;

	// reset viewport
	vec2 size = app_get_size();
	gpu_set_viewport(rect_t(0, 0, size.x, size.y));

	gpu_bind_render_target(nullptr);


	// Start post processing effect
	// ===============================================================================================
	// SSAO
	gpu_bind_render_target(ctx.postfx.buffer);
		ctx.postfx.ssao.shader->set_value("_proj", ctx.cameras[0]->get_projection_matrix());
		ctx.postfx.ssao.shader->set_value("_invprojection", ctx.cameras[0]->get_projection_matrix().inversed());
		ctx.postfx.ssao.shader->set_value("_radius", ctx.postfx.ssao.radius);
		ctx.postfx.ssao.shader->set_value("_bias", ctx.postfx.ssao.bias);
		ctx.postfx.ssao.shader->set_value_int("_main_tex", 0);
		ctx.postfx.ssao.shader->set_value_int("_depth_tex", 1);
		ctx.main_render_target->depth_buffer->bind(1);
		draw_texture(ctx.main_render_target->color_buffer, vec2(), ctx.postfx.ssao.shader);
		swap_post_process_buffer();

	gpu_bind_render_target(ctx.postfx.buffer);
		ctx.postfx.ssao.blur_shader->set_value_int("_main_tex", 0);
		ctx.postfx.ssao.blur_shader->set_value_int("_ssao_tex", 1);
		ctx.postfx.output_buffer->color_buffer->bind(1);	// _ssao_tex
		draw_texture(ctx.main_render_target->color_buffer, vec2(), ctx.postfx.ssao.blur_shader);
		swap_post_process_buffer();


	// ===============================================================================================
	// Fog
	gpu_bind_render_target(ctx.postfx.buffer);
		ctx.postfx.fog.shader->set_value("_near", ctx.cameras[0]->near);
		ctx.postfx.fog.shader->set_value("_far", ctx.cameras[0]->far);
		ctx.postfx.fog.shader->set_value("_fog_start", ctx.postfx.fog.start);
		ctx.postfx.fog.shader->set_value("_fog_end", ctx.postfx.fog.end);
		ctx.postfx.fog.shader->set_value("_color", ctx.postfx.fog.color);
		ctx.postfx.fog.shader->set_value_int("_main_tex", 0);
		ctx.postfx.fog.shader->set_value_int("_depth_tex", 1);
		ctx.main_render_target->depth_buffer->bind(1);
		draw_texture(ctx.postfx.output_buffer->color_buffer, vec2(), ctx.postfx.fog.shader);
		swap_post_process_buffer();


	// Rendering to main buffer
	gpu_bind_render_target(nullptr);
	gpu_clear(CLEAR_BUFFER_COLOR | CLEAR_BUFFER_DEPTH);
	draw_texture(ctx.postfx.output_buffer->color_buffer, vec2(), ctx.postfx.ssao.blur_shader);
}

void renderer_commit(CameraRef camera)
{
	vec3 position;
	quat rotation;
	camera->get_shaked_transform(&position, &rotation);

	ctx.view_matrix = mat4::lookat(position, position + rotation.forward(), rotation.up());
	ctx.projection_matrix = mat4::perspective(camera->fov * DEG2RAD, camera->viewport.w / camera->viewport.h, camera->near, camera->far);
	gpu_clear(camera->clear_mode, camera->clear_color);
	gpu_set_viewport(camera->viewport);


	// sort rendering commands
	static std::vector<RenderingCommand*> commands;
	static std::vector<RenderingCommand*> late_commands;	// like transparent commands

	std::sort(ctx.rendering_commands.begin(), ctx.rendering_commands.end());
	for(auto it = ctx.rendering_commands.begin(); it != ctx.rendering_commands.end(); it++)
	{
		if(it->material->render_mode == Material::TRANSPARENT)
		{
			late_commands.push_back(it._Ptr);
		}
		else
		{
			commands.push_back(it._Ptr);
		}
	}
	commands.insert(commands.end(), late_commands.begin(), late_commands.end());
	

	for(int i=0; i<commands.size(); i++)
	{
		RenderingCommand *cmd = commands[i];
		switch(cmd->type)
		{
		case RenderingCommandType::DrawMesh:
			draw_mesh_impl(cmd->mesh, cmd->material, cmd->transform);
			break;
		case RenderingCommandType::DrawMeshInstance:
			draw_mesh_instance_impl(cmd->mesh, cmd->material, cmd->count, cmd->transforms);
			delete [] cmd->transforms;
			break;
		case RenderingCommandType::DrawMeshLines:
			draw_mesh_lines_impl(cmd->mesh, cmd->material, cmd->transform);
			break;
		case RenderingCommandType::DrawModel:
			draw_model_impl(cmd->model, cmd->transform);
			break;
		case RenderingCommandType::DrawModelInstance:
			draw_model_instance_impl(cmd->model, cmd->count, cmd->transforms);
			delete [] cmd->transforms;
			break;

		default:
			break;
		}
	}

	// cleanup
	ctx.rendering_commands.clear();
	commands.clear();
	late_commands.clear();
	ctx.line.count = 0;
}




// 3D draw functions ==============================================================================
void draw_mesh(MeshRef mesh, MaterialRef material, const mat4 &transform)
{
	if(mesh && material)
	{
		RenderingCommand cmd;
		cmd.type = RenderingCommandType::DrawMesh;
		cmd.mesh = mesh.get();
		cmd.material = material.get();
		cmd.transform = transform;
		ctx.rendering_commands.push_back(cmd);
	}
}

// instanced drawing
void draw_mesh(MeshRef mesh, MaterialRef material, u32 count, const mat4 *transforms)
{
	if(mesh && material)
	{
		RenderingCommand cmd;
		cmd.type = RenderingCommandType::DrawMeshInstance;
		cmd.mesh = mesh.get();
		cmd.material = material.get();
		cmd.count = count;
		cmd.transforms = new mat4[count];
		memcpy(cmd.transforms, transforms, sizeof(mat4)*count);
		ctx.rendering_commands.push_back(cmd);
	}
}

void draw_mesh_lines(MeshRef mesh, MaterialRef material, const mat4 &transform)
{
	if(mesh && material)
	{
		RenderingCommand cmd;
		cmd.type = RenderingCommandType::DrawMeshLines;
		cmd.mesh = mesh.get();
		cmd.material = material.get();
		cmd.transform = transform;
		ctx.rendering_commands.push_back(cmd);
	}
}

void draw_model(ModelRef model, const mat4 &transform)
{
	if(model && model->mesh)
	{
		RenderingCommand cmd;
		cmd.type = RenderingCommandType::DrawModel;
		cmd.model = model.get();
		cmd.transform = transform;
		cmd.material = model->materials[0].get();
		ctx.rendering_commands.push_back(cmd);
	}
}

void draw_model(ModelRef model, u32 count, const mat4 *transforms)
{
	if(model && model->mesh)
	{
		RenderingCommand cmd;
		cmd.type = RenderingCommandType::DrawModelInstance;
		cmd.model = model.get();
		cmd.count = count;
		cmd.transforms = new mat4[count];
		cmd.material = model->materials[0].get();
		memcpy(cmd.transforms, transforms, sizeof(mat4)*count);
		ctx.rendering_commands.push_back(cmd);
	}
}




// Line Renderer
void draw_line(const LineData &data, const vec4 &color)
{
	if(data.points.size() < 2)
		return;

	// line mesh vertices
	static std::vector<Vertex> vertices;
	vertices.clear();
	for(int i=1; i<data.points.size(); i++)
	{
		LinePoint p1 = data.points[i-1];
		LinePoint p2 = data.points[i];
		vec3 dir = (p2.position - p1.position).normalized();
		float k = vec3::dot(ctx.current_camera->position - p1.position, dir);
		vec3 perpendicular_foot = p1.position + (dir * k);
		vec3 n = (ctx.current_camera->position - perpendicular_foot).normalized();
		vec3 c = vec3::cross(dir, n);
		Vertex v1 = {};
		v1.position = p1.position + c * data.thickness * p1.scale;
		v1.color = color;
		Vertex v2 = {};
		v2.position = p1.position - c * data.thickness * p1.scale;
		v2.color = color;
		vertices.push_back(v1);
		vertices.push_back(v2);

		// final segment
		if(i == data.points.size()-1)
		{
			v1.position = p2.position + c * data.thickness * p1.scale;
			v2.position = p2.position - c * data.thickness * p1.scale;
			vertices.push_back(v1);
			vertices.push_back(v2);	
		}
	}

	// line mesh indices
	static std::vector<u16> indices;
	indices.clear();
	for(int i=0; i<data.points.size()-1; i++)
	{
		//[0]-[2]
		// | / |
		//[1]-[3]

		indices.push_back((i*2)+0);
		indices.push_back((i*2)+1);
		indices.push_back((i*2)+2);

		indices.push_back((i*2)+2);
		indices.push_back((i*2)+1);
		indices.push_back((i*2)+3);
	}

	if(ctx.line.meshes.size() <= ctx.line.count)
	{
		// new line mesh
		ctx.line.meshes.push_back(gpu_create_mesh(0,0,0,0));
	}
	ctx.line.meshes[ctx.line.count]->update(vertices.data(), (u32)vertices.size(), indices.data(), (u32)indices.size());
	draw_mesh(ctx.line.meshes[ctx.line.count], ctx.line.material, mat4::identity());
	ctx.line.count++;
}