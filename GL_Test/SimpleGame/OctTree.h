#pragma once
#include "collision_utils.h"
#include "mesh_utils.h"


#include <vector>
#include <queue>
class AABB;
class Entity;

//AABB used to keep track of regions
class AABB {
public:
	//members
	glm::vec3 min;
	glm::vec3 max;
public:
	//constructors
	AABB();
	AABB(glm::vec3 min, glm::vec3 max);

	//returns the 8 vertices, that's why no size is given. there are 8.
	void getVerticies(glm::vec3 *verts) const;

	//checks if another AABB rests within
	bool contains(const AABB &box);

	static bool collide(const AABB &, const AABB &);

	virtual void translate(glm::vec3);

	virtual AABB getBounds();
#ifdef COLLIDERS_DRAWABLE
	glm::vec3 colour = glm::vec3(0, .1, .1);

	static void init_drawable();
	static bool isdrawableinit;
	static class Mesh* drawable;
	virtual void draw(const glm::mat4 &vp);
#endif
};

struct CollisionInfo {//used to store a position 
	collideResult collision;
	Collider* member[2];
	
	CollisionInfo() {
	}

	CollisionInfo(collideResult a) {
		collision = a;
	}
};

//Todo:	-Remove Static things, instead have "OctreeBase" that Octree nodes reference
//		-Improve AABB to interact with  OBB
//		-Optimizations for static (non-stationary) objects (if an object hasn't moved, it doesn't need to be compared to other objects that haven't moved)
class OctTree {
private:
	AABB region;
	std::vector<Collider*> objList;

	OctTree *child[8];
	OctTree *parent;

	static const int minSize = 1;
	static const int minObj = 10;
	static const int maxDepth = 20;
	static std::queue<Collider*> pendingInsertion;
	bool treeBuilt = false;
	bool treeReady = false;

	int depth;

	std::vector<Collider*> collisionList;

private:
	void BuildTree();
	void CheckForMove();
	bool Reposition(Collider*);
public:
	OctTree(AABB region);
	OctTree(AABB region, std::vector<Collider*> objList);
	OctTree(AABB region, std::vector<Collider*> objList, OctTree* parent);

	void UpdateTree();
	//Perhaps don't do it as a reference? perhaps as a member of the octtree head?
	void CheckCollision(std::vector<CollisionInfo> &Collisions);
	void CheckCollision(std::vector<CollisionInfo> &Collisions, std::vector<Collider*> &incObjList);

	void Insert(Collider*);

	AABB getRegion() { return region; }

	void draw(const glm::mat4 &vp);

	void DrawContainingRegion(Collider* obj, glm::mat4 &vp);

};