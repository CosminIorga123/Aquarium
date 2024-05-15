#pragma once
#include <vector>
#include <iostream>
#include "Texture.h"
#include "Mesh.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

unsigned int TextureFromFile(const char* path, const string& directory, bool gamma = false);

class Model
{
public:
    // model data 
    vector<Texture> textures_loaded;	// stores all the textures loaded so far, optimization to make sure textures aren't loaded more than once.
    vector<Mesh>    meshes;
    string directory;
    bool gammaCorrection;
    glm::vec3 startPos;
    glm::vec3 targetPos;
    glm::vec3 currentPos;
    float rotation;

    // constructor, expects a filepath to a 3D model.
    Model(string const& path, bool bSmoothNormals, bool gamma = false);

    Model() = default;

    void setPos(glm::vec3 startPos, glm::vec3 targetPos, float rotation);

    // draws the model, and thus all its meshes
    void Draw(Shader& shader);

    bool moveObject(float moveIncrement, float rotateIncrement);

private:

    float currentRotationIncrement = 0.0f;
    bool rotate180(float rotateIncrement);

    // loads a model with supported ASSIMP extensions from file and stores the resulting meshes in the meshes vector.
    void loadModel(string const& path, bool bSmoothNormals);

    // processes a node in a recursive fashion. Processes each individual mesh located at the node and repeats this process on its children nodes (if any).
    void processNode(aiNode* node, const aiScene* scene);

    Mesh processMesh(aiMesh* mesh, const aiScene* scene);

    // checks all material textures of a given type and loads the textures if they're not loaded yet.
    // the required info is returned as a Texture struct.
    vector<Texture> loadMaterialTextures(aiMaterial* mat, aiTextureType type, string typeName);
};