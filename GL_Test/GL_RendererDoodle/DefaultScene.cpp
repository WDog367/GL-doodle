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

struct SceneInfo {
	glm::vec3 eye = { 0, 0, 0 };
	glm::vec3 view = { 0, 0, -1 };
	glm::vec3 up = { 0, 1, 0 };
	float fov = 50;
	glm::vec3 ambient = { 0, 0, 0 };
	std::list<Light*> lights = {};
};

inline void render(SceneNode* root, std::string fname, size_t w, size_t h, glm::vec3 pos, glm::vec3 eye, glm::vec3 up, double fovy, glm::vec3 color, std::list<Light*> lights) {
	Image screenshot(w, h);
	renderToWindow(root, screenshot, pos, eye, up, fovy, color, lights);
	screenshot.savePng(fname);
}

SceneNode* boxScene(struct SceneInfo& scene_info) {
	SceneNode* scene = new SceneNode("root");

	auto dirt = new PhongMaterial("dirt.png", "dirt_n.png", { 0.1, 0.1, 0.1 }, 20);

	auto dirt_pbr = new PBRMaterial("dirt.png", "dirt_n.png", 0.9, 0.0);
	
	std::list<Light*> lights = { new Light({0, 0, 10 + 5}, {0.9, 0.9, 0.9}, {1, 0, 0}) };

	auto s3 = new GeometryNode("s3", new Mesh("box_10.obj"));
	//s3->scale({ 5, 5, 5 });

	auto s4 = new GeometryNode("s4", new Mesh("box_10.obj"));
	//s4->scale({ 5, 5, 5 });

	scene->add_child(s3);
	s3->setMaterial(dirt);

	scene->add_child(s4);
	s4->setMaterial(dirt_pbr);
	s4->translate({ 10, 0, 0 });

	render(scene,
		"box_obj_1.png", 512, 512,
		{ 0, 0, 10 + 5 }, { 0.00, 0.0, 0.0 }, { 0, 1, 0 }, 45,
		{ 0.4, 0.4, 0.4 }, lights);

		render(scene,
			"box_obj_2.png", 512, 512,
			{ 0, 0, 20 + 5 }, { 0.00, 0.0, 0.0 }, { 0, 1, 0 }, 45,
			{ 0.4, 0.4, 0.4 }, lights);

		render(scene,
			"box_obj_3.png", 512, 512,
			{ 0, 0, 40 + 5 }, { 0.00, 0.0, 0.0 }, { 0, 1, 0 }, 45,
			{ 0.4, 0.4, 0.4 }, lights);

		render(scene,
			"box_obj_4.png", 512, 512,
			{ 0, 0, 80 + 5 }, { 0.00, 0.0, 0.0 }, { 0, 1, 0 }, 45,
			{ 0.4, 0.4, 0.4 }, lights);

		render(scene,
			"box_obj_5.png", 512, 512,
			{ 0, 0, 160 + 5 }, { 0.00, 0.0, 0.0 }, { 0, 1, 0 }, 45,
			{ 0.4, 0.4, 0.4 }, lights);

		render(scene,
			"box_obj_6.png", 512, 512,
			{ 0, 0, 320 + 5 }, { 0.00, 0.0, 0.0 }, { 0, 1, 0 }, 45,
			{ 0.4, 0.4, 0.4 }, lights);

		render(scene,
			"box_obj_7.png", 512, 512,
			{ 0, 0, 640 + 5 }, { 0.00, 0.0, 0.0 }, { 0, 1, 0 }, 45,
			{ 0.4, 0.4, 0.4 }, lights);

		render(scene,
			"box_obj_8.png", 512, 512,
			{ 0, 0, 1280 + 5 }, { 0.00, 0.0, 0.0 }, { 0, 1, 0 }, 45,
			{ 0.4, 0.4, 0.4 }, lights);
	
		scene_info = SceneInfo{ { 0, 0, 10 + 5 }, { 0.00, 0.0, 0.0 }, { 0, 1, 0 }, 45, { 0.4, 0.4, 0.4 }, lights };

		return scene;
}


SceneNode* simpleScene(struct SceneInfo &scene_info) {
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

  Image screenshot(1024, 1024);
  renderToWindow(root, screenshot, { 0, 0, 800 }, { 0, 0, -1 }, { 0, 1, 0 }, 50, { 0.3, 0.3, 0.3 }, {});
  // screenshot.savePng("nonhier.png");
	scene_info = SceneInfo{ { 0, 0, 800 }, { 0, 0, -1 }, { 0, 1, 0 }, 50, { 0.3, 0.3, 0.3 } };

  return root;
}

SceneNode* bigCoatScene(struct SceneInfo& scene_info) {
	auto skin = new PhongMaterial({ 0.75, 0.75, 0.0 }, { 0.01, 0.01, 0.01 }, 10);
	auto green = new PhongMaterial({ 0.0, 1.0, 0.0 }, { 0.05, 0.05, 0.05 }, 10);
	auto red = new PhongMaterial({ 1.0, 0.0, 0.0 }, { 0.05, 0.05, 0.05 }, 10);

	auto plae_skin = new PhongMaterial({ 0.9, 0.6, 0.25 }, { 0.01, 0.01, 0.01 }, 10);
	auto orange = new PhongMaterial({ 0.5, 0.30, 0.05 }, { 0.05, 0.05, 0.05 }, 10);
	auto blue = new PhongMaterial({ 0.1, 0.1, 0.9 }, { 0.05, 0.05, 0.05 }, 10);

	auto grass = new PhongMaterial({ 0.1, 0.7, 0.1 }, { 0.0, 0.0, 0.0 }, 0);

	auto metal = new PhongMaterial({ 0.3, 0.3, 0.3 }, { 0.6, 0.6, 0.6 }, 78);
	auto white = new PhongMaterial({ 1.0, 1.0, 1.0 }, { 0.5, 0.5, 0.5 }, 78);

	auto concrete = new PhongMaterial({ 0.9, 0.9, 0.9 }, { 0.1, 0.1, 0.1 }, 10);
	auto asphalt = new PhongMaterial({ 0.1, 0.1, 0.1 }, { 0.2, 0.2, 0.2 }, 30);

	auto add_car = [&](std::string name, Material* paint)->SceneNode* {
		auto rubber = new PhongMaterial({ 0.2, 0.2, 0.2 }, { 0.3, 0.3, 0.3 }, 40);

		auto length = 35;
		auto width = 15;
		auto height = 15;

		auto wheel_height = 5;
		auto wheel_ratio = 0.5f;

		auto light_rad = 1.5;

		auto root = new SceneNode(name + "_root");

		static auto addWheel = [&](std::string name)->SceneNode* {
			auto tire = new GeometryNode(name + "_tire", new Mesh("cylinder.obj"));
			root->add_child(tire);
			tire->setMaterial(rubber);
			tire->scale({1.f, wheel_ratio, 1.f});

			auto hub = new GeometryNode(name + "_hub", new Mesh("cylinder.obj"));
			tire->add_child(hub);
			hub->scale({0.5, 1.0, 0.5});
			hub->translate({0, 0.1, 0});
			hub->setMaterial(metal);

			tire->rotate('x', 90);
			tire->scale({wheel_height, wheel_height, wheel_height});

			return tire;
		};

		auto body = new GeometryNode(name + "_body", new Cube());
		root->add_child(body);
		body->setMaterial(paint);
		body->translate({-0.5, 0, -0.5});
		body->scale({length, height / 2, width});
		body->translate({0, wheel_height, 0});

		auto grill = new GeometryNode(name + "_grill", new Cube());
		body->add_child(grill);
		grill->setMaterial(concrete);
		grill->scale({1.02, 0.5, 0.75});
		grill->translate({-0.01, 0.125, 0.125});

		auto cab = new GeometryNode(name + "_cab", new Cube());
		root->add_child(cab);
		cab->setMaterial(paint);
		cab->translate({-0.5, 1.0, -0.5});
		cab->scale({length / 2, height / 2, width});
		cab->translate({0, wheel_height, 0});

		auto windshield = new GeometryNode(name + "_windshield", new Cube());
		cab->add_child(windshield);
		windshield->setMaterial(metal);
		windshield->scale({1.02, 0.8, 0.8});
		windshield->translate({-0.01, 0, 0.1});

		auto f_window = new GeometryNode(name + "_f_window", new Cube());
		cab->add_child(f_window);
		f_window->setMaterial(metal);
		f_window->scale({0.40, 0.8, 1.02});
		f_window->translate({0, 0, -0.01});
		f_window->translate({0.0, 0, 0});

		auto glass_light = new PhongMaterial({ 5.0, 5.0, 0.0 }, { 0.5, 0.5, 0.5 }, 78);

		auto fl = new GeometryNode(name + "_fl", new Sphere{});
		root->add_child(fl);
		fl->setMaterial(glass_light);
		fl->scale({light_rad, light_rad, light_rad});
		fl->translate({-length / 2, wheel_height + height / 4, (width / 2 - light_rad - width * 0.05)});

		auto fr = new GeometryNode(name + "_fr", new Sphere{});
		root->add_child(fr);
		fr->setMaterial(glass_light);
		fr->scale({light_rad, light_rad, light_rad});
		fr->translate({-length / 2, wheel_height + height / 4, -(width / 2 - light_rad - width * 0.05)});

		auto fl_tire = addWheel(name + "fl_wheel");
		fl_tire->rotate('y', 0);
		fl_tire->translate({-(length / 2 - wheel_height), wheel_height / 2, (width / 2 - wheel_height * wheel_ratio / 2)});
		root->add_child(fl_tire);

		auto fr_tire = addWheel(name + "fr_wheel");
		fr_tire->rotate('y', 180);
		fr_tire->translate({-(length / 2 - wheel_height), wheel_height / 2, -(width / 2 - wheel_height * wheel_ratio / 2)});
		root->add_child(fr_tire);

		auto bl_tire = addWheel(name + "bl_wheel");
		bl_tire->rotate('y', 0);
		bl_tire->translate({(length / 2 - wheel_height), wheel_height / 2, (width / 2 - wheel_height * wheel_ratio / 2)});
		root->add_child(bl_tire);

		auto br_tire = addWheel(name + "br_wheel");
		br_tire->rotate('y', 180);
		br_tire->translate({(length / 2 - wheel_height), wheel_height / 2, -(width / 2 - wheel_height * wheel_ratio / 2)});
		root->add_child(br_tire);

		return root;
	};


	auto add_Lamp = [&](std::string name)->SceneNode* {
		auto hieght = 30;
		auto length = 5;

		auto root = new SceneNode(name + "_root");

		auto pole = new GeometryNode(name + "_pole", new Mesh("cylinder.obj"));
		root->add_child(pole);
		pole->setMaterial(metal);
		pole->scale({1, hieght, 1});

		auto joint = new GeometryNode(name + "_joint", new Sphere{});
		root->add_child(joint);
		joint->translate({0, hieght, 0});
		joint->setMaterial(metal);

		auto pole2 = new GeometryNode(name + "_pole2", new Mesh("cylinder.obj"));
		root->add_child(pole2);
		pole2->setMaterial(metal);
		pole2->scale({1, length, 1});
		pole2->rotate('x', 90);
		pole2->translate({0, hieght, 0});

		auto head = new GeometryNode(name + "_head", new Cube());
		root->add_child(head);
		head->setMaterial(metal);
		head->translate({-0.5, 0, 0});
		head->scale({3, 1, 3});
		head->translate({0, hieght, length});

		auto light_geometry = new GeometryNode(name + "light_glass", new Sphere{});
		head->add_child(light_geometry);
		light_geometry->setMaterial(white);
		light_geometry->scale({0.5, 1, 0.5});
		light_geometry->translate({0.5, 0, 0.5});

		auto light = new Light({ 0.5, -1.1, 0.5 }, { 20, 20, 0.5 }, { 1, 0.2, 0.05 });
		auto light_node = new LightNode(name + "_light", light);
		head->add_child(light_node);

		return root;
	};

	struct arm {
		SceneNode* shoulder;
		SceneNode* elbow;
		SceneNode* hand;
	};

	struct leg {
		SceneNode* hip;
		SceneNode* knee;
		SceneNode* foot;
	};

	struct body {
		SceneNode* root;
		SceneNode* neck;
		SceneNode* head;
		arm l_arm;
		arm r_arm;
		leg l_leg;
		leg r_leg;
		SceneNode* tail;
	};

	auto add_bigCoat = [](Material* red, Material* green, Material* skin)->body {
		auto rootnode = new SceneNode("root");
		auto blue = new PhongMaterial({ 0.0, 0.0, 1.0 }, { 0.001, 0.001, 0.001 }, 5);
		auto white = new PhongMaterial({ 1.0, 1.0, 1.0 }, { 0.001, 0.001, 0.001 }, 5);
		auto pant = new PhongMaterial({ 0.5, 0.5, 0.75 }, { 0.001, 0.001, 0.001 }, 5);

		auto torso2 = new SceneNode("torso2");
		rootnode->add_child(torso2);
		torso2->scale({10, 10, 10});

		auto add_cylinder = [](std::string name)->GeometryNode* {
			auto c = new GeometryNode(name, new Mesh("cylinder.obj"));
			c->translate({ 0, -0.5, 0 });
			c->scale({ 1, 2, 1 });

			return c;
		};

		auto add_cube = [](std::string name)->GeometryNode* {
			//auto c = new GeometryNode(name, new Mesh("cube.obj"));
			auto c = new GeometryNode(name, new Cube());
			c->translate({ -0.5, -0.5, -0.5 });
			return c;
		};

		auto add_sphere = [](std::string name)->GeometryNode* {
			//auto s = new GeometryNode(name, new Mesh("sphere.obj"));
			auto s = new GeometryNode(name, new Sphere{});
			return s;
		};

		auto g_t1 = add_cylinder("g_t1");
		g_t1->scale({0.207, 0.066, 0.159});
		g_t1->translate({0, -0.0172, 0.02});
		g_t1->setMaterial(red);
		torso2->add_child(g_t1);

		auto g_t2 = add_sphere("g_t2");
		g_t2->scale({0.228, 0.148, 0.207});
		g_t2->translate({0, 0.06850, 0.041});
		g_t2->setMaterial(white);
		torso2->add_child(g_t2);

		auto g_t3 = add_sphere("g_t3");
		g_t3->scale({0.195, 0.194, 0.163});
		g_t3->rotate('x', 0.818);
		g_t3->translate({0, 0.18022, 0.03});
		g_t3->setMaterial(white);
		torso2->add_child(g_t3);

		auto g_t4 = add_sphere("g_t4");
		g_t4->scale({0.199, 0.345, 0.156});
		g_t4->rotate('x', -6.26);
		g_t4->translate({0, 0.318, 0.0392});
		g_t4->setMaterial(white);
		torso2->add_child(g_t4);

		auto g_t5 = add_sphere("g_t5");
		g_t5->scale({0.201, 0.143, 0.156});
		g_t5->translate({0, 0.52452, 0.02629});
		g_t5->setMaterial(white);
		torso2->add_child(g_t5);

		auto g_t6 = add_sphere("g_t6");
		g_t6->scale({0.285, 0.087, 0.087});
		g_t6->translate({0, 0.59393, 0.04389});
		g_t6->setMaterial(green);
		torso2->add_child(g_t6);

		auto g_t7 = add_sphere("g_t7");
		g_t7->scale({0.173, 0.072, 0.087});
		g_t7->translate({0, -0.05686, 0.05163});
		g_t7->setMaterial(pant);
		torso2->add_child(g_t7);

		auto g_t9 = add_cylinder("g_t9");
		g_t9->scale({0.125, 0.037, 0.117});
		g_t9->translate({0, 0.64925, 0.03845});
		g_t9->setMaterial(red);
		torso2->add_child(g_t9);

		//neck
		auto neck = new SceneNode("neck");
		neck->translate({0, 0.61428, 0});
		torso2->add_child(neck);

		auto g_n = add_cylinder("g_n");
		g_n->scale({0.074, 0.1, 0.068});
		g_n->translate({0, 0.1019, 0.02496});
		g_n->setMaterial(skin);
		neck->add_child(g_n);

		//head
		auto head = new SceneNode("head");
		head->translate({0, 0.20716, 0.01806});
		neck->add_child(head);

		auto g_h1 = add_sphere("g_h1");
		g_h1->scale({0.113, 0.113, 0.113});
		g_h1->translate({0, 0.06283, 0.03697});
		g_h1->setMaterial(skin);
		head->add_child(g_h1);

		auto g_h2 = add_sphere("g_h2");
		g_h2->scale({0.041, 0.123, 0.039});
		g_h2->rotate('x', -90);
		g_h2->translate({0, 0.01321, 0.16425});
		g_h2->setMaterial(skin);
		head->add_child(g_h2);

		auto g_h3 = add_sphere("g_h3");
		g_h3->scale({0.031, 0.036, 0.036});
		g_h3->translate({0.00974, 0.00236, 0.27590});
		g_h3->setMaterial(skin);
		head->add_child(g_h3);

		auto g_h4 = add_sphere("g_h4");
		g_h4->scale({0.031, 0.036, 0.036});
		g_h4->translate({-0.00974, 0.00236, 0.27590});
		g_h4->setMaterial(skin);
		head->add_child(g_h4);

		auto g_h5 = add_sphere("g_h5");
		g_h5->scale({0.031, 0.040, 0.092});
		g_h5->translate({0.01958, 0.00360, 0.21393});
		g_h5->setMaterial(skin);
		head->add_child(g_h5);

		auto g_h6 = add_sphere("g_h6");
		g_h6->scale({0.031, 0.040, 0.092});
		g_h6->translate({-0.01958, 0.00360, 0.21393});
		g_h6->setMaterial(skin);
		head->add_child(g_h6);

		auto g_h7 = add_sphere("g_h7");
		g_h7->scale({0.07, 0.057, 0.113});
		g_h7->translate({0, 0.011, 0.09952});
		g_h7->setMaterial(skin);
		head->add_child(g_h7);

		auto g_h8 = add_cube("nose");
		g_h8->scale({0.04, 0.035, 0.06});
		g_h8->translate({0, 0.03133, 0.27322});
		g_h8->setMaterial(blue);
		head->add_child(g_h8);

		auto g_h9 = add_sphere("g_h9");
		g_h9->scale({0.154, 0.05, 0.05});
		g_h9->translate({0, -0.01, 0.08});
		g_h9->setMaterial(skin);
		head->add_child(g_h9);

		auto addEar = [&](float offset, std::string prefix) {
			auto base = add_sphere(prefix + "_ear_base");
			base->scale({0.05, 0.073, 0.04});
			base->translate({offset * 0.05, 0.12, 0.01736});
			base->setMaterial(skin);
			head->add_child(base);

			auto tip = new GeometryNode(prefix + "_ear_tip", new Mesh("cone.obj"));
			tip->scale({0.05, 0.073, 0.022});
			tip->translate({offset * 0.054, 0.19, 0.02});
			tip->setMaterial(skin);
			head->add_child(tip);
		};

		addEar(-1, "r");
		addEar(1, "l");

		auto addEye = [&](float offset, std::string prefix) {
			auto eye = add_sphere(prefix + "_eye");
			eye->scale({ 0.026, 0.026, 0.026 });
			eye->translate({ offset * 0.05, 0.07, 0.12 });
			eye->setMaterial(white);
			head->add_child(eye);

			auto pupil = add_sphere(prefix + "_pupil");
			pupil->scale({ 0.019, 0.019, 0.019 });
			pupil->translate({ offset * 0.05, 0.07, 0.13 });
			pupil->setMaterial(blue);
			head->add_child(pupil);
		};

		addEye(-1, "r");
		addEye(1, "l");

		auto addLimb = [&](float width, float length, SceneNode* parent, Material* material, std::string name) {
			auto base = add_sphere(name + "_base");
			base->scale({width, width, width});
			base->translate({0, 0, 0});
			base->setMaterial(material);
			parent->add_child(base);

			auto limb = new GeometryNode(name, new Mesh("cylinder.obj"));
			limb->translate({0, -1, 0});
			limb->scale({width, length, width});
			limb->translate({0, 0, 0});
			limb->setMaterial(material);
			parent->add_child(limb);
		};

		//arm
		auto addArm = [&](float offset, std::string prefix)->arm {
			auto j_shoulder = new SceneNode(prefix + "_shoulder");
			j_shoulder->translate({ offset * 0.24, 0.565, 0.04 });
			torso2->add_child(j_shoulder);

			addLimb(0.056, 0.26, j_shoulder, green, prefix + "upper_arm");

			auto j_elbow = new SceneNode(prefix + "_elbow");
			j_elbow->translate({ 0, -0.26, 0.0 });
			j_shoulder->add_child(j_elbow);

			addLimb(0.056, 0.26, j_elbow, green, prefix + "forearm");

			auto g_s = add_sphere(prefix + "_sleeve1");
			g_s->scale({ 0.08, 0.165, 0.08 });
			g_s->translate({ 0, -0.26 + 0.13, 0 });
			g_s->setMaterial(green);
			j_elbow->add_child(g_s);

			auto g_s2 = add_sphere(prefix + "_sleeve2");
			g_s2->scale({ 0.09, 0.1, 0.09 });
			g_s2->translate({ 0, -0.26 + 0.075, 0 });
			g_s2->setMaterial(green);
			j_elbow->add_child(g_s2);

			auto g_wrist = add_cylinder(prefix + "_wrist");
			g_wrist->scale({ 0.062, 0.036, 0.062 });
			g_wrist->translate({ 0, -0.26, 0 });
			g_wrist->setMaterial(red);
			j_elbow->add_child(g_wrist);

			auto j_hand = new SceneNode(prefix + "_hand");
			j_hand->translate({ 0, -0.26, 0.0 });
			j_elbow->add_child(j_hand);

			auto addFinger = [&](float z, float y, float scale, SceneNode* parent, std::string prefix) {
				auto base = new SceneNode(prefix + "_base");
				base->scale({ scale, scale, scale });
				base->translate({ 0, y, z });
				parent->add_child(base);

				auto g_f0 = add_sphere(prefix + "_g_f0");
				g_f0->scale({ 0.4, 1, 0.4 });
				g_f0->translate({ 0, -0.4, 0 });
				g_f0->setMaterial(skin);
				base->add_child(g_f0);

				auto g_f1 = add_sphere(prefix + "_g_f1");
				g_f1->scale({ 0.4, 1, 0.4 });
				g_f1->translate({ 0, -1.6, 0 });
				g_f1->setMaterial(skin);
				base->add_child(g_f1);

				auto g_f2 = add_sphere(prefix + "_g_f2");
				g_f2->scale({ 0.4 * 0.9, 1 * 0.6, 0.4 * 0.9 });
				g_f2->translate({ 0, -2.7, 0 });
				g_f2->setMaterial(skin);
				base->add_child(g_f2);
			};

			auto g_hand = add_sphere(prefix + "_hand");
			g_hand->translate({ 0, -1, 0 });
			g_hand->scale({ 0.033, 0.065, 0.047 });
			g_hand->setMaterial(skin);
			j_hand->add_child(g_hand);

			addFinger(0.03, -0.11, 0.032, j_hand, prefix + "_fing0");
			addFinger(0.01, -0.12, 0.035, j_hand, prefix + "_fing1");
			addFinger(-0.01, -0.12, 0.030, j_hand, prefix + "_fing2");
			addFinger(-0.03, -0.11, 0.024, j_hand, prefix + "_fing3");

			auto thumb = new SceneNode("thumb");
			thumb->rotate('z', offset * -10);
			thumb->translate({ offset * -0.025, -0.05, 0.03 });
			j_hand->add_child(thumb);
			addFinger(0, 0, 0.032, thumb, prefix + "_thumb");


			return arm{ j_shoulder,
					j_elbow,
					j_hand };
		};

		auto r_arm = addArm(-1, "r");
		auto l_arm = addArm(1, "l");

		//leg
		auto addLeg = [&](float offset, std::string prefix)->leg {
			auto j_hip = new SceneNode(prefix + "_hip");
			j_hip->translate({ offset * 0.11458, -0.11052, 0.04844 });
			torso2->add_child(j_hip);

			addLimb(0.07, 0.34817, j_hip, pant, prefix + "_thigh");

			auto j_knee = new SceneNode(prefix + "_knee");
			j_knee->translate({ offset * 0.0, -0.34817, 0.0 });
			j_hip->add_child(j_knee);

			addLimb(0.07, 0.34817, j_knee, pant, prefix + "_calf");

			auto g_k3 = add_cylinder(prefix + "_g_k3");
			g_k3->scale({ 0.073, 0.029, 0.073 });
			g_k3->translate({ offset * 0, -0.3606, 0 });
			g_k3->setMaterial(white);
			j_knee->add_child(g_k3);

			auto j_foot = new SceneNode(prefix + "_foot");
			j_foot->translate({ offset * 0.0, -0.38, 0.0 });
			j_knee->add_child(j_foot);

			auto g_f1 = add_cylinder(prefix + "_g_f1");
			g_f1->scale({ 0.048, 0.035, 0.055 });
			g_f1->translate({ offset * 0, 0, 0 });
			g_f1->setMaterial(red);
			j_foot->add_child(g_f1);

			auto g_f2 = add_sphere(prefix + "_g_f2");
			g_f2->scale({ 0.055, 0.049, 0.055 });
			g_f2->translate({ offset * 0, -0.04347, -0.03564 });
			g_f2->setMaterial(red);
			j_foot->add_child(g_f2);

			auto g_f3 = add_sphere(prefix + "_g_f3");
			g_f3->scale({ 0.057, 0.043, 0.090 });
			g_f3->translate({ offset * -0.01015, -0.05196, 0.04045 });
			g_f3->setMaterial(red);
			j_foot->add_child(g_f3);

			auto g_f4 = add_sphere(prefix + "_g_f4");
			g_f4->scale({ 0.057, 0.034, 0.089 });
			g_f4->translate({ offset * -0.01188, -0.055, 0.10184 });
			g_f4->setMaterial(red);
			j_foot->add_child(g_f4);

			auto g_f5 = add_cylinder(prefix + "_g_f5");
			g_f5->scale({ 0.06, 0.010, 0.081 });
			g_f5->translate({ offset * -0.00765, -0.08342, -0.01105 });
			g_f5->setMaterial(white);
			j_foot->add_child(g_f5);

			auto g_f6 = add_cylinder(prefix + "_g_f6");
			g_f6->scale({ 0.06, 0.010, 0.108 });
			g_f6->translate({ offset * -0.01336, -0.08342, 0.08163 });
			g_f6->setMaterial(white);
			j_foot->add_child(g_f6);

			return leg{ j_hip,
						 j_knee,
						 j_foot };
		};

		auto r_leg = addLeg(-1, "r");
		auto l_leg = addLeg(1, "l");

		//tail
		auto simpleLimb = [&](float width, float length, SceneNode* parent, Material* material, std::string name) {
			auto limb = add_sphere(name);
			limb->translate({0, -0.65, 0});
			limb->scale({width, (1 / 0.65) * length / 2, width});
			limb->translate({0, 0, 0});
			limb->setMaterial(material);
			parent->add_child(limb);
		};

		auto j_tail = new SceneNode("tail");
		j_tail->rotate('y', 90);
		j_tail->rotate('x', 30);
		torso2->add_child(j_tail);

		simpleLimb(0.056, 0.135 * 2, j_tail, skin, "g_tail0");

		auto n_t1 = new SceneNode("tail1");
		n_t1->rotate('z', 30);
		n_t1->translate({0, -0.135 * 2, 0});
		j_tail->add_child(n_t1);
		simpleLimb(0.04, 0.10 * 2, n_t1, skin, "g_tail1");

		auto g_tail_tip = add_sphere("g_tail_tip");
		g_tail_tip->translate({0, -0.65, 0});
		g_tail_tip->scale({0.04, 0.075, 0.04});
		g_tail_tip->rotate('z', 30);
		g_tail_tip->translate({0, -0.20, 0});
		g_tail_tip->setMaterial(skin);
		n_t1->add_child(g_tail_tip);


		auto scale = 10 / 9.25;
		rootnode->scale({scale, scale, scale});

		return body{
				rootnode,
				neck,
				head,
				l_arm,
				r_arm,
				l_leg,
				r_leg,
				j_tail };
	};

		// ~~~~~~~~~~~~~~~~~~~~~~~~~ START OF SCENE ~~~~~~~~~~~~~~~~~~~~~
	auto scene = new SceneNode("scene");

	auto bigCoat = add_bigCoat(red, green, skin);
	bigCoat.root->rotate('y', -35);
	bigCoat.root->translate({0.0, 11, -30.0});

#define rotate_joint pre_rotate

	bigCoat.head->rotate_joint('x', 1.17);
	bigCoat.head->rotate_joint('y', 40.78);

	bigCoat.neck->rotate_joint('x', 5.16);

	bigCoat.r_arm.shoulder->rotate_joint('x', -75);
	bigCoat.r_arm.elbow->rotate_joint('x', -70);
	bigCoat.r_arm.hand->rotate_joint('x', -15);

	bigCoat.l_arm.shoulder->rotate_joint('x', 11);
	bigCoat.l_arm.elbow->rotate_joint('x', -15);
	bigCoat.l_arm.hand->rotate_joint('x', 0);

	bigCoat.r_leg.hip->rotate_joint('x', -18);
	bigCoat.r_leg.knee->rotate_joint('x', 27);

	auto bigCoat2 = add_bigCoat(orange, blue, plae_skin);
	bigCoat2.root->rotate('y', 35);
	bigCoat2.root->translate({5.0, 11, -35.0});

	bigCoat2.head->rotate_joint('y', -40);

	scene->add_child(bigCoat.root);
	scene->add_child(bigCoat2.root);

	auto lamp_dist = 40;
	auto lamp_num = 20;

	auto sidewalk_dist = 10;
	auto sidewalk_num = lamp_dist * lamp_num / sidewalk_dist;

	auto street_width = 40;

	auto street_root = new SceneNode("street_root");
	street_root->rotate('y', -60);
	street_root->translate({10, 0, -40});
	scene->add_child(street_root);

	auto street = new SceneNode("street");
	street_root->add_child(street);

	auto opposite_street = new SceneNode("opposite_street");
	street_root->add_child(opposite_street);
	opposite_street->add_child(street);
	opposite_street->rotate('y', 180);
	opposite_street->translate({0, 0, 2 * street_width});

	auto road = new GeometryNode("road", new Cube());
	street->add_child(road);
	road->setMaterial(asphalt);
	road->translate({-0.5, -0.5, 0});
	road->scale({2 * lamp_dist * lamp_num, 0.5, street_width});

	for (int i = 0; i < sidewalk_num; i++) {
		auto sidewalk_square = new GeometryNode("sidewalk_" + i, new Cube());
		sidewalk_square->scale({sidewalk_dist * 0.9, 1, 15});
		sidewalk_square->translate({sidewalk_dist * (i - sidewalk_num / 2), -0.5, 0});
		sidewalk_square->setMaterial(concrete);
		street->add_child(sidewalk_square);
	}

	for (int i = 0; i < lamp_num; i++) {
		auto lamp = add_Lamp(std::string("lamp_") + std::to_string(i));
		lamp->translate({lamp_dist * (i - lamp_num / 2), 0, 0});
		street->add_child(lamp);
	}

	auto street_car = add_car("street_car", blue);
	street_root->add_child(street_car);
	street_car->rotate('y', 0);
	street_car->translate({50, 1, 25});

	//auto glass = gr.dialectric_material(1.5);
	auto glass = white;
	auto glass_ball = new GeometryNode("sphere glass", new Sphere{});
	glass_ball->setMaterial(glass);
	glass_ball->scale({3, 3, 3});
	glass_ball->translate({5, 5, -25});
	scene->add_child(glass_ball);

	auto chrome = new PhongMaterial({ 0.0, 0.0, 0.0 }, { 1.0, 1.0, 1.0 }, 100);
	auto chrome_ball = new GeometryNode("sphere chrome", new Sphere{});
	chrome_ball->setMaterial(chrome);
	chrome_ball->scale({8, 8, 8});
	chrome_ball->translate({-10, 5, -35});
	scene->add_child(chrome_ball);

	auto random_float = [](float min = 0, float max = 1) -> float {
		return (rand() / static_cast <float> (RAND_MAX)) * (max - min) + min;
	};

	auto add_random_object = [&](int i) {
		auto spec = random_float();
		auto shininess = random_float(1, 128);

		auto m = new PhongMaterial({ random_float(), random_float(), random_float() },
			{ spec, spec, spec },
			shininess);


		auto s = new GeometryNode("sphere" + i, new Sphere{});
		auto size = 2;
		s->scale({ size, size, size });
		s->translate({ 2 * 25 * (random_float() - 0.5), size, -20 - random_float(0, 45) });
		s->setMaterial(m);

		scene->add_child(s);
	};

	for (int i = 1; i < 25; i++) {
		add_random_object(i);
	}

	auto car = add_car("car", orange);
	car->rotate('y', 45);
	car->translate({15, 2, -70});
	scene->add_child(car);

	// add the ground
	auto plane = new GeometryNode("plane", new Mesh("plane.obj"));
	scene->add_child(plane);
	plane->setMaterial(grass);
	plane->scale({200, 1, 200});
	plane->translate({0, 0, -30});

	// The lights
	auto l1 = new Light({ 200,200,400 }, { 0.8, 0.8, 0.8 }, { 1, 0, 0 });
	auto l2 = new Light({ -10, 15, -28 }, { 0.8, 0.4, 0.4 }, { 1, 0, 0.005 });

	auto world_lights = { l1, l2 };

	// gr.render(scene, "bigCoat.png", 128, 128, 
	//	  {0, 18, 5}, {0, 13, -35}, {0, 1, 0}, 50,
	//      {0.4, 0.4, 0.4}, {l2})
	Image screenshot(128, 128);
	renderToWindow(scene, screenshot, { 0, 18, 5 }, { 0, 13, -35 }, { 0, 1, 0 }, 50, { 0.4, 0.4, 0.4 }, { l2 });
	screenshot.savePng("bigCoat.png");

	scene_info = SceneInfo{ { 0, 18, 5 }, { 0, 13, -35 }, { 0, 1, 0 }, 50, { 0.4, 0.4, 0.4 }, {l2} };

	return scene;
}