#ifndef WDB_MESH_UTIL
#define WDB_MESH_UTIL

#include <vector>
#include <string>

#include "GL/glew.h"
#include "glm/glm.hpp"//includes most (if not all?) of GLM library

#define MAXBONES 50//might use arrays at some point; not today, suckkaah
#define MAXCHANNEL 155//should be around MAXBONES*3+3 plus a few more for kicks, why not?

class Mesh {
public:
	GLuint vao;
	GLuint vbo_coord;
	GLuint ibo_elements;
	GLuint uniform_Matrix;
	GLuint program;

	GLuint uniform_collision;

	//this data is stored in VBO's,
	//not strictly necessary to have application-accessible copies
	//may make this a 'mutable Mesh' subclass, to handle "morphing" the mesh
	//std::vector<float> vertices;
	//std::vector<float> indices;
public:
	unsigned int vCount;
	
	Mesh();
	Mesh(GLuint prog);
	Mesh(char * fileName, GLuint prog);//assumes .obj
	Mesh(const GLfloat *vertices, int vNum, const GLuint *elements, int eNum, GLuint program);
	virtual ~Mesh();

	public:
	virtual void Draw(const glm::mat4 &mvp);

	void updateMesh(const GLfloat *vertices, int vNum, const GLuint *elements, int eNum);
	void updateProgram(GLuint program);//also indices of vertex and element attributes as parameters?

	int collision = false;
};

#endif