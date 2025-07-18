#pragma once

#include "engine/shader/Shader.hpp"

#include "engine/model/Model.hpp"

#include "engine/camera/Camera.hpp"

#include "engine/sound/SoundSource.hpp"

#include <GLFW/glfw3.h>

#include <map>

using namespace engine::shader;

using namespace engine::model;

using namespace engine::camera;

using namespace engine::sound;

namespace engine::scene {

class Scene {
  public:
    Scene();

    void load();

    void update(GLFWwindow *window, float deltaTime);

    void updateResolution(int width, int height);

    void renderGrid();
    void render();

    Camera &getCamera();

  private:
    std::map<std::string, Shader> _shaders;

    Camera _camera;

    SoundSource _source;

    std::vector<Model> _models;
};

} // namespace engine::scene