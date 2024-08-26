#include "app.h"
#include "input.h"
#include "gpu.h"
#include "collision.h"
#include "renderer.h"
#include "scene_edit.h"
#include "../map.h"
#include "../foliage_system.h"
#include "../entity/entity.h"
#include "../entity/player.h"
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"

static struct scene_edit_ctx
{
	int tmp;

	struct
	{
		vec3 position;
		float size;
		bool is_drawing;
	} brush;

	vec2 cam_angle;
	CameraRef camera;
	char model_file_name[64];
	int current_tool;

	TeeingArea teeing_area;
	ModelRef teeing_area_model;


	MeshRef circle_mesh;
	MaterialRef circle_mat;
} ctx = {};

static const float TOOL_RAY_DISTANCE = 500.0f;
static const float CAMERA_SPEED = 10.0f;
static const float CAMERA_FAST_MOVE_FACTOR = 4.0f;

enum
{
	TOOL_FOLIAGE,
	TOOL_HOLE,
	TOOL_TEEING_AREA,
	MAX_TOOLS,
};

static char tool_names[MAX_TOOLS][32] = {"Foliage", "Hole", "Teeing"};

void EditScene::init()
{
	ctx.camera = renderer_create_camera();
	map_init();
	MaterialRef mat = create_material("unlit");
	mat->color = vec4(0.7f, 1.0f, 0.7f, 1.0f);
	ctx.teeing_area_model = create_model(create_box_mesh(vec3(1,1,1)), mat);
	ctx.teeing_area.width = 4.0f;

	// camera
	ctx.camera->position = vec3(0, 3, -8);


	// circle wire model
	const int count = 32;
	const float d = 360.0f/ (float)count;
	Vertex verts[count] = {};
	u16 indices[count*2];
	for(int i=0; i<count; i++)
	{
		verts[i].position = quat(0, (float)i * d, 0) * vec3(0,0,1);
		indices[i * 2] = i;
		indices[i * 2 + 1] = i<count-1 ? i + 1 : 0;
	}
	ctx.circle_mesh = gpu_create_mesh(verts, count, indices, count*2);
	ctx.circle_mat = create_material("unlit");
	ctx.circle_mat->color = vec4(0,0,1,1);

	//player = new Player();
	//g_ball = new Ball();
}

void EditScene::uninit()
{
	//delete player;
	//delete g_ball;
	map_uninit();
}

void EditScene::update()
{
	if(input_hit(KEY_ESCAPE))
	{
		app_quit();
	}

	auto io = ImGui::GetIO();
	bool is_window_hovered = io.WantCaptureMouse; 

	ImGui::Begin("Editor");

	// Map Load
	static char map_file_name[64];
	ImGui::Text("Map Model File");
	ImGui::InputText("##map_file_name", map_file_name, 64);
	//if(ImGui::IsItemActive()){is_imgui_active = true;}
	ImGui::SameLine();
	if(ImGui::Button("Load##load_map"))
	{
		map_load(map_file_name);
		const MapData *data = map_get_data();
		strcpy(ctx.model_file_name, data->model_name);
		memcpy(&ctx.teeing_area, &data->teeing_area, sizeof(ctx.teeing_area));
	}
	ImGui::SameLine();
	if(ImGui::Button("Save##save_map"))
	{
		map_save(map_file_name);
	}


	// Model Load
	ImGui::Text("Map Model File");
	ImGui::InputText("##map_model_name", ctx.model_file_name, 64);
	//if(ImGui::IsItemActive()){is_imgui_active = true;}
	ImGui::SameLine();
	if(ImGui::Button("Load##load_model"))
	{
		//ModelRef map_model = load_model(model_file_name);
		//if(map_model)
		//{
		//	map_model->material = create_material("unlit");
		//	map_model->material->color = vec4(0.4f, 0.7f, 0.4f, 1);
		//	map_set_model(map_model);
		//}
		map_load_model(ctx.model_file_name);
	}

	ImGui::End();



	// Tool window
	ImGui::Begin("Tools");
	// tool select button
	{
		static int selected = 0;
		if (ImGui::BeginTable("split1", 3, ImGuiTableFlags_Resizable | ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_Borders))
		{
			for (int i=0; i<MAX_TOOLS; i++)
			{
				//char label[32];
				//sprintf(label, "Item %d", tool_names[i]);
				ImGui::TableNextColumn();
				if(ImGui::Selectable(tool_names[i], selected == i))
					selected = i;

				if(i==0 && ImGui::IsItemHovered())
					ImGui::SetTooltip("LeftClick - Draw objects\nCtrl + LeftClick - Remove objects\nShift + LeftClick - Place a object");
			}
			ImGui::EndTable();
		}
		ctx.current_tool = selected;
	}
	ImGui::Spacing();

	// tool settings
	static float spacing_factor = 1.0f;
	static int selected_foliage_type = 1;
	switch(ctx.current_tool)
	{
		case TOOL_FOLIAGE:
			{
				ImGui::DragFloat("Brush Size##brush_size", &ctx.brush.size, 0.1f, 0.0f, 100.0f);
				ImGui::DragFloat("Desity Factor##spacing_factor", &spacing_factor, 0.01f, 0.01f, 10.0f);

				ImGui::Spacing();
				//ImGui::LabelText("Foliage Objects");
				ImGui::SeparatorText("Foliage Objects");
				const FoliageDesc *descs = foliage_get_descs();
				for(int i=0; i<(int)FoliageType::max_types; i++)
				{
					if(descs[i].type == FoliageType::none) continue;
					char buf[64];
					sprintf(buf, "%s##foliage_type_%d", descs[i].name, descs[i].type);
					if(ImGui::Selectable(buf, selected_foliage_type == i))
					{
						selected_foliage_type = i;
					}
				}
			}
			break;

		case TOOL_HOLE:
			{
				vec3 pos = map_get_hole_position();
				if(ImGui::DragFloat("Y Position##hole_y_position", &pos.y, 0.001f))
				{
					map_set_hole(pos);
				}
			}
			break;
		case TOOL_TEEING_AREA:
			{
				ImGui::DragFloat("Width##teeing_area_width", &ctx.teeing_area.width, 0.1f);
				ImGui::DragFloat("Angle##teeing_area_angle", &ctx.teeing_area.angle);
				map_set_teeing_area(ctx.teeing_area.position, ctx.teeing_area.width, ctx.teeing_area.angle);
			}
			break;

		default:
			break;
	}

	ImGui::End();







	// camera rotation
	if(!is_window_hovered && input_down(KEY_MB1))
	{
		ImGui::SetWindowFocus(NULL);
		input_mouse_lock(1);
		vec2 mouse = input_get_mouse_delta();
		ctx.cam_angle.x += mouse.y * 0.1f;
		ctx.cam_angle.y += -mouse.x * 0.1f;
		ctx.cam_angle.x = clamp(ctx.cam_angle.x, -89, 89);

		// camera move ment
		vec3 move;
		quat rotation =  quat::euler(0, ctx.cam_angle.y, 0) * quat::euler(ctx.cam_angle.x, 0, 0);
		float speed = CAMERA_SPEED;
		vec3 forward = rotation.forward();
		vec3 right = rotation.right();
		if(input_down(KEY_W))
		{
			move += forward;
		}
		if(input_down(KEY_S))
		{
			move -= forward;
		}
		if(input_down(KEY_A))
		{
			move -= right;
		}
		if(input_down(KEY_D))
		{
			move += right;
		}
		if(input_down(KEY_SHIFT))
		{
			speed *= CAMERA_FAST_MOVE_FACTOR;
		}
		ctx.camera->position += move * speed * time_dt();
		ctx.camera->rotation = rotation;
	}
	else
	{
		input_mouse_lock(0);
	}



	ctx.brush.is_drawing = false;
	ray_t ray = ctx.camera->get_ray(input_get_mouse_pos());
	ray.dir = ray.dir * TOOL_RAY_DISTANCE;
	RayHit hitinfo;
	bool rayhit = raycast(ray, &hitinfo);
	switch(ctx.current_tool)
	{
		case TOOL_FOLIAGE:
		{
			if(!is_window_hovered && rayhit)
			{
				ctx.brush.is_drawing = true;
				ctx.brush.position = hitinfo.point + hitinfo.normal * 0.05f;

				// place the plants
				if(input_down(KEY_SHIFT))
				{
					if(input_hit(KEY_MB0))
						foliage_add((FoliageType)selected_foliage_type, hitinfo.point);
				}
				else if(input_down(KEY_ALT))
				{
					if(input_hit(KEY_MB0))
						foliage_add_replace((FoliageType)selected_foliage_type, hitinfo.point, ctx.brush.size);
				}
				else
				{
					if(input_down(KEY_MB0))
					{
						if(input_down(KEY_CNTROL))
						{
							// remove
							foliage_remove(hitinfo.point, ctx.brush.size);
						}
						else
						{
							// draw
							foliage_add((FoliageType)selected_foliage_type, hitinfo.point, ctx.brush.size, spacing_factor);
						}
					}
				}
			}
		}
		break;
		case TOOL_HOLE:
		{
			if(!is_window_hovered && input_hit(KEY_MB0) && rayhit)
			{
				map_set_hole(hitinfo.point + vec3(0, 0.008f, 0));
			}
		}
		break;
		case TOOL_TEEING_AREA:
		{
			if(!is_window_hovered && input_hit(KEY_MB0) && rayhit)
			{
				ctx.teeing_area.position = hitinfo.point;
				map_set_teeing_area(ctx.teeing_area.position, ctx.teeing_area.width, ctx.teeing_area.angle);
			}
		}
		break;
	}
}

void EditScene::draw()
{
	// rendering
	map_draw();

	// teeing area
	quat rot = quat(0, ctx.teeing_area.angle, 0);
	draw_model(ctx.teeing_area_model, mat4(ctx.teeing_area.position, rot, vec3(ctx.teeing_area.width, 1, 2.5f)));

	// draw brush gizmo
	if(ctx.brush.is_drawing)
	{
		if(input_down(KEY_CNTROL))
		{
			// remove
			ctx.circle_mat->color = vec4(1,0,0,1);
			draw_mesh_lines(ctx.circle_mesh, ctx.circle_mat, mat4(ctx.brush.position, quat::identity(), vec3(ctx.brush.size, 1 ,ctx.brush.size)));
		}
		else if(input_down(KEY_SHIFT))
		{
			// single place
			ctx.circle_mat->color = vec4(1,1,0,1);
			draw_mesh_lines(ctx.circle_mesh, ctx.circle_mat, mat4(ctx.brush.position, quat::identity(), vec3(0.5f,1,0.5f)));
		}
		else if(input_down(KEY_ALT))
		{
			ctx.circle_mat->color = vec4(1,0,1,1);
			draw_mesh_lines(ctx.circle_mesh, ctx.circle_mat, mat4(ctx.brush.position, quat::identity(), vec3(ctx.brush.size, 1, ctx.brush.size)));
		}
		else
		{
			// draw
			ctx.circle_mat->color = vec4(0,0,1,1);
			draw_mesh_lines(ctx.circle_mesh, ctx.circle_mat, mat4(ctx.brush.position, quat::identity(), vec3(ctx.brush.size, 1, ctx.brush.size)));
		}
	}
}