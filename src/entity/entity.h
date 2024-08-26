#pragma once

#include <memory>
#include "mathf.h"
#include "gpu.h"
#include "model.h"
#include "collision.h"

class Entity : public std::enable_shared_from_this<Entity>
{
public:
	Entity(){}
	~Entity(){}

	virtual void init(){}
	virtual void uninit();
	virtual void update(){}
	virtual void draw();
	void destroy();

	virtual void take_damage(float value);

	void set_position(const vec3 &position);
	void set_rotation(const quat &rotation);

	u32 id = 0;
	vec3 position = vec3(0,0,0);
	quat rotation = quat::identity();
	vec3 scale = vec3(1,1,1);
	float health = 0;
	ColliderRef collider;
	ModelRef model;
};
typedef std::shared_ptr<Entity> EntityRef;

// Entity management
EntityRef add_entity(EntityRef entity);

template <class T, class... Args>
std::shared_ptr<T> add_entity(Args&&... args) {
	return  std::dynamic_pointer_cast<T>(add_entity(std::make_shared<T>(args...)));
}
template <class T>
std::shared_ptr<T> add_entity() {
	return  std::dynamic_pointer_cast<T>(add_entity(std::make_shared<T>()));
}

void free_entity(EntityRef entity);
void free_entities();

EntityRef get_entity(u32 id);

void update_entities();
void draw_entities();
