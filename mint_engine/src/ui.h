#pragma once
#include <vector>
#include <string>
#include "mathf.h"
#include "font.h"

namespace UI
{

enum class WidgetConstraintType
{
	None,
	Pixel,
	PixelBack,
	Relative,
	RelativeBack,
	Aspect,
	// positioning
	CenteringPixel,
	CenteringPixelBack,
	CenteringRelative,
	CenteringRelativeBack,
};

enum class AlignmentType
{
	TopLeft,
	TopCenter,
	TopRight,
	MiddleLeft,
	MiddleCenter,
	MiddleRight,
	BottomLeft,
	BottomCenter,
	BottomRight,
};

struct WidgetConstraint
{
	WidgetConstraint() : type(WidgetConstraintType::None), value(0){}
	WidgetConstraint(WidgetConstraintType type, float value) : type(type), value(value){}
	WidgetConstraintType type;
	float value;
};

#define CONSTRAINT_PIXEL(v) UI::WidgetConstraint{UI::WidgetConstraintType::Pixel, v}
#define CONSTRAINT_PIXELBACK(v) UI::WidgetConstraint{UI::WidgetConstraintType::PixelBack, v}
#define CONSTRAINT_RELATIVE(v) UI::WidgetConstraint{UI::WidgetConstraintType::Relative, v}
#define CONSTRAINT_RELATIVEBACK(v) UI::WidgetConstraint{UI::WidgetConstraintType::RelativeBack, v}
#define CONSTRAINT_CENTER UI::WidgetConstraint{UI::WidgetConstraintType::CenteringRelative, 0.5f}
#define CONSTRAINT_CENTERINGRELATIVE(v) UI::WidgetConstraint{UI::WidgetConstraintType::CenteringRelative, v}
#define CONSTRAINT_ASPECT(v) UI::WidgetConstraint{UI::WidgetConstraintType::Aspect, v}

struct Widget;
typedef std::shared_ptr<Widget> WidgetRef;
struct Widget : std::enable_shared_from_this<Widget>
{
	Widget() :color(vec4(1,1,1,1)), parent(){}
	vec4 color;
	WidgetRef parent;
	std::vector<WidgetRef> children;
	std::string name;
	bool is_active = true;
	struct
	{
		WidgetConstraint x;
		WidgetConstraint y;
		WidgetConstraint w;
		WidgetConstraint h;
	} constraint;

	void free();
	virtual void draw();
	void set_parent(WidgetRef widget);
	void add_child(WidgetRef widget);
	rect_t get_rect();
};

struct Rect : Widget
{
	Rect(){}
	void draw();
};
typedef std::shared_ptr<Rect> RectRef;

struct Ring : Widget
{
	Ring(){}
	void draw();

	float start_angle=0;
	float angle = 360;
	float thickness = 0.2f;
};
typedef std::shared_ptr<Ring> RingRef;

struct Text : Widget
{
	Text() : font(nullptr), size(12), alignment(AlignmentType::TopLeft){}
	Text(const char *text) : font(nullptr), size(12), alignment(AlignmentType::TopLeft), text(text){}
	FontRef font;
	float size;
	AlignmentType alignment;
	std::string text;
	void draw();
};
typedef std::shared_ptr<Text> TextRef;

struct Image : Widget
{
	Image() : texture(nullptr){}
	Image(const char *filepath);
	Image(TextureRef texture) : texture(texture){}
	TextureRef texture;
	void draw();
};
typedef std::shared_ptr<Image> ImageRef;

void init();
void uninit();
void add(WidgetRef widget);	// add a widget to root
template <class T, class... Args>
std::shared_ptr<T> add(Args&&... args) {
	std::shared_ptr<T> p = std::make_shared<T>(args...);
	add(p);
	return  std::shared_ptr<T>(p);
}
template <class T>
std::shared_ptr<T> add() {
	std::shared_ptr<T> p = std::make_shared<T>();
	add(p);
	return  std::shared_ptr<T>(p);
}

void draw();

void set_default_font(FontRef font);

} // end namespace UI