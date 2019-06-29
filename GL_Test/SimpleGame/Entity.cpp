#pragma once
#include "Entity.h"
#include "glm/common.hpp"
#include "glm/glm.hpp"//includes most (if not all?) of GLM library
#include "glm/gtx/transform.hpp"

//Entity
//#########################################################

void Entity::Tick(float delta) {
	lastLoc = loc;
	glm::vec3 colour = bound.colour;
	bound = collider->getBounds();
	bound.colour = colour;

	translate(velocity*delta);
	//velocity *= 0.5;
}

bool Entity::hasMoved() {
	glm::vec3 velocity = loc - lastLoc;
	return velocity.x != 0 && velocity.y != 0 && velocity.z != 0;
}

const AABB * Entity::getBound() {
	return &bound;
}

Entity::Entity(Collider* collider) {
	//copy collider array
	this->collider = collider;
	collider->owner = this;
	this->bound = collider->getBounds();
}

void Entity::draw(const glm::mat4 &vp) {
	if (mesh) {
		glm::mat4 model = glm::translate(glm::mat4(1.0f), loc);
		mesh->Draw(vp, collider->getTransform());
	}
	if (collider) {
	//	collider->draw(vp);
	}
}

void Entity::translate(const glm::vec3 &translation) {
	collider->translate(translation);
	loc += translation;
}

void Entity::rotate(glm::quat &rotator)
{
	//collider->rotate(rotator);
}
