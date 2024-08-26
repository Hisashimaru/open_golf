#pragma once
#include <memory>
#include "mathf.h"
#include "gpu.h"

enum MaterialShaderType
{
	STATIC_OPAQUE,
	STATIC_CUTOUT,
	STATIC_TRANSPARENT,
	SKINNED_OPAQUE,
	SKINNED_CUTOUT,
	SKINNED_TRANSPARENT,
	INSTANCING_OPAQUE,
	INSTANCING_CUTOUT,
	INSTANCING_TRANSPARENT
};

class Material
{
public:
	Material(): render_mode(OPAQUE), color(vec4(1,1,1,1)), texture(nullptr), shaders(), queue(0){};
	~Material();

	virtual void apply(ShaderRef shader);

	enum RenderMode
	{
		OPAQUE,
		CUTOUT,
		TRANSPARENT,
		DEPTHE_MASK,	// write only depth buffer
	};

	RenderMode render_mode;
	vec4 color;
	TextureRef texture;
	ShaderRef shaders[9]; // satic(opaque, cutout, transparent), skinned(...), instancing(...)
	int queue;
	bool zwrite;
};

typedef std::shared_ptr<Material> MaterialRef;


void material_init_builtins();
void material_uninit();
MaterialRef create_material(const char *type_name);
MaterialRef clone_material(MaterialRef src);