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

//Used to convert collider
enum collider_Type {
	SPHERE_TYPE,
	OBB_TYPE,
	AABB_TYPE,
	TRY_TYPE,
	EDGE_TYPE,
	CVHULL_TYPE,
	NULL_TYPE,
};

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
	glm::vec3 colour = glm::vec3(0,1,0);
	//could make the helper functions I have static functions of this

public:
	Collider::Collider(): type(NULL_TYPE) {}

	virtual virtual void draw(const glm::mat4 &vp)=0;

	virtual void translate(glm::vec3) =0;
};

//Sphere
//##########################################################
class Sphere : public Collider, public IGJKFcns {
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
};

//Oriented Bounding Box
//##########################################################
//should it store the world coordinates? maybe?
class OBB : public Collider {

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
class CV_Hull : public Collider, public IGJKFcns {
public:
	glm::vec3 *points;
	int num;
	glm::mat4 trans;
public:
	//copies points over. should store adjacency info?
	//could be better in many ways
	CV_Hull(glm::vec3* inpoints, int num, const glm::mat4 &trans);

	~CV_Hull();
	//GJK necessary interface
	virtual glm::vec3 furthestPointInDirection(glm::vec3 dir);
	//should also store adjacency information, it'll be usefull for getFurthestPoint()
	//could also do it as an array of triangles, but that seems... bad

	virtual void draw(const glm::mat4 &vp);

	virtual void translate(glm::vec3);

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
bool collision_EPA(IGJKFcns* a, IGJKFcns* b, std::vector<SupportPoint> sim, glm::vec3 *coll_point, glm::vec3 *coll_norm, float *coll_depth);

bool collision_GJK(IGJKFcns* a, IGJKFcns* b);

//##############################################################################################################
//The Collision Tests~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//for now, just return true/false; add collision info after
//##############################################################################################################


//Sphere Functions~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//Functions: Sphere vs. Sphere

bool collide(const Sphere &a, const Sphere& b);

//Sphere vs. edge
bool collide(const Sphere &s, const Edge& e);

//Sphere vs. triangle
bool collide(const Sphere &s, const Triangle &t);

//Sphere vs. AABB
double SquaredDistPointAABB(const glm::vec3 & p, const AABB & aabb);
bool collide(const Sphere &s, const AABB &b);

//Sphere vs. OBB
bool collide(const Sphere &s, const OBB &b);

//Sphere vs. Convex Hull
//(is just GJK, just need a findfurthestpoint implementation for the sphere, which is easy
bool collide(const Sphere &s, const CV_Hull &h);

//Rays vs. Things~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

//Ray vs. Plane
bool collide(const Edge &e, const Triangle &t);

//Ray vs. Ray //Really? this will almost never be true

//Ray vs. OBB (SAT)
bool collide(const OBB &a, const Edge &b);

//Ray vs. AABB (SAT?)
//technically a bit less efficient than it could be, but who do I care
bool collide(const AABB &a, const Edge &b);

//plane things~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//Plane vs. OBB
//SAT
bool collide(const OBB &a, const Triangle &b);

//Plane vs. AABB
//SAT
bool collide(const AABB &a, const Triangle &b);

//Plane vs. Convex Hull
bool collide(const CV_Hull &a, const Triangle &b);

//AABB things~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

//AABB vs. AABB
//Simple Check
bool collide(const AABB &a, const AABB &b);


//AABB vs. OBB
//SAT
bool collide(const OBB &a, const AABB &b);

//AABB vs. Convex Hull
bool collide(const CV_Hull &a, const AABB &b);
//OBB things~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

//OBB vs. OBB (SAT) (because I want to do it now, shuddup
//the first chunk of this is stuff

bool collide(const OBB &a, const OBB &b);

//OBB vs. Convex Hull
bool collide(const CV_Hull &a, const OBB &b);

//Convex Hull things~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

//CVHull vs. CVHull
bool collide(const CV_Hull &a, const CV_Hull &b);
