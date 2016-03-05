#ifndef WDB_ARM_UTIL
#define WDB_ARM_UTIL

#include <vector>
#include <string>
#include "glm/glm.hpp"//includes most (if not all?) of GLM library

#define MAXBONES 50//might use arrays at some point; not today, suckkaah
#define MAXCHANNEL 155//should be around MAXBONES*3+3 plus a few more for kicks, why not?

class Armature {
public://everything public, because I'm a hack
	struct joint { //storing skeletons as linked lists, maybe will do different later

		//these are a very bad idea while everything is stored in vectors;
		//the vector could get resized, and then these will be pointing to the wrong spot
		//float* offset;//point to the spot in the offset array
		//float* rotation;//ditto to the rotation array
		glm::mat4* matrix;

		//this should be used more than the pointers, I may get rid of them
		int index;

		std::string name;
		joint* Parent;
		std::vector<joint*> Child;
	};

	//bone related things, stored in array for quick access, easy transfer to shaders; could do vector, potentially, but meh
	std::vector<float> offset;// [MAXBONES][3];
	std::vector<float> rotation;// [MAXBONES][3];//stored in zyx euler?
	std::vector<float> parent;// [MAXBONES][3];//stored in zyx euler?
	std::vector<glm::mat4> matrix; //[MAXBONES]; //should swap this out for a quaternian in the future
	
	int size = 0;
	joint * root;


	//Animation related things
	std::vector<float*> channelMap;
	std::vector<std::vector<float>> frames;


	//programs and tools?
	void draw(const glm::mat4 &objMat);
	void print();

	void setMatrices();

	bool loadBVHFull(const char *fileName);
	bool loadBVHArm(const char *fileName);
	bool loadBVHAnim(const char *fileName);
	bool loadBVHAnim_quick(const char *fileName);

	Armature::joint * getJoint(std::string name);
	Armature::joint * getJoint(int index);
private:
	//mostly recursive helper functions
	void setMatrices(joint *);
};


/*
//classes below here aren't strictly armature stuff, may move to another file
class Mesh {
	GLuint vao;
	GLuint vbo_coord;
	GLuint ibo_elements;
	GLuint uniform_Matrix;
	GLuint program;

public:
	Mesh(char * fileName, GLuint prog);
	~Mesh();

public:
	void Draw(const glm::mat4 &mvp);
};

class Skeletal_Mesh:Mesh {
	Armature skeleton;
	Mesh mesh; //might move these to be pointers,

	void genMesh(void);

};
*/

/*
Mesh::Mesh(char *fileName, GLuint prog) {
using namespace std;
vector <GLfloat> verticies;
vector <GLuint> elements;

GLuint attrib_vertices;

program = prog;

loadObj(verticies, elements, fileName);

glGenBuffers(1, &ibo_elements);
glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_elements);
glBufferData(GL_ELEMENT_ARRAY_BUFFER, elements.size()*sizeof(elements[0]), elements.data(), GL_STATIC_DRAW);

glGenBuffers(1, &vbo_coord);
glBindBuffer(GL_ARRAY_BUFFER, vbo_coord);
glBufferData(GL_ARRAY_BUFFER, verticies.size()*sizeof(verticies[0]), verticies.data(), GL_STATIC_DRAW);

glGenVertexArrays(1, &vao);
glBindVertexArray(vao);

glBindBuffer(GL_ARRAY_BUFFER, vbo_coord); //not needed, emphasizing that we're using this buffer
attrib_vertices = getAttrib(program, "coord");
glVertexAttribPointer(attrib_vertices, 3, GL_FLOAT, GL_FALSE, 0, NULL);
glEnableVertexAttribArray(attrib_vertices);

uniform_Matrix = getUniform(program, "mvp");
}
Mesh::~Mesh() {
glDeleteBuffers(1, &vbo_coord);
glDeleteBuffers(1, &ibo_elements);
glDeleteVertexArrays(1, &vao);
}
void Mesh::Draw(const glm::mat4 &mvp) {
glBindVertexArray(vao);
glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_elements);
glUniformMatrix4fv(uniform_Matrix, 1, GL_FALSE, glm::value_ptr(mvp));

int size;  glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &size);

glDrawElements(GL_TRIANGLES, size / sizeof(GLuint), GL_UNSIGNED_INT, NULL);
}
*/
#endif