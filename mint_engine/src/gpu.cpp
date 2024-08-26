#include <vector>
#include "gpu.h"

#define GLAD_GL_IMPLEMENTATION
#include "external/gl.h"
#include "external/stb_image.h"

#define STB_IMAGE_IMPLEMENTATION
#include "external/stb_image.h"

vec2 app_get_size();
void app_set_resize_cb(void(*resize_cb)(int width, int height));

static struct gfx_ctx
{
	rect_t viewport;
	std::vector<std::shared_ptr<Mesh>> mesh_list;
	std::vector<std::shared_ptr<Shader>> shader_list;
	std::vector<std::shared_ptr<Texture>> texture_list;
} ctx{};


static void resize_call_back(int width, int height)
{
	ctx.viewport.x = 0;
	ctx.viewport.y = 0;
	ctx.viewport.w = (float)width;
	ctx.viewport.h = (float)height;
	gpu_set_viewport(ctx.viewport);
}

void gpu_init(void *context)
{
	gladLoaderLoadGL();

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	//glEnable(GL_CULL_FACE);
	//glCullFace(GL_BACK);
	vec2 size = app_get_size();
	ctx.viewport = rect_t(0, 0, size.x, size.y);
	gpu_set_viewport(ctx.viewport);
	app_set_resize_cb(resize_call_back);
}

void gpu_uninit()
{
	for(int i=0; i<ctx.mesh_list.size(); i++)
	{
		ctx.mesh_list[i]->free();
	}

	for(int i=0; i<ctx.shader_list.size(); i++)
	{
		ctx.shader_list[i]->free();
	}

	for(int i=0; i<ctx.texture_list.size(); i++)
	{
		ctx.texture_list[i]->free();
	}

	ctx.mesh_list.clear();
	ctx.shader_list.clear();
	ctx.texture_list.clear();
	gladLoaderUnloadGL();
}

void gpu_collect_garbage()
{
	// free unused mesh
	for(int i=0; i<ctx.mesh_list.size(); i++)
	{
		if(ctx.mesh_list[i].use_count() == 1)
		{
			ctx.mesh_list[i] = ctx.mesh_list.back();
			ctx.mesh_list.pop_back();
			i--;
		}
	}

	// texture
	for(int i=0; i<ctx.texture_list.size(); i++)
	{
		if(ctx.texture_list[i].use_count() == 1)
		{
			ctx.texture_list[i] = ctx.texture_list.back();
			ctx.texture_list.pop_back();
			i--;
		}
	}

	// shader
	for(int i=0; i<ctx.shader_list.size(); i++)
	{
		if(ctx.shader_list[i].use_count() == 1)
		{
			ctx.shader_list[i] = ctx.shader_list.back();
			ctx.shader_list.pop_back();
			i--;
		}
	}
}

void gpu_set_viewport(const rect_t &rect)
{
	glViewport((GLsizei)rect.x, (GLsizei)rect.y, (GLsizei)rect.w, (GLsizei)rect.h);
	glScissor((GLsizei)rect.x, (GLsizei)rect.y, (GLsizei)rect.w, (GLsizei)rect.h);
}

rect_t gpu_get_viewport()
{
	return ctx.viewport;
}

void gpu_clear(u32 flag, const vec4 &color)
{
	glClearColor(color.r, color.g, color.b, color.a);
	glClear((GLbitfield)flag);
}

void gpu_enable_colors(bool flag)
{
	glColorMask(flag, flag, flag, flag);
}

void gpu_enable_depth(bool flag)
{
	glDepthMask(flag);
}

void gpu_enable_depth_test(bool flag)
{
	if(flag){
		glEnable(GL_DEPTH_TEST);
	} else {
		glDisable(GL_DEPTH_TEST);
	}
}

void gpu_set_blend_mode(BlendMode mode)
{
	glEnable(GL_BLEND);
	switch(mode)
	{
	case BlendMode::None:
		glDisable(GL_BLEND);
		break;
	case BlendMode::Alpha:
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		break;
	case BlendMode::Add:
		glBlendFunc(GL_SRC_ALPHA, GL_ONE);
		break;
	case BlendMode::Multiply:
		glBlendFunc(GL_ZERO, GL_SRC_COLOR);
		break;
	case BlendMode::Screen:
		glBlendFunc(GL_ONE_MINUS_DST_COLOR, GL_ONE);
		break;
	default:
		break;
	}
}

void gpu_set_blend_mode()
{
}

void gpu_bind_render_target(std::shared_ptr<RenderTarget> render_target)
{
	u32 id = 0;
	vec2 size = app_get_size();
	int width = (int)size.x;
	int height = (int)size.y;

	if(render_target != nullptr)
	{
		id = render_target->get_frame_buffer();
		width = render_target->get_width();
		height = render_target->get_height();
	}

	glBindFramebuffer(GL_FRAMEBUFFER, id);
	glViewport(0, 0, width, height);
}

void gpu_bind_texture(u32 slot, std::shared_ptr<Texture> texture)
{
	if(texture == nullptr)
	{
		glBindTexture(slot, 0);
		return;
	}
	texture->bind(slot);
}

std::shared_ptr<Mesh> gpu_create_mesh()
{
	auto mesh = std::make_shared<Mesh>();
	ctx.mesh_list.push_back(mesh);
	return mesh;
}

std::shared_ptr<Mesh> gpu_create_mesh(const Vertex *vertices, u32 vertex_count, const u16 *indices, u32 index_count)
{
	auto mesh = std::make_shared<Mesh>(vertices, vertex_count, indices, index_count);
	ctx.mesh_list.push_back(mesh);
	return mesh;
}

std::shared_ptr<Texture> gpu_load_texture(const char *filename)
{
	auto tex = std::make_shared<Texture>();
	tex->load(filename);
	ctx.texture_list.push_back(tex);
	return tex;
}

std::shared_ptr<Texture> gpu_create_texture(const u8 *data, int width, int height)
{
	auto tex = std::make_shared<Texture>();
	tex->create(data, width, height);
	ctx.texture_list.push_back(tex);
	return tex;
}

std::shared_ptr<RenderTarget> gpu_create_render_target(int width, int height)
{
	auto tex = std::make_shared<RenderTarget>();
	tex->create(width, height);
	return tex;
}

std::shared_ptr<Shader> gpu_create_shader(const char *vertex_src, const char *frag_src)
{
	auto shader = std::make_shared<Shader>();
	ctx.shader_list.push_back(shader);
	shader->create(vertex_src, frag_src);
	return shader;
}

std::shared_ptr<Shader> gpu_load_shader(const char *vertex_file_name, const char *frag_file_name)
{
	auto shader = std::make_shared<Shader>();
	ctx.shader_list.push_back(shader);
	shader->load(vertex_file_name, frag_file_name);
	return shader;
}




Mesh::Mesh(const Vertex *vertices, u32 vertex_count, const u16 *indices, u32 index_count)
{
	create(vertices, vertex_count, indices, index_count);
}

Mesh::~Mesh()
{
	free();
}

static bounds_t calc_bounds(const Vertex *vertices, u32 vertex_count)
{
	if(vertex_count <= 0) return bounds_t();

	bounds_t b(vertices[0].position, vec3(0,0,0));
	for(u32 i=1; i<vertex_count; i++)
	{
		vec3 p = vertices[i].position;
		b.encapsulate(p);
	}
	return b;
}

static Submesh create_submesh(const Vertex *vertices, u32 vertex_count, const u16 *indices, u32 index_count, bool is_dynamic)
{
	Submesh submesh = {};
	bool skinned = false;
	
	// check the vertex data is skinned
	for(u32 i=0; i<vertex_count; i++)
	{
		if(vertices[i].weights[0] > 0.0f || vertices[i].weights[1] > 0.0f || vertices[i].weights[2] > 0.0f || vertices[i].weights[3] > 0.0f)
		{
			skinned = true;
			break;
		}
	}

	glGenVertexArrays(1, &submesh.vao);
	glGenBuffers(1, &submesh.vbo);
	glGenBuffers(1, &submesh.ebo);

	GLenum usage = is_dynamic ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW;

	// vertices
	glBindVertexArray(submesh.vao);
	glBindBuffer(GL_ARRAY_BUFFER, submesh.vbo);
	glBufferData(GL_ARRAY_BUFFER, vertex_count * sizeof(Vertex), vertices, usage);  


	// indices
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, submesh.ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, index_count * sizeof(u16), indices, usage);


	// vertex positions
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
	// vertex normals
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
	// vertex texture coords
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, uv));
	// vertex color
	glEnableVertexAttribArray(3);
	glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, color));


	// skinned mesh ///////////////////////////////////////////////////////////////////////////////
	// bone id
	glEnableVertexAttribArray(4);
	glVertexAttribIPointer(4, 4, GL_INT, sizeof(Vertex), (void*)offsetof(Vertex, bones));

	// bone weight
	glEnableVertexAttribArray(5);
	glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, weights));


	glBindVertexArray(0);


	// copy data
	submesh.vertices = new Vertex[vertex_count]; memcpy(submesh.vertices, vertices, vertex_count * sizeof(Vertex));
	submesh.indices = new u16[index_count]; memcpy(submesh.indices, indices, index_count * sizeof(u16));
	submesh.vertex_count = vertex_count;
	submesh.index_count = index_count;
	submesh.aabb = calc_bounds(vertices, vertex_count);
	submesh.is_dynamic = is_dynamic;

	return submesh;
}

void Mesh::create(const Vertex *vertices, u32 vertex_count, const u16 *indices, u32 index_count, int index)
{
	const Submesh *old_submesh = get_submesh(index);
	if(old_submesh != nullptr)
	{
		free_submesh(index);
	}

	if(submeshes.size() <= index)
	{
		submeshes.resize(index+1);
	}
	submeshes[index] = create_submesh(vertices, vertex_count, indices, index_count, false);
}

void Mesh::update(const Vertex *vertices, u32 vertex_count, const u16 *indices, u32 index_count, int index)
{
	Submesh *submesh = get_submesh(index);

	// create dynamic submesh
	if(submesh == nullptr || !submesh->is_dynamic || index_count > submesh->index_count || vertex_count > submesh->vertex_count)
	{
		free_submesh(index);
		if(submeshes.size() <= index)
		{
			submeshes.resize(index+1);
		}
		submeshes[index] = create_submesh(vertices, vertex_count, indices, index_count, true);
		return;
	}

	// update buffer
	glBindBuffer(GL_ARRAY_BUFFER, submesh->vbo);
	glBufferSubData(GL_ARRAY_BUFFER, 0, vertex_count * sizeof(Vertex), vertices);
	glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, index_count * sizeof(u16), indices);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	submesh->aabb = calc_bounds(vertices, vertex_count);
}

void Mesh::update(const Vertex *vertices, u32 vertex_count, u32 offset, int index)
{
	Submesh *submesh = get_submesh(index);

	// create dynamic submesh
	if(submesh == nullptr || !submesh->is_dynamic || vertex_count + offset >= submesh->vertex_count)
	{
		Submesh new_submesh = create_submesh(vertices, vertex_count, submesh->indices, submesh->index_count, true);
		free_submesh(index);
		if(submeshes.size() <= index)
		{
			submeshes.resize(index+1);
		}
		submesh[index] = new_submesh;
		return;
	}

	glBindBuffer(GL_ARRAY_BUFFER, submesh->vbo);
	glBufferSubData(GL_ARRAY_BUFFER, offset * sizeof(Vertex), vertex_count * sizeof(Vertex), vertices);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void Mesh::free()
{
	for(int i=0; i<submeshes.size(); i++)
	{
		free_submesh(i);
	}
	submeshes.clear();
}

void Mesh::free_submesh(int index)
{
	if(index < 0 || index >= submeshes.size())
		return;

	Submesh submesh = submeshes[index];
	glDeleteVertexArrays(1, &submesh.vao);
	glDeleteBuffers(1, &submesh.vbo);
	glDeleteBuffers(1, &submesh.ebo);
	delete[] submesh.vertices;
	delete[] submesh.indices;
	submeshes[index] = {};
}

void Mesh::draw(ShaderRef shader, int index)
{
	Submesh *submesh = get_submesh(index);
	if(submesh == nullptr)
		return;

	shader->use();
	glBindVertexArray(submesh->vao);
	glDrawElements(GL_TRIANGLES, submesh->index_count, GL_UNSIGNED_SHORT, 0);
	glBindVertexArray(0);
}

void Mesh::draw_lines(ShaderRef shader, int index)
{
	Submesh *submesh = get_submesh(index);
	if(submesh == nullptr)
		return;

	shader->use();
	glBindVertexArray(submesh->vao);
	glDrawElements(GL_LINES, submesh->index_count, GL_UNSIGNED_SHORT, 0);
	glBindVertexArray(0);
}

void Mesh::draw(ShaderRef shader, u32 count, const mat4 *transforms, int index)
{
	Submesh *submesh = get_submesh(index);
	if(submesh == nullptr)
		return;

	if(shader == nullptr)
		return;

	// set vertex attributes
	int loc = shader->get_attribute_location("_instance_matrix");
	if(loc < 0) return;	// the shader not supported instancing

	glBindVertexArray(submesh->vao);

	// create a instanced VBO
	u32 instancedVBO = 0;
	glGenBuffers(1, &instancedVBO);
	glBindBuffer(GL_ARRAY_BUFFER, instancedVBO);
	glBufferData(GL_ARRAY_BUFFER, count * sizeof(mat4), transforms, GL_STATIC_DRAW);

	glEnableVertexAttribArray(loc);
	glVertexAttribPointer(loc, 4, GL_FLOAT, GL_FALSE, sizeof(mat4), (void*)0);
	glEnableVertexAttribArray(loc+1);
	glVertexAttribPointer(loc+1, 4, GL_FLOAT, GL_FALSE, sizeof(mat4), (void*)offsetof(mat4, m21));
	glEnableVertexAttribArray(loc+2);
	glVertexAttribPointer(loc+2, 4, GL_FLOAT, GL_FALSE, sizeof(mat4), (void*)offsetof(mat4, m31));
	glEnableVertexAttribArray(loc+3);
	glVertexAttribPointer(loc+3, 4, GL_FLOAT, GL_FALSE, sizeof(mat4), (void*)offsetof(mat4, m41));
	glVertexAttribDivisor(loc,   1);
	glVertexAttribDivisor(loc+1, 1);
	glVertexAttribDivisor(loc+2, 1);
	glVertexAttribDivisor(loc+3, 1);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	shader->use();
	glDrawElementsInstanced(GL_TRIANGLES, submesh->index_count, GL_UNSIGNED_SHORT, 0, count);
	glBindVertexArray(0);

	// remove instanced buffer
	glDeleteBuffers(1, &instancedVBO);
}

bounds_t Mesh::get_bounds()
{
	bounds_t b;
	for(int i=0; i<submeshes.size(); i++)
	{
		b.encapsulate(submeshes[i].aabb);
	}
	return b;
}

bounds_t Mesh::get_bounds(int index)
{
	const Submesh *submesh = get_submesh(index);
	if(submesh)
		return submesh->aabb;

	return bounds_t();
}

const Vertex* Mesh::get_vertices(int index)
{
	if(index < 0 || index >= submeshes.size())
		return nullptr;
	return submeshes[index].vertices;
}

u32 Mesh::get_vertex_count(int index)
{
	if(index < 0 || index >= submeshes.size())
		return 0;
	return submeshes[index].vertex_count;
}

const u16* Mesh::get_indices(int index)
{
	if(index < 0 || index >= submeshes.size())
		return nullptr;
	return submeshes[index].indices;
}

u32 Mesh::get_index_count(int index)
{
	if(index < 0 || index >= submeshes.size())
		return 0;
	return submeshes[index].index_count;
}

Submesh* Mesh::get_submesh(int index)
{
	if(index < 0 || index >= submeshes.size())
		return nullptr;
	return &submeshes[index];
}



bool Texture::load(const char *filename)
{
	int bpp;
	stbi_set_flip_vertically_on_load(1);
	u8 *data = stbi_load(filename, &width, &height, &bpp, 4);
	if(data == nullptr){return nullptr;}

	bool res = create(data, width, height);
	stbi_image_free(data);
	return res;
}

bool Texture::create(int width, int height, TextureFormat format)
{
	if(this->id > 0) free();

	int gl_internal_format, gl_format, data_type;
	bool mipmap = true;

	switch(format)
	{
	case TextureFormat::R:
		gl_internal_format = gl_format = GL_R;
		data_type = GL_UNSIGNED_BYTE;
		break;
	case TextureFormat::RGB:
		gl_internal_format = gl_format = GL_RGB;
		data_type = GL_UNSIGNED_BYTE;
		break;
	case TextureFormat::RGBA:
		gl_internal_format = gl_format = GL_RGBA;
		data_type = GL_UNSIGNED_BYTE;
		break;
	case TextureFormat::DEPTH24:
		gl_internal_format = GL_DEPTH_COMPONENT24;
		gl_format = GL_DEPTH_COMPONENT;
		data_type = GL_FLOAT;
		break;
	default:
		gl_internal_format = gl_format = GL_RGBA;
		data_type = GL_UNSIGNED_BYTE;
		break;
	}

	u32 id = 0;
	glGenTextures(1, &id);
	glBindTexture(GL_TEXTURE_2D, id);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, gl_internal_format, width, height, 0, gl_format, data_type, NULL);
	glGenerateMipmap(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, 0);

	this->width = width;
	this->height = height;
	this->id = id;

	return true;
}

bool Texture::create(const u8 *data, int width, int height)
{
	if(data == nullptr) return false;

	if(this->id > 0) free();

	u32 id = 0;
	glGenTextures(1, &id);
	glBindTexture(GL_TEXTURE_2D, id);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
	glGenerateMipmap(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, 0);

	this->width = width;
	this->height = height;
	this->id = id;

	return true;
}

void Texture::free()
{
	glDeleteTextures(1, &id);
	id = 0;
	width = 0;
	height = 0;
}

void Texture::bind(int index)
{
	glActiveTexture(GL_TEXTURE0 + index);
	glBindTexture(GL_TEXTURE_2D, id);
}

void Texture::unbind_all()
{
	for(int i=0; i<8; i++)
	{
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, 0);
	}
}

bool RenderTarget::create(int width, int height)
{
	free();

	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);    

	// color buffer
	color_buffer = std::make_shared<Texture>(width, height, TextureFormat::RGBA);
	depth_buffer = std::make_shared<Texture>(width, height, TextureFormat::DEPTH24);

	// attatch color buffer
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color_buffer->get_gl_id(), 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depth_buffer->get_gl_id(), 0);

	// render buffer

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	
	this->width = width;
	this->height = height;

	return true;
}

void RenderTarget::free()
{
	if(fbo == 0) return;
	glDeleteRenderbuffers(1, &rbo);
	glDeleteFramebuffers(1, &fbo);
	fbo = rbo = 0;
	color_buffer = nullptr;
	depth_buffer = nullptr;
}






Shader::~Shader()
{
	free();
}

bool Shader::load(const char *vertex_file_name, const char *frag_file_name)
{
	// Vertex shader file
	FILE *v_file = fopen(vertex_file_name, "rb");
	if(v_file == nullptr)
	{
		printf("ERROR: Could not open vertex shader file. (%s)\n", vertex_file_name);
		return false;
	}

	// Fragment shader file
	FILE* f_file = fopen(frag_file_name, "rb");
	if (f_file == nullptr)
	{
		fclose(v_file);
		printf("ERROR: Could not open vertex shader file. (%s)\n", frag_file_name);
		return false;
	}

	// Load text
	fseek(v_file, 0, SEEK_END);
	long v_size = ftell(v_file);
	char *v_src = (char*)malloc(v_size+1);
	fseek(v_file, 0, SEEK_SET);
	fread(v_src, 1, v_size, v_file);
	v_src[v_size] = '\0';	// EOF

	fseek(f_file, 0, SEEK_END);
	long f_size = ftell(f_file);
	char* f_src = (char*)malloc(f_size+1);
	fseek(f_file, 0, SEEK_SET);
	fread(f_src, 1, f_size, f_file);
	f_src[f_size] = '\0';	// EOF

	bool result = create(v_src, f_src);

	::free(v_src);
	::free(f_src);
	fclose(v_file);
	fclose(f_file);

	return result;
}

bool Shader::create(const char *vertex_src, const char *frag_src)
{
	// create a shader program
	int success = 0;
	char info_log[512] = "";
	u32 shader_program = 0;
	u32 vertex_shader = 0;
	u32 frag_shader = 0;

	vertex_shader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertex_shader, 1, &vertex_src, NULL);
	glCompileShader(vertex_shader);
	glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(vertex_shader, 512, NULL, info_log);
		printf("ERROR::SHADER::VERTEX::COMPILATION_FAILED\n%s\n", info_log);
		glDeleteShader(vertex_shader);
		return false;
	}

	frag_shader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(frag_shader, 1, &frag_src, NULL);
	glCompileShader(frag_shader);
	glGetShaderiv(frag_shader, GL_COMPILE_STATUS, &success);
	if(!success)
	{
		glGetShaderInfoLog(frag_shader, 512, NULL, info_log);
		printf("ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n%s\n", info_log);
		glDeleteShader(vertex_shader);
		glDeleteShader(frag_shader);
		return false;
	}

	shader_program = glCreateProgram();
	glAttachShader(shader_program, vertex_shader);
	glAttachShader(shader_program, frag_shader);
	glLinkProgram(shader_program);
	glGetProgramiv(shader_program, GL_LINK_STATUS, &success);
	if(!success) {
		glGetProgramInfoLog(shader_program, 512, NULL, info_log);
		printf("ERROR::SHADER::PROGRAM::LINK_FAILED\n%s\n", info_log);
		glDeleteShader(vertex_shader);
		glDeleteShader(frag_shader);
		glDeleteProgram(shader_program);
		return false;
	}

	glDeleteShader(vertex_shader);
	glDeleteShader(frag_shader);

	program = shader_program;
	return true;
}

void Shader::free()
{
	if(program == 0) return;
	glDeleteProgram(program);
	program = 0;
}

void Shader::use()
{
	glUseProgram(program);
}

void Shader::set_value_int(const char* name, int value)
{
	if(program == 0) return; 
	int loc = glGetUniformLocation(program, name);
	if(loc >= 0)
	{
		glUseProgram(program);
		glUniform1i(loc, value);
		glUseProgram(0);
	}
}

void Shader::set_value_uint(const char* name, u32 value)
{
	if(program == 0) return; 
	int loc = glGetUniformLocation(program, name);
	if(loc >= 0)
	{
		glUseProgram(program);
		glUniform1ui(loc, value);
		glUseProgram(0);
	}
}

void Shader::set_value(const char* name, float value)
{
	if(program == 0) return; 
	int loc = glGetUniformLocation(program, name);
	if(loc >= 0)
	{
		glUseProgram(program);
		glUniform1f(loc, value);
		glUseProgram(0);
	}
}

void Shader::set_value(const char *name, const vec3 &value)
{
	if(program == 0) return; 
	int loc = glGetUniformLocation(program, name);
	if(loc >= 0)
	{
		glUseProgram(program);
		glUniform3f(loc, value.x, value.y, value.z);
		glUseProgram(0);
	}
}

void Shader::set_value(const char *name, const vec4 &value)
{
	if(program == 0) return; 
	int loc = glGetUniformLocation(program, name);
	if(loc >= 0)
	{
		glUseProgram(program);
		glUniform4f(loc, value.x, value.y, value.z, value.w);
		glUseProgram(0);
	}
}

void Shader::set_value(const char *name, const mat4 &value)
{
	if(program == 0) return; 
	int loc = glGetUniformLocation(program, name);
	if(loc >= 0)
	{
		glUseProgram(program);
		glUniformMatrix4fv(loc, 1, false, value.m);
		glUseProgram(0);
	}
}

int Shader::get_uniform_location(const char *name)
{
	if(program == 0) return -1;
	return glGetUniformLocation(program, name);
}

int Shader::get_attribute_location(const char *name)
{
	if(program == 0) return -1;
	return glGetAttribLocation(program, name);
}