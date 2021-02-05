// Fall 2019

#define _USE_MATH_DEFINES 

#include "Primitive.h"

#include <vector>
#include <cmath>

#include <glm/glm.hpp>
#include <glm/gtx/scalar_multiplication.hpp>

#include "Scene.h"
#include "RenderRaytracer.h"

Primitive::~Primitive()
{
}


//todo: fast rayIntersectSphere + intersectBox that doesn't write out all shading info
bool rayIntersectSphere(IntersectResult &out_result, 
        const Ray &r, float min_t, float max_t,
        const glm::vec3 &pos, double rad) {
  // p = o + t*l
  // (o + t*l - c) . (o + t*l - c) = r^2
  // (t*l + (o - c)) . (t*l + (o - c)) = r^2
  // (t*l)^2 + 2*(t*l . (o-c)) + (o-c)^2 - r^2
  // t^2 (l.l) + 2*t*(l.(o-c)) + (o-c)^2 - r^2 = 0

  // x == t
  // a == l.l
  // b == 2 * (l . (o-c))
  // c == (o-c) . (o-c) - r^2

  // det = b*b - 4*a*c
  // t ==  (-b +- sqrt(det)) / 2 / a
  
  glm::vec3 diff =  r.o - pos;

  double a = glm::dot(r.l, r.l); 
  double b = 2 * glm::dot(r.l, diff);
  double c = glm::dot(diff, diff) - rad*rad;
  
  double det = b*b - 4*a*c;

  if (det < 0) { // no collision
    return false;
  }

  double t1 = (-b - sqrt(det)) / 2 / a;
  double t2 = (-b + sqrt(det)) / 2 / a;

  if (t1 > t2) {
    std::swap(t1, t2);
  }
  
  double t;
  if (t1 >= min_t && t1 <= max_t) {
    t = t1;
  } else if (t2 >= min_t && t2 <= max_t) {
    t = t2;
  } else {
    return false;
  }

  glm::vec3 hit_pos = r.o + float(t) * r.l;
  glm::vec3 norm = (hit_pos - pos) / float(rad);

  out_result = IntersectResult();
  out_result.position = hit_pos;
  out_result.normal = norm;
  out_result.t = t;

  glm::vec3 local_hit = hit_pos - pos;

  // uvs
  out_result.surface_uv = glm::vec2( 
    glm::mod(std::atan2(local_hit.x, local_hit.z) - M_PI_2 + 2 * M_PI, 2 * M_PI)  / 2.0 / M_PI,
    glm::acos(glm::dot(norm, glm::vec3(0, -1, 0))) / M_PI
  );

  // rotate in the xz plane 90 degrees to get tangent vec
  // length of vector will be radius of circle, we want it to be circumference, so multiply length ny 2_PI
  out_result.tangent = (float)(2.f* M_PI) * glm::vec3(-local_hit.z, 0, -local_hit.x);

  //todo: more efficient way?
  out_result.bitangent =  (float)(M_PI * rad) * glm::normalize(glm::cross(norm, out_result.tangent));

  if (!std::isfinite(out_result.surface_uv.x)) {
    out_result.surface_uv.x = 0;
  }
    if (!std::isfinite(out_result.surface_uv.y)) {
    out_result.surface_uv.y = 0;
  }

  return true;
}

// ------------------------------------------------------------------------------------------
//

bool intersectBox(IntersectResult &out_result, 
        const Ray &r,float min_t, float max_t,
        const glm::vec3 &pos, const glm::vec3 &size)  {
  bool hasIntersection = false;
#if 0 // todo: is this one better? idk?
  glm::vec3 half_size = size / 2.f;
  glm::vec3 center = centroid();


  float t0 = -std::numeric_limits<float>::infinity();
  float t1 = std::numeric_limits<float>::infinity();
  int a0 = -1;
  int a1 = -1;
  
  for (int i = 0; i < 3; i++) {
    float tmin = (pos[i] - r.o[i]) / r.l[i];
    float tmax = tmin + size[i] / r.l[i];
    if ( tmin > tmax ) { std::swap(tmin, tmax); }
    if ( tmin > t0 ) { t0 = tmin; a0 = i;}
    if ( tmax < t1 ) { t1 = tmax; a1 = i; }
  }

  float t;
  float n;
  int a;

  if ( t0 > min_t && t0 < max_t) {
    t = t0;
    a = a0;
    n = -r.l[a] / r.l[a];
  } else if (t1 > min_t && t1 < max_t) {
    t = t1;
    a = a1;
    n = r.l[a] / r.l[a];
  } else {
    return false;
  }

  glm::vec3 extent = this->max(); // store value
  a0 = (a + 1) % 3;
  a1 = (a + 2) % 3;
  glm::vec3 p = t * r.l + r.o;

  if ( p[a0] >= pos[a0] && p[a0] <= extent[a0] &&
       p[a1] >= pos[a1] && p[a1] <= extent[a1]) {
      out_result.t = t;
      out_result.position = p;
      out_result.normal = glm::vec3(0);
      out_result.normal[a] = n;
      return true;
  } 

  return false;

#elif 1
  glm::vec3 o_local = r.o - pos; // offest view vector, so box lies at {0,0,0} relative

  for (int i = 0; i < 3; i++) {
    if (r.l[i] == 0.0) { continue;  }
    for (int f = 0; f < 2; f++) {
      double t = (size[i]*f - o_local[i]) / r.l[i];

      // distance check
      if (t < min_t || t > max_t) { continue; }

      glm::vec3 p = o_local + float(t)*r.l;

      // aperture test
      uint32_t a1 = (i + 1) % 3;
      uint32_t a2 = (i + 2) % 3;
      if (   p[a1] < 0 || p[a1] > size[a1] 
          || p[a2] < 0 || p[a2] > size[a2]) {
        continue;
      }

      glm::vec3 n(0, 0, 0);
      n[i] = f == 0 ? -1 : 1;

      out_result = IntersectResult();
      out_result.t = t;
      out_result.position = p + pos; // undo offset
      out_result.normal = n;
      out_result.surface_uv = glm::vec2(p[a1], p[a2]) / glm::vec2(size[a1], size[a2]);
      out_result.tangent = glm::vec3(0); out_result.tangent[a1] = size[a1];
      out_result.bitangent = glm::vec3(0); out_result.bitangent[a2] = size[a2];

      max_t = t;
      hasIntersection = true;

    }
  }
#endif
  return hasIntersection;
}
void generateCube(unsigned int &out_count, std::vector<glm::vec3> &out_vertices, std::vector<glm::vec3> &out_normals, std::vector<glm::vec2> &out_uvs, std::vector<glm::vec3> &out_tangents, std::vector<glm::vec3> &out_bitangents) {
  /*
	  2-----3
	 /|    /|
	6-----7 |
	| 0---|-1
	|/    |/
	4-----5
	*/

  int size = 3 * 2 * 6;
  out_normals.reserve(size + out_normals.size());
  out_uvs.reserve(size + out_uvs.size());
  out_tangents.reserve(size + out_tangents.size());
  out_bitangents.reserve(size + out_bitangents.size());

  out_count = size;

	std::vector<glm::vec3> verts{
		{0, 0, 0,}, // 0
		{0, 1, 0,}, // 2 
		{1, 0, 0,}, // 1
		{1, 0, 0,}, // 1
		{0, 1, 0,}, // 2
		{1, 1, 0,}, // 3

		{1, 0, 0,}, // 1
		{1, 1, 0,}, // 3
		{1, 0, 1,}, // 5
		{1, 0, 1,}, // 5
		{1, 1, 0,}, // 3
		{1, 1, 1,}, // 7

		{1, 0, 1,}, // 5
		{1, 1, 1,}, // 7
		{0, 0, 1,}, // 4
		{0, 0, 1,}, // 4
		{1, 1, 1,}, // 7
		{0, 1, 1,}, // 6

		{0, 0, 1,}, // 4
		{0, 1, 1,}, // 6
		{0, 0, 0,}, // 0
		{0, 0, 0,}, // 0
		{0, 1, 1,}, // 6
		{0, 1, 0,}, // 2

		{0, 1, 0,}, // 2
		{0, 1, 1,}, // 6
		{1, 1, 0,}, // 3
		{1, 1, 0,}, // 3
		{0, 1, 1,}, // 6
		{1, 1, 1,}, // 7

		{1, 0, 1,}, // 5
		{1, 0, 0,}, // 1
		{0, 0, 1,}, // 4
		{0, 0, 1,}, // 4
		{1, 0, 0,}, // 1
		{0, 0, 0,}, // 0
	};

  for (glm::vec3 n : {
                      glm::vec3{ 0, 0,-1}, 
                      glm::vec3{ 1, 0, 0},
                      glm::vec3{ 0, 0, 1},
                      glm::vec3{-1, 0, 0},
                      glm::vec3{ 0, 1, 0},
                      glm::vec3{ 0,-1, 0}
                    } ) 
  {
    for (int i = 0; i < 3*2; i++) {
      out_normals.push_back(n);
    }
  }

  for (int i = 0; i < 6; i ++) {
    out_uvs.emplace_back(1, 0);
    out_uvs.emplace_back(1, 1);
    out_uvs.emplace_back(0, 0);
    
    out_uvs.emplace_back(0, 0);
    out_uvs.emplace_back(1, 1);
    out_uvs.emplace_back(0, 1);
  }

  for (int i = 0; i < 6; i ++) {
    glm::vec3 tangent = glm::normalize(verts[i*6 + 0] - verts[i*6 + 2]);
    glm::vec3 bitangent = glm::normalize(verts[i*6 + 5] - verts[i*6 + 2]);
    for (int v = 0; v < 6; v++){
      out_tangents.push_back(tangent);
      out_bitangents.push_back(bitangent);
    }
  }

  out_vertices.reserve(out_vertices.size() + verts.size());
  out_vertices.insert(out_vertices.end(), verts.begin(), verts.end());
}

void generateSphere(unsigned int &out_count, std::vector<glm::vec3> &out_vertices, std::vector<glm::vec3> &out_normals, std::vector<glm::vec2> &out_uvs, std::vector<glm::vec3> &out_tangents, std::vector<glm::vec3> &out_bitangents,
                    int rows = 16, int segs = 32 ) {
	int size = 3 * 2 * rows * segs;
  out_count = size;
  out_vertices.reserve(out_vertices.size() + size);
  out_normals.reserve(out_normals.size() + size);
  out_uvs.reserve(out_uvs.size() + size);
  out_tangents.reserve(out_tangents.size() + size);
  out_bitangents.reserve(out_bitangents.size() + size);

	int cnt = 0;

	int row = 0;
	int seg = 0;

	const float y_inc = M_PI / static_cast<float>(rows);
	float y_angle = -M_PI / 2.f;

	const float x_inc = 2.f * M_PI / static_cast<float>(segs);
	
	for ( int row = 0; row < rows; row++) {
		y_angle = -M_PI / 2.f + y_inc * row;

		float cur_y = sin(y_angle);
		float cur_rad = cos(y_angle);

		y_angle += y_inc; 

		float next_y = sin(y_angle);
		float next_rad = cos(y_angle);

		for (int seg = 0; seg < segs; seg++) {
			float x_angle = x_inc * seg;
			
			float cur_x = cos(x_angle);
			float cur_z = -sin(x_angle);

			x_angle += x_inc;

			float next_x = cos(x_angle);
			float next_z = -sin(x_angle);

			out_vertices.push_back({ cur_rad * cur_x,
                              cur_y, 
                              cur_rad * cur_z});
      out_normals.push_back(out_vertices.back());
      out_uvs.emplace_back(float(seg) / segs, float(row) / rows );

			out_vertices.push_back({ cur_rad * next_x,
			                        cur_y,
			                        cur_rad * next_z});
      out_normals.push_back(out_vertices.back());
      out_uvs.emplace_back(float(seg + 1) / segs, float(row) / rows );

			out_vertices.push_back({ next_rad * next_x,
			                        next_y,
			                        next_rad * next_z});
      out_normals.push_back(out_vertices.back());
      out_uvs.emplace_back(float(seg + 1) / segs, float(row + 1) / rows );

			out_vertices.push_back({ cur_rad * cur_x,
			                        cur_y,
			                        cur_rad * cur_z });
      out_normals.push_back(out_vertices.back());
      out_uvs.emplace_back(float(seg) / segs, float(row) / rows );

			out_vertices.push_back({ next_rad * next_x,
			                        next_y,
			                        next_rad * next_z});
      out_normals.push_back(out_vertices.back());
      out_uvs.emplace_back(float(seg + 1) / segs, float(row + 1) / rows );

			out_vertices.push_back({ next_rad * cur_x,
			                        next_y,
			                        next_rad * cur_z });
      out_normals.push_back(out_vertices.back());
      out_uvs.emplace_back(float(seg) / segs, float(row + 1) / rows );
		}
	}


  for (int i = 0; i < size; i++) {
    glm::vec3 tangent = glm::cross(glm::vec3(0, 1, 0), out_normals[i]);
    glm::vec3 bitangent = glm::cross(out_normals[i], tangent);
    out_tangents.push_back(tangent);
    out_bitangents.push_back(bitangent);
  }
}

// ------------------------------------------------------------------------------------------
//
Sphere::~Sphere()
{
}

bool Sphere::intersect(IntersectResult &out_result, const Ray &r, float min_t, float max_t) const {
  return rayIntersectSphere(out_result, r, min_t, max_t, {0, 0, 0}, 1.0);
}

NonhierBox Sphere::getBounds() const {
  return NonhierBox({-1, -1, -1}, 2);
}


// ------------------------------------------------------------------------------------------
//
Cube::~Cube()
{
}

bool Cube::intersect(IntersectResult &out_result, const Ray &r, float min_t, float max_t) const{
  return intersectBox(out_result, r, min_t, max_t, {0, 0, 0}, {1, 1, 1});
}

NonhierBox Cube::getBounds() const {
  return NonhierBox({0, 0, 0}, 1);
}


// ------------------------------------------------------------------------------------------
//
NonhierSphere::~NonhierSphere()
{
}

bool NonhierSphere::intersect(IntersectResult &out_result, const Ray &r, float min_t, float max_t) const {
  return rayIntersectSphere(out_result, r, min_t, max_t, m_pos, m_radius);
}

NonhierBox NonhierSphere::getBounds() const {
  return NonhierBox(m_pos - glm::vec3(m_radius), 2 * m_radius);
}


// ------------------------------------------------------------------------------------------
//
NonhierBox::~NonhierBox()
{
}

bool NonhierBox::intersect(IntersectResult &out_result, const Ray &r,float min_t, float max_t) const {
  return intersectBox(out_result, r, min_t, max_t, m_pos, m_size);
}

NonhierBox NonhierBox::getBounds() const {
  return *this;
};
