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

Animation::Animation() {
	curTime = 0;
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~Neat Animation~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~Simple keyframe Animation~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


Animation_Simple::Animation_Simple(const int mapSize, const std::string *channelMapType, const int *channelMapIndex, const std::vector<std::vector<GLfloat>> &frames, const int frameTime,
	const int size, const GLfloat rot[], const GLfloat off[]) {
	rotation.resize(size * 3);
	offset.resize(size * 3);

	for (int i = 0; i < size * 3; i++) {
		rotation[i] = rot[i];
		offset[i] = off[i];
	}

	channelMap.resize(mapSize);
	for (int i = 0; i < channelMap.size(); i++) {
		if (channelMapType[i] == "Xposition") {
			channelMap[i] = (&(offset[channelMapIndex[i] * 3 + 0])); // (offset.data() + 0 + curSize)
		}
		else if (channelMapType[i] == "Yposition") {
			channelMap[i] = (&(offset[channelMapIndex[i] * 3 + 1]));
		}
		else if (channelMapType[i] == "Zposition") {
			channelMap[i] = (&(offset[channelMapIndex[i] * 3 + 2]));
		}
		else if (channelMapType[i] == "Xrotation") {
			channelMap[i] = (&(rotation[channelMapIndex[i] * 3 + 0]));
		}
		else if (channelMapType[i] == "Yrotation") {
			channelMap[i] = (&(rotation[channelMapIndex[i] * 3 + 1]));
		}
		else if (channelMapType[i] == "Zrotation") {
			channelMap[i] = (&(rotation[channelMapIndex[i] * 3 + 2]));
		}
	}

	this->frames.resize(frames.size());
	for (int i = 0; i < frames.size(); i++) {
		for (int j = 0; j < channelMap.size(); j++) {
			this->frames[i].push_back(frames[i][j]);
		}
	}

	curFrame = 0;
	nextFrame = 1;
}

void Animation_Simple::tick(int dt) {
	curTime += dt;
	if (curTime > frameLength) {
		curFrame = nextFrame;
		nextFrame++;
		if (nextFrame > frames.size() - 1) {
			nextFrame = 0;
		}

		//update the rotation and offset
		for (int i = 0; i < channelMap.size(); i++) {
			*(channelMap[i]) = frames[curFrame][i];
		}
	}
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ARMATURE STUFF~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void Armature::tick(int dt) {
	GLfloat *newRot;
	GLfloat *newOff;

	if (anim != NULL) {
		anim->tick(dt);

		newRot = anim->getRot();
		newOff = anim->getOff();

		for (int i = 0; i < rotation.size(); i++) {
			rotation[i] = newRot[i];
			offset[i] = newOff[i];
		}
	}

	setMatrices_simple();
}

bool Armature::loadBVHFull(const char *fileName) {
	if (!loadBVHArm(fileName)) return false;
	if (!loadBVHAnim(fileName)) return false;
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

	std::vector <std::string> channelMapType;
	std::vector <int> channelMapIndex;
	std::vector<bool> mapTypeIsRotation;

	file.open(fileName);
	if (!file) {
		std::cerr << "Error loading file: " << fileName << std::endl;
		return false;
	}

	//resetting the armature, just in case, to get ready for loading
	offset.resize(0);
	rotation.resize(0);
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

	do { file >> token; } while (token != "MOTION" && !file.fail());//reaching the start of the hierarchy
	if (file.fail()) {
		std::cerr << "Error reading file: " << fileName << std::endl;
	}
	getline(file, line, ':');//discard these lines
	getline(file, line);
	getline(file, line);

	//reading the binding pose
	for (int i = 0; i < channelMapIndex.size(); i++) {
		if (channelMapType[i] == "Xposition") {
			file >> (offset[channelMapIndex[i] * 3 + 0]); // (offset.data() + 0 + curSize)
		}
		else if (channelMapType[i] == "Yposition") {
			file >> (offset[channelMapIndex[i] * 3 + 1]);
		}
		else if (channelMapType[i] == "Zposition") {
			file >> (offset[channelMapIndex[i] * 3 + 2]);
		}
		else if (channelMapType[i] == "Xrotation") {
			file >> (rotation[channelMapIndex[i] * 3 + 0]);
		}
		else if (channelMapType[i] == "Yrotation") {
			file >> (rotation[channelMapIndex[i] * 3 + 1]);
		}
		else if (channelMapType[i] == "Zrotation") {
			file >> (rotation[channelMapIndex[i] * 3 + 2]);
		}
	}
	file.close();

	setMatrices_simple();
	for (int i = 0; i < matrix.size(); i++) {
		invBasePose.push_back(glm::inverse(matrix[i]));
	}

	//This is basically deprecated
	loc.resize(size * 3);
	for (int i = 0; i < size; i++){
		for (int j = 0; j < 3; j++) {
			if (parent[i] != -1) {
				loc[i*3 + j] = offset[i*3 + j] + loc[parent[i]*3 + j];
			}
			else {
				loc[i*3 + j] = offset[i * 3 + j];
			}
		}
	}

	return true;
}

bool Armature::loadBVHAnim(const char *fileName) {
	//assumes the skeleton will be the exact same, and thus the channel map too
	std::ifstream file;
	std::string token;
	std::string line;

	std::vector <std::string> channelMapType;
	std::vector <int> channelMapIndex;
	std::vector<bool> mapTypeIsRotation;

	std::vector<std::vector<GLfloat>> frames;

	int frameNum;
	float frameTime;

	int index = -1;


	file.open(fileName);
	if (!file) {
		std::cerr << "Error loading file: " << fileName << std::endl;
		return false;
	}


	do { file >> token; } while (token != "HIERARCHY" && !file.fail());//reaching the start of the hierarchy
	if (file.fail()) {
		std::cerr << "Error reading file: " << fileName << std::endl;
	}

	//reading loop;
	while (!file.fail()) {
		file >> token;

		if (token == "ROOT" || token == "JOINT") {
			index++;
		}
		else if (token == "CHANNELS") {
			int numChannel;
			file >> numChannel;
			for (int i = 0; i < numChannel; i++) {
				file >> token;
				channelMapType.push_back(token);
				channelMapIndex.push_back(index);
			}
		}
		else if (token == "MOTION") {
			break;
		}
	}

	//reached the Motion
	getline(file, line, ':'); //this line will say how many frames
	file >> frameNum;

	getline(file, line, ':'); //this line will say the length of each frame.
	file >> frameTime;

	frames.resize(frameNum);

	for (int i = 0; i < frameNum; i++) {
		if (frames[i].size() != channelMapType.size()) { frames[i].resize(channelMapType.size()); }//just to be safe
		for (int j = 0; j < channelMapType.size(); j++){
			file >> frames[i][j];
		}
	}

	file.close();

	anim = new Animation_Simple(channelMapType.size(), channelMapType.data(), channelMapIndex.data(), frames, frameTime*1000, this->size, this->rotation.data(), this->offset.data());

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

	std::vector<GLfloat> skeleton;

	setMatrices_simple();
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

	uniform_matrix = getUniform(program, "mvp");

	glUniformMatrix4fv(uniform_matrix, 1, GL_FALSE, glm::value_ptr(mvp));

	glEnable(GL_PROGRAM_POINT_SIZE);
	glDrawArrays(GL_POINTS, 0, skeleton.size());

	glDeleteBuffers(1, &vbo);
	glDeleteVertexArrays(1, &vao);
	glDeleteProgram(program);

}

void Armature::setMatrices_simple() {
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
}

void Armature::setMatrices() {
	glm::vec3 cloc;
	glm::vec3 crotation;	
	glm::vec3 coffset;

	//this requires that they are ordered such that i > parent[i] for all bones
	for (int i = 0; i < size; i++) {
		cloc = glm::vec3(loc[i * 3], loc[i * 3 + 1], loc[i * 3 + 2]);
		crotation = glm::vec3(rotation[i * 3], rotation[i * 3 + 1], rotation[i * 3 + 2]);
		coffset = glm::vec3(offset[i * 3], offset[i * 3 + 1], offset[i * 3 + 2]);

		//first the rotation part;
		matrix[i] = glm::translate(coffset)*
			glm::rotate(crotation.z*(3.14159f) / 180.0f, glm::vec3(0, 0, 1))*
			glm::rotate(crotation.y*(3.14159f) / 180.0f, glm::vec3(0, 1, 0))*
			glm::rotate(crotation.x*(3.14159f) / 180.0f, glm::vec3(1, 0, 0));
		//then multiply by it's parent
		if (parent[i] != -1) {
			matrix[i] = matrix[parent[i]] * matrix[i];
		}
	}

	for (int i = 0; i < size; i++) {
		matrix[i] = matrix[i] * invBasePose[i];
	}
}

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

Armature::joint * Armature::getJoint(std::string name) {
	return getJoint(root, name);

}

Armature::joint * Armature::getJoint(joint* root, std::string name) {
	joint* result = NULL;
	if (root->name == name) {
		result = root;
	}
	else {
		for (int i = 0; i < root->Child.size() && result == NULL; i++) {
			result = getJoint(root->Child[i], name);
		}
	}

	return result;
}

Armature::joint * Armature::getJoint(int index) {
	return getJoint(root, index);
}

Armature::joint * Armature::getJoint(joint* root, int index) {
	joint* result = NULL;
	if (root->index == index) {
		result = root;
	}
	else {
		for (int i = 0; i < root->Child.size() && result == NULL; i++) {
			result = getJoint(root->Child[i], index);
		}
	}

	return result;
}