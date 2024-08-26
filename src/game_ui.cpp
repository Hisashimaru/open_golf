#include "ui.h"
#include "hud.h"
#include "app.h"
#include "resource_manager.h"

static struct game_ui_ctx {
	float time = 0.0f;
	UI::WidgetRef text_cupin;
	UI::WidgetRef next_img;

	// fade
	UI::RectRef fade_panel;
	float fade_time_target;
	float fade_time;
	bool fade_in;

	struct{
		UI::WidgetRef panel;
		UI::WidgetRef panel_versus;

		UI::TextRef single_result_text;
		UI::TextRef shots;
		UI::TextRef time;
		UI::TextRef score;

		UI::TextRef multi_result_text;
		UI::TextRef player1_shots;
		UI::TextRef player1_time;
		UI::TextRef player1_score;
		UI::TextRef player2_shots;
		UI::TextRef player2_time;
		UI::TextRef player2_score;

		UI::ImageRef crown_image;
	} result;

	UI::RectRef thankyou_panel;
} ctx = {};


static void ui_result_init()
{
	// Singler result panel
	UI::WidgetRef panel = std::make_shared<UI::Widget>();
	panel->constraint = {
		CONSTRAINT_CENTER,
		CONSTRAINT_CENTER,
		CONSTRAINT_RELATIVE(1.0f),
		CONSTRAINT_RELATIVE(1.0f)};
	UI::add(panel);
	ctx.result.panel = panel;
	panel->is_active = false;

	// back ground
	UI::RectRef r = UI::add<UI::Rect>();
	panel->add_child(r);
	r->constraint = {
		CONSTRAINT_RELATIVE(0.0f),
		CONSTRAINT_RELATIVE(0.0f),
		CONSTRAINT_RELATIVE(1.0f),
		CONSTRAINT_RELATIVE(1.0f)
	};
	r->color = vec4(0,0,0,0.3f);

	UI::TextRef text = std::make_shared<UI::Text>("RESULT");
	text->size = 64;
	text->constraint = {
		CONSTRAINT_CENTER,
		CONSTRAINT_PIXEL(0),
		CONSTRAINT_RELATIVE(0.5f),
		CONSTRAINT_RELATIVE(0.1f)};
	panel->add_child(text);
	ctx.result.single_result_text = text;

	text = std::make_shared<UI::Text>("Shots: 999");
	text->size = 64;
	text->constraint = {
		CONSTRAINT_CENTERINGRELATIVE(0.5f),
		CONSTRAINT_RELATIVE(0.3f),
		CONSTRAINT_RELATIVE(0.15f),
		CONSTRAINT_PIXEL(64.0f)};
	panel->add_child(text);
	ctx.result.shots = text;

	text = std::make_shared<UI::Text>("Time: 9:99");
	text->size = 64;
	text->constraint = {
		CONSTRAINT_CENTERINGRELATIVE(0.52f),
		CONSTRAINT_RELATIVE(0.4f),
		CONSTRAINT_RELATIVE(0.15f),
		CONSTRAINT_PIXEL(64.0f)};
	panel->add_child(text);
	ctx.result.time = text;

	text = std::make_shared<UI::Text>("Score: 999");
	text->size = 64;
	text->constraint = {
		CONSTRAINT_CENTERINGRELATIVE(0.54f),
		CONSTRAINT_RELATIVE(0.5f),
		CONSTRAINT_RELATIVE(0.23f),
		CONSTRAINT_PIXEL(64.0f)};
	panel->add_child(text);
	ctx.result.score = text;



	// multi result panel
	panel = std::make_shared<UI::Widget>();
	panel->constraint = {
		CONSTRAINT_CENTER,
		CONSTRAINT_CENTER,
		CONSTRAINT_RELATIVE(1.0f),
		CONSTRAINT_RELATIVE(1.0f)};
	UI::add(panel);
	ctx.result.panel_versus = panel;
	panel->is_active = false;

	// back ground
	r = UI::add<UI::Rect>();
	panel->add_child(r);
	r->constraint = {
		CONSTRAINT_RELATIVE(0.0f),
		CONSTRAINT_RELATIVE(0.0f),
		CONSTRAINT_RELATIVE(1.0f),
		CONSTRAINT_RELATIVE(1.0f)
	};
	r->color = vec4(0,0,0,0.3f);

	text = std::make_shared<UI::Text>("RESULT");
	text->size = 64;
	text->constraint = {
		CONSTRAINT_CENTER,
		CONSTRAINT_PIXEL(0),
		CONSTRAINT_RELATIVE(0.5f),
		CONSTRAINT_RELATIVE(0.1f)};
	panel->add_child(text);
	ctx.result.multi_result_text = text;

	auto img = std::make_shared<UI::Image>("data/ui/crown.png");
	img->constraint = {
		CONSTRAINT_CENTERINGRELATIVE(0.25f),
		CONSTRAINT_RELATIVE(0.2f),
		CONSTRAINT_RELATIVE(0.08f),
		CONSTRAINT_ASPECT(1)};
	panel->add_child(img);
	ctx.result.crown_image = img;


	text = std::make_shared<UI::Text>("Player 1");
	text->size = 64;
	text->constraint = {
		CONSTRAINT_CENTERINGRELATIVE(0.25f),
		CONSTRAINT_RELATIVE(0.3f),
		CONSTRAINT_RELATIVE(0.15f),
		CONSTRAINT_PIXEL(64.0f)};
	panel->add_child(text);

	text = std::make_shared<UI::Text>("Shots: 999");
	text->size = 64;
	text->constraint = {
		CONSTRAINT_CENTERINGRELATIVE(0.26f),
		CONSTRAINT_RELATIVE(0.45f),
		CONSTRAINT_RELATIVE(0.15f),
		CONSTRAINT_PIXEL(64.0f)};
	panel->add_child(text);
	ctx.result.player1_shots = text;

	text = std::make_shared<UI::Text>("Time: 9:99");
	text->size = 64;
	text->constraint = {
		CONSTRAINT_CENTERINGRELATIVE(0.27f),
		CONSTRAINT_RELATIVE(0.55f),
		CONSTRAINT_RELATIVE(0.15f),
		CONSTRAINT_PIXEL(64.0f)};
	panel->add_child(text);
	ctx.result.player1_time = text;

	text = std::make_shared<UI::Text>("Score: 999");
	text->size = 64;
	text->constraint = {
		CONSTRAINT_CENTERINGRELATIVE(0.28f),
		CONSTRAINT_RELATIVE(0.65f),
		CONSTRAINT_RELATIVE(0.23f),
		CONSTRAINT_PIXEL(64.0f)};
	panel->add_child(text);
	ctx.result.player1_score = text;

	// Player 2
	text = std::make_shared<UI::Text>("Player 2");
	text->size = 64;
	text->constraint = {
		CONSTRAINT_CENTERINGRELATIVE(0.75f),
		CONSTRAINT_RELATIVE(0.3f),
		CONSTRAINT_RELATIVE(0.15f),
		CONSTRAINT_PIXEL(64.0f)};
	panel->add_child(text);

	text = std::make_shared<UI::Text>("Shots: 999");
	text->size = 64;
	text->constraint = {
		CONSTRAINT_CENTERINGRELATIVE(0.76f),
		CONSTRAINT_RELATIVE(0.45f),
		CONSTRAINT_RELATIVE(0.15f),
		CONSTRAINT_PIXEL(64.0f)};
	panel->add_child(text);
	ctx.result.player2_shots = text;

	text = std::make_shared<UI::Text>("Time: 9:99");
	text->size = 64;
	text->constraint = {
		CONSTRAINT_CENTERINGRELATIVE(0.77f),
		CONSTRAINT_RELATIVE(0.55f),
		CONSTRAINT_RELATIVE(0.15f),
		CONSTRAINT_PIXEL(64.0f)};
	panel->add_child(text);
	ctx.result.player2_time = text;

	text = std::make_shared<UI::Text>("Score: 999");
	text->size = 64;
	text->constraint = {
		CONSTRAINT_CENTERINGRELATIVE(0.78f),
		CONSTRAINT_RELATIVE(0.65f),
		CONSTRAINT_RELATIVE(0.23f),
		CONSTRAINT_PIXEL(64.0f)};
	panel->add_child(text);
	ctx.result.player2_score = text;


	// press key text
	UI::ImageRef next_img = std::make_shared<UI::Image>("data/ui/next.png");
	next_img->constraint = {
		CONSTRAINT_CENTER,
		CONSTRAINT_RELATIVE(0.8f),
		CONSTRAINT_RELATIVE((720.0f/1280)*0.7f),
		CONSTRAINT_RELATIVE((120.0f/720)*0.7f),
	};
	UI::add(next_img);
	ctx.next_img = next_img;
	ctx.next_img->is_active = false;



	// Thank you for playing panel
	ctx.thankyou_panel = UI::add<UI::Rect>();
	ctx.thankyou_panel->color = vec4(0,0,0,1);
	ctx.thankyou_panel->constraint = {
		CONSTRAINT_RELATIVE(0),
		CONSTRAINT_RELATIVE(0),
		CONSTRAINT_RELATIVE(1),
		CONSTRAINT_RELATIVE(1)
	};
	ctx.thankyou_panel->is_active = false;

	text = UI::add<UI::Text>("Thank You");
	ctx.thankyou_panel->add_child(text);
	text->constraint = {
		CONSTRAINT_CENTER,
		CONSTRAINT_RELATIVE(0.35f),
		CONSTRAINT_RELATIVE(0.3f),
		CONSTRAINT_PIXEL(64.0f)
	};

	text = UI::add<UI::Text>("For Playing!");
	ctx.thankyou_panel->add_child(text);
	text->constraint = {
		CONSTRAINT_CENTER,
		CONSTRAINT_RELATIVE(0.5f),
		CONSTRAINT_RELATIVE(0.35f),
		CONSTRAINT_PIXEL(64.0f)
	};
}


void ui_init()
{
	if(global.game_mode != GameMode::Edit)
	{
		UI::TextRef text = std::make_shared<UI::Text>("CUP IN!");
		UI::add(text);
		text->size = 64;
		ctx.text_cupin = text;
		ctx.text_cupin->is_active = false;

		hud_init();
		ui_result_init();

		FontRef font = load_font("data/unitblock.ttf");
		UI::set_default_font(font);
	}

	// screen fading panel
	ctx.fade_panel = std::make_shared<UI::Rect>();
	ctx.fade_panel->constraint = {
		CONSTRAINT_CENTER,
		CONSTRAINT_CENTER,
		CONSTRAINT_RELATIVE(1.0f),
		CONSTRAINT_RELATIVE(1.0f)};
	ctx.fade_panel->color = vec4(0,0,0,1);
	UI::add(ctx.fade_panel);
}

void ui_uninit()
{
	hud_uninit();
	UI::uninit();
	ctx = {};
}

void ui_update()
{
	// fading panel
	float t = 1.0f;
	if(ctx.fade_time > 0)
	{
		t = clamp01((ctx.fade_time_target - time_now()) / ctx.fade_time);
	}
	if(!ctx.fade_in)
	{
		t = 1.0f - t;
	}
	ctx.fade_panel->color.a = t;
}

void ui_fade_out(float time)
{
	ctx.fade_in = false;
	ctx.fade_time = time;
	ctx.fade_time_target = time_now() + time;
}

void ui_fade_in(float time)
{
	ctx.fade_in = true;
	ctx.fade_time = time;
	ctx.fade_time_target = time_now() + time;
}

void ui_reset()
{
	ctx.text_cupin->is_active = false;
	ctx.result.panel->is_active = false;
	ctx.result.panel_versus->is_active = false;
	ctx.next_img->is_active = false;
}

void ui_show_cupin()
{
	ctx.text_cupin->is_active = true;
}

void ui_show_result()
{
	ctx.next_img->is_active = false;
	if(global.game_mode == GameMode::Single || global.game_mode == GameMode::Coop)
	{
		ctx.result.panel->is_active = true;
		ctx.result.single_result_text->text = "ROUND RESULTS";
		ctx.result.shots->text = "Shots: " + std::to_string(global.result.p1.shots);
		int m = (int)floorf(global.result.p1.time/60.0f);
		int s = (int)floorf(fmodf(global.result.p1.time, 60.0f));
		ctx.result.time->text = "Time: " + std::to_string(m) + ":" + std::to_string(s);
		ctx.result.score->text = "Score: " + std::to_string(global.result.p1.score);
	}
	else
	{
		ctx.result.panel_versus->is_active = true;
		ctx.result.multi_result_text->text = "ROUND RESULTS";
		ctx.result.player1_shots->text = "Shots: " + std::to_string(global.result.p1.shots);
		int m = (int)floorf(global.result.p1.time/60.0f);
		int s = (int)floorf(fmodf(global.result.p1.time, 60.0f));
		ctx.result.player1_time->text = "Time: " + std::to_string(m) + ":" + std::to_string(s);
		ctx.result.player1_score->text = "Score: " + std::to_string(global.result.p1.score);

		ctx.result.player2_shots->text = "Shots: " + std::to_string(global.result.p2.shots);
		m = (int)floorf(global.result.p2.time/60.0f);
		s = (int)floorf(fmodf(global.result.p2.time, 60.0f));
		ctx.result.player2_time->text = "Time: " + std::to_string(m) + ":" + std::to_string(s);
		ctx.result.player2_score->text = "Score: " + std::to_string(global.result.p2.score);

		ctx.result.crown_image->is_active = false;
	}
}

void ui_show_final_result()
{
	ctx.next_img->is_active = false;
	if(global.game_mode == GameMode::Single || global.game_mode == GameMode::Coop)
	{
		ctx.result.panel->is_active = true;
		ctx.result.single_result_text->text = "FINAL RESULT";
		ctx.result.shots->text = "Shots: " + std::to_string(global.result.p1.total_shots);
		int m = (int)floorf(global.result.p1.total_time/60.0f);
		int s = (int)floorf(fmodf(global.result.p1.total_time, 60.0f));
		ctx.result.time->text = "Time: " + std::to_string(m) + ":" + std::to_string(s);
		ctx.result.score->text = "Score: " + std::to_string(global.result.p1.total_score);
	}
	else
	{
		ctx.result.panel_versus->is_active = true;
		ctx.result.multi_result_text->text = "FINAL RESULT";
		ctx.result.player1_shots->text = "Shots: " + std::to_string(global.result.p1.total_shots);
		int m = (int)floorf(global.result.p1.total_time/60.0f);
		int s = (int)floorf(fmodf(global.result.p1.total_time, 60.0f));
		ctx.result.player1_time->text = "Time: " + std::to_string(m) + ":" + std::to_string(s);
		ctx.result.player1_score->text = "Score: " + std::to_string(global.result.p1.total_score);

		ctx.result.player2_shots->text = "Shots: " + std::to_string(global.result.p2.total_shots);
		m = (int)floorf(global.result.p2.total_time/60.0f);
		s = (int)floorf(fmodf(global.result.p2.total_time, 60.0f));
		ctx.result.player2_time->text = "Time: " + std::to_string(m) + ":" + std::to_string(s);
		ctx.result.player2_score->text = "Score: " + std::to_string(global.result.p2.total_score);

		ctx.result.crown_image->is_active = true;
		if(global.result.p1.total_score > global.result.p2.total_score)
		{
			ctx.result.crown_image->constraint.x = CONSTRAINT_CENTERINGRELATIVE(0.25f);
		}
		else if(global.result.p1.total_score < global.result.p2.total_score)
		{
			ctx.result.crown_image->constraint.x = CONSTRAINT_CENTERINGRELATIVE(0.75f);
		}
		else
		{
			ctx.result.crown_image->is_active = false;
		}
	}
}

void ui_show_thanks()
{
	ctx.thankyou_panel->is_active = true;
}

void ui_show_next_text()
{
	ctx.next_img->is_active = true;
}