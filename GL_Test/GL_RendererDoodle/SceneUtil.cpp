#include "SceneUtil.h"

#include "Scene.h"

void gatherLightNodes(std::vector<Light>& out_lights, SceneNode* node, const glm::mat4& parentTransform) {
	glm::mat4 nodeTransform = parentTransform * node->localTransform();

	// todo: maybe handle encountering a BVH node as well...
	if (node->m_nodeType == NodeType::LightNode) {

		LightNode* light_node = static_cast<LightNode*>(node);
		out_lights.emplace_back(*light_node->m_light);
		out_lights.back().position = glm::vec3(nodeTransform * glm::vec4(light_node->m_light->position, 1.f));
	}

	for (SceneNode* child : node->children) {
		gatherLightNodes(out_lights, child, nodeTransform);
	}
}