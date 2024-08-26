#include "gpu.h"
#include "model.h"
#include "material.h"
#include "renderer.h"
#include "foliage_system.h"
#include "collision.h"
#include "map.h"


static struct map_ctx
{
	ModelRef map_model;
	ColliderRef collider;
	ModelRef sea_model;
	ColliderRef sea_collider;
	ModelRef skybox_model;
	ModelRef hole_model;
	ModelRef hole_mask_model;
	struct
	{
		vec3 pos;
	} hole;
	MapData data;
} ctx = {};

void map_init()
{
	memset(&ctx, 0, sizeof(map_ctx));
	foliage_init();

	// sea model
	MaterialRef sea_mat = create_material("unlit");
	sea_mat->color = vec4(0.3f, 0.3f, 0.6f, 1.0f);
	ctx.sea_model = create_model(mesh_create_plane(500.0f , 500.0f), sea_mat);

	if(ctx.sea_collider == nullptr)
	{
		ctx.sea_collider = create_mesh_collider(ctx.sea_model->mesh);
		//col->set_position(vec3(0,0,0));
	}

	// golf cup model
	ctx.hole_model = load_model("golf_cup");
	ctx.hole_mask_model = load_model("golf_cup_mask");

	ctx.skybox_model = load_model("skybox");
}

void map_uninit()
{
	foliage_uninit();
	free_collider(ctx.collider);
	ctx = {};
}

void map_load(const char *filename)
{
	//ctx.mesh = mesh_create_plane(50.0f , 50.0f);
	map_init();


	if(filename)
	{
		FILE *fp = fopen(filename, "rb");
		if(!fp) return;

		strcpy(ctx.data.map_name, filename);

		// magic number
		char magic[4] = {};
		fread(&magic, 3, 1, fp);
		if(strcmp(magic, "MAP") != 0) {
			return;	// the file is not map file
		}

		// read model file name
		int model_name_count = 0;
		fread(&model_name_count, sizeof(int), 1, fp);
		fread(&ctx.data.model_name, sizeof(char), model_name_count, fp);

		// load map model
		ctx.map_model= load_model(ctx.data.model_name);
		MaterialRef mat = create_material("unlit");
		mat->color= vec4(0.4f, 0.7f, 0.4f, 1);
		ctx.map_model->materials.push_back(mat);
		ctx.collider = create_mesh_collider(ctx.map_model->mesh);

		// load hole
		fread(&ctx.hole, sizeof(ctx.hole), 1, fp);

		// load teeing area
		fread(&ctx.data.teeing_area, sizeof(ctx.data.teeing_area), 1, fp);

		// load foliage
		foliage_load(fp);
		fclose(fp);
	}
	else
	{
		ctx.map_model= load_model("data/models/island.obj");
		MaterialRef mat = create_material("unlit");
		mat->color = vec4(0.4f, 0.7f, 0.4f, 1);
		ctx.map_model->materials.push_back(mat);
		ctx.collider = create_mesh_collider(ctx.map_model->mesh);

		// foalige data
		foliage_init();
		const int MAX_GRASS = 3000;
		bounds_t bounds = ctx.map_model->mesh->get_bounds();
		vec3 min = bounds.get_min();
		vec3 max = bounds.get_max();
		for(int i=0; i<MAX_GRASS; i++)
		{
			vec3 pos = vec3(rand_range(min.x,max.x), 0, rand_range(min.z,max.z));
			RayHit hitinfo;
			if(raycast(ray_t(pos, vec3(0,-1,0)), &hitinfo))
			{
				foliage_add(FoliageType::grass1, pos);
			}
			else
			{
				// retry
				i--;
			}
		}
	}
}

void map_save(const char *filename)
{
	if(!filename) return;
	FILE *fp;
	fp = fopen(filename, "wb");
	if(!fp) return;

	// magic number
	fwrite("MAP", 3, 1, fp);

	// map model
	int model_name_count = (int)strlen(ctx.data.model_name);
	fwrite(&model_name_count, sizeof(int), 1, fp);
	fwrite(ctx.data.model_name, sizeof(char), model_name_count, fp);

	// hole
	fwrite(&ctx.hole, sizeof(ctx.hole), 1, fp);

	// teeing area
	fwrite(&ctx.data.teeing_area, sizeof(ctx.data.teeing_area), 1, fp);

	// foliage
	foliage_save(fp);
	fclose(fp);
}

void map_set_model(ModelRef model)
{
	if(!model) return;
	free_collider(ctx.collider);

	ctx.map_model = model;
	ctx.collider = create_mesh_collider(ctx.map_model->mesh);
}

void map_draw()
{
	CameraRef cam = renderer_get_current_camera();
	draw_model(ctx.skybox_model, mat4::translate(cam->position));
	draw_model(ctx.hole_mask_model, mat4::translate(ctx.hole.pos));
	draw_model(ctx.hole_model, mat4::translate(ctx.hole.pos));
	draw_model(ctx.map_model, mat4::identity());
	draw_model(ctx.sea_model, mat4::translate(vec3(0,0,0)));
	foliage_draw();
}

void map_load_model(const char *filename)
{
	ModelRef model = load_model(filename);
	if(!model) return;

	// create model material
	if(model->materials.size() == 0)
	{
		MaterialRef mat = create_material("unlit");
		mat->color = vec4(0.4f, 0.7f, 0.4f, 1);
		model->materials.push_back(mat);
	}

	// unload model
	free_collider(ctx.collider);

	ctx.map_model = model;
	ctx.collider = create_mesh_collider(ctx.map_model->mesh);
	strcpy(ctx.data.model_name, filename);
}

void map_set_hole(const vec3 &pos)
{
	ctx.hole.pos = pos;
}

void map_set_teeing_area(const vec3 &pos, float width, float angle)
{
	ctx.data.teeing_area.position = pos;
	ctx.data.teeing_area.width = width;
	ctx.data.teeing_area.angle = angle;
}

vec3 map_get_hole_position()
{
	return ctx.hole.pos;
}

const MapData* map_get_data()
{
	return &ctx.data;
}