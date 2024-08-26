#pragma once

class Scene
{
public:
	Scene(){};
	~Scene(){};
	
	virtual void init(){};
	virtual void uninit(){};
	virtual void update(){};
	virtual void draw(){};
};