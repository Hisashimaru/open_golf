#include <vector>
#include "entity.h"
#include "renderer.h"
#include "app.h"
#include "collision.h"
#include "../game.h"
#include "../map.h"


static struct entity_ctx
{
	u32 entity_id_counter = 0;
	std::vector<EntityRef> entities;
} ctx = {};

void Entity::uninit()
{
	free_collider(collider);
}

void Entity::draw()
{
	if(model)
	{
		draw_model(model, mat4(position, rotation, scale));
	}
}

void Entity::destroy()
{
	free_entity(shared_from_this());
}

void Entity::take_damage(float value)
{
	health -= value;
}

void Entity::set_position(const vec3 &position)
{
	this->position = position;
	if(collider) {
		collider->set_position(position);
	}
}

void Entity::set_rotation(const quat &rotation)
{
	this->rotation = rotation;
	if(collider) {
		collider->set_rotation(rotation);
	}
}




EntityRef add_entity(EntityRef entity)
{
	if(entity == nullptr) return nullptr;

	u32 id = 0;
	if(ctx.entity_id_counter < 0xFFFFFFFF)
	{
		ctx.entity_id_counter++;
		id = ctx.entity_id_counter;
	}

	entity->id = id;
	ctx.entities.push_back(entity);
	entity->init();
	return entity;
}

void free_entity(EntityRef entity)
{
	if(entity)
	{
		auto it = std::find(ctx.entities.begin(), ctx.entities.end(), entity);
		if(it != ctx.entities.end())
		{
			std::iter_swap(it, ctx.entities.end() -1);
			ctx.entities.pop_back();
			entity->uninit();
		}
	}
}

void free_entities()
{
	for(int i=0; i<ctx.entities.size(); i++)
	{
		ctx.entities[i]->uninit();
	}
	ctx.entities.clear();
	ctx.entity_id_counter = 0;
}

EntityRef get_entity(u32 id)
{
	for(int i=0; i<ctx.entities.size(); i++)
	{
		if(ctx.entities[i]->id == id)
		{
			return ctx.entities[i];
		}
	}

	return nullptr;
}

void update_entities()
{
	for(int i=0; i<ctx.entities.size(); i++)
	{
		ctx.entities[i]->update();
	}
}

void draw_entities()
{
	for(int i=0; i<ctx.entities.size(); i++)
	{
		ctx.entities[i]->draw();
	}
}