#ifndef WDB_MESH_UTIL
#define WDB_MESH_UTIL

#include <vector>
#include <string>
#include "armature_utils.h"

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

	//this data is stored in VBO's,
	//not strictly necessary to have application-accessible copies
	//may make this a 'mutable Mesh' subclass, to handle "morphing" the mesh
	//std::vector<float> vertices;
	//std::vector<float> indices;
public:
	Mesh();
	Mesh(GLuint prog);
	Mesh(char * fileName, GLuint prog);//assumes .obj
	Mesh(const GLfloat *vertices, int vNum, const GLuint *elements, int eNum, GLuint program);
	virtual ~Mesh();

	public:
	virtual void Draw(const glm::mat4 &mvp);

	void updateMesh(const GLfloat *vertices, int vNum, const GLuint *elements, int eNum);
	void updateProgram(GLuint program);//also indices of vertex and element attributes as parameters?
};

//these should probably be sub-classes of object
//implement them with same subroutine names, so changing it later will be easy
class Skeletal_Mesh_simple {
public:
	Armature *skeleton;
	std::vector<Mesh*> mesh;


public:
	Skeletal_Mesh_simple(Armature* skeleton, GLuint program);
	~Skeletal_Mesh_simple();

	void Draw(const glm::mat4 &mvp);
};

class Skeletal_Mesh : public Mesh{
public:
	GLuint vbo_bIndex;
	GLuint vbo_bWeight;
	GLuint vbo_bNum;

	GLuint uniform_BoneTransform;

	glm::mat4 bindSpaceMatrix;

	Armature* skeleton;

	static const int maxBNum = 4;

	Skeletal_Mesh(const GLfloat *vertices, int vNum, const GLuint *elements, int eNum, Armature *skeleton, GLuint program);
	Skeletal_Mesh(const GLfloat *vertices, int vNum, const GLuint *elements, int eNum, const GLuint *bIndex, const GLfloat* bWeight, const GLuint * bNum, const glm::mat4 &bindSpaceMatrix, Armature *skeleton, GLuint program);
	virtual void Draw(const glm::mat4 &mvp);
private:
	void updateWeights(const GLuint *bIndex, const GLfloat* bWeight, const GLuint * bNum, int vNum);
	};
#endif