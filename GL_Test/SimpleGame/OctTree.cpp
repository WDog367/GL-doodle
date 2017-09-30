#include "glm/glm.hpp"

#include "glm/gtx/transform.hpp"
#include "glm/gtc/type_ptr.hpp"

#include "mesh_utils.h"
#include "GL/glew.h"
#include "shader_utils.h"

#include <iostream>

#include "OctTree.h"

//Axis Aligned Bounding Box
//##########################################################
//constructors
AABB::AABB() : min(glm::vec3(0, 0, 0)), max(glm::vec3(0, 0, 0)) {
}
AABB::AABB(glm::vec3 min, glm::vec3 max) : min(min), max(max) {
}

AABB AABB::getBounds() {
	return *this;
}

bool AABB::isdrawableinit = false;
Mesh *AABB::drawable = nullptr;

void AABB::init_drawable() {
	if (!isdrawableinit) {
		GLfloat vertices[] = {
			-1, -1, -1,
			-1, 1, -1,
			1, 1, -1,
			1, -1, -1,
			-1, -1, 1,
			-1, 1, 1,
			1, 1, 1,
			1, -1, 1,
		};
		GLuint elements[] = {
			0, 1,
			1, 2,
			2, 3,
			3, 0,

			4, 5,
			5, 6,
			6, 7,
			7, 4,

			0, 4,
			1, 5,
			2, 6,
			3, 7,
		};

		struct shaderInfo shaders[] = { { GL_VERTEX_SHADER, "vs_wireframe.glsl" },{ GL_FRAGMENT_SHADER, "fs_wireframe.glsl" } };
		GLuint program = LoadProgram(shaders, 2);

		drawable = new Mesh(vertices, 24, elements, 24, program, GL_LINES);

		isdrawableinit = true;
	}
}


//maybe should return an array instead?
void AABB::getVerticies(glm::vec3 *verts) const {
	for (unsigned char i = 0; i < 8; i++) {
		verts[i] = glm::vec3(
			i & 0x04 ? min.x : max.x,
			i & 0x02 ? min.y : max.y,
			i & 0x01 ? min.z : max.z);
	}
}

bool AABB::contains(const AABB &box) {
	return (box.min.x > this->min.x && box.min.y > this->min.y && box.min.z > this->min.z) &&
		(box.max.x < this->max.x && box.max.y < this->max.y && box.max.z < this->max.z);
}

void AABB::translate(glm::vec3 diff) {
	min += diff;
	max += diff;
}

void AABB::draw(const glm::mat4 &vp) {
	init_drawable();

	glm::vec3 halfWidth = (max - min) * .5f;
	glm::mat4 model = glm::translate(min + halfWidth)*glm::scale(halfWidth);

	GLuint program = drawable->getProgram();
	glUseProgram(program);

	//set the colour value of the program (probably need a better system to do this in the mesh)
	GLuint uniform_colour = getUniform(program, "colour");
	glUniform3fv(uniform_colour, 1, glm::value_ptr(colour));

	drawable->Draw(vp*model);
}

bool AABB::collide(const AABB &a, const AABB &b) {
	return (a.min.x <= b.max.x && a.max.x >= b.min.x) &&
		(a.min.y <= b.max.y && a.max.y >= b.min.y) &&
		(a.min.z <= b.max.z && a.max.z >= b.min.z);
}

//Octtree Collision Partitioner

//#######################################

//class Declarations~~~~~~~~~~~~~~~~~
std::queue<Collider*> OctTree::pendingInsertion = std::queue<Collider*>();

OctTree::OctTree(AABB region) :region(region), parent(nullptr), depth(0) {
}

OctTree::OctTree(AABB region, std::vector<Collider*> objList) : region(region), objList(objList), parent(nullptr), depth(0) {
}
OctTree::OctTree(AABB region, std::vector<Collider*> objList, OctTree* parent) : region(region), objList(objList), parent(parent), depth(parent->depth + 1) {
}

void OctTree::draw(const glm::mat4 &vp) {
	bool isLeaf = true;
	for (int i = 0; i < 8; i++) {
		if (child[i] != nullptr) {
			child[i]->draw(vp);
		}
	}

	region.draw(vp);

}

void OctTree::UpdateTree() {

	if (!treeBuilt) {
		while (pendingInsertion.size() > 0) {
			//objList.insert(objList.end(), pendingInsertion.front());
			objList.push_back(pendingInsertion.front());
			pendingInsertion.pop();
		}
		BuildTree();
		treeBuilt = true;
	}
	else {
		//		while (pendingInsertion.size() > 0) {
		//			Insert(pendingInsertion.front());
		//			pendingInsertion.pop();
		//		}
		//doing this instead of rebuilding the tree can shave a a fraction of time off, when in debug mode
		//in release build it will prolly be less than a millisecond of savings, but #yolo
		CheckForMove();
	}

	treeReady = true;
}

/**
	debug function, tries to find obj in the tree,
	and draws the region that holds it
*/
void OctTree::DrawContainingRegion(Collider* obj, glm::mat4 &vp) {

	std::vector<Collider*>::iterator it;
	std::vector<Collider*>::iterator id;

	id = objList.end();

	//check owned objects, to see if we own this one
	for (it = objList.begin(); it != id; it++) {
		if (*it == obj) {
			region.draw(vp);
		}
	}

	//check children regions for object
	for (int i = 0; i < 8; i++) {
		if (child[i] != nullptr) {
			child[i]->DrawContainingRegion(obj, vp);
		}
	}
}

/** 
	check for colliders that have moved, 
	and see if they need to be repositioned
	in th octree	
*/
void OctTree::CheckForMove() {

	std::vector<Collider*>::iterator it;
	std::vector<Collider*>::iterator id;

	id = objList.end();
	it = objList.begin();
	while (it != id) {
		if ((*it)->isMoving()) {
			if (Reposition(*it)) {
				it = objList.erase(it);
				id = objList.end();
			}
			else {
				it++;
			}
		}
		else {
			it++;
		}
	}

	for (int i = 0; i < 8; i++) {
		if (child[i] != nullptr) {
			child[i]->CheckForMove();
		}
	}
}

/**
	Position an object in the octtree
	returns true, if the object has changed to a new region
	so that the caller can delete it from their list, if necessary

	todo: look to combine parts with BuildTree
*/
bool OctTree::Reposition(Collider* obj) {
	bool canCreateChild;
	bool inPos = false;
	OctTree* curNode = this;
	bool dropped;

	while (1) {
		dropped = false;
		glm::vec3 dimensions = curNode->region.max - curNode->region.min;
		if (!(//check if we're in a position where we'll be able to move down, or create a new node
			(curNode->objList.size() < minObj) ||
			(dimensions.x < minSize && dimensions.y < minSize && dimensions.z < minSize) ||
			curNode->depth >= maxDepth
			)) {
			//divide the current tree into octants
			glm::vec3 center = curNode->region.min + dimensions / 2.0f;
			AABB octant[8];

			octant[0] = AABB(region.min, center);
			octant[1] = AABB(glm::vec3(region.min.x, center.y, region.min.z), glm::vec3(center.x, region.max.y, center.z));
			octant[2] = AABB(glm::vec3(center.x, region.min.y, region.min.z), glm::vec3(region.max.x, center.y, center.z));
			octant[3] = AABB(glm::vec3(center.x, center.y, region.min.z), glm::vec3(region.max.x, region.max.y, center.z));
			octant[4] = AABB(glm::vec3(region.min.x, region.min.y, center.z), glm::vec3(center.x, center.y, region.max.z));
			octant[5] = AABB(glm::vec3(region.min.x, center.y, center.z), glm::vec3(center.x, region.max.y, region.max.z));
			octant[6] = AABB(glm::vec3(center.x, region.min.y, center.z), glm::vec3(region.max.x, center.y, region.max.z));
			octant[7] = AABB(glm::vec3(center.x, center.y, center.z), glm::vec3(region.max.x, region.max.y, region.max.z));

			for (int i = 0; i < 8; i++) {
				if (octant[i].contains(obj->getBounds())) {
					if (curNode->child[i] != nullptr) {
						curNode = curNode->child[i];
						dropped = true;
					}
					else {
						std::vector<Collider*> octList;
						octList.push_back(obj);
						curNode->child[i] = new OctTree(octant[i], octList, this);
						curNode->child[i]->BuildTree();
						return true;//building the tree will take care of it, we can just return
					}
				}
			}
		}
		if (!dropped) {
			if (curNode->region.contains(obj->getBounds()) || curNode->parent == nullptr) {
				//didn't fit into any of the children, but does fit this one
				//so this is where we want to be
				if (curNode != this) {
					curNode->objList.push_back(obj);
					return true;
				}
				else {
					return false;

				}
			}
			else {
				//doesn't fit in current region anymore, so move up to parent
				//and restart the loop
				curNode = curNode->parent;
			}
		}
	}

}

void OctTree::Insert(Collider* obj) {
	pendingInsertion.push(obj);
}

void OctTree::BuildTree() {
	//return conditions for this recursive fcn
	glm::vec3 dimensions = region.max - region.min;
	if ((objList.size() <= minObj) ||
		(dimensions.x < minSize && dimensions.y < minSize && dimensions.z < minSize) ||
		depth >= maxDepth) {
		for (int i = 0; i < 8; i++) {
			child[i] = nullptr;
		}

		return;
	}

	//divide the current tree into octants
	glm::vec3 center = region.min + dimensions / 2.0f;

	AABB octant[8];
	std::vector<Collider*> octList[8];

	octant[0] = AABB(region.min, center);
	octant[1] = AABB(glm::vec3(region.min.x, center.y, region.min.z), glm::vec3(center.x, region.max.y, center.z));
	octant[2] = AABB(glm::vec3(center.x, region.min.y, region.min.z), glm::vec3(region.max.x, center.y, center.z));
	octant[3] = AABB(glm::vec3(center.x, center.y, region.min.z), glm::vec3(region.max.x, region.max.y, center.z));
	octant[4] = AABB(glm::vec3(region.min.x, region.min.y, center.z), glm::vec3(center.x, center.y, region.max.z));
	octant[5] = AABB(glm::vec3(region.min.x, center.y, center.z), glm::vec3(center.x, region.max.y, region.max.z));
	octant[6] = AABB(glm::vec3(center.x, region.min.y, center.z), glm::vec3(region.max.x, center.y, region.max.z));
	octant[7] = AABB(glm::vec3(center.x, center.y, center.z), glm::vec3(region.max.x, region.max.y, region.max.z));

	//for each object in this tree, compare it to each octant; if it fits neatly in one, insert it
	std::vector<Collider*>::iterator it = objList.begin();
	while (it != objList.end()) {
		bool fit = false;

		for (int i = 0; i < 8; i++) {
			if (octant[i].contains((*it)->getBounds())) {
				octList[i].push_back(*it);
				fit = true;
				break;
			}
		}

		if (fit) {
			it = objList.erase(it);
		}
		else {
			it++;
		}
	}

	//recurse with each octant
	for (int i = 0; i < 8; i++) {
		if (octList[i].size() > 0) {
			child[i] = new OctTree(octant[i], octList[i], this);

			child[i]->BuildTree();

		}
		else {
			child[i] = nullptr;
		}
	}

	//the easy part
	treeBuilt = true;
}

/*
	Provides allocated memeber variable collisionList, as the storage for the object list
	needed in the recursive CheckCollision function
*/
void OctTree::CheckCollision(std::vector<CollisionInfo> &Collisions) {
	CheckCollision(Collisions, collisionList);
}


/*
	check a single collider agianst a list of possible colliders
	specified by the start and end iterators
	used in CheckCollision, below
*/
void inline CheckIndividualCollision(Collider* collider, 
	std::vector<Collider*>::iterator &it, std::vector<Collider*>::iterator &id,
	std::vector<CollisionInfo> &outCollisions) {
	for (; it != id; it++) {
		//check collision
		if (AABB::collide(collider->getBounds(), (*it)->getBounds())) {
			//reaction to collision (or summarize information)
			collideResult result = collider->intersect(*(*it));

			if (result) {
				outCollisions.emplace_back(result);
				outCollisions.back().member[0] = collider;
				outCollisions.back().member[1] = *it;
			}
		}
	}
}

/*
	Proceed down the tree, and compare each region's objects, to see if they've collided
*/
void OctTree::CheckCollision(std::vector<CollisionInfo> &Collisions, std::vector<Collider*> &incObjList) {

	static std::vector<Collider*>::iterator it;
	static std::vector<Collider*>::iterator jt;
	static std::vector<Collider*>::iterator id;
	std::vector<Collider*>::iterator kt;

	id = objList.end();

	glm::vec3 coll_point;
	glm::vec3 coll_norm;
	GLfloat coll_depth;

	collideResult tempResult;

	for (it = objList.begin(); it != id; it++) {
		//fist, check objects of this node against other ones in the node
		jt = it;
		jt++;
		CheckIndividualCollision(*it, jt, id, Collisions);

		//next, check them against the incoming object list
		CheckIndividualCollision(*it, incObjList.begin(), incObjList.end(), Collisions);
	}

	//add this node's objects to the object list, 
	//to send it on to the children nodes
	for (auto i = objList.begin(); i != objList.end(); i++) {
		incObjList.push_back(*i);
	}

	for (int i = 0; i < 8; i++) {
		if (child[i] != nullptr) {
			child[i]->CheckCollision(Collisions, incObjList);
		}
	}
	
	//remove added elements from incObjectList, before returning to parent
	incObjList.resize(incObjList.size() - objList.size());
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~