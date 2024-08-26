#pragma once
#include "scene_base.h"

class EditScene : public Scene
{
public:
	EditScene(){}
	~EditScene(){}

	void init();
	void uninit();
	void update();
	void draw();
};