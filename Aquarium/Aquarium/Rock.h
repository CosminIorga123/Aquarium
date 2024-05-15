#pragma once
#include "RenderableObject.h"
class Rock : public RenderableObject
{
private:
public:
    Rock(std::string&& modelsPath);
    ~Rock();
    void draw(Shader* sp);
};