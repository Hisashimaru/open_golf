#pragma once

#include <memory>
#include "gpu.h"
#include "model.h"
#include "font.h"

struct ParticleEmitter;
typedef std::shared_ptr<ParticleEmitter> ParticleEmitterRef;


TextureRef res_get_texture(const char *name);
MeshRef res_get_mesh(const char *name);
ShaderRef res_get_shader(const char *name);
ModelRef res_get_model(const char *name);
ParticleEmitterRef res_get_particle_emitter(const char *name);

void res_register(const char *name, TextureRef texture);
void res_register(const char *name, MeshRef mesh);
void res_register(const char *name, ShaderRef shader);
void res_register(const char *name, MaterialRef material);
void res_register(const char *name, ModelRef model);
void res_register(const char *name, FontRef font);
void res_register(const char *name, ParticleEmitterRef emitter);

void res_init(const char *filename);
void res_uninit();


TextureRef load_texture(const char *filename);
ShaderRef load_shader(const char *vs_filename, const char *fs_filename);
FontRef load_font(const char *filename);
MaterialRef load_material(const char *filename);
ModelRef load_model(const char *filename);
ParticleEmitterRef load_particle_emitter(const char *filename);