#include "AquariumObj.h"

AquariumObj::AquariumObj(std::string&& modelsPath)
{
	object = new Model(modelsPath+"\\saltwater_aquarium_v1_L1.123cdde764e6-103e-4374-98e3-c8863fc34c2c\\12987_Saltwater_Aquarium_v1_l1.obj"
		, false);
	x = 0.0f; y = 0.0f; z = 0.0f;
	rx = -90.0f; ry = 0.0f; rz = 0.0f;
	float scale = 1.0f;
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
