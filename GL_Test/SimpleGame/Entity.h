#pragma once

#include "collision_utils.h"
#include "mesh_utils.h"
#include "OctTree.h"
#include "glm/glm.hpp"

class Entity {
public:
	glm::vec3 lastLoc;

	AABB bound;

public:
	Collider	*collider = nullptr;
	Mesh		*mesh = nullptr;

	bool hasMoved();

	glm::vec3 loc;
	glm::vec3 velocity;

	void Tick(float delta);

	const AABB* getBound();

	void draw(const glm::mat4 &mv);

	Entity(Collider*);

	void translate(const glm::vec3 &);
	void rotate(glm::quat &);
};
