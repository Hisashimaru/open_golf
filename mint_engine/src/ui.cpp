#include "ui.h"
#include "app.h"
#include "renderer.h"
#include "resource_manager.h"

namespace UI
{

static struct ctx_ui
{
	WidgetRef root;
	FontRef default_font;
} ctx;


// ============================================================================
// Widget class
// ============================================================================
void Widget::set_parent(WidgetRef widget)
{
	if(widget.get() == this || parent.get() == this) return;
	if(parent)
	{
		auto it = std::find(parent->children.begin(), parent->children.end(), shared_from_this());
		if(it != parent->children.end())
		{
			WidgetRef tmp =	parent->children.back();
			//(*it) = tmp;
			std::iter_swap(it, parent->children.end() -1);
			parent->children.pop_back();
		}
	}

	// new parent
	parent = widget;
	if(parent)
	{
		parent->children.push_back(shared_from_this());
	}
	else
	{
		// free children
		while(children.size() > 0)
		{
			WidgetRef child = children[0];
			child->set_parent(nullptr);
		}
	}
}

void Widget::add_child(WidgetRef widget)
{
	if(widget == nullptr || widget.get() == this || widget->parent.get() == this) return;
	if(widget->parent != nullptr)
	{
		auto it = std::find(widget->parent->children.begin(), widget->parent->children.end(), widget);
		if(it != widget->parent->children.end())
		{
			WidgetRef tmp =	widget->parent->children.back();
			(*it) = tmp;
			widget->parent->children.pop_back();
		}
	}

	widget->parent = shared_from_this();
	children.push_back(widget);
}

rect_t Widget::get_rect()
{
	rect_t prect;

	if(parent)
	{
		prect = parent->get_rect();
	}
	else
	{
		// base rect
		vec2 size = app_get_size();
		prect.x = 0.0f;
		prect.y = 0.0f;
		prect.w = size.x;
		prect.h = size.y;
	}

	float prect_left = prect.x;
	float prect_right = prect.x + prect.w;
	float prect_top = prect.y;
	float prect_bottom = prect.y + prect.h;
	float prect_center_x = prect.x + (prect.w * 0.5f);
	float prect_center_y = prect.y + (prect.h * 0.5f);

	rect_t rect;
	// width
	switch(constraint.w.type)
	{
	case WidgetConstraintType::Pixel:
		rect.w = constraint.w.value;
		break;
	case WidgetConstraintType::Relative:
		rect.w = prect.w * constraint.w.value;
		break;
	default:
		rect.w = prect.w;
		break;
	}

	// height
	switch(constraint.h.type)
	{
	case WidgetConstraintType::Pixel:
		rect.h = constraint.h.value;
		break;
	case WidgetConstraintType::Relative:
		rect.h = prect.h * constraint.h.value;
		break;
	case WidgetConstraintType::Aspect:
		rect.h = rect.w * constraint.h.value;
		break;
	default:
		rect.h = prect.h;
		break;
	}


	// late evaluation aspect width
	if(constraint.w.type == WidgetConstraintType::Aspect)
	{
		rect.w = rect.h * constraint.w.value;
	}

	// position x
	switch(constraint.x.type)
	{
	case WidgetConstraintType::Pixel:
		rect.x = prect_left + constraint.x.value;
		break;
	case WidgetConstraintType::PixelBack:
		rect.x = prect_right - rect.w - constraint.x.value;
		break;
	case WidgetConstraintType::Relative:
		rect.x = prect_left + prect.w * constraint.x.value;
		break;
	case WidgetConstraintType::RelativeBack:
		rect.x = prect_right - rect.w - (prect.w * constraint.x.value);
		break;
	case WidgetConstraintType::CenteringPixel:
		rect.x = prect_left + constraint.x.value - rect.w;
		break;
	case WidgetConstraintType::CenteringPixelBack:
		rect.x = prect_right - (rect.w * 0.5f) - constraint.x.value;
		break;
	case WidgetConstraintType::CenteringRelative:
		rect.x = prect_left + prect.w * constraint.x.value - (rect.w * 0.5f);
		break;
	case WidgetConstraintType::CenteringRelativeBack:
		rect.x = prect_right - (rect.w * 0.5f) - (prect.w * constraint.x.value);
		break;
	default:
		rect.x = prect.x;
		break;
	}

	// position y
	switch(constraint.y.type)
	{
	case WidgetConstraintType::Pixel:
		rect.y = prect_top + constraint.y.value;
		break;
	case WidgetConstraintType::PixelBack:
		rect.y = prect_bottom - rect.h - constraint.y.value;
		break;
	case WidgetConstraintType::Relative:
		rect.y = prect_top + (prect.h * constraint.y.value);
		break;
	case WidgetConstraintType::RelativeBack:
		rect.y = prect_bottom  - rect.h - (prect.h * constraint.y.value);
		break;
	case WidgetConstraintType::CenteringPixel:
		rect.y = prect_top + constraint.y.value - (rect.h * 0.5f);
		break;
	case WidgetConstraintType::CenteringPixelBack:
		rect.y = prect_bottom - (rect.h * 0.5f) - constraint.y.value;
		break;
	case WidgetConstraintType::CenteringRelative:
		rect.y = prect_top + (prect.h * constraint.y.value) - (rect.h * 0.5f);
		break;
	case WidgetConstraintType::CenteringRelativeBack:
		rect.y = prect_bottom  - (rect.h*0.5f) - (prect.h * constraint.y.value);
		break;
	default:
		rect.y = prect.y;
		break;
	}

	return rect;
}

void Widget::free()
{
	set_parent(nullptr);
}

void Widget::draw()
{
}


void Rect::draw()
{
	rect_t rect = get_rect();
	draw_rect(rect, color);
}

void Ring::draw()
{
	rect_t rect = get_rect();
	vec2 center = vec2(rect.w, rect.h) * 0.5f;
	float radius = fmaxf(rect.w, rect.h);
	draw_ring(vec2(rect.x, rect.y) + center, start_angle, angle, radius * (1.0f-thickness), radius, color);
}

void Text::draw()
{
	rect_t rect = get_rect();

	FontRef f = font ? font : ctx.default_font;
	vec2 ts = f->calc_text_size(size, text.c_str());
	float font_scale = (ts.x == 0) ? 1.0f : (rect.w/ts.x);

	renderer_set_font(f);
	draw_text(vec2(rect.x, rect.y), size * font_scale, text.c_str());
	renderer_set_font(nullptr);
}

Image::Image(const char *filepath)
{
	texture = load_texture(filepath);
}

void Image::draw()
{
	rect_t rect = get_rect();

	if(texture)
	{
		draw_texture(texture, rect_t{0,0,(float)texture->get_width(), (float)texture->get_height()}, rect);
	}
	else
	{
		draw_rect(rect, color);
	}
}







void init()
{
	ctx.root = std::make_shared<Widget>();
	ctx.root->constraint.x = WidgetConstraint{WidgetConstraintType::Relative, 0};
	ctx.root->constraint.y = WidgetConstraint{WidgetConstraintType::Relative, 0};
	ctx.root->constraint.w = WidgetConstraint{WidgetConstraintType::Relative, 1};
	ctx.root->constraint.h = WidgetConstraint{WidgetConstraintType::Relative, 1};
	ctx.root->color.a = 0.0f;
}

void uninit()
{
	if(ctx.root)
		ctx.root->free();
	ctx.root = nullptr;
	ctx.default_font = nullptr;
}

void add(WidgetRef widget)
{
	if(ctx.root == nullptr)
		init();

	ctx.root->add_child(widget);
}

static void _draw_widget(WidgetRef widget)
{
	if(!widget->is_active) return;
	widget->draw();
	for(int i=0; i<widget->children.size(); i++)
	{
		_draw_widget(widget->children[i]);
	}
}

void draw()
{
	_draw_widget(ctx.root);
}

void set_default_font(FontRef font)
{
	ctx.default_font = font;
}


}