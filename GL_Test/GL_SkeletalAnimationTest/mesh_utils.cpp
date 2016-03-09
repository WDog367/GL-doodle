#include "mesh_utils.h"
#include "file_utils.h"
#include "armature_utils.h"
#include "shader_utils.h"

#include <vector>
#include <string>
#include <iostream>
#include "GL/glew.h"
#include "glm/glm.hpp"
#include "glm/gtx/transform.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "math.h"


///Standard Mesh Stuff~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
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

	uniform_Matrix = getUniform(program, "matrix");
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

	uniform_Matrix = getUniform(program, "matrix");
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

	uniform_Matrix = getUniform(program, "matrix");

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
	for (int i = 0; i < mesh.size(); i++) {
		delete mesh[i];
	}
	mesh.clear();
	delete skeleton;
}

void Skeletal_Mesh_simple::Draw(const glm::mat4 &mvp) {
	for (int i = 0; i < mesh.size(); i++) {
		mesh[i]->Draw(mvp*skeleton->matrix[i]);
	}
}