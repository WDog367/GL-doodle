#pragma once

#include "glm/glm.hpp"
#include <vector>
#include <queue>
#include <list>
#include <iostream>
//Need proper constructors for all of these guys

//##############################################################################################################
//Classes and types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//##############################################################################################################
#define COLLIDERS_DRAWABLE

class Sphere;
class OBB;
class Capsule;
class ConvexHull;
class AABB;
class Entity;

struct collideResult{
	bool isIntersect;
	glm::vec3 normal;
	float depth;
	glm::vec3 point;

	collideResult() {}

	collideResult(bool thing) {
		isIntersect = thing;
	}

	operator bool() const
	{
		return isIntersect;
	}

};

class Collider {
public:
	//generic transformations
	virtual void translate(glm::vec3) = 0;
	virtual void rotate(glm::quat) = 0;
	virtual void scale(glm::vec3) = 0;

	virtual glm::mat4 & getTransform() =0;

	//getting bounds, for use in coarse collision
	virtual AABB getBounds() = 0;
	virtual AABB getBounds(glm::mat4 &model) =0;

	//for GJK algorithm, for convex collisions
	virtual glm::vec3 furthestPointInDirection(glm::vec3 dir) = 0;

	//functions for the double dispatch
	virtual collideResult intersect(Collider &a) = 0;
	virtual collideResult intersect(Sphere &a) = 0;
	virtual collideResult intersect(OBB &a) = 0;
	virtual collideResult intersect(Capsule &a) = 0;
	virtual collideResult intersect(ConvexHull &a) = 0;

	virtual bool isMoving();

	//members?
public:
	Entity* owner = nullptr;
	virtual void tick(float delta);

#ifdef COLLIDERS_DRAWABLE
	glm::vec3 colour = glm::vec3(0, 1, 0);

	virtual void draw(const glm::mat4 &vp) = 0;
#endif
};

class OBB : public Collider{
public:
	//Oriented bounding box stored as a set of half widths, and a transformation
	glm::mat4 trans;
	glm::vec3 halfWidths;
	
public:
	//todo: minimize number of constructors
	//replace with static initialization functions (code becomes more readable, and creates more of a "funnel" to a single constructor
	OBB(glm::vec3 HalfWidths, glm::mat4 &trans);
	OBB(glm::vec3 min, glm::vec3 max);
	OBB(glm::vec3* points, int num);

	void getVertices(glm::vec3 verts[8]);

public:

	//generic transformations
	virtual void translate(glm::vec3);
	virtual void rotate(glm::quat);
	virtual void scale(glm::vec3);

	virtual glm::mat4 & getTransform();

	//getting bounds, for use in coarse collision
	virtual AABB getBounds();
	virtual AABB getBounds(glm::mat4 &model);

	//for GJK algorithm, for convex collisions
	virtual glm::vec3 furthestPointInDirection(glm::vec3 dir);

	//functions for the double dispatch
	virtual collideResult intersect(Collider &a);
	virtual collideResult intersect(Sphere &a);
	virtual collideResult intersect(OBB &a);
	virtual collideResult intersect(Capsule &a);
	virtual collideResult intersect(ConvexHull &a);

#ifdef COLLIDERS_DRAWABLE
	static void init_drawable();
	static bool isdrawableinit;
	static class Mesh* drawable;
	virtual void draw(const glm::mat4 &vp);
#endif
};

class ConvexHull : public Collider {
public:
	glm::vec3 *points;
	int num;
	glm::mat4 trans;
public:
	//copies points over. should store adjacency info?
	//could be better in many ways
	ConvexHull(glm::vec3* inpoints, int num, const glm::mat4 &trans);

	virtual ~ConvexHull();

public:
	//generic transformations
	virtual void translate(glm::vec3);
	virtual void rotate(glm::quat);
	virtual void scale(glm::vec3);

	//getting bounds, for use in coarse collision
	virtual AABB getBounds();
	virtual AABB getBounds(glm::mat4 &model);

	virtual glm::mat4 & getTransform();

	//GJK necessary interface
	virtual glm::vec3 furthestPointInDirection(glm::vec3 dir);
	//should also store adjacency information, it'll be usefull for getFurthestPoint()
	//could also do it as an array of triangles, but that seems... bad

	//functions for the double dispatch
	virtual collideResult intersect(Collider &a);
	virtual collideResult intersect(Sphere &a);
	virtual collideResult intersect(OBB &a);
	virtual collideResult intersect(Capsule &a);
	virtual collideResult intersect(ConvexHull &a);

#ifdef COLLIDERS_DRAWABLE
//	static void init_drawable();
//	static bool isdrawableinit;
//	static class Mesh* drawable;
	virtual void draw(const glm::mat4 &vp);
#endif

};

//actual collision functions
collideResult intersect(OBB &a, OBB &b);

//helper functions
//Does SAT (Seperating Axis Theorem) to check for collision using the axes and vertices provided. 
//i.e. it projects each shape onto each axis and checks for overlap
//This just leaves setting up the axes and verts in the collision fcns, which is a bit less code
bool SAT_collide(glm::vec3 &mtv, glm::vec3 &impactpoint, const glm::vec3 *axes, const glm::vec3 *a_vert, const glm::vec3 *b_vert, int axes_num, int a_num, int b_num);

//##################################################################################################################
//Stuff for GJK and EPA collision algorithms (Move to other file)
//##################################################################################################################
struct SupportPoint {
	glm::vec3 v;//Difference between a and b; i.e. the point on the minowski(spelled wrong) difference

	glm::vec3 a;//Point taken from hull A
	glm::vec3 b;//Point taken from hull B

	bool operator==(const SupportPoint &r) const;

};
//maybe these structs should be in the the cpp file? they're just for implementation?
/*
struct SP_Triangle {
SupportPoint points[3];
glm::vec3 norm;

SP_Triangle(const SupportPoint &a, const SupportPoint &b, const SupportPoint &c) {
points[0] = a;
points[1] = b;
points[2] = c;
norm = glm::normalize(glm::cross((b.v - a.v), (c.v - a.v)));
}
};

struct SP_Edge {
SupportPoint points[2];

SP_Edge(const SupportPoint &a, const SupportPoint &b) {
points[0] = a;
points[1] = b;
}
};

SupportPoint GenerateSupportPoint(IGJKFcns *a, IGJKFcns* b, glm::vec3 dir);
*/
//#########################################################
//find the baycentric point of a point on a triangle
void barycentric(const glm::vec3 &p, const glm::vec3 &a, const glm::vec3 &b, const glm::vec3 &c, float *u, float *v, float *w);

//#########################################################
//GJK stuff;
bool collision_GJK(Collider* a, Collider* b, glm::vec3 *coll_point, glm::vec3 *coll_norm, float *coll_depth);
void collision_EPA(Collider* a, Collider* b, std::vector<SupportPoint> sim, glm::vec3 *coll_point, glm::vec3 *coll_norm, float *coll_depth);

collideResult intersect_GJK(Collider& a, Collider& b);

#if 0
//classes which aren't rellay colliders
//class AABB;//will only be used in the course pass, compared against other AABBs
//class Triangle; //not 100% sure about this one, as it may be useful for polygon soups, but I won't deal with that yet

//interface for the furthestPointInDirection Function.
class IGJKFcns {
public:
	virtual glm::vec3 furthestPointInDirection(glm::vec3 dir) = 0;

	//make GJK and EPA static fcns
};

//need to add translation and/or rotation functions
//General class
class Collider {
protected:
	const collider_Type type;
	glm::vec3 colour = glm::vec3(0, 1, 0);
	//could make the helper functions I have static functions of this

public:
	Collider::Collider() : type(NULL_TYPE) {}

	virtual void draw(const glm::mat4 &vp) = 0;

	virtual void translate(glm::vec3) = 0;
};

//virtual functions used for the double dispatch
//perhaps could do it as a manual dispatch table? probably not
//primitives doing the whole "collide with each other" thing:
class Collidable : public Collider{
public:
	virtual collideResult intersect(Collidable &a)=0;
	virtual collideResult intersect(Sphere &a)=0;
	virtual collideResult intersect(OBB &a) = 0;
	virtual collideResult intersect(Capsule &a) =0;
	virtual collideResult intersect(ConvexHull &a)=0;
};

//Sphere
//##########################################################
class Sphere : public IGJKFcns, public Collidable {
public:
	glm::vec3 loc;
	float rad;
public:
	//constructor
	Sphere(glm::vec3 loc, float rad);

	//collider interface
	virtual glm::vec3 furthestPointInDirection(glm::vec3);

	virtual void draw(const glm::mat4 &vp);

	virtual void translate(glm::vec3);

	//functions for the double dispatch
public:
	virtual collideResult intersect(Collidable &a) ;
	virtual collideResult intersect(Sphere &a) ;
	virtual collideResult intersect(OBB &a) ;
	virtual collideResult intersect(Capsule &a) ;
	virtual collideResult intersect(ConvexHull &a) ;
};

//Oriented Bounding Box
//##########################################################
//should it store the world coordinates? maybe?
class OBB : public Collidable {

public:
	glm::vec3 max;
	glm::vec3 min;
	glm::mat4 trans;//transform multiplies the min and max to transform into world space

public:
	//constructors
	OBB(glm::vec3 min, glm::vec3 max);
	OBB(glm::vec3 min, glm::vec3 max, glm::mat4 &trans);

	//methods
	void getVerticies(glm::vec3 *verts) const;

	virtual void draw(const glm::mat4 &vp);

	virtual void translate(glm::vec3);

	//functions for the double dispatch
public:
	virtual collideResult intersect(Collidable &a) ;
	virtual collideResult intersect(Sphere &a) ;
	virtual collideResult intersect(OBB &a) ;
	virtual collideResult intersect(Capsule &a) ;
	virtual collideResult intersect(ConvexHull &a) ;
};

//Axis Aligned Bounding Box
//##########################################################
class AABB : public Collider {
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

	virtual void draw(const glm::mat4 &vp);

	virtual void translate(glm::vec3);
};

//##########################################################
class Triangle : public Collider {
public:
	glm::vec3 points[3];
	glm::vec3 norm;

public:
	Triangle(const glm::vec3 &a, const glm::vec3 &b, const glm::vec3 &c);

	virtual void draw(const glm::mat4 &vp);

	virtual void translate(glm::vec3);
};

//##########################################################
class Edge : public Collider {
public:
	glm::vec3 points[2];
	glm::vec3 n;

	float length;

	Edge(const glm::vec3 &a, const glm::vec3 &b);


	virtual void draw(const glm::mat4 &vp);

	virtual void translate(glm::vec3);
};

//##########################################################
class ConvexHull : public IGJKFcns, public Collidable {
public:
	glm::vec3 *points;
	int num;
	glm::mat4 trans;
public:
	//copies points over. should store adjacency info?
	//could be better in many ways
	ConvexHull(glm::vec3* inpoints, int num, const glm::mat4 &trans);

	virtual ~ConvexHull();
	//GJK necessary interface
	virtual glm::vec3 furthestPointInDirection(glm::vec3 dir);
	//should also store adjacency information, it'll be usefull for getFurthestPoint()
	//could also do it as an array of triangles, but that seems... bad

	virtual void draw(const glm::mat4 &vp);

	virtual void translate(glm::vec3);

	//functions for the double dispatch
public:
	virtual collideResult intersect(Collidable &a) ;
	virtual collideResult intersect(Sphere &a) ;
	virtual collideResult intersect(OBB &a) ;
	virtual collideResult intersect(Capsule &a) ;
	virtual collideResult intersect(ConvexHull &a) ;

};

class Capsule : public Collidable, public IGJKFcns {
public:
	glm::vec3 points[2];
	float radius;
public:
	Capsule(glm::vec3 a, glm::vec3 b, float radius);
	Capsule(float length, float radius);

//	virtual ~Capsule();

	virtual void draw(const glm::mat4 &vp);

	//transformations
	virtual void translate(glm::vec3);

public:
	virtual collideResult intersect(Collidable &a);
	virtual collideResult intersect(Sphere &a);
	virtual collideResult intersect(OBB &a);
	virtual collideResult intersect(Capsule &a);
	virtual collideResult intersect(ConvexHull &a);

public:
	virtual glm::vec3 furthestPointInDirection(glm::vec3 dir);
};
//##############################################################################################################
//Helper functions used in the collision Tests~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//##############################################################################################################
//Does SAT (Seperating Axis Theorem) to check for collision using the axes and vertices provided. 
//i.e. it projects each shape onto each axis and checks for overlap
//This just leaves setting up the axes and verts in the collision fcns, which is a bit less code
bool SAT_collide(const glm::vec3 *axes, const glm::vec3 *a_vert, const glm::vec3 *b_vert, int axes_num, int a_num, int b_num);
//##################################################################################################################
//Stuff for GJK and EPA collision algorithms ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//##################################################################################################################
struct SupportPoint {
	glm::vec3 v;//Difference between a and b; i.e. the point on the minowski(spelled wrong) difference

	glm::vec3 a;//Point taken from hull A
	glm::vec3 b;//Point taken from hull B

	bool operator==(const SupportPoint &r) const;

};
//maybe these structs should be in the the cpp file? they're just for implementation?
/*
struct SP_Triangle {
	SupportPoint points[3];
	glm::vec3 norm;

	SP_Triangle(const SupportPoint &a, const SupportPoint &b, const SupportPoint &c) {
		points[0] = a;
		points[1] = b;
		points[2] = c;
		norm = glm::normalize(glm::cross((b.v - a.v), (c.v - a.v)));
	}
};

struct SP_Edge {
	SupportPoint points[2];

	SP_Edge(const SupportPoint &a, const SupportPoint &b) {
		points[0] = a;
		points[1] = b;
	}
};

SupportPoint GenerateSupportPoint(IGJKFcns *a, IGJKFcns* b, glm::vec3 dir);
*/
//#########################################################
//find the baycentric point of a point on a triangle
void barycentric(const glm::vec3 &p, const glm::vec3 &a, const glm::vec3 &b, const glm::vec3 &c, float *u, float *v, float *w);

//#########################################################
//GJK stuff;
bool collision_GJK(IGJKFcns* a, IGJKFcns* b, glm::vec3 *coll_point, glm::vec3 *coll_norm, float *coll_depth);
void collision_EPA(IGJKFcns* a, IGJKFcns* b, std::vector<SupportPoint> sim, glm::vec3 *coll_point, glm::vec3 *coll_norm, float *coll_depth);

bool collision_GJK(IGJKFcns* a, IGJKFcns* b);

//##############################################################################################################
//The Collision Tests~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//for now, just return true/false; add collision info after
//##############################################################################################################

//make all of these static functions of the collider class, so we can not bother with everything being public

double SquaredDistPointAABB(const glm::vec3 & p, const AABB & aabb);

bool collide(const Sphere &a, const Sphere& b);
bool collide(const Sphere &s, const Edge& e);
bool collide(const Sphere &s, const Triangle &t);
bool collide(const Sphere &s, const AABB &b);
bool collide(const Sphere &s, const OBB &b);
bool collide(const Sphere &s, const ConvexHull &h);

bool collide(const ConvexHull &a, const OBB &b);
bool collide(const ConvexHull &a, const AABB &b);
bool collide(const ConvexHull &a, const Triangle &b);
bool collide(const ConvexHull &a, const ConvexHull &b);

bool collide(const AABB &a, const Edge &b);
bool collide(const AABB &a, const Triangle &b);
bool collide(const AABB &a, const AABB &b);

bool collide(const OBB &a, const OBB &b);
bool collide(const OBB &a, const Edge &b);
bool collide(const OBB &a, const Triangle &b);
bool collide(const OBB &a, const AABB &b);

bool collide(const Edge &e, const Triangle &t);

bool collide(const Capsule &a, const Capsule &b);
bool collide(const Capsule &a, const Sphere &b);
bool collide(const Capsule &a, const OBB &b);
bool collide(const Capsule &a, const ConvexHull &b);

#endif