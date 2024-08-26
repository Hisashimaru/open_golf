#pragma once
#include <memory>
#include "mathf.h"

enum class ColliderType
{
	SPHERE,
	BOX,
	CAPSULE,
	MESH,
};

struct RayHit;
class CollisionQuadTreeSpace;
struct Collider : public std::enable_shared_from_this<Collider>
{
	u32 id;
	ColliderType type;
	u32 layer;
	vec3 position;
	quat rotation = quat::identity();
	vec3 scale = vec3(1,1,1);
	vec3 offset;
	bounds_t bounds;
	bool enabled;
	CollisionQuadTreeSpace *space;
	union {
		vec3 size;
		struct {float radius;} sphere;
		struct {vec3 dir; float radius; float height;} capsule;
		struct {vec3 *vertices; int count;} mesh;
	};

	struct UserData
	{
		void *data;
		u32	type;
	} user_data;

	Collider(){}
	~Collider();
	bool intersect_ray(const ray_t &ray, RayHit *hitinfo);
	bool intersect_spherecast(const ray_t &ray, float radius, RayHit *hitinfo);
	bounds_t get_bounds_world() const;

	void set_position(const vec3 &position);
	void set_rotation(const quat &rotation);
};
typedef std::shared_ptr<Collider> ColliderRef;

struct RayHit
{
	RayHit(){}

	vec3 point;
	vec3 normal;
	float distance;
	ColliderRef collider;
};

struct Collision
{
	vec3 point;
	vec3 normal;
	float depth;
	ColliderRef collider;
};

struct Vertex;
class Mesh;
typedef std::shared_ptr<Mesh> MeshRef;


void collision_init();
void collision_uninit();

// create colliders
ColliderRef create_sphere_collider(float radius, const vec3 &offset);
ColliderRef create_box_collider(const vec3 &center, const vec3 &size);
ColliderRef create_capsule_collider(const vec3 &offset, const vec3 &dir, float radius, float height);
ColliderRef create_mesh_collider(const vec3 *vertices, u32 count, const vec3 &offset=vec3());
ColliderRef create_mesh_collider(const Vertex *vertices, u32 count, const vec3 &offset=vec3());
ColliderRef create_mesh_collider(MeshRef mesh, const vec3 &offset=vec3());

void free_collider(ColliderRef collider);

// raycast
int raycast(const ray_t &ray, RayHit *rayhit, u32 layermask=0xFFFFFFFF);
int raycast_all(const ray_t &ray, RayHit *rayhit, int rayhit_count, u32 layermask=0xFFFFFFFF);

int spherecast(const ray_t &ray, float radius, RayHit *rayhit, u32 layermask=0xFFFFFFFF);
int spherecast_all(const ray_t &ray, float radius, RayHit *rayhit, int rayhit_count, u32 layermask=0xFFFFFFFF);
vec3 spherecast_slide(const vec3 &position, const vec3 &velocity, float radius, u32 layermask=0xFFFFFFFF, int recursion=8);
