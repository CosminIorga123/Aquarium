#pragma once
#include "RenderableObject.h"
class Fish : public RenderableObject
{
private:

public:
	Fish(std::string&& modelsPath);
	~Fish();
	void draw(Shader* sp);
	void behave();

};

