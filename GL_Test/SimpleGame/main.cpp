#include "SDL.h"
//#include "SDL_keyboard.h"
//#include "SDL_keycode.h"

#include <vector>
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
glm::vec3 cameraPos = glm::vec3(0, 0, 10);
glm::quat cameraRot = glm::quat();

typedef struct {
	glm::vec3 min;
	glm::vec3 max;
} AABB;

class Actor {
public:
	Mesh *mesh;
	AABB bound;

	glm::vec3 pos;

	Actor(Mesh *mesh, GLfloat x, GLfloat y, GLfloat z) : mesh(mesh), pos(glm::vec3(x,y,z)) {

	}

	void calcAABB() {
		GLfloat* vertices;
		int size;

		glBindBuffer(GL_COPY_READ_BUFFER, mesh->vbo_coord);
		vertices = (GLfloat*)glMapBuffer(GL_COPY_READ_BUFFER, GL_READ_ONLY);
		glGetBufferParameteriv(GL_COPY_READ_BUFFER, GL_BUFFER_SIZE, &size);

		//calculations would be better done on GPU w\ openGL call, but meh
		//also, might be better to just to rotation, and then do the translation on the resulting two points
		for (int i = 0; i < size/sizeof(GLfloat); i += 3) {
			glm::vec3 tempV = glm::vec3(
				glm::translate(glm::mat4(1.0f), pos)
				*glm::vec4(vertices[i * 3], vertices[i * 3 + 1], vertices[i * 3 + 2], 1.0));
			
			if (i == 0) {
				bound.min = tempV;
				bound.max = tempV;
			}
			else 
			if (tempV.x < bound.min.x) {
				bound.min.x = tempV.x;
			}else if (tempV.x > bound.max.x) {
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

		//std::cout << "(" << bound.min.x << "," << bound.min.y << "," << bound.min.z << ")" << ",(" << bound.max.x << "," << bound.max.y << "," << bound.max.z << ")" << std::endl;

		glUnmapBuffer(GL_COPY_READ_BUFFER);
		glBindBuffer(GL_COPY_READ_BUFFER, -1);
	}
};

GLuint prog;
Mesh *box;
std::vector<Actor*> boxes;

void model_Init() {
	GLfloat baseVertices[] = {
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
	GLuint baseElements[] = {
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

	struct shaderInfo shaders[] = { { GL_VERTEX_SHADER, "vs.glsl" },
	{ GL_FRAGMENT_SHADER, "fs.glsl" } };

	prog = LoadProgram(shaders, 2);
	box = new Mesh(baseVertices, 8 * 3, baseElements, 12 * 3, prog);

	boxes.push_back(new Actor(box, 0, 0, 0));
	for (int i = 1; i < 10; i++) {
		boxes.push_back(new Actor(box, rand() % 20, rand() % 20, rand() % 20));
	}
}

bool intersect(AABB a, AABB b) {
	return (a.min.x <= b.max.x && a.max.x >= b.min.x) &&
		(a.min.y <= b.max.y && a.max.y >= b.min.y) &&
		(a.min.z <= b.max.z && a.max.z >= b.min.z);
}

void handleKeyUp(const char key){

}
void handleKeyDown(const char key) {
	switch (key) {
	case 'W' :
	case 'w': boxes[0]->pos += cameraRot*glm::vec3(0, 0, -1); break;
	case 'S':
	case 's': boxes[0]->pos -= cameraRot*glm::vec3(0, 0, -1); break;
	case 'A':
	case 'a': boxes[0]->pos += cameraRot*glm::vec3(1, 0, 0); break;
	case 'D':
	case 'd': boxes[0]->pos -= cameraRot*glm::vec3(1, 0, 0); break;
	case 'Q':
	case 'q': boxes[0]->pos += cameraRot*glm::vec3(0, 1, 0); break;
	case 'E':
	case 'e': boxes[0]->pos -= cameraRot*glm::vec3(0, 1, 0); break;
	case 'Z':
	case 'z': cameraRot = glm::quat(cos(M_PI/12), (float)sin(M_PI / 12)*(cameraRot*glm::vec3(0, 1, 0)))*cameraRot; break;
	case 'C':
	case 'c': cameraRot = glm::quat(-cos(M_PI / 12), (float)sin(M_PI / 12)*(cameraRot*glm::vec3(0, 1, 0)))*cameraRot; break;
	case 'R':
	case 'r': cameraRot = glm::quat(cos(M_PI / 12), (float)sin(M_PI / 12)*(cameraRot*glm::vec3(1, 0, 0)))*cameraRot;  break;
	case 'F':
	case 'f':cameraRot = glm::quat(-cos(M_PI / 12), (float)sin(M_PI / 12)*(cameraRot*glm::vec3(1, 0, 0)))*cameraRot;  break;
	}
}

void handleMotion(int x, int y) {
	float dx = M_PI/180*x/10;
	float dy = M_PI / 180 * y / 10;

	cameraRot = glm::quat(-cos(dx/2), (float)sin(dx/2)*(cameraRot*glm::vec3(0, 1, 0)))*cameraRot;
	cameraRot = glm::quat(-cos(dy/2), (float)sin(dy/2)*(cameraRot*glm::vec3(1, 0, 0)))*cameraRot;

}
int main(int argc, char *argv[]) {
	SDL_Window *window;

	//initializing SDL window
	SDL_Init(SDL_INIT_VIDEO);
	window = SDL_CreateWindow("Super Amazing Fun Times", 100,100, 640, 480, SDL_WINDOW_RESIZABLE|SDL_WINDOW_OPENGL);
	SDL_SetRelativeMouseMode(SDL_TRUE);

	//initializing Opengl stuff
	SDL_GL_CreateContext(window);
	glewInit();

	//initialize the models
	model_Init();

	for (int i = 0; i < boxes.size(); i++) {
		boxes[i]->calcAABB();
	}

	//main loop
	while (1) {
		//polling for events
		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT) return 0;
			if (event.type == SDL_KEYDOWN) handleKeyDown(event.key.keysym.sym);
			if (event.type == SDL_KEYUP) handleKeyUp(event.key.keysym.sym);
			if (event.type == SDL_MOUSEMOTION) handleMotion(event.motion.xrel, event.motion.yrel); 
		}
		//collision detection

		boxes[0]->calcAABB();

		for (int i = 1; i < boxes.size(); i++) {
			if (intersect(boxes[0]->bound, boxes[i]->bound))
				std::cout << i << ", ";
		}
		std::cout << std::endl;
		//the drawing happens here
		glClearColor(1.0, 1.0, 1.0, 1.0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		cameraPos = boxes[0]->pos + cameraRot*glm::vec3(0, 3, 10);

		glm::mat4 view = glm::inverse(glm::mat4_cast(cameraRot))*glm::translate(glm::mat4(1.0), -cameraPos);
		//glm::mat4 view = glm::lookAt(glm::vec3(0.0, 0.0, -10.0), glm::vec3(0.0, 0.0, 4.0), glm::vec3(0.0, 1.0, 0.0));//I don't necessarily like this
		glm::mat4 projection = glm::perspective(45.0f, 1.0f * 600 / 480, 0.1f, 1000.0f);

		glm::mat4 vp = projection*view;//combination of model, view, projection matrices
									//mvp = model;
		for (int i = 0; i < boxes.size(); i++) {
			glm::mat4 model = glm::translate(glm::mat4(1.0f), boxes[i]->pos);
			glm::mat4 mvp = vp*model;

			boxes[i]->mesh->Draw(mvp);
		}

		SDL_GL_SwapWindow(window);
	}

//	return 0;
}