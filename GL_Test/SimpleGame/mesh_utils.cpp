#include "mesh_utils.h"
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
//Helper function for loading .obj files
void loadObj(std::vector<float> &vertices, std::vector<unsigned int> &elements, char *fileName);

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
	uniform_collision = getUniform(program, "collision");

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	vCount = 8;
	drawShape = GL_TRIANGLES;
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

	uniform_collision = getUniform(program, "collision");

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	vCount = verticies.size() / 3;
	drawShape = GL_TRIANGLES;
}

Mesh::Mesh(const GLfloat *vertices, int vNum, const GLuint *elements, int eNum, GLuint program, GLuint drawShape = GL_TRIANGLES) {
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

	uniform_collision = getUniform(program, "collision");

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	vCount = vNum / 3;
	this->drawShape = drawShape;
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
	glUniform1i(uniform_collision, (int)(collision));

	int size;  glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &size);

	glDrawElements(drawShape, size / sizeof(GLuint), GL_UNSIGNED_INT, NULL);

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
vCount = vNum / 3;
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

GLuint Mesh::getProgram() {
	return program;
}

void loadObj(std::vector<float> &vertices, std::vector<unsigned int> &elements, char *fileName) {
	using namespace std;
	ifstream file;
	//istringstream sline;
	string line;
	string token;

	int in_int;
	float in_float;

	file.open(fileName);
	if (!fileName) {
		cerr << "Error loading .obj: " << fileName << endl;
		return;
	}
	//.obj format:
	//v lines have vertices
	//f lines have face (indices)
	while (getline(file, line)) {

		if (line.substr(0, 2) == "v ") {
			//Loading a vertex
			istringstream sline(line.substr(2));
			//getting the three vertex coordinates on this line
			for (int i = 0; i < 3; i++) {//there might be a fourth component, but it's not necessarily useful
				sline >> in_float;
				vertices.push_back(in_float);
			}
			//vertices.push_back(1.0f);

		}
		else if (line.substr(0, 2) == "f ") {
			//Loading a face
			istringstream sline(line.substr(2));;

			for (int i = 0; i < 3; i++) {
				string temp;

				sline >> token;
				if (token.find("/") != -1) {
					in_int = atoi(token.substr(0, token.find("/")).c_str());
				}
				else {
					in_int = atoi(token.c_str());
				}
				elements.push_back(in_int - 1);

			}
		}
	}

	file.close();
}