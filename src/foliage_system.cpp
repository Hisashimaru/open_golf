#include <stdio.h>
#include <vector>
#include <unordered_map>
#include "mathf.h"
#include "foliage_system.h"
#include "renderer.h"
#include "collision.h"
#include "common.h"
#include "entity/entity.h"
#include "entity/tree.h"
#include "particle.h"
#include "sound.h"


struct FoliageTransforms
{
	std::vector<mat4> transforms;
};


static struct foliage_system_ctx
{
	ModelRef grass;
	ParticleEmitterRef particle_grass;
	ParticleEmitterRef particle_damage_tree;

	SoundRef choping_sound;
	SoundRef mowing_sound;
} ctx = {};

static FoliageDesc foliage_descs[(int)FoliageType::max_types];


struct FoliageQuadTreeSpace;
struct FoliageObject
{
	FoliageQuadTreeSpace *space;
	FoliageType type;
	transform_t transform;
	ColliderRef collider;
	float health;
	u32 id;
};

struct FoliageQuadTreeSpace
{

	void add(const FoliageObject &obj) {
		int layer = -1;
		for(int i=0; i<layer_types.size(); i++)
		{
			if(layer_types[i] == obj.type) {
				layer = i;
				break;
			}
		}

		if(layer == -1) {
			layer = (int)layer_types.size();
			layer_types.resize(layer+1);
			objects.resize(layer+1);
			transforms.resize(layer+1);
		}

		layer_types[layer] = obj.type;
		objects[layer].push_back(obj.id);
		transforms[layer].push_back(obj.transform.to_mat4());
	}

	void remove(const FoliageObject &obj) {
		int layer = -1;
		for(int i=0; i<layer_types.size(); i++)
		{
			if(layer_types[i] == obj.type) {
				layer = i;
				break;
			}
		}
		if(layer == -1) return;

		for(int i=0; i<objects[layer].size(); i++)
		{
			if(objects[layer][i] == obj.id)
			{
				// update data
				objects[layer][i] = objects[layer].back();
				objects[layer].pop_back();
				transforms[layer][i] = transforms[layer].back();
				transforms[layer].pop_back();

				if(objects[layer].size() == 0)
				{
					objects[layer] = objects.back();
					objects.pop_back();
					transforms[layer] = transforms.back();
					transforms.pop_back();
					layer_types[layer] = layer_types.back();
					layer_types.pop_back();
				}
				break;
			}
		}
	}

	void draw()
	{
		for(int i=0; i<layer_types.size(); i++) {
			draw_model(foliage_descs[(int)layer_types[i]].model, (u32)transforms[i].size(), transforms[i].data());
		}
	}

	std::vector<std::vector<u32>> objects;
	std::vector<FoliageType> layer_types;
	std::vector<std::vector<mat4>> transforms;
	FoliageQuadTreeSpace *parent;
	FoliageQuadTreeSpace *children[4];
	bounds_t bounds;
};

static struct FoliageQuadTree
{
	void init(u32 level, const bounds_t &bounds);
	u32 add(FoliageObject obj);

	void remove(u32 id)
	{
		if(id >= objects.size()) return;
		remove(objects[id]);
	}

	void remove(const FoliageObject &obj)
	{
		obj.space->remove(obj);

		if(obj.id == objects.size()-1)
		{
			objects.pop_back();
		}
		else
		{
			objects[obj.id] = {};
			free_id.push_back(obj.id);
		}
	}

	void clear()
	{
		for(u32 i=0; i<space_count; i++)
		{
			delete spaces[i];
			spaces[i] = nullptr;
		}

		for(int i=0; i<objects.size(); i++)
		{
			free_collider(objects[i].collider);
		}

		objects.clear();
		free_id.clear();
	}

	std::vector<FoliageObject> query_objects(const bounds_t &bounds)
	{
		static std::vector<FoliageQuadTreeSpace*> space_stack;
		space_stack.clear();
		std::vector<FoliageObject> obj_list;
		u32 n = get_space_number(bounds);

		if(spaces[n]) {
			space_stack.push_back(spaces[n]);
		} else {
			return obj_list;
		}

		// collect children
		for(int i=0; i<space_stack.size(); i++)
		{
			for(int k=0; k<4; k++)
			{
				if(space_stack[i]->children[k] && space_stack[i]->children[k]->bounds.intersects(bounds)) {
					space_stack.push_back(space_stack[i]->children[k]);
				}
			}
		}

		// collect parents
		auto parent = space_stack[0]->parent;
		while(parent)
		{
			space_stack.push_back(parent);
			parent = parent->parent;
		}

		// collect objects
		for(int i=0; i<space_stack.size(); i++)
		{
			auto space = space_stack[i];
			for(int layer=0; layer<space->layer_types.size(); layer++) {
				for(int k=0; k<space->objects[layer].size(); k++) {
					obj_list.push_back(objects[space->objects[layer][k]]);
				}
			}
		}

		//query_objects_recursive(spaces[n], bounds, &obj_list);
		return obj_list;
	}

	void draw()
	{
		for(int i=0; i<spaces.size(); i++)
		{
			if(spaces[i] == nullptr) continue;
			spaces[i]->draw();
		}
	}

	void optimize_memory()
	{
		for(int i=0; i<free_id.size(); i++)
		{
			objects[free_id[i]] = objects.back();
			objects.pop_back();
		}
		free_id.clear();
	}

	const std::vector<FoliageObject>* get_objects(){return &objects;}
	FoliageObject* get_object(u32 id){return id < objects.size() ? &objects[id] : nullptr;}

protected:
	bool create_new_space(u32 elem);
	u32 bit_separete32(u32 n);
	u32 get_morton_number(u32 x, u32 y);
	u32 get_point_elem(float x, float y);
	u32 get_space_number(const bounds_t &bounds);

	void query_objects_recursive(FoliageQuadTreeSpace *space, const bounds_t &bounds, std::vector<FoliageObject>  *list)
	{
		if(space == nullptr) return;

		for(int layer=0; layer<space->layer_types.size(); layer++) {
			for(int i=0; i<space->objects[layer].size(); i++) {
				list->push_back(objects[space->objects[layer][i]]);
			}
		}

		for(int i=0; i<4; i++) {
			if(space->children[i] && space->children[i]->bounds.intersects(bounds)) {
				query_objects_recursive(space->children[i], bounds, list);
			}
		}
	}


	std::vector<FoliageQuadTreeSpace*> spaces;
	const static u32 MAX_LEVEL = 8;
	u32 level = 0;
	u32 space_count_of_level[MAX_LEVEL+1];
	u32 space_count;
	bounds_t bounds;
	float width;
	float depth;
	float unit_w;
	float unit_d;

	std::vector<FoliageObject> objects;
	std::vector<u32> free_id;
} quad_tree;

void FoliageQuadTree::init(u32 level, const bounds_t &bounds)
{
	this->level = level <= MAX_LEVEL ? level : MAX_LEVEL;

	// calculate space count
	space_count_of_level[0] = 1;
	for(int i=1; i<MAX_LEVEL+1; i++)
	{
		space_count_of_level[i] = space_count_of_level[i-1] * 4;
	}

	space_count = (space_count_of_level[this->level+1]-1)/3;
	spaces.resize(space_count, nullptr);


	this->bounds = bounds;
	this->bounds.extents.y = FLOAT_MAX;
	width = bounds.extents.x * 2.0f;
	depth = bounds.extents.y * 2.0f;
	unit_w = width/(1<<this->level);
	unit_d = depth/(1<<this->level);
}

u32 FoliageQuadTree::add(FoliageObject obj)
{
	const FoliageDesc &desc = foliage_descs[(int)obj.type];
	float r = desc.radius;
	bounds_t bounds(obj.transform.pos, vec3(r,r,r));
	u32 elem = get_space_number(bounds);
	if(elem < space_count)
	{
		if(!spaces[elem]){create_new_space(elem);}

		// find object layer
		auto space = spaces[elem];

		FoliageObject regobj;
		obj.space = space;

		// add object
		if(free_id.size() > 0) {
			// add object to free space
			obj.id = free_id.back();
			free_id.pop_back();
			objects[obj.id] = obj;
		}
		else
		{
			// add object to the tail
			obj.id = (u32)objects.size();
			objects.push_back(obj);
		}
		space->add(obj);

		// set collider user data
		if(obj.collider)
		{
			obj.collider->user_data.type = (int)ColliderUserDataType::FoliageObject;
			obj.collider->user_data.data = (void*)((u64)obj.id);
		}
	}

	return obj.id;
}

bool FoliageQuadTree::create_new_space(u32 elem)
{
	if(elem >= space_count) return false;

	// create the space
	if(!spaces[elem])
	{
		FoliageQuadTreeSpace *parent_space = nullptr;
		FoliageQuadTreeSpace *space = nullptr;
		spaces[elem] = new FoliageQuadTreeSpace();
		space = spaces[elem];

		if(elem > 0)
		{
			// create the parent space
			u32 parent_elem = (elem-1)>>2;	// get a element of the parent space
			create_new_space(parent_elem);
			parent_space = spaces[parent_elem];
		}
		space->parent = parent_space;

		// set the bounds of the space
		bounds_t bounds;
		if(elem > 0)
		{
			u32 n = (elem-1)%4;
			parent_space->children[n] = space;
			float width = parent_space->bounds.extents.x;
			float depth = parent_space->bounds.extents.z;
			bounds.extents = vec3(width*0.5f, FLOAT_MAX, depth*0.5f);
			if(n == 0)	// left-top
			{
				bounds.center = parent_space->bounds.center + vec3(-width*0.5f, 0.0f, -depth*0.5f);
			}
			else if(n == 1)	// right-top
			{
				bounds.center = parent_space->bounds.center + vec3(width*0.5f, 0.0f, -depth*0.5f);
			}
			else if(n == 2)	// left-bottom
			{
				bounds.center = parent_space->bounds.center + vec3(-width*0.5f, 0.0f, depth*0.5f);
			}
			else if(n == 3)	// right-bottom
			{
				bounds.center = parent_space->bounds.center + vec3(width*0.5f, 0.0f, depth*0.5f);
			}
		}
		else
		{
			// root space
			bounds = this->bounds;
		}
		spaces[elem]->bounds = bounds;
	}
	return true;
}

u32 FoliageQuadTree::bit_separete32(u32 n)
{
	n = (n|(n<<8)) & 0x00ff00ff;
	n = (n|(n<<4)) & 0x0f0f0f0f;
	n = (n|(n<<2)) & 0x33333333;
	return (n|(n<<1)) & 0x55555555;
}

u32 FoliageQuadTree::get_morton_number(u32 x, u32 y)
{
	return bit_separete32(x) | (bit_separete32(y) << 1);
}

// woldspace to morton number
u32 FoliageQuadTree::get_point_elem(float x, float y)
{
	vec3 min = bounds.get_min();
	return get_morton_number((u32)((x-min.x)/unit_w), (u32)((y-min.z)/unit_d));
}

u32 FoliageQuadTree::get_space_number(const bounds_t &bounds)
{
	vec3 min = bounds.get_min();
	vec3 max = bounds.get_max();
	u32 lt = get_point_elem(min.x, min.z);
	u32 rb = get_point_elem(max.x, max.z);
	u32 def = rb ^ lt;
	u32 hilevel = 0;
	for(u32 i=0; i<level; i++)
	{
		u32 check = (def>>(i*2)) &0x3;
		if(check != 0)
			hilevel = i+1;
	}

	u32 space_num = rb>>(hilevel*2);
	u32 addnum = (space_count_of_level[level - hilevel]-1)/3;
	space_num += addnum;

	if(space_num > space_count)
		return 0xffffffff;

	return space_num;
}





void foliage_init()
{
	quad_tree.init(4, bounds_t(vec3(0,0,0), vec3(500, 500, 500)));
	foliage_clear();

	// Skip model loading, if the model has already been loaded
	if(ctx.grass) return;

	// load models
	FoliageDesc *desc = &foliage_descs[(int)FoliageType::grass1];
	strcpy(desc->name, "Grass 1");
	desc->type = FoliageType::grass1;
	desc->model = load_model("grass");
	desc->radius = 0.4f;
	desc->grass_object = true;

	// bush
	MaterialRef mat = create_material("unlit");
	desc = &foliage_descs[(int)FoliageType::bush1];
	strcpy(desc->name, "Bush 1");
	desc->type = FoliageType::bush1;
	desc->model = load_model("data/models/bush.obj");
	mat = create_material("unlit");
	mat->render_mode = Material::CUTOUT;
	mat->texture = gpu_load_texture("data/models/bush.png");
	desc->model->materials.push_back(mat);
	desc->radius = 1.8f;

	// tree
	desc = &foliage_descs[(int)FoliageType::tree1];
	strcpy(desc->name, "Tree 1");
	desc->type = FoliageType::tree1;
	desc->radius = 2.0f;
	desc->collider.type = ColliderType::CAPSULE;
	desc->collider.center = vec3(0, 5, 0);
	desc->collider.capsule.dir = vec3(0,1,0);
	desc->collider.capsule.radius = 0.4f;
	desc->collider.capsule.height = 10.0f;
	desc->health = 4.0f;
	desc->model = load_model("tree");

	
	// load particles
	ctx.particle_grass = load_particle("data/particles/grass.fx");
	ctx.particle_grass->position = vec3(19,8,-46);
	ctx.particle_damage_tree = load_particle("data/particles/woodchips.fx");


	ctx.mowing_sound = sound_load("data/sounds/mowing_grass.wav");
	ctx.choping_sound = sound_load("data/sounds/chop_wood.wav");
}

void foliage_uninit()
{
	quad_tree.clear();
	quad_tree = {};
}

void foliage_clear()
{
	quad_tree.clear();
}

static u32 hash(float x, float y, float z)
{
	return ((u32)x * 92837111) ^ ((u32)y * 689287499) ^ ((u32)z * 283923481);
}

static float spacial_rand_value(vec3 position)
{
	static u32 table[256] = {36, 102, 45, 194, 188, 241, 32, 141, 115, 97, 117, 82, 143, 209, 1, 112, 158, 169,
		213, 77, 223, 253, 43, 133, 238, 76, 40, 90, 222, 177, 139, 95, 83, 219, 55, 191, 144, 26,
		203, 37, 232, 221, 0, 17, 100, 59, 138, 11, 204, 134, 38, 71, 207, 84, 114, 235, 210, 23,
		248, 251, 130, 81, 183, 201, 145, 93, 31, 151, 9, 6, 152, 94, 127, 99, 176, 61, 54, 212,
		51, 22, 142, 192, 33, 19, 208, 189, 74, 157, 88, 24, 60, 147, 64, 50, 202, 181, 53, 250,
		215, 186, 228, 150, 105, 30, 69, 140, 35, 200, 224, 107, 27, 57, 185, 225, 92, 155, 226,
		220, 78, 164, 87, 66, 172, 132, 116, 67, 126, 42, 246, 217, 146, 70, 108, 171, 2, 242,
		166, 96, 52, 62, 44, 121, 240, 167, 89, 214, 16, 124, 129, 197, 41, 216, 49, 8, 211, 72,
		120, 46, 170, 48, 122, 174, 153, 104, 68, 5, 125, 101, 230, 205, 187, 179, 58, 182, 21,
		65, 249, 137, 12, 243, 252, 165, 85, 245, 86, 254, 123, 7, 154, 47, 4, 28, 136, 34, 14,
		15, 161, 135, 79, 218, 29, 25, 131, 10, 56, 156, 234, 119, 63, 229, 233, 91, 103, 39, 190,
		118, 3, 198, 113, 75, 244, 163, 80, 178, 160, 173, 227, 106, 196, 149, 148, 175, 255, 236,
		18, 206, 168, 128, 231, 247, 111, 13, 110, 180, 73, 109, 162, 193, 199, 98, 184, 195, 237, 20, 239, 159};
	u32 h = hash(position.x, position.y, position.z);
	return (float)table[h%255] / (float)255;
}

void foliage_add(FoliageType type, const vec3 &position)
{
	// add collider
	FoliageDesc *desc = &foliage_descs[(int)type];
	ColliderRef collider = nullptr;
	if(desc->collider.type == ColliderType::BOX)
	{
		collider = create_box_collider(desc->collider.center, desc->collider.box.size);
	}
	else if(desc->collider.type == ColliderType::CAPSULE)
	{
		collider = create_capsule_collider(desc->collider.center, desc->collider.capsule.dir, desc->collider.capsule.radius, desc->collider.capsule.height);
	}

	if(collider)
	{
		collider->set_position(position);
	}

	// add the object to the object main list
	float scale = (spacial_rand_value(position) * 0.5f) + 0.5f;
	FoliageObject obj = {};
	obj.type = type;
	obj.transform = transform_t(position, quat::identity(), vec3(1,scale,1));
	obj.collider = collider;
	obj.health = desc->health;
	quad_tree.add(obj);
}

void foliage_add(FoliageType type, const vec3 &position, float radius, float spacing_factor)
{
	const int count = 10;
	for(int i=0; i<count; i++)
	{
		vec3 p = rand_in_sphere(radius) + position;
		p.y = position.y;

		bool overlapped = false;
		float object_radius = foliage_descs[(int)type].radius;
		std::vector<FoliageObject> objects = quad_tree.query_objects(bounds_t(p, vec3(object_radius, object_radius, object_radius)));
		for(int k=0; k<objects.size(); k++)
		{
			FoliageType t = objects[k].type;
			float other_radius = foliage_descs[(int)t].radius;
			float dist = (p - objects[k].transform.pos).len();
			if(dist < object_radius + other_radius)
			{
				overlapped = true;
				break;
			}
		}

		if(!overlapped)
		{
			ray_t ray = ray_t(p + vec3(0,1,0), vec3(0,-2,0));
			RayHit hitinfo;
			if(raycast(ray, &hitinfo))
				foliage_add(type, hitinfo.point);
		}
	}
}

void foliage_add_replace(FoliageType type, const vec3 &position, float radius)
{
	const int count = 1;
	for(int i=0; i<count; i++)
	{
		vec3 p = rand_in_sphere(radius) + position;
		p.y = position.y;

		foliage_replace(type, p);
	}
}

void foliage_replace(FoliageType type, const vec3 &position)
{
	foliage_remove(position, foliage_descs[(int)type].radius);
	foliage_add(type, position);
}

void foliage_remove(u32 id)
{
	FoliageObject *obj = quad_tree.get_object(id);
	if(obj != nullptr)
	{
		free_collider(obj->collider);
		quad_tree.remove(*obj);
	}
}

void foliage_remove(const vec3 &position, float radius)
{
	std::vector<FoliageObject> objects = quad_tree.query_objects(bounds_t(position, vec3(radius, radius, radius)));
	for(int i=0; i<objects.size(); i++)
	{
		FoliageType type = objects[i].type;
		float r = foliage_descs[(int)type].radius;
		if((objects[i].transform.pos - position).sqrlen() < radius * radius + r * r)
		{
			foliage_remove(objects[i].id);
		}
	}
}

void foliage_take_damage(u32 id, vec3 dir, float damage)
{
	FoliageObject *obj = quad_tree.get_object(id);
	ctx.particle_damage_tree->emit(obj->transform.pos + obj->transform.rot.up(), quat::identity());
	ctx.choping_sound->emit();
	obj->health -= damage;

	if(obj->health <= 0)
	{
		std::shared_ptr<Tree> tree = add_entity<Tree>();
		tree->position = obj->transform.pos;
		tree->rotation = obj->transform.rot;
		foliage_remove(id);

		tree->angular_velocity = vec3::cross(vec3(0,1,0), dir.normalized()) * 30.0f;
	}
}

void foliage_mowing(const vec3 &position, float radius)
{
	std::vector<FoliageObject> objects = quad_tree.query_objects(bounds_t(position, vec3(radius, radius, radius)));
	bool mowed = false;
	for(int i=0; i<objects.size(); i++)
	{
		FoliageType type = objects[i].type;
		const FoliageDesc *desc = &foliage_descs[(int)type];
		float r = desc->radius;
		if(desc->grass_object && (objects[i].transform.pos - position).sqrlen() < radius * radius + r * r)
		{
			foliage_remove(objects[i].id);
			ctx.particle_grass->emit(objects[i].transform.pos, quat::euler(-90,0,0));
			mowed = true;
		}
	}

	if(mowed)
	{
		ctx.mowing_sound->emit();
	}
}

void foliage_draw()
{
	quad_tree.draw();
}

void foliage_save(FILE *fp)
{
	// write foliage type count
	quad_tree.optimize_memory();
	const std::vector<FoliageObject> *objects = quad_tree.get_objects();
	int object_count = (int)objects->size();

	// collect data
	std::vector<FoliageObject> objlist[(int)FoliageType::max_types];
	for(int i=0; i<object_count; i++)
	{
		FoliageObject o = objects->at(i);
		objlist[(int)o.type].push_back(o);
	}


	fwrite(&object_count, sizeof(int), 1, fp);
	for(int i=1; i<(int)FoliageType::max_types; i++)
	{
		int count = (int)objlist[i].size();
		if(count == 0) continue;

		// write foliage type
		int type = i;
		fwrite(&type, sizeof(int), 1, fp);
		fwrite(&count, sizeof(int), 1, fp);

		// write foliage object position
		for(int k=0; k<count; k++)
		{
			vec3 pos = objlist[i][k].transform.pos;
			fwrite(&pos, sizeof(vec3), 1, fp);
		}
	}
}

void foliage_load(FILE *fp)
{
	foliage_clear();

	// read foliage type count
	int foliage_type_count = 0;
	fread(&foliage_type_count, sizeof(int), 1, fp);

	for(int i=0; i<foliage_type_count; i++)
	{
		// write foliage type
		int type = 0;
		fread(&type, sizeof(int), 1, fp);
		int count = 0;
		fread(&count, sizeof(int), 1, fp);

		// write foliage object position
		for(int k=0; k<count; k++)
		{
			vec3 pos;
			fread(&pos, sizeof(vec3), 1, fp);
			foliage_add((FoliageType)type, pos);
		}
	}
}

const FoliageDesc* foliage_get_descs()
{
	return foliage_descs;
}