static char *unlit_vs_src = 
R"(
		#version 330

		layout(location = 0) in vec3 _position;
		layout(location = 1) in vec3 _normal;
		layout(location = 2) in vec2 _texcoord;
		layout(location = 3) in vec4 _color;

		uniform mat4 _model;
		uniform mat4 _view;
		uniform mat4 _projection;

		out vec2 o_texcoord;
		out vec4 o_color;

		void main()
		{
			gl_Position = _projection * _view * _model * vec4(_position, 1.0);
			//gl_Position = vec4(_position, 1.0);
			o_texcoord = _texcoord;
			o_color = _color;
		}
)";

static char *unlit_vs_skin_src = 
R"(
		#version 330

		layout(location = 0) in vec3 _position;
		layout(location = 1) in vec3 _normal;
		layout(location = 2) in vec2 _texcoord;
		layout(location = 3) in vec4 _color;
		layout(location = 4) in ivec4 _bone_ids;
		layout(location = 5) in vec4 _bone_weights;

		uniform mat4 _model;
		uniform mat4 _view;
		uniform mat4 _projection;
		#define MAX_BONES 128
		uniform mat4 _bone_matrix[MAX_BONES];

		out vec2 o_texcoord;
		out vec4 o_color;

		void main()
		{
			mat4 bone_transform = _bone_matrix[_bone_ids[0]] * _bone_weights[0];
			bone_transform += _bone_matrix[_bone_ids[1]] * _bone_weights[1];
			bone_transform += _bone_matrix[_bone_ids[2]] * _bone_weights[2];
			bone_transform += _bone_matrix[_bone_ids[3]] * _bone_weights[3];
			vec4 local_position = bone_transform * vec4(_position, 1.0);

			gl_Position = _projection * _view * _model * local_position;

			o_texcoord = _texcoord;
			o_color = _color;
		}
)";

static char *unlit_vs_inst_src = 
R"(
		#version 330

		layout(location = 0) in vec3 _position;
		layout(location = 1) in vec3 _normal;
		layout(location = 2) in vec2 _texcoord;
		layout(location = 3) in vec4 _color;
		layout(location = 4) in mat4 _instance_matrix;

		uniform mat4 _model;
		uniform mat4 _view;
		uniform mat4 _projection;

		out vec2 o_texcoord;
		out vec4 o_color;

		void main()
		{
			gl_Position = _projection * _view * _instance_matrix * vec4(_position, 1.0);
			//gl_Position = vec4(_position, 1.0);
			o_texcoord = _texcoord;
			o_color = _color;
		}
)";

static char *unlit_billboard_vs_inst_src = 
R"(
		#version 330

		layout(location = 0) in vec3 _position;
		layout(location = 1) in vec3 _normal;
		layout(location = 2) in vec2 _texcoord;
		layout(location = 3) in vec4 _color;
		layout(location = 4) in mat4 _instance_matrix;

		uniform mat4 _model;
		uniform mat4 _view;
		uniform mat4 _projection;

		out vec2 o_texcoord;
		out vec4 o_color;

		void main()
		{
			mat4 model_view = _view * _instance_matrix;
			model_view[0][0] = 1.0;
			model_view[0][1] = 0.0;
			model_view[0][2] = 0.0;

			// if you want cylindrical billboard, comment out this block ==========
			model_view[1][0] = 0.0;
			model_view[1][1] = 1.0;
			model_view[1][2] = 0.0;
			// ====================================================================

			model_view[2][0] = 0.0;
			model_view[2][1] = 0.0;
			model_view[2][2] = 1.0;

			mat4 m = _instance_matrix;
			m[3] = vec4(0,0,0,1);

			vec4 p = model_view * m * vec4(_position, 1.0);
			gl_Position = _projection * p;
			o_texcoord = _texcoord;
			o_color = _color;
		}
)";


static char *unlit_fs_src =
R"(
		#version 330

		out vec4 frag_color;
		in vec2 o_texcoord;
		in vec4 o_color;

		uniform vec4 _color;
		uniform sampler2D _main_tex;

		void main()
		{
			frag_color = texture(_main_tex, o_texcoord) * o_color * _color;
			// frag_color = o_color;
		}
)";

static char *unlit_cutout_fs_src =
R"(
		#version 330

		out vec4 frag_color;
		in vec2 o_texcoord;
		in vec4 o_color;

		uniform vec4 _color;
		uniform sampler2D _main_tex;

		void main()
		{
			vec4 color = texture(_main_tex, o_texcoord) * o_color * _color;
			if(color.a < 0.1)
				discard;

			frag_color = vec4(color.rgb, 1.0);
		}
)";