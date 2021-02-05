#include "Mesh.h"

#include <iostream>
#include <fstream>
#include <sstream>

#include "glm/ext.hpp"
#include "glm/gtx/extended_min_max.hpp"

#include "Asset.h"
#include "BVHTree.h"

#include <set>

Mesh::Mesh( const std::string& fname )
	: m_name(fname)
	, m_vertices()
	, m_faces()
	, m_boundingBox()
{
	std::string code;
	double vx, vy, vz;

	std::string fpath = searchForAsset(fname);

	std::ifstream ifs( fpath.c_str() );
	if (!ifs) {
		std::cout << "file failed to load: " << fname << std::endl;
	}

	while( ifs >> code ) {
		if( code == "v" ) {
			ifs >> vx >> vy >> vz;
			m_vertices.push_back( glm::vec3( vx, vy, vz ) );

		} else if( code == "vt" ) {
			ifs >> vx >> vy;
			m_uvs.push_back(glm::vec2(vx, vy));

		} else if( code == "f" ) { //todo: clean up names
			std::vector<size_t> verts[3];
			
			for (auto &v : verts ) {
				std::string vertString;
				ifs >> vertString;
				std::stringstream vs(vertString);
				
				while(vs) {
					v.push_back(0);
					vs >> v.back();
					vs.ignore(1, '/');
				}
			}
			
			assert(verts[0].size() == verts[1].size() && verts[1].size() == verts[2].size());
			assert(verts[0].size() > 0);

			if(verts[0].size() > 0 ) {
				m_faces.push_back( TrianglePrimitive( verts[0][0] - 1, verts[1][0] - 1, verts[2][0] - 1 , m_vertices) );
			}
			if (verts[0].size() > 1) {
				m_uvMap.push_back( Triangle( verts[0][1] - 1, verts[1][1] - 1, verts[2][1] - 1) );
			}
		}
	}

	// generate bounds
	generateBounds();

#ifndef DISABLE_MESH_BVH
	{
		ScopedTimer t("Generating BVH for " + fname, 5);
		m_bvhTreePtr = new BVHTree(m_faces);
	}
#else
	std::cout << ("BVH Generation Disabled for " + fname) << std::endl;
#endif

#ifdef PHONG_SHADING
	// generate normals if none are provided, this will effectively give all meshes phong shading
	if (m_normals.empty()) {
		generateNormals();
	}
#endif

	generateTangents();
}

Mesh::~Mesh() {
#ifndef DISABLE_MESH_BVH
delete m_bvhTreePtr;
#endif
}

std::ostream& operator<<(std::ostream& out, const Mesh& mesh)
{
  out << "mesh {";
  /*
  
  for( size_t idx = 0; idx < mesh.m_verts.size(); ++idx ) {
  	const MeshVertex& v = mesh.m_verts[idx];
  	out << glm::to_string( v.m_position );
	if( mesh.m_have_norm ) {
  	  out << " / " << glm::to_string( v.m_normal );
	}
	if( mesh.m_have_uv ) {
  	  out << " / " << glm::to_string( v.m_uv );
	}
  }

*/
  out << "}";
  return out;
}

inline bool triIntersect(IntersectResult &out_result,
						 const glm::vec3 &p1, const glm::vec3 &p2, const glm::vec3 &p3,
						 const glm::vec3 &r, const glm::vec3 &l,
						 double min_t, double max_t) {		
	glm::vec3 e1 = p2 - p1;
	glm::vec3 e2 = p3 - p2;
	glm::vec3 e3 = p1 - p3;
	
	glm::vec3 n = glm::cross(e1, -e3);

	double denominator = glm::dot(l, n);
	if( denominator == 0) {  // todo: epsilon
		return false;
	}

	double t = glm::dot((p1 - r), n) / denominator;

	// distance check
	if (t < min_t || t > max_t ) {
		return false;
	}

	glm::vec3 i = r + float(t)*l;

	// aperture test // todo: do containment test first
	double u = glm::dot(glm::cross(e2, i-p2), n);
	double v = glm::dot(glm::cross(e3, i-p3), n);
	double w = glm::dot(glm::cross(e1, i-p1), n);

	if ( !(u >= 0 && v >= 0 && w >= 0) &&
			!(u <= 0 && v <= 0 && w <= 0)) {
		return false;
	}
	
	float ln_squared = glm::dot(n, n);
	u /= ln_squared;
	v /= ln_squared;
	w /= ln_squared;

	out_result.surface_uv = glm::vec2(u, v);
	
	out_result.tangent = glm::cross(n, e2);
	out_result.bitangent = glm::cross(n, e3);

	out_result.position = i;
	out_result.normal = glm::normalize(n);
	out_result.t = t;

	return true;
}

// ------------------------------------------------------------------------------------------
//
void Mesh::generateBounds() {
	if (m_vertices.empty()) {
		m_boundingBox = NonhierBox();
	}
	glm::vec3 min(std::numeric_limits<float>::infinity());
	glm::vec3 max(-std::numeric_limits<float>::infinity());

	for (const glm::vec3 &v : m_vertices) {
		min = glm::min(min, v);
		max = glm::max(max, v);
	}

	glm::vec3 size = (max - min);
	glm::vec3 pos = min;

	m_boundingBox = NonhierBox(pos, size);
}

void Mesh::generateNormals() {
	m_normals.resize(m_vertices.size(), glm::vec3(0,0,0));
	std::vector<int> influence(m_vertices.size(), 0);

	for (const TrianglePrimitive &f : m_faces) {
		const glm::vec3 &p1 = m_vertices[f.v1];
		const glm::vec3 &p2 = m_vertices[f.v2];
		const glm::vec3 &p3 = m_vertices[f.v3]; 

		glm::vec3 e1 = p2 - p1;
		glm::vec3 e2 = p3 - p2;
		glm::vec3 e3 = p1 - p3;

		glm::vec3 n = glm::normalize(glm::cross(e1, -e3));

		for (int vi : {f.v1, f.v2, f.v3}) {
			influence[vi]++;
			m_normals[vi] += (n - m_normals[vi]) / float(influence[vi]);
		}
	}

	for (int i = 0; i < m_normals.size(); i++) {
		m_normals[i] = glm::normalize(m_normals[i]);
	}
}

void Mesh::generateTangents() {
	m_tangents.resize(m_uvs.size());
	m_bitangents.resize(m_uvs.size());

	std::vector<int> influence(m_uvs.size(), 0);

	for (int i = 0; i < m_uvMap.size(); i++) {
		const auto &fuv = m_uvMap[i];
		const auto &f = m_faces[i];

		glm::vec3 e1 = f.a2() - f.a1();
		glm::vec3 e2 = f.a3() - f.a2();
		glm::vec3 e3 = f.a1() - f.a3();

		glm::vec2 euv1 = m_uvs[fuv.v2] - m_uvs[fuv.v1];
		glm::vec2 euv2 = m_uvs[fuv.v3] - m_uvs[fuv.v2];
		glm::vec2 euv3 = m_uvs[fuv.v1] - m_uvs[fuv.v3];

		glm::mat2x2 uvmat{euv1, -euv3};
		glm::mat2x3 emat{e1, -e3};

		glm::mat2x3 tangentmat = emat * glm::inverse(uvmat);

		glm::vec3 tangent = tangentmat[0];
		glm::vec3 bitangent = tangentmat[1];

		for (int vi : {fuv.v1, fuv.v2, fuv.v3}) {
			influence[vi]++;
			m_tangents[vi] += (tangent - m_tangents[vi]) / float(influence[vi]);
			m_bitangents[vi] += (bitangent - m_bitangents[vi]) / float(influence[vi]);
		}

	}

	// todo: maybe normalize on upload instead .. so it can be used in the raytracer better...
	for (int i = 0; i < m_uvs.size(); i++) {
		m_tangents[i] = glm::normalize(m_tangents[i]);
		m_bitangents[i] = glm::normalize(m_bitangents[i]);
	}
}

// ------------------------------------------------------------------------------------------
//
bool Mesh::intersect(IntersectResult &out_result, const Ray &r, float min_t, float max_t) const {
	bool hasintersection = false;
	const TrianglePrimitive* hitTri = nullptr;

	// first test bounding box
	IntersectResult boundsIntersection;
	if (!m_boundingBox.intersect(boundsIntersection, r, min_t, max_t)) {
		return false;
	}

#ifdef RENDER_BOUNDING_VOLUMES
	// use bounding box intersection as result
	out_result = boundsIntersection;
	return true;
#elif ! defined(DISABLE_MESH_BVH)
	const Primitive* hitPrimitive = nullptr;
	hasintersection = m_bvhTreePtr->intersect(out_result, hitPrimitive, r, min_t, max_t);
	hitTri = dynamic_cast<const TrianglePrimitive*>(hitPrimitive);
#else
	// test all triangles
	for (const TrianglePrimitive& tri : m_faces) {
		if (tri.intersect(out_result, r, min_t, max_t) ) {
			hitTri = &tri;
			hasintersection = true;
			max_t = out_result.t;
		}
	}
#endif
	if(hitTri) {
		// interpolate values for the triangle
		uint32_t i = hitTri - &m_faces[0];
		const glm::vec2 &tri_uv = out_result.surface_uv;
		glm::vec3 tri_uvw = glm::vec3(tri_uv.x, tri_uv.y, 1 - tri_uv.x - tri_uv.y);

		if (!m_normals.empty()) {
			const glm::vec3 &n1 = m_normals[hitTri->v1];
			const glm::vec3 &n2 = m_normals[hitTri->v2];
			const glm::vec3 &n3 = m_normals[hitTri->v3];

			out_result.normal = n1*tri_uvw.x + n2*tri_uvw.y + n3*tri_uvw.z;
		}

		if (!m_uvs.empty()) {
			const Triangle &uvFace = m_uvMap[i];
			const glm::vec2 &u1 = m_uvs[uvFace.v1];
			const glm::vec2 &u2 = m_uvs[uvFace.v2];
			const glm::vec2 &u3 = m_uvs[uvFace.v3];

			out_result.surface_uv = u1*tri_uvw.x + u2*tri_uvw.y + u3*tri_uvw.z;
		}

		// todo: can't use stored tangents, since they're normalized
		if (false && !m_tangents.empty() && !m_bitangents.empty()) {
				const Triangle& uvFace = m_uvMap[i];
				const glm::vec3& t1 = m_tangents[uvFace.v1];
				const glm::vec3& t2 = m_tangents[uvFace.v2];
				const glm::vec3& t3 = m_tangents[uvFace.v3];

				out_result.tangent = t1 * tri_uvw.x + t2 * tri_uvw.y + t3 * tri_uvw.z;

				const glm::vec3& b1 = m_bitangents[uvFace.v1];
				const glm::vec3& b2 = m_bitangents[uvFace.v2];
				const glm::vec3& b3 = m_bitangents[uvFace.v3];

				out_result.bitangent = b1 * tri_uvw.x + b2 * tri_uvw.y + b3 * tri_uvw.z;
		}
		else if (!m_uvs.empty())
		{
				// todo: move duplicate code into function
				// generate based on uvs for this face
				const auto& fuv = m_uvMap[i];
				const auto& f = m_faces[i];

				glm::vec3 e1 = f.a2() - f.a1();
				glm::vec3 e2 = f.a3() - f.a2();
				glm::vec3 e3 = f.a1() - f.a3();

				glm::vec2 euv1 = m_uvs[fuv.v2] - m_uvs[fuv.v1];
				glm::vec2 euv2 = m_uvs[fuv.v3] - m_uvs[fuv.v2];
				glm::vec2 euv3 = m_uvs[fuv.v1] - m_uvs[fuv.v3];

				glm::mat2x2 uvmat{ euv1, -euv3 };
				glm::mat2x3 emat{ e1, -e3 };

				glm::mat2x3 tangentmat = emat * glm::inverse(uvmat);

				out_result.tangent = tangentmat[0];
				out_result.bitangent = tangentmat[1];
		}

	}
	return hasintersection;
}

NonhierBox Mesh::getBounds() const {
	return m_boundingBox;
}


bool Mesh::TrianglePrimitive::intersect(IntersectResult &out_result, const Ray &r, float min_t, float max_t) const {
	return triIntersect(out_result, this->a1(), this->a2(), this->a3(), r.o, r.l, min_t, max_t);
};

NonhierBox Mesh::TrianglePrimitive::getBounds() const {
	glm::vec3 min = glm::min(a1(), a2(), a3());
	glm::vec3 max = glm::max(a1(), a2(), a3());
	return NonhierBox(min, max - min);
}
