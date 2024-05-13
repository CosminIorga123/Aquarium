#include "Fish.h"

std::vector<std::string> my_fishes= {
	"\\Fish01\\13007_Blue-Green_Reef_Chromis_v2_l3.obj", "\\Fish02\\OBJ.obj"
};


Fish::Fish(std::string&& modelsPath)
{
	int randomFishIndex = random(0, 1);
	object = new Model(modelsPath + my_fishes[randomFishIndex], false);


	x = random(-MAX_X, MAX_X); y = random(1.0f, MAX_Y); z = random(-MAX_Z, MAX_Z);
	rx = random(0.0f, 360.0f); ry = random(0.0f, 360.0f); rz = 0.0f;
	float scale = random(1.f, 10.f);
	sx = scale;  sy = scale; sz = scale;

}

Fish::~Fish()
{
	std::cout << "destructor of fish" << std::endl;
}

void Fish::draw(Shader* sp)
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

void Fish::behave()
{
}
