#include <vector>
#include "material.h"
#include "renderer.h"
#include "resource_manager.h"
#include "builtin_shaders.h"

Material::~Material()
{
	texture = nullptr;

	for(int i=0; i<9; i++)
	{
		shaders[i] = nullptr;
	}
}

void Material::apply(ShaderRef shader)
{
	if(shader == nullptr) return;
	shader->set_value("_model", renderer_get_model_matrix());
	shader->set_value("_view", renderer_get_view_matrix());
	shader->set_value("_projection", renderer_get_projection_matrix());
	shader->set_value("_color", color);
	gpu_bind_texture(0, texture);
	shader->use();
}

static struct material_ctx
{
	std::vector<MaterialRef> materials;
} ctx = {};

void material_init_builtins()
{
	// unlit
	MaterialRef mat = std::make_shared<Material>();
	mat->shaders[STATIC_OPAQUE] = gpu_create_shader(unlit_vs_src, unlit_fs_src);
	mat->shaders[STATIC_CUTOUT] = gpu_create_shader(unlit_vs_src, unlit_cutout_fs_src);
	mat->shaders[STATIC_TRANSPARENT] = mat->shaders[STATIC_OPAQUE];
	mat->shaders[SKINNED_OPAQUE] = gpu_create_shader(unlit_vs_skin_src, unlit_fs_src);
	mat->shaders[SKINNED_CUTOUT] = mat->shaders[STATIC_OPAQUE];
	mat->shaders[SKINNED_TRANSPARENT] = mat->shaders[SKINNED_OPAQUE];
	mat->shaders[INSTANCING_OPAQUE] = gpu_create_shader(unlit_vs_inst_src, unlit_fs_src);
	mat->shaders[INSTANCING_CUTOUT] = gpu_create_shader(unlit_vs_inst_src, unlit_cutout_fs_src);
	mat->shaders[INSTANCING_TRANSPARENT] = mat->shaders[INSTANCING_OPAQUE];
	mat->texture = load_texture("_white");
	res_register("unlit", mat);

	// unlit billboard
	mat = clone_material(mat);
	mat->shaders[INSTANCING_OPAQUE] = gpu_create_shader(unlit_billboard_vs_inst_src, unlit_fs_src);
	mat->shaders[INSTANCING_CUTOUT] = gpu_create_shader(unlit_billboard_vs_inst_src, unlit_cutout_fs_src);
	mat->shaders[INSTANCING_TRANSPARENT] = mat->shaders[INSTANCING_OPAQUE];
	res_register("unlit_billboard", mat);
}

void material_uninit()
{
	ctx.materials.clear();
}

MaterialRef clone_material(MaterialRef src)
{
	if(src == nullptr) return nullptr;

	MaterialRef mat = std::make_shared<Material>();
	*mat.get() = *src.get();
	ctx.materials.push_back(mat);
	return mat;
}

MaterialRef create_material(const char *type_name)
{
	MaterialRef mat = load_material(type_name);
	if(mat != nullptr)
	{
		mat = clone_material(mat);
	}
	return mat;
}