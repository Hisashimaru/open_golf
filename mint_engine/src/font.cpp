#include <stdarg.h>
#include <string>
#include "font.h"
#include "resource_manager.h"
#include "external/stb_image.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "external/stb_truetype.h"

#define IMAGE_SIZE 1024
#define PIXEL_DATA_STRIDE 4 // RGBA(4byte)

Font::Font(const char *filename) : texture(nullptr)
{
	load(filename);
}

Font::Font(const u8 *data, size_t size) : texture(nullptr)
{
	load(data, size);
}

Font::Font(const u8 *data, int width, int height) : texture(nullptr)
{
	load(data, width, height);
}

Font::~Font()
{
	free();
}


#define CHECK_COLOR(c) (c.r == 0xff && c.g == 0x00 && c.b == 0xff && c.a == 0xff)
typedef struct Color32{u8 r, g, b, a;} Color32;
bool Font::load(const char *filename)
{
	const char *ext = strrchr(filename, '.');
	if(ext != nullptr)
	{
		if(strcmp(ext, ".png") == 0)
		{
			free();
			int width, height, bpp;
			stbi_set_flip_vertically_on_load(0);
			u8 *data = (u8*)stbi_load(filename, &width, &height, &bpp, 4);
			if(data == nullptr)
				return false;

			load(data, width, height);
			stbi_image_free((void*)data);
			return true;
		}
		else if(strcmp(ext, ".ttf") == 0)
		{
			load(filename, 128);
			return true;
		}
	}
	return false;
}

bool Font::load(const u8 *data, size_t size)
{
	free();
	int width, height, bpp;
	stbi_set_flip_vertically_on_load(0);
	u8 *rgba_data = (u8*)stbi_load_from_memory(data, (int)size, &width, &height, &bpp, 4);
	if(rgba_data == nullptr)
		return false;

	load(rgba_data, width, height);
	stbi_image_free((void*)rgba_data);
	return true;
}

bool Font::load(const u8 *data, int width, int height)
{
	free();
	Color32* colors = (Color32*)data;
	if(colors == nullptr)
		return false;

	// parse image
	int n=0;
	for(;n<width*height; n++)
	{
		// Find font pixel
		if(!CHECK_COLOR(colors[n]))
			break;
	}

	int hspace = n % width;
	int vspace = n / width;

	// Get the hight of font
	int h = 0;
	for(int y=vspace+1; y<height; y++)
	{
		if(CHECK_COLOR(colors[y*width + hspace]))
		{
			h = y - vspace;
			break;
		}
	}

	int cnt = 0;
	// Parse all characters
	for(int y=vspace; y<height; y+=h+vspace)
	{
		int x = hspace;
		if(CHECK_COLOR(colors[y * width + x])){break;} // no more characters
		for(int cx=hspace; cx<width; cx++)
		{
			if(CHECK_COLOR(colors[y * width + cx]))
			{
				int w = cx - x;
				struct glyph_t g;
				int codepoint = 0x20 + cnt;
				g.x = x;
				g.y = y;
				g.w = w;
				g.h = h;
				g.advance = (float)g.w;
				g.bearing_x = 0;
				g.bearing_y = (float)-h;
				glyph_table[codepoint] = g;

				x += w + hspace;
				cnt++;
				if(CHECK_COLOR(colors[y * width + x])){break;} // no more characters on the horizon
			}
		}
	}
	size = (float)glyph_table[' '].h;
	ascent = size;
	descent = 0;
	line_space = size;

	// replace the system color
	for(int i=0; i<(width*height); i++)
	{
		if(CHECK_COLOR(colors[i]))
		{
			colors[i] = {0x00, 0x00, 0x00, 0xff};
		}
	}

	// flip data upside down
	for(int i=0; i<height/2; i++){
		for(int j=0; j<width; j++)
		{
			Color32 tmp = colors[(i*width)+j];
			colors[(i*width)+j] = colors[((height-i-1)*width)+j];
			colors[((height-i-1)*width)+j] = tmp;
		}
	}

	texture = gpu_create_texture((u8*)colors, width, height);	
	return true;
}

bool Font::load(const char *filename, float font_size)
{
	FILE *fp = fopen(filename, "rb");
	if(fp == nullptr) {
		return false;
	}

	// get file size
	fseek(fp, 0, SEEK_END);
	long file_size = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	font_data = new unsigned char[file_size];
	fread(font_data, 1, file_size, fp);
	fclose(fp);

	stbtt_InitFont(&stb_font, font_data, stbtt_GetFontOffsetForIndex(font_data, 0));
	size = font_size;

	int ascent, descent, line_gap;
	float scale = stbtt_ScaleForPixelHeight(&stb_font, size);
	stbtt_GetFontVMetrics(&stb_font, &ascent, &descent, &line_gap);
	this->ascent = (float)ascent * scale;
	this->descent = (float)descent * scale;
	line_space = (float)line_gap * scale;

	// load standard glyphs " " to "~"
	for(int i=0x20; i<0x7e; i++) {
		load_glyph(i);
	}
	update_texture();

	return true;
}

void Font::free()
{
	texture = nullptr;
	delete[] image_data;
	image_data = nullptr;
	delete[] font_data;
	font_data = nullptr;
	glyph_table.clear();
}

vec2 Font::calc_text_size(float font_size, const char *str, ...)
{
	static std::string buffer;
	//char buffer[1024];
	va_list arg_ptr;
	va_start(arg_ptr, str);
	//vsprintf(buffer, str, arg_ptr);
	int size = vsnprintf(nullptr, 0, str, arg_ptr);
	buffer.resize(size+1);
	vsnprintf(&buffer[0], size+1, str, arg_ptr);
	va_end(arg_ptr);

	float scale = font_size/this->size;
	float w = 0;
	float h = 0;
	float x = 0;
	float y = ascent;
	for(int i=0; buffer[i]!='\0'; i++)
	{
		if(buffer[i] == '\n')
		{
			x = 0;
			y += line_space;
			continue;
		}

		glyph_t g = get_glyph(buffer[i]);
		x += g.advance * scale;
		if(x > w) w = x;
		if(y > h) h = y + descent;
	}

	return vec2(w, h);
}

glyph_t Font::get_glyph(int codepoint)
{
	if(glyph_table.count(codepoint) == 0) {
		load_glyph(codepoint);
	}
	return glyph_table[codepoint];
}

void Font::load_glyph(int codepoint)
{
	if(stb_font.data == nullptr) return;
	if(glyph_table.count(codepoint) > 0) return;

	int glyph_index = stbtt_FindGlyphIndex(&stb_font, codepoint);
	if(glyph_index == 0)
	{
		glyph_t g = get_glyph('?');
		glyph_table[codepoint] = g;
		return;
	}

	// create image data
	if(image_data == nullptr)
	{
		image_data = new u8[IMAGE_SIZE * IMAGE_SIZE * PIXEL_DATA_STRIDE];
		for(int y=0; y<IMAGE_SIZE; y++) {
			for(int x=0; x<IMAGE_SIZE; x++) {
				image_data[(y*IMAGE_SIZE*PIXEL_DATA_STRIDE) + x*PIXEL_DATA_STRIDE] = 255;		// R
				image_data[(y*IMAGE_SIZE*PIXEL_DATA_STRIDE) + x*PIXEL_DATA_STRIDE + 1] = 255;	// G
				image_data[(y*IMAGE_SIZE*PIXEL_DATA_STRIDE) + x*PIXEL_DATA_STRIDE + 2] = 255;	// B
				image_data[(y*IMAGE_SIZE*PIXEL_DATA_STRIDE) + x*PIXEL_DATA_STRIDE + 3] = 0;	// A
			}
		}
		texture = gpu_create_texture(nullptr, 0, 0);
		packer_x_offset = pack_margin;
		packer_y_offset = pack_margin;
	}

	int width, height;
	float scale = stbtt_ScaleForPixelHeight(&stb_font, size);
	u8 *pixels = stbtt_GetCodepointBitmap(&stb_font, scale, scale, codepoint, &width, &height, 0, 0);

	// check pack space
	if(packer_x_offset + width + pack_margin >= IMAGE_SIZE) {
		packer_x_offset = pack_margin;
		packer_y_offset = pack_heighest + pack_margin;
	}
	if(packer_y_offset + height + pack_margin >= IMAGE_SIZE) {
		// no packing space
		stbtt_FreeBitmap(pixels, 0);
		glyph_t g = get_glyph('?');
		glyph_table[codepoint] = g;
		return ;
	}

	// update pack heighest
	if(packer_y_offset + height > pack_heighest) {
		pack_heighest = packer_y_offset + height;
	}

	// copy font pixels to image_data
	for(int y=0; y<height; y++) {
		for(int x=0; x<width; x++) {
			image_data[((packer_y_offset + y) * IMAGE_SIZE * PIXEL_DATA_STRIDE) + (packer_x_offset + x) * PIXEL_DATA_STRIDE + 3] = pixels[(y*width) + x];
		}
	}

	int advance, left_bearing, min_x, min_y, max_x, max_y;
	stbtt_GetCodepointHMetrics(&stb_font, codepoint, &advance, &left_bearing);
	stbtt_GetCodepointBitmapBox(&stb_font, codepoint, scale, scale, &min_x, &min_y, &max_x, &max_y);
	glyph_t g;
	g.w = width;
	g.h = height;
	g.x = packer_x_offset;
	g.y = packer_y_offset;
	g.advance = (float)advance * scale;
	g.bearing_x = (float)left_bearing * scale;
	g.bearing_y = (float)min_y;
	glyph_table[codepoint] = g;
	packer_x_offset += width + pack_margin;
	stbtt_FreeBitmap(pixels, 0);
	need_update_texture = true;
}

void Font::load_glyphs(int *codepoints, int count)
{
	for(int i=0; i<count; i++)
	{
		load_glyph(codepoints[i]);
	}
}

void Font::update_texture()
{
	if(need_update_texture)
	{
		// flip image_data
		static u8 tmp[IMAGE_SIZE * IMAGE_SIZE * PIXEL_DATA_STRIDE];
		for(int y=0; y<IMAGE_SIZE; y++)	{
			for(int x=0; x<IMAGE_SIZE; x++) {
				int src_y = IMAGE_SIZE - y - 1;
				for(int n=0; n<4; n++)
				{
					tmp[(y*IMAGE_SIZE*PIXEL_DATA_STRIDE) + x * PIXEL_DATA_STRIDE + n] = image_data[(src_y*IMAGE_SIZE*PIXEL_DATA_STRIDE) + x * PIXEL_DATA_STRIDE + n];
				}
			}
		}
		texture->create(tmp, IMAGE_SIZE, IMAGE_SIZE);
		need_update_texture = false;
	}
}