#pragma once
#include "scene_base.h"

class GameScene : public Scene
{
public:
	GameScene(){}
	~GameScene(){}

	void init();
	void uninit();
	void update();
	void draw();
};