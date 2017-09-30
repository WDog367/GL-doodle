#pragma once
#include "Entity.h"
//Entity
//#########################################################

void Entity::Tick(float delta) {
	lastLoc = loc;
	glm::vec3 colour = bound.colour;
	bound = collider->getBounds();
	bound.colour = colour;
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
	collider->draw(vp);
}

void Entity::translate(const glm::vec3 &translation) {
	collider->translate(translation);
}
