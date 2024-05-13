#pragma once
#include "Model.h"
#include <random>

extern std::mt19937 gen;

//random values, need to be updated
const float MAX_X = 9.0f;
const float MAX_Y = 5.0f;
const float MAX_Z = 5.5f;
const float MIN_Y = 1.3f;

class RenderableObject
{
	protected:
		float x, y, z;
		//pozitia obiectului
		float rx, ry, rz;	//rotatia obiectului
		float sx, sy, sz;	//marimea obiectului

		//obiectul
		Model* object;

		template<class T>
		T random(T min, T max);

	public:
		RenderableObject();
		~RenderableObject();
		virtual void draw(Shader* sp) = 0;
		virtual void behave() = 0;
};

template<class T>
T RenderableObject::random(T min, T max)
{
	using dist = std::conditional_t<
		std::is_integral<T>::value,
		std::uniform_int_distribution<T>,
		std::uniform_real_distribution<T>>;
	return dist{ min, max }(gen);
}