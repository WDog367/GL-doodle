#include <iostream>
#include <fstream>
#include <sstream>//allows you to make streams from strings, saves code in .obj file reading
#include <string>
#include <vector>
#include "GL/glew.h"
#include "GL/freeglut.h"//may want to start using "GL/glut.h" to wean off of freeglut exclusive stuff?
#include "glm/glm.hpp"//includes most (if not all?) of GLM library


//custom headers
#include "shader_utils.h"

//TODO
//Stop using globals (hahaha)

using namespace std;//mmmmmmmmmmmmmmmmmmmmmm
GLuint program;

GLuint vbo_coord;
GLuint vbo_colour;//will probably put these in an array, or some better storage system
GLuint vao;//VAOs appear to be necessary in OpenGL 4.3, aww nuts


bool init() {
	//Setting up the render program (frag/vert shaders
	struct shaderInfo shaders[] =	{ { GL_VERTEX_SHADER, "vs.glsl" },
									{ GL_FRAGMENT_SHADER, "fs.glsl" } };

	program = LoadProgram(shaders, 2);

	//Setting up the buffers storing data for vertex attribs
	vector <GLfloat> vertices = {
		0.0f,  0.8f,
		-0.8f, -0.8f,
		0.8f, -0.1f
	};
	glGenBuffers(1, &vbo_coord);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_coord);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices[0])*vertices.size(), vertices.data(), GL_STATIC_DRAW);

	vector <GLfloat> colours = {
		0.0f, 0.0f, 1.0f,
		0.0f, 1.0f, 0.0f,
		1.0f, 0.0f, 0.0f
	};
	glGenBuffers(1, &vbo_colour);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_colour);
	glBufferData(GL_ARRAY_BUFFER, sizeof(colours[0])*colours.size(), colours.data(), GL_STATIC_DRAW);
		
	//setting up the vertex attribute/arrays
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	glBindBuffer(GL_ARRAY_BUFFER, vbo_coord);
	GLint attrib_vcoord = getAttrib(program, "coord2d");
	glVertexAttribPointer(
		attrib_vcoord,	//attrib 'index'
		2,				//pieces of data per index
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

	//setting the program to use
	glUseProgram(program);
	
	return true;
}

void display() {
	glClearColor(1.0, 1.0, 1.0, 1.0);
	glClear(GL_COLOR_BUFFER_BIT);
	
	glBindVertexArray(vao);

	glDrawArrays(GL_TRIANGLES, 0, 3);

	glFlush();
}

void free_res() {
	glDeleteProgram(program);
	glDeleteBuffers(1, &vbo_coord);
	glDeleteBuffers(1, &vbo_colour);
}

int main(int argc, char *argv[]) {
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGBA);
	glutInitWindowSize(600, 480);
	glutInitContextVersion(4, 3);
	glutInitContextProfile(GLUT_CORE_PROFILE);
	glutCreateWindow("Test Display");

	glewExperimental = true;
	if (glewInit()) {
		cerr << "Unable to initialize GLEW\n";
	}

	cout << glGetString(GL_VERSION) << endl;

	init();

	glutDisplayFunc(display);

	glutMainLoop();

	free_res();
}