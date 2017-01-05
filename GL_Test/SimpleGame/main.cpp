#include "SDL.h"
//#include "SDL_keyboard.h"
//#include "SDL_keycode.h"

#include <vector>
#include <queue>
#include <list>
#include "math.h"
#include "GL/glew.h"
#include "glm/glm.hpp"//includes most (if not all?) of GLM library
#include "glm/gtx/transform.hpp"
#include "glm/gtc/type_ptr.hpp"

#include "GL/glew.h"

#include "mesh_utils.h"
#include "shader_utils.h"

#include <iostream>

#include <random>

#include "collision_utils.h"
#include "OctTree.h"

Collider *player;
Collider *opponent;

Mesh* mesh;
OBB *third;
std::vector<glm::vec3> dots;

glm::vec3 playerpos = glm::vec3(0, 0, 0);

glm::vec3 cameraPos = glm::vec3(0, 0, 10);
glm::quat cameraRot = glm::quat();

void drawLine(const glm::vec3 pos, glm::vec3 pos2, glm::mat4 &mvp, glm::vec3 colour = glm::vec3(0, 0, 0)) {
	using namespace std;

	GLuint vao;
	GLuint vbo;
	GLuint vbo_index;
	GLuint program;
	GLuint attrib_coord;
	GLuint attrib_index;
	GLuint uniform_matrix;

	GLfloat vertices[] = {
		pos.x, pos.y, pos.z,
		pos2.x, pos2.y, pos2.z,
	};

	std::vector<GLfloat> skeleton;

	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	struct shaderInfo shaders[] = { { GL_VERTEX_SHADER, "vs_wireframe.glsl" },
	{ GL_FRAGMENT_SHADER, "fs_wireframe.glsl" } };

	program = LoadProgram(shaders, 2);
	glUseProgram(program);

	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	attrib_coord = getAttrib(program, "coord");
	glEnableVertexAttribArray(attrib_coord);
	glVertexAttribPointer(
		attrib_coord,	//attrib 'index'
		3,				//pieces of data per index
		GL_FLOAT,		//type of data
		GL_FALSE,		//whether to normalize
		0,				//space b\w data
		0				//offset from buffer start
		);

	uniform_matrix = getUniform(program, "mvp");

	GLuint uniform_colour = getUniform(program, "colour");
	glUniform3fv(uniform_colour, 1, glm::value_ptr(colour));

	glUniformMatrix4fv(uniform_matrix, 1, GL_FALSE, glm::value_ptr(mvp));

	int size;  glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &size);

	glDrawArrays(GL_LINES, 0, 2);

	glDeleteBuffers(1, &vbo);
	glDeleteVertexArrays(1, &vao);
	glDeleteProgram(program);

}
void drawDot(const glm::vec3 pos, const glm::mat4 &mvp, glm::vec3 colour = glm::vec3(0, 0, 0)) {
	using namespace std;

	GLuint vao;
	GLuint vbo;
	GLuint vbo_index;
	GLuint program;
	GLuint attrib_coord;
	GLuint attrib_index;
	GLuint uniform_matrix;

	GLfloat vertices[] = {
		pos.x, pos.y, pos.z,
	};

	std::vector<GLfloat> skeleton;

	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	struct shaderInfo shaders[] = { { GL_VERTEX_SHADER, "vs_wireframe.glsl" },
	{ GL_FRAGMENT_SHADER, "fs_wireframe.glsl" } };

	program = LoadProgram(shaders, 2);
	glUseProgram(program);

	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	attrib_coord = getAttrib(program, "coord");
	glEnableVertexAttribArray(attrib_coord);
	glVertexAttribPointer(
		attrib_coord,	//attrib 'index'
		3,				//pieces of data per index
		GL_FLOAT,		//type of data
		GL_FALSE,		//whether to normalize
		0,				//space b\w data
		0				//offset from buffer start
		);

	uniform_matrix = getUniform(program, "mvp");

	GLuint uniform_colour = getUniform(program, "colour");
	glUniform3fv(uniform_colour, 1, glm::value_ptr(colour));

	glUniformMatrix4fv(uniform_matrix, 1, GL_FALSE, glm::value_ptr(mvp));

	int size;  glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &size);

	glDrawArrays(GL_POINTS, 0, 1);

	glDeleteBuffers(1, &vbo);
	glDeleteVertexArrays(1, &vao);
	glDeleteProgram(program);

}

void Init_shapes() {
	opponent = new OBB(glm::vec3(0, 0, 0), glm::vec3(1, 1, 1));
	player = new OBB(glm::vec3(0,0,0), glm::vec3(1,1,1));

	struct shaderInfo shaders[] = { { GL_VERTEX_SHADER, "vs.glsl" },
	{ GL_FRAGMENT_SHADER, "fs.glsl" } };

	GLuint prog = LoadProgram(shaders, 2);
	mesh = new Mesh("ship.obj", prog);


	//
	GLfloat* vertices;
	int size;

	glBindBuffer(GL_COPY_READ_BUFFER, mesh->vbo_coord);
	vertices = (GLfloat*)glMapBuffer(GL_COPY_READ_BUFFER, GL_READ_ONLY);
	glGetBufferParameteriv(GL_COPY_READ_BUFFER, GL_BUFFER_SIZE, &size);

	third = new OBB((glm::vec3*)vertices, size / sizeof(glm::vec3));

	glUnmapBuffer(GL_COPY_READ_BUFFER);
	glBindBuffer(GL_COPY_READ_BUFFER, -1);

//	for (int i = 0; i < 55; i++) {
//		dots.push_back(glm::vec3(rand() %50 - 50, rand()%100 + 25, rand() %100 - 5));
//	}
//	third = new OBB(dots.data(), dots.size());
}

//does a big old copy, kinda slow?
std::vector<Entity*> fill_level(const glm::vec3 &levelBound, int num) {
	std::vector<Entity*> result(num);

	for (int i = 0; i < num; i++) {
		glm::vec3 min(rand() % (int)levelBound.x, rand() % (int)levelBound.y, rand() % (int)levelBound.z);
		glm::vec3 max = min + glm::vec3(2, 2, 2);

		result[i] = new Entity(new OBB(min, max));
	}

	return result;
}

void handleKeyUp(const char key) {

}
void handleKeyDown(const char key) {
	switch (key) {
	case 'W':
	case 'w':player->translate(cameraRot*glm::vec3(0, 0, -1)); playerpos += cameraRot*glm::vec3(0, 0, -1); break;
	case 'S':
	case 's':player->translate(cameraRot*glm::vec3(0, 0, 1));  playerpos += cameraRot*glm::vec3(0, 0, 1);  break;
	case 'A':
	case 'a':player->translate(cameraRot*glm::vec3(-1, 0, 0)); playerpos += cameraRot*glm::vec3(-1, 0, 0);  break;
	case 'D':
	case 'd':player->translate((cameraRot*glm::vec3(1, 0, 0))); playerpos += cameraRot*glm::vec3(1, 0, 0);  break;
	case 'Q':
	case 'q':player->translate(cameraRot*glm::vec3(0, -1, 0)); playerpos += cameraRot*glm::vec3(0, -1, 0); break;
	case 'E':
	case 'e':player->translate(cameraRot*glm::vec3(0, 1, 0)); playerpos += cameraRot*glm::vec3(0, 1, 0);  break;
	case 'Z':
	case 'z':player->rotate(glm::quat(cos(M_PI / 12), (float)sin(M_PI / 12)*(cameraRot*glm::vec3(0, 1, 0)))); break;
	case 'C':
	case 'c':player->rotate(glm::quat(-cos(M_PI / 12), (float)sin(M_PI / 12)*(cameraRot*glm::vec3(0, 1, 0)))); break;
	case 'R':
	case 'r': player->rotate(glm::quat(cos(M_PI / 12), (float)sin(M_PI / 12)*(cameraRot*glm::vec3(1, 0, 0))));  break;
	case 'F':
	case 'f': player->rotate(glm::quat(-cos(M_PI / 12), (float)sin(M_PI / 12)*(cameraRot*glm::vec3(1, 0, 0))));  break;
	}
}

//mouse movemet
void handleMotion(int x, int y) {
	float dx = M_PI / 180 * x / 10;
	float dy = M_PI / 180 * y / 10;

	cameraRot = glm::quat(-cos(dx / 2), (float)sin(dx / 2)*(cameraRot*glm::vec3(0, 1, 0)))*cameraRot;
	cameraRot = glm::quat(-cos(dy / 2), (float)sin(dy / 2)*(cameraRot*glm::vec3(1, 0, 0)))*cameraRot;

}

AABB levelRegion(glm::vec3(-256, -256, -256), glm::vec3(256, 256, 256));
OctTree *collisionTree;
std::vector<Entity*> Entitys;
std::vector<CollisionInfo> collisionList;
int Entitynum = 50;
int main(int argc, char *argv[]) {
	SDL_Window *window;
	OBB ball(glm::vec3(-1, -1, -1), glm::vec3(1, 1, 1));

	//initializing SDL window
	SDL_Init(SDL_INIT_VIDEO);
	window = SDL_CreateWindow("Super Amazing Fun Times", 100, 100, 640, 480, SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL);
	SDL_SetRelativeMouseMode(SDL_TRUE);

	//initializing Opengl stuff
	SDL_GL_CreateContext(window);
	glewInit();
	glEnable(GL_DEPTH_TEST);
	
	//initialize the models
	Init_shapes();

	Entitys = fill_level(levelRegion.max, Entitynum);
	Entitys.push_back(new Entity(player));

	collisionTree = new OctTree(levelRegion, Entitys);
	collisionTree->UpdateTree();

	int timeCurr = SDL_GetTicks();
	int timeLast = timeCurr;

	//main loop
	glEnable(GL_PROGRAM_POINT_SIZE);
	glPointSize(10);

	while (1) {
		timeLast = timeCurr;
		timeCurr = SDL_GetTicks();

		//polling for events
		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT) return 0;
			if (event.type == SDL_KEYDOWN) handleKeyDown(event.key.keysym.sym);
			if (event.type == SDL_KEYUP) handleKeyUp(event.key.keysym.sym);
			if (event.type == SDL_MOUSEMOTION) handleMotion(event.motion.xrel, event.motion.yrel);
		}

		//collision detection
		if (player->intersect(*opponent)) {
			std::cout << "it collided" << std::endl;
			opponent->colour = player->colour = glm::vec3(1, 1, 0);
		}
		else {
			opponent->colour = player->colour = glm::vec3(1, 0, 0);
		}


		//process the collision
		collisionList.clear();

		collisionTree->UpdateTree();
		collisionTree->CheckCollision(collisionList);

		//the drawing happens here
		glClearColor(1.0, 1.0, 1.0, 1.0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		cameraPos = playerpos + cameraRot*glm::vec3(0, 3, 10);
//		boxes[0]->rot = cameraRot;
//		boxes[0]->calcAABB();
		glm::mat4 view = glm::inverse(glm::mat4_cast(cameraRot))*glm::translate(glm::mat4(1.0), -cameraPos);
		//glm::mat4 view = glm::lookAt(glm::vec3(0.0, 0.0, -10.0), glm::vec3(0.0, 0.0, 4.0), glm::vec3(0.0, 1.0, 0.0));//I don't necessarily like this
		glm::mat4 projection = glm::perspective(45.0f, 1.0f * 600 / 480, 0.1f, 1000.0f);

		glm::mat4 vp = projection*view;//combination of model, view, projection matrices
									   //mvp = model;

		glm::mat4 model = glm::mat4(1.0f);//glm::translate(glm::mat4(1.0f), boxes[i]->pos)*glm::scale(boxes[i]->scale)*glm::mat4_cast(boxes[i]->rot);
		glm::mat4 mvp = vp*model;
		
		std::cout << collisionList.size() << std::endl;
		for (int i = 0; i < collisionList.size(); i++) {
			collisionList[i].member[0]->bound.colour = glm::vec3(1, 1, 0);
			collisionList[i].member[1]->bound.colour = glm::vec3(1, 1, 0);
			collisionList[i].member[0]->translate(-collisionList[i].collision.normal*collisionList[i].collision.depth);
			drawDot(collisionList[i].collision.point, vp, glm::vec3(1, 0, 0));
			drawLine(collisionList[i].collision.point, collisionList[i].collision.point - collisionList[i].collision.normal*collisionList[i].collision.depth, vp, glm::vec3(0, 0, 1));
		}

		collisionTree->draw(vp);

		opponent->draw(mvp);
		player->draw(mvp);

		for (int i = 0; i < Entitys.size(); i++) {
			Entitys[i]->Tick(0.016);
			Entitys[i]->draw(vp);
		}
		//levelRegion.draw(vp);

		third->draw(mvp);
		mesh->Draw(mvp);
		for (int i = 0; i < dots.size(); i++) {
			drawDot(dots[i], mvp);
		}

		SDL_GL_SwapWindow(window);

	}

	//	return 0;
}

#if 0
#include "SDL.h"
//#include "SDL_keyboard.h"
//#include "SDL_keycode.h"

#include <vector>
#include <queue>
#include <list>
#include "math.h"
#include "GL/glew.h"
#include "glm/glm.hpp"//includes most (if not all?) of GLM library
#include "glm/gtx/transform.hpp"
#include "glm/gtc/type_ptr.hpp"

#include "GL/glew.h"

#include "mesh_utils.h"
#include "shader_utils.h"

#include <iostream>

#include <random>

#include "collision_utils.h"

//forward declarations
class Entity;
class AABB;
class OctTree;

std::vector<Entity*> boxes;
static int comp;
glm::vec3 tempV (0, 0, 0);

//could be a static function of AABB

class OctTree {
private:
	AABB region;
	std::vector<Entity*> objList;

	OctTree *child[8];
	OctTree *parent;

	static const int minSize = 1;
	static const int minObj = 10;
	static const int maxDepth = 20;
	static std::queue<Entity*> pendingInsertion;
	bool treeBuilt = false;
	bool treeReady = false;

	int depth;

	std::vector<Entity*> collisionList;

private:
	void BuildTree();
	void Insert(Entity*);

	void CheckForMove();
	bool Reposition(Entity*);
public:
	OctTree(AABB region);
	OctTree(AABB region, std::vector<Entity*> objList);
	OctTree(AABB region, std::vector<Entity*> objList, OctTree* parent);

	void UpdateTree();
	void CheckCollision();
	void CheckCollision(std::vector<Entity*> &incObjList);

	AABB getRegion() { return region; }

	void draw(const glm::mat4 &vp);

	void DrawContainingRegion(Entity* obj, glm::mat4 &vp);

};

void OctTree::draw(const glm::mat4 &vp) {
	bool isLeaf = true;
	for (int i = 0; i < 8; i++) {
		if (child[i] != nullptr) {
			child[i]->draw(vp);
		}
	}

	if (objList.size()>0) {
		region.draw(vp);
	}

}

class Entity : public IGJKFcns {
public:
	Mesh *mesh;
	AABB bound;

	glm::vec3 pos;
	glm::vec3 scale;
	glm::quat rot;

	glm::vec3 vel;
	bool hasMoved = false;

	void calcAABB() {
		GLfloat* vertices;
		int size;

		glBindBuffer(GL_COPY_READ_BUFFER, mesh->vbo_coord);
		vertices = (GLfloat*)glMapBuffer(GL_COPY_READ_BUFFER, GL_READ_ONLY);
		glGetBufferParameteriv(GL_COPY_READ_BUFFER, GL_BUFFER_SIZE, &size);

		//calculations would be better done on GPU w\ openGL call, but meh
		//also, might be better to just to rotation, and then do the translation on the resulting two point
		for (int i = 0; i < size / sizeof(GLfloat); i += 3) {
			glm::vec3 tempV = glm::vec3(
				glm::translate(glm::mat4(1.0f), pos)
				*glm::scale(scale)
				*glm::mat4_cast(rot)
				*glm::vec4(vertices[i], vertices[i + 1], vertices[i + 2], 1.0));
			
			if (i == 0) {
				bound.min = tempV;
				bound.max = tempV;
			}
			else {
				if (tempV.x < bound.min.x) {
					bound.min.x = tempV.x;
				}
				else if (tempV.x > bound.max.x) {
					bound.max.x = tempV.x;
				}

				if (tempV.y < bound.min.y) {
					bound.min.y = tempV.y;
				}
				else if (tempV.y > bound.max.y) {
					bound.max.y = tempV.y;
				}

				if (tempV.z < bound.min.z) {
					bound.min.z = tempV.z;
				}
				else if (tempV.z > bound.max.z) {
					bound.max.z = tempV.z;
				}
			}
		}


		glUnmapBuffer(GL_COPY_READ_BUFFER);
		glBindBuffer(GL_COPY_READ_BUFFER, -1);
	}

	virtual glm::vec3 furthestPointInDirection(glm::vec3 dir) {
		glm::vec3 result;
		GLfloat length;
		GLfloat* vertices;
		int size;

		//dir = glm::inverse(glm::mat3(glm::mat4_cast(rot)))*dir;

		glBindBuffer(GL_COPY_READ_BUFFER, mesh->vbo_coord);
		vertices = (GLfloat*)glMapBuffer(GL_COPY_READ_BUFFER, GL_READ_ONLY);
		glGetBufferParameteriv(GL_COPY_READ_BUFFER, GL_BUFFER_SIZE, &size);

		for (int i = 0; i < size / sizeof(GLfloat); i += 3) {
			//it would be more efficient to multiply the dir by the inverse, rather than mult every point by the trans
			glm::vec3 tempV = glm::vec3(
				glm::translate(glm::mat4(1.0f), pos)
				*glm::scale(scale)
				*glm::mat4_cast(rot)
				*glm::vec4(vertices[i], vertices[i + 1], vertices[i + 2], 1.0));

			GLfloat tempLength = glm::dot(dir, tempV);

			if (i == 0) {
				result = tempV;
				length = tempLength;
			}
			else {
				if (tempLength > length) {
					result = tempV;
					length = tempLength;
				}
			}
		}


		glUnmapBuffer(GL_COPY_READ_BUFFER);
		glBindBuffer(GL_COPY_READ_BUFFER, -1);

		return result;
	}
public:
	Entity(Mesh *mesh, GLfloat x, GLfloat y, GLfloat z) : mesh(mesh), pos(glm::vec3(x, y, z)) {

	}

	void translate(glm::vec3 offset) {
		pos += offset;
		bound.min += offset;
		bound.max += offset;
		hasMoved = true;
	}
	void translateTo(glm::vec3 newPos) {
		translate(newPos - pos);
	}
};


//class Declarations~~~~~~~~~~~~~~~~~
std::queue<Entity*> OctTree::pendingInsertion = std::queue<Entity*>();

OctTree::OctTree(AABB region) :region(region), parent(nullptr), depth(0) {
}

OctTree::OctTree(AABB region, std::vector<Entity*> objList) : region(region), objList(objList), parent(nullptr), depth(0) {
}
OctTree::OctTree(AABB region, std::vector<Entity*> objList, OctTree* parent) : region(region), objList(objList), parent(parent), depth(parent->depth + 1) {
}
void OctTree::UpdateTree() {

	if (!treeBuilt) {
		while (pendingInsertion.size() > 0) {
			//objList.insert(objList.end(), pendingInsertion.front());
			objList.push_back(pendingInsertion.front());//looks a bit nicer
			pendingInsertion.pop();
		}
		BuildTree();
		treeBuilt = true;
	}
	else {
//		while (pendingInsertion.size() > 0) {
//			Insert(pendingInsertion.front());
//			pendingInsertion.pop();
//		}
		//doing this instead of rebuilding the tree can shave a a fraction of time off, when in debug mode
		//in release build it will prolly be less than a millisecond of savings, but #yolo
		CheckForMove();
	}

	treeReady = true;
}


void OctTree::DrawContainingRegion(Entity* obj, glm::mat4 &vp) {
	
	std::vector<Entity*>::iterator it;
	std::vector<Entity*>::iterator jt;
	std::vector<Entity*>::iterator id;
	std::vector<Entity*>::iterator jd;
	std::vector<Entity*>::iterator kt;

	id = objList.end();

	for (it = objList.begin(); it != id; it++) {
		if (*it == obj) {
			region.draw(vp);
		}
	}

	for (int i = 0; i < 8; i++) {
		if (child[i] != nullptr) {
			child[i]->DrawContainingRegion(obj, vp);
		}
	}
}

void OctTree::CheckForMove() {
	
	std::vector<Entity*>::iterator it;
	std::vector<Entity*>::iterator jt;
	std::vector<Entity*>::iterator id;
	std::vector<Entity*>::iterator jd;
	std::vector<Entity*>::iterator kt;

	id = objList.end();
	it = objList.begin();
	while (it != id) {
		if ((*it)->hasMoved) {
			(*it)->mesh->collision = 3;
			if (Reposition(*it)) {
				(*it)->mesh->collision = 4;
				it = objList.erase(it);
				id = objList.end();
			}
			else {
				it++;
			}
		}
		else {
			it++;
		}
	}
	for (; it != id; it++) {
		if ((*it)->hasMoved) {
			(*it)->mesh->collision = 3;
			if (Reposition(*it)) {
				(*it)->mesh->collision = 4;
				it = objList.erase(it);
				if (it == objList.end())
					break;//probably a for loop is a bad call
				it--;
				id = objList.end();
			}
		}
	}

	for (int i = 0; i < 8; i++) {
		if (child[i] != nullptr) {
			
			child[i]->CheckForMove();
			
		}
	}
}

//doing this recursively, but could probably do it iteratively
bool OctTree::Reposition(Entity* obj) {
	bool canCreateChild;
	bool inPos = false;
	OctTree* curNode = this;
	bool dropped;

	while (1) {
		dropped = false;
		glm::vec3 dimensions = curNode->region.max - curNode->region.min;
		if (!(//check if we're in a position where we'll be able to move down, or create a new node
			(curNode->objList.size() < minObj) ||
			(dimensions.x < minSize && dimensions.y < minSize && dimensions.z < minSize) ||
			curNode->depth >= maxDepth
		)){
			//divide the current tree into octants
			glm::vec3 center = curNode->region.min + dimensions / 2.0f;
			AABB octant[8];

			octant[0] = AABB(region.min, center);
			octant[1] = AABB(glm::vec3(region.min.x, center.y, region.min.z), glm::vec3(center.x, region.max.y, center.z));
			octant[2] = AABB(glm::vec3(center.x, region.min.y, region.min.z), glm::vec3(region.max.x, center.y, center.z));
			octant[3] = AABB(glm::vec3(center.x, center.y, region.min.z), glm::vec3(region.max.x, region.max.y, center.z));
			octant[4] = AABB(glm::vec3(region.min.x, region.min.y, center.z), glm::vec3(center.x, center.y, region.max.z));
			octant[5] = AABB(glm::vec3(region.min.x, center.y, center.z), glm::vec3(center.x, region.max.y, region.max.z));
			octant[6] = AABB(glm::vec3(center.x, region.min.y, center.z), glm::vec3(region.max.x, center.y, region.max.z));
			octant[7] = AABB(glm::vec3(center.x, center.y, center.z), glm::vec3(region.max.x, region.max.y, region.max.z));

			for (int i = 0; i < 8; i++) {
				if (octant[i].contains(obj->bound)) {
					if (curNode->child[i] != nullptr) {
						curNode = curNode->child[i];
						dropped = true;
					}
					else {
						std::vector<Entity*> octList;
						octList.push_back(obj);
						curNode->child[i] = new OctTree(octant[i], octList, this);
						curNode->child[i]->BuildTree();
						return true;//building the tree will take care of it, we can just return
					}
				}
			}
		}
		if (!dropped) {
			if (curNode->region.contains(obj->bound) || curNode->parent == nullptr) {
				//didn't fit into any of the children, but does fit this one
				//so this is where we want to be
				if (curNode != this) {
					curNode->objList.push_back(obj);
					return true;
				}
				else {
					return false;

				}
			}
			else {
				curNode = curNode->parent;
			}
		}
	}

}

void OctTree::Insert(Entity* obj) {
	std::cout << "Insert: this shouldn't be run\n";
}

void OctTree::BuildTree() {
	//return conditions for this recursive fcn
	glm::vec3 dimensions = region.max - region.min;
	if ((objList.size() <= minObj) ||
		(dimensions.x < minSize && dimensions.y < minSize && dimensions.z < minSize) ||
		depth >= maxDepth) {
		for (int i = 0; i < 8; i++) {
			child[i] = nullptr;
		}

		return;
	}

	//divide the current tree into octants
	glm::vec3 center = region.min + dimensions / 2.0f;

	AABB octant[8];
	std::vector<Entity*> octList[8];

	octant[0] = AABB(region.min, center);
	octant[1] = AABB(glm::vec3(region.min.x, center.y, region.min.z), glm::vec3(center.x, region.max.y, center.z));
	octant[2] = AABB(glm::vec3(center.x, region.min.y, region.min.z), glm::vec3(region.max.x, center.y, center.z));
	octant[3] = AABB(glm::vec3(center.x, center.y, region.min.z), glm::vec3(region.max.x, region.max.y, center.z));
	octant[4] = AABB(glm::vec3(region.min.x, region.min.y, center.z), glm::vec3(center.x, center.y, region.max.z));
	octant[5] = AABB(glm::vec3(region.min.x, center.y, center.z), glm::vec3(center.x, region.max.y, region.max.z));
	octant[6] = AABB(glm::vec3(center.x, region.min.y, center.z), glm::vec3(region.max.x, center.y, region.max.z));
	octant[7] = AABB(glm::vec3(center.x, center.y, center.z), glm::vec3(region.max.x, region.max.y, region.max.z));

	//for each object in this tree, compare it to each octant; if it fits neatly in one, insert it
	std::vector<Entity*>::iterator it = objList.begin();
	while (it != objList.end()) {
		bool fit = false;

		for (int i = 0; i < 8; i++) {
			if (octant[i].contains((*it)->bound)) {
				octList[i].push_back(*it);
				fit = true;
				break;
			}
		}

		if (fit) {
			it = objList.erase(it);
		}
		else {
			it++;
		}
	}

	//recurse with each octant
	for (int i = 0; i < 8; i++) {
		if (octList[i].size() > 0) {
			child[i] = new OctTree(octant[i], octList[i], this);
			
			child[i]->BuildTree();
			
		}
		else {
			child[i] = nullptr;
		}
	}

	//the easy part
	treeBuilt = true;
}

void OctTree::CheckCollision() {
	CheckCollision(collisionList);
}

void OctTree::CheckCollision(std::vector<Entity*> &incObjList) {

	static std::vector<Entity*>::iterator it;
	static std::vector<Entity*>::iterator jt;
	static std::vector<Entity*>::iterator id;
	static std::vector<Entity*>::iterator jd;
	std::vector<Entity*>::iterator kt;

	id = objList.end();
	jd = incObjList.end();

	glm::vec3 coll_point;
	glm::vec3 coll_norm;
	GLfloat coll_depth;

	for (it = objList.begin(); it != id; it++) {
		//fist, check the objects of this node against other ones in the node
		jt = it;
		jt++;
		for (; jt != id; jt++) {
			//check collision
			if (collide((*it)->bound, (*jt)->bound)) {
				if ((*it == boxes[0]) || *jt == boxes[0]) {
					if (collision_GJK(*it, *jt, &coll_point, &coll_norm, &coll_depth)) {
						//std::cout << "hit!\n";
						tempV = coll_point;
						(*it)->mesh->collision = true;
						(*jt)->mesh->collision = true;
						(*it)->translate(coll_depth*coll_norm);
						//(*jt)->translate(-coll_norm);
					}
				}
				else {
					(*it)->mesh->collision = true;
					(*jt)->mesh->collision = true;
				}
			}
		}

		//next, check them against the incoming object list
		for (jt = incObjList.begin(); jt != jd; jt++) {
			if (collide((*it)->bound, (*jt)->bound)) {
				//do something

				if ((*it == boxes[0]) || *jt == boxes[0]) {
					if (collision_GJK(*it, *jt, &coll_point, &coll_norm, &coll_depth)) {
						//std::cout << "hit!\n";
						tempV = coll_point;
						(*it)->mesh->collision = true;
						(*jt)->mesh->collision = true;
						(*it)->translate(coll_depth*coll_norm);
						//(*jt)->translate(-coll_norm);
					}
				}
				else {
					(*it)->mesh->collision = true;
					(*jt)->mesh->collision = true;
				}
			}
		}
	}

	//next, add this node's objects to the object list, and send it on to the children nodes
	for (std::vector<Entity*>::iterator i = objList.begin(); i != objList.end(); i++) {
		incObjList.push_back(*i);
	}
	//kt = incObjList.begin();
	//incObjList.splice(kt, objList);

	for (int i = 0; i < 8; i++) {
		if (child[i] != nullptr) {
			child[i]->CheckCollision(incObjList);
		}
	}

	incObjList.resize(incObjList.size() - objList.size());
	//	for (int i = 0; i < objList.size(); i++) {
	//		incObjList.erase(incObjList.begin());
	//	}

	//putting things back where they belong
	//objList.splice(objList.begin(), incObjList, incObjList.begin(), kt);
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
bool useOctTree = true;
bool drawOctTree = false;
int lastchange;
glm::vec3 cameraPos = glm::vec3(0, 0, 10);
glm::quat cameraRot = glm::quat();

glm::vec3 levelSize = glm::vec3(512, 512, 512);
AABB levelregion(glm::vec3(0, 0, 0), levelSize);
int num = 3000;

OctTree *collisionTree;

GLuint prog;
Mesh *box;
std::vector<Entity*> boxList;

void model_Init() {
	std::vector<GLfloat> vertices = {
		               // front
		-1.0, -1.0,  1.0,
		1.0, -1.0,  1.0,
		1.0,  1.0,  1.0,
		-1.0,  1.0,  1.0,
		               // back
		-1.0, -1.0, -1.0,
		1.0, -1.0, -1.0,
		1.0,  1.0, -1.0,
		-1.0,  1.0, -1.0,
		};
		               
	std::vector<GLuint> indices = {
		// front
		0, 1, 2,
		2, 3, 0,
		// top
		1, 5, 6,
		6, 2, 1,
		// back
		7, 6, 5,
		5, 4, 7,
		// bottom
		4, 0, 3,
		3, 7, 4,
		// left
		4, 5, 1,
		1, 0, 4,
		// right
		3, 2, 6,
		6, 7, 3,
	};

//uncomment this for circles instead of squares
#define SPHERE 1
#if SPHERE
	vertices.resize(0);
	indices.resize(0);

	int rings = 20;
	int sectors = 20;
	GLfloat radius = 1;
	
	int r, s;
	vertices.resize(rings * sectors * 3);
	std::vector<GLfloat>::iterator v = vertices.begin();

	for (r = 0; r < rings; r++) for (s = 0; s < sectors; s++) {
		float y = -radius + radius*2.f * r / (rings - 1);
		float rad = sqrt(radius*radius - y*y);

		float x = rad*cos(2 * M_PI * s / sectors);
		float z = rad*sin(2 * M_PI * s / sectors);

		*v++ = x;
		*v++ = y;
		*v++ = z;
	}

	indices.resize(rings * sectors * 3*2);
	std::vector<GLuint>::iterator i = indices.begin();
	for (r = 0; r < rings - 1; r++) for (s = 0; s < sectors - 1; s++) {
		if (s == 10000) {

		}
		else {
			*i++ = r * sectors + s;
			*i++ = r * sectors + (s + 1);
			*i++ = (r + 1) * sectors + (s + 1);

			*i++ = (r + 1) * sectors + (s + 1);
			*i++ = (r + 1) * sectors + (s);
			*i++ = r * sectors + s;
		}
	}
#endif
	struct shaderInfo shaders[] = { { GL_VERTEX_SHADER, "vs.glsl" },
	{ GL_FRAGMENT_SHADER, "fs.glsl" } };

	prog = LoadProgram(shaders, 2);
	box = new Mesh(vertices.data(), vertices.size(), indices.data(), indices.size(), prog);

	boxes.push_back(new Entity(new Mesh("ship.obj", prog), 0, 0, 0));
	//boxes.push_back(new Entity(new Mesh(vertices.data(), vertices.size(), indices.data(), indices.size(), prog), 0, 0, 0));
	boxes[0]->scale = glm::vec3(1, 1, 1);
	for (int i = 1; i < num; i++) {
		std::vector<GLfloat> newVerts = vertices;

#if SPHERE && 0
		std::vector<float> map((rings)*(sectors));

		for (int i = 0; i < rings; i++) {
			for (int j = 0; j < sectors; j+= 2) {
				if (i % 2 == 1) {
					j++;
				}
				map[i*sectors + j] = (float)(rand() % 100) / 100 + 0.5;
			}
		}
		for (int i = 0; i < rings; i++) {
			for (int j = 0; j < sectors; j += 2) {
				if (i % 2 == 0) {
					j++;
				}
				int nh, ph, nv, pv;
				float thing =0;
				nh = (i < rings - 1) ? i + 1 : -1;
				ph = (i > 0) ? i - 1 : -1;
				nv = (j < sectors - 1) ? j + 1 : 0;
				pv = (j > 0) ? j - 1: sectors - 1;
				if (nh != -1) {
					thing += map[nh*sectors + j];
				}
				else {
					thing += 1;
				}
				if (ph != -1) {
					thing += map[ph*sectors + j];
				}
				else {
					thing += 1;
				}
				thing += map[i*sectors + pv];

				thing += map[i*sectors + nv];

				map[i*sectors + j] = thing / 4;
			}
		}

		for (int i = 0; i < newVerts.size() /3; i++) {
			glm::vec3 tempVec = glm::vec3(newVerts[i*3], newVerts[i * 3 + 1], newVerts[i * 3 + 2]);
			tempVec = tempVec + map[i]*tempVec;

			newVerts[i * 3] = tempVec.x; 
			newVerts[i * 3 + 1] = tempVec.y;
			newVerts[i * 3 + 2] = tempVec.z;
		}
#endif

		box = new Mesh(newVerts.data(), newVerts.size(), indices.data(), indices.size(), prog);
		boxes.push_back(new Entity(box, rand() % (int)levelSize.x, rand() % (int)levelSize.y, rand() % (int)levelSize.z));
		boxes[i]->scale = glm::vec3((float)(rand() % 100) / 50, (float)(rand() % 100) / 50, (float)(rand() % 100) / 50);
		boxes[i]->scale = glm::vec3(1, 1, 1);
	}
	for (int i = 0; i < num; i++) {
		boxList.push_back(boxes[i]);
	}
}

void print(glm::vec3 a) {
	std::cout << a.x << ", " << a.y << ", " << a.z;
}


void handleKeyUp(const char key) {

}
void handleKeyDown(const char key) {
	switch (key) {
	case 'W':
	case 'w': boxes[0]->translate(cameraRot*glm::vec3(0, 0, -1)); break;
	case 'S':
	case 's': boxes[0]->translate(cameraRot*glm::vec3(0, 0, 1)); break;
	case 'A':
	case 'a': boxes[0]->translate(cameraRot*glm::vec3(-1, 0, 0)); break;
	case 'D':
	case 'd': boxes[0]->translate((cameraRot*glm::vec3(1, 0, 0))); break;
	case 'Q':
	case 'q': boxes[0]->translate(cameraRot*glm::vec3(0, -1, 0)); break;
	case 'E':
	case 'e': boxes[0]->translate(cameraRot*glm::vec3(0, 1, 0)); break;
	case 'Z':
	case 'z': cameraRot = glm::quat(cos(M_PI / 12), (float)sin(M_PI / 12)*(cameraRot*glm::vec3(0, 1, 0)))*cameraRot; break;
	case 'C':
	case 'c': cameraRot = glm::quat(-cos(M_PI / 12), (float)sin(M_PI / 12)*(cameraRot*glm::vec3(0, 1, 0)))*cameraRot; break;
	case 'R':
	case 'r': cameraRot = glm::quat(cos(M_PI / 12), (float)sin(M_PI / 12)*(cameraRot*glm::vec3(1, 0, 0)))*cameraRot;  break;
	case 'F':
	case 'f':cameraRot = glm::quat(-cos(M_PI / 12), (float)sin(M_PI / 12)*(cameraRot*glm::vec3(1, 0, 0)))*cameraRot;  break;
	case 'b':
	case 'B': if (SDL_GetTicks() - lastchange > 1000) { useOctTree = !useOctTree; lastchange = SDL_GetTicks(); std::cout << "changed\n"; } break;
	case 'n':
	case 'N': if (SDL_GetTicks() - lastchange > 1000) { drawOctTree = !drawOctTree; lastchange = SDL_GetTicks(); std::cout << "changed\n"; } break;
	}
}

//mouse movemet
void handleMotion(int x, int y) {
	float dx = M_PI / 180 * x / 10;
	float dy = M_PI / 180 * y / 10;

	cameraRot = glm::quat(-cos(dx / 2), (float)sin(dx / 2)*(cameraRot*glm::vec3(0, 1, 0)))*cameraRot;
	cameraRot = glm::quat(-cos(dy / 2), (float)sin(dy / 2)*(cameraRot*glm::vec3(1, 0, 0)))*cameraRot;

}
int main(int argc, char *argv[]) {
	SDL_Window *window;
	OBB ball(glm::vec3(-1, -1, -1), glm::vec3(1, 1, 1));


	//initializing SDL window
	SDL_Init(SDL_INIT_VIDEO);
	window = SDL_CreateWindow("Super Amazing Fun Times", 100, 100, 640, 480, SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL);
	SDL_SetRelativeMouseMode(SDL_TRUE);

	//initializing Opengl stuff
	SDL_GL_CreateContext(window);
	glewInit();
	glEnable(GL_DEPTH_TEST);
	//initialize the models
	model_Init();

	for (int i = 0; i < boxes.size(); i++) {
		boxes[i]->calcAABB();
		if (i % 5 == 1) {
			boxes[i]->vel = glm::vec3(1, 1, 1);
		}
		else{
			boxes[i]->vel = glm::vec3(0, 0, 0);
		}
	}

	int timeCurr = SDL_GetTicks();
	int timeLast = timeCurr;

	collisionTree = new OctTree(levelregion, boxList);
	collisionTree->UpdateTree();

	//main loop
	comp = 0;
	while (1) {
		timeLast = timeCurr;
		timeCurr = SDL_GetTicks();
		std::cout << "time: " << timeCurr - timeLast << " , comparisons: " << comp << std::endl;
		comp = 0;

		for (int i = 0; i < boxes.size(); i++) {
			boxes[i]->mesh->collision = false;
			if (i != 0) {
				glm::vec3 pos = boxes[i]->pos;
				pos += (float)10*(timeCurr - timeLast)/1000* (boxes[i]->vel);
				pos.x = (float)((int)(pos.x * 1000) % ((int)levelSize.x * 1000)) / 1000;
				pos.y = (float)((int)(pos.y * 1000) % ((int)levelSize.y * 1000)) / 1000;
				pos.z = (float)((int)(pos.z * 1000) % ((int)levelSize.z * 1000)) / 1000;
			
				boxes[i]->translateTo(pos);
			}
		}

		//polling for events
		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT) return 0;
			if (event.type == SDL_KEYDOWN) handleKeyDown(event.key.keysym.sym);
			if (event.type == SDL_KEYUP) handleKeyUp(event.key.keysym.sym);
			if (event.type == SDL_MOUSEMOTION) handleMotion(event.motion.xrel, event.motion.yrel);
		}
		//collision detection
		//boxes[0]->calcAABB();

		if (!useOctTree) {
			//std::cout << "standard" << std::endl;
			for (int i = 0; i < boxes.size(); i++) {
				for (int j = i + 1; j < boxes.size(); j++) {
					if (collide(boxes[j]->bound, boxes[i]->bound)) {
						boxes[i]->mesh->collision = true;
						boxes[j]->mesh->collision = true;
						if (i == 0 || j == 0) {
							std::cout << "Hit!\n";
						}
					}
				}
			}
		}
		else {
			//std::cout << "octtree" << std::endl;

//			if (collisionTree != nullptr) {
//				delete collisionTree;
//				collisionTree = nullptr;
//			}

//			collisionTree = new OctTree(levelregion, boxList);


		}

		//the drawing happens here
		glClearColor(1.0, 1.0, 1.0, 1.0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		cameraPos = boxes[0]->pos + cameraRot*glm::vec3(0, 3, 10);
		boxes[0]->rot = cameraRot;
		boxes[0]->calcAABB();
		glm::mat4 view = glm::inverse(glm::mat4_cast(cameraRot))*glm::translate(glm::mat4(1.0), -cameraPos);
		//glm::mat4 view = glm::lookAt(glm::vec3(0.0, 0.0, -10.0), glm::vec3(0.0, 0.0, 4.0), glm::vec3(0.0, 1.0, 0.0));//I don't necessarily like this
		glm::mat4 projection = glm::perspective(45.0f, 1.0f * 600 / 480, 0.1f, 1000.0f);

		glm::mat4 vp = projection*view;//combination of model, view, projection matrices
									   //mvp = model;
		for (int i = 0; i < boxes.size(); i++) {
			glm::mat4 model = glm::translate(glm::mat4(1.0f), boxes[i]->pos)*glm::scale(boxes[i]->scale)*glm::mat4_cast(boxes[i]->rot);
			glm::mat4 mvp = vp*model;

			//	boxes[i]->bound.draw(vp);

			boxes[i]->mesh->Draw(mvp);
		}

		boxes[0]->bound.draw(vp);

		ball.trans = ball.trans*glm::rotate(1.f, glm::vec3(1, 0, 0));
		ball.draw(vp);

		//Drawing the closest point on the main object to the camera, as a test
		glDisable(GL_DEPTH_TEST);
		glEnable(GL_PROGRAM_POINT_SIZE);
		glPointSize(10);
		//tempV = boxes[0]->furthestPointInDirection(cameraRot*glm::vec3(0, 0, 1));
		//std::cout << tempV.x << ", " << tempV.y << ", " << tempV.z << ", " << std::endl;
		drawDot(tempV, vp);
		glEnable(GL_DEPTH_TEST);

		collisionTree->DrawContainingRegion(boxes[0], vp);

		if (drawOctTree) {
			collisionTree->draw(vp);
		}
		SDL_GL_SwapWindow(window);

	}

	//	return 0;
}
#endif