#pragma once

#include <glm/glm.hpp>
#include <string>
#include <list>
#include <array>

#include "Primitive.h"

enum class NodeType {
		SceneNode,
		GeometryNode,
		JointNode,
		FlattenedSceneNode,
		LightNode,
		ParticleSystemNode,
#if 0
		BASE_NODE = 0,
		GEOMETRY_NODE,
		JOINT_NODE,
		LIGHT_NODE,
#endif
};

struct SceneNode {
	NodeType m_nodeType;
	std::string m_name;
	unsigned int m_nodeId;

	glm::mat4 trans = glm::mat4(1.f);
	glm::mat4 invtrans;

	std::list<SceneNode*> parents;
	std::list<SceneNode*> children;
public:
	SceneNode(const std::string& name);
	SceneNode::SceneNode(const SceneNode& other);

	virtual ~SceneNode();

	void set_transform(const glm::mat4& m);
	const glm::mat4& get_transform() const;
	const glm::mat4& get_inverse() const;

	virtual glm::mat4 localTransform() const;
	glm::mat4 globalTransform() const;

	void add_child(SceneNode* child);
	void remove_child(SceneNode* child);

	void rotate(char axis, float angle);
	void scale(const glm::vec3& amount);
	void translate(const glm::vec3& amount);
	};

typedef SceneNode* Scene;

#include "Primitive.h"
#include "Material.h"

class GeometryNode : public SceneNode {
public:
		GeometryNode(const std::string& name, Primitive* prim,
				Material* mat = nullptr);

		void setMaterial(Material* material);

		Material* m_material;
		Primitive* m_primitive;
};

struct JointNode : public SceneNode {

};

class Light {
public:
	Light()
		: colour(0.0, 0.0, 0.0),
		position(0.0, 0.0, 0.0)
	{
		falloff[0] = 1.0;
		falloff[1] = 0.0;
		falloff[2] = 0.0;
	}
	Light(const glm::vec3& position, const glm::vec3& colour, const float inFalloff[3])
		: colour(colour), position(position), falloff{ inFalloff[0], inFalloff[1], inFalloff[2] } {}
	Light(const glm::vec3& position, const glm::vec3& colour, const std::array<float, 3>& inFalloff)
		: colour(colour), position(position), falloff{ inFalloff[0], inFalloff[1], inFalloff[2] } {}

	glm::vec3 colour;
	glm::vec3 position;
	float falloff[3];
};

std::ostream& operator<<(std::ostream& out, const Light& l);

class LightNode : public SceneNode {
public:
	LightNode(const std::string& name, Light* light)
		: SceneNode(name)
		, m_light(light)
	{
		m_nodeType = NodeType::LightNode;
	}

	Light* m_light;
};