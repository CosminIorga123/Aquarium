#pragma once
#include "RenderableObject.h"
class AquariumObj : public RenderableObject
{
private:

public:
	AquariumObj(std::string&& modelsPath);
	~AquariumObj();
	void draw(Shader* sp);
	void behave();
};

