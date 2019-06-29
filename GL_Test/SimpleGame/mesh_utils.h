#ifndef WDB_MESH_UTIL
#define WDB_MESH_UTIL

#include <vector>
#include <string>

#include "GL/glew.h"
#include "glm/glm.hpp"//includes most (if not all?) of GLM library

#define MAXBONES 50//might use arrays at some point; not today, suckkaah
#define MAXCHANNEL 155//should be around MAXBONES*3+3 plus a few more for kicks, why not?

void loadObj(std::vector<float> &vertices, std::vector<unsigned int> &elements, char *fileName, std::vector<glm::vec3> normals);

class Mesh {
public:
	GLuint vao;
	GLuint vbo_coord;
	GLuint vbo_normals;
	GLuint ibo_elements;
	GLuint uniform_Matrix;
	GLuint uniform_m_3x3_inv_transp;
	GLuint program;

	GLuint drawShape;

	//this data is stored in VBO's,
	//not strictly necessary to have application-accessible copies
public:
	unsigned int vCount;
	
	Mesh();
	Mesh(GLuint prog);
	Mesh(char * fileName, GLuint prog);//assumes .obj
	Mesh(const GLfloat *vertices, int vNum, const GLuint *elements, int eNum, GLuint program, GLuint drawShape);
	virtual ~Mesh();

	public:
	virtual void Draw(const glm::mat4 &mvp);
	virtual void Draw(const glm::mat4 &vp, const glm::mat4 &model);

	void updateMesh(const GLfloat *vertices, int vNum, const GLuint *elements, int eNum);
	void updateProgram(GLuint program);//also indices of vertex and element attributes as parameters?

	GLuint getProgram();
};

#endif