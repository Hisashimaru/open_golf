#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include "gpu.h"
#include "material.h"
#include "animation.h"

struct Bone
{
	std::string name;
	mat4 inv_matrix;
	mat4 matrix;
	std::vector<int> children;
	int parent;
};

class Model
{
public:
	Model(){}
	~Model(){}

	bool load(const char *filename);
	void update();

	void play(const char *name, bool loop=false);

	MeshRef mesh;
	std::vector<MaterialRef> materials;
	std::vector<Bone> bones;
	Animator animator;
};
typedef std::shared_ptr<Model> ModelRef;

ModelRef create_model();
ModelRef create_model(MeshRef mesh, MaterialRef material);
ModelRef load_model(const char *filename);


// Mesh

MeshRef load_mesh(const char *filename);

MeshRef mesh_create_plane(float width, float depth);
MeshRef create_box_mesh(const vec3 &size);
MeshRef create_sphere_mesh(float radius, int slice=32, int stack=16);