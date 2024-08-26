#include "app.h"
#include "gpu.h"
#include "renderer.h"
#include "ui.h"
#include "hud.h"
#include "common.h"
#include "resource_manager.h"

struct PlayerHUD
{
	UI::WidgetRef rect;
	UI::ImageRef club_image;
	UI::ImageRef crosshair;
	UI::RingRef hold_ring;
};

static struct hud_ctx
{
	TextureRef crosshair;
	PlayerHUD player[4];
} ctx = {};

static void create_player_hud(int player_index)
{
	if(player_index < 0 || player_index >= 4) return;

	// club image
	ctx.player[player_index].club_image = std::make_shared<UI::Image>();
	ctx.player[player_index].club_image->constraint = {CONSTRAINT_RELATIVEBACK(0.04f), CONSTRAINT_RELATIVEBACK(0.04f),
														CONSTRAINT_PIXEL(128), CONSTRAINT_PIXEL(128)};
	ctx.player[player_index].rect->add_child(ctx.player[player_index].club_image);

	// crosshair
	ctx.player[player_index].crosshair = std::make_shared<UI::Image>();
	ctx.player[player_index].crosshair->constraint = {CONSTRAINT_CENTER, CONSTRAINT_CENTER,
														CONSTRAINT_PIXEL(32), CONSTRAINT_PIXEL(32)};
	ctx.player[player_index].crosshair->texture = load_texture("data/ui/crosshair.png");
	ctx.player[player_index].rect->add_child(ctx.player[player_index].crosshair);

	ctx.player[player_index].hold_ring = std::make_shared<UI::Ring>();
	ctx.player[player_index].hold_ring->constraint = {CONSTRAINT_CENTER, CONSTRAINT_CENTER,
														CONSTRAINT_PIXEL(20), CONSTRAINT_PIXEL(20)};
	ctx.player[player_index].hold_ring->thickness;
	ctx.player[player_index].rect->add_child(ctx.player[player_index].hold_ring);
}

void hud_init()
{
	ctx.crosshair = gpu_load_texture("data/ui/crosshair.png");

	ctx.player[0].rect = std::make_shared<UI::Widget>();
	UI::add(ctx.player[0].rect);

	if(global.game_mode == GameMode::Versus || global.game_mode == GameMode::Coop)
	{
		// Half HUD
		// player 1
		ctx.player[0].rect->constraint = {CONSTRAINT_RELATIVE(0),
											CONSTRAINT_RELATIVE(0),
											CONSTRAINT_RELATIVE(0.5f),
											CONSTRAINT_RELATIVE(1.0f)};

		// player 2
		ctx.player[1].rect = std::make_shared<UI::Widget>();
		UI::add(ctx.player[1].rect);
		ctx.player[1].rect->constraint = {CONSTRAINT_RELATIVE(0.5f),
											CONSTRAINT_RELATIVE(0),
											CONSTRAINT_RELATIVE(0.5f),
											CONSTRAINT_RELATIVE(1.0f)};
	}
	else
	{
		// Full HUD
		ctx.player[0].rect->constraint = {CONSTRAINT_RELATIVE(0),
										CONSTRAINT_RELATIVE(0),
										CONSTRAINT_RELATIVE(1.0f),
										CONSTRAINT_RELATIVE(1.0f)};
	}

	int players = global.splitscreen ? 2 : 1;
	for(int i=0; i<players; i++)
	{
		create_player_hud(i);
	};
}

void hud_uninit()
{
	ctx = {};
}

void hud_draw()
{
	vec2 vs = app_get_size();
	vec2 center = vs * 0.5f;
	draw_texture(ctx.crosshair, rect_t{0,0,32,32}, rect_t{center.x-16.0f, center.y-16.0f, 32.0f, 32.0f}, vec4(1,1,1,0.7f));
}


void hud_change_club(ClubType type, int player_index)
{
	if(player_index < 0 || player_index >= 4) return;
	ctx.player[player_index].club_image->texture = golf_clubs[(int)type].image;
}

void hud_hold_action(float value, int player_index)
{
	if(player_index < 0 || player_index >= 4) return;
	value = clamp01(value);
	ctx.player[player_index].hold_ring->angle = 360.0f * value;
	ctx.player[player_index].hold_ring->color.a = value;
}