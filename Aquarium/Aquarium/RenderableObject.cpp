#include "RenderableObject.h"
std::mt19937 gen{ std::random_device{}() };

RenderableObject::RenderableObject()
{
	this->x = 0.0f;
	this->y = 0.0f;
	this->z = 0.0f;

	this->rx = 0.0f;
	this->ry = 0.0f;
	this->rz = 0.0f;

	this->sx = 1.0f;
	this->sy = 1.0f;
	this->sz = 1.0f;

	object = nullptr;
}

RenderableObject::~RenderableObject()
{
	if (object)
		delete object;
	std::cout << "destructor of renderable" << std::endl;
}