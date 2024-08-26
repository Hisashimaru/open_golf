#include "app.h"
#include "renderer.h"
#include "particle.h"
#include "resource_manager.h"
#include "external/saml.hpp"

static struct particle_ctx
{
	std::vector<ParticleEmitterRef> emitters;
	std::vector<mat4> transforms;
	MeshRef quad_mesh;
	MaterialRef default_material;
	TextureRef default_texture;
} ctx = {};




void particle_init()
{
	Vertex v[4] = {
		{vec3(0.5f, 0.5f, 0.0f), vec3(0,0,1), vec2(1,1), vec4(1,1,1,1)},
		{vec3(0.5f, -0.5f, 0.0f), vec3(0,0,1), vec2(1,0), vec4(1,1,1,1)},
		{vec3(-0.5f, -0.5f, 0.0f), vec3(0,0,1), vec2(0,0), vec4(1,1,1,1)},
		{vec3(-0.5f, 0.5f, 0.0f), vec3(0,0,1), vec2(0,1), vec4(1,1,1,1)}
	};
	u16 indices[6] = {0, 1, 3, 1, 2, 3};
	ctx.quad_mesh = gpu_create_mesh(v, 4, indices, 6);

	// create default material
	ctx.default_material = create_material("unlit_billboard");
	ctx.default_texture = ctx.default_material->texture;
}

void particle_uninit()
{
	ctx.emitters.clear();
	ctx.quad_mesh = nullptr;
}

void particle_update()
{
	for(int i=0; i<ctx.emitters.size(); i++)
	{
		ctx.emitters[i]->update();
	}
}

void particle_draw()
{
	for(int i=0; i<ctx.emitters.size(); i++)
	{
		ctx.emitters[i]->draw();
	}
}

ParticleEmitterRef particle_emitter_load(const char *filename)
{
	ParticleEmitterRef e = std::make_shared<ParticleEmitter>();
	if(e->load(filename))
	{
		ctx.emitters.push_back(e);
	}
	else
	{
		e = nullptr;
	}
	return e;
}

ParticleEmitterRef particle_emitter_create(const EmitterDesc &desc)
{
	ParticleEmitterRef e = std::make_shared<ParticleEmitter>();
	e->emitter_desc = desc;
	ctx.emitters.push_back(e);

	return e;
}





void ParticleEmitter::update()
{
	float dt = time_dt();
	for(int i=0; i<particles.size(); i++)
	{
		Particle *p = &particles[i];
		if(p->life_time <= time_now())
		{
			// remove a particle
			particles[i] = particles.back();
			particles.pop_back();
			i--;
			continue;
		}

		p->velocity += emitter_desc.gravity * dt;
		p->position += p->velocity * dt;

		p->rotation += p->rotation_speed * dt;
	}


	// emit particles
	float rate = emitter_desc.emission_rate;
	if(is_active == false ||rate <= 0.0f) return;
	elapsed_time += dt;

	float emission_time = 1.0f / rate;
	int count = 0;
	while(elapsed_time >= emission_time)
	{
		elapsed_time -= emission_time;
		count++;
	}

	for(int i=0; i<count; i++)
	{
		emit();
	}
}

void ParticleEmitter::draw()
{
	if(particles.size() == 0)
		return;

	ctx.transforms.clear();
	for(int i=0; i<particles.size(); i++)
	{
		Particle *p = &particles[i];
		ctx.transforms.push_back(mat4(p->position, quat::euler(0, 0, p->rotation), vec3(p->size, p->size, p->size)));
	}

	MaterialRef mat = material;
	if(mat == nullptr)
	{
		mat = ctx.default_material;
	}
	draw_mesh(ctx.quad_mesh, mat, (int)ctx.transforms.size(), &ctx.transforms[0]);
}

void ParticleEmitter::emit(int count)
{
	count = (count == 0) ? emitter_desc.emission_count : count;
	for(int i=0; i<count; i++)
	{
		if((int)particles.size() >= emitter_desc.max_particles) break;
		Particle emit_desc = {};

		switch(emitter_desc.shape.type)
		{
		case EMITTER_SHAPE_POINT:
			emit_desc.position = position;
			emit_desc.velocity = rand_in_sphere() * rand_range(emitter_desc.start_speed.x, emitter_desc.start_speed.y);
			break;

		case EMITTER_SHAPE_SPHERE:
		{
			vec3 pos = rand_in_sphere(emitter_desc.shape.radius);
			emit_desc.position = position + pos;
			emit_desc.velocity = pos.normalized() * rand_range(emitter_desc.start_speed.x, emitter_desc.start_speed.y);
			break;
		}

		case EMITTER_SHAPE_HEMISPHERE:
		{
			vec3 pos = rand_in_sphere(emitter_desc.shape.radius);
			pos.z = fabsf(pos.z);
			emit_desc.position = position + (rotation * pos);
			emit_desc.velocity = rotation * (pos.normalized() * rand_range(emitter_desc.start_speed.x, emitter_desc.start_speed.y));
			break;
		}

		case EMITTER_SHAPE_CONE:
		{
			vec2 c = rand_in_circle();
			vec3 pos = vec3(c.x, c.y, 0.0f);
			vec3 dir = lerp(vec3(0,0,1), pos.normalized(), (emitter_desc.shape.angle/90.0f) * c.len());
			emit_desc.position = position + (rotation * (pos * emitter_desc.shape.radius));
			emit_desc.velocity = rotation * (dir * rand_range(emitter_desc.start_speed.x, emitter_desc.start_speed.y));
			break;
		}

		default:
			emit_desc.position = position;
			emit_desc.velocity = rand_in_sphere() * rand_range(emitter_desc.start_speed.x, emitter_desc.start_speed.y);
			break;
		}

		float sign = rand_get() >= 0.5f ? 1.0f : -1.0f;
		emit_desc.size = rand_range(emitter_desc.start_size.x, emitter_desc.start_size.y);
		emit_desc.rotation_speed = rand_range(emitter_desc.start_rotation_speed.x, emitter_desc.start_rotation_speed.y) * sign;
		emit_desc.rotation = rand_range(emitter_desc.start_rotation.x, emitter_desc.start_rotation.y) * sign;
		emit_desc.life_time = time_now() + rand_range(emitter_desc.life_time.x, emitter_desc.life_time.y);
		emit_desc.color = emitter_desc.color;
		particles.push_back(emit_desc);
	}
}

void ParticleEmitter::emit(const vec3 &position, const quat &rotation, int count)
{
	vec3 tmp_pos = this->position;
	quat tmp_rot = this->rotation;
	this->position = position;
	this->rotation = rotation;

	emit(count);

	this->position = tmp_pos;
	this->rotation = tmp_rot;
}

void ParticleEmitter::emit(const Particle &particle)
{
	particles.push_back(particle);
}

bool ParticleEmitter::load(const char *filename)
{
	// reset particle
	particles.clear();
	elapsed_time = 0.0f;
	material = nullptr;
	emitter_desc = {};

	saml::Value v = saml::parse_file(filename);
	if(v.is_nil())
		return false;

	emitter_desc.max_particles = v["max_particles"].to_int();
	emitter_desc.start_size = v["start_size"].to_vec2();
	emitter_desc.start_rotation = v["start_rotation"].to_vec2();
	emitter_desc.start_rotation_speed = v["start_rotation_speed"].to_vec2();
	emitter_desc.start_speed = v["start_speed"].to_vec2();
	emitter_desc.life_time = v["life_time"].to_vec2();
	emitter_desc.color = v["color"].to_vec4(vec4(1,1,1,1));
	emitter_desc.gravity = v["gravity"].to_vec3();
	emitter_desc.emission_rate = v["emission_rate"].to_float();
	emitter_desc.emission_count = v["emission_count"].to_int();
	emitter_desc.shape.type = (EmitterShape)v["shape_type"].to_int();
	emitter_desc.shape.radius = v["shape_radius"].to_float();
	emitter_desc.shape.angle = v["shape_angle"].to_float();
	material = load_material(v["material"].to_string("_white").c_str());
	is_active = v["active"].to_int(1) == 1;

	return true;
}