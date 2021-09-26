#pragma once

#include <glm/glm.hpp>

#include <vector>

class SceneNode;
class Light;

void gatherLightNodes(std::vector<Light>& out_lights, SceneNode* node, const glm::mat4& parentTransform = glm::mat4(1.0));
