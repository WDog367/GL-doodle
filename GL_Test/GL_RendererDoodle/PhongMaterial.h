// Fall 2019

#pragma once
#include "Material.h"

#include <glm/glm.hpp>

#include <vector>

#include "Scene.h"

// Use this define to use stochiastic soft shadows instead of hard shadows for phong material
//#define SOFT_SHADOWS

// Use this define to use stochiastic soft reflections instead of hard reflections for phong material
//#define SOFT_REFLECTIONS

// Use to disable the extra feature, reflections
//#define DISABLE_REFLECTIONS


//TODO: use composition instead of inheritence for the different components....
// i.e. -> ColourProvider Kd, ColourProvider Ks, NormalProvider norm, FloatProvider shininess
//  -> determine overall ID by concatenating provider id's
//  -> ditto for shader flags..
// maybe do something about light sources too?

// FOR NOW, just make PBR classes inherit from Phong classes (So they'll have a raytraced shading function)

// will need some conversion functions too, if going to convert PHONG to PBR and vice-versa
// FOR NOW it will be fine to use shininess = [0, 128] instead of roughness, no its not, then i can't use resorces from the internet, well I can still use albedo + normal map..

// THEN do different class for PBR vs phong (since they are different models)

// (may need ENVMap provider for GPU versions...)


#include "Image.h"
#include "Texture.h"


class PhongMaterial : public Material {
public:
  PhongMaterial(const glm::vec3& kd, const glm::vec3& ks, double shininess);
  virtual ~PhongMaterial();

  glm::vec3 shade(
    SceneNode* root, 
    const glm::vec3 & ambient, const std::list<Light *> & lights,
    const CameraRay &view, 
    IntersectResult* fragmentIntersection,
    double attenuation) override;

    const glm::vec3 &getKd() const { return m_kd; }
    const glm::vec3 &getKs() const { return m_ks; }
    double getShininess() const { return m_shininess; }

    std::string getId() const override;
    
   
private:
  glm::vec3 m_kd;
  glm::vec3 m_ks;

  double m_shininess;

  friend struct GLR_PhongMaterial;
};

class TexturedPhongMaterial : public Material {
public:
  TexturedPhongMaterial(const std::string &textureName, const glm::vec3& ks, double shininess);
  virtual ~TexturedPhongMaterial();

  glm::vec3 shade(
    SceneNode* root, 
    const glm::vec3 & ambient, const std::list<Light *> & lights,
    const CameraRay &view, 
    IntersectResult* fragmentIntersection,
    double attenuation) override;

  std::string getId() const override;

private:
  Texture m_kd;
  glm::vec3 m_ks;

  double m_shininess;
};

class NormalTexturedPhongMaterial : public Material {
public:
  NormalTexturedPhongMaterial(const std::string &textureName, const std::string &normalMapName,const glm::vec3& ks, double shininess);
  virtual ~NormalTexturedPhongMaterial();

  glm::vec3 shade(
    SceneNode* root, 
    const glm::vec3 & ambient, const std::list<Light *> & lights,
    const CameraRay &view, 
    IntersectResult* fragmentIntersection,
    double attenuation) override;

  std::string getId() const override;


private:
  Texture m_kd;
  Texture m_normalMap;
  glm::vec3 m_ks;

  double m_shininess;

  friend struct GLR_NormalTexturedPhongMaterial;
};

class PBRMaterial : public Material {
public:
  glm::vec3 shade(
    SceneNode* root, 
    const glm::vec3 & ambient, const std::list<Light *> & lights,
    const CameraRay &view, 
    IntersectResult* fragmentIntersection,
    double attenuation) override;

  PBRMaterial(const glm::vec3 &albedo, float roughness, float metal);
  ~PBRMaterial();
  std::string getId() const override;

private:
  glm::vec3 m_kd;
  float m_metal;
  float m_roughness;

  friend struct GLR_PBRMaterial;
};

class NormalTexturedPBRMaterial : public Material {
public:
  glm::vec3 shade(
    SceneNode* root, 
    const glm::vec3 & ambient, const std::list<Light *> & lights,
    const CameraRay &view, 
    IntersectResult* fragmentIntersection,
    double attenuation) override;

  NormalTexturedPBRMaterial(const std::string &textureName, const std::string &normalMapName, float roughness, float metal);
  ~NormalTexturedPBRMaterial();
  
  std::string getId() const override;

private:
  Texture m_kd;
  Texture m_normalMap;
  float m_metal;
  float m_roughness;

  friend struct GLR_NormalTexturedPBRMaterial;
};