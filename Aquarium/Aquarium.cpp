#define STB_IMAGE_IMPLEMENTATION
#define STBI_HEADER_FILE_ONLY
#define _CRT_SECURE_NO_WARNINGS

#include <Windows.h>
#include <locale>
#include <codecvt>
#include <vector>

#include "Camera.h"
#include "ECameraMovement.h"
#include "Model.h"

#include "stb_image.h"
#include <minwindef.h>

// settings
const unsigned int SCR_WIDTH = 1080;
const unsigned int SCR_HEIGHT = 720;

static std::string GetCurrentPath()
{
	wchar_t buffer[MAX_PATH];
	GetCurrentDirectoryW(MAX_PATH, buffer);

	std::wstring executablePath(buffer);
	std::wstring wscurrentPath = executablePath.substr(0, executablePath.find_last_of(L"\\/"));

	std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
	std::string currentPath = converter.to_bytes(wscurrentPath);
	return currentPath;
}


Camera* pCamera = nullptr;

std::string currentPath = GetCurrentPath();
void setFaces(std::vector<std::string>& faces, unsigned int& cubemapTexture);
static unsigned int CreateTexture(const std::string& strTexturePath)
{
	unsigned int textureId = -1;

	// load image, create texture and generate mipmaps
	int width, height, nrChannels;
	stbi_set_flip_vertically_on_load(true); // tell stb_image.h to flip loaded texture's on the y-axis.
	unsigned char* data = stbi_load(strTexturePath.c_str(), &width, &height, &nrChannels, 0);
	if (data) {
		GLenum format;
		if (nrChannels == 1)
			format = GL_RED;
		else if (nrChannels == 3)
			format = GL_RGB;
		else if (nrChannels == 4)
			format = GL_RGBA;

		glGenTextures(1, &textureId);
		glBindTexture(GL_TEXTURE_2D, textureId);
		glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);

		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	}
	else {
		std::cout << "Failed to load texture: " << strTexturePath << std::endl;
	}
	stbi_image_free(data);

	return textureId;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window, std::vector<std::string>& faces, unsigned int& cubemapTexture);

void renderScene(const Shader& shader, unsigned int floorTexture, unsigned int cubeTexture);
void renderCube();
void renderFloor();
unsigned int loadCubemap(const std::vector<std::string>& faces);
glm::vec3 lightPos(-2.0f, 4.0f, -1.0f);
// timing
double deltaTime = 0.0f;	// time between current frame and last frame
double lastFrame = 0.0f;
bool rotatingLight = false;
bool isDay = true;

int main(int argc, char** argv)
{
	//use if you want exe path
	/*std::string strFullExeFileName = argv[0];
	std::string strExePath;
	const size_t last_slash_idx = strFullExeFileName.rfind('\\');
	if (std::string::npos != last_slash_idx) {
		strExePath = strFullExeFileName.substr(0, last_slash_idx);
	}*/

	// glfw: initialize and configure
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// glfw window creation
	GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Aquarium", NULL, NULL);
	if (window == NULL) {
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}

	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetScrollCallback(window, scroll_callback);

	// tell GLFW to capture our mouse
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	glewInit();

	// Create camera
	pCamera = new Camera(SCR_WIDTH, SCR_HEIGHT, glm::vec3(0.0, 1.0, 10.0));

	// configure global opengl state
	// -----------------------------
	glEnable(GL_DEPTH_TEST);

	// build and compile shaders
	// -------------------------
	Shader shadowMappingShader((currentPath + "\\Shaders" + "\\ShadowMapping.vs").c_str(), (currentPath + "\\Shaders" + "\\ShadowMapping.fs").c_str());
	Shader shadowMappingDepthShader((currentPath + "\\Shaders" + "\\ShadowMappingDepth.vs").c_str(), (currentPath + "\\Shaders" + "\\ShadowMappingDepth.fs").c_str());
	Shader cubeMapsShader((currentPath + "\\Shaders" + "\\CubeMaps.vs").c_str(), (currentPath + "\\Shaders" + "\\CubeMaps.fs").c_str());
	Shader skyBoxShader((currentPath + "\\Shaders" + "\\SkyBox.vs").c_str(), (currentPath + "\\Shaders" + "\\SkyBox.fs").c_str());
	Shader fishShader((currentPath + "\\Shaders" + "\\Model.vs").c_str(), (currentPath + "\\Shaders" + "\\Model.fs").c_str());
	Shader fish2Shader((currentPath + "\\Shaders" + "\\Model.vs").c_str(), (currentPath + "\\Shaders" + "\\Model.fs").c_str());
	Shader aquariumShader((currentPath + "\\Shaders" + "\\Model.vs").c_str(), (currentPath + "\\Shaders" + "\\Model.fs").c_str());
	Shader fish3Shader((currentPath + "\\Shaders" + "\\Model.vs").c_str(), (currentPath + "\\Shaders" + "\\Model.fs").c_str());
	Shader fish4Shader((currentPath + "\\Shaders" + "\\Model.vs").c_str(), (currentPath + "\\Shaders" + "\\Model.fs").c_str());
	Shader rockShader((currentPath + "\\Shaders" + "\\Model.vs").c_str(), (currentPath + "\\Shaders" + "\\Model.fs").c_str());

	// load textures
	// -------------
	unsigned int tableTexture = CreateTexture(currentPath + "\\Textures\\TableTexture.jpg");
	unsigned int cubeTexture = CreateTexture(currentPath + "\\Textures\\CubeTexture.png");

	std::string modelFileName = currentPath + "\\Models\\TropicalFish9\\TropicalFish09.obj";
	std::string modelFileName2 = currentPath + "\\Models\\Fish01\\13007_Blue-Green_Reef_Chromis_v2_l3.obj";
	std::string modelFileName3 = currentPath + "\\Models\\saltwater_aquarium_v1_L1.123cdde764e6-103e-4374-98e3-c8863fc34c2c\\12987_Saltwater_Aquarium_v1_l1.obj";
	std::string modelFileName4 = currentPath + "\\Models\\Fish02\\OBJ.obj";
	std::string modelFileName5 = currentPath + "\\Models\\Boesemani_Rainbow_v1_L2.123cbc6a2e9e-f317-488f-abf9-8b3bcb3898c5\\12999_Boesemani_Rainbow_v1_l2.obj";
	std::string modelFileName6 = currentPath + "\\Models\\Rock1\\Rock1.obj";
	Model fish(modelFileName, false);
	Model fish2(modelFileName2, false);
	Model aquarium(modelFileName3, false);
	Model fish3(modelFileName4, false);
	Model fish4(modelFileName5, false);
	Model rock(modelFileName6, false);

	// configure depth map FBO
	// -----------------------
	const unsigned int SHADOW_WIDTH = 4096, SHADOW_HEIGHT = 4096;
	unsigned int depthMapFBO;
	glGenFramebuffers(1, &depthMapFBO);
	// create depth texture
	unsigned int depthMap;
	glGenTextures(1, &depthMap);
	glBindTexture(GL_TEXTURE_2D, depthMap);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	float borderColor[] = { 1.0, 1.0, 1.0, 1.0 };
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
	// attach depth texture as FBO's depth buffer
	glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// skybox vertices

	float skyboxVertices[] = {
		// positions          
		-1.0f,  1.0f, -1.0f,
		-1.0f, -1.0f, -1.0f,
		 1.0f, -1.0f, -1.0f,
		 1.0f, -1.0f, -1.0f,
		 1.0f,  1.0f, -1.0f,
		-1.0f,  1.0f, -1.0f,

		-1.0f, -1.0f,  1.0f,
		-1.0f, -1.0f, -1.0f,
		-1.0f,  1.0f, -1.0f,
		-1.0f,  1.0f, -1.0f,
		-1.0f,  1.0f,  1.0f,
		-1.0f, -1.0f,  1.0f,

		 1.0f, -1.0f, -1.0f,
		 1.0f, -1.0f,  1.0f,
		 1.0f,  1.0f,  1.0f,
		 1.0f,  1.0f,  1.0f,
		 1.0f,  1.0f, -1.0f,
		 1.0f, -1.0f, -1.0f,

		-1.0f, -1.0f,  1.0f,
		-1.0f,  1.0f,  1.0f,
		 1.0f,  1.0f,  1.0f,
		 1.0f,  1.0f,  1.0f,
		 1.0f, -1.0f,  1.0f,
		-1.0f, -1.0f,  1.0f,

		-1.0f,  1.0f, -1.0f,
		 1.0f,  1.0f, -1.0f,
		 1.0f,  1.0f,  1.0f,
		 1.0f,  1.0f,  1.0f,
		-1.0f,  1.0f,  1.0f,
		-1.0f,  1.0f, -1.0f,

		-1.0f, -1.0f, -1.0f,
		-1.0f, -1.0f,  1.0f,
		 1.0f, -1.0f, -1.0f,
		 1.0f, -1.0f, -1.0f,
		-1.0f, -1.0f,  1.0f,
		 1.0f, -1.0f,  1.0f
	};


	// skybox VAO
	unsigned int skyboxVAO, skyboxVBO;
	glGenVertexArrays(1, &skyboxVAO);
	glGenBuffers(1, &skyboxVBO);
	glBindVertexArray(skyboxVAO);
	glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

	// load textures
	// -------------
	std::vector<std::string> faces
	{
		currentPath + "\\Textures\\SkyBox\\right.jpg",
		currentPath + "\\Textures\\SkyBox\\left.jpg",
		currentPath + "\\Textures\\SkyBox\\top.jpg",
		currentPath + "\\Textures\\SkyBox\\bottom.jpg",
		currentPath + "\\Textures\\SkyBox\\front.jpg",
		currentPath + "\\Textures\\SkyBox\\back.jpg"
	};
	unsigned int cubemapTexture = loadCubemap(faces);



	// shader configuration
	// --------------------
	shadowMappingShader.use();
	shadowMappingShader.setInt("diffuseTexture", 0);
	shadowMappingShader.setInt("shadowMap", 1);

	cubeMapsShader.use();
	cubeMapsShader.setInt("skybox", 0);

	skyBoxShader.use();
	skyBoxShader.setInt("skybox", 0);
	// lighting info
	// -------------

	glEnable(GL_CULL_FACE);

	// render loop
	// -----------
	while (!glfwWindowShouldClose(window))
	{
		// per-frame time logic
		// --------------------
		float currentFrame = (float)glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		// input
		// -----
		processInput(window, faces, cubemapTexture);
		if (rotatingLight)
			lightPos = glm::vec3(2.0f * sin(glfwGetTime()), 15.0f, 2.0f * cos(glfwGetTime()));
		// render
		// ------
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// 1. render depth of scene to texture (from light's perspective)
		glm::mat4 lightProjection, lightView;
		glm::mat4 lightSpaceMatrix;
		float near_plane = 1.0f, far_plane = 7.5f;
		lightProjection = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, near_plane, far_plane);
		lightView = glm::lookAt(lightPos, glm::vec3(0.0f), glm::vec3(0.0, 1.0, 0.0));
		lightSpaceMatrix = lightProjection * lightView;

		// render scene from light's point of view
		shadowMappingDepthShader.use();
		shadowMappingDepthShader.setMat4("lightSpaceMatrix", lightSpaceMatrix);

		glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
		glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
		glClear(GL_DEPTH_BUFFER_BIT);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, cubeTexture);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, tableTexture);
		glEnable(GL_CULL_FACE);
		glCullFace(GL_FRONT);
		renderScene(shadowMappingDepthShader, tableTexture, cubeTexture);
		glCullFace(GL_BACK);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		// reset viewport
		glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// 2. render scene as normal using the generated depth/shadow map 
		glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		shadowMappingShader.use();
		glm::mat4 projection = pCamera->GetProjectionMatrix();
		glm::mat4 view = pCamera->GetViewMatrix();
		shadowMappingShader.setMat4("projection", projection);
		shadowMappingShader.setMat4("view", view);
		// set light uniforms
		shadowMappingShader.SetVec3("viewPos", pCamera->GetPosition());
		shadowMappingShader.SetVec3("lightPos", lightPos);
		shadowMappingShader.setMat4("lightSpaceMatrix", lightSpaceMatrix);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, cubeTexture);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, depthMap);
		glDisable(GL_CULL_FACE);
		renderScene(shadowMappingShader, tableTexture, cubeTexture);

		fishShader.use();
		fish2Shader.use();
		aquariumShader.use();
		fish3Shader.use();
		fish4Shader.use();
		rockShader.use();

		fishShader.setMat4("projection", projection);
		fishShader.setMat4("view", view);
		fish2Shader.setMat4("projection", projection);
		fish2Shader.setMat4("view", view);
		aquariumShader.setMat4("projection", projection);
		aquariumShader.setMat4("view", view);
		fish3Shader.setMat4("projection", projection);
		fish3Shader.setMat4("view", view);
		fish4Shader.setMat4("projection", projection);
		fish4Shader.setMat4("view", view);
		rockShader.setMat4("projection", projection);
		rockShader.setMat4("view", view);

		//yellow fish
		auto modelMatrix = glm::mat4(1.0f);
		modelMatrix = glm::translate(modelMatrix, glm::vec3(25.0f, -3.0f, 0.0f));
		modelMatrix = glm::scale(modelMatrix, glm::vec3(0.01f));
		fishShader.setMat4("model", modelMatrix);
		fish.Draw(fishShader);

		//blue fish
		auto fish2Matrix = glm::mat4(1.0f);
		fish2Matrix = glm::translate(fish2Matrix, glm::vec3(30.0f, -3.0f, 0.0f));
		fish2Matrix = glm::rotate(fish2Matrix, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		fish2Matrix = glm::scale(fish2Matrix, glm::vec3(0.5f));
		fish2Shader.setMat4("model", fish2Matrix);
		fish2.Draw(fish2Shader);

		auto aquariumMatrix = glm::mat4(1.0f);
		aquariumMatrix = glm::translate(aquariumMatrix, glm::vec3(0.0f, 0.0f, -10.0f));
		aquariumShader.setMat4("model", aquariumMatrix);
		aquarium.Draw(aquariumShader);

		//clown fish
		auto fish3Matrix = glm::mat4(1.0f);
		fish3Matrix = glm::translate(fish3Matrix, glm::vec3(20.0f, -3.0f, 0.0f));
		fish3Shader.setMat4("model", fish3Matrix);
		fish3.Draw(fish3Shader);

		//green-yellow fish
		auto fish4Matrix = glm::mat4(1.0f);
		fish4Matrix = glm::translate(fish4Matrix, glm::vec3(35.0f, -3.0f, 0.0f));
		fish4Matrix=glm::rotate(fish4Matrix, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		fish4Matrix=glm::rotate(fish4Matrix, glm::radians(-90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		fish4Matrix=glm::scale(fish4Matrix, glm::vec3(0.5f));
		fish4Shader.setMat4("model", fish4Matrix);
		fish4.Draw(fish4Shader);

		//rock - can be multiplied for decoration
		auto rockMatrix = glm::mat4(1.0f);
		rockMatrix = glm::translate(rockMatrix, glm::vec3(50.0f, -3.0f, 0.0f));
		rockShader.setMat4("model", rockMatrix);
		rock.Draw(rockShader);


		// draw skybox as last
		glDepthFunc(GL_LEQUAL);  // change depth function so depth test passes when values are equal to depth buffer's content
		skyBoxShader.use();
		view = glm::mat4(glm::mat3(pCamera->GetViewMatrix())); // remove translation from the view matrix
		skyBoxShader.setMat4("view", view);
		skyBoxShader.setMat4("projection", projection);
		// skybox cube
		glBindVertexArray(skyboxVAO);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
		glDrawArrays(GL_TRIANGLES, 0, 36);
		glBindVertexArray(0);
		glDepthFunc(GL_LESS); // set depth function back to default




		// glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	// optional: de-allocate all resources once they've outlived their purpose:
	delete pCamera;

	glfwTerminate();
	return 0;
}

// renders the 3D scene
// --------------------
void renderScene(const Shader& shader, unsigned int tableTexture, unsigned int cubeTexture)
{
	shader.setInt("diffuseTexture", 0); // Assuming 1 for tableTexture (GL_TEXTURE1)
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, tableTexture);

	// floor
	glm::mat4 model;
	shader.setMat4("model", model);
	renderFloor();

	shader.setInt("diffuseTexture", 1); // Assuming 1 for tableTexture (GL_TEXTURE1)
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, cubeTexture);

	// cube
	model = glm::mat4();
	model = glm::scale(model, glm::vec3(6.0f));

	shader.setMat4("model", model);
	renderCube();


}

unsigned int planeVAO = 0;
void renderFloor()
{
	unsigned int planeVBO;

	if (planeVAO == 0) {
		// set up vertex data (and buffer(s)) and configure vertex attributes
		float planeVertices[] = {
			// positions            // normals         // texcoords
			25.0f, -6.f,  25.0f,  0.0f, 1.0f, 0.0f,  25.0f,  0.0f,
			-25.0f, -6.f,  25.0f,  0.0f, 1.0f, 0.0f,   0.0f,  0.0f,
			-25.0f, -6.f, -25.0f,  0.0f, 1.0f, 0.0f,   0.0f, 25.0f,

			25.0f, -6.f,  25.0f,  0.0f, 1.0f, 0.0f,  25.0f,  0.0f,
			-25.0f, -6.f, -25.0f,  0.0f, 1.0f, 0.0f,   0.0f, 25.0f,
			25.0f, -6.f, -25.0f,  0.0f, 1.0f, 0.0f,  25.0f, 25.0f
		};
		// plane VAO
		glGenVertexArrays(1, &planeVAO);
		glGenBuffers(1, &planeVBO);
		glBindVertexArray(planeVAO);
		glBindBuffer(GL_ARRAY_BUFFER, planeVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(planeVertices), planeVertices, GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
		glBindVertexArray(0);
	}

	glBindVertexArray(planeVAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);
}


// renderCube() renders a 1x1 3D cube in NDC.
// -------------------------------------------------
unsigned int cubeVAO = 0;
unsigned int cubeVBO = 0;
void renderCube()
{
	// initialize (if necessary)
	if (cubeVAO == 0)
	{
		float vertices[] = {
			// back face
			-1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
			1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
			1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 0.0f, // bottom-right         
			1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
			-1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
			-1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f, // top-left
			// front face
			-1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
			1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 0.0f, // bottom-right
			1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
			1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
			-1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f, // top-left
			-1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
			// left face
			-1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
			-1.0f,  1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-left
			-1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
			-1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
			-1.0f, -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-right
			-1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
			// right face
			1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
			1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
			1.0f,  1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-right         
			1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
			1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
			1.0f, -1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-left     

			// top face
			-1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
			1.0f,  1.0f , 1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
			1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 1.0f, // top-right     
			1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
			-1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
			-1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f  // bottom-left        
		};
		glGenVertexArrays(1, &cubeVAO);
		glGenBuffers(1, &cubeVBO);
		// fill buffer
		glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
		// link vertex attributes
		glBindVertexArray(cubeVAO);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);
	}
	// render Cube
	glBindVertexArray(cubeVAO);
	glDrawArrays(GL_TRIANGLES, 0, 30);
	glBindVertexArray(0);
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
void processInput(GLFWwindow* window, std::vector<std::string>& faces, unsigned int& cubemapTexture)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		pCamera->ProcessKeyboard(ECameraMovementType::FORWARD, (float)deltaTime);
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		pCamera->ProcessKeyboard(ECameraMovementType::BACKWARD, (float)deltaTime);
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		pCamera->ProcessKeyboard(ECameraMovementType::LEFT, (float)deltaTime);
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		pCamera->ProcessKeyboard(ECameraMovementType::RIGHT, (float)deltaTime);
	if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
		pCamera->ProcessKeyboard(ECameraMovementType::UP, (float)deltaTime);
	if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
		pCamera->ProcessKeyboard(ECameraMovementType::DOWN, (float)deltaTime);

	//night and day
	if (glfwGetKey(window, GLFW_KEY_M) == GLFW_PRESS && !isDay)
	{
		isDay = true;
		setFaces(faces, cubemapTexture);
	}
	if (glfwGetKey(window, GLFW_KEY_N) == GLFW_PRESS && isDay)
	{
		isDay = false;
		setFaces(faces, cubemapTexture);
	}


	if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
		int width, height;
		glfwGetWindowSize(window, &width, &height);
		pCamera->Reset(width, height);

	}
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	// make sure the viewport matches the new window dimensions; note that width and 
	// height will be significantly larger than specified on retina displays.
	pCamera->Reshape(width, height);
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
	pCamera->MouseControl((float)xpos, (float)ypos);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yOffset)
{
	pCamera->ProcessMouseScroll((float)yOffset);
}


// loads a cubemap texture from 6 individual texture faces
// order:
// +X (right)
// -X (left)
// +Y (top)
// -Y (bottom)
// +Z (front) 
// -Z (back)
// -------------------------------------------------------
unsigned int loadCubemap(const std::vector<std::string>& faces)
{
	stbi_set_flip_vertically_on_load(false); // tell stb_image.h to flip loaded texture's on the y-axis.
	unsigned int textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

	int width, height, nrComponents;
	for (unsigned int i = 0; i < faces.size(); i++)
	{
		unsigned char* data = stbi_load(faces[i].c_str(), &width, &height, &nrComponents, 0);
		if (data)
		{
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
			stbi_image_free(data);
		}
		else
		{
			std::cout << "Cubemap texture failed to load at path: " << faces[i] << std::endl;
			stbi_image_free(data);
		}
	}
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	return textureID;
}

void setFaces(std::vector<std::string>& faces, unsigned int& cubemapTexture)
{
	if (isDay)
	{
		faces =
		{
			currentPath + "\\Textures\\SkyBox\\right.jpg",
			currentPath + "\\Textures\\SkyBox\\left.jpg",
			currentPath + "\\Textures\\SkyBox\\top.jpg",
			currentPath + "\\Textures\\SkyBox\\bottom.jpg",
			currentPath + "\\Textures\\SkyBox\\front.jpg",
			currentPath + "\\Textures\\SkyBox\\back.jpg",
		};
	}
	else
	{
		faces =
		{
			currentPath + "\\Textures\\SkyBox\\right_night.jpg",
			currentPath + "\\Textures\\SkyBox\\left_night.jpg",
			currentPath + "\\Textures\\SkyBox\\top_night.jpg",
			currentPath + "\\Textures\\SkyBox\\bottom_night.jpg",
			currentPath + "\\Textures\\SkyBox\\front_night.jpg",
			currentPath + "\\Textures\\SkyBox\\back_night.jpg"
		};
	}
	cubemapTexture = loadCubemap(faces);
}