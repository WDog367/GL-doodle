#pragma once

#include "collision_utils.h"
#include "mesh_utils.h"
#include "OctTree.h"

class Entity {
public:
	glm::vec3 lastLoc;

	AABB bound;

public:
	Collider *collider;
	Mesh *mesh;

	bool hasMoved();

	glm::vec3 loc;

	void Tick(float delta);

	const AABB* getBound();

	void draw(const glm::mat4 &mv);

	Entity(Collider*);

	void translate(const glm::vec3 &);
};
