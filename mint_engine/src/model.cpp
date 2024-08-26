#include "model.h"
#include "external/par_shapes.h"
#include "resource_manager.h"

ModelRef create_model()
{
	ModelRef model = std::make_shared<Model>();
	model->materials.push_back(load_material("unlit"));

	return model;
}

ModelRef create_model(MeshRef mesh, MaterialRef material)
{
	ModelRef model = std::make_shared<Model>();
	model->mesh = mesh;
	if(material){model->materials.push_back(material);}
	return model;
}

ModelRef _load_smd(const char *filename);
bool Model::load(const char *filename)
{
	const char *ext = strrchr(filename, '.');
	if(ext != nullptr)
	{
		if(strcmp(ext, ".obj") == 0)
		{
			MeshRef mesh = load_mesh(filename);
			if(mesh == nullptr) return false;

			this->mesh = mesh;
			return true;
		}
		else if(strcmp(ext, ".smd") == 0)
		{
			ModelRef model = _load_smd(filename);
			*this = *(model.get());	// copy values
			return true;
		}
	}

	return false;
}

void Model::update()
{
	if(bones.size() > 0 && animator.current_animation)
	{
		animator.update();
		static std::vector<int> bone_stack;
		bone_stack.clear();

		for(int i=0; i<bones.size(); i++)
		{
			int frame = animator.frame;
			auto it = animator.current_animation->animation_keys.find(bones[i].name);
			if(it == animator.current_animation->animation_keys.end()){ continue; }
			if(frame >= it->second->keys.size())
			{
				frame = (int)it->second->keys.size()-1;
			}
			bones[i].matrix = it->second->keys[frame].to_mat4();

			if(bones[i].parent == -1){ bone_stack.push_back(i); }
		}

		int idx = 0;
		while(idx < bone_stack.size())
		{
			int parent_idx = bone_stack[idx];
			for(int i=0; i<bones[parent_idx].children.size(); i++)
			{
				int child_idx = bones[parent_idx].children[i];
				bones[child_idx].matrix = bones[child_idx].matrix * bones[parent_idx].matrix;
				bone_stack.push_back(child_idx);
			}
			idx++;
		}
	}
}

void Model::play(const char *name, bool loop)
{
	animator.play(name, loop);
}


static MeshRef _load_obj(const char *filename);
MeshRef load_mesh(const char *filename)
{
	const char *ext = strrchr(filename, '.');
	if(ext != nullptr)
	{
		if(strcmp(ext, ".obj") == 0)
		{
			return _load_obj(filename); 
		}
	}

	return nullptr;
}

static MeshRef _load_obj(const char *filename)
{
	MeshRef tmp_mesh = res_get_mesh(filename);
	if(tmp_mesh)
		return tmp_mesh;

	// get file path
	char filepath[256] = {};
	strcpy(filepath, filename);
	const char *obj_name_point = strrchr(filename, '@');
	if(obj_name_point != nullptr)
	{
		filepath[obj_name_point - filepath] = '\0';
	}

	FILE* fp = nullptr;
	fp = fopen(filepath, "r");
	if(fp == NULL){return nullptr;}

	std::vector<vec3> vertices;
	std::vector<vec2> uvs;
	std::vector<vec3> normals;
	std::vector<u16> indices;
	int offset = 0;
	std::vector<Vertex> verts;

	char line[1024];
	char type[32];

	bool hasUV = false;
	bool hasNormal = false;
	while(fgets(line, 1024, fp) != NULL)
	{
		sscanf(line, "%s", &type);
		if (strcmp(type, "v") == 0)
		{
			vec3 v;
			sscanf(line, "%s %f %f %fn", &type, &v.x, &v.y, &v.z);
			vertices.push_back(v);
		}
		else if (strcmp(type, "vt") == 0)
		{
			hasUV = true;
			vec2 uv;
			sscanf(line, "%s %f %fn", &type, &uv.x, &uv.y);
			uvs.push_back(uv);
		}
		else if (strcmp(type, "vn") == 0)
		{
			hasNormal = true;
			vec3 n;
			sscanf(line, "%s %f %f %fn", &type, &n.x, &n.y, &n.z);
			normals.push_back(n);
		}
		else if (strcmp(type, "f") == 0)
		{
			int spaces = 0;
			// Count the space of separater
			for (int i = 0; i < 256; i++)
			{
				if (line[i] == ' ')
					spaces++;
				else if (line[i] == '\n')
					break;
			}

			u32 vindex[4], uvindex[4], nindex[4];
			int vcount = 0;
			if (spaces == 3)
			{
				// read as triangle
				vcount = 3;
				if (hasUV && hasNormal)
				{
					sscanf(line, "%s %d/%d/%d %d/%d/%d %d/%d/%dn", &type,
						&vindex[0], &uvindex[0], &nindex[0],
						&vindex[1], &uvindex[1], &nindex[1],
						&vindex[2], &uvindex[2], &nindex[2]);
				}
				else if (hasNormal)
				{
					sscanf(line, "%s %d//%d %d//%d %d//%dn", &type,
						&vindex[0], &nindex[0],
						&vindex[1], &nindex[1],
						&vindex[2], &nindex[2]);
				}
			}
			else if (spaces == 4)
			{
				// read as quad
				vcount = 4;
				if (hasUV && hasNormal)
				{
					sscanf(line, "%s %d/%d/%d %d/%d/%d %d/%d/%d %d/%d/%dn", &type,
						&vindex[0], &uvindex[0], &nindex[0],
						&vindex[1], &uvindex[1], &nindex[1],
						&vindex[2], &uvindex[2], &nindex[2],
						&vindex[3], &uvindex[3], &nindex[3]);
				}
				else if(hasNormal)
				{
					sscanf(line, "%s %d//%d %d//%d %d//%d %d//%dn", &type,
						&vindex[0], &nindex[0],
						&vindex[1], &nindex[1],
						&vindex[2], &nindex[2],
						&vindex[3], &nindex[3]);
				}
			}
			else
			{
				// this file format not supported
				printf("Error : The model must consist of triangles or rectangles.[%s]", filename);
				return nullptr;
			}

			// Add vertex formats
			Vertex v[4];
			for (int i = 0; i < vcount; i++)
			{
				v[i].position = vertices[vindex[i] - 1];
				v[i].uv = uvs[uvindex[i] - 1];
				v[i].color = vec4(1,1,1,1);
				v[i].normal = (hasNormal) ? normals[nindex[i]-1] : vec3();
				//vf.normal = (hasNormal) ? normals[nindex[i]-1] : vec3();
			}

			// Add triangle indieces
			if(vcount == 3)
			{
				indices.push_back(offset);
				indices.push_back(offset + 1);
				indices.push_back(offset + 2);
				offset += 3;

				verts.push_back(v[0]);
				verts.push_back(v[1]);
				verts.push_back(v[2]);
			}
			else if(vcount == 4)
			{
				indices.push_back(offset);
				indices.push_back(offset + 1);
				indices.push_back(offset + 2);

				indices.push_back(offset);
				indices.push_back(offset + 2);
				indices.push_back(offset + 3);
				offset += 4;

				verts.push_back(v[0]);
				verts.push_back(v[1]);
				verts.push_back(v[2]);
				verts.push_back(v[3]);
			}
		}
	}

	MeshRef mesh = gpu_create_mesh(verts.data(), (int)verts.size(), indices.data(), (int)indices.size());
	res_register(filepath, mesh);

	// Cleanup
	fclose(fp);
	return mesh;
}

#define SMD_VERTEX_COLOR	1
#define SMD_VERTEX_UV2		2
#define SMD_VERTEX_SKINNED	4
static ModelRef _load_smd(const char *filename)
{
	ModelRef model = res_get_model(filename);
	if(model != nullptr)
		return model;

	// get file path
	char filepath[256] = {};
	strcpy(filepath, filename);
	const char *obj_name_point = strrchr(filename, '@');
	if(obj_name_point != nullptr)
	{
		filepath[obj_name_point - filepath] = '\0';
	}

	FILE *fp = fopen(filepath, "rb");
	if(fp == nullptr) return nullptr;

	char signature[4] = {};
	u8 version = 0;
	fread(signature, sizeof(char), 4, fp);
	fread(&version, sizeof(u8), 1, fp);

	u32 model_count = 0;
	fread(&model_count, sizeof(u32), 1, fp);
	fpos_t model_index_pos;
	fgetpos(fp, &model_index_pos);


	for(u32 i=0; i<model_count; i++)
	{
		ModelRef model = std::make_shared<Model>();

		// get model offset
		fseek(fp, (long)model_index_pos + (long)(i * sizeof(u32)), 0);
		u32 model_offset;
		fread(&model_offset, sizeof(u32), 1, fp);
		fseek(fp, model_offset, 0);

		u32 model_name_count;
		fread(&model_name_count, sizeof(u32), 1, fp);
		char name[256] = {};
		fread(&name, sizeof(char), model_name_count, fp);
		u32 vertex_count;
		fread(&vertex_count, sizeof(u32), 1, fp);
		u32 submesh_count;
		fread(&submesh_count, sizeof(u32), 1, fp);
		u8 flags;
		fread(&flags, sizeof(u8), 1, fp);

		// read vertex
		if(vertex_count > 0)
		{
			vec3 *position = new vec3[vertex_count];
			vec3 *normal = new vec3[vertex_count];
			vec2 *uv = new vec2[vertex_count];

			fread(position, sizeof(vec3), vertex_count, fp);
			fread(normal, sizeof(vec3), vertex_count, fp);
			fread(uv, sizeof(vec2), vertex_count, fp);

			Vertex *vertices = new Vertex[vertex_count];
			for(u32 vi=0; vi<vertex_count; vi++)
			{
				vertices[vi].position = position[vi];
				vertices[vi].normal = normal[vi];
				vertices[vi].uv = uv[vi];
				vertices[vi].color = vec4(1,1,1,1);
			}
			delete[] position;
			delete[] normal;
			delete[] uv;

			// read colors
			if(flags & SMD_VERTEX_COLOR)
			{
				vec4 *color = new vec4[vertex_count];
				fread(color, sizeof(vec4), vertex_count, fp);
				for(u32 vi=0; vi<vertex_count; vi++)
				{
					vertices[vi].color = color[vi];
				}
				delete[] color;
			}

			// read skinned vertex infomation
			if(flags & SMD_VERTEX_SKINNED)
			{
				u8 *bones = new u8[vertex_count * 4];
				float *weights = new float[vertex_count * 4];
				fread(bones, sizeof(u8), vertex_count*4, fp);
				fread(weights, sizeof(float), vertex_count*4, fp);
				for(u32 j=0; j<vertex_count; j++)
				{
					for(int k=0; k<4; k++)
					{
						vertices[j].bones[k] = bones[(j*4)+k];
						vertices[j].weights[k] = weights[(j*4)+k];
					}
					if(vertices[j].weights[0] <= EPSILON && vertices[j].weights[1] <= EPSILON && vertices[j].weights[2] <= EPSILON && vertices[j].weights[3] <= EPSILON)
					{
						vertices[j].weights[0] = 1.0f;
					}
				}
				delete[] bones;
				delete[] weights;
			}

			// indices
			u32 *index_counts = new u32[submesh_count];
			u16 **indices = new u16*[submesh_count];
			Vertex **verts = new Vertex*[submesh_count];
			for(u32 si=0; si<submesh_count; si++)
			{
				fread(&index_counts[si], sizeof(u32), 1, fp);
				indices[si] = new u16[index_counts[si]];
				verts[si] = new Vertex[index_counts[si]];
				u32 *u32_indices = new u32[index_counts[si]];
				fread(u32_indices, sizeof(u32), index_counts[si], fp);
				for(u32 n=0; n<index_counts[si]; n++)
				{
					indices[si][n] = n;
					verts[si][n] = vertices[u32_indices[n]];
				}
				delete [] u32_indices;
			}

		
			// create mesh
			model->mesh = gpu_create_mesh();
			for(u32 si=0; si<submesh_count; si++)
			{
				model->mesh->create(verts[si], index_counts[si], indices[si], index_counts[si], si);
			}


			// free data
			for(u32 si=0; si<submesh_count; si++)
			{
				delete [] verts[si];
				delete [] indices[si];
			}
			delete [] vertices;
			delete [] index_counts;
			delete [] verts;
			delete [] indices;
		}


		// Bones
		u8 bone_count;
		fread(&bone_count, sizeof(u8), 1, fp);
		for(int ii=0; ii<bone_count; ii++)
		{
			u32 name_count;
			fread(&name_count, sizeof(u32), 1, fp);
			char bone_name[256] = {};
			fread(bone_name, sizeof(char), name_count, fp);
			
			Bone bone = {};
			bone.parent = -1;
			bone.name = bone_name;
			mat4 matrix;
			fread(&matrix, sizeof(mat4), 1, fp);
			bone.inv_matrix = matrix.inversed();
			bone.matrix = matrix;
			u8 children_count;
			fread(&children_count, sizeof(u8), 1, fp);
			for(int iii=0; iii<children_count; iii++)
			{
				u8 idx=0;
				fread(&idx, sizeof(u8), 1, fp);
				bone.children.push_back(idx);
			}

			model->bones.push_back(bone);
		}
		// set bone parent
		for(int j=0; j<bone_count; j++)
		{
			for(int k=0; k<model->bones[j].children.size(); k++)
			{
				int child = model->bones[j].children[k];
				model->bones[child].parent = j;
			}
		}


		// animations
		int animation_count;
		fread(&animation_count, sizeof(int), 1, fp);
		for(int j=0; j<animation_count; j++)
		{
			int name_count;
			char animation_name[256] = {};
			fread(&name_count, sizeof(int), 1, fp);
			fread(animation_name, sizeof(char), name_count, fp);

			u32 frame_rate;
			fread(&frame_rate, sizeof(u32), 1, fp);

			u32 frame_count;
			fread(&frame_count, sizeof(u32), 1, fp);
			AnimationClipRef anim_clip = std::make_shared<AnimationClip>();
			anim_clip->frame_count = frame_count;
			anim_clip->frame_rate = frame_rate;
			for(int b=0; b<bone_count; b++)
			{
				AnimationKey *keys = new AnimationKey();
				keys->keys.resize(frame_count);
				fread(keys->keys.data(), sizeof(transform_t), frame_count, fp);
				anim_clip->animation_keys[model->bones[b].name] = std::unique_ptr<AnimationKey>(keys);
			}

			model->animator.animations.emplace(animation_name, anim_clip);
			if(j == 0)
			{
				model->animator.current_animation = anim_clip;
			}
		}


		// register resources
		char res_name[512] = {};
		sprintf(res_name, "%s@%s", filepath, name);
		res_register(res_name, model->mesh);
		res_register(res_name, model);

		// register the first model and mesh as the representative of the file
		if(i == 0)
		{
			res_register(filepath, model->mesh);
			res_register(filepath, model);
		}
	}

	fclose(fp);
	return res_get_model(filename);
}



static MeshRef _create_mesh_with_par_shapes(par_shapes_mesh *psm)
{
	par_shapes_compute_normals(psm);

	int vertex_count = psm->npoints;
	Vertex *vertices = new Vertex[vertex_count]{};
	for(int i=0; i<vertex_count; i++)
	{
		vertices[i].position = vec3(psm->points[(i*3)], psm->points[(i*3)+1], psm->points[(i*3)+2]);
		vertices[i].normal = vec3(psm->normals[(i*3)], psm->normals[(i*3)+1], psm->normals[(i*3)+2]);
		vertices[i].uv = vec2();
		vertices[i].color = vec4(1,1,1,1);
	}

	int index_count = psm->ntriangles * 3;
	u16 *indices = new u16[index_count];
	for(int i=0; i<index_count; i++)
	{
		indices[i] = psm->triangles[i];
	}


	MeshRef mesh = gpu_create_mesh(vertices, vertex_count, indices, index_count);
	delete[] vertices;
	delete[] indices;

	return mesh;
}

MeshRef mesh_create_plane(float width, float depth)
{
	par_shapes_mesh *plane = par_shapes_create_plane(1, 1);
	par_shapes_translate(plane, -0.5f, -0.5f, 0);
	float axis[] = {1, 0, 0};
	par_shapes_rotate(plane, -90.0f*DEG2RAD, axis);
	par_shapes_scale(plane, width, 1.0f, depth);
	MeshRef mesh = _create_mesh_with_par_shapes(plane);
	par_shapes_free_mesh(plane);
	return mesh;
}

MeshRef create_box_mesh(const vec3 &size)
{
	par_shapes_mesh *box = par_shapes_create_cube();
	par_shapes_translate(box, -0.5f, -0.5f, -0.5f); 
	par_shapes_scale(box, size.x, size.y, size.z);

	MeshRef mesh = _create_mesh_with_par_shapes(box);
	par_shapes_free_mesh(box);

	return mesh;
}

MeshRef create_sphere_mesh(float radius, int slice, int stack)
{
	par_shapes_mesh *sphere = par_shapes_create_parametric_sphere(slice, stack);
	par_shapes_scale(sphere, radius, radius, radius);

	MeshRef mesh = _create_mesh_with_par_shapes(sphere);
	par_shapes_free_mesh(sphere);

	return mesh;
}