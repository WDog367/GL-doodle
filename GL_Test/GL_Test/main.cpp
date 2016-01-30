#include <iostream>
#include <fstream>
#include <string>
#include "GL/glew.h"
#include "GL/freeglut.h"//may want to start using "GL/glut.h" to wean off of freeglut exclusive stuff?
#include "glm/vec2.hpp"

using namespace std;//mmmmmmmmmmmmmmmmmmmmmm

GLuint program;
GLuint vbo;
GLuint vao;//VAOs appear to be necessary in OpenGL 4.3, fuck them
GLuint attrib_vcoord;

bool init() {

	GLint compile_ok = GL_FALSE, link_ok = GL_FALSE;

	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	GLuint vs = glCreateShader(GL_VERTEX_SHADER);
	const char *vs_source =
		//"#version 100\n"  // OpenGL ES 2.0
		"#version 430\n"  // OpenGL 2.1
		"layout(location = 0) in vec2 coord2d;                  "
		"void main(void) {                        "
		"  gl_Position = vec4(coord2d, 0.0, 1.0); "
		"}";
	glShaderSource(vs, 1, &vs_source, NULL);
	glCompileShader(vs);
	glGetShaderiv(vs, GL_COMPILE_STATUS, &compile_ok);
	if (!compile_ok) {
		cerr << "Error in vertex shader" << endl;
		return false;
	}

	GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
	const char *fs_source =
		//"#version 100\n"  // OpenGL ES 2.0
		"#version 430\n"  // OpenGL 2.1
		"void main(void) {        "
		"  gl_FragColor[0] = 0.0; "
		"  gl_FragColor[1] = 0.0; "
		"  gl_FragColor[2] = 1.0; "
		"}";
	glShaderSource(fs, 1, &fs_source, NULL);
	glCompileShader(fs);
	glGetShaderiv(fs, GL_COMPILE_STATUS, &compile_ok);
	if (!compile_ok) {
		cerr << "Error in fragment shader" << endl;
		return false;
	}

	program = glCreateProgram();
	glAttachShader(program, vs);
	glAttachShader(program, fs);
	glLinkProgram(program);
	glGetProgramiv(program, GL_LINK_STATUS, &link_ok);
	if (!link_ok) {
		cerr << "error in linking\n";
	}

	const char* attribute_name = "coord2d";
	attrib_vcoord = glGetAttribLocation(program, attribute_name);
	if (attrib_vcoord == -1) {
		cerr << "couldn't bind attribute\n";
		cout << "wodda wodda\n";
		return false;
	}

	GLfloat triangle_vertices[] = {
		0.0,  0.8,
		-0.8, -0.8,
		0.8, -0.8,
	};
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(triangle_vertices), triangle_vertices, GL_STATIC_DRAW);

	glUseProgram(program);

	glVertexAttribPointer(
		attrib_vcoord,
		2,
		GL_FLOAT,
		GL_FALSE,
		0,
		0
		);
	glEnableVertexAttribArray(attrib_vcoord);

	return true;
}

void display() {
	glClearColor(1.0, 1.0, 1.0, 1.0);
	glClear(GL_COLOR_BUFFER_BIT);

	glBindVertexArray(vao);

	glDrawArrays(GL_TRIANGLES, 0, 3);

	cout << "display" << endl;
	glFlush();
}

void free_res() {
	glDeleteProgram(program);
	glDeleteBuffers(1, &vbo);
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