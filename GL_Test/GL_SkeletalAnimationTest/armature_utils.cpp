#include "armature_utils.h"
#include "shader_utils.h"

#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

#include "math.h"
#include "GL/glew.h"
#include "GL/freeglut.h"//may want to start using "GL/glut.h" to wean off of freeglut exclusive stuff?
#include "glm/glm.hpp"//includes most (if not all?) of GLM library
#include "glm/gtx/transform.hpp"
#include "glm/gtc/type_ptr.hpp"

#include "file_utils.h"

void readBVH(joint *arm, std::ifstream &file, std::vector<float*> &channelMap, std::vector<std::vector<float>> &frames) {
	using namespace std;
	string line;
	string token;
	istringstream *stream;
	
	file >> token;

	if (token == "HIERARCHY") {
		//the main one, starting the file
		int frameNum;
		
		arm->Parent = NULL;

		do file >> token; while (token != "ROOT");//reached the Root
		getline(file, line, '{');

		stream = new istringstream(line);

		*stream >> token;
		arm->name = token;
		delete stream;

		readBVH(arm, file, channelMap, frames);
		
		do file >> token; while (token != "MOTION");//reached the Motion
		getline(file, line, ':'); //this line will say how many frames
		file >> frameNum;


		getline(file, line, ':'); //this line will say the length of a frame.
		file >> token; //discarding for now
		
		frames.resize(frameNum);

		for (int i = 0; i < frameNum; i++) {
			do {
				getline(file, line); // this line will be the base pose
			} while (line.length() == 0);

			stream = new istringstream(line);
			while (*stream >> token) {
				frames[i].push_back(stof(token));
			}
			delete stream;
		}
	}
	else if (token == "OFFSET") {
		int numChannels;

		file >> arm->offset[0];
		file >> arm->offset[1];
		file >> arm->offset[2];

		do file >> token; while (token != "CHANNELS");//reached the Root
		file >> numChannels;

		for (int i = 0; i < numChannels; i++) {
			file >> token;
			if (token == "Xposition") {
				channelMap.push_back(&(arm->offset[0]));
			}
			else if (token == "Yposition") {
				channelMap.push_back(&(arm->offset[1]));
			}
			else if (token == "Zposition") {
				channelMap.push_back(&(arm->offset[2]));
			}
			else if (token == "Xrotation") {
				channelMap.push_back(&(arm->rotation[0]));
			}
			else if (token == "Yrotation") {
				channelMap.push_back(&(arm->rotation[1]));
			}
			else if (token == "Zrotation") {
				channelMap.push_back(&(arm->rotation[2]));
			}
		}

		while (token != "}"){
			file >> token;
			if (token == "JOINT") {
				joint *child;
								
				getline(file, line, '{');

				stream = new istringstream(line);
				*stream >> token;
				delete stream;

				child = new joint;
				arm->Child.push_back(child);
				child->Parent = arm;
				child->name = token;
				
				readBVH(child, file, channelMap, frames);
			}
			else if (token == "End") {
				getline(file, line, '}');
				//the "End Site" contains the length of the last bone
				//but I don't think I have a use for that data, so
				//just skipping over it
			}
		}
	}
}

void loadBVH(Arm &arm, const char *fileName) {
	std::ifstream file;

	file.open(fileName);
	if (!file) {
		std::cerr << "Error loading file: " << fileName << std::endl;
		return;
	}

	arm.root = new joint;

	readBVH(arm.root, file, arm.channelMap, arm.frames);

	//putting the skeleton in it's base pose
	for (int i = 0; i < arm.channelMap.size(); i++) {
		*(arm.channelMap[i]) = arm.frames[0][i];
	}
}

void drawJoint(joint *root, const glm::mat4 &objMat) {
	glm::mat4 jointMat;

	glm::vec3 offset = glm::vec3(root->offset[0], root->offset[1], root->offset[2]);
	glm::vec3 rotation = glm::vec3(root->rotation[0], root->rotation[1], root->rotation[2]);

	jointMat = objMat*glm::translate(offset)*
		glm::rotate(rotation.x, glm::vec3(1, 0, 0))*
		glm::rotate(rotation.y, glm::vec3(0, 1, 0))*
		glm::rotate(rotation.z, glm::vec3(0, 0, 1));

	for (int i = 0; i < root->Child.size(); i++) {
		drawJoint(root->Child[i], jointMat);
	}

	int size;  glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &size);
	glDrawElements(GL_TRIANGLES, size / sizeof(GLuint), GL_UNSIGNED_INT, NULL);

}

void vectorize(joint *root, const glm::mat4 &mat,  std::vector<GLfloat> &vect) {
	glm::mat4 jointMat;

	glm::vec3 offset = glm::vec3(root->offset[0], root->offset[1], root->offset[2]);
	glm::vec3 rotation = glm::vec3(root->rotation[0], root->rotation[1], root->rotation[2]);

	glm::vec4 loc;

	glm::mat4 x = glm::rotate(rotation.x*(3.14159f) / 180.0f, glm::vec3(1, 0, 0));
	glm::mat4 y = glm::rotate(rotation.y*(3.14159f) / 180.0f, glm::vec3(0, 1, 0));
	glm::mat4 z = glm::rotate(rotation.z*(3.14159f) / 180.0f, glm::vec3(0, 0, 1));

	//I'm reasonably sure that  prev*offset*z*y*x is the right order
	jointMat = mat*glm::translate(offset)*z*y*x;
	
	loc = (jointMat*glm::vec4(0, 0, 0, 1));

	vect.push_back(loc.x);
	vect.push_back(loc.y);
	vect.push_back(loc.z);
	vect.push_back(loc.w);

	for (int i = 0; i < root->Child.size(); i++) {
		vectorize(root->Child[i], jointMat, vect);
	}
}

void drawArm(Arm& arm, const glm::mat4 &mvp) {
	using namespace std;
	
	GLuint vao;
	GLuint vbo;
	GLuint program;
	GLuint attrib_coord;
	GLuint uniform_matrix;

	vector <GLfloat> vertices;
	vector <GLfloat> colours;
	vector <GLuint> elements;

	loadObj(vertices, elements, "object");


	std::vector<GLfloat> skeleton;

	vectorize(arm.root, glm::mat4(1.0f), skeleton);

	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(skeleton[0])*skeleton.size(), skeleton.data(), GL_STATIC_DRAW);

	struct shaderInfo shaders[] = { { GL_VERTEX_SHADER, "vs.glsl" },
									{ GL_FRAGMENT_SHADER, "fs.glsl" } };

	program = LoadProgram(shaders, 2);
	glUseProgram(program);

	attrib_coord = getAttrib(program, "coord");
	glEnableVertexAttribArray(attrib_coord);
	glVertexAttribPointer(
		attrib_coord,	//attrib 'index'
		4,				//pieces of data per index
		GL_FLOAT,		//type of data
		GL_FALSE,		//whether to normalize
		0,				//space b\w data
		0				//offset from buffer start
		);

	uniform_matrix = getUniform(program, "matrix");

	glUniformMatrix4fv(uniform_matrix, 1, GL_FALSE, glm::value_ptr(mvp));

	glEnable(GL_PROGRAM_POINT_SIZE);
	glDrawArrays(GL_POINTS, 0, skeleton.size());

	glDeleteBuffers(1, &vbo);
	glDeleteVertexArrays(1, &vao);

}

void printArm(joint *arm, int level) {
	using namespace std;
	for (int i = 0; i < level; i++) cout << "    ";
	cout << "Bone: " << arm->name << " {" << endl;
	for (int i = 0; i < level; i++) cout << "    ";
	cout << "Offset: " << arm->offset[0] << ", " << arm->offset[1] << ", " << arm->offset[2] <<  endl;
	for (int i = 0; i < level; i++) cout << "    ";

	if(arm->Parent != NULL)
		cout << "Parent: " << arm->Parent->name << endl;
		
	cout << endl;
	for (int i = 0; i < arm->Child.size(); i++) {
		printArm(arm->Child[i], level + 1);
	}
}

void nukeJoint(joint *root) {
	for (int i = 0; i < root->Child.size(); i++) {
		nukeJoint(root->Child[i]);
	}
	delete root;

}

void nukeArm(Arm &arm) {
	for (int i = 0; i < arm.frames.size(); i++) {
		arm.frames[i].resize(0);
	}
	arm.frames.resize(0);

	arm.channelMap.resize(0);

	nukeJoint(arm.root);
}