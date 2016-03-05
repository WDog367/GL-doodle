#include "armature_utils.h"
#include "shader_utils.h"

#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

#include "math.h"
#include "GL/glew.h"
#include "glm/glm.hpp"//includes most (if not all?) of GLM library
#include "glm/gtx/transform.hpp"
#include "glm/gtc/type_ptr.hpp"

#include "file_utils.h"


bool Armature::loadBVHFull(const char *fileName) {
	if (!loadBVHArm(fileName)) return false;
	if (!loadBVHAnim_quick(fileName)) return false;
	return true;
}

//now non-recursive!
bool Armature::loadBVHArm(const char *fileName) {
	using namespace std;
	
	joint *curArm;
	string line;
	string token;
	istringstream *stream;
	ifstream file;

	vector <string> channelMapType;
	vector <int> channelMapIndex;

	file.open(fileName);
	if (!file) {
		std::cerr << "Error loading file: " << fileName << std::endl;
		return false;
	}

	//resetting the armature, just in case, to get ready for loading
	offset.resize(0);
	rotation.resize(0);
	channelMap.resize(0);
	size = 0;

	//should nuke the joints here as well
	root = new joint;
	root->Parent = NULL;

//	readBVH(root, offset, rotation, channelMap, file);//helper fcn to do it recursively, may change
	
	do { file >> token; } while (token != "HIERARCHY" && !file.fail());//reaching the start of the hierarchy
	if (file.fail()) {
		cerr << "Error reading file: " << fileName << endl;
	}
	
	//reading loop;
	while (!file.fail()) {
		file >> token;

		cout << token << endl;
		if (token == "ROOT" || token == "JOINT") {
		//Adding a new bone to the hierarchy
			if (token == "ROOT") {
				//parent already set
				curArm = root;

			}
			else {
				joint *newArm = new joint;
				curArm->Child.push_back(newArm);
				newArm->Parent = curArm;
				curArm = newArm;
			}
			file >> curArm->name;
			curArm->index = size;
			size++;

			//if these weren't vectors, wouldn't have to bother with this
			//adding more entries to each of these
			offset.resize(offset.size() + 3);
			rotation.resize(rotation.size() + 3);
			matrix.resize(matrix.size() + 1);
			parent.resize(parent.size() + 1);

			if (curArm->Parent != NULL) {
				parent[curArm->index] = curArm->Parent->index;
			}
			else {
				parent[curArm->index] = -1;
			}

			getline(file, line, '{');//moving the file just past the next open brace

		}
		else if (token == "OFFSET") {
			file >> offset[curArm->index * 3];
			file >> offset[curArm->index * 3 + 1];
			file >> offset[curArm->index * 3 + 2];
		}
		else if (token == "CHANNELS") {
			int numChannel;
			file >> numChannel;
			for (int i = 0; i < numChannel; i++) {
				file >> token;
				channelMapType.push_back(token);
				channelMapIndex.push_back(curArm->index);
				//these won't work, pointers to vector elements are bad
				//they'll be fine if I change to array implementation, but for now
				/*
				if (token == "Xposition") {
					this->channelMap.push_back(&(offset[curArm->index * 3 + 0])); // (offset.data() + 0 + curSize)
				}
				else if (token == "Yposition") {
					this->channelMap.push_back(&(offset[curArm->index * 3 + 1]));
				}
				else if (token == "Zposition") {
					this->channelMap.push_back(&(offset[curArm->index * 3 + 2]));
				}
				else if (token == "Xrotation") {
					this->channelMap.push_back(&(rotation[curArm->index * 3 + 0]));
				}
				else if (token == "Yrotation") {
					this->channelMap.push_back(&(rotation[curArm->index * 3 + 1]));
				}
				else if (token == "Zrotation") {
					this->channelMap.push_back(&(rotation[curArm->index * 3 + 2]));
				}
				*/
			}
		}
		else if (token == "End") {
		//not too interested in the information in the "End Site" branch
			getline(file, line, '{');
			getline(file, line, '}');
			//skipping over whatever is in here; using getline because I don't want to write a subprogram
		}
		else if (token == "}") {
			if (curArm->Parent == NULL) {
				break;
			}
			curArm = curArm->Parent;
		}
	}
	
	file.close();

	//setting up the channel map; this should be done in the main part, if array implemetation instead of vector
	channelMap.resize(channelMapType.size());
	for (int i = 0; i < channelMap.size(); i++) {
		if (channelMapType[i] == "Xposition") {
			this->channelMap[i] = (&(offset[channelMapIndex[i] * 3 + 0])); // (offset.data() + 0 + curSize)
		}
		else if (channelMapType[i] == "Yposition") {
			this->channelMap[i] = (&(offset[channelMapIndex[i] * 3 + 1]));
		}
		else if (channelMapType[i] == "Zposition") {
			this->channelMap[i] = (&(offset[channelMapIndex[i] * 3 + 2]));
		}
		else if (channelMapType[i] == "Xrotation") {
			this->channelMap[i] = (&(rotation[channelMapIndex[i] * 3 + 0]));
		}
		else if (channelMapType[i] == "Yrotation") {
			this->channelMap[i] = (&(rotation[channelMapIndex[i] * 3 + 1]));
		}
		else if (channelMapType[i] == "Zrotation") {
			this->channelMap[i] = (&(rotation[channelMapIndex[i] * 3 + 2]));
		}
	}
	cout << "size: " << size << ", predicted: " << offset.size() / 3 << endl;
	//size = offset.size() / 3;
	return true;
}

bool Armature::loadBVHAnim_quick(const char *fileName) {
	//assumes the skeleton will be the exact same, and thus the channel map too
	std::ifstream file;
	std::string token;
	std::string line;

	int frameNum;

	file.open(fileName);
	if (!file) {
		std::cerr << "Error loading file: " << fileName << std::endl;
		return false;
	}

	do { file >> token; } while (token != "MOTION");//reached the Motion
	getline(file, line, ':'); //this line will say how many frames
	file >> frameNum;

	getline(file, line, ':'); //this line will say the length of a frame.
	file >> token; //discarding for now

	frames.resize(frameNum);

	for (int i = 0; i < frameNum; i++) {
		if (frames[i].size() != channelMap.size()) { frames[i].resize(channelMap.size()); }//just to be safe
		for (int j = 0; j < channelMap.size(); j++){
			file >> frames[i][j];
		}
	}

	file.close();
	return true;
}

void Armature::draw(const glm::mat4 &mvp) {
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

	setMatrices();
	for (int i = 0; i < size; i++) {
		glm::vec4 loc = glm::vec4(0, 0, 0, 1);

		loc = matrix[i] * loc;
		skeleton.push_back(loc.x);
		skeleton.push_back(loc.y);
		skeleton.push_back(loc.z);
		skeleton.push_back(loc.w);
	}

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
	glDeleteProgram(program);

}

void Armature::setMatrices() {
	glm::vec3 coffset;
	glm::vec3 crotation;
	
	//this requires that they are ordered such that i > parent[i] for all bones
	for (int i = 0; i < size; i++) {
		coffset = glm::vec3(offset[i * 3], offset[i * 3 + 1], offset[i * 3 + 2]);
		crotation = glm::vec3(rotation[i * 3], rotation[i * 3 + 1], rotation[i * 3 + 2]);

		//first the rotation part;
		matrix[i] = glm::translate(coffset) *
			glm::rotate(crotation.z*(3.14159f) / 180.0f, glm::vec3(0, 0, 1))*
			glm::rotate(crotation.y*(3.14159f) / 180.0f, glm::vec3(0, 1, 0))*
			glm::rotate(crotation.x*(3.14159f) / 180.0f, glm::vec3(1, 0, 0));
		//then multiply by it's parent
		if (parent[i] != -1) {
			matrix[i] = matrix[parent[i]] * matrix[i];
		}
	}

	//recursive version
	//setMatrices(root);
}
void Armature::setMatrices(Armature::joint *arm) {
	glm::vec3 coffset;
	glm::vec3 crotation;

	coffset = glm::vec3(offset[arm->index * 3], offset[arm->index * 3 + 1], offset[arm->index * 3 + 2]);
	crotation = glm::vec3(rotation[arm->index * 3], rotation[arm->index * 3 + 1], rotation[arm->index * 3 + 2]);

	//first the rotation part;
	matrix[arm->index] = glm::translate(coffset) *
		glm::rotate(crotation.z, glm::vec3(0, 0, 1))*
		glm::rotate(crotation.y, glm::vec3(0, 1, 0))*
		glm::rotate(crotation.x, glm::vec3(1, 0, 0));

	if (arm->Parent != NULL) {
		matrix[arm->index] = matrix[arm->Parent->index] * matrix[arm->index];
	}

	for (int i = 0; i < arm->Child.size(); i++) {
		setMatrices(arm->Child[i]);
	}

}

/*
void printArm(Armature::joint *arm, int level) {
	using namespace std;
	for (int i = 0; i < level; i++) cout << "    ";
	cout << "Bone: " << arm->name << ", " << arm->index << " {" << endl;
	for (int i = 0; i < level; i++) cout << "    ";
	cout << arm->offset << endl;
	for (int i = 0; i < level; i++) cout << "    ";
	cout << "Offset: " << arm->offset[0] << ", " << arm->offset[1] << ", " << arm->offset[2] <<  endl;
	for (int i = 0; i < level; i++) cout << "    ";

	if(arm->Parent != NULL)
		cout << "Parent: " << arm->Parent->name << ", " << arm->Parent->index << endl;
		
	cout << endl;
	for (int i = 0; i < arm->Child.size(); i++) {
		printArm(arm->Child[i], level + 1);
	}
}
*/


void Armature::print() {
	using namespace std;

	for (int i = 0; i < size; i++) {
		cout << "Bone: " << i << " {" << endl;
		cout << "Offset: " << offset[i*3] << ", " << offset[i*3 + 1] << ", " << offset[i * 3 + 2] << endl;
		cout << (&(offset[i * 3])) << endl;
		cout << "Parent: " << parent[i] << endl;

		cout << endl;

	}
}