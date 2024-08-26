#include "app.h"
#include "gpu.h"
#include "resource_manager.h"
#include "material.h"
#include "renderer.h"
#include "material.h"
#include "collision.h"
#include "particle.h"
#include "sound.h"
#include "ui.h"


struct engine_ctx
{
	void(*draw_cb)();
}ctx = {};
static void _draw();

void init_engine()
{
	app_create(1280, 720, "");
	gpu_init(nullptr);
	res_init("data/res.txt");
	material_init_builtins();
	renderer_init();
	collision_init();
	sound_init();
	particle_init();
	UI::init();
}

void uninit_engine()
{
	UI::uninit();
	res_uninit();
	material_uninit();
	particle_uninit();
	sound_uninit();
	collision_uninit();
	gpu_uninit();
}

void start_engine(void(*update_cb)(), void(*draw_cb)())
{
	ctx.draw_cb = draw_cb;
	while(app_process())
	{
		sound_update();
		update_cb();
		particle_update();

		// draw
		renderer_draw(_draw);
		UI::draw();

		app_swap_buffer();
	}
}

static void _draw()
{
	if(ctx.draw_cb) ctx.draw_cb();
	particle_draw();
}