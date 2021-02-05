// - Program the Scene in C++ so we don't have to embed a scripting language...
// - I want to figure out Python, but Lua may be the better choice after all
//

#include "Scene.h"

#include "RenderRaytracer.h"

#include "Material.h"
#include "PhongMaterial.h"

#include "Primitive.h"
#include "Mesh.h"
#include "Asset.h"

SceneNode* defaultScene() {
  SceneNode* root = new SceneNode("root");

  Material* mat1 = new PhongMaterial({0.7, 1.0, 0.7 }, {0.5, 0.7, 0.5 }, 25);
  Material* mat2 = new PhongMaterial({0.5, 0.5, 0.5 }, {0.5, 0.7, 0.5 }, 25);
  Material* mat3 = new PhongMaterial({ 1.0, 0.6, 0.1 }, {0.5, 0.7, 0.5 }, 25);
  Material* mat4 = new PhongMaterial({ 0.7, 0.6, 1.0 }, { 0.5, 0.4, 0.8 }, 25);
  Material* mat5 = new PhongMaterial({ 0.7, 0.7, 1.7 }, { 0.8, 0.8, 0.7 }, 50);

  auto s1 = new GeometryNode("s1", new NonhierSphere({ 0, 0, -400 }, 100));
  root->add_child(s1);
  s1->setMaterial(mat1);
  
  auto s2 = new GeometryNode("s2", new NonhierSphere({ 200, 50, -100 }, 150));
  root->add_child(s2);
  s2->setMaterial(mat1);

  auto s3 = new GeometryNode("s3", new NonhierSphere({ 0, -1200, -500 }, 1000));
  root->add_child(s3);
  s3->setMaterial(mat2);

  auto b1 = new GeometryNode("b1", new NonhierBox({ -200, -125, 0 }, 100));
  root->add_child(b1);
  b1->setMaterial(mat4);

  auto b2 = new GeometryNode("b2", new NonhierBox({ -100, 25, -300 }, 50));
  root->add_child(b2);
  b2->setMaterial(mat3);

  auto b3 = new GeometryNode("b3", new NonhierBox({ 0, 100, -250 }, 25));
  root->add_child(b3);
  b3->setMaterial(mat1);

  auto m1 = new GeometryNode("m1", new Mesh("clannfear.obj"));
  root->add_child(m1);
  m1->setMaterial(mat4);
  m1->rotate('z', 25.f);
  m1->scale({ 50, 50, 50 });
  m1->translate({ 50, -150, -100 });

  auto white_light = new LightNode("light1", new Light({ -100.0, 150.0, 400.0 }, { 0.9, 0.9, 0.9 }, { 1, 0, 0 }));
  auto magenta_light = new LightNode("light2", new Light({ 400.0, 100.0, 150.0 }, { 0.7, 0.0, 0.7 }, { 1, 0, 0 }));

  root->add_child(white_light);
  root->add_child(magenta_light);

  Image screenshot(128, 128);
  renderToWindow(root, screenshot, { 0, 0, 800 }, { 0, 0, -1 }, { 0, 1, 0 }, 50, { 0.3, 0.3, 0.3 }, {});
  screenshot.savePng("nonhier.png");

  return root;
}