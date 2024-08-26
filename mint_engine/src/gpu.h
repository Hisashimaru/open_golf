#pragma once

#include <vector>
#include <memory>
#include "mathf.h"
#define GRAPHICS_API_OPENGL

enum class BlendMode
{
	None,
	Alpha,
	Add,
	Multiply,
	Screen,
};

#ifdef GRAPHICS_API_OPENGL

#define CLEAR_BUFFER_DEPTH 0x00000100
#define CLEAR_BUFFER_STENCIL 0x00000400
#define CLEAR_BUFFER_COLOR 0x00004000


enum class TextureFormat
{
	R,
	RGB,
	RGBA,
	DEPTH24,
};

#endif // GRAPHICS_API_OPENGL


class Mesh;
class Texture;
class RenderTarget;
class Shader;
typedef std::shared_ptr<Mesh> MeshRef;
typedef std::shared_ptr<Texture> TextureRef;
typedef std::shared_ptr<RenderTarget> RenderTargetRef;
typedef std::shared_ptr<Shader> ShaderRef;


struct Vertex
{
	vec3 position;
	vec3 normal;
	vec2 uv;
	vec4 color;

	// skinned
	int bones[4];
	float weights[4];
};

struct Submesh
{
	Vertex *vertices;
	u32 vertex_count;
	u16 *indices;
	u32 index_count;
	u32 vao, vbo, ebo;	
	bool is_dynamic;
	bounds_t aabb;
};

class Mesh
{
public:
	Mesh(){}
	Mesh(const Vertex *vertices, u32 vertex_count, const u16 *indices, u32 index_count);
	~Mesh();

	void create(const Vertex *vertices, u32 vertex_count, const u16 *indices, u32 index_count, int index=0);
	void update(const Vertex *vertices, u32 vertex_count, const u16 *indices, u32 index_count, int index=0);
	void update(const Vertex *vertices, u32 vertex_count, u32 offset=0, int index=0);
	void free();

	void draw(ShaderRef shader, int index=0);
	void draw_lines(ShaderRef shader, int index=0);
	void draw(ShaderRef shader, u32 count, const mat4 *transforms, int index=0);

	bounds_t get_bounds();
	bounds_t get_bounds(int index);

	const Vertex* get_vertices(int index=0);
	u32 get_vertex_count(int index=0);
	const u16* get_indices(int index=0);
	u32 get_index_count(int index=0);
	Submesh* get_submesh(int index);
protected:
	void free_submesh(int index);
protected:
	std::vector<Submesh> submeshes;
};

class Texture
{
public:
	Texture() : width(0), height(0), id(0){}
	Texture(int width, int height, TextureFormat format=TextureFormat::RGBA){create(width, height, format);}
	~Texture(){free();}

	bool load(const char *filename);
	bool create(int width, int height, TextureFormat format);
	bool create(const u8 *data, int width, int height);
	virtual void free();
	void bind(int index);
	static void unbind_all();

	int get_width(){return width;}
	int get_height(){return height;}

	u32 get_gl_id(){return id;}
protected:
	int width;
	int height;
	u32 id;
};

class RenderTarget
{
public:
	RenderTarget() : fbo(0), rbo(0), width(0), height(0){}
	bool create(int width, int height);
	void free();

	u32 get_frame_buffer(){return fbo;}
	int get_width(){return width;}
	int get_height(){return height;}

	TextureRef color_buffer;
	TextureRef depth_buffer;
protected:
	u32 fbo;
	u32 rbo;
	int width;
	int height;
};

class Shader
{
public:
	Shader() : program(0){}
	~Shader();

	bool load(const char *vertex_file_name, const char *frag_file_name);
	bool create(const char *vertex_src, const char *frag_src);
	void free();

	void use();
	void set_value_int(const char* name, int value);
	void set_value_uint(const char* name, u32 value);
	void set_value(const char* name, float value);
	void set_value(const char *name, const vec3 &value);
	void set_value(const char *name, const vec4 &value);
	void set_value(const char *name, const mat4 &value);

	int get_uniform_location(const char *name);
	int get_attribute_location(const char *name);
protected:
	u32 program;
};


void gpu_init(void *context);
void gpu_uninit();
void gpu_collect_garbage();

void gpu_set_viewport(const rect_t &rect);
rect_t gpu_get_viewport();

void gpu_clear(u32 flag, const vec4 &color=vec4(0.4f, 0.4f, 0.8f, 1.0f));
void gpu_enable_colors(bool flag);
void gpu_enable_depth(bool flag);
void gpu_enable_depth_test(bool flag);
void gpu_set_blend_mode(BlendMode mode);

void gpu_bind_render_target(std::shared_ptr<RenderTarget> render_texture);
void gpu_bind_texture(u32 slot, std::shared_ptr<Texture> texture);

std::shared_ptr<Mesh> gpu_create_mesh();
std::shared_ptr<Mesh> gpu_create_mesh(const Vertex *vertices, u32 vertex_count, const u16 *indices, u32 index_count);
std::shared_ptr<Texture> gpu_load_texture(const char *filename);
std::shared_ptr<Texture> gpu_create_texture(const u8 *data, int width, int height);
std::shared_ptr<RenderTarget> gpu_create_render_target(int width, int height);
std::shared_ptr<Shader> gpu_create_shader(const char *vertex_src, const char *frag_src);
std::shared_ptr<Shader> gpu_load_shader(const char *vertex_file_name, const char *frag_file_name);