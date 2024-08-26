static char *postfx_vs_src = 
R"(
	#version 330

	layout(location = 0) in vec3 _position;
	layout(location = 2) in vec2 _texcoord;
	layout(location = 3) in vec4 _color;

	out vec2 o_texcoord;
	out vec4 o_color;

	void main()
	{
		gl_Position =  vec4(_position, 1.0);
		o_texcoord = _texcoord;
		o_color = _color;
	}
)";


static char *postfx_fog_src =
R"(
	#version 330

	#define remap(x,in0,in1,out0,out1) ((out0)+((out1)-(out0))*((x)-(in0))/((in1)-(in0)))

	out vec4 frag_color;
	in vec2 o_texcoord;
	in vec4 o_color;

	uniform float _near;
	uniform float _far;
	uniform float _fog_start;
	uniform float _fog_end;
	uniform vec4 _color;
	uniform sampler2D _main_tex;
	uniform sampler2D _depth_tex;

	void main()
	{
		float depth = texture(_depth_tex, o_texcoord).x * 2.0 - 1.0;
		float linear_z = (2.0 * _near) / (_far + _near - depth * (_far - _near));
		vec4 color = texture(_main_tex, o_texcoord);
		float fog_start = remap(_fog_start, _near, _far, 0.0, 1.0);
		float fog_end = remap(_fog_end, _near, _far, 0.0, 1.0);
		frag_color = mix(color, _color, clamp(remap(linear_z, fog_start, fog_end, 0.0, 1.0), 0.0, 1.0));
	}
)";


static char *postfx_ssao_src =
R"(
	#version 330

	#define remap(x,in0,in1,out0,out1) ((out0)+((out1)-(out0))*((x)-(in0))/((in1)-(in0)))

	out vec4 frag_color;
	in vec2 o_texcoord;
	in vec4 o_color;

	uniform float _radius;
	uniform float _bias;
	uniform vec3 _samples[64];
	uniform mat4 _proj;		// _projection use as 2D projection matrix in vertex shader
	uniform mat4 _invprojection;
	uniform sampler2D _main_tex;
	uniform sampler2D _depth_tex;

	vec3 viewpos_from_depth(vec2 coord, float depth)
	{
		vec4 clip_pos = vec4(coord * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);
		vec4 view_pos = _invprojection * clip_pos;
		return view_pos.xyz / view_pos.w;
	}

	#define remap(x,in0,in1,out0,out1) ((out0)+((out1)-(out0))*((x)-(in0))/((in1)-(in0)))

	float rand(vec2 co){
		return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453);
	}

	void main()
	{
		vec3 view_pos = viewpos_from_depth(o_texcoord, texture(_depth_tex, o_texcoord).x);


		float occlusion = 0.0;
		for(int i=0; i<64; i++)
		{
			// random sampling rotation (make dithering AO)
			vec3 normal = normalize(vec3(rand(view_pos.xy), rand(view_pos.yz), rand(view_pos.zx)));
			vec3 rndvec = normalize(vec3(rand(view_pos.yx), rand(view_pos.zy), rand(view_pos.xz)));
			vec3 tangent =  normalize(rndvec - normal * dot(rndvec, normal));
			vec3 bitangent = cross(normal, tangent);
			mat3 tbn = mat3(tangent, bitangent, normal);
			vec3 offset = tbn * _samples[i];

			vec3 offset_view_pos = view_pos + offset * _radius;
			vec4 offset_clip_pos = _proj * vec4(offset_view_pos, 1.0);

			vec2 sampling_coord = (offset_clip_pos.xy / offset_clip_pos.w) * 0.5 + 0.5;
			float sampling_raw_depth = texture(_depth_tex, sampling_coord).x;
			vec3 sampling_view_pos = viewpos_from_depth(sampling_coord, sampling_raw_depth);

			if(length(view_pos - sampling_view_pos) > 5.0)
				continue;

			float range_check = smoothstep(0.0, 1.0, _radius / abs(view_pos.z - sampling_view_pos.z));
			if(sampling_view_pos.z >= offset_view_pos.z + _bias)
			{
				occlusion += 1.0 * range_check;
			}
		}

		occlusion = 1.0 - occlusion / 64.0;

		//frag_color = vec4(0.0, p.y, 0.0, 1.0);
		occlusion = clamp(occlusion + 0.5, 0.0, 1.0);
		//occlusion = clamp(remap(occlusion, 0.1, 0.5, 0.0, 1.0), 0.0, 1.0);
		frag_color = vec4(texture(_main_tex, o_texcoord).xyz * occlusion, 1.0);
		//frag_color = vec4(o_texcoord, 0.0, 1.0);
		frag_color = vec4(occlusion, occlusion, occlusion, 1.0);
	}
)";


static char *postfx_ssao_blur_src =
R"(
	#version 330 core

	out vec4 frag_color;
	in vec2 o_texcoord;
  
	uniform sampler2D _main_tex;
	uniform sampler2D _ssao_tex;

	void main() {
		vec2 texel_size = 1.0 / vec2(textureSize(_ssao_tex, 0));
		float result = 0.0;
		for(int x = -2; x < 2; ++x) 
		{
			for(int y = -2; y < 2; ++y) 
			{
				vec2 offset = vec2(float(x), float(y)) * texel_size;
				result += texture(_ssao_tex, o_texcoord + offset).r;
			}
		}
		float c = result / (4.0 * 4.0);
		//c = texture(_ssao_tex, o_texcoord).r;
		frag_color = texture(_main_tex, o_texcoord) * vec4(c,c,c,1.0);//mix(vec4(0, 0, 0, 1.0), texture(_main_tex, o_texcoord), c);
	}
)";