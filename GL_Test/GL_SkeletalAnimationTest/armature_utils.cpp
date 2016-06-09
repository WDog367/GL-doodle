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
#include <cassert>
#include <iomanip>

void printmat4n(glm::mat4 &a) {
	std::cout << std::setprecision(5);
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			std::cout << a[j][i] << ", ";
		}
		std::cout << "\n";
	}
}
void printvec4n(glm::vec4 &a) {
	using namespace std;
	for (int j = 0; j < 4; j++) {
		std::cout << a[j] << ", ";
	}
}
Animation::Animation() {
	curTime = 0;
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~Sampler Functionality~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

glm::mat4 Sampler::getMatrix(float t) {
	using namespace std;
	int curMat = -1, nextMat;
	float t1, t2;
	float weight;
	glm::mat4 result;

	while (curMat < frameNum - 1 && keyTime[curMat + 1] < t) {
		curMat++;
	}

	if (curMat == frameNum - 1) {
		nextMat = -1;
	}
	else {
		nextMat = curMat + 1;
	}

	if (curMat != -1) { t = t - keyTime[curMat]; }

	t1 = ((curMat == -1) ? 0 : keyTime[curMat]);
	t2 = ((nextMat == -1) ? length : keyTime[nextMat]);
	weight = t / (t2 - t1);

	result = glm::mat4(0.0);
	if (curMat != -1) result += (1 - weight)*keyTransform[curMat];
	if (nextMat != -1) result += (weight) * keyTransform[nextMat];

	return result;

}


void Sampler::addKey(const GLfloat &newTime, const glm::mat4 &newTransform) {
	int i = 0;
	std::vector<GLfloat>::iterator it;

	//finding the proper place to 
	using namespace std;
	cout << "Framenum v size" << frameNum << ", " << keyTime.size() << ", " << keyTransform.size() << endl;
	while (i < frameNum) {
		if (newTime < keyTime[i]) {
			keyTime.insert(keyTime.begin() + i, newTime);
			keyTransform.insert(keyTransform.begin() + i, newTransform);
			frameNum++;
			return;
		}
		else if (newTime == keyTime[i]) {
			keyTime[i] = newTime;
			keyTransform[i] = newTransform;
			return;
		}
		i++;
	}

	keyTime.push_back(newTime);
	keyTransform.push_back(newTransform);
	frameNum++;
	if (newTime > length) length = newTime;
}

Sampler::Sampler() : frameNum(0), length(0) {}

Sampler::Sampler(GLfloat* time, glm::mat4* transform, int frameNum) {
	this->frameNum = 0;
	for (int i = 0; i < frameNum; i++) {
		addKey(time[i], transform[i]);
	}
	length = keyTime[frameNum - 1];
}

Sampler::Sampler(GLfloat* time, glm::mat4* transform, int frameNum, GLfloat length) : Sampler(time, transform, frameNum) {
	if (length > this->length) {
		this->length = length;
	}
	else {
		std::cerr << "invalid length passed to sampler" << std::endl;
	}
}

Sampler::Sampler(std::vector<GLfloat> time, std::vector<glm::mat4> transform) : Sampler(time.data(), transform.data(), time.size()) {}
Sampler::Sampler(std::vector<GLfloat> time, std::vector<glm::mat4> transform, GLfloat length) : Sampler(time.data(), transform.data(), time.size(), length) {}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~Slerp Sampler~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~



//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~Sampler Animation~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void Animation_Sampler::tick(int dt) {
	curTime += dt;

	if (curTime > length * 1000) {
		curTime -= length * 1000;
	}

	using namespace std;
	for (int i = 0; i < channelMap.size(); i++) {
		transform[channelMap[i]] = sampler[i]->getMatrix((float)curTime / 1000);
	}

}
void Animation_Sampler::addSampler(Sampler* sampler, std::string channelName) {
	this->sampler.push_back(sampler);
	this->channelName.push_back(channelName);

	if (sampler->length > length)length = sampler->length;
	channelNum++;
	transform.resize(channelNum);
}

Animation_Sampler::Animation_Sampler() {
	channelNum = 0;
	length = 0;
}

Animation_Sampler::Animation_Sampler(std::vector<Sampler*> sampler, std::vector<std::string> channelName) : sampler(sampler), channelName(channelName) {
	channelNum = sampler.size();
	transform.resize(channelNum);
	for (int i = 0; i < channelNum; i++)
		if (length < sampler[i]->length) length = sampler[i]->length;//really shouldn't have animations where channels have different lengths
}
Animation_Sampler::Animation_Sampler(std::vector<Sampler*> sampler, std::vector<std::string> channelName, float length) : Animation_Sampler(sampler, channelName) {
	if (this->length < length) this->length = length;//may want to remove this 'clipping'
}
Animation_Sampler::Animation_Sampler(Sampler** sampler, std::string* channelNames, int channelNum, float length) : channelNum(channelNum), length(length) {
	this->sampler.resize(channelNum);
	this->channelName.resize(channelNum);
	transform.resize(channelNum);

	for (int i = 0; i < channelNum; i++) {
		this->sampler[i] = sampler[i];
		this->channelName[i] = channelNames[i];
	}
}
Animation_Sampler::~Animation_Sampler() {
	for (int i = 0; i < sampler.size(); i++) {
		delete sampler[i];
	}
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~Simple keyframe Animation~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


Animation_Simple::Animation_Simple(const int mapSize, const std::string *channelMapType, const int *channelMapIndex, const std::vector<std::vector<GLfloat>> &frames, const int frameTime,
	const int size, const GLfloat rot[], const GLfloat off[]) {
	rotation.resize(size * 3);
	offset.resize(size * 3);
	transform.resize(size);

	for (int i = 0; i < size * 3; i++) {
		glm::value_ptr(rotation[0])[i] = rot[i];
		glm::value_ptr(offset[0])[i] = off[i];
	}

	simpleChannelMap.resize(mapSize);
	for (int i = 0; i < simpleChannelMap.size(); i++) {
		if (channelMapType[i] == "Xposition") {
			simpleChannelMap[i] = (&(offset[channelMapIndex[i]].x)); // (offset.data() + 0 + curSize)
		}
		else if (channelMapType[i] == "Yposition") {
			simpleChannelMap[i] = (&(offset[channelMapIndex[i]].y));
		}
		else if (channelMapType[i] == "Zposition") {
			simpleChannelMap[i] = (&(offset[channelMapIndex[i]].z));
		}
		else if (channelMapType[i] == "Xrotation") {
			simpleChannelMap[i] = (&(rotation[channelMapIndex[i]].x));
		}
		else if (channelMapType[i] == "Yrotation") {
			simpleChannelMap[i] = (&(rotation[channelMapIndex[i]].y));
		}
		else if (channelMapType[i] == "Zrotation") {
			simpleChannelMap[i] = (&(rotation[channelMapIndex[i]].z));
		}
	}

	this->frames.resize(frames.size());
	for (int i = 0; i < frames.size(); i++) {
		for (int j = 0; j < simpleChannelMap.size(); j++) {
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
		for (int i = 0; i < simpleChannelMap.size(); i++) {
			*(simpleChannelMap[i]) = frames[curFrame][i];
		}
		for (int i = 0; i < transform.size(); i++) {
			glm::vec3 coffset = offset[i];
			glm::vec3 crotation = rotation[i];

			transform[i] = glm::translate(coffset)*
				glm::rotate(crotation.z*(3.14159f) / 180.0f, glm::vec3(0, 0, 1))*
				glm::rotate(crotation.y*(3.14159f) / 180.0f, glm::vec3(0, 1, 0))*
				glm::rotate(crotation.x*(3.14159f) / 180.0f, glm::vec3(1, 0, 0));
		}

	}
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ARMATURE STUFF~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Armature::Armature() {
	size = 0;
	root = NULL;
	anim = NULL;
}

Armature::Armature(std::vector<glm::mat4> baseTrans, std::vector<GLint> parent, std::vector<std::string> name) : baseTrans(baseTrans), transform(baseTrans), parent(parent){
	size = baseTrans.size();//should also check that all of the sizes line up, but let's not worry about that

	joint* curJoint;

	//construct linked list
	for (int i = 0; i < size; i++) {
		assert(parent[i] < i);
		curJoint = new joint;

		curJoint->index = i;
		curJoint->name = name[i];
		
		if (parent[i] != -1) {
			curJoint->Parent = getJoint(parent[i]);
			curJoint->Parent->Child.push_back(curJoint);
		}
		else {
			curJoint->Parent = NULL;
			root = curJoint;
		}
	}

	invBasePose.resize(size);
	matrix.resize(size);
	setMatrices_simple();

	for (int i = 0; i < size; i++) {
		invBasePose[i] = glm::inverse(matrix[i]);
	}
	matrix.resize(size);

	anim = NULL;
}

void Armature::tick(int dt) {
	GLfloat *newRot;
	GLfloat *newOff;
	glm::mat4 *newTrans;
	if (anim != NULL) {
		anim->tick(dt);
		newTrans = anim->getTrans();

		for (int i = 0; i < size; i++) {
			transform[i] = newTrans[i];
		}
	}

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

	glm::vec3 coffset;
	glm::vec3 crotation;

	baseTrans.resize(size);

	//this requires that they are ordered such that i > parent[i] for all bones
	for (int i = 0; i < size; i++) {
		coffset = glm::vec3(offset[i * 3], offset[i * 3 + 1], offset[i * 3 + 2]);
		crotation = glm::vec3(rotation[i * 3], rotation[i * 3 + 1], rotation[i * 3 + 2]);

		//first the rotation part;
		baseTrans[i] = glm::translate(coffset) *
			glm::rotate(crotation.z*(3.14159f) / 180.0f, glm::vec3(0, 0, 1))*
			glm::rotate(crotation.y*(3.14159f) / 180.0f, glm::vec3(0, 1, 0))*
			glm::rotate(crotation.x*(3.14159f) / 180.0f, glm::vec3(1, 0, 0));
		matrix[i] = baseTrans[i];

		//then multiply by it's parent
		if (parent[i] != -1) {
			matrix[i] = matrix[parent[i]] * matrix[i];
		}
	}
	for (int i = 0; i < matrix.size(); i++) {
		invBasePose.push_back(glm::inverse(matrix[i]));
	}

	transform.resize(size);
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

	std::vector<Sampler*> sampler(this->size);
	std::vector<std::string> name(this->size);
	std::vector<glm::vec3> off(this->size);
	std::vector<glm::vec3> rot(this->size);
	std::vector<glm::mat4> trans(this->size);

	for (int i = 0; i < this->size; i++) {
		sampler[i] = new Sampler;
		name[i] = getJoint(i)->name;
	}
	using namespace std;
	for (int i = 0; i < this->size; i++) {
		off[i] = glm::vec3(this->offset[i*3], this->offset[i * 3 +1], this->offset[i * 3 +2]);
		rot[i] = glm::vec3(this->rotation[i * 3], this->rotation[i * 3 + 1], this->rotation[i * 3 + 2]);
	}

	for (int i = 0; i < frameNum; i+= 10) {
		for (int j = 0; j < channelMapType.size(); j++) {
			if (channelMapType[j] == "Xposition") {
				off[channelMapIndex[j]].x = frames[i][j]; // (offset.data() + 0 + curSize)
			}
			else if (channelMapType[j] == "Yposition") {
				off[channelMapIndex[j]].y = frames[i][j];
			}
			else if (channelMapType[j] == "Zposition") {
				off[channelMapIndex[j]].z = frames[i][j];
			}
			else if (channelMapType[j] == "Xrotation") {
				rot[channelMapIndex[j]].x = frames[i][j];
			}
			else if (channelMapType[j] == "Yrotation") {
				rot[channelMapIndex[j]].y = frames[i][j];
			}
			else if (channelMapType[j] == "Zrotation") {
				rot[channelMapIndex[j]].z = frames[i][j];
			}
		}

		for (int j = 0; j < this->size; j++) {
			trans[j] = glm::translate(off[j])*
				glm::rotate(rot[j].z*(3.14159f) / 180.0f, glm::vec3(0, 0, 1))*
				glm::rotate(rot[j].y*(3.14159f) / 180.0f, glm::vec3(0, 1, 0))*
				glm::rotate(rot[j].x*(3.14159f) / 180.0f, glm::vec3(1, 0, 0));
			sampler[j]->addKey(i*frameTime, trans[j]);
		}
	}

	//anim = new Animation_Simple(channelMapType.size(), channelMapType.data(), channelMapIndex.data(), frames, frameTime * 1000, this->size, this->rotation.data(), this->offset.data());
	anim = new Animation_Sampler(sampler, name);;

	anim->channelMap.resize(this->size);
	for (int i = 0; i < this->size; i++) {
		anim->channelMap[i] = getJoint(name[i])->index;
	}

	return true;
}

void Armature::draw(const glm::mat4 &mvp) {
	using namespace std;
	
	GLuint vao;
	GLuint vbo;
	GLuint vbo_index;
	GLuint program;
	GLuint attrib_coord;
	GLuint attrib_index;
	GLuint uniform_matrix;

	vector <GLfloat> vertices;
	vector <GLfloat> colours;
	vector <GLuint> elements;
	vector <GLuint> index;

	std::vector<GLfloat> skeleton;

	setMatrices_simple();
	for (int i = 0; i < size; i++) {
		glm::vec4 loc = glm::vec4(0, 0, 0, 1);

		loc = matrix[i] * loc;
		skeleton.push_back(loc.x);
		skeleton.push_back(loc.y);
		skeleton.push_back(loc.z);
		skeleton.push_back(loc.w);
		index.push_back(i);
	}

	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(skeleton[0])*skeleton.size(), skeleton.data(), GL_STATIC_DRAW);

	glGenBuffers(1, &vbo_index);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_index);
	glBufferData(GL_ARRAY_BUFFER, sizeof(index[0])*index.size(), index.data(), GL_STATIC_DRAW);

	struct shaderInfo shaders[] = { { GL_VERTEX_SHADER, "vs.glsl" },
									{ GL_FRAGMENT_SHADER, "fs.glsl" } };

	program = LoadProgram(shaders, 2);
	glUseProgram(program);

	glBindBuffer(GL_ARRAY_BUFFER, vbo);
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


	glBindBuffer(GL_ARRAY_BUFFER, vbo_index);
	attrib_index = getAttrib(program, "index");
	glEnableVertexAttribArray(attrib_index);
	glVertexAttribPointer(
		attrib_index,	//attrib 'index'
		1,				//pieces of data per index
		GL_UNSIGNED_INT,		//type of data
		GL_FALSE,		//whether to normalize
		0,				//space b\w data
		0				//offset from buffer start
		);


	uniform_matrix = getUniform(program, "mvp");

	glUniformMatrix4fv(uniform_matrix, 1, GL_FALSE, glm::value_ptr(mvp));

	glEnable(GL_TEXTURE_2D);
	glEnable(GL_POINT_SPRITE);
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
		/*
		coffset = glm::vec3(offset[i * 3], offset[i * 3 + 1], offset[i * 3 + 2]);
		crotation = glm::vec3(rotation[i * 3], rotation[i * 3 + 1], rotation[i * 3 + 2]);

		//first the rotation part;
		matrix[i] = glm::translate(coffset) *
			glm::rotate(crotation.z*(3.14159f) / 180.0f, glm::vec3(0, 0, 1))*
			glm::rotate(crotation.y*(3.14159f) / 180.0f, glm::vec3(0, 1, 0))*
			glm::rotate(crotation.x*(3.14159f) / 180.0f, glm::vec3(1, 0, 0));

		//then multiply by it's parent
		*/
		if (parent[i] != -1) {
			matrix[i] = matrix[parent[i]] * transform[i];
		}
		else {
			matrix[i] = transform[i];
		}
	}
}

void Armature::setMatrices() {
	setMatrices_simple();
	for (int i = 0; i < size; i++) {
		matrix[i] = matrix[i] * invBasePose[i];
	}
}

void Armature::print() {
	using namespace std;

	for (int i = 0; i < size; i++) {
		cout << "Bone: " << i << " {" << endl;
		cout << "Name: " << getJoint(i)->name << endl;
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

void Armature::addBone(std::string name, int parentIndex, const glm::mat4 &trans) {
	joint *newJoint = new joint;

	newJoint->name = name;
	newJoint->index = size;
	transform.push_back(trans);
	parent.push_back(parentIndex);
	size++;

	if (parentIndex != -1) {
		joint *parentJoint = getJoint(parentIndex);

		parentJoint->Child.push_back(newJoint);
		newJoint->Parent = parentJoint;

		invBasePose.push_back(glm::inverse(glm::inverse(invBasePose[parentIndex])*trans));
	}
	else {
		newJoint->Parent = NULL;
		root = newJoint;
		invBasePose.push_back(glm::inverse(trans));
	}
}

void Armature::addBone(std::string name, std::string parentName, const glm::mat4 &trans) {
	if (parentName == "") {
		addBone(name, -1, trans);
	}
	else {
		addBone(name, getJoint(parentName)->index, trans);
	}

}
