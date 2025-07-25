#include "external/stb/stb_image.h"

#include "engine/model/Model.hpp"

#include "logger/LoggerMacros.hpp"

#include <assimp/Importer.hpp>

#include <assimp/postprocess.h>

namespace engine::model {

Model::Model(std::string path) : _position(glm::vec3(0.0f)) {
    this->loadModel(path);
}

Model::Model(std::string path, glm::vec3 position) : _position(position) {
    this->loadModel(path);
}

void Model::setPosition(glm::vec3 position) {
    this->_position = position;
}

void Model::setPosition(float x, float y, float z) {
    this->setPosition(glm::vec3(x, y, z));
}

glm::vec3 Model::getPosition() {
    return this->_position;
}

void Model::draw(Shader &shader) {
    for (Mesh &mesh : this->_meshes) {
        mesh.draw(shader);
    }
}

void Model::loadModel(std::string path) {
    Assimp::Importer importer;

    const aiScene *scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs);

    if (!scene || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) || !scene->mRootNode) {
        LOG_ERROR("Assimp Import Error: {}", importer.GetErrorString());
        return;
    }

    this->_directory = path.substr(0, path.find_last_of('/'));

    this->processNode(scene->mRootNode, scene);
}

void Model::processNode(aiNode *node, const aiScene *scene) {
    for (unsigned int i = 0; i < node->mNumMeshes; i++) {
        aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];

        this->_meshes.push_back(this->processMesh(mesh, scene));
    }

    for (unsigned int i = 0; i < node->mNumChildren; i++) {
        this->processNode(node->mChildren[i], scene);
    }
}

Mesh Model::processMesh(aiMesh *mesh, const aiScene *scene) {
    std::vector<Vertex> vertices;

    std::vector<unsigned int> indices;

    std::vector<Texture> textures;

    for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
        Vertex vertex;

        vertex.position = glm::vec3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);

        if (mesh->HasNormals()) {
            vertex.normal = glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);
        }

        vertex.textureCoordinates = glm::vec2(0.0f, 0.0f);

        if (mesh->mTextureCoords[0]) {
            vertex.textureCoordinates.x = mesh->mTextureCoords[0][i].x;
            vertex.textureCoordinates.y = mesh->mTextureCoords[0][i].y;
        }

        vertices.push_back(vertex);
    }

    for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
        aiFace face = mesh->mFaces[i];

        for (unsigned int j = 0; j < face.mNumIndices; j++) {
            indices.push_back(face.mIndices[j]);
        }
    }

    // Textures
    aiMaterial *material = scene->mMaterials[mesh->mMaterialIndex];

    this->loadDiffuseMaps(material, textures);
    this->loadSpecularMaps(material, textures);

    // Material properties
    Material _material = this->loadMaterial(material);

    return Mesh(vertices, indices, textures, _material);
}

Material Model::loadMaterial(aiMaterial *material) {
    Material _material;

    aiColor3D colour(0.0f, 0.0f, 0.0f);

    float shininess;

    material->Get(AI_MATKEY_COLOR_DIFFUSE, colour);
    _material.diffuse = glm::vec3(colour.r, colour.g, colour.b);

    material->Get(AI_MATKEY_COLOR_AMBIENT, colour);
    _material.ambient = glm::vec3(colour.r, colour.g, colour.b);

    material->Get(AI_MATKEY_COLOR_SPECULAR, colour);
    _material.specular = glm::vec3(colour.r, colour.g, colour.b);

    material->Get(AI_MATKEY_SHININESS, shininess);
    _material.shininess = shininess;

    return _material;
}

void Model::loadDiffuseMaps(aiMaterial *material, std::vector<Texture> &textures) {
    std::vector<Texture> diffuseMaps = this->loadTexturesFromMaterial(material, aiTextureType_DIFFUSE, "texture_diffuse");

    if (diffuseMaps.empty()) {
        diffuseMaps.push_back(this->createDiffuseTexture(material));
    }

    textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
}

Texture Model::createDiffuseTexture(aiMaterial *material) {
    aiColor3D diffuseColour(1.0f, 1.0f, 1.0f);

    material->Get(AI_MATKEY_COLOR_DIFFUSE, diffuseColour);

    Texture texture;

    texture.id = this->getTextureIdFromColour(diffuseColour);
    texture.type = "texture_diffuse";
    texture.path = "";

    return texture;
}

unsigned int Model::getTextureIdFromColour(aiColor3D colour) {
    std::vector<unsigned char> pixel = this->getPixelFromColour(colour);

    unsigned int textureId;

    glGenTextures(1, &textureId);

    glBindTexture(GL_TEXTURE_2D, textureId);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, pixel.data());

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    return textureId;
}

std::vector<unsigned char> Model::getPixelFromColour(aiColor3D colour) {
    std::vector<unsigned char> pixel(3);

    pixel[0] = static_cast<unsigned char>(colour.r * 255);
    pixel[1] = static_cast<unsigned char>(colour.g * 255);
    pixel[2] = static_cast<unsigned char>(colour.b * 255);

    return pixel;
}

void Model::loadSpecularMaps(aiMaterial *material, std::vector<Texture> &textures) {
    std::vector<Texture> specularMaps = this->loadTexturesFromMaterial(material, aiTextureType_SPECULAR, "texture_specular");

    textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
}

std::vector<Texture> Model::loadTexturesFromMaterial(aiMaterial *material, aiTextureType type, std::string name) {
    std::vector<Texture> textures;

    for (unsigned int i = 0; i < material->GetTextureCount(type); i++) {
        bool isLoaded = false;

        aiString path;

        material->GetTexture(type, i, &path);

        for (unsigned int j = 0; j < this->_loadedTextures.size(); j++) {
            if (std::strcmp(this->_loadedTextures[j].path.data(), path.C_Str()) == 0) {
                textures.push_back(this->_loadedTextures[j]);

                isLoaded = true;

                break;
            }
        }

        if (!isLoaded) {
            Texture texture;

            texture.id = this->getTextureIdFromFile(path.C_Str(), this->_directory);
            texture.type = name;
            texture.path = path.C_Str();

            textures.push_back(texture);

            this->_loadedTextures.push_back(texture);
        }
    }

    return textures;
}

unsigned int Model::getTextureIdFromFile(const char *path, const std::string &directory) {
    std::string filename = directory + '/' + std::string(path);

    unsigned textureId;

    glGenTextures(1, &textureId);

    int width;
    int height;
    int components;

    unsigned char *data = stbi_load(filename.c_str(), &width, &height, &components, 0);

    LOG_DEBUG("Loading texture from file: {}", filename);

    if (data) {
        GLenum format;

        if (components == 1) {
            format = GL_RED;
        } else if (components == 3) {
            format = GL_RGB;
        } else if (components == 4) {
            format = GL_RGBA;
        }

        glBindTexture(GL_TEXTURE_2D, textureId);

        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);

        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    } else {
        LOG_ERROR("Texture failed to load at path: {}", path);
    }

    stbi_image_free(data);

    return textureId;
}

} // namespace engine::model