#define STB_IMAGE_IMPLEMENTATION
#define STBI_HEADER_FILE_ONLY
#define _CRT_SECURE_NO_WARNINGS

#include <Windows.h>
#include <locale>
#include <codecvt>
#include <vector>
#include <corecrt_math_defines.h>

#include <irrKlang.h>
#include "stb_image.h"
#include <minwindef.h>

#include "Camera.h"
#include "Model.h"
#include "TextRenderer.h"
#include <random>

enum class WindowType {
    NONE,
    SQUARE,
    RECTANGULAR,
    CEILING,
    BUBBLE
};


// settings
const unsigned int SCR_WIDTH = 1920;
const unsigned int SCR_HEIGHT = 1080;

template<class T>
T random(T min, T max)
{
    static std::mt19937 gen{ std::random_device{}() };
    using dist = std::conditional_t<
        std::is_integral<T>::value,
        std::uniform_int_distribution<T>,
        std::uniform_real_distribution<T>>;
    return dist{ min, max }(gen);
}

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


void setFaces(std::vector<std::string>& faces, unsigned int& cubemapTexture);
unsigned int loadCubemap(const std::vector<std::string>& faces);
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

int main(int argc, char** argv);

void renderMenu();
void renderScene(Shader& shader);
void renderCube();
void renderFloor();
void renderSphere();
void renderSquare();
void renderRectangular();
void renderCeiling();
void freeMemory();

// program variables
std::string currentPath = GetCurrentPath();
double deltaTime = 0.0f;	// time between current frame and last frame
double lastFrame = 0.0f;
float factorOfDarkness = 90 / 100.0;
bool rotatingLight = false;
bool isDay = true;
bool menuShouldBeShown = false;
bool hideMenuCompletely = false;
bool objectsShouldMove = true;
glm::vec3 lightPos(0.0f, 2.0f, 2.5f);

irrklang::ISoundEngine* SoundEngine = irrklang::createIrrKlangDevice();
TextRenderer* Text;
Model* fishObjModel;
Model* fishObjModel2;
Model* fishObjModel3;
Model* fishObjModel4;
//bank of yellow fishes
Model* fishObjModel5;
Model* fishObjModel6;
Model* fishObjModel7;
Model* fishObjModel8;
//
Model* fishObjModel9;
Model* fishObjModel10;
Model* fishMan;
Model* rock;
Model* seaObjects;
Model* krab;
Model* statue;
Model* wall;
Model* greek;
Camera* camera;
std::vector<glm::vec3> bubbles;

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
	camera = new Camera(SCR_WIDTH, SCR_HEIGHT, glm::vec3(0.0, 1.0, 10.0));

    // Initialize textRenderer and load font
    Text = new TextRenderer(SCR_WIDTH, SCR_HEIGHT, currentPath + "\\Shaders");
    Text->Load(currentPath + "\\Fonts\\" + "OCRAEXT.TTF", 24);

	// configure global opengl state
	// -----------------------------
    glEnable(GL_DEPTH_TEST);

	// build and compile shaders
	// -------------------------
	Shader skyBoxShader((currentPath + "\\Shaders" + "\\SkyBox.vs").c_str(), (currentPath + "\\Shaders" + "\\SkyBox.fs").c_str());
	Shader usedShader((currentPath + "\\Shaders" + "\\Model.vs").c_str(), (currentPath + "\\Shaders" + "\\Model.fs").c_str());
	Shader shadowMappingShader((currentPath + "\\Shaders" + "\\ShadowMapping.vs").c_str(), (currentPath + "\\Shaders" + "\\ShadowMapping.fs").c_str());
	Shader shadowMappingDepthShader((currentPath + "\\Shaders" + "\\ShadowMappingDepth.vs").c_str(), (currentPath + "\\Shaders" + "\\ShadowMappingDepth.fs").c_str());
	Shader windowShader((currentPath + "\\Shaders" + "\\Blending.vs").c_str(), (currentPath + "\\Shaders" + "\\Blending.fs").c_str());

	// load textures
	// -------------
	unsigned int aquariumFloorTexture = CreateTexture(currentPath + "\\Textures\\CubeTexture.jpg");
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


	// shader configuration
	// --------------------

    windowShader.use();
    windowShader.setInt("texture1", 0);

	skyBoxShader.use();
	skyBoxShader.setInt("skybox", 0);

	shadowMappingShader.use();
	shadowMappingShader.setInt("diffuseTexture", 0);
	shadowMappingShader.setInt("shadowMap", 1);

    usedShader.setInt("texture_diffuse1", 0);

    // prepare 3d models for aquarium
    // --------------

    const float aquariumLength = 20.0f;
    const float aquariumHeight = 5.0f;
    const float aquariumWidth = 6.0f;

    std::vector<std::pair<glm::vec3, WindowType>> transparentObjects;
    //front and back walls
    for (float i = 0.0f; i < aquariumLength; i += 1.0f) {
        for (float j = 0.0f; j < aquariumHeight; j += 1.0f) {
            transparentObjects.emplace_back(std::make_pair(glm::vec3(i, j, -0.0001f),WindowType::RECTANGULAR));
            transparentObjects.emplace_back(std::make_pair(glm::vec3(i, j, aquariumWidth), WindowType::RECTANGULAR));
        }
    }
    //left and right walls
    for (float i = 0.0f; i < aquariumWidth; i += 1.0f) {
        for (float j = 0.0f; j < aquariumHeight; j += 1.0f) {
            transparentObjects.emplace_back(std::make_pair(glm::vec3(-0.0001f, j, i), WindowType::SQUARE));
            transparentObjects.emplace_back(std::make_pair(glm::vec3(aquariumLength, j, i), WindowType::SQUARE));
        }
    }
    //ceiling
    for (float i = 0.0f; i < aquariumLength; i += 1.0f) {
        for (float j = 0.0f; j < aquariumWidth; j += 1.0f) {
            transparentObjects.emplace_back(std::make_pair(glm::vec3(i, aquariumHeight - 1.5f, j), WindowType::CEILING));
        }
    }

    // load models
    // -----------
    
	//yellow fish
    fishObjModel = new Model{ currentPath + "\\Models\\Fish06\\Fish06.obj", false };
    //fishObjModel->setPos(glm::vec3(2.0f, 1.0f, 3.3f), glm::vec3(12.0f, 2.0f, 2.0f), 180.0f);
    fishObjModel->setPosSplice(glm::vec3(2.0f, 1.0f, 3.0f), glm::vec3(10.5f, 1.0f, 4.5f), glm::vec3(15.0f, 3.0f, 2.0f), 0.0f);

    fishObjModel2 = new Model{ currentPath + "\\Models\\Fish02\\Fish02.obj", false };
    fishObjModel2->setPos(glm::vec3(18.0f, 2.0f, 4.0f), glm::vec3(2.0f, 1.0f, 5.3f), 0.0f);

    fishObjModel3 = new Model{ currentPath + "\\Models\\Fish04\\TropicalFish09.obj", false };
    fishObjModel3->setPos(glm::vec3(18.0f, 3.0f, 3.0f), glm::vec3(3.0f, 3.0f, 3.0f), 0.0f);

    fishObjModel4 = new Model{ currentPath + "\\Models\\Fish03\\TropicalFish10.obj", false };
    fishObjModel4->setPos(glm::vec3(11.0f, 0.5f, 5.0f), glm::vec3(11.0f, 0.5f, 1.0f), 180.0f);

    //bank
    fishObjModel5 = new Model{ currentPath + "\\Models\\Fish05\\TropicalFish05.obj", false };
    fishObjModel5->setPos(glm::vec3(15.0f, 0.5f, 1.0f), glm::vec3(19.0f, 0.5f, 1.0f), 180.0f);

    fishObjModel6 = new Model{ currentPath + "\\Models\\Fish05\\TropicalFish05.obj", false };
    fishObjModel6->setPos(glm::vec3(15.0f, 0.4f, 1.3f), glm::vec3(19.0f, 0.4f, 1.3f), 180.0f);

    fishObjModel7 = new Model{ currentPath + "\\Models\\Fish05\\TropicalFish05.obj", false };
    fishObjModel7->setPos(glm::vec3(15.0f, 0.7f, 0.7f), glm::vec3(19.0f, 0.6f, 0.7f), 180.0f);

    fishObjModel8 = new Model{ currentPath + "\\Models\\Fish05\\TropicalFish05.obj", false };
    fishObjModel8->setPos(glm::vec3(15.0f, 0.6f, 1.5f), glm::vec3(19.0f, 0.5f, 1.5f), 180.0f);

    //

    fishObjModel9 = new Model{ currentPath + "\\Models\\Fish01\\TropicalFish08.obj", false };
    fishObjModel9->setPos(glm::vec3(1.75f, 3.f, 1.5f), glm::vec3(1.75f, 3.f, 4.5f), 0.0f);

    fishObjModel10 = new Model{ currentPath + "\\Models\\Fish07\\TropicalFish01.obj", false };
    fishObjModel10->setPos(glm::vec3(19.75f, 0.5f, 5.f), glm::vec3(15.75f, 0.5f, 3.5f), 0.0f);

    fishMan = new Model{ currentPath + "\\Models\\AquaMan\\13018_Aquarium_Deep_Sea_Diver_v1_L1.obj", false };
    fishMan->setPos(glm::vec3(19.25f, 0.f, 4.f), glm::vec3(2.0f, 0.f, 3.0f), 0.0f);

    rock=new Model{ currentPath + "\\Models\\Rock1\\Rock1.obj", false };
    rock->setPos(glm::vec3(16.0f, -0.2f, 4.f), glm::vec3(17.0f, -0.2f, 3.0f), 0.0f);

    seaObjects = new Model{ currentPath + "\\Models\\SeaObjects\\model.obj", false };
    seaObjects->setPos(glm::vec3(11.f, 0.f, 3.0f), glm::vec3(10.0f, 0.f, 3.0f), 0.0f);

    krab = new Model{ currentPath + "\\Models\\Krab\\model.obj", false };
    krab->setPos(glm::vec3(0.5f, 0.f, 5.5f), glm::vec3(19.5f, 0.f, 5.5f), 180.0f);

    statue = new Model{ currentPath + "\\Models\\Ship\\model.obj", false };
    statue->setPos(glm::vec3(17.5f, 0.f, 2.2f), glm::vec3(19.5f, 0.f, 5.5f), 0.0f);

    wall = new Model{ currentPath + "\\Models\\Wall\\model.obj", false };
    wall->setPos(glm::vec3(1.75f, 0.f, 3.0f), glm::vec3(0.0f, 0.f, 0.0f), 0.0f);

    greek = new Model{ currentPath + "\\Models\\Statue\\model.obj", false };
    greek->setPos(glm::vec3(3.7f, 0.f, 2.2f), glm::vec3(0.0f, 0.f, 0.0f), 0.0f);

    // Play background sound
    SoundEngine->play2D((currentPath + "\\Sounds\\" + "background.mp3").c_str(), true);

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

        // reset background color
        // ------
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // randomly generate new bubble
        // ------
        if (random(0.f, 1.f) < 0.2f && objectsShouldMove)
            bubbles.emplace_back(glm::vec3(random(0.f, 20.f), 0.01f, random(0.f, 6.f)));

        // sort transparent objects
        // ------
        std::multimap<float, std::pair<glm::vec3, WindowType>> sortedMap;
        for (int i = 0; i < transparentObjects.size(); i++) {
            float distance = glm::length(camera->GetPosition() - transparentObjects[i].first);
            sortedMap.emplace(std::make_pair(distance, (transparentObjects[i])));
        }
        for (int i = bubbles.size() - 1; i >= 0; --i) {
            float distance = glm::length(camera->GetPosition() - bubbles[i]);
            sortedMap.emplace(std::make_pair(distance, std::make_pair(bubbles[i], WindowType::BUBBLE)));
            if (objectsShouldMove)
            {
                //increase y position of bubble
                bubbles[i].y += 0.04f;
                // delete bubble if above aquarium
                if (bubbles[i].y > 4.f) {
                    bubbles.erase(bubbles.begin() + i);
                }
            }
        }

        // light movement
        // -----------
        /*if (isDay)
        {
            static float fRadius = 15.0f;
            lightPos.x = fRadius * std::cos(currentFrame);
            lightPos.y = 3.0f;
        }
        else
        {
            lightPos.y = -1000.0f;
        }*/

        // render
        // ------
        // 1. render depth of scene to texture (from light's perspective)
        glm::mat4 projection = camera->GetProjectionMatrix();
        glm::mat4 view = camera->GetViewMatrix();
        if (isDay)
        {
            glm::mat4 lightProjection, lightView;
            glm::mat4 lightSpaceMatrix;
            float near_plane = 1.0f, far_plane = 30.5f;
            lightProjection = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, near_plane, far_plane);

            lightView = glm::lookAt(lightPos, glm::vec3(12.5f, 0.f, 3.0f), glm::vec3(0.0, 1.0, 0.0));
            lightSpaceMatrix = lightProjection * lightView;

            // render scene from light's point of view
            shadowMappingDepthShader.use();
            shadowMappingDepthShader.setMat4("lightSpaceMatrix", lightSpaceMatrix);

            glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
            glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
            glClear(GL_DEPTH_BUFFER_BIT);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, aquariumFloorTexture);
            glEnable(GL_CULL_FACE);
            glCullFace(GL_FRONT);
            renderScene(shadowMappingDepthShader);
            glCullFace(GL_BACK);
            glBindFramebuffer(GL_FRAMEBUFFER, 0);

            // reset viewport
            glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            // render the scene
            // 2. render scene as normal using the generated depth/shadow map 

            shadowMappingShader.use();
            shadowMappingShader.setMat4("projection", projection);
            shadowMappingShader.setMat4("view", view);
            // set light uniforms
            shadowMappingShader.SetVec3("viewPos", camera->GetPosition());
            shadowMappingShader.SetVec3("lightPos", lightPos);
            shadowMappingShader.setMat4("lightSpaceMatrix", lightSpaceMatrix);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, aquariumFloorTexture);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, depthMap);
            glDisable(GL_CULL_FACE);
            renderScene(shadowMappingShader);
        }
        else
        {
            usedShader.use(); 
            usedShader.setMat4("view", view);
            usedShader.setMat4("projection", projection); 
            usedShader.setFloat("factor", factorOfDarkness);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, aquariumFloorTexture);
            renderScene(usedShader);
        }
        
        // draw skybox as last
        glDepthFunc(GL_LEQUAL);  // change depth function so depth test passes when values are equal to depth buffer's content
        skyBoxShader.use();
        view = glm::mat4(glm::mat3(camera->GetViewMatrix())); // remove translation from the view matrix
        skyBoxShader.setMat4("view", view);
        skyBoxShader.setMat4("projection", projection);
        // skybox cube
        glBindVertexArray(skyboxVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
        glDepthFunc(GL_LESS); // set depth function back to default
    

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        windowShader.use();
        projection = camera->GetProjectionMatrix();
        view = camera->GetViewMatrix();
        windowShader.setMat4("projection", projection);
        windowShader.setMat4("view", view);

        //render light source as a sphere
        glm::mat4 model(1.f);
        model = glm::translate(model, lightPos);
        model = glm::scale(model, glm::vec3(0.03f));
        windowShader.setMat4("model", model);
        windowShader.SetVec4("color", glm::vec4(1, 1, 1, 1));
        renderSphere();

        //furthest object is drawn first
        for (auto it = sortedMap.rbegin(); it != sortedMap.rend(); ++it)
        {
                glm::mat4 model = glm::mat4(1.0f);
                glm::vec4 color(0.133f, 0.424f, 1.0f, 0.2f); //default aquarium color
                model = glm::translate(model, it->second.first);
                //bootom layer = brown
                if (it->second.first.y== 0)
                    color = glm::vec4(0.078f, 0.031f, 0.008f, 1.0f);
                //top layer light gray
                if (it->second.first.y == aquariumHeight - 1.0f)
                    color = glm::vec4(0.473, 0.543, 0.629, 0.15f);
                //bubbles
                if (it->second.second == WindowType::BUBBLE)
                {
                    model = glm::scale(model, glm::vec3(0.03f));
                    color = glm::vec4(0.773, 0.843, 0.929, 0.2f);
                }
                if (!isDay)
                {
                    color.x = color.x - 0.5f;
                    color.y = color.y - 0.5f;
                    color.z = color.z - 0.85f;
                }
                windowShader.setMat4("model", model);
                windowShader.SetVec4("color", color);
                switch (it->second.second) 
                {
                    case WindowType::SQUARE:
                        renderSquare();
                        break;
                    case WindowType::RECTANGULAR:
                        renderRectangular();
                        break;
                    case WindowType::CEILING:
                        renderCeiling();
                        break;
                    case WindowType::BUBBLE:
                        renderSphere();
                        break;
                    default:
                        throw std::exception{ "no window type provided" };
                        break;
                }
        }
        glm::vec4 color = glm::vec4(0.078f, 0.031f, 0.008f, 1.0f);
        if (!isDay)
        {
            color.x = color.x - 0.5f;
            color.y = color.y - 0.5f;
            color.z = color.z - 0.9f;
        }
        windowShader.setMat4("model", glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -0.5f, 0.0f)));
        windowShader.SetVec4("color", color);
        renderFloor();

        renderMenu();
        glDisable(GL_BLEND);

		// glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	// optional: de-allocate all resources once they've outlived their purpose:
    freeMemory();
	glfwTerminate();
	return 0;
}

// renders the menu
// --------------------
void renderMenu()
{
    if (hideMenuCompletely)
        return;
    glDisable(GL_DEPTH_TEST);

    static std::vector<const char*> messages{ "MENU", "Press 1 to show menu", "Press 2 to hide menu", "Press 3 to reset camera position", 
        "Press 4 for day", "Press 5 for night","Press 6 to disable movement", "Press 7 to enable movement" };
    if (menuShouldBeShown)
    {
        Text->RenderText(messages[0], 25.0f, SCR_HEIGHT - 50.0f, 0.7f, glm::vec3(1.0f, 1.0f, 1.0f), SCR_WIDTH, SCR_HEIGHT);
        Text->RenderText(messages[2], 25.0f, SCR_HEIGHT - 65.0f, 0.5f, glm::vec3(1.0f, 1.0f, 1.0f), SCR_WIDTH, SCR_HEIGHT);
        Text->RenderText(messages[3], 25.0f, SCR_HEIGHT - 75.0f, 0.5f, glm::vec3(1.0f, 1.0f, 1.0f), SCR_WIDTH, SCR_HEIGHT);
        Text->RenderText(messages[4], 25.0f, SCR_HEIGHT - 85.0f, 0.5f, glm::vec3(1.0f, 1.0f, 1.0f), SCR_WIDTH, SCR_HEIGHT);
        Text->RenderText(messages[5], 25.0f, SCR_HEIGHT - 95.0f, 0.5f, glm::vec3(1.0f, 1.0f, 1.0f), SCR_WIDTH, SCR_HEIGHT);
        Text->RenderText(messages[6], 25.0f, SCR_HEIGHT - 105.0f, 0.5f, glm::vec3(1.0f, 1.0f, 1.0f), SCR_WIDTH, SCR_HEIGHT);
        Text->RenderText(messages[7], 25.0f, SCR_HEIGHT - 115.0f, 0.5f, glm::vec3(1.0f, 1.0f, 1.0f), SCR_WIDTH, SCR_HEIGHT);
    }
    else 
    {
        Text->RenderText(messages[1], 25.0f, SCR_HEIGHT - 50.0f, 0.7f, glm::vec3(1.0f, 1.0f, 1.0f), SCR_WIDTH, SCR_HEIGHT);
    }
    glEnable(GL_DEPTH_TEST);
}

float incrementMoveSpeed = 0.004;
float incrementRotationSpeed = 0.4;
// renders the 3D scene
// --------------------
void renderScene(Shader& shader)
{
    glm::mat4 model{ glm::mat4(1.0f) };
    model = glm::translate(model, glm::vec3(0.0f));
    shader.setMat4("model", model);
    renderFloor();

    model = glm::mat4(1.0f);
    if(objectsShouldMove)
        fishObjModel->moveObjectSplice(incrementMoveSpeed, incrementRotationSpeed);
    model = glm::translate(glm::mat4(1.0f), fishObjModel->currentPos);
    //std::cout << fishObjModel->currentPos.x << " " << fishObjModel->currentPos.y << " " << fishObjModel->currentPos.z << std::endl;
    model = glm::scale(model, glm::vec3(0.03f));
    model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    model = glm::rotate(model, glm::radians(180.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    model = glm::rotate(model, glm::radians(fishObjModel->rotation), glm::vec3(0.0f, 0.0f, 1.0f));
    shader.setMat4("model", model);
    fishObjModel->Draw(shader);

    model = glm::mat4(1.0f);
    if (objectsShouldMove)
        fishObjModel2->moveObjectLinear(incrementMoveSpeed, incrementRotationSpeed);
    model = glm::translate(glm::mat4(1.0f), fishObjModel2->currentPos);
    model = glm::scale(model, glm::vec3(0.1f));
    model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::rotate(model, glm::radians(fishObjModel2->rotation), glm::vec3(0.0f, 1.0f, 0.0f));
    shader.setMat4("model", model);
    fishObjModel2->Draw(shader);

    model = glm::mat4(1.0f);
    if (objectsShouldMove)
        fishObjModel3->moveObjectLinear(incrementMoveSpeed, incrementRotationSpeed);
    model = glm::translate(glm::mat4(1.0f), fishObjModel3->currentPos);
    model = glm::scale(model, glm::vec3(0.001f));
    model = glm::rotate(model, glm::radians(270.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::rotate(model, glm::radians(fishObjModel3->rotation), glm::vec3(0.0f, 1.0f, 0.0f));
    shader.setMat4("model", model);
    fishObjModel3->Draw(shader);

    model = glm::mat4(1.0f);
    if (objectsShouldMove)
        fishObjModel4->moveObjectLinear(incrementMoveSpeed, incrementRotationSpeed);
    model = glm::translate(glm::mat4(1.0f), fishObjModel4->currentPos);
    model = glm::scale(model, glm::vec3(0.0009f));
    model = glm::rotate(model, glm::radians(180.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    model = glm::rotate(model, glm::radians(fishObjModel4->rotation), glm::vec3(0.0f, 1.0f, 0.0f));
    shader.setMat4("model", model);
    fishObjModel4->Draw(shader);

    //bank of yellow fishes
    model = glm::mat4(1.0f);
    if (objectsShouldMove)
        fishObjModel5->moveObjectLinear(incrementMoveSpeed, incrementRotationSpeed);
    model = glm::translate(glm::mat4(1.0f), fishObjModel5->currentPos);
    model = glm::scale(model, glm::vec3(0.0009f));
    //model = glm::rotate(model, glm::radians(180.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::rotate(model, glm::radians(fishObjModel5->rotation), glm::vec3(0.0f, 1.0f, 0.0f));
    shader.setMat4("model", model);
    fishObjModel5->Draw(shader);

    model = glm::mat4(1.0f);
    if (objectsShouldMove)
        fishObjModel6->moveObjectLinear(incrementMoveSpeed, incrementRotationSpeed);
    model = glm::translate(glm::mat4(1.0f), fishObjModel6->currentPos);
    model = glm::scale(model, glm::vec3(0.0009f));
    //model = glm::rotate(model, glm::radians(180.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::rotate(model, glm::radians(fishObjModel6->rotation), glm::vec3(0.0f, 1.0f, 0.0f));
    shader.setMat4("model", model);
    fishObjModel6->Draw(shader);

    model = glm::mat4(1.0f);
    if (objectsShouldMove)
        fishObjModel7->moveObjectLinear(incrementMoveSpeed, incrementRotationSpeed);
    model = glm::translate(glm::mat4(1.0f), fishObjModel7->currentPos);
    model = glm::scale(model, glm::vec3(0.0009f));
    //model = glm::rotate(model, glm::radians(180.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::rotate(model, glm::radians(fishObjModel7->rotation), glm::vec3(0.0f, 1.0f, 0.0f));
    shader.setMat4("model", model);
    fishObjModel7->Draw(shader);

    model = glm::mat4(1.0f);
    if (objectsShouldMove)
        fishObjModel8->moveObjectLinear(incrementMoveSpeed, incrementRotationSpeed);
    model = glm::translate(glm::mat4(1.0f), fishObjModel8->currentPos);
    model = glm::scale(model, glm::vec3(0.0009f));
    //model = glm::rotate(model, glm::radians(180.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::rotate(model, glm::radians(fishObjModel8->rotation), glm::vec3(0.0f, 1.0f, 0.0f));
    shader.setMat4("model", model);
    fishObjModel8->Draw(shader);
    //

    model = glm::mat4(1.0f);
    if (objectsShouldMove)
        fishObjModel9->moveObjectLinear(incrementMoveSpeed, incrementRotationSpeed);
    model = glm::translate(glm::mat4(1.0f), fishObjModel9->currentPos);
    model = glm::scale(model, glm::vec3(0.0009f));
    model = glm::rotate(model, glm::radians(180.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    model = glm::rotate(model, glm::radians(fishObjModel9->rotation), glm::vec3(0.0f, 1.0f, 0.0f));
    shader.setMat4("model", model);
    fishObjModel9->Draw(shader);
    
    model = glm::mat4(1.0f);
    if (objectsShouldMove)
        fishObjModel10->moveObjectLinear(incrementMoveSpeed, incrementRotationSpeed);
    model = glm::translate(glm::mat4(1.0f), fishObjModel10->currentPos);
    model = glm::scale(model, glm::vec3(0.0009f));
    model = glm::rotate(model, glm::radians(63.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::rotate(model, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::rotate(model, glm::radians(fishObjModel10->rotation), glm::vec3(0.0f, 1.0f, 0.0f));
    shader.setMat4("model", model);
    fishObjModel10->Draw(shader);

    model = glm::mat4(1.0f);
    model = glm::translate(glm::mat4(1.0f), fishMan->currentPos);
    model = glm::scale(model, glm::vec3(0.15f));
    model = glm::rotate(model, glm::radians(270.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    shader.setMat4("model", model);
    fishMan->Draw(shader);

    model=glm::mat4(1.0f);
    model=glm::translate(glm::mat4(1.0f), rock->currentPos);
    model=glm::scale(model, glm::vec3(0.3f));
    model = glm::rotate(model, glm::radians(270.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    shader.setMat4("model", model);
    rock->Draw(shader);


    model = glm::mat4(1.0f);
    model = glm::translate(glm::mat4(1.0f), seaObjects->currentPos);
    model = glm::scale(model, glm::vec3(0.0065f));
    model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    // model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    shader.setMat4("model", model);
    seaObjects->Draw(shader);

    model = glm::mat4(1.0f);
    if (objectsShouldMove)
        krab->moveObjectLinear(incrementMoveSpeed-0.0037, incrementRotationSpeed);
    model = glm::translate(glm::mat4(1.0f), krab->currentPos);
    model = glm::scale(model, glm::vec3(0.006f));
    model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    model = glm::rotate(model, glm::radians(krab->rotation), glm::vec3(0.0f, 0.0f, 1.0f));
    shader.setMat4("model", model);
    krab->Draw(shader);

    model = glm::mat4(1.0f);
    model = glm::translate(glm::mat4(1.0f), statue->currentPos);
    model = glm::scale(model, glm::vec3(0.045f));
    model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    shader.setMat4("model", model);
    statue->Draw(shader);

    model = glm::mat4(1.0f);
    model = glm::translate(glm::mat4(1.0f), wall->currentPos);
    model = glm::scale(model, glm::vec3(0.002f));
    model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    //model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    shader.setMat4("model", model);
    wall->Draw(shader);

    model = glm::mat4(1.0f);
    model = glm::translate(glm::mat4(1.0f), greek->currentPos);
    model = glm::scale(model, glm::vec3(0.07f));
    //model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    //model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    shader.setMat4("model", model);
    greek->Draw(shader);
}

unsigned int planeVAO = 0;
void renderFloor()
{
    unsigned int planeVBO;

    if (planeVAO == 0) {
        // set up vertex data (and buffer(s)) and configure vertex attributes
        float planeVertices[] = {
            // positions            // normals         // texcoords
            0.0f, 0.0f,  0.0f,  0.0f, 1.0f, 0.0f,  12.0f,  0.0f,
            20.0f, 0.0f,  0.0f,  0.0f, 1.0f, 0.0f,   0.0f,  0.0f,
            20.0f, 0.0f, 6.0f,  0.0f, 1.0f, 0.0f,   0.0f, 5.0f,

            0.0f, 0.0f,  0.0f,  0.0f, 1.0f, 0.0f,  12.0f,  0.0f,
            20.0f, 0.0f, 6.0f,  0.0f, 1.0f, 0.0f,   0.0f, 5.0f,
            0.0f, 0.0f, 6.0f,  0.0f, 1.0f, 0.0f,  12.0f, 5.0f
        };
        // plane VAO
        unsigned int planeVBO;
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
    glBindVertexArray(0);
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

unsigned int transparentRectVAO = 0;
unsigned int transparentRectVBO = 0;
void renderRectangular() {
	if (transparentRectVAO == 0) {
		float transparentVertices[] = {
			// positions         // texture Coords (swapped y coordinates because texture is flipped upside down)
			0.0f,  0.5f,  0.0f,
			0.0f, -0.5f,  0.0f,
			1.0f, -0.5f,  0.0f,

			0.0f,  0.5f,  0.0f,
			1.0f, -0.5f,  0.0f,
			1.0f,  0.5f,  0.0f
		};

		glGenVertexArrays(1, &transparentRectVAO);
		glGenBuffers(1, &transparentRectVBO);
		glBindVertexArray(transparentRectVAO);
		glBindBuffer(GL_ARRAY_BUFFER, transparentRectVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(transparentVertices), transparentVertices, GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
		/*glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));*/
		glBindVertexArray(0);
	}
	//draw aquarium
	glBindVertexArray(transparentRectVAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);
}

unsigned int transparentSquareVAO = 0;
unsigned int transparentSquareRectVBO = 0;
void renderSquare() {
    if (transparentSquareVAO == 0) {
        float transparentVertices[] = {
            // positions         // texture Coords (swapped y coordinates because texture is flipped upside down)
            0.0f,  0.5f,  0.0f,
            0.0f, -0.5f,  0.0f,
            0.0f, -0.5f,  1.0f,

            0.0f,  0.5f,  0.0f,
            0.0f, -0.5f,  1.0f,
            0.0f,  0.5f,  1.0f
        };

        glGenVertexArrays(1, &transparentSquareVAO);
        glGenBuffers(1, &transparentSquareRectVBO);
        glBindVertexArray(transparentSquareVAO);
        glBindBuffer(GL_ARRAY_BUFFER, transparentSquareRectVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(transparentVertices), transparentVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        /*glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));*/
        glBindVertexArray(0);
    }
    //draw aquarium
    glBindVertexArray(transparentSquareVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

unsigned int transparentCeilingVAO = 0;
unsigned int transparentCeilingVBO = 0;
void renderCeiling() {
    if (transparentCeilingVAO == 0) {
        float transparentVertices[] = {
            // positions         // texture Coords (swapped y coordinates because texture is flipped upside down)
            0.0f, 0.0f,  0.0f,
            1.0f, 0.0f,  0.0f,
            1.0f, 0.0f, 1.0f,

            0.0f, 0.0f,  0.0f,
            1.0f, 0.0f, 1.0f,
            0.0f, 0.0f, 1.0f
        };
        glGenVertexArrays(1, &transparentCeilingVAO);
        glGenBuffers(1, &transparentCeilingVBO);
        glBindVertexArray(transparentCeilingVAO);
        glBindBuffer(GL_ARRAY_BUFFER, transparentCeilingVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(transparentVertices), transparentVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        /*glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));*/
        glBindVertexArray(0);
    }
    //draw aquarium
    glBindVertexArray(transparentCeilingVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

unsigned int sphereVAO = 0;
unsigned int sphereVBO = 0;
const unsigned int X_SEGMENTS = 64;
const unsigned int Y_SEGMENTS = 64;
void renderSphere()
{
    if (sphereVAO == 0)
    {
        std::vector<float> vertices;

        for (unsigned int y = 0; y <= Y_SEGMENTS; ++y)
        {
            for (unsigned int x = 0; x <= X_SEGMENTS; ++x)
            {
                float xSegment = (float)x / (float)X_SEGMENTS;
                float ySegment = (float)y / (float)Y_SEGMENTS;
                float xPos = std::cos(xSegment * 2.0f * M_PI) * std::sin(ySegment * M_PI);
                float yPos = std::cos(ySegment * M_PI);
                float zPos = std::sin(xSegment * 2.0f * M_PI) * std::sin(ySegment * M_PI);

                vertices.push_back(xPos);
                vertices.push_back(yPos);
                vertices.push_back(zPos);
                vertices.push_back(xPos);
                vertices.push_back(yPos);
                vertices.push_back(zPos);
                vertices.push_back(xSegment);
                vertices.push_back(ySegment);
            }
        }

        std::vector<unsigned int> indices;
        bool oddRow = false;
        for (unsigned int y = 0; y < Y_SEGMENTS; ++y)
        {
            if (!oddRow) // even rows: y == 0, y == 2; and so on
            {
                for (unsigned int x = 0; x <= X_SEGMENTS; ++x)
                {
                    indices.push_back(y * (X_SEGMENTS + 1) + x);
                    indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
                }
            }
            else
            {
                for (int x = X_SEGMENTS; x >= 0; --x)
                {
                    indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
                    indices.push_back(y * (X_SEGMENTS + 1) + x);
                }
            }
            oddRow = !oddRow;
        }

        glGenVertexArrays(1, &sphereVAO);
        glGenBuffers(1, &sphereVBO);
        unsigned int ebo;
        glGenBuffers(1, &ebo);

        glBindVertexArray(sphereVAO);
        glBindBuffer(GL_ARRAY_BUFFER, sphereVBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }

    glBindVertexArray(sphereVAO);
    glDrawElements(GL_TRIANGLE_STRIP, (X_SEGMENTS + 1) * 2 * Y_SEGMENTS, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
void processInput(GLFWwindow* window, std::vector<std::string>& faces, unsigned int& cubemapTexture)
{
    //camera
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		camera->ProcessKeyboard(Camera::ECameraMovementType::FORWARD, (float)deltaTime);
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		camera->ProcessKeyboard(Camera::ECameraMovementType::BACKWARD, (float)deltaTime);
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		camera->ProcessKeyboard(Camera::ECameraMovementType::LEFT, (float)deltaTime);
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		camera->ProcessKeyboard(Camera::ECameraMovementType::RIGHT, (float)deltaTime);
	if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
		camera->ProcessKeyboard(Camera::ECameraMovementType::UP, (float)deltaTime);
	if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
		camera->ProcessKeyboard(Camera::ECameraMovementType::DOWN, (float)deltaTime);

    //reset camera position
    if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS) 
    {
        int width, height;
        glfwGetWindowSize(window, &width, &height);
        camera->Reset(width, height);

    }

    //menu
    if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS)
    {
        menuShouldBeShown = true;
    }
    if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS)
    {
        menuShouldBeShown = false;
    }
    if (glfwGetKey(window, GLFW_KEY_F1) == GLFW_PRESS)
    {
        hideMenuCompletely = true;
    }
    if (glfwGetKey(window, GLFW_KEY_F2) == GLFW_PRESS)
    {
        hideMenuCompletely = false;
    }

    
	//night and day
	if (glfwGetKey(window, GLFW_KEY_4) == GLFW_PRESS && !isDay)
	{
		isDay = true;
		setFaces(faces, cubemapTexture);
	}
	if (glfwGetKey(window, GLFW_KEY_5) == GLFW_PRESS && isDay)
	{
		isDay = false;
		setFaces(faces, cubemapTexture);
	}

    //movement
    if (glfwGetKey(window, GLFW_KEY_6) == GLFW_PRESS)
    {
        objectsShouldMove = false;
    }
    if (glfwGetKey(window, GLFW_KEY_7) == GLFW_PRESS)
    {
        objectsShouldMove = true;
    }

    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
    {
        lightPos.z -= 0.1f;
    }
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
    {
        lightPos.z += 0.1f;
    }
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
    {
        lightPos.x -= 0.1f;
    }
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
    {
        lightPos.x += 0.1f;
    }
    if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS)
    {
        lightPos.y -= 0.1f;
    }
    if (glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS)
    {
        lightPos.y += 0.1f;
    }

    //std::cout << lightPos.x <<" "<< lightPos.y<<" "<< lightPos.z<<"\n";
}

//GOOD LIGHT POS
//18 11.8 5.3 lightView = glm::lookAt(lightPos, glm::vec3(10.0f, 0.0f, 4.0f), glm::vec3(0.0, 1.0, 0.0));
//

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	// make sure the viewport matches the new window dimensions; note that width and 
	// height will be significantly larger than specified on retina displays.
	camera->Reshape(width, height);
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
	camera->MouseControl((float)xpos, (float)ypos);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yOffset)
{
	camera->ProcessMouseScroll((float)yOffset);
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
	stbi_set_flip_vertically_on_load(true); 
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

void freeMemory()
{
    delete camera;
    delete SoundEngine;
    delete Text;
    delete fishObjModel;
    delete fishObjModel2;
    delete fishMan;
    delete rock;
    delete seaObjects;
    delete krab;
    delete statue;
    delete wall;
    delete greek;
}