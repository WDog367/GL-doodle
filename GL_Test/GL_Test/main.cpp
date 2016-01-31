#include <iostream>
#include <fstream>
#include <string>
#include "GL/glew.h"
#include "GL/freeglut.h"//may want to start using "GL/glut.h" to wean off of freeglut exclusive stuff?
#include "glm/vec2.hpp"


////////////////////////////////////////////////////////////////////////////////////////
//To Do:
// Check if VAO stores which program is being used (It should logically, right?
// setup more accessible functions, move them to new file
///////////////////////////////////////////////////////////////////////////////////////

using namespace std;//mmmmmmmmmmmmmmmmmmmmmm
//eventually I will stop using globals
GLuint program;

GLuint vbo;
GLuint vao;//VAOs appear to be necessary in OpenGL 4.3, aww nuts

GLuint LoadShader(GLenum type, const char *fileName) {
	GLuint shader;
	ifstream fsource;
	string source;
	const char *csource;
	string line;
	GLint compiled;

	fsource.open(fileName);
	if (!fsource) {//not 100% sure if this is a valid thing
		cerr << fileName << ": Can't open shader source" << endl;
		return 0;
	}
	//reading the source file
	source = "";
	while (true) {
		getline(fsource, line);

		if (fsource.fail()) {
			break;
		}
		source += line + "\n";
	}
	csource = source.c_str();
	
	//the three important gl-relavent lines for shaders
	shader = glCreateShader(type);
	glShaderSource(shader, 1, &csource, NULL);
	glCompileShader(shader);

	//Checking status and error log, if something goes wrong
	glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
	if (compiled == GL_FALSE) {
		GLint logLen;//this feels dirty
		char *log;
		
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLen);
		log = new char[logLen];
		glGetShaderInfoLog(shader, logLen, NULL, log);
		
		cerr << fileName << ": " << log << endl;
		return 0;
	}

	return shader;
}

struct shaderInfo {
	GLenum type;
	char *fileName;
};
GLuint LoadProgram(struct shaderInfo *si, int size) {//I saw an example of this w\ a null terminated array, but butts to that
	GLuint *cs = new GLuint[size];
	GLuint program;
	GLint link_ok;

	program = glCreateProgram();

	for (int i = 0; i < size; i++) {
		cs[i] = LoadShader(si[i].type, si[i].fileName);
		if (cs[i] == 0) return 0;//quitting if error loading shader
		glAttachShader(program, cs[i]);
	}

	glLinkProgram(program);

	//Checking for errors
	glGetProgramiv(program, GL_LINK_STATUS, &link_ok);
	if (!link_ok) {
		GLint logLen;
		char *log;

		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLen);
		log = new char[logLen];
		glGetProgramInfoLog(program, logLen, NULL, log);

		cerr << "Error in linking: "<<log<<endl;
		return 0;
	}

	//deleting all of the shaders, avoiding memleaks(?)
	for (int i = 0; i < size; i++) {
		glDeleteShader(cs[i]);
	}

	return program;
}

bool init() {
	//Setting up the render program (frag/vert shaders
	struct shaderInfo shaders[] = { { GL_VERTEX_SHADER, "vs.glsl" },
	{ GL_FRAGMENT_SHADER, "fs.glsl" } };

	program = LoadProgram(shaders, 2);

	//Setting up the buffers storing data for vertex attribs
	GLfloat triangle_vertices[] = {
		0.0,  0.8,
		-0.8, -0.8,
		0.8, -0.8,
	};
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(triangle_vertices), triangle_vertices, GL_STATIC_DRAW);

	//setting up the vertex attribute/arrays
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	GLuint attrib_vcoord;
	const char* attribute_name = "coord2d";
	attrib_vcoord = glGetAttribLocation(program, attribute_name);
	if (attrib_vcoord == -1) {
		cerr << attribute_name << ": couldn't bind attribute\n";
		return false;
	}
	glVertexAttribPointer(
		attrib_vcoord,
		2,
		GL_FLOAT,
		GL_FALSE,
		0,
		0
		);
	glEnableVertexAttribArray(attrib_vcoord);

	//setting the program to use
	glUseProgram(program);
	
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