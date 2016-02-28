#ifndef _SHADER_UTILS
#define _SHADER_UTILS

#include "GL/glew.h"

struct shaderInfo {
	GLenum type;
	char *fileName;
};

GLuint LoadShader(GLenum type, const char *fileName);
GLuint LoadProgram(struct shaderInfo *si, int size);
GLint getAttrib(GLuint program, char * name);
GLint getUniform(GLuint program, char * name);
#endif