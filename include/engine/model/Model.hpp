#pragma once

#include "engine/shader/Shader.hpp"

#include "engine/model/Mesh.hpp"
#include "engine/model/Material.hpp"

#include <assimp/scene.h>

#include <string>
#include <vector>

using namespace engine::shader;

namespace engine::model {

class Model {
  public:
    Model(std::string path);

    Model(std::string path, glm::vec3 position);

    void setPosition(glm::vec3 position);
    void setPosition(float x, float y, float z);

    glm::vec3 getPosition();

    void draw(Shader &shader);

  private:
    std::vector<Mesh> _meshes;

    std::vector<Texture> _loadedTextures;

    std::string _directory;

    glm::vec3 _position;

    void loadModel(std::string path);

    void processNode(aiNode *node, const aiScene *scene);

    Mesh processMesh(aiMesh *mesh, const aiScene *scene);

    std::vector<Texture> loadTexturesFromMaterial(aiMaterial *material, aiTextureType type, std::string name);

    unsigned int getTextureIdFromFile(const char *path, const std::string &directory);

    unsigned int getTextureIdFromColour(aiColor3D colour);

    Material loadMaterial(aiMaterial *material);

    void loadDiffuseMaps(aiMaterial *material, std::vector<Texture> &textures);

    void loadSpecularMaps(aiMaterial *material, std::vector<Texture> &textures);

    Texture createDiffuseTexture(aiMaterial *material);

    std::vector<unsigned char> getPixelFromColour(aiColor3D colour);
};

} // namespace engine::model