// Fall 2019

#pragma once

#include <glm/glm.hpp>
#include "glm/gtx/transform.hpp"
#include "glm/gtx/norm.hpp"

#include <algorithm>
#include <iostream>
#include <vector>

#undef min
#undef max

struct IntersectResult;
struct Ray;
class NonhierBox;

class Primitive {
public:
  virtual ~Primitive();

  virtual bool intersect(IntersectResult &out_result, const Ray &r, float min_t, float max_t) const = 0;
  virtual bool intersectSphere(IntersectResult& out_result, const glm::vec3 &pos, float radius) const { return false; }

  virtual NonhierBox getBounds() const = 0;

};

class Sphere : public Primitive {
public:
  virtual ~Sphere();

  bool intersect(IntersectResult &out_result, const Ray &r, float min_t, float max_t) const override;
  NonhierBox getBounds() const override;
  
  virtual bool intersectSphere(IntersectResult& out_result, const glm::vec3 &pos, float radius) const { 
    return (glm::length2(pos)) < (radius + 1) * (radius + 1); 
  }

};

class Cube : public Primitive {
public:
  virtual ~Cube();

  bool intersect(IntersectResult &out_result, const Ray &r, float min_t, float max_t) const override;
  NonhierBox getBounds() const override;

};

class NonhierSphere : public Sphere {
public:
  NonhierSphere(const glm::vec3& pos, double radius)
    : m_pos(pos), m_radius(radius)
  {
  }
  virtual ~NonhierSphere();

  bool intersect(IntersectResult &out_result, const Ray &r, float min_t, float max_t) const override;
  virtual bool intersectSphere(IntersectResult& out_result, const glm::vec3 &pos, float radius) const { 
    return (glm::length2(pos - m_pos)) < (radius + m_radius) * (radius + m_radius); 
  }
  NonhierBox getBounds() const override;

private:
  glm::vec3 m_pos;
  double m_radius;
};

class NonhierBox : public Cube {
public:
  NonhierBox(const glm::vec3& pos, double size)
    : m_pos(pos), m_size(size)
  {
  }
  
  NonhierBox(const glm::vec3& pos, glm::vec3 size) 
    : m_pos(pos), m_size(size)
  {
  }

  NonhierBox() 
    : m_pos(std::numeric_limits<float>::quiet_NaN()), m_size(0)
  {
  }

  virtual ~NonhierBox();

  bool intersect(IntersectResult &out_result, const Ray &r, float min_t, float max_t) const override;
  NonhierBox getBounds() const override;

  glm::vec3 centroid() const {
    return m_pos + m_size * 0.5f;
  }

  glm::vec3 min() const {
    return m_pos;
  }

  glm::vec3 max() const {
    return m_pos + m_size;
  }

  glm::vec3 size() const { 
    return m_size;
  }

  NonhierBox extend(const NonhierBox & other) {
    if (glm::all(glm::isnan(this->m_pos))) {
      return NonhierBox(other);
    }
    if (glm::all(glm::isnan(other.m_pos))) {
      return NonhierBox(*this);
    }

    const glm::vec3 max = glm::max(this->max(), other.max());
    const glm::vec3 min = glm::min(this->min(), other.min());
    const glm::vec3 size = max - min;
    return NonhierBox(min, size);
  }

  NonhierBox extend(const glm::vec3 & point) {
    if (this->m_pos == glm::vec3(std::numeric_limits<float>::quiet_NaN()) ) {
      return NonhierBox(point, 0);
    }

    const glm::vec3 max = glm::max(this->max(), point);
    const glm::vec3 min = glm::min(this->min(), point);
    const glm::vec3 size = max - min;
    return NonhierBox(min, size);
  }

  double surfaceArea() const {
    return 2.0 * (m_size.x * m_size.y + 
                  m_size.x * m_size.z +
                  m_size.y * m_size.z);
  }

  bool contains(const glm::vec3 &p) {
    const glm::vec3 diff_2 = 2.f*(p - centroid());
    return diff_2 == glm::clamp(diff_2, -m_size, m_size);
  }

private:
  glm::vec3 m_pos;
  glm::vec3 m_size;
};

// mesh generation
void generateCube(unsigned int &out_count, std::vector<glm::vec3> &out_vertices, std::vector<glm::vec3> &out_normals, std::vector<glm::vec2> &out_uvs, std::vector<glm::vec3> &out_tangents, std::vector<glm::vec3> &out_bitangents);