#pragma once

#include <glm/glm.hpp>

#include "Scene.h"
#include "Material.h"
#include "Image.h"

// ~~~~~~~~~~~~~~~
#include <time.h>
#include <iostream>
#include <iomanip>

class ScopedTimer {
  std::string message;
  uint32_t precision;

  clock_t start_time;
public:
  ScopedTimer(std::string message = "", uint32_t precision = 0) :
    message(message), precision(precision), start_time(clock()) { }

  ~ScopedTimer() {
    std::cout << std::fixed << std::setprecision(precision)
      << float(clock() - start_time) / float(CLOCKS_PER_SEC) << "s " << message << std::endl;
  }

  const clock_t& startTime() const { return start_time; }
};
// ~~~~~~~~~~~~~~~

struct IntersectResult {
  IntersectResult() = default;
  IntersectResult& operator=(const IntersectResult& other) = default;
  IntersectResult(const glm::vec3& p, glm::vec3& n) : position(p), normal(n), surface_uv(0, 0) {}

  glm::vec3 position;
  glm::vec3 normal;
  glm::vec3 tangent = { 0, 0, 0 };
  glm::vec3 bitangent = { 0, 0, 0 };
  glm::vec2 surface_uv = { 0, 0 };
  double t;
  Material* material;
};

struct Ray {
  glm::vec3 o; // origin
  glm::vec3 l; // length

  inline Ray transformed(const glm::mat4& trans) const {
    return Ray{ glm::vec3(trans * glm::vec4(o, 1.f)),  glm::vec3(trans * glm::vec4(l, 0.f)) };
  }

  friend Ray operator*(const glm::mat4& trans, const Ray& ray);

  Ray() = delete; // no default constructor

};

inline Ray operator*(const glm::mat4& trans, const Ray& ray) {
  return ray.transformed(trans);
}

struct CameraRay {
  Ray r;
  bool hasDifferentials = false;
  Ray dx;
  Ray dy;

  CameraRay(Ray r) : r(r), hasDifferentials(false), dx{ {0,0,0},{0,0,0} }, dy{ {0,0,0},{0,0,0} } {}
};


// Shading Functions
bool intersectScene(
  IntersectResult& out_result, 
  SceneNode* node, 
  const Ray& r, 
  double min_t = 0.0, 
  double max_t = std::numeric_limits<double>::infinity()
);

glm::vec3 rayColor(
  SceneNode* root,
  const glm::vec3& ambient,
  const std::list<Light*>& lights,
  const CameraRay& r,
  double attenuation = 1.0
);

// Main render function
void RenderRaytracer(
  SceneNode* root,

  // Output image
  Image& image,

  // Viewing parameters
  const glm::vec3& eye,
  const glm::vec3& view,
  const glm::vec3& up,
  double fovy,

  // Lighting parameters
  const glm::vec3& ambient,
  const std::list<Light*>& lights
);

void RenderRaytracer_ST(
  SceneNode* root,

  // Output image
  Image& image,

  // Viewing parameters
  const glm::vec3& eye,
  const glm::vec3& view,
  const glm::vec3& up,
  double fovy,

  // Lighting parameters
  const glm::vec3& ambient,
  const std::list<Light*>& lights
);

int renderToWindow(
  SceneNode* root,

  // Output image
  Image& image,

  // Viewing parameters
  const glm::vec3& eye,
  const glm::vec3& view,
  const glm::vec3& up,
  double fovy,

  // Lighting parameters
  const glm::vec3& ambient,
  const std::list<Light*>& lights
);