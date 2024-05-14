#include "AquariumObj.h"

AquariumObj::AquariumObj(std::string&& modelsPath)
{
	object = new Model(modelsPath+"\\Aquarium\\Fishtank.obj"
		, false);
	x = 0.0f; y = 0.0f; z = 0.0f;
	rx = 0.0f; ry = 0.0f; rz = 0.0f;
	float scale = 5.0f;
	sx = scale;  sy = scale; sz = scale;
}

AquariumObj::~AquariumObj()
{
	std::cout << "destructor of aquarium object" << std::endl;
}

void AquariumObj::draw(Shader* sp)
{
	glm::mat4 Model = glm::mat4(1.0f);
	Model = glm::scale(Model, glm::vec3(sx, sy, sz));
	Model = glm::translate(Model, glm::vec3(x, y, z));
	Model = glm::rotate(Model, glm::radians(this->rx), glm::vec3(1.0f, 0.0f, 0.0f));
	Model = glm::rotate(Model, glm::radians(this->ry), glm::vec3(0.0f, 1.0f, 0.0f));
	Model = glm::rotate(Model, glm::radians(this->rz), glm::vec3(0.0f, 0.0f, 1.0f));
	sp->setMat4("model", Model);
	object->Draw(*sp);
}

void AquariumObj::behave()
{
}
