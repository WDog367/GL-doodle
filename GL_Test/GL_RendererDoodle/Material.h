#pragma once

#include <list>
#include <map>
#include <string>

#include "glm/glm.hpp"

#include "Image.h"
#include "Texture.h"

struct SceneNode;
struct IntersectResult;
struct Light;
struct Image;
struct CameraRay;

class Material {
public:
  id_code id;

  virtual ~Material();

  virtual glm::vec3 shade(
    SceneNode* root, const glm::vec3 & ambient, const std::list<Light *> & lights,
    const CameraRay &view, IntersectResult* IntersectResult,
    double attenuation = 1.0) = 0;

    virtual std::string getId() const = 0;
    
protected:
  Material();
};
