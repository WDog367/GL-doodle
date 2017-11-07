#include "collision_utils.h"

#include "glm/glm.hpp"
#include <vector>
#include <queue>
#include <list>
#include <iostream>

#include "glm/gtx/transform.hpp"
#include "glm/gtc/type_ptr.hpp"

#include "OctTree.h"
#include "Entity.h"

//stuff for drawing
#ifdef COLLIDERS_DRAWABLE
#include "mesh_utils.h"
#include "GL/glew.h"
#include "shader_utils.h"
#endif

#include "math.h"

//from the Eigen library, the only place it's used is calculating the OBB from a point cloud
#include <Eigen/Eigenvalues>

bool Collider::isMoving()
{
	if (owner) {
		return owner->hasMoved();
	}

	return false;
}

void Collider::tick(float delta)
{

}

Collider::Collider(): trans(1.0f) {}

Collider::Collider(const glm::mat4& trans) : trans(trans) {}

//generic transformations
void Collider::translate(glm::vec3 v) {
	//keep translations on the right side
	trans = glm::translate(v)*trans;
}
void Collider::rotate(glm::quat q) {
	//not sure how I feel about this one
	//keep rotations on the left side
	trans = trans*glm::mat4_cast(q);
}
void Collider::scale(glm::vec3 s) {
	//keep scaling on the right side; shouldn't fight with rotation,
	trans = glm::scale(s)*trans;
}

glm::mat4 & Collider::getTransform()
{
	return trans;
}

AABB Collider::getBounds() {
	glm::vec3 x(1, 0, 0);
	glm::vec3 y(0, 1, 0);
	glm::vec3 z(0, 0, 1);

	//could be optimized a bit, rather than just calling furthestpointindirection
	glm::vec3 max(furthestPointInDirection(x).x, furthestPointInDirection(y).y, furthestPointInDirection(z).z);
	glm::vec3 min(furthestPointInDirection(-x).x, furthestPointInDirection(-y).y, furthestPointInDirection(-z).z);

	return AABB(min, max);

}

//OBB functions
OBB::OBB(glm::vec3 hw, glm::mat4 &trans) :halfWidths(hw), Collider(trans) {
}
OBB::OBB(glm::vec3 min, glm::vec3 max) {
	halfWidths = 0.5f*(max - min);
	trans = glm::translate(halfWidths + min);
}
OBB::OBB(glm::vec3* points, int num) {
	//create OBB to enscapulate point cloud, find algorithm on internet
	//an approximate algorithm

	//find the mean position of the set of points
	glm::vec3 meanPos = glm::vec3(0, 0, 0);
	for (int i = 0; i < num; i++) {
		meanPos += points[i];
	}
	meanPos = (1.f/num)*meanPos;

	std::cout << "meanPos: " << meanPos.x << ", " << meanPos.y << ", " << meanPos.z << std::endl;

	//create the co-variance matrix
	//(might make this more automatical)
	float xVariance = 0, yVariance = 0, zVariance = 0, xyCovariance = 0, xzCovariance = 0, yzCovariance = 0;
	for (int i = 0; i < num; i++) {
		xVariance += points[i].x*points[i].x;
		yVariance += points[i].y*points[i].y;
		zVariance += points[i].z*points[i].z;
		xyCovariance += points[i].x*points[i].y;
		xzCovariance += points[i].x*points[i].z;
		yzCovariance += points[i].y*points[i].z;
	}
	xVariance = xVariance / num - meanPos.x*meanPos.x;
	yVariance = yVariance / num - meanPos.y*meanPos.y;
	zVariance = zVariance / num - meanPos.z*meanPos.z;
	xyCovariance = xyCovariance / num - meanPos.x*meanPos.y;
	xzCovariance = xzCovariance / num - meanPos.x*meanPos.z;
	yzCovariance = yzCovariance / num - meanPos.y*meanPos.z;
	
	glm::mat3 covarianceMat = {	{xVariance, xyCovariance, xzCovariance}, 
								{xyCovariance, yVariance, yzCovariance}, 
								{xzCovariance, yzCovariance, zVariance} 
								};

	//using Eigen library here, for it's matrix solving usefulness
	Eigen::Matrix3f covMat; 
	covMat(0, 0) = xVariance;		covMat(0, 1) = xyCovariance;	covMat(0, 2) = xzCovariance;
	covMat(1, 0) = xyCovariance;	covMat(1, 1) = yVariance;		covMat(1, 2) = yzCovariance;
	covMat(2, 0) = xzCovariance;	covMat(2, 1) = yzCovariance;	covMat(2, 2) = zVariance;
	
	std::cout << "covmat: " << std::endl << covMat << std::endl;

	Eigen::EigenSolver<Eigen::Matrix3f> es(covMat);

	Eigen::Matrix3f ev = es.eigenvectors().real();

	Eigen::Matrix3f ev_imag = es.eigenvectors().imag();

	std::cout << "imaginary portion is (should be zero): " << std::endl << ev_imag << std::endl;

	for (int i = 0; i < 3; i++) {
		std::cout << "eigenvector " << i << "is:" << std::endl << ev.col(i) << std::endl;
	}

	//returning to glm now that we have the eigenVectors
	glm::mat4 tempTrans;
	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 3; j++) {
			tempTrans[i][j] = ev.col(i)[j];
		}
	}
	glm::mat3 invTrans = glm::inverse(glm::mat3(tempTrans));
	glm::vec3 min, max;
	min = max = glm::mat3(invTrans)*points[0];
	for (int i = 1; i < num; i++) {
		glm::vec3 temp = glm::mat3(invTrans)*points[i];
		for (int j = 0; j < 3; j++) {
			if (temp[j] > max[j]) {
				max[j] = temp[j];
			}
			if (temp[j] < min[j]) {
				min[j] = temp[j];
			}
		}
	}
	//assigning halfWidths
	halfWidths = (max - min)*0.5f;

	glm::vec3 midPoint = min + halfWidths;


	midPoint = glm::mat3(tempTrans)*midPoint;

	std::cout << "midpoint: " << midPoint.x << ", " << midPoint.y << ", " << midPoint.z << std::endl;

	tempTrans[3] = glm::vec4(midPoint, 1.0f);

	//assigning trans
	trans = tempTrans;
	//QED it's done
}

OBB::~OBB() {}

void OBB::getVertices(glm::vec3 verts[8]) {
	verts[0] = glm::vec3(trans*glm::vec4(-halfWidths.x, -halfWidths.y, -halfWidths.z, 1.0));
	verts[1] = glm::vec3(trans*glm::vec4(-halfWidths.x, halfWidths.y, -halfWidths.z, 1.0));
	verts[2] = glm::vec3(trans*glm::vec4(halfWidths.x, halfWidths.y, -halfWidths.z, 1.0));
	verts[3] = glm::vec3(trans*glm::vec4(halfWidths.x, -halfWidths.y, -halfWidths.z, 1.0));
	verts[4] = glm::vec3(trans*glm::vec4(-halfWidths.x, -halfWidths.y, halfWidths.z, 1.0));
	verts[5] = glm::vec3(trans*glm::vec4(-halfWidths.x, halfWidths.y, halfWidths.z, 1.0));
	verts[6] = glm::vec3(trans*glm::vec4(halfWidths.x, halfWidths.y, halfWidths.z, 1.0));
	verts[7] = glm::vec3(trans*glm::vec4(halfWidths.x, -halfWidths.y, halfWidths.z, 1.0));
}

//for GJK algorithm, for convex collisions
glm::vec3 OBB::furthestPointInDirection(glm::vec3 dir) {
	//convert the direction into the local space of this box
	glm::vec4  newDir = glm::vec4(glm::inverse(glm::mat3(trans))*dir, 1.0);

	//"clamp" the direction to the box
	for (int i = 0; i < 3; i++) {
		newDir[i] = newDir[i] > 0 ? halfWidths[i] : -halfWidths[i];
	}

	return glm::vec3(trans*newDir);
}

//functions for the double dispatch
collideResult OBB::intersect(Collider &a) {
	return a.intersect(*this);
}
collideResult OBB::intersect(Sphere &a) {
	return false;
}
collideResult OBB::intersect(OBB &a) {
	return ::intersect(a, *this);
}
collideResult OBB::intersect(Capsule &a) {
	return false;
}
collideResult OBB::intersect(ConvexHull &a) {
	return intersect_GJK(a, *this);
}

#ifdef COLLIDERS_DRAWABLE

bool OBB::isdrawableinit = false;
Mesh * OBB::drawable = nullptr;

void OBB::init_drawable() {
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

void OBB::draw(const glm::mat4 &vp) {
	init_drawable();
	
	GLuint program = drawable->getProgram();
	glUseProgram(program);

	//set the colour value of the program (probably need a better system to do this in the mesh)
	GLuint uniform_colour = getUniform(program, "colour");
	glUniform3fv(uniform_colour, 1, glm::value_ptr(colour));

	drawable->Draw(vp*trans*glm::scale(halfWidths));
}
#endif //COLLIDERS_DRAWABLE

//actual collision

collideResult intersect(OBB &a, OBB &b) {
	collideResult result;
	
	glm::vec3 axes[15];
	int num_axes;

	glm::vec3 a_vert[8];
	glm::vec3 b_vert[8];

	glm::vec3 mvt;
	glm::vec3 point;

	//Setup the world coord vertices
	a.getVertices(a_vert);
	b.getVertices(b_vert);

	//Setup the axes to test against: all of the face normals, plus their cross products
	for (int i = 0; i < 3; i++) {
		axes[i] = glm::normalize(glm::mat3(a.getTransform())[i]);//should make sure that this does work the way I think it does //(also, may need to normalize them)
		//std::cout << i << ": " << axes[i].x << ", " << axes[i].y << ", " << axes[i].z << std::endl;
	}
	for (int i = 0; i < 3; i++) {
		axes[i + 3] = glm::normalize(glm::mat3(b.getTransform())[i]);
		//std::cout << i + 3 << ": " << axes[i + 3].x << ", " << axes[i + 3].y << ", " << axes[i + 3].z << std::endl;
	}
	num_axes = 6;
	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 3; j++) {
			axes[num_axes] = glm::cross(axes[i], axes[j]);
			if (axes[num_axes].x != 0 || axes[num_axes].y != 0 || axes[num_axes].z != 0) {
				//could just not add it if length is zero, but refraining from normalization is fine
				axes[num_axes] = glm::normalize(axes[num_axes]);
				num_axes++;
			}
			//std::cout << 6 + 3 * i + j << ": " << axes[6 + 3 * i + j].x << ", " << axes[6 + 3 * i + j].y << ", " << axes[6 + 3 * i + j].z << std::endl;
		}
	}

	result.isIntersect = SAT_collide(mvt, point, axes, a_vert, b_vert, num_axes, 8, 8);
	result.depth = mvt.length();
	result.normal = mvt/result.depth;
	result.point = point;
	
	return result;
}

//helper functions
void ProjectOntoAxis(float &min, int &min_ind, float &max, int &max_ind, const glm::vec3 &axis, const glm::vec3 *vert, int num) {
	min = max = glm::dot(vert[0], axis);
	min_ind = max_ind = 0;
	for (int j = 1; j < num; j++) {
		float temp = glm::dot(vert[j], axis);
		if (temp < min) {
			min = temp;
			min_ind = j;
		}
		if (temp > max) {
			max = temp;
			max_ind = j;
		}
	}
}

bool SAT_collide(glm::vec3 &mtv, glm::vec3 &impactpoint, const glm::vec3 *axes, const glm::vec3 *a_vert, const glm::vec3 *b_vert, int axes_num, int a_num, int b_num) {
	//When projecting onto the axis, we only really need to store the min and max coords,
	//since they just make a 1-d line
	float a_min, a_max;
	float b_min, b_max;
	int a_min_i, a_max_i;
	int b_min_i, b_max_i;

	mtv = glm::vec3(0, 0, 0);
	float mtv_length = -1;

	for (int i = 0; i < axes_num; i++) {

		ProjectOntoAxis(a_min, a_min_i, a_max, a_max_i, axes[i], a_vert, a_num);
		ProjectOntoAxis(b_min, b_min_i, b_max, b_max_i, axes[i], b_vert, b_num);


		//testing for overlap
		//a---------------a
		//          b----------------------b
		if (a_max >= b_min && a_min <= b_max) {
			//there is an overlap on this axis
			float overlap;
			glm::vec3 nearestpoint;

			if (abs(a_max - b_min) < abs(a_min - b_max)) {
				overlap = a_max - b_min;
				nearestpoint = b_vert[b_min_i];
			}
			else {
				overlap = a_min - b_max;
				nearestpoint = b_vert[b_max_i];
			}

			if (abs(overlap) < mtv_length || mtv_length < 0) {
				mtv_length = abs(overlap);
				mtv = overlap*axes[i];

				impactpoint = mtv + nearestpoint;
			}
		}
		else {
			return false;// no overlap, therefor, we have a separating plane
		}

	}

	return true;
}

//##################################################################################################################
//Stuff for GJK and EPA collision algorithms ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//##################################################################################################################
bool SupportPoint::operator==(const SupportPoint &r) const { return v == r.v; }

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

SupportPoint GenerateSupportPoint(Collider *a, Collider* b, glm::vec3 dir) {
	SupportPoint res;
	res.a = a->furthestPointInDirection(dir);	//get the point from a
	res.b = b->furthestPointInDirection(-dir);	//get the point from b (why -dir again?)
	res.v = res.a - res.b;						//the difference is the point on the difference hull

	return res;
}

//#########################################################
//find the baycentric point of a point on a triangle
void barycentric(const glm::vec3 &p, const glm::vec3 &a, const glm::vec3 &b, const glm::vec3 &c, float *u, float *v, float *w) {
	// code from Crister Erickson's Real-Time Collision Detection
	glm::vec3 v0 = b - a, v1 = c - a, v2 = p - a;
	float d00 = glm::dot(v0, v0);
	float d01 = glm::dot(v0, v1);
	float d11 = glm::dot(v1, v1);
	float d20 = glm::dot(v2, v0);
	float d21 = glm::dot(v2, v1);
	float denom = d00 * d11 - d01 * d01;
	*v = (d11 * d20 - d01 * d21) / denom;
	*w = (d00 * d21 - d01 * d20) / denom;
	*u = 1.0f - *v - *w;
}

collideResult intersect_GJK(Collider& a, Collider& b) {
	collideResult result;
	result.isIntersect = collision_GJK(&a, &b, &result.point, &result.normal, &result.depth);
	return result;
}

//#########################################################
//made w\ this as reference: http://hacktank.net/blog/?p=93
//collision between two convex hulls, using GJK algoritm
bool collision_GJK(Collider* a, Collider* b, glm::vec3 *coll_point, glm::vec3 *coll_norm, float *coll_depth) {
	bool canExit = false;
	glm::vec3 dir(0, 0, 1);
	std::vector<SupportPoint> simplex;
	SupportPoint newPoint;

	glm::vec3 ao;	//point_a->origin

	glm::vec3 ac;	//point_a->point_c
	glm::vec3 ad;	//point_a->point_d
	//1. generate support point in arbitrary direction
	simplex.push_back(GenerateSupportPoint(a, b, dir));
	//2. set search direction to negative of support point
	dir = -(simplex[0].v);
	//iterative portion
	while (1) {
		//3. find support point in search direction
		//newPoint = (a->furthestPointInDirection(dir) - b->furthestPointInDirection(-dir));
		newPoint = GenerateSupportPoint(a, b, dir);
		if (!(glm::dot(newPoint.v, dir) > 0)) {
			//4. if new point dit not pass the origin (i.e. if it's dot product w\ the search dir is negative),  there is no intersection
			return false;
		}
		//5. push that suckah to the simplex
		simplex.push_back(newPoint);
		//6. depends on the size of our simplex
		ao = -newPoint.v;
		if (simplex.size() == 2) {
			glm::vec3 ab;
			//2 point case
			//set the search direction to be perpendicular to the simplex line, and coplanar w\ the 2 points and the origin
			ab = simplex[0].v - simplex[1].v;
			dir = glm::cross(glm::cross(ab, ao), ab);
		}
		else if (simplex.size() == 3) {
			glm::vec3 ab = simplex[1].v - simplex[2].v;
			glm::vec3 ac = simplex[0].v - simplex[2].v;
			glm::vec3 norm = glm::cross(ab, ac);

			if (glm::dot((glm::cross(ab, norm)), ao) > 0) {
				//it's outside the triangle, in front of ab
				simplex[0] = simplex[1];
				simplex[1] = simplex[2];
				simplex.resize(2);

				dir = glm::cross(glm::cross(ab, ao), ab);
			}
			else if (glm::dot((glm::cross(norm, ac)), ao) > 0) {
				//it's outside the triangle in front of ac
				simplex[0] = simplex[0];
				simplex[1] = simplex[2];
				simplex.resize(2);

				dir = glm::cross(glm::cross(ac, ao), ac);
			}
			//don't need to check cb, since it was checked already in the 2 point case
			else {
				//the point lies inside the triangle, hurrah!
				if (glm::dot(norm, ao) > 0) {
					//the origin is above the triangle, so the simplex isn't modified
					dir = norm;
				}
				else {
					//the origin is below the triangle
					//reverse the triangle (i.e. swap b/c)
					SupportPoint temp = simplex[0];
					simplex[0] = simplex[1];
					simplex[1] = temp;
					dir = -norm;
				}
			}
		}
		else if (simplex.size() == 4) {
			glm::vec3 ab = simplex[2].v - simplex[3].v;
			glm::vec3 ac = simplex[1].v - simplex[3].v;
			glm::vec3 ad = simplex[0].v - simplex[3].v;
			glm::vec3 norm;

			if (glm::dot(glm::cross(ab, ac), ao) > 0) {
				//origin is in front of abc
				//set simplex to be abc triangle
				simplex.erase(simplex.begin());
			}
			else if (glm::dot(glm::cross(ac, ad), ao) > 0) {
				//origin is in front of acd; simplex becomes this triangle
				simplex[0] = simplex[0];
				simplex[1] = simplex[1];
				simplex[2] = simplex[3];
				simplex.resize(3);
			}
			else if (glm::dot(glm::cross(ad, ab), ao) > 0) {
				//origin in front of triangle adb, simplex becomes this
				simplex[1] = simplex[0];
				simplex[0] = simplex[2];
				simplex[2] = simplex[3];
				simplex.resize(3);
			}
			//don't need to check face cbd, as it was already checked in the 3 point case
			else {
				//simplex is inside of the silly pyramid!
				break;
			}

			//same as 3-point case with our new simplex
			//(copy paste code necessary?
			ab = simplex[1].v - simplex[2].v;
			ac = simplex[0].v - simplex[2].v;
			norm = glm::cross(ab, ac);

			if (glm::dot((glm::cross(ab, norm)), ao) > 0) {
				//it's outside the triangle, in front of ab
				simplex[0] = simplex[1];
				simplex[1] = simplex[2];
				simplex.resize(2);

				dir = glm::cross(glm::cross(ab, ao), ab);
			}
			else if (glm::dot((glm::cross(norm, ac)), ao) > 0) {
				//it's outside the triangle in front of ac
				simplex[0] = simplex[0];
				simplex[1] = simplex[2];
				simplex.resize(2);

				dir = glm::cross(glm::cross(ac, ao), ac);
			}
			else {
				//the point lies inside the triangle, hurrah!
				if (glm::dot(norm, ao) > 0) {
					//the origin is above the triangle, so the simplex isn't modified
					dir = norm;
				}
				else {
					//this should never be called, as the origin should be above this simplex
					// if it was below, it would have been inside the pyramid, and we would have left the loop
					//the origin is below the triangle
					//reverse the triangle (i.e. swap b/c)
					std::cout << "#####this shouldn't ever happen, wtf#####\n";
					SupportPoint temp = simplex[0];
					simplex[0] = simplex[1];
					simplex[1] = temp;
					dir = -norm;
				}
			}
		}


	}

	if (coll_point != nullptr && coll_norm != nullptr && coll_depth != nullptr) {
		collision_EPA(a, b, simplex, coll_point, coll_norm, coll_depth);
	}
	return true;
}

//made w\ this as reference: http://hacktank.net/blog/?p=119
//used to find the normal and depths
void collision_EPA(Collider* a, Collider* b, std::vector<SupportPoint> sim, glm::vec3 *coll_point, glm::vec3 *coll_norm, float *coll_depth) {
	// list because we will be removing and adding a lot
	std::list<SP_Triangle> lst_triangles;
	std::list<SP_Edge> lst_edges;

	//1. build initial shape from GJK's
	lst_triangles.push_back(SP_Triangle(sim[3], sim[2], sim[1]));
	lst_triangles.push_back(SP_Triangle(sim[3], sim[1], sim[0]));
	lst_triangles.push_back(SP_Triangle(sim[3], sim[0], sim[2]));
	lst_triangles.push_back(SP_Triangle(sim[2], sim[0], sim[1]));

	while (1) {
		float dist;
		std::list<SP_Triangle>::iterator closest;
		SupportPoint newPoint;

		//2. Find the closest face on the polytype to the origin
		closest = lst_triangles.begin();
		dist = glm::dot((*closest).norm, (*closest).points[0].v);
		for (std::list<SP_Triangle>::iterator i = lst_triangles.begin(); i != lst_triangles.end(); i++) {
			float tempDist = glm::dot(i->norm, i->points[0].v);
			if (tempDist < dist) {
				dist = tempDist;
				closest = i;
			}
		}

		//3. find next support point for polytype, using closest tri's norm as direction
		newPoint = GenerateSupportPoint(a, b, (*closest).norm);

		//4. if this point is no further from the origin than the triangle, break
		//todo: what if glm::dot is less than dist? can't happen; why?
		if (glm::dot(closest->norm, newPoint.v) - dist < 0.001f) {

			//6. calulate all of that collision info!
			float bary_u, bary_v, bary_w;
			barycentric(closest->norm*dist,
				closest->points[0].v,
				closest->points[1].v,
				closest->points[2].v,
				&bary_u,
				&bary_v,
				&bary_w);

			//use the barycentric coords to get the collision in world space
			*coll_point = glm::vec3((bary_u*closest->points[0].a) +
				(bary_v*closest->points[1].a) +
				(bary_w*closest->points[2].a));
			*coll_norm = (closest->norm);
			*coll_depth = dist;
			break;
		}

		//5. Integrate the new point into the polytype
		//firstly, remove the faces that the new point can "see"
		//also, keep track of the edges, doing this garbage
		auto lam_addEdge = [&](const SupportPoint &a, const SupportPoint &b)->void {
			for (auto it = lst_edges.begin(); it != lst_edges.end(); it++) {
				if (it->points[0] == b && it->points[1] == a) {
					lst_edges.erase(it);
					return;
				}
			}
			lst_edges.emplace_back(a, b);
		};

		std::list<SP_Triangle>::iterator i = lst_triangles.begin();
		while (i != lst_triangles.end()) {
			if (glm::dot(i->norm, newPoint.v - i->points[0].v) > 0) {
				lam_addEdge(i->points[0], i->points[1]);
				lam_addEdge(i->points[1], i->points[2]);
				lam_addEdge(i->points[2], i->points[0]);
				i = lst_triangles.erase(i);
			}
			else {
				i++;
			}
		}
		//next, add the new triangles
		for (auto i = lst_edges.begin(); i != lst_edges.end(); i++) {
			lst_triangles.emplace_back(newPoint, i->points[0], i->points[1]);
		}
		lst_edges.clear();
	}
}
//End of the GJK and EPA things ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

//copies points over. should store adjacency info?
//could be better in many ways
ConvexHull::ConvexHull(glm::vec3* inpoints, int num, const glm::mat4 &trans) :num(num), Collider(trans) {
	points = (glm::vec3*)malloc(num * sizeof(glm::vec3));
	memcpy(points, inpoints, num * sizeof(glm::vec3));
}

ConvexHull::~ConvexHull() {
	delete points;
}

glm::vec3 ConvexHull::furthestPointInDirection(glm::vec3 originaldir) {
	glm::vec3 result;
	float length;

	//convert the search direction into object space
	glm::vec3 dir = glm::inverse(glm::mat3(trans))*originaldir;


	result = points[0];
	length = glm::dot(dir, result);

	for (int i = 0; i < num; i++) {
		float tempLength = glm::dot(dir, points[i]);
		if (tempLength > length) {
			result = points[i];
			length = tempLength;
		}
	}

	//transfrom result back into world space as it leaves
	return glm::vec3(trans*glm::vec4(result, 1.f));
}

//Convex Hull
collideResult ConvexHull::intersect(Collider &a) {
	return a.intersect(*this);
}
collideResult ConvexHull::intersect(Sphere &a) {
	return false;
}
collideResult ConvexHull::intersect(OBB &a) {
	return ::intersect_GJK(a, *this);
}
collideResult ConvexHull::intersect(Capsule &a) {
	return false;
}
collideResult ConvexHull::intersect(ConvexHull &a) {
	return intersect_GJK(a, *this);
}

//prolly don't ever use this one, until it's actually efficient
void ConvexHull::draw(const glm::mat4 &vp) {
	using namespace std;

	GLuint vao;
	GLuint vbo;
	GLuint vbo_index;
	GLuint program;
	GLuint attrib_coord;
	GLuint attrib_index;
	GLuint uniform_matrix;
	GLuint uniform_colour;

	glm::mat4 mvp = vp*trans;

	GLfloat *vertices = new GLfloat[num * 3];

	for (int i = 0; i < num; i++) {
		vertices[3 * i + 0] = points[i].x;
		vertices[3 * i + 1] = points[i].y;
		vertices[3 * i + 2] = points[i].z;
	}

	GLuint *elements = new GLuint[(num - 1) * 2];
	for (int i = 0; i < num - 1; i++) {
		elements[2 * i + 0] = i;
		elements[2 * i + 1] = i + 1;
	}

	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*(num * 3), vertices, GL_STATIC_DRAW);

	glGenBuffers(1, &vbo_index);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo_index);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint)*((num - 1) * 2), elements, GL_STATIC_DRAW);

	struct shaderInfo shaders[] = { { GL_VERTEX_SHADER, "vs_wireframe.glsl" },
	{ GL_FRAGMENT_SHADER, "fs_wireframe.glsl" } };

	program = LoadProgram(shaders, 2);
	glUseProgram(program);

	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	attrib_coord = getAttrib(program, "coord");
	glEnableVertexAttribArray(attrib_coord);
	glVertexAttribPointer(
		attrib_coord,	//attrib 'index'
		3,				//pieces of data per index
		GL_FLOAT,		//type of data
		GL_FALSE,		//whether to normalize
		0,				//space b\w data
		0				//offset from buffer start
	);

	uniform_matrix = getUniform(program, "mvp");
	glUniformMatrix4fv(uniform_matrix, 1, GL_FALSE, glm::value_ptr(mvp));

	uniform_colour = getUniform(program, "colour");
	glUniform3fv(uniform_colour, 1, glm::value_ptr(colour));

	int size;  glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &size);

	//glDrawArrays(GL_POINTS, 0, num);
	glDrawElements(GL_LINES, size / sizeof(GLuint), GL_UNSIGNED_INT, NULL);

	glDeleteBuffers(1, &vbo);
	glDeleteBuffers(1, &vbo_index);
	glDeleteVertexArrays(1, &vao);
	glDeleteProgram(program);

	delete[] vertices;
	delete[] elements;
}

#if 0
//Sphere
//##########################################################

Sphere::Sphere(glm::vec3 loc, float rad) :loc(loc), rad(rad){
}

glm::vec3 Sphere::furthestPointInDirection(glm::vec3 dir) {
	return loc + glm::normalize(dir)*rad;
}

void Sphere::translate(glm::vec3 diff) {
	loc += diff;
}

//Oriented Bounding Box
//##########################################################
//constructors
OBB::OBB(glm::vec3 min, glm::vec3 max) : min(min), max(max), trans(glm::mat4(1.0)) {
}
OBB::OBB(glm::vec3 min, glm::vec3 max, glm::mat4 &trans) : min(min), max(max), trans(trans) {
}

//not sure in which order this will give them.
//should mayber figure that out?
void OBB::getVerticies(glm::vec3 *verts) const{
	for (unsigned char i = 0; i < 8; i++) {
		verts[i] = glm::vec3(trans*glm::vec4(
			i & 0x04 ? min.x : max.x,
			i & 0x02 ? min.y : max.y,
			i & 0x01 ? min.z : max.z,
			1.0
			));
	}

}

void OBB::translate(glm::vec3 diff) {
	trans = trans*glm::translate(diff);
}

//Axis Aligned Bounding Box
//##########################################################
//constructors
AABB::AABB() : min(glm::vec3(0, 0, 0)), max(glm::vec3(0, 0, 0)) {
}
AABB::AABB(glm::vec3 min, glm::vec3 max) :min(min), max(max) {
}

//maybe should return an array instead?
void AABB::getVerticies(glm::vec3 *verts) const{
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

//##########################################################
Triangle::Triangle(const glm::vec3 &a, const glm::vec3 &b, const glm::vec3 &c) {
	points[0] = a;
	points[1] = b;
	points[2] = c;
	norm = glm::normalize(glm::cross((b - a), (c - a)));
}

void Triangle::translate(glm::vec3 diff) {
	for (int i = 0; i < 3; i++) {
		points[i] += diff;
	}
}
//##########################################################
Edge::Edge(const glm::vec3 &a, const glm::vec3 &b) {
	points[0] = a;
	points[1] = b;
	n = (b - a);
	//normalize the normal, and store the length
	length = glm::length(n);
	n = n / length;
}
void Edge::translate(glm::vec3 diff) {
	for (int i = 0; i < 2; i++) {
		points[i] += diff;
	}
}
//##########################################################
//copies points over. should store adjacency info?
//could be better in many ways
ConvexHull::ConvexHull(glm::vec3* inpoints, int num, const glm::mat4 &trans) :num(num), trans(trans) {
	points = (glm::vec3*)malloc(num*sizeof(glm::vec3));
	memcpy(points, inpoints, num*sizeof(glm::vec3));
}

ConvexHull::~ConvexHull() {
	delete points;
}

glm::vec3 ConvexHull::furthestPointInDirection(glm::vec3 originaldir) {
	glm::vec3 result;
	float length;
	
	glm::vec3 dir = glm::inverse(glm::mat3(trans))*originaldir;

	for (int i = 0; i < num; i++) {
		//it would be more efficient to multiply the dir by the inverse, rather than mult every point by the trans
//		glm::vec3 tempV = glm::vec3(
//			glm::translate(glm::mat4(1.0f), pos)
//			*glm::scale(scale)
//			*glm::mat4_cast(rot)
//			*glm::vec4(vertices[i], vertices[i + 1], vertices[i + 2], 1.0));

		float tempLength = glm::dot(dir, points[i]);

		if (i == 0) {
			result = points[i];
			length = tempLength;
		}
		else {
			if (tempLength > length) {
				result = points[i];
				length = tempLength;
			}
		}
	}

	return result;
}

void ConvexHull::translate(glm::vec3 diff) {
	trans = trans*glm::translate(diff);
}
//##############################################################################################################
//Helper functions used in the collision Tests~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//##############################################################################################################
//Does SAT (Seperating Axis Theorem) to check for collision using the axes and vertices provided. 
//i.e. it projects each shape onto each axis and checks for overlap
//This just leaves setting up the axes and verts in the collision fcns, which is a bit less code
bool SAT_collide(const glm::vec3 *axes, const glm::vec3 *a_vert, const glm::vec3 *b_vert, int axes_num, int a_num, int b_num) {
	//When projecting onto the axis, we only really need to store the min and max coords,
	//since they just make a 1-d line
	float a_min, a_max;
	float b_min, b_max;

	for (int i = 0; i < axes_num; i++) {

		//projecting A onto the axis
		a_min = a_max = glm::dot(a_vert[0], axes[i]);
		for (int j = 1; j < a_num; j++) {
			float temp = glm::dot(a_vert[j], axes[i]);
			if (temp < a_min) {
				a_min = temp;
			}
			if (temp > a_max) {
				a_max = temp;
			}
		}

		//projecting B onto the axis
		b_min = b_max = glm::dot(b_vert[0], axes[i]);
		for (int j = 1; j < b_num; j++) {
			float temp = glm::dot(b_vert[j], axes[i]);
			if (temp < b_min) {
				b_min = temp;
			}
			if (temp > b_max) {
				b_max = temp;
			}
		}

		//testing for overlap
		if (a_max > b_min && a_min < b_max) {
			continue;//dumb and pointless
		}
		else {
			return false;// no overlap, therefor, we have a separating plane
		}

	}

	return true;
}
//##################################################################################################################
//Stuff for GJK and EPA collision algorithms ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//##################################################################################################################
bool SupportPoint::operator==(const SupportPoint &r) const { return v == r.v; }

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

SupportPoint GenerateSupportPoint(IGJKFcns *a, IGJKFcns* b, glm::vec3 dir) {
	SupportPoint res;
	res.a = a->furthestPointInDirection(dir);
	res.b = b->furthestPointInDirection(-dir);
	res.v = res.a - res.b;

	return res;
}

//#########################################################
//find the baycentric point of a point on a triangle
void barycentric(const glm::vec3 &p, const glm::vec3 &a, const glm::vec3 &b, const glm::vec3 &c, float *u, float *v, float *w) {
	// code from Crister Erickson's Real-Time Collision Detection
	glm::vec3 v0 = b - a, v1 = c - a, v2 = p - a;
	float d00 = glm::dot(v0, v0);
	float d01 = glm::dot(v0, v1);
	float d11 = glm::dot(v1, v1);
	float d20 = glm::dot(v2, v0);
	float d21 = glm::dot(v2, v1);
	float denom = d00 * d11 - d01 * d01;
	*v = (d11 * d20 - d01 * d21) / denom;
	*w = (d00 * d21 - d01 * d20) / denom;
	*u = 1.0f - *v - *w;
}

bool collision_GJK(IGJKFcns* a, IGJKFcns* b) {
	return collision_GJK(a, b, nullptr, nullptr, nullptr);
}

//#########################################################
//made w\ this as reference: http://hacktank.net/blog/?p=93
//collision between two convex hulls, using GJK algoritm
bool collision_GJK(IGJKFcns* a, IGJKFcns* b, glm::vec3 *coll_point, glm::vec3 *coll_norm, float *coll_depth) {
	bool canExit = false;
	glm::vec3 dir(0, 0, 1);
	std::vector<SupportPoint> simplex;
	SupportPoint newPoint;

	glm::vec3 ao;

	glm::vec3 ac;
	glm::vec3 ad;
	//1. generate support point in arbitrary direction
	simplex.push_back(GenerateSupportPoint(a, b, dir));
	//2. set search direction to negative of support point

	dir = -(simplex[0].v);
	//iterative portion
	while (1) {
		//3. find support point in search direction
		//newPoint = (a->furthestPointInDirection(dir) - b->furthestPointInDirection(-dir));
		newPoint = GenerateSupportPoint(a, b, dir);
		if (!(glm::dot(newPoint.v, dir) > 0)) {
			//4. if new point dit not pass the origin (i.e. if it's dot product w\ the search dir is negative),  there is no intersection
			std::cout << "no intersection occured" << std::endl;
			return false;
		}
		//5. push that suckah to the simplex
		simplex.push_back(newPoint);
		//6. depends on the size of our simplex
		ao = -newPoint.v;
		if (simplex.size() == 2) {
			glm::vec3 ab;
			//2 point case
			//set the search direction to be perpendicular to the simplex line, and coplanar w\ the 2 points and the origin
			ab = simplex[0].v - simplex[1].v;
			dir = glm::cross(glm::cross(ab, ao), ab);
		}
		else if (simplex.size() == 3) {
			glm::vec3 ab = simplex[1].v - simplex[2].v;
			glm::vec3 ac = simplex[0].v - simplex[2].v;
			glm::vec3 norm = glm::cross(ab, ac);

			if (glm::dot((glm::cross(ab, norm)), ao) > 0) {
				//it's outside the triangle, in front of ab
				simplex[0] = simplex[1];
				simplex[1] = simplex[2];
				simplex.resize(2);

				dir = glm::cross(glm::cross(ab, ao), ab);
			}
			else if (glm::dot((glm::cross(norm, ac)), ao) > 0) {
				//it's outside the triangle in front of ac
				simplex[0] = simplex[0];
				simplex[1] = simplex[2];
				simplex.resize(2);

				dir = glm::cross(glm::cross(ac, ao), ac);
			}
			else {
				//the point lies inside the triangle, hurrah!
				if (glm::dot(norm, ao) > 0) {
					//the origin is above the triangle, so the simplex isn't modified
					dir = norm;
				}
				else {
					//the origin is below the triangle
					//reverse the triangle (i.e. swap b/c)
					SupportPoint temp = simplex[0];
					simplex[0] = simplex[1];
					simplex[1] = temp;
					dir = -norm;
				}
			}
		}
		else if (simplex.size() == 4) {
			glm::vec3 ab = simplex[2].v - simplex[3].v;
			glm::vec3 ac = simplex[1].v - simplex[3].v;
			glm::vec3 ad = simplex[0].v - simplex[3].v;
			glm::vec3 norm;

			if (glm::dot(glm::cross(ab, ac), ao) > 0) {
				//origin is in front of abc
				//set simplex to be abc triangle
				simplex.erase(simplex.begin());
			}
			else if (glm::dot(glm::cross(ac, ad), ao) > 0) {
				//origin is in front of acd; simplex becomes this triangle
				simplex[0] = simplex[0];
				simplex[1] = simplex[1];
				simplex[2] = simplex[3];
				simplex.resize(3);
			}
			else if (glm::dot(glm::cross(ad, ab), ao) > 0) {
				//origin in front of triangle adb, simplex becomes this
				simplex[1] = simplex[0];
				simplex[0] = simplex[2];
				simplex[2] = simplex[3];
				simplex.resize(3);
			}
			else {
				//simplex is inside of the silly pyramid!
				break;
			}

			//same as 3-point case with our new simplex
			ab = simplex[1].v - simplex[2].v;
			ac = simplex[0].v - simplex[2].v;
			norm = glm::cross(ab, ac);

			if (glm::dot((glm::cross(ab, norm)), ao) > 0) {
				//it's outside the triangle, in front of ab
				simplex[0] = simplex[1];
				simplex[1] = simplex[2];
				simplex.resize(2);

				dir = glm::cross(glm::cross(ab, ao), ab);
			}
			else if (glm::dot((glm::cross(norm, ac)), ao) > 0) {
				//it's outside the triangle in front of ac
				simplex[0] = simplex[0];
				simplex[1] = simplex[2];
				simplex.resize(2);

				dir = glm::cross(glm::cross(ac, ao), ac);
			}
			else {
				//the point lies inside the triangle, hurrah!
				if (glm::dot(norm, ao) > 0) {
					//the origin is above the triangle, so the simplex isn't modified
					dir = norm;
				}
				else {
					//this should never be called, as the origin should be above this simplex
					//the origin is below the triangle
					//reverse the triangle (i.e. swap b/c)
					std::cout << "#####this shouldn't ever happen, wtf#####\n";
					SupportPoint temp = simplex[0];
					simplex[0] = simplex[1];
					simplex[1] = temp;
					dir = -norm;
				}
			}
		}


	}

	if (coll_point != nullptr && coll_norm != nullptr && coll_depth != nullptr) {
		collision_EPA(a, b, simplex, coll_point, coll_norm, coll_depth);
	}
	return true;
}

//made w\ this as reference: http://hacktank.net/blog/?p=119
//used to find the normal and depths
void collision_EPA(IGJKFcns* a, IGJKFcns* b, std::vector<SupportPoint> sim, glm::vec3 *coll_point, glm::vec3 *coll_norm, float *coll_depth) {
	// list because we will be removing and adding a lot
	std::list<SP_Triangle> lst_triangles;
	std::list<SP_Edge> lst_edges;

	//1. build initial shape from GJK's
	lst_triangles.push_back(SP_Triangle(sim[3], sim[2], sim[1]));
	lst_triangles.push_back(SP_Triangle(sim[3], sim[1], sim[0]));
	lst_triangles.push_back(SP_Triangle(sim[3], sim[0], sim[2]));
	lst_triangles.push_back(SP_Triangle(sim[2], sim[0], sim[1]));


	while (1) {
		float dist;
		std::list<SP_Triangle>::iterator closest;
		SupportPoint newPoint;

		//2. Find the closest face on the polytype to the origin
		closest = lst_triangles.begin();
		dist = glm::dot((*closest).norm, (*closest).points[0].v);
		for (std::list<SP_Triangle>::iterator i = lst_triangles.begin(); i != lst_triangles.end(); i++) {
			float tempDist = glm::dot(i->norm, i->points[0].v);
			if (tempDist < dist) {
				dist = tempDist;
				closest = i;
			}
		}

		//3. find next support point for polytype, using closest tri's norm as direction
		newPoint = GenerateSupportPoint(a, b, (*closest).norm);

		//4. if this point is no further from the origin than the triangle, break
		if (glm::dot(closest->norm, newPoint.v) - dist < 0.001f) {
			//6. calulate all of that collision info!
			float bary_u, bary_v, bary_w;
			barycentric(closest->norm*dist,
				closest->points[0].v,
				closest->points[1].v,
				closest->points[2].v,
				&bary_u,
				&bary_v,
				&bary_w);

			//use the barycentric coords to get the collision in world space
			*coll_point = glm::vec3((bary_u*closest->points[0].a) +
				(bary_v*closest->points[1].a) +
				(bary_w*closest->points[2].a));
			*coll_norm = -(closest->norm);
			*coll_depth = dist;
			break;
		}

		//5. Integrate the new point into the polytype
		//firstly, remove the faces that the new point can "see"
		//also, keep track of the edges, doing this garbage

		//freakin' lambda functions, aren't they great?
		auto lam_addEdge = [&](const SupportPoint &a, const SupportPoint &b)->void {
			for (auto it = lst_edges.begin(); it != lst_edges.end(); it++) {
				if (it->points[0] == b && it->points[1] == a) {
					lst_edges.erase(it);
					return;
				}
			}
			lst_edges.emplace_back(a, b);
		};

		std::list<SP_Triangle>::iterator i = lst_triangles.begin();
		while (i != lst_triangles.end()) {
			if (glm::dot(i->norm, newPoint.v - i->points[0].v) > 0) {
				lam_addEdge(i->points[0], i->points[1]);
				lam_addEdge(i->points[1], i->points[2]);
				lam_addEdge(i->points[2], i->points[0]);
				i = lst_triangles.erase(i);
			}
			else {
				i++;
			}
		}
		//next, add the new triangles
		for (auto i = lst_edges.begin(); i != lst_edges.end(); i++) {
			lst_triangles.emplace_back(newPoint, i->points[0], i->points[1]);
		}
		lst_edges.clear();
	}
}
//End of the GJK and EPA things ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

//##############################################################################################################
//Sphere Functions~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//##############################################################################################################

//#########################################################
//Functions: Sphere vs. Sphere
//for now, just return true/false; add collision info after
bool collide(const Sphere &a, const Sphere& b) {
	glm::vec3 d = b.loc - a.loc;
	return (d.x*d.x + d.y*d.y + d.z*d.z) < (a.rad + b.rad)*(a.rad + b.rad);
}

//#########################################################
//Sphere vs. edge
bool collide(const Sphere &s, const Edge& e) {
	float c = glm::dot((s.loc - e.points[0]), e.n);//the distance to nearest point on edge, from point[0] of the line
	glm::vec3 p;

	if (c < 0) {
		//the closest point to the circle doesn't lie within the edge
		//still, check if nearest edge points are in it
		p = e.points[0];
	}
	else if (c > e.length) {
		//same as above
		p = e.points[1];
	}
	else {
		p = c*e.n + e.points[0];//the nearest point on the edge
	}
	
	glm::vec3 d = s.loc - p;//the vec b/w sphere and nearest point

	//need to also check if the point is b\w the two points
	return (d.x*d.x + d.y*d.y + d.z*d.z) < (s.rad)*(s.rad);

}

//#########################################################
//Sphere vs. triangle
bool collide(const Sphere &s, const Triangle &t) {
	float d = glm::dot(s.loc - t.points[0], t.norm);
	//if is infinite plane, just check that: abs(d) < s.rad
	//return abs(d) <= s.rad;

	//the nearest point is outside of the sphere, no point is within
	if (abs(d) > s.rad) return false;

	glm::vec3 p = s.loc + (-d)*t.norm;//should be the point on the plane
	float u, v, w;

	//check if the point on the plane is within the triangle
	barycentric(p, t.points[0], t.points[1], t.points[2], &u, &v, &w);
	if (u > 0 && v > 0 && w > 0) return true;//the point is within the triangle

	//check that none of the three points are within the sphere
	for (int i = 0; i < 3; i++) {
		p = t.points[i];
		if ((p.x*p.x + p.y*p.y + p.z*p.z) < (s.rad)*(s.rad)) {
			return true;
		}
	}

	//no collision
	return false;

}

//#########################################################
//Sphere vs. AABB
double SquaredDistPointAABB(const glm::vec3 & p, const AABB & aabb)
{
	//finds the squared distance b\w a point and AABB
	auto check = [&](
		const double pn,
		const double bmin,
		const double bmax) -> double
	{
		double out = 0;
		double v = pn;

		if (v < bmin)
		{
			double val = (bmin - v);
			out += val * val;
		}
		else if (v > bmax)
		{
			double val = (v - bmax);
			out += val * val;
		}

		return out;
	};

	// Squared distance
	double sq = 0.0;

	sq += check(p.x, aabb.min.x, aabb.max.x);
	sq += check(p.y, aabb.min.y, aabb.max.y);
	sq += check(p.z, aabb.min.z, aabb.max.z);

	return sq;
}

//Sphere vs. AABB
bool collide(const Sphere &s, const AABB &b) {
	double d = SquaredDistPointAABB(s.loc, b);

	//will need to do it the other way to find the point of impact
	if (d < s.rad*s.rad) 
		return true;
	//check if the sphere is contained, in case it is smaller than AABB
	else if (s.loc.x > b.min.x && s.loc.x <= b.max.x &&
		s.loc.y > b.min.y && s.loc.y <= b.max.y &&
		s.loc.z > b.min.z && s.loc.z <= b.max.z)
		return true;
	else
		return false;

}


//Sphere vs. OBB
bool collide(const Sphere &s, const OBB &b) {
	glm::mat4 inv = glm::inverse(b.trans);
	
	//get the sphere in the local space of the OBB
	Sphere s_prime(glm::vec3(inv*glm::vec4(s.loc, 1.0)), s.rad);
	AABB b_prime(b.min, b.max);

	//bam, easy peasy
	return collide(s_prime, b_prime);
}

//Sphere vs. Convex Hull
//(is just GJK, just need a findfurthestpoint implementation for the sphere, which is easy
bool collide(const Sphere &s, const ConvexHull &h) {
	return collision_GJK((IGJKFcns*)&s, (IGJKFcns*)&h);
}

//Rays vs. Things~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

//Ray vs. Plane
bool collide(const Edge &e, const Triangle &t) {
	double temp = glm::dot(e.n, t.norm);

	if (temp == 0) return false;

	float d = glm::dot((t.points[0] - e.points[0]), t.norm) / temp;

	glm::vec3 point = e.points[0] + d*e.n;

	//check if point is on linem, and in triangle
	return true;

}

//Ray vs. Ray //Really? this will almost never be true

//Ray vs. OBB (SAT)
bool collide(const OBB &a, const Edge &b) {
	glm::vec3 axes[6];

	glm::vec3 a_vert[8];
	glm::vec3 const *b_vert;

	//Setup the world coord vertices (should probably be a function of the class)
	//not sure if this gives them in a reasonable order?
	a.getVerticies(a_vert);
	b_vert = b.points;


	//Setup the axes to test against: all of the face normals, plus their cross products
	for (int i = 0; i < 3; i++) {
		axes[i] = glm::mat3(a.trans)[i];//should make sure that this does work the way I think it does //(also, may need to normalize them)
	}

	for (int i = 0; i < 3; i++) {
		axes[i + 3] = glm::cross(glm::mat3(a.trans)[i], b.n);//should make sure that this does work the way I think it does //(also, may need to normalize them)
	}

	return SAT_collide(axes, a_vert, b_vert, 6, 8, 2);
}

//Ray vs. AABB (SAT?)
//technically a bit less efficient than it could be, but who do I care
bool collide(const AABB &a, const Edge &b) {
	return collide(OBB(a.min, a.max), b);
}

//plane things~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//Plane vs. OBB
//SAT
bool collide(const OBB &a, const Triangle &b) {
	glm::vec3 axes[1];

	glm::vec3 a_vert[8];
	glm::vec3 const *b_vert;

	//Setup the world coord vertices (should probably be a function of the class)
	//not sure if this gives them in a reasonable order?
	a.getVerticies(a_vert);
	b_vert = b.points;

	//only the normal of the triangle can be an axis
	axes[0] = b.norm;

	bool collide = SAT_collide(axes, a_vert, b_vert, 1, 8, 3);
	if (!collide) { return false; }
	else {
		//got to check that the collision actually happened on the triangle
		return true; 
	}
}

//Plane vs. AABB
//SAT
bool collide(const AABB &a, const Triangle &b) {
	return collide(OBB(a.min, a.max), b);
}

//Plane vs. Convex Hull
bool collide(const ConvexHull &a, const Triangle &b) {
	return collide(a, ConvexHull((glm::vec3*)b.points, 3, glm::mat4(1.0)));
}

//AABB things~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

//AABB vs. AABB
//Simple Check
bool collide(const AABB &a, const AABB &b) {
	return (a.min.x <= b.max.x && a.max.x >= b.min.x) &&
		(a.min.y <= b.max.y && a.max.y >= b.min.y) &&
		(a.min.z <= b.max.z && a.max.z >= b.min.z);
}


//AABB vs. OBB
//SAT
bool collide(const OBB &a, const AABB &b) {
	return collide(a, OBB(b.min, b.max));
}

//AABB vs. Convex Hull
bool collide(const ConvexHull &a, const AABB &b) {
	glm::vec3 verts[8];
	b.getVerticies(verts);
	return collide(a, ConvexHull(verts, 8, glm::mat4(1.0)));
}
//OBB things~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

//OBB vs. OBB (SAT) (because I want to do it now, shuddup
//the first chunk of this is stuff

bool collide(const OBB &a, const OBB &b) {
	glm::vec3 axes[15];

	glm::vec3 a_vert[8];
	glm::vec3 b_vert[8];

	//Setup the world coord vertices
	a.getVerticies(a_vert);
	b.getVerticies(b_vert);


	//Setup the axes to test against: all of the face normals, plus their cross products
	for (int i = 0; i < 3; i++) {
		axes[i] = glm::mat3(a.trans)[i];//should make sure that this does work the way I think it does //(also, may need to normalize them)
	}
	for (int i = 0; i < 3; i++) {
		axes[i + 3] = glm::mat3(b.trans)[i];
	}
	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 3; j++) {
			axes[6 + 3 * i + j] = glm::cross(axes[i], axes[j]);
		}
	}
	
	return SAT_collide(axes, a_vert, b_vert, 15, 8, 8);
}

//OBB vs. Convex Hull
bool collide(const ConvexHull &a, const OBB &b) {
	glm::vec3 verts[8];
	b.getVerticies(verts);
	return collide(a, ConvexHull((glm::vec3*)verts, 8, b.trans));
}

bool collide(const ConvexHull &a, const ConvexHull &b) {
	return collision_GJK((IGJKFcns*)&a, (IGJKFcns*)&b);
}


//Drawing the colliders, for debug purpuses mostly
//This is very bad for a number of reasons:
// Generating the mesh, and reallocating storage, every time
// Compiling the shader, every time
// bad bad bad
void Sphere::draw(const glm::mat4 &vp) {
	using namespace std;

	GLuint vao;
	GLuint vbo;
	GLuint vbo_index;
	GLuint program;
	GLuint attrib_coord;
	GLuint attrib_index;
	GLuint uniform_matrix;
	GLuint uniform_colour;

	std::vector<GLfloat> vertices;
	std::vector<GLuint> indices;

	glm::mat4 mvp = vp*glm::scale(glm::vec3(rad, rad, rad))*glm::translate(loc);

	vertices.resize(0);
	indices.resize(0);

	int rings = 20;
	int sectors = 20;
	GLfloat radius = 1;

	int r, s;
	vertices.resize(rings * sectors * 3);
	std::vector<GLfloat>::iterator v = vertices.begin();

	const float M_PI = 3.14159265359;

	//generating a sphere mesh

	for (r = 0; r < rings; r++) for (s = 0; s < sectors; s++) {
		float y = -radius + radius*2.f * r / (rings - 1);
		float rad = sqrt(radius*radius - y*y);

		float x = rad*cos(2 * M_PI * s / sectors);
		float z = rad*sin(2 * M_PI * s / sectors);

		*v++ = x;
		*v++ = y;
		*v++ = z;
	}

	indices.resize(rings * sectors * 3 * 2);
	std::vector<GLuint>::iterator i = indices.begin();
	for (r = 0; r < rings - 1; r++) for (s = 0; s < sectors - 1; s++) {
		if (s == 10000) {

		}
		else {
			*i++ = r * sectors + s;
			*i++ = r * sectors + (s + 1);
			*i++ = (r + 1) * sectors + (s + 1);

			*i++ = (r + 1) * sectors + (s + 1);
			*i++ = (r + 1) * sectors + (s);
			*i++ = r * sectors + s;
		}
	}

	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices[0])*vertices.size(), vertices.data(), GL_STATIC_DRAW);

	glGenBuffers(1, &vbo_index);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo_index);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices[0])*indices.size(), indices.data(), GL_STATIC_DRAW);

	struct shaderInfo shaders[] = { { GL_VERTEX_SHADER, "vs_wireframe.glsl" },
	{ GL_FRAGMENT_SHADER, "fs_wireframe.glsl" } };

	program = LoadProgram(shaders, 2);
	glUseProgram(program);

	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	attrib_coord = getAttrib(program, "coord");
	glEnableVertexAttribArray(attrib_coord);
	glVertexAttribPointer(
		attrib_coord,	//attrib 'index'
		3,				//pieces of data per index
		GL_FLOAT,		//type of data
		GL_FALSE,		//whether to normalize
		0,				//space b\w data
		0				//offset from buffer start
		);

	uniform_matrix = getUniform(program, "mvp");
	glUniformMatrix4fv(uniform_matrix, 1, GL_FALSE, glm::value_ptr(mvp));

	uniform_colour = getUniform(program, "colour");
	glUniform3fv(uniform_colour, 1, glm::value_ptr(colour));

	int size;  glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &size);

	glDrawElements(GL_TRIANGLES, size / sizeof(GLuint), GL_UNSIGNED_INT, NULL);

	glDeleteBuffers(1, &vbo);
	glDeleteBuffers(1, &vbo_index);
	glDeleteVertexArrays(1, &vao);
	glDeleteProgram(program);
}

void AABB::draw(const glm::mat4 &mvp) {
	using namespace std;

	GLuint vao;
	GLuint vbo;
	GLuint vbo_index;
	GLuint program;
	GLuint attrib_coord;
	GLuint attrib_index;
	GLuint uniform_matrix;
	GLuint uniform_colour;

	GLfloat vertices[] = {
		min.x, min.y, min.z,
		min.x, max.y, min.z,
		max.x, max.y, min.z,
		max.x, min.y, min.z,
		min.x, min.y, max.z,
		min.x, max.y, max.z,
		max.x, max.y, max.z,
		max.x, min.y, max.z,
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


	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glGenBuffers(1, &vbo_index);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo_index);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(elements), elements, GL_STATIC_DRAW);

	struct shaderInfo shaders[] = { { GL_VERTEX_SHADER, "vs_wireframe.glsl" },
	{ GL_FRAGMENT_SHADER, "fs_wireframe.glsl" } };

	program = LoadProgram(shaders, 2);
	glUseProgram(program);

	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	attrib_coord = getAttrib(program, "coord");
	glEnableVertexAttribArray(attrib_coord);
	glVertexAttribPointer(
		attrib_coord,	//attrib 'index'
		3,				//pieces of data per index
		GL_FLOAT,		//type of data
		GL_FALSE,		//whether to normalize
		0,				//space b\w data
		0				//offset from buffer start
		);

	uniform_matrix = getUniform(program, "mvp");
	glUniformMatrix4fv(uniform_matrix, 1, GL_FALSE, glm::value_ptr(mvp));

	uniform_colour = getUniform(program, "colour");

	glUniform3fv(uniform_colour, 1, glm::value_ptr(colour));

	int size;  glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &size);

	glDrawElements(GL_LINES, size / sizeof(GLuint), GL_UNSIGNED_INT, NULL);

	glDeleteBuffers(1, &vbo);
	glDeleteBuffers(1, &vbo_index);
	glDeleteVertexArrays(1, &vao);
	glDeleteProgram(program);
}

void OBB::draw(const glm::mat4 &vp) {
	using namespace std;

	GLuint vao;
	GLuint vbo;
	GLuint vbo_index;
	GLuint program;
	GLuint attrib_coord;
	GLuint attrib_index;
	GLuint uniform_matrix;
	GLuint uniform_colour;

	glm::mat4 mvp = vp*trans;

	GLfloat vertices[] = {
		min.x, min.y, min.z,
		min.x, max.y, min.z,
		max.x, max.y, min.z,
		max.x, min.y, min.z,
		min.x, min.y, max.z,
		min.x, max.y, max.z,
		max.x, max.y, max.z,
		max.x, min.y, max.z,
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

	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glGenBuffers(1, &vbo_index);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo_index);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(elements), elements, GL_STATIC_DRAW);

	struct shaderInfo shaders[] = { { GL_VERTEX_SHADER, "vs_wireframe.glsl" },
	{ GL_FRAGMENT_SHADER, "fs_wireframe.glsl" } };

	program = LoadProgram(shaders, 2);
	glUseProgram(program);

	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	attrib_coord = getAttrib(program, "coord");
	glEnableVertexAttribArray(attrib_coord);
	glVertexAttribPointer(
		attrib_coord,	//attrib 'index'
		3,				//pieces of data per index
		GL_FLOAT,		//type of data
		GL_FALSE,		//whether to normalize
		0,				//space b\w data
		0				//offset from buffer start
		);

	uniform_matrix = getUniform(program, "mvp");
	glUniformMatrix4fv(uniform_matrix, 1, GL_FALSE, glm::value_ptr(mvp));

	uniform_colour = getUniform(program, "colour");
	glUniform3fv(uniform_colour, 1, glm::value_ptr(colour));

	int size;  glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &size);

	glDrawElements(GL_LINES, size / sizeof(GLuint), GL_UNSIGNED_INT, NULL);

	glDeleteBuffers(1, &vbo);
	glDeleteBuffers(1, &vbo_index);
	glDeleteVertexArrays(1, &vao);
	glDeleteProgram(program);
}

void Triangle::draw(const glm::mat4 &vp) {
	using namespace std;

	GLuint vao;
	GLuint vbo;
	GLuint vbo_index;
	GLuint program;
	GLuint attrib_coord;
	GLuint attrib_index;
	GLuint uniform_matrix;
	GLuint uniform_colour;

	GLfloat vertices[] = {
		points[0].x, points[0].y, points[0].z,
		points[1].x, points[1].y, points[1].z,
		points[2].x, points[2].y, points[2].z,
	};


	//for lines
	GLuint elements[] = {
		0, 1,
		1, 2,
		2, 0,
	};

	//for fill
	/*
	GLuint elements[] = {
		0, 1, 2,
	};
	*/

	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glGenBuffers(1, &vbo_index);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo_index);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(elements), elements, GL_STATIC_DRAW);

	struct shaderInfo shaders[] = { { GL_VERTEX_SHADER, "vs_wireframe.glsl" },
	{ GL_FRAGMENT_SHADER, "fs_wireframe.glsl" } };

	program = LoadProgram(shaders, 2);
	glUseProgram(program);

	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	attrib_coord = getAttrib(program, "coord");
	glEnableVertexAttribArray(attrib_coord);
	glVertexAttribPointer(
		attrib_coord,	//attrib 'index'
		3,				//pieces of data per index
		GL_FLOAT,		//type of data
		GL_FALSE,		//whether to normalize
		0,				//space b\w data
		0				//offset from buffer start
		);

	uniform_matrix = getUniform(program, "mvp");
	glUniformMatrix4fv(uniform_matrix, 1, GL_FALSE, glm::value_ptr(vp));

	uniform_colour = getUniform(program, "colour");
	glUniform3fv(uniform_colour, 1, glm::value_ptr(colour));

	int size;  glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &size);

	glDrawElements(GL_LINES, size / sizeof(GLuint), GL_UNSIGNED_INT, NULL);

	glDeleteBuffers(1, &vbo);
	glDeleteBuffers(1, &vbo_index);
	glDeleteVertexArrays(1, &vao);
	glDeleteProgram(program);
}
void Edge::draw(const glm::mat4 &vp) {
	using namespace std;

	GLuint vao;
	GLuint vbo;
	GLuint vbo_index;
	GLuint program;
	GLuint attrib_coord;
	GLuint attrib_index;
	GLuint uniform_matrix;
	GLuint uniform_colour;

	GLfloat vertices[] = {
		points[0].x, points[0].y, points[0].z,
		points[1].x, points[1].y, points[1].z,
	};

	GLuint elements[] = {
		0, 1,
	};


	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glGenBuffers(1, &vbo_index);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo_index);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(elements), elements, GL_STATIC_DRAW);

	struct shaderInfo shaders[] = { { GL_VERTEX_SHADER, "vs_wireframe.glsl" },
	{ GL_FRAGMENT_SHADER, "fs_wireframe.glsl" } };

	program = LoadProgram(shaders, 2);
	glUseProgram(program);

	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	attrib_coord = getAttrib(program, "coord");
	glEnableVertexAttribArray(attrib_coord);
	glVertexAttribPointer(
		attrib_coord,	//attrib 'index'
		3,				//pieces of data per index
		GL_FLOAT,		//type of data
		GL_FALSE,		//whether to normalize
		0,				//space b\w data
		0				//offset from buffer start
		);

	uniform_matrix = getUniform(program, "mvp");
	glUniformMatrix4fv(uniform_matrix, 1, GL_FALSE, glm::value_ptr(vp));

	uniform_colour = getUniform(program, "colour");
	glUniform3fv(uniform_colour, 1, glm::value_ptr(colour));

	int size;  glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &size);

	glDrawElements(GL_LINES, size / sizeof(GLuint), GL_UNSIGNED_INT, NULL);

	glDeleteBuffers(1, &vbo);
	glDeleteBuffers(1, &vbo_index);
	glDeleteVertexArrays(1, &vao);
	glDeleteProgram(program);
}

//prolly don't ever use this one, until it's actually efficient
void ConvexHull::draw(const glm::mat4 &vp) {
	using namespace std;

	GLuint vao;
	GLuint vbo;
	GLuint vbo_index;
	GLuint program;
	GLuint attrib_coord;
	GLuint attrib_index;
	GLuint uniform_matrix;
	GLuint uniform_colour;

	glm::mat4 mvp = vp*trans;

	GLfloat *vertices = new GLfloat[num * 3];

	for (int i = 0; i < num; i++) {
		vertices[3 * i + 0] = points[i].x;
		vertices[3 * i + 1] = points[i].y;
		vertices[3 * i + 2] = points[i].z;
	}


	//derpaderp
	GLuint elements[] = {
		0, 1,
	};


	std::cout << "size: " << sizeof(vertices);
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glGenBuffers(1, &vbo_index);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo_index);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(elements), elements, GL_STATIC_DRAW);

	struct shaderInfo shaders[] = { { GL_VERTEX_SHADER, "vs_wireframe.glsl" },
	{ GL_FRAGMENT_SHADER, "fs_wireframe.glsl" } };

	program = LoadProgram(shaders, 2);
	glUseProgram(program);

	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	attrib_coord = getAttrib(program, "coord");
	glEnableVertexAttribArray(attrib_coord);
	glVertexAttribPointer(
		attrib_coord,	//attrib 'index'
		3,				//pieces of data per index
		GL_FLOAT,		//type of data
		GL_FALSE,		//whether to normalize
		0,				//space b\w data
		0				//offset from buffer start
		);

	uniform_matrix = getUniform(program, "mvp");
	glUniformMatrix4fv(uniform_matrix, 1, GL_FALSE, glm::value_ptr(mvp));

	uniform_colour = getUniform(program, "colour");
	glUniform3fv(uniform_colour, 1, glm::value_ptr(colour));

	int size;  glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &size);

	glDrawElements(GL_LINES, size / sizeof(GLuint), GL_UNSIGNED_INT, NULL);

	glDeleteBuffers(1, &vbo);
	glDeleteBuffers(1, &vbo_index);
	glDeleteVertexArrays(1, &vao);
	glDeleteProgram(program);
}

//Double dispatch stuff
collideResult Sphere::intersect(Collidable &a) {
	return a.intersect(*this);
}
collideResult Sphere::intersect(Sphere &a) {
	return collide(*this, a);
}
collideResult Sphere::intersect(OBB &a) {
	return collide(*this, a);
}
collideResult Sphere::intersect(ConvexHull &a) {
	return collide(*this, a);
}
collideResult Sphere::intersect(Capsule &a) {
	return collide(a, *this);
}

collideResult OBB::intersect(Collidable &a) {
	return a.intersect(*this);
}
collideResult OBB::intersect(Sphere &a) {
	return collide(a, *this);
}
collideResult OBB::intersect(OBB &a) {
	return collide(*this, a);
}
collideResult OBB::intersect(ConvexHull &a) {
	return collide(a, *this);
}
collideResult OBB::intersect(Capsule &a) {
	return collide(a, *this);
}

collideResult ConvexHull::intersect(Collidable &a) {
	return a.intersect(*this);
}
collideResult ConvexHull::intersect(Sphere &a) {
	return collide(a, *this);
}
collideResult ConvexHull::intersect(OBB &a) {
	return collide(*this, a);
}
collideResult ConvexHull::intersect(ConvexHull &a) {
	return collide(a, *this);
}
collideResult ConvexHull::intersect(Capsule &a) {
	return collide(a, *this);
}

collideResult Capsule::intersect(Collidable &a) {
	return a.intersect(*this);
}
collideResult Capsule::intersect(Sphere &a) {
	return collide(*this, a);
}
collideResult Capsule::intersect(OBB &a) {
	return collide(*this, a);
}
collideResult Capsule::intersect(ConvexHull &a) {
	return collide(*this, a);
}
collideResult Capsule::intersect(Capsule &a) {
	return collide(*this, a);
}

///CAPSULE AFTERTHOUGHR########################################################################


Capsule::Capsule(glm::vec3 a, glm::vec3 b, GLfloat radius) : radius(radius) {
	this->points[1] = b;
	this->points[0] = a;
}
Capsule::Capsule(GLfloat length, GLfloat radius) : radius(radius) {
	points[0] = glm::vec3(0, -length / 2, 0);
	points[1] = glm::vec3(0, length / 2, 0);
}

void Capsule::draw(const glm::mat4 &vp) {
	//I'm so sorry
	Sphere *balls[2];

	balls[0] = new Sphere(points[0], radius);
	balls[1] = new Sphere(points[1], radius);

	balls[0]->draw(vp);
	balls[1]->draw(vp);

	delete balls[0];
	delete balls[1];
}

void Capsule::translate(glm::vec3 diff) {
	points[0] += diff;
	points[1] += diff;
}

glm::vec3 Capsule::furthestPointInDirection(glm::vec3 dir) {
	glm::vec3 capDir = points[1] - points[0];

	float res = glm::dot(capDir, dir) == 0;

	if (res > 0) {
		points[1] + radius*(glm::normalize(dir));
	}
	else {
		return points[0] + radius*(glm::normalize(dir));
	}
}

//should clean all this up
bool collide(const Capsule &c_a, const Capsule &c_b)
{
	glm::vec3   u = c_a.points[1] - c_a.points[0];
	glm::vec3   v = c_b.points[1] - c_b.points[0];
	glm::vec3   w = c_a.points[0] - c_b.points[0];
	float    a = glm::dot(u, u);         // always >= 0
	float    b = glm::dot(u, v);
	float    c = glm::dot(v, v);         // always >= 0
	float    d = glm::dot(u, w);
	float    e = glm::dot(v, w);
	float    D = a*c - b*b;        // always >= 0
	float    sc, tc;

	// compute the line parameters of the two closest points
	if (D < 0.00000001) {          // the lines are almost parallel
		sc = 0.0;
		tc = (b>c ? d / b : e / c);    // use the largest denominator
	}
	else {
		sc = (b*e - c*d) / D;
		tc = (a*e - b*d) / D;
	}

	glm::vec3 p_a;
	if (sc < 0) {
		p_a = c_a.points[0];
	}
	else if (sc > 1) {
		p_a = c_a.points[1];
	}
	else {
		p_a = c_a.points[0] + sc*u;
	}

	glm::vec3 p_b;

	if (tc < 0) {
		p_b = c_b.points[0];
	}
	else if (tc > 1) {
		p_b = c_b.points[1];
	}
	else {
		p_b = c_b.points[0] + tc*v;
	}


	glm::vec3 dist = p_a - p_b;

	float rad = c_a.radius + c_b.radius;

	return (dist.x*dist.x + dist.y*dist.y + dist.z*dist.z) < (rad)*(rad);
}
bool collide(const Capsule &e, const Sphere &s) {
	//maybe should store these instead of calcing each time?
	glm::vec3 norm = glm::normalize(e.points[1] - e.points[0]);
	float length = glm::length(e.points[1] - e.points[0]);

	float c = glm::dot((s.loc - e.points[0]), norm);//the distance to nearest point on edge, from point[0] of the line
	glm::vec3 p;

	if (c < 0) {
		//the closest point to the circle doesn't lie within the edge
		//still, check if nearest edge points are in it
		p = e.points[0];
	}
	else if (c > length) {
		//same as above
		p = e.points[1];
	}
	else {
		p = c*norm + e.points[0];//the nearest point on the edge
	}

	glm::vec3 d = s.loc - p;//the vec b/w sphere and nearest point

	float rad = s.rad + e.radius;

	return (d.x*d.x + d.y*d.y + d.z*d.z) < (rad)*(rad);
}
bool collide(const Capsule &a, const OBB &b) {
	glm::vec3 axes[6];

	glm::vec3 b_vert[8];

	//Setup the world coord vertices
	b.getVerticies(b_vert);

	glm::vec3 norm = glm::normalize(a.points[1] - a.points[0]);

	//Setup the axes to test against: all of the face normals, plus their cross products
	for (int i = 0; i < 3; i++) {
		axes[i] = glm::mat3(b.trans)[i];
	}
	for (int i = 0; i < 3; i++) {
		axes[i + 3] = glm::cross(axes[i], norm);
	}

	float a_min, a_max;
	float b_min, b_max;

	int axes_num = 6;
	int b_num = 8;

	for (int i = 0; i < axes_num; i++) {

		//projecting A onto the axis
		a_min = glm::dot(a.points[0], axes[i]);
		a_max = glm::dot(a.points[1], axes[i]);

		if (a_min > a_max) {
			float temp = a_min;
			a_min = a_max;
			a_max = temp;
		}
		a_min -= a.radius;
		a_max += a.radius;

		//projecting B onto the axis
		b_min = b_max = glm::dot(b_vert[0], axes[i]);
		for (int j = 1; j < b_num; j++) {
			float temp = glm::dot(b_vert[j], axes[i]);
			if (temp < b_min) {
				b_min = temp;
			}
			if (temp > b_max) {
				b_max = temp;
			}
		}

		//testing for overlap
		if (a_max > b_min && a_min < b_max) {
			continue;//dumb and pointless
		}
		else {
			return false;// no overlap, therefor, we have a separating plane
		}

	}

	return true;
}
bool collide(const Capsule &a, const ConvexHull &b) {
	return collision_GJK((IGJKFcns*)&a, (IGJKFcns*)&b);
}
#endif