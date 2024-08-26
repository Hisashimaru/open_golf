#include <vector>
#include "collision.h"
#include "gpu.h"


class CollisionQuadTreeSpace
{
public:
	void add(ColliderRef collider);
	void remove(ColliderRef collider);

	std::vector<ColliderRef> colliders;
	CollisionQuadTreeSpace *parent;
	CollisionQuadTreeSpace *children[4];
	bounds_t bounds;
};

void CollisionQuadTreeSpace::add(ColliderRef collider)
{
	colliders.push_back(collider);
	collider->space = this;
}

void CollisionQuadTreeSpace::remove(ColliderRef collider)
{
	colliders.erase(std::remove(colliders.begin(), colliders.end(), collider), colliders.end());
}









class CollisionQuadTree
{
public:
	void init(u32 level, const bounds_t &bounds);
	void uninit();
	void add(ColliderRef collider);
	void remove(ColliderRef collider);
	void update(ColliderRef collider);

	int query_colliders(const bounds_t &bounds, std::vector<ColliderRef> *list);
	void query_colliders_recursive(CollisionQuadTreeSpace *space, const bounds_t &bounds, std::vector<ColliderRef> *list);

protected:
	bool create_new_space(u32 elem);
	u32 bit_separete32(u32 n);
	u32 get_morton_number(u32 x, u32 y);
	u32 get_point_elem(float x, float y);
	u32 get_space_number(const bounds_t &bounds);

protected:
	std::vector<CollisionQuadTreeSpace*> spaces;
	const static u32 MAX_LEVEL = 8;
	u32 level = 0;
	u32 space_count_of_level[MAX_LEVEL+1];
	u32 space_count;
	bounds_t bounds;
	float width;
	float depth;
	float unit_w;
	float unit_d;
};


void CollisionQuadTree::init(u32 level, const bounds_t &bounds)
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

void CollisionQuadTree::uninit()
{
	for(int i=0; i<spaces.size(); i++)
	{
		if(spaces[i] == nullptr) continue;
		for(int c=0; c<spaces[i]->colliders.size(); c++)
		{
			free_collider(spaces[i]->colliders[c]);
		}
		spaces[i]->colliders.clear();
		delete spaces[i];
		spaces[i] = nullptr;
	}
}

void CollisionQuadTree::add(ColliderRef collider)
{
	bounds_t bounds = collider->get_bounds_world();
	u32 elem = get_space_number(bounds);
	if(elem < space_count)
	{
		if(!spaces[elem]){create_new_space(elem);}

		// find object layer
		auto space = spaces[elem];
		space->add(collider);
	}
}

void CollisionQuadTree::remove(ColliderRef collider)
{
	if(collider->space != nullptr)
	{
		collider->space->remove(collider);
		collider->space = nullptr;
	}
}

void CollisionQuadTree::update(ColliderRef collider)
{
	if(collider->space != nullptr)
	{
		collider->space->remove(collider);
		add(collider);
	}
}

int CollisionQuadTree::query_colliders(const bounds_t &bounds, std::vector<ColliderRef> *list)
{
	if(list == nullptr) return 0;

	list->clear();
	query_colliders_recursive(spaces[0], bounds, list);
	return (int)list->size();
}

void CollisionQuadTree::query_colliders_recursive(CollisionQuadTreeSpace *space, const bounds_t &bounds, std::vector<ColliderRef> *list)
{
	if(space == nullptr || list == nullptr) return;

	for(int i=0; i<space->colliders.size(); i++)
	{
		list->push_back(space->colliders[i]);
	}

	for(int i=0; i<4; i++) {
		if(space->children[i] && space->children[i]->bounds.intersects(bounds)) {
			query_colliders_recursive(space->children[i], bounds, list);
		}
	}
}

bool CollisionQuadTree::create_new_space(u32 elem)
{
	if(elem >= space_count) return false;

	// create the space
	if(!spaces[elem])
	{
		CollisionQuadTreeSpace *parent_space = nullptr;
		CollisionQuadTreeSpace *space = nullptr;
		spaces[elem] = new CollisionQuadTreeSpace();
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

u32 CollisionQuadTree::bit_separete32(u32 n)
{
	n = (n|(n<<8)) & 0x00ff00ff;
	n = (n|(n<<4)) & 0x0f0f0f0f;
	n = (n|(n<<2)) & 0x33333333;
	return (n|(n<<1)) & 0x55555555;
}

u32 CollisionQuadTree::get_morton_number(u32 x, u32 y)
{
	return bit_separete32(x) | (bit_separete32(y) << 1);
}

// woldspace to morton number
u32 CollisionQuadTree::get_point_elem(float x, float y)
{
	vec3 min = bounds.get_min();
	return get_morton_number((u32)((x-min.x)/unit_w), (u32)((y-min.z)/unit_d));
}

u32 CollisionQuadTree::get_space_number(const bounds_t &bounds)
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






static struct collision_ctx
{
	CollisionQuadTree tree;
} ctx = {};

void collision_init()
{
	ctx.tree.init(5, bounds_t(vec3(), vec3(500, 500, 500)));
}

void collision_uninit()
{
	ctx.tree.uninit();
}

ColliderRef create_sphere_collider(float radius, const vec3 &offset)
{
	ColliderRef c = std::make_shared<Collider>();
	c->enabled = true;
	c->type = ColliderType::SPHERE;
	c->layer = 1;
	c->offset = offset;
	c->sphere.radius = radius;
	c->bounds = bounds_t(offset, vec3(radius, radius, radius));
	ctx.tree.add(c);

	return c;
}

ColliderRef create_box_collider(const vec3 &center, const vec3 &size)
{
	ColliderRef c = std::make_shared<Collider>();
	c->enabled = true;
	c->type = ColliderType::BOX;
	c->layer = 1;
	c->offset = center;
	c->size = size;
	c->bounds = bounds_t(center, size * 0.5f);
	ctx.tree.add(c);

	return c;
}

ColliderRef create_capsule_collider(const vec3 &offset, const vec3 &dir, float radius, float height)
{
	ColliderRef c = std::make_shared<Collider>();
	c->enabled = true;
	c->type = ColliderType::CAPSULE;
	c->layer = 1;
	c->offset = offset;
	c->capsule.dir = dir.normalized();
	c->capsule.radius = radius;
	c->capsule.height = height;
	c->bounds = bounds_t();
	c->bounds.encapsulate(offset + (dir * height * 0.5f) + vec3(radius, radius, radius));
	c->bounds.encapsulate(offset - (dir * height * 0.5f) - vec3(radius, radius, radius));
	ctx.tree.add(c);

	return c;
}

static bounds_t calc_mesh_bounds(const vec3 *vertices, u32 count)
{
	if(count <= 0) return bounds_t();

	bounds_t b(vertices[0], vec3(0,0,0));
	for(u32 i=1; i<count; i++)
	{
		b.encapsulate(vertices[i]);
	}
	return b;
}

ColliderRef create_mesh_collider(const vec3 *vertices, u32 count, const vec3 &offset)
{
	if(!vertices || count <= 0) return nullptr;

	ColliderRef c = std::make_shared<Collider>();
	c->enabled = true;
	c->type = ColliderType::MESH;
	c->layer = 1;
	c->offset = offset;
	c->bounds = calc_mesh_bounds(vertices, count);
	
	c->mesh.vertices = new vec3[count];
	c->mesh.count = count;
	memcpy(c->mesh.vertices, vertices, sizeof(vec3)*count);
	ctx.tree.add(c);

	return c;
}

ColliderRef create_mesh_collider(const Vertex *vertices, u32 count, const vec3 &offset)
{
	if(!vertices || count <= 0) return nullptr;

	vec3 *points = new vec3[count];
	for(u32 i=0; i<count; i++)
	{
		points[i] = vertices[i].position;
	}
	ColliderRef c = create_mesh_collider(points, count, offset);
	delete [] points;

	return c;
}

ColliderRef create_mesh_collider(MeshRef mesh, const vec3 &offset)
{
	if(!mesh) return nullptr;

	const Vertex *vertices = mesh->get_vertices();
	u32 index_count = mesh->get_index_count();
	const u16 *indices = mesh->get_indices();
	vec3 *points = new vec3[index_count];
	for(u32 i=0; i<index_count; i++)
	{
		points[i] = vertices[indices[i]].position;
	}

	ColliderRef collider = create_mesh_collider(points, index_count, offset);
	delete [] points;
	return collider;
}

void free_collider(ColliderRef collider)
{
	if(collider == nullptr) return;

	ctx.tree.remove(collider);
}


int raycast(const ray_t &ray, RayHit *rayhit, u32 layermask)
{
	const int RAYHIT_COUNT = 64;
	RayHit rayhits[RAYHIT_COUNT] = {};
	if(raycast_all(ray, rayhits, RAYHIT_COUNT, layermask) > 0)
	{
		*rayhit = rayhits[0];
		return 1;
	}

	return 0;
}

int raycast_all(const ray_t &ray, RayHit *rayhit, int rayhit_count, u32 layermask)
{
	int hit_count = 0;
	static std::vector<ColliderRef> colliders;
	colliders.clear();

	bounds_t bounds;
	bounds.encapsulate(ray.pos);
	bounds.encapsulate(ray.pos + ray.dir);

	ctx.tree.query_colliders(bounds, &colliders);
	for(int i=0; i<(int)colliders.size(); i++)
	{
		ColliderRef c = colliders[i];
		RayHit hitinfo = {};
		if(c->intersect_ray(ray, &hitinfo))
		{
			hitinfo.collider = c;
			rayhit[hit_count] = hitinfo;
			hit_count++;
			if(hit_count >= rayhit_count) break;
		}
	}
	colliders.clear();

	// sort hit infomation with the hit distance
	for(int i=0; i<hit_count; i++){
		for(int j=i+1; j<hit_count; j++){
			if(rayhit[i].distance > rayhit[j].distance)
			{
				RayHit tmp = rayhit[i];
				rayhit[i] = rayhit[j];
				rayhit[j] = tmp;
			}
		}
	}

	return hit_count;
}

// http://marupeke296.com/COL_3D_No24_RayToSphere.html
bool ray_vs_sphere(const ray_t &ray, const vec3 &pos, float radius, RayHit *hitinfo)
{
	vec3 ray_normal = ray.dir.normalized();
	vec3 P = pos - ray.pos;
	float A = vec3::dot(ray_normal, ray_normal);
	float B = vec3::dot(ray_normal, P);
	float C = vec3::dot(P,P) - (radius*radius);

	if(A == 0.0f) return false;	// ray distance is 0

	float s = B * B - A * C;
	if(s < 0.0f) return false;	// not hited

	s = sqrtf(s);
	float a1 = (B - s) / A;
	//float a2 = (B + s) / A;

	if(a1 < 0.0f) return false;	// the ray direction is opposite to the sphere
	if(a1*a1 > ray.dir.sqrlen()) return false; // the hit point distance is longer than the ray

	hitinfo->distance = a1;
	hitinfo->point = ray.pos + ray.dir.normalized() * a1;
	hitinfo->normal = -(hitinfo->point - pos).normalized();

	return true;
}

static bool ray_vs_aabb(const ray_t &ray, const vec3 &pos, const vec3 &size, RayHit *hitinfo)
{
	vec3 ray_start = ray.pos - pos;
	vec3 ray_normal = ray.dir.normalized();
	vec3 t1 = (-size*0.5f) - ray_start;
	vec3 t2 = (size*0.5f) - ray_start;
	float nx = (ray_normal.x == 0.0f) ? FLOAT_MIN : ray_normal.x;
	float ny = (ray_normal.y == 0.0f) ? FLOAT_MIN : ray_normal.y;
	float nz = (ray_normal.z == 0.0f) ? FLOAT_MIN : ray_normal.z;
	t1.x /= nx; t1.y /= ny; t1.z /= nz;
	t2.x /= nx; t2.y /= ny; t2.z /= nz;

	float min = fmaxf(fmaxf(fminf(t1.x, t2.x), fminf(t1.y, t2.y)), fminf(t1.z, t2.z));
	float max = fminf(fminf(fmaxf(t1.x, t2.x), fmaxf(t1.y, t2.y)), fmaxf(t1.z, t2.z));

	if(max < 0.0f || min > max || min > ray.dir.len()) return false;

	hitinfo->point = ray.pos + ray.dir.normalized() * min;
	hitinfo->distance = min;

	// calc normal
	float min_dist = FLOAT_MAX;
	vec3 point = hitinfo->point - pos;
	float dist = fabsf(size.x*0.5f - fabsf(point.x));
	if(dist < min_dist){
		hitinfo->normal = vec3(1,0,0) * signf(point.x);
		min_dist = dist;
	}
	dist = fabsf(size.y*0.5f - fabsf(point.y));
	if(dist < min_dist){
		hitinfo->normal = vec3(0,1,0) * signf(point.y);
		min_dist = dist;
	}
	dist = fabsf(size.z*0.5f - fabsf(point.z));
	if(dist < min_dist){
		hitinfo->normal = vec3(0,0,1) * signf(point.z);
		min_dist = dist;
	}

	return true;
}

static bool ray_vs_box(const ray_t &ray, const vec3 &pos, const vec3 &size, const quat &rot, RayHit *hitinfo)
{
	mat4 mat = mat4(pos, rot, size);
	mat4 mat_inv = mat.inversed();
	ray_t newray;
	newray.pos = mat_inv.multiply_point(ray.pos);
	newray.dir = mat_inv.multiply_vector(ray.dir);
	if(ray_vs_aabb(newray, vec3(), vec3(1,1,1), hitinfo) == 0){return false;}
	hitinfo->point = mat.multiply_point(hitinfo->point);
	hitinfo->normal = rot * hitinfo->normal;
	return true;
}

static bool ray_vs_mesh(const ray_t &ray, const vec3 *vertices, u32 vertex_count, const mat4 &transform, RayHit *hitinfo)
{
	if(vertex_count % 3 != 0) return false; // Error vertex count is missmatch

	mat4 inv = transform.inversed();
	ray_t inv_ray;
	inv_ray.dir = inv.multiply_point_3x4(ray.dir);
	inv_ray.pos = inv.multiply_point_3x4(ray.pos);

	RayHit nearest_hit = {};
	nearest_hit.distance = FLOAT_MAX;
	bool hited = false;

	for(u32 i=0; i<=vertex_count-3; i+=3)
	{
		vec3 v0 = vertices[i];
		vec3 v1 = vertices[i+1];
		vec3 v2 = vertices[i+2];
		vec3 e1 = v1 - v0;
		vec3 e2 = v2 - v0;

		vec3 p = vec3::cross(inv_ray.dir.normalized(), e2);
		float det = vec3::dot(e1, p);
		if((det > -EPSILON) && (det < EPSILON)) continue;

		float invdet = 1.0f/det;

		vec3 tv = inv_ray.pos - v0;
		float u = vec3::dot(tv, p) * invdet;
		if((u < 0.0f) || (u > 1.0f)) continue;

		vec3 q = vec3::cross(tv, e1);
		float v = vec3::dot(inv_ray.dir.normalized(), q) * invdet;
		if((v < 0.0f) || ((u + v) > 1.0f)) continue;

		float t = vec3::dot(e2, q)*invdet;
		if(t > EPSILON && t*t < inv_ray.dir.sqrlen())
		{
			if(t < nearest_hit.distance)
			{
				nearest_hit.point = inv_ray.pos + inv_ray.dir.normalized() * t;
				nearest_hit.normal = vec3::cross(e1, e2).normalized();
				nearest_hit.distance = t;
				hited = true;
			}
		}
	}

	// transform the ray hit infomation local to world
	hitinfo->point = transform.multiply_point_3x4(nearest_hit.point);
	hitinfo->normal = transform.multiply_point_3x4(nearest_hit.normal).normalized();
	hitinfo->distance = transform.multiply_point_3x4(hitinfo->point - inv_ray.pos).len();
	return hited;
}



/////////////////////////////////////////////////////////////////////////////////////////
// Spherecast
bool spherecast_vs_sphere(const ray_t &ray, float rayradius, const vec3 &pos, float radius, RayHit *hitinfo)
{
	vec3 dir = ray.dir.normalized();
	float raylen = ray.dir.len();
	float d = vec3::dot(dir, pos - ray.pos);
	vec3 nearest_point;
	d = fmaxf(0.0f, fminf(raylen, d));	// clamp
	float h = (pos - dir * d).len();
	if(h < rayradius + radius)
	{
		float slope = rayradius + radius;
		float base = fabsf(sqrtf(slope*slope - h*h));
		vec3 p = dir * (d - base);
		hitinfo->normal = (p - pos).normalized();
		hitinfo->point = pos + hitinfo->normal * radius;
		hitinfo->distance = d - base;
		return true;
	}
	return false;
}

static bool check_point_in_triangle(const vec3& point, const vec3& p1, const vec3& p2, const vec3& p3)
{
	vec3 u, v, w, vw, vu, uw, uv;

	u = p2 - p1;
	v = p3 - p1;
	w = point - p1;

	vw = vec3::cross(v, w);
	vu = vec3::cross(v, u);

	if(vec3::dot(vw, vu) < 0.0f) {
		return false;
	}

	uw = vec3::cross(u, w);
	uv = vec3::cross(u, v);

	if(vec3::dot(uw, uv) < 0.0f) {
		return 0;
	}

	float d = uv.len();
	float r = vw.len() / d;
	float t = uw.len() / d;

	return ((r + t) <= 1);

}

bool get_lowest_root(float a, float b, float c, float maxR, float *root)
{
	// Check if a solution exists
	float determinant = b*b - 4.0f*a*c;

	// If determinant is negative it means no solutions.
	if (determinant < 0.0f) return false;

	// calculate the two roots: (if determinant == 0 then
	// x1==x2 but letfs disregard that slight optimization)
	float sqrtD = sqrtf(determinant);
	float r1 = (-b - sqrtD) / (2*a);
	float r2 = (-b + sqrtD) / (2*a);

	// Sort so x1 <= x2
	if (r1 > r2) {
		float temp = r2;
		r2 = r1;
		r1 = temp;
	}
	// Get lowest root:
	if (r1 > 0 && r1 < maxR) {
		*root = r1;
		return true;
	}
	// It is possible that we want x2 - this can happen
	// if x1 < 0
	if (r2 > 0 && r2 < maxR) {
		*root = r2;
		return true;
	}
	// No (valid) solutions
	return false;
}

static bool spherecast_to_triangle(const ray_t &ray, const vec3 &p1, const vec3 &p2, const vec3 &p3, RayHit *hitinfo)
{
	plane_t plane(p1, p2, p3);

	bool is_ray_crossing = vec3::dot(plane.normal, ray.dir.normalized()) < 0;
	if(!is_ray_crossing) return false;

	float t0, t1;
	bool embedded_in_plane = false;
	float distance_to_plane = plane.signed_distance_to(ray.pos);
	float normal_dot_velocity = vec3::dot(plane.normal, ray.dir);

	// if sphere is travelling parrallel to the palne
	if(normal_dot_velocity == 0.0f)
	{
		if(fabsf(distance_to_plane) >= 1.0f){
			// sphere is not embedded in plane.
			// No collision
			return false;
		}
		else
		{
			// sphere is embedded in plane
			embedded_in_plane = true;
			t0 = 0.0f;
			t1 = 1.0f;
		}
	}
	else
	{
		t0 = (-1.0f - distance_to_plane) / normal_dot_velocity;
		t1 = (1.0f - distance_to_plane) / normal_dot_velocity;
		if(t0 > t1){
			float tmp = t1;
			t1 = t0;
			t0 = tmp;
		}

		// check the range
		if(t0 > 1.0f || t1 < 0.0f){
			// no collision possible
			return false;
		}

		// clamp to 01
		t0 = clamp01(t0);
		t1 = clamp01(t1);
	}

	vec3 collision_point;
	bool found_collision = false;
	float t = 1.0;

	// check triangle face
	if(!embedded_in_plane)
	{
		vec3 plane_intersection_point = (ray.pos - plane.normal) + ray.dir * t0;
		if(check_point_in_triangle(plane_intersection_point, p1, p2, p3))
		{
			found_collision = true;
			t = t0;
			collision_point = plane_intersection_point;
		}
	}

	// check collision to vertex and edges
	if(found_collision == false)
	{
		vec3 velocity = ray.dir;
		vec3 base = ray.pos;
		float velocity_sqrlen = velocity.sqrlen();
		float a,b,c;
		float new_t;

		a = velocity_sqrlen;

		// p1
		b = 2.0f * vec3::dot(velocity, base - p1);
		c = vec3::dot(p1 - base, p1 - base) - 1.0f;
		if(get_lowest_root(a, b, c, t, &new_t))
		{
			t = new_t;
			found_collision = true;
			collision_point = p1;
		}

		// p2
		b = 2.0f * vec3::dot(velocity, base - p2);
		c = vec3::dot(p2 - base, p2 - base) - 1.0f;
		if(get_lowest_root(a, b, c, t, &new_t))
		{
			t = new_t;
			found_collision = true;
			collision_point = p2;
		}

		// p3
		b = 2.0f * vec3::dot(velocity, base - p3);
		c = vec3::dot(p3 - base, p3 - base) - 1.0f;
		if(get_lowest_root(a, b, c, t, &new_t))
		{
			t = new_t;
			found_collision = true;
			collision_point = p3;
		}


		// p1 -> p2
		vec3 edge = p2 - p1;
		vec3 base_to_vertex = p1 - base;
		float edge_sqrtlen = edge.sqrlen();
		float edge_dot_velocity = vec3::dot(edge, velocity);
		float edge_dot_basetovertex = vec3::dot(edge, base_to_vertex);

		// calculate parameters for equation
		a = edge_sqrtlen * -velocity_sqrlen + edge_dot_velocity * edge_dot_velocity;
		b = edge_sqrtlen * 2 * vec3::dot(velocity, base_to_vertex) - 2.0f * edge_dot_velocity * edge_dot_basetovertex;
		c = edge_sqrtlen * (1 - vec3::dot(base_to_vertex, base_to_vertex)) + edge_dot_basetovertex * edge_dot_basetovertex;
		if(get_lowest_root(a, b, c, t, &new_t))
		{
			// check if intersection is within line segment
			float f = (edge_dot_velocity * new_t - edge_dot_basetovertex) / edge_sqrtlen;
			if(f >= 0.0f && f <= 1.0f)
			{
				t = new_t;
				found_collision = true;
				collision_point = p1 + edge * f;
			}
		}


		// p2 -> p3
		edge = p3 - p2;
		base_to_vertex = p2 - base;
		edge_sqrtlen = edge.sqrlen();
		edge_dot_velocity = vec3::dot(edge, velocity);
		edge_dot_basetovertex = vec3::dot(edge, base_to_vertex);

		// calculate parameters for equation
		a = edge_sqrtlen * -velocity_sqrlen + edge_dot_velocity * edge_dot_velocity;
		b = edge_sqrtlen * 2 * vec3::dot(velocity, base_to_vertex) - 2.0f * edge_dot_velocity * edge_dot_basetovertex;
		c = edge_sqrtlen * (1 - vec3::dot(base_to_vertex, base_to_vertex)) + edge_dot_basetovertex * edge_dot_basetovertex;
		if(get_lowest_root(a, b, c, t, &new_t))
		{
			// check if intersection is within line segment
			float f = (edge_dot_velocity * new_t - edge_dot_basetovertex) / edge_sqrtlen;
			if(f >= 0.0f && f <= 1.0f)
			{
				t = new_t;
				found_collision = true;
				collision_point = p2 + edge * f;
			}
		}


		// p3 -> p1
		edge = p1 - p3;
		base_to_vertex = p3 - base;
		edge_sqrtlen = edge.sqrlen();
		edge_dot_velocity = vec3::dot(edge, velocity);
		edge_dot_basetovertex = vec3::dot(edge, base_to_vertex);

		// calculate parameters for equation
		a = edge_sqrtlen * -velocity_sqrlen + edge_dot_velocity * edge_dot_velocity;
		b = edge_sqrtlen * 2 * vec3::dot(velocity, base_to_vertex) - 2.0f * edge_dot_velocity * edge_dot_basetovertex;
		c = edge_sqrtlen * (1 - vec3::dot(base_to_vertex, base_to_vertex)) + edge_dot_basetovertex * edge_dot_basetovertex;
		if(get_lowest_root(a, b, c, t, &new_t))
		{
			// check if intersection is within line segment
			float f = (edge_dot_velocity * new_t - edge_dot_basetovertex) / edge_sqrtlen;
			if(f >= 0.0f && f <= 1.0f)
			{
				t = new_t;
				found_collision = true;
				collision_point = p3 + edge * f;
			}
		}
	}

	// set result
	if(found_collision)
	{
		float dist_to_collision = ray.dir.len() * t;
		if(hitinfo->distance > dist_to_collision)
		{
			hitinfo->point = collision_point;
			hitinfo->normal = ((ray.pos + ray.dir * t) - collision_point).normalized();
			hitinfo->distance = dist_to_collision;
		}
		return true;
	}

	return false;
}

bool spherecast_vs_AABB(const ray_t &ray, float rayradius, const vec3 &pos, const vec3 &size, RayHit *hitinfo)
{
	//static vec3 v[] = {
	//	vec3(-0.5f, 0.5f, 0.5f),
	//	vec3(0.5f, 0.5f, 0.5f),
	//	vec3(0.5f, 0.5f, -0.5f),
	//	vec3(-0.5f, 0.5f, -0.5f),
	//	vec3(-0.5f, -0.5f, 0.5f),
	//	vec3(0.5f, -0.5f, 0.5f),
	//	vec3(0.5f, -0.5f, -0.5f),
	//	vec3(-0.5f, -0.5f, -0.5f)
	//};

	//static int indices[] = {0,1, 1,2, 2,3, 3,0,
	//	0,4, 1,5, 2,6, 3,7,
	//	4,5, 5,6, 6,7, 7,4
	//};
	//const int indices_count = 24;

	//float dist = FLOAT_MAX;
	//vec3 p1;
	//vec3 p2;
	//vec3 ray_start = ray.pos;
	//vec3 ray_end = ray.pos + ray.dir;

	//// find the shortest distance between a line segment and AABB edges
	//for(int i=0; i<indices_count; i+=2)
	//{
	//	vec3 q1, q2;
	//	closest_point_line_segments(v[indices[i]]*size+pos, v[indices[i+1]]*size+pos, ray_start, ray_end, &q1, &q2);
	//	float d = (q2 - q1).sqrlen();
	//	if(d < dist)
	//	{
	//		dist = d;
	//		p1 = q1;
	//		p2 = q2;
	//	}
	//}

	//// find the shortest distance between a line segment and a surface
	//vec3 q1 = clamp(ray_start, vec3(-0.5f, -0.5f, -0.5f)*size+pos, vec3(0.5f, 0.5f, 0.5f)*size+pos);
	//float d = (q1 - ray_start).sqrlen();
	//if(d < dist)
	//{
	//	p1 = q1;
	//	p2 = ray_start;
	//}
	//q1 = clamp(ray_end, vec3(-0.5f, -0.5f, -0.5f)*size+pos, vec3(0.5f, 0.5f, 0.5f)*size+pos);
	//d = (q1 - ray_end).sqrlen();
	//if(d < dist)
	//{
	//	p1 = q1;
	//	p2 = ray_end;
	//}
	//return 0;
	return false;
}

bool spherecast_vs_box(const ray_t &ray, float rayradius, const vec3 &pos, const quat &rotation, const vec3 &size, RayHit *hitinfo)
{
	mat4 m = mat4(pos, rotation, vec3(1,1,1));
	mat4 inv = m.inversed().inversed();
	vec3 ray_start = inv.multiply_point(ray.pos);
	vec3 ray_dir = inv.multiply_vector(ray.dir);
	hitinfo = {};
	if(spherecast_vs_AABB(ray_t(ray_start, ray_dir), rayradius, vec3(), size, hitinfo))
	{
		hitinfo->point = m.multiply_point(hitinfo->point);
		hitinfo->normal = m.multiply_vector(hitinfo->normal);
		return true;
	}
	return false;
}

static bool spherecast_vs_capsule(const ray_t &ray, float radius, vec3 capsule_dir, float capsule_radius, float capsule_height, const mat4 &transform, RayHit *hitinfo)
{
	float r = radius + capsule_radius;
	mat4 inv = transform.inversed();
	ray_t inv_ray;
	inv_ray.dir = inv.multiply_vector(ray.dir)/r;
	inv_ray.pos = inv.multiply_point_3x4(ray.pos)/r;

	vec3 p1 = capsule_dir * capsule_height * 0.5f;
	vec3 p2 = -capsule_dir * capsule_height * 0.5f;
	p1 = p1/r;
	p2 = p2/r;

	// check collision
	bool found_collision = false;
	vec3 collision_point;
	vec3 velocity = inv_ray.dir;
	vec3 base = inv_ray.pos;
	float velocity_sqrlen = velocity.sqrlen();
	float a,b,c;
	float t = 1.0f;
	float new_t;

	a = velocity_sqrlen;

	// p1
	b = 2.0f * vec3::dot(velocity, base - p1);
	c = vec3::dot(p1 - base, p1 - base) - 1.0f;
	if(get_lowest_root(a, b, c, t, &new_t))
	{
		t = new_t;
		found_collision = true;
		collision_point = p1;
	}

	// p2
	b = 2.0f * vec3::dot(velocity, base - p2);
	c = vec3::dot(p2 - base, p2 - base) - 1.0f;
	if(get_lowest_root(a, b, c, t, &new_t))
	{
		t = new_t;
		found_collision = true;
		collision_point = p2;
	}


	// p1 -> p2
	vec3 edge = p2 - p1;
	vec3 base_to_vertex = p1 - base;
	float edge_sqrtlen = edge.sqrlen();
	float edge_dot_velocity = vec3::dot(edge, velocity);
	float edge_dot_basetovertex = vec3::dot(edge, base_to_vertex);

	// calculate parameters for equation
	a = edge_sqrtlen * -velocity_sqrlen + edge_dot_velocity * edge_dot_velocity;
	b = edge_sqrtlen * 2 * vec3::dot(velocity, base_to_vertex) - 2.0f * edge_dot_velocity * edge_dot_basetovertex;
	c = edge_sqrtlen * (1 - vec3::dot(base_to_vertex, base_to_vertex)) + edge_dot_basetovertex * edge_dot_basetovertex;
	if(get_lowest_root(a, b, c, t, &new_t))
	{
		// check if intersection is within line segment
		float f = (edge_dot_velocity * new_t - edge_dot_basetovertex) / edge_sqrtlen;
		if(f >= 0.0f && f <= 1.0f)
		{
			t = new_t;
			found_collision = true;
			collision_point = p1 + edge * f;
		}
	}

	// set result
	if(found_collision)
	{
		float dist_to_collision = inv_ray.dir.len() * t;
		hitinfo->point = transform.multiply_point_3x4(collision_point*r);
		hitinfo->distance = dist_to_collision * r;
		hitinfo->normal =  ((ray.pos + ray.dir.normalized() * hitinfo->distance) - hitinfo->point).normalized();
		hitinfo->point += hitinfo->normal * capsule_radius;
		return true;
	}

	return false;
}

static bool spherecast_vs_mesh(const ray_t &ray, float radius, const vec3 *vertices, u32 vertex_count, const mat4 &transform, RayHit *hitinfo)
{
	if(vertex_count % 3 != 0) return false; // Error vertex count is missmatch

	// convert ray to unit sphere space and then model sapce
	mat4 inv = transform.inversed();
	ray_t inv_ray;
	inv_ray.dir = inv.multiply_vector(ray.dir)/radius;
	inv_ray.pos = inv.multiply_point_3x4(ray.pos)/radius;

	RayHit nearest_hit = {};
	nearest_hit.distance = FLOAT_MAX;
	bool hited = false;

	for(u32 i=0; i<=vertex_count-3; i+=3)
	{
		RayHit hit = {};
		hit.distance = FLOAT_MAX;
		// convert triagle to unit sphere space
		if(spherecast_to_triangle(inv_ray, vertices[i]/radius, vertices[i+1]/radius, vertices[i+2]/radius, &hit))
		{
			hited = true;
			if(hit.distance < nearest_hit.distance)
			{
				nearest_hit = hit;
			}
		}
	}

	if(hited)
	{
		// convert hit result back from unit sphere space and then back from model space
		hitinfo->point = transform.multiply_point_3x4(nearest_hit.point*radius);
		hitinfo->normal = transform.multiply_point_3x4(nearest_hit.normal).normalized();
		hitinfo->distance = nearest_hit.distance * radius;
	}

	return hited;
}

int spherecast(const ray_t &ray, float radius, RayHit *rayhit, u32 layermask)
{
	const int RAYHIT_COUNT = 64;
	RayHit rayhits[RAYHIT_COUNT] = {};
	if(spherecast_all(ray, radius, rayhits, RAYHIT_COUNT, layermask) > 0)
	{
		if(rayhit)
			*rayhit = rayhits[0];
		return 1;
	}

	return 0;
}

int spherecast_all(const ray_t &ray, float radius, RayHit *rayhit, int rayhit_count, u32 layermask)
{
	int hit_count = 0;
	static std::vector<ColliderRef> colliders;
	colliders.clear();

	bounds_t bounds;
	bounds_t b;
	b.center = ray.pos;
	b.extents = vec3(radius, radius, radius);
	bounds.encapsulate(b);
	b.center = ray.pos + ray.dir;
	bounds.encapsulate(b);

	ctx.tree.query_colliders(bounds, &colliders);
	for(int i=0; i<(int)colliders.size(); i++)
	{
		ColliderRef c = colliders[i];
		RayHit hitinfo = {};
		hitinfo.distance = ray.dir.len();
		if(c->intersect_spherecast(ray, radius, &hitinfo))
		{
			hitinfo.collider = c;
			rayhit[hit_count] = hitinfo;
			hit_count++;
			if(hit_count >= rayhit_count) break;
		}
	}
	colliders.clear();

	// sort hit infomation with the hit distance
	for(int i=0; i<hit_count; i++){
		for(int j=i+1; j<hit_count; j++){
			if(rayhit[i].distance > rayhit[j].distance)
			{
				RayHit tmp = rayhit[i];
				rayhit[i] = rayhit[j];
				rayhit[j] = tmp;
			}
		}
	}

	return hit_count;
}


vec3 spherecast_slide(const vec3 &position, const vec3 &velocity, float radius, u32 layermask, int recursion)
{
	const float close_distance = 0.005f;

	// dont recurse if the new velocity is very small
	if(velocity.len() < close_distance) {
		return position;
	}

	if(recursion <= 0)
		return position;

	RayHit hitinfo;
	if(!spherecast(ray_t(position, velocity), radius, &hitinfo, layermask))
	{
		return position + velocity;
	}

	// found collision
	vec3 destination_point = position + velocity;
	vec3 new_base_point = position;
	if(hitinfo.distance >= close_distance)
	{
		vec3 v = velocity.normalized() * (hitinfo.distance - close_distance);
		new_base_point = position + v;
	}

	vec3 slide_plane_origin = hitinfo.point;
	vec3 slide_plane_normal = (new_base_point - hitinfo.point).normalized();
	plane_t slide_plane(slide_plane_origin, slide_plane_normal);
	vec3 new_destination_point = destination_point - slide_plane_normal * slide_plane.signed_distance_to(destination_point);
	vec3 new_velocity = new_destination_point - hitinfo.point;

	return spherecast_slide(new_base_point, new_velocity, radius, layermask, recursion-1);
}




Collider::~Collider()
{
	if(type == ColliderType::MESH)
	{
		delete[] mesh.vertices;
	}
}

bool Collider::intersect_ray(const ray_t &ray, RayHit *hitinfo)
{
	switch(type)
	{
	case ColliderType::SPHERE:
		return ray_vs_sphere(ray, position+offset, sphere.radius, hitinfo);
		break;
	case ColliderType::BOX:
		return ray_vs_box(ray, position+offset, size, rotation, hitinfo);
		break;
	case ColliderType::MESH:
		return ray_vs_mesh(ray, mesh.vertices, mesh.count, mat4(position, rotation, scale), hitinfo);
		break;
	default:
		break;
	}
	return false;
}

bool Collider::intersect_spherecast(const ray_t &ray, float radius, RayHit *hitinfo)
{
	switch(type)
	{
	case ColliderType::SPHERE:
		//return ray_vs_sphere(ray, position+offset, sphere.radius, hitinfo);
		break;
	case ColliderType::BOX:
		//return ray_vs_box(ray, position+offset, size, rotation, hitinfo);
		break;
	case ColliderType::CAPSULE:
		return spherecast_vs_capsule(ray, radius, capsule.dir, capsule.radius, capsule.height, mat4(offset+position, rotation, scale), hitinfo);
		break;
	case ColliderType::MESH:
		return spherecast_vs_mesh(ray, radius, mesh.vertices, mesh.count, mat4(position, rotation, scale), hitinfo);
		break;
	default:
		break;
	}
	return false;
}

bounds_t Collider::get_bounds_world() const
{
	vec3 min = -bounds.extents;
	vec3 max = bounds.extents;

	vec3 v[8] = {
		vec3(min.x, max.y, min.z),
		vec3(min.x, max.y, max.z),
		vec3(max.x, max.y, min.z),
		vec3(max.x, max.y, max.z),
		vec3(min.x, min.y, min.z),
		vec3(min.x, min.y, max.z),
		vec3(max.x, min.y, min.z),
		vec3(max.x, min.y, max.z)
	};

	bounds_t b;
	for(int i=0; i<8; i++)
	{
		v[i] = rotation * v[i];
		b.encapsulate(v[i]);
	}
	b.center = bounds.center + position;
	return b;
}

void Collider::set_position(const vec3 &position)
{
	this->position = position;
	ctx.tree.update(shared_from_this());
}

void Collider::set_rotation(const quat &rotation)
{
	this->rotation = rotation;
	ctx.tree.update(shared_from_this());
}