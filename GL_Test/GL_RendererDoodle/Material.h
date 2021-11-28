#pragma once

#include <list>
#include <map>
#include <string>

#include "glm/glm.hpp"

#include "Image.h"
#include "Texture.h"

#include "Asset.h"

struct SceneNode;
struct IntersectResult;
struct Light;
struct Image;
struct CameraRay;

struct MaterialValue {
  virtual glm::vec3 shade(const CameraRay& view, IntersectResult* fragmentIntersection) = 0;
};

struct UnassignedMaterialValue : public MaterialValue {
  UnassignedMaterialValue() {};
  virtual glm::vec3 shade(const CameraRay& view, IntersectResult* fragmentIntersection) override;
};

struct ColorMaterialValue : public MaterialValue {
  glm::vec3 color;
  ColorMaterialValue(const glm::vec3& color) : color(color) {}

  virtual glm::vec3 shade(const CameraRay& view, IntersectResult* fragmentIntersection) override;
};

struct TextureMaterialValue : public MaterialValue {
  const Texture tex;
  TextureMaterialValue(const Texture& tex) : tex(tex) {}
  TextureMaterialValue(const std::string& filename) : tex(Image::loadPng(searchForAsset(filename))) {}

  virtual glm::vec3 shade(const CameraRay& view, IntersectResult* fragmentIntersection) override;
};

struct TextureNormalMaterialValue : public MaterialValue {
  const Texture tex;
  TextureNormalMaterialValue(const Texture& tex) : tex(tex) {}
  TextureNormalMaterialValue(const std::string& filename) : tex(Image::loadPng(searchForAsset(filename))) {}

  virtual glm::vec3 shade(const CameraRay& view, IntersectResult* fragmentIntersection) override;
};

struct MeshNormalMaterialValue : public MaterialValue {
  MeshNormalMaterialValue() {}
  
  virtual glm::vec3 shade(const CameraRay& view, IntersectResult* fragmentIntersection) override;
};

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
