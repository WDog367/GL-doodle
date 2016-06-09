#include "mesh_utils.h"
#include "file_utils.h"
#include "armature_utils.h"
#include "shader_utils.h"

#include <vector>
#include <string>
#include <sstream>
#include <iostream>
#include "GL/glew.h"
#include "glm/glm.hpp"
#include "glm/gtx/transform.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "math.h"

#include <fstream>
///Standard Mesh Stuff~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Mesh::Mesh() {
	glGenVertexArrays(1, &vao);
	glGenBuffers(1, &ibo_elements);
	glGenBuffers(1, &vbo_coord);

	glBindVertexArray(vao);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_elements);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_coord);

	glBindVertexArray(0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

//Initialized the mesh as a cube, clean simple, sweet
Mesh::Mesh(GLuint program) {
	GLuint attrib_vertices;

	GLfloat vertices[] = {
		// front
		-1.0, -1.0,  1.0,
		1.0, -1.0,  1.0,
		1.0,  1.0,  1.0,
		-1.0,  1.0,  1.0,
		// back
		-1.0, -1.0, -1.0,
		1.0, -1.0, -1.0,
		1.0,  1.0, -1.0,
		-1.0,  1.0, -1.0,
	};
	GLuint elements[] = {
		// front
		0, 1, 2,
		2, 3, 0,
		// top
		1, 5, 6,
		6, 2, 1,
		// back
		7, 6, 5,
		5, 4, 7,
		// bottom
		4, 0, 3,
		3, 7, 4,
		// left
		4, 5, 1,
		1, 0, 4,
		// right
		3, 2, 6,
		6, 7, 3,
	};

	//-------------setting up the buffers and other gl-stuff
	this->program = program;

	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	glGenBuffers(1, &ibo_elements);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_elements);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(elements), elements, GL_STATIC_DRAW);

	glGenBuffers(1, &vbo_coord);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_coord);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	using namespace std;
	glBindBuffer(GL_ARRAY_BUFFER, vbo_coord); //not needed, emphasizing that we're using this buffer
	attrib_vertices = getAttrib(program, "coord");
	glVertexAttribPointer(attrib_vertices, 3, GL_FLOAT, GL_FALSE, 0, NULL);
	glEnableVertexAttribArray(attrib_vertices);

	uniform_Matrix = getUniform(program, "mvp");

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}
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

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

Mesh::Mesh(const GLfloat *vertices, int vNum, const GLuint *elements, int eNum, GLuint program) {
	GLuint attrib_vertices;

	this->program = program;

	glGenBuffers(1, &ibo_elements);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_elements);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(elements[0])*eNum, elements, GL_STATIC_DRAW);

	glGenBuffers(1, &vbo_coord);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_coord);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices[0])*vNum, vertices, GL_STATIC_DRAW);

	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	glBindBuffer(GL_ARRAY_BUFFER, vbo_coord);// it is already bound, but this is good for emphasis;
	attrib_vertices = getAttrib(program, "coord");
	glVertexAttribPointer(attrib_vertices, 3, GL_FLOAT, GL_FALSE, 0, NULL);
	glEnableVertexAttribArray(attrib_vertices);

	uniform_Matrix = getUniform(program, "mvp");

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}
Mesh::~Mesh() {
	glDeleteBuffers(1, &vbo_coord);
	glDeleteBuffers(1, &ibo_elements);
	glDeleteVertexArrays(1, &vao);
}
void Mesh::Draw(const glm::mat4 &mvp) {
	glUseProgram(program);

	glBindVertexArray(vao);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_elements);
	glUniformMatrix4fv(uniform_Matrix, 1, GL_FALSE, glm::value_ptr(mvp));

	int size;  glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &size);

	glDrawElements(GL_TRIANGLES, size / sizeof(GLuint), GL_UNSIGNED_INT, NULL);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

void Mesh::updateMesh(const GLfloat *vertices, int vNum, const GLuint *elements, int eNum) {
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_elements);//could probably use GL_COPY_READ_BUFFER as target
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(elements[0])*eNum, elements, GL_STATIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, vbo_coord);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices[0])*vNum, vertices, GL_STATIC_DRAW);

glBindBuffer(GL_ARRAY_BUFFER, 0);
glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

//the attribute information stored in the vao should still be valid
}

void Mesh::updateProgram(GLuint program) {
	GLuint attrib_vertices;
	this->program = program;

	glBindVertexArray(vao);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_coord);
	attrib_vertices = getAttrib(program, "coord");
	glVertexAttribPointer(attrib_vertices, 3, GL_FLOAT, GL_FALSE, 0, NULL);
	glEnableVertexAttribArray(attrib_vertices);

	uniform_Matrix = getUniform(program, "mvp");

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}
//Simple Skeletal Mesh stuff~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Skeletal_Mesh_simple::Skeletal_Mesh_simple(Armature* skeleton, GLuint program) {
	Mesh* newMesh;
	this->skeleton = skeleton;

	for (int i = 0; i < skeleton->size; i++) {
		GLfloat vertices[] = {
			// front
			0.0, -1.0,  0.0,
			1.0, -1.0,  0.0,
			1.0,  1.0,  0.0,
			-1.0,  1.0,  0.0,
			// back
			0.0, 0.0, 1.0
		};
		GLuint elements[] = {
			// front
			0, 1, 2,
			2, 3, 0,
			//peak
			4, 0, 3,
			1, 0, 4,
			2, 1, 4,
			3, 2, 4
		};

		if (skeleton->getJoint(i)->Child.size() > 0) {
			int ind = skeleton->getJoint(i)->Child[0]->index;

			glm::mat4 transform;
			glm::vec3 offset = glm::vec3(skeleton->offset[ind * 3], skeleton->offset[ind * 3 + 1], skeleton->offset[ind * 3 + 2]);
			glm::vec3 axis = glm::vec3(0.0, 0.0, 1.0);
			glm::vec4 curVert;

			//scale it to be the length, and then rotate it to face the child
			transform = glm::rotate(acos(glm::dot(glm::normalize(offset), axis)), glm::cross(axis, glm::normalize(offset)))*
				glm::scale(glm::vec3(1.0, 1.0, glm::length(offset)));

			int size = sizeof(vertices) / sizeof(vertices[0]);

			for (int i = 0; i < size - 2; i += 3) {
				curVert = glm::vec4(vertices[i], vertices[i + 1], vertices[i + 2], 1.0);
				curVert = transform*curVert;

				vertices[i] = curVert.x;
				vertices[i + 1] = curVert.y;
				vertices[i + 2] = curVert.z;
			}
		}
		newMesh = new Mesh(vertices, sizeof(vertices) / sizeof(vertices[0]), elements, sizeof(elements) / sizeof(elements[0]), program);
		mesh.push_back(newMesh);
	}
}

Skeletal_Mesh_simple::~Skeletal_Mesh_simple() {
	for (unsigned int i = 0; i < mesh.size(); i++) {
		delete mesh[i];
	}
	mesh.clear();
	delete skeleton;
}

void Skeletal_Mesh_simple::Draw(const glm::mat4 &mvp) {
	for (unsigned int i = 0; i < mesh.size(); i++) {
		mesh[i]->Draw(mvp*skeleton->matrix[i]);
	}
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~SKELETAL MESH~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void Skeletal_Mesh::updateWeights(const GLuint *bIndex, const GLfloat* bWeight, const GLuint * bNum, int vNum) {
	GLuint attribute_boneIndex;
	GLuint attribute_boneWeight;
	GLuint attribute_boneNum;

	using namespace std;
	cout << "A" << endl;
	cout << bIndex[0] << endl;
	cout << vNum << endl;

	glGenBuffers(1, &vbo_bIndex);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_bIndex);
	glBufferData(GL_ARRAY_BUFFER, (vNum/3 * maxBNum)*sizeof(bIndex[0]), bIndex, GL_DYNAMIC_DRAW);

	glGenBuffers(1, &vbo_bWeight);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_bWeight);
	glBufferData(GL_ARRAY_BUFFER, (vNum / 3 * maxBNum)*sizeof(bWeight[0]), bWeight, GL_DYNAMIC_DRAW);

	glGenBuffers(1, &vbo_bNum);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_bNum);
	glBufferData(GL_ARRAY_BUFFER, (vNum / 3)*sizeof(bNum[0]), bNum, GL_DYNAMIC_DRAW);

	glBindVertexArray(vao);

	glBindBuffer(GL_ARRAY_BUFFER, vbo_bIndex);
	attribute_boneIndex = getAttrib(program, "boneIndex");
	glVertexAttribPointer(attribute_boneIndex, maxBNum, GL_UNSIGNED_INT, GL_FALSE, 0, NULL);
	glEnableVertexAttribArray(attribute_boneIndex);

	glBindBuffer(GL_ARRAY_BUFFER, vbo_bWeight);
	attribute_boneWeight = getAttrib(program, "boneWeight");
	glVertexAttribPointer(attribute_boneWeight, maxBNum, GL_FLOAT, GL_FALSE, 0, NULL);
	glEnableVertexAttribArray(attribute_boneWeight);

	glBindBuffer(GL_ARRAY_BUFFER, vbo_bNum);
	attribute_boneNum = getAttrib(program, "boneNum");
	glVertexAttribIPointer(attribute_boneNum, 1, GL_UNSIGNED_INT, 0, NULL);
	glEnableVertexAttribArray(attribute_boneNum);
}

Skeletal_Mesh::Skeletal_Mesh(const GLfloat *vertices, int vNum, const GLuint *elements, int eNum, Armature *skeleton, GLuint program) : Mesh(vertices, vNum, elements, eNum, program), skeleton(skeleton) {
	std::vector<GLuint> bIndex;
	std::vector<GLfloat> bWeight;
	std::vector<GLuint> bNum;

	GLuint attribute_boneIndex;
	GLuint attribute_boneWeight;
	GLuint attribute_boneNum;

	bIndex.resize(vNum / 3 * 4);//Number of vertices *5
	bWeight.resize(vNum / 3 * 4);
	bNum.resize(vNum / 3);

	/*
	int vertPerBone = vNum / 3 / skeleton->size;
	int curBone = -1;
	for (int i = 0; i < vNum / 3; i++) {
	if (i%vertPerBone == 0) curBone++;

	bIndex[i * 4] = curBone;
	bWeight[i * 4] = 1.0;
	bNum[i] = 1;
	}
	*/

	//automaticall assigns each vertex to follow the bone witht the closest head
	for (int i = 0; i < vNum / 3; i++) {
		int closestBone = 0;
		float dist;
		float minDist;
		glm::vec3 tempVec = glm::vec3(vertices[i * 3], vertices[i * 3 + 1], vertices[i * 3 + 2]);
		glm::vec3 locVec = glm::vec3(glm::inverse(skeleton->invBasePose[0]) * glm::vec4(0, 0, 0, 1));//glm::vec3(skeleton->loc[0], skeleton->loc[0 + 1], vertices[0 + 2]);
		dist = abs(glm::length(tempVec - locVec));
		minDist = dist;
		for (int j = 1; j < skeleton->size; j++) {
			locVec = glm::vec3(glm::inverse(skeleton->invBasePose[j]) * glm::vec4(0, 0, 0, 1));
			dist = abs(glm::length(tempVec - locVec));
			if (dist < minDist) {
				minDist = dist;
				closestBone = j;
			}
		}

		bIndex[i * 4] = closestBone;
		bWeight[i * 4] = 1.0;
		bNum[i] = 1;
	}

	updateWeights(bIndex.data(), bWeight.data(), bNum.data(), vNum);
	//this needs to be in an "updateProgram"
	uniform_BoneTransform = getUniform(program, "boneMatrices");
	bindSpaceMatrix = glm::mat4(1.0);
}

Skeletal_Mesh::Skeletal_Mesh(const GLfloat *vertices, int vNum, const GLuint *elements, int eNum, const GLuint *bIndex, const GLfloat* bWeight, const GLuint * bNum, const glm::mat4 &bindSpaceMatrix, Armature *skeleton, GLuint program) : Mesh(vertices, vNum, elements, eNum, program), bindSpaceMatrix(bindSpaceMatrix), skeleton(skeleton) {
	updateWeights(bIndex, bWeight, bNum, vNum);
	uniform_BoneTransform = getUniform(program, "boneMatrices");
}

void Skeletal_Mesh::Draw(const glm::mat4 &mvp) {
	glUseProgram(program);

	skeleton->setMatrices();

	std::vector<glm::mat4> matrices;

	matrices.resize(skeleton->size);
	for (int i = 0; i < matrices.size(); i++) {
		matrices[i] = skeleton->matrix[i]* bindSpaceMatrix;
	}

	glBindVertexArray(vao);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_elements);
	glUniformMatrix4fv(uniform_Matrix, 1, GL_FALSE, glm::value_ptr(mvp));
	glUniformMatrix4fv(uniform_BoneTransform, skeleton->size, GL_FALSE, (GLfloat *)matrices.data());


	int size;  glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &size);

	glDrawElements(GL_TRIANGLES, size / sizeof(GLuint), GL_UNSIGNED_INT, NULL);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}