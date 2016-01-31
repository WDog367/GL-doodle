#include "shader_utils.h"
#include <iostream>
#include <fstream>
#include <string>

using namespace std;

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

		cerr << fileName << ": compilation error: " << log << endl;
		return 0;
	}

	return shader;
}

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

		cerr << "Link error: " << log << endl;
		return 0;
	}

	//deleting all of the shaders, avoiding memleaks(?)
	for (int i = 0; i < size; i++) {
		glDeleteShader(cs[i]);
	}

	return program;
}

GLint getAttrib(GLuint program, char * name) {
	GLint attrib;
	attrib = glGetAttribLocation(program, name);
	if (attrib == -1) {
		cerr << name << ": couldn't bind attribute\n";
		return false;
	}
	return attrib;
}