#include <unordered_map>
#include "resource_manager.h"
#include "external/saml.hpp"
#include "material.h"
#include "particle.h"

struct res_ctx
{
	std::unordered_map<std::string, TextureRef> textures;
	std::unordered_map<std::string, MeshRef> meshes;
	std::unordered_map<std::string, ShaderRef> shaders;
	std::unordered_map<std::string, MaterialRef> materials;
	std::unordered_map<std::string, ModelRef> models;
	std::unordered_map<std::string, FontRef> fonts;
	std::unordered_map<std::string, ParticleEmitterRef> emitters;

	saml::Value root;
	TextureRef tex_white;
} ctx = {};

TextureRef res_get_texture(const char *name)
{
	if(ctx.textures.count(name) != 0)
		return ctx.textures[name];
	return nullptr;
}

MeshRef res_get_mesh(const char *name)
{
	if(ctx.meshes.count(name) != 0)
		return ctx.meshes[name];
	return nullptr;
}

ShaderRef res_get_shader(const char *name)
{
	if(ctx.shaders.count(name) != 0)
		return ctx.shaders[name];
	return nullptr;
}

MaterialRef res_get_material(const char *name)
{
	if(ctx.materials.count(name) != 0)
		return ctx.materials[name];
	return nullptr;
}

ModelRef res_get_model(const char *name)
{
	if(ctx.models.count(name) != 0)
		return ctx.models[name];
	return nullptr;
}

FontRef res_get_font(const char *name)
{
	if(ctx.fonts.count(name) != 0)
		return ctx.fonts[name];
	return nullptr;
}

ParticleEmitterRef res_get_particle_emitter(const char *name)
{
	if(ctx.emitters.count(name) != 0)
		return ctx.emitters[name];
	return nullptr;
}


void res_register(const char *name, TextureRef texture)
{
	ctx.textures[name] = texture;
}

void res_register(const char *name, MeshRef mesh)
{
	ctx.meshes[name] = mesh;
}

void res_register(const char *name, ShaderRef shader)
{
	ctx.shaders[name] = shader;
}

void res_register(const char *name, MaterialRef material)
{
	ctx.materials[name] = material;
}

void res_register(const char *name, ModelRef model)
{
	ctx.models[name] = model;
}

void res_register(const char *name, FontRef font)
{
	ctx.fonts[name] = font;
}

void res_register(const char *name, ParticleEmitterRef emitter)
{
	ctx.emitters[name] = emitter;
}

void res_init(const char *filename)
{
	ctx.root = saml::parse_file(filename);

	// create null texture
	u8 data[4] = {255, 255, 255, 255};
	ctx.tex_white = gpu_create_texture(data, 1, 1);
	res_register("_white", ctx.tex_white);
}

void res_uninit()
{
	ctx.materials.clear();
	ctx.textures.clear();
	ctx.meshes.clear();
	ctx.shaders.clear();
	ctx.models.clear();
	ctx.fonts.clear();
	ctx.emitters.clear();
	ctx.tex_white = nullptr;
}

TextureRef load_texture(const char *filename)
{
	TextureRef texture = res_get_texture(filename);
	if(texture == nullptr)
	{
		texture = gpu_load_texture(filename);
		if(texture->get_height() != 0 && texture->get_width() != 0) {
			res_register(filename, texture);
		} else {
			texture = ctx.tex_white;
		}
	}
	return texture;
}

ShaderRef load_shader(const char *vs_filename, const char *fs_filename)
{
	char buf[512] = {};
	strcat(buf, vs_filename);
	strcat(buf, fs_filename);
	ShaderRef shader = res_get_shader(buf);
	if(shader == nullptr)
	{
		shader = gpu_load_shader(vs_filename, fs_filename);
		if(shader)
			res_register(buf, shader);
	}
	return shader;
}

FontRef load_font(const char *filename)
{
	FontRef font = res_get_font(filename);
	if(font == nullptr)
	{
		font = std::make_shared<Font>();
		if(font->load(filename))
		{
			res_register(filename, font);
		}
		else
		{
			font = nullptr;
		}
	}
	return font;
}

MaterialRef load_material(const char *filename)
{
	MaterialRef mat = res_get_material(filename);
	if(mat == nullptr)
	{
		if(ctx.root.is_table())
		{
			saml::Value materials = ctx.root["materials"];
			saml::Value m = materials[filename];
			if(m.is_nil()) return nullptr;

			// create new material
			std::string type_name = m["type"].to_string("unlit");
			mat = create_material(type_name.c_str());
			std::string  tex_name = m["texture"].to_string();
			mat->texture = !tex_name.empty() ? load_texture(tex_name.c_str()) : ctx.tex_white;

			// rendering mode
			std::string render_mode = m["mode"].to_string();
			if(render_mode == "cutout") {
				mat->render_mode = Material::RenderMode::CUTOUT;
				mat->zwrite = true;
			}
			else if(render_mode == "transparent") {
				mat->render_mode = Material::RenderMode::TRANSPARENT;
				mat->zwrite = false;
			}
			else if(render_mode == "depthmask") {
				mat->render_mode = Material::RenderMode::DEPTHE_MASK;
				mat->zwrite = true;
			}
			else  {
				mat->render_mode = Material::RenderMode::OPAQUE;
				mat->zwrite = true;
			}

			// rendering queue
			mat->queue = m["queue"].to_int();

			// color
			mat->color = m["color"].to_vec4(vec4(1,1,1,1));

			// write z buffer
			mat->zwrite = m["zwrite"].to_bool(mat->zwrite);

			res_register(filename, mat);
		}
	}

	return mat;
}

ModelRef load_model(const char *filename)
{
	ModelRef srcmodel = res_get_model(filename);
	if(srcmodel == nullptr)
	{
		saml::Value models = ctx.root["models"];
		saml::Value m = models[filename];
		if(m.is_nil())
		{
			// load model from mesh file
			srcmodel = std::make_shared<Model>();
			srcmodel->load(filename);
		}
		else
		{
			// load model from resfile
			ModelRef model = std::make_shared<Model>();
			std::string mesh_name = m.get_string("mesh");
			if(!mesh_name.empty())
				model->load(mesh_name.c_str());

			// load material
			saml::Value materials = m["materials"];
			for(int i=0; i<materials.get_size(); i++)
			{
				if(materials[i].str.empty()) continue;
				model->materials.push_back(load_material(materials[i].str.c_str()));
			}

			srcmodel = model;
		}
		res_register(filename, srcmodel);
	}

	// clone model
	ModelRef model = std::make_shared<Model>();
	*(model.get()) = *(srcmodel.get());

	return model;
}

ParticleEmitterRef load_particle(const char *name)
{
	ParticleEmitterRef emitter = res_get_particle_emitter(name);
	if(emitter == nullptr)
	{
		emitter = particle_emitter_load(name);
		if(emitter)
		{
			res_register(name, emitter);
		}
	}
	return emitter;
}