#pragma once

#include <vector>
#include <iosfwd>
#include <string>

#include <glm/glm.hpp>
#include <glm/gtx/common.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/norm.hpp>

#include "Primitive.h"
#include "RenderRaytracer.h"

// Use this #define to selectively compile your code to render the
// bounding boxes around your mesh objects. Uncomment this option
// to turn it on.
//#define RENDER_BOUNDING_VOLUMES

// Use this #define to disable the BVH tree inside the triangle;
// intersection test will be done by testing all triangles
//#define DISABLE_MESH_BVH

// Use this #define to enable phong-shading for all meshes by default
//todo: use separate member in intersect result for shading normal and actual normal?
//#define PHONG_SHADING

// Forward Declares
class BVHTree;

// Structs ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
struct Triangle
{
	size_t v1;
	size_t v2;
	size_t v3;

	Triangle( size_t pv1, size_t pv2, size_t pv3)
		: v1( pv1 )
		, v2( pv2 )
		, v3( pv3 )
	{}
};

template<typename T>
struct AttributeTriangle {
	size_t v1;
	size_t v2;
	size_t v3;

	std::vector<T> &vertAttributes;

	AttributeTriangle( size_t pv1, size_t pv2, size_t pv3, std::vector<T> &vertices) :
		v1(pv1), v2(pv2), v3(pv3), vertAttributes(vertices) { };

	inline T& a1() { return vertAttributes[v1]; }
	inline T& a2() { return vertAttributes[v2]; }
	inline T& a3() { return vertAttributes[v3]; }

	inline const T& a1() const { return vertAttributes[v1]; }
	inline const T& a2() const { return vertAttributes[v2]; }
	inline const T& a3() const { return vertAttributes[v3]; }
};

inline glm::vec3 getNormal(const glm::vec3 &p1, const glm::vec3 &p2, const glm::vec3 &p3) {
	glm::vec3 e1 = p2 - p1;
	glm::vec3 e2 = p3 - p2;
	glm::vec3 e3 = p1 - p3;

	return glm::cross(e1, -e3);
}

// A polygonal mesh.
class Mesh : public Primitive {
public:
  Mesh( const std::string& fname );
  ~Mesh();

	virtual bool intersectSphere(IntersectResult& out_result, const glm::vec3 &pos, float radius) const override { 
		for( const auto &tri : m_faces) {
			if(tri.intersectSphere(out_result, pos, radius)) {
				return true;
			}
		}
		return false;
	}
	
	struct TrianglePrimitive : public Primitive, public AttributeTriangle<glm::vec3>
	{
		TrianglePrimitive( size_t pv1, size_t pv2, size_t pv3, std::vector<glm::vec3> &vertices)
			: Primitive(), AttributeTriangle(pv1, pv2, pv3, vertices) {}

		bool intersect(IntersectResult &out_result, const Ray &r, float min_t, float max_t) const override;
		
		virtual bool intersectSphere(IntersectResult& out_result, const glm::vec3 &pos, float radius) const override { 
			//based on: http://citeseerx.ist.psu.edu/viewdoc/download?doi=10.1.1.49.9172&rep=rep1&type=pdf

			NonhierSphere sphere(pos, radius); 
			IntersectResult &tempResult = out_result;

			glm::vec3 e1 = a2() - a1();
			glm::vec3 e2 = a3() - a2();
			glm::vec3 e3 = a1() - a3();

			// test intersection of edges
			if (sphere.intersect(tempResult, Ray{a1(), e1}, 0.f, 1.f)) { return true; }
			if (sphere.intersect(tempResult, Ray{a3(), e3}, 0.f, 1.f)) { return true; }
			if (sphere.intersect(tempResult, Ray{a2(), e2}, 0.f, 1.f)) { return true; }

			// test if points lie within sphere
			float r2 = radius * radius;
			if (glm::length2(a1() - pos) < r2) { return true; }
			if (glm::length2(a2() - pos) < r2) { return true; }
			if (glm::length2(a3() - pos) < r2) { return true; }

			// test if sphere projects onto triangle
			glm::vec3 n = glm::normalize(getNormal(a1(), a2(), a3()));
			float nDist = glm::dot(n, a1() - pos);
			if (glm::abs(nDist) > radius) { return false; }
			glm::vec3 i = pos + n *nDist;

			// aperture test
			double u = glm::dot(glm::cross(e2, i-a2()), n);
			double v = glm::dot(glm::cross(e3, i-a3()), n);
			double w = glm::dot(glm::cross(e1, i-a1()), n);

			if ( !(u >= 0 && v >= 0 && w >= 0) &&
					!(u <= 0 && v <= 0 && w <= 0)) {
				return false;
			}
			return true;
		}

		NonhierBox getBounds() const override;
	};
  
public:
	std::string m_name;

	// per vertex
	std::vector<glm::vec3> m_vertices;
	std::vector<glm::vec3> m_normals;

	// per uv-vertex (to allow for splits in the map, etc. )
	std::vector<glm::vec2> m_uvs;
	std::vector<glm::vec3> m_tangents;
	std::vector<glm::vec3> m_bitangents;

	// stimultaneous arrays
	std::vector<TrianglePrimitive> m_faces;
	std::vector<Triangle> m_uvMap;	// one per face

#ifndef DISABLE_MESH_BVH
	BVHTree* m_bvhTreePtr = nullptr;
#endif

	NonhierBox m_boundingBox;

	void generateBounds();
	void generateNormals();
	void generateTangents();

    friend std::ostream& operator<<(std::ostream& out, const Mesh& mesh);

	bool intersect(IntersectResult &out_result, const Ray &r, float min_t, float max_t) const override;
	NonhierBox getBounds() const override;

};