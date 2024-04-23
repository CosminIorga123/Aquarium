#pragma once
#include <stdlib.h>
#include <stdio.h>
#include <math.h> 
#include <GL/glew.h>
#define GLM_FORCE_CTOR_INIT 
#include <GLM.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>

#include <glfw3.h>

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>


#pragma comment (lib, "glfw3dll.lib")
#pragma comment (lib, "glew32.lib")
#pragma comment (lib, "OpenGL32.lib")

class Shader
{
public:
	// constructor generates the shaderStencilTesting on the fly
	// ------------------------------------------------------------------------
	Shader(const char* vertexPath, const char* fragmentPath);
	~Shader();
	void Use() const;
	unsigned int GetID() const;

	unsigned int loc_model_matrix;
	unsigned int loc_view_matrix;
	unsigned int loc_projection_matrix;

	// utility uniform functions
	void SetInt(const std::string& name, int value) const;
	void SetFloat(const std::string& name, float value) const;
	void SetVec3(const std::string& name, const glm::vec3& value) const;
	void SetVec3(const std::string& name, float x, float y, float z) const;
	void SetMat4(const std::string& name, const glm::mat4& mat) const;

private:
	void Init(const char* vertexPath, const char* fragmentPath);
	void CheckCompileErrors(unsigned int shaderStencilTesting, std::string type);

private:
	unsigned int ID;
};