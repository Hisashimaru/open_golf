#pragma once

#include <memory>
#include <unordered_map>
#include "gpu.h"
#include "external/stb_truetype.h"
typedef unsigned char u8;

struct glyph_t
{
	int x, y;
	int w, h;
	float advance; 
	float bearing_x, bearing_y;
};

struct Font
{
public:
	Font(){}
	Font(const char *filename);
	Font(const u8 *data, size_t size);
	Font(const u8 *data, int width, int height);
	~Font();

	bool load(const char *filename);
	bool load(const char *filename, float font_size);	// load tff data
	bool load(const u8 *data, size_t size);				// load data as image file data
	bool load(const u8 *data, int width, int height);	// load data as RGBA data
	void free();
	void update_texture();
	float get_size(){return size;}
	vec2 calc_text_size(float font_size, const char *str, ...);

	float get_ascent(){return ascent;}
	float get_descent(){return descent;}
	float get_line_space(){return line_space;}

	// Glyph
	void load_glyph(int codepoint);
	void load_glyphs(int *codepoint, int count);
	glyph_t get_glyph(int codepoint);
	int get_glyph_count(){return (int)glyph_table.size();}

	TextureRef texture;
protected:

	u8 *image_data = nullptr;
	int packer_x_offset = 0;
	int packer_y_offset = 0;
	int pack_margin = 4;
	int pack_heighest = 0;

	float size = 0;
	float ascent = 0;
	float descent = 0;
	float line_space = 0;
	u8 *font_data = nullptr;
	stbtt_fontinfo stb_font = {};
	bool need_update_texture = false;
	std::unordered_map<int, glyph_t> glyph_table;
};
typedef std::shared_ptr<Font> FontRef;