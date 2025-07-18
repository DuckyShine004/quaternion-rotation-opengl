#pragma once

#include <glm/glm.hpp>

namespace engine::model {

struct Material {
    glm::vec3 diffuse;
    glm::vec3 specular;
    glm::vec3 ambient;

    float shininess;
};

} // namespace engine::model