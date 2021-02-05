#include "Scene.h"

#include <iostream>
#include <string>

#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/string_cast.hpp>

// Scene Node --------------------------------------------------------------

static unsigned int s_nextId = 0;


SceneNode::SceneNode(const std::string& name) 
    : m_name(name)
    , m_nodeType(NodeType::SceneNode)
    , trans(glm::mat4(1.f))
    , invtrans(glm::mat4(1.f))
    , m_nodeId(s_nextId++)
{
};


// Deep Copy
SceneNode::SceneNode(const SceneNode& other)
		: m_nodeType(other.m_nodeType)
		, m_name(other.m_name)
		, trans(other.trans)
		, invtrans(other.invtrans)
		, m_nodeId(s_nextId++)
{
		for (SceneNode* child : other.children) {
				this->children.push_front(new SceneNode(*child));
		}
}

SceneNode::~SceneNode() {
		for (SceneNode* child : children) {

				// todo: convert to using shared_ptrs
				child->parents.remove(this);
				if (child->parents.empty()) {
						delete child;
				}
		}

		for (SceneNode* parent : parents) {
				parent->children.remove(this);
		}
}

//---------------------------------------------------------------------------------------
void SceneNode::set_transform(const glm::mat4& m) {
		trans = m;
		invtrans = glm::inverse(m);
}

//---------------------------------------------------------------------------------------
const glm::mat4& SceneNode::get_transform() const {
		return trans;
}

//---------------------------------------------------------------------------------------
const glm::mat4& SceneNode::get_inverse() const {
		return invtrans;
}

//---------------------------------------------------------------------------------------
glm::mat4 SceneNode::localTransform() const {
		return get_transform();
}

//---------------------------------------------------------------------------------------
glm::mat4 SceneNode::globalTransform() const {
		if (parents.empty()) {
				return localTransform();
		}
		return parents.front()->globalTransform() * localTransform();
};

//---------------------------------------------------------------------------------------
void SceneNode::add_child(SceneNode* child) {
		children.push_back(child);
		child->parents.push_back(this);
}

//---------------------------------------------------------------------------------------
void SceneNode::remove_child(SceneNode* child) {
		children.remove(child);
		child->parents.remove(this);
}

//---------------------------------------------------------------------------------------
void SceneNode::rotate(char axis, float angle) {
		glm::vec3 rot_axis;

		switch (axis) {
		case 'x':
				rot_axis = glm::vec3(1, 0, 0);
				break;
		case 'y':
				rot_axis = glm::vec3(0, 1, 0);
				break;
		case 'z':
				rot_axis = glm::vec3(0, 0, 1);
				break;
		default:
				break;
		}
		glm::mat4 rot_matrix = glm::rotate(glm::radians(angle), rot_axis);
		set_transform(rot_matrix * trans);
}

//---------------------------------------------------------------------------------------
void SceneNode::scale(const glm::vec3& amount) {
		set_transform(glm::scale(amount) * trans);
}

//---------------------------------------------------------------------------------------
void SceneNode::translate(const glm::vec3& amount) {
		set_transform(glm::translate(amount) * trans);
}

//---------------------------------------------------------------------------------------
std::ostream& operator << (std::ostream& os, const SceneNode& node) {

		//os << "SceneNode:[NodeType: ___, name: ____, id: ____, isSelected: ____, transform: ____"
		switch (node.m_nodeType) {
		case NodeType::SceneNode:
				os << "SceneNode";
				break;
		case NodeType::GeometryNode:
				os << "GeometryNode";
				break;
		case NodeType::JointNode:
				os << "JointNode";
				break;
		}
		os << ":[";

		os << "name:" << node.m_name << ", ";
		os << "id:" << node.m_nodeId;

		os << "]\n";
		return os;
}

// GeometryNode ------------------------------------------------------------
#include "Primitive.h"
#include "Material.h"

//---------------------------------------------------------------------------------------
GeometryNode::GeometryNode(
		const std::string& name, Primitive* prim, Material* mat)
		: SceneNode(name)
		, m_material(mat)
		, m_primitive(prim)
{
		m_nodeType = NodeType::GeometryNode;
}

void GeometryNode::setMaterial(Material* mat)
{
		m_material = mat;
}

std::ostream& operator<<(std::ostream& out, const Light& l)
{
	out << "L[" << glm::to_string(l.colour)
		<< ", " << glm::to_string(l.position) << ", ";
	for (int i = 0; i < 3; i++) {
		if (i > 0) out << ", ";
		out << l.falloff[i];
	}
	out << "]";
	return out;
}
