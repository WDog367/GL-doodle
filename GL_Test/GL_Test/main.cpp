#include <iostream>
#include <fstream>
#include <sstream>//allows you to make streams from strings, saves code in .obj file reading
#include <string>
#include <vector>
#include "math.h"
#include "GL/glew.h"

// no more glut..
//#include "GL/freeglut.h"//may want to start using "GL/glut.h" to wean off of freeglut exclusive stuff?
#include "SDL.h"
#include "PROJECT_OPTIONS.h"
#include "glm/glm.hpp"//includes most (if not all?) of GLM library
#include "glm/gtx/transform.hpp"
#include "glm/gtc/type_ptr.hpp"

///skeletal stuff

//custom headers
#include "shader_utils.h"

//TODO
//Stop using globals (hahaha)

using namespace std;//mmmmmmmmmmmmmmmmmmmmmm
GLuint program;

GLuint vbo_coord;
GLuint vbo_colour;//will probably put these in an array, or some better storage system
GLuint ibo_elements;
GLuint uniform_matrix;
GLuint vao;//VAOs appear to be necessary in OpenGL 4.3, aww nuts

unsigned int timeLast, timeCurr;

void loadObj(vector <float> &vertices, vector <unsigned int> &elements, char *fileName) {
	ifstream file;
	//istringstream sline;
	string line;
	string token;

	int in_int;
	float in_float;

	string full_filename = string(RESOURCE_DIR "/") + string(fileName);
	file.open(full_filename);
	if (!file) {
		cerr << "Error loading .obj: " << fileName << endl;
		return;
	}
	//.obj format:
	//v lines have vertices
	//f lines have face (indices)
	while (getline(file, line)) {

		if (line.substr(0, 2) == "v ") {
		//Loading a vertex
			istringstream sline (line.substr(2));
			//getting the three vertex coordinates on this line
			for (int i = 0; i < 3; i++) {//there might be a fourth component, but it's not necessarily useful
				sline >> in_float;
				vertices.push_back(in_float);
			}

		}
		else if (line.substr(0, 2) == "f ") {
		//Loading a face
			istringstream sline(line.substr(2));;

			for (int i = 0; i < 3; i++) {
				string temp;

				sline >> token;
				if (token.find("/") != -1) {
					in_int = atoi(token.substr(0, token.find("/")).c_str());
				}
				else {
					in_int = atoi(token.c_str());
				}
				elements.push_back(in_int);

			}
		}
	}


}

bool init() {
	//Setting up the render program (frag/vert shaders
	struct shaderInfo shaders[] =	{ { GL_VERTEX_SHADER, "vs.glsl" },
									{ GL_FRAGMENT_SHADER, "fs.glsl" } };

	program = LoadProgram(shaders, 2);

	//Setting up the buffers storing data for vertex attribs
	vector <GLfloat> vertices;
	vector <GLfloat> colours;
	vector <GLuint> elements;
	/*
	vertices = {
		0.0f,  0.8f, 0.0f,
		-0.8f, -0.8f, 0.0f,
		0.8f, -0.8f, 0.0f,
		0.0f, 0.0f, -2.0f
	};

	elements = {
		0, 1, 2,
		0, 1, 3,
		1, 2, 3,
		2, 0, 3
	};

	*/
	loadObj(vertices, elements, "object");

	colours = {
		0.0f, 0.0f, 1.0f,
		0.0f, 1.0f, 0.0f,
		1.0f, 0.0f, 0.0f,
		1.0f, 1.0f, 0.0f
	};
	glGenBuffers(1, &vbo_coord);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_coord);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices[0])*vertices.size(), vertices.data(), GL_STATIC_DRAW);


	glGenBuffers(1, &vbo_colour);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_colour);
	glBufferData(GL_ARRAY_BUFFER, sizeof(colours[0])*colours.size(), colours.data(), GL_STATIC_DRAW);
	

	glGenBuffers(1, &ibo_elements);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_elements);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(elements[0])*elements.size(), elements.data(), GL_STATIC_DRAW);

	//setting up the vertex attribute/arrays
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	glBindBuffer(GL_ARRAY_BUFFER, vbo_coord);
	GLint attrib_vcoord = getAttrib(program, "coord");
	glVertexAttribPointer(
		attrib_vcoord,	//attrib 'index'
		3,				//pieces of data per index
		GL_FLOAT,		//type of data
		GL_FALSE,		//whether to normalize
		0,				//space b\w data
		0				//offset from buffer start
		);
	glEnableVertexAttribArray(attrib_vcoord);

	glBindBuffer(GL_ARRAY_BUFFER, vbo_colour);
	GLint attrib_vcolour = getAttrib(program, "colour");
	glVertexAttribPointer(
		attrib_vcolour,	//attrib 'index'
		3,				//pieces of data per index
		GL_FLOAT,		//type of data
		GL_FALSE,		//whether to normalize
		0,				//space b\w data
		0				//offset from buffer start
		);
	glEnableVertexAttribArray(attrib_vcolour);

	uniform_matrix = getUniform(program, "matrix");

	//setting the program to use
	glUseProgram(program);
	

	timeLast = SDL_GetTicks();
	return true;
}

void idle() {
	//updating the matrics (i.e. animation) done in the idle function

	timeCurr = SDL_GetTicks();
	float angle = (timeCurr - timeLast) / 1000.0*(3.14159265358979323846) / 4;

	//model matrix: transforms local model coordinates into global world coordinates
	glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0, 1.0, -2.0))*
		glm::rotate(angle, glm::vec3(0.0, 1.0, 0.0));

	//view matrix: transforms world coordinates into relative-the-camera view-space coordinates
	glm::mat4 view = glm::lookAt(glm::vec3(0.0, 2.0, 0.0), glm::vec3(0.0, 0.0, -4.0), glm::vec3(0.0, 1.0, 0.0));//I don't necessarily like this

	//projection matrix: transforms view space coordinates into 2d screen coordinates
	glm::mat4 projection = glm::perspective(45.0f, 1.0f * 600 / 480, 0.1f, 10.0f);

	glm::mat4 mvp = projection*view*model;//combination of model, view, projection matrices

	glUniformMatrix4fv(uniform_matrix, 1, GL_FALSE, glm::value_ptr(mvp));
}

void display() {
	glClearColor(1.0, 1.0, 1.0, 1.0);
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
	
	glBindVertexArray(vao);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_elements);//not sure if 'glbindVertexArray' will include this guy
	
	glDrawElements(GL_TRIANGLES, 12, GL_UNSIGNED_INT, NULL);

	glFlush();
}

void free_res() {
	glDeleteProgram(program);
	glDeleteBuffers(1, &vbo_coord);
	glDeleteBuffers(1, &vbo_colour);
}

int main(int argc, char *argv[]) {
	SDL_Window* window;
	SDL_Init(SDL_INIT_VIDEO);
	window = SDL_CreateWindow("Test Display", 100, 100, 640, 480, SDL_WINDOW_OPENGL);
	SDL_GL_CreateContext(window);

	glewExperimental = true;
	if (glewInit()) {
		cerr << "Unable to initialize GLEW\n";
	}

	cout << glGetString(GL_VERSION) << endl;

	init();

	glEnable(GL_DEPTH_TEST);
	bool run = true;
	while (run) {
		//polling for events
		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT) run = false;
		}
		idle();
		display();
		SDL_GL_SwapWindow(window);
	}
	free_res();
	return 0;
}