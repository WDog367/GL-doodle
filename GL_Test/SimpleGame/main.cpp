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
#include "Entity.h"

//todo: get rid of global variables
//maybe use singleton game class?

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


	//read the vertices of the ship, and use them to initialize an OBB, for collision
	GLfloat* vertices;
	int size;

	glBindBuffer(GL_COPY_READ_BUFFER, mesh->vbo_coord);
	vertices = (GLfloat*)glMapBuffer(GL_COPY_READ_BUFFER, GL_READ_ONLY);
	glGetBufferParameteriv(GL_COPY_READ_BUFFER, GL_BUFFER_SIZE, &size);

	third = new OBB((glm::vec3*)vertices, size / sizeof(glm::vec3));

	glUnmapBuffer(GL_COPY_READ_BUFFER);
	glBindBuffer(GL_COPY_READ_BUFFER, -1);
}

//fills the level region with randomly placed cubes of size (2,2,2)
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

//key presses
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

	collisionTree = new OctTree(levelRegion);
	for (Entity* entity : Entitys) {
		collisionTree->Insert(entity->collider);
	}
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

		//check player vs. opponent collision 
		//opponent is not included in the octtree, and sits at 0,0,0
		//should probably remove opponent
		if (player->intersect(*opponent)) {
			opponent->colour = player->colour = glm::vec3(1, 1, 0);
		}
		else {
			opponent->colour = player->colour = glm::vec3(1, 0, 0);
		}


		//process the collision
		collisionList.clear();

		collisionTree->UpdateTree();
		collisionTree->CheckCollision(collisionList);

		//
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
		
		//draw points to show the collision results
		for (int i = 0; i < collisionList.size(); i++) {
			collisionList[i].member[0]->owner->bound.colour = glm::vec3(1, 1, 0);
			collisionList[i].member[1]->owner->bound.colour = glm::vec3(1, 1, 0);
			collisionList[i].member[0]->owner->translate(-collisionList[i].collision.normal*collisionList[i].collision.depth);
			drawDot(collisionList[i].collision.point, vp, glm::vec3(1, 0, 0));
			drawLine(collisionList[i].collision.point, collisionList[i].collision.point - collisionList[i].collision.normal*collisionList[i].collision.depth, vp, glm::vec3(0, 0, 1));
		}

		//draw the OctTree
		collisionTree->draw(vp);

		opponent->draw(mvp);
		player->draw(mvp);

		//todo: use some simple occlusion culling
		for (int i = 0; i < Entitys.size(); i++) {
			Entitys[i]->Tick(0.016);
			Entitys[i]->draw(vp);
		}

		third->draw(mvp);
		mesh->Draw(mvp);
		for (int i = 0; i < dots.size(); i++) {
			drawDot(dots[i], mvp);
		}

		SDL_GL_SwapWindow(window);

	}

	//	return 0;
}