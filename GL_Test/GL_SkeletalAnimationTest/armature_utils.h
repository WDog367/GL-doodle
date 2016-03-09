#ifndef WDB_ARM_UTIL
#define WDB_ARM_UTIL

#include <vector>
#include <string>
#include "GL/glew.h"
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
	std::vector<GLfloat> loc;//[MAXBONES][3];
	std::vector<GLfloat> offset;// [MAXBONES][3];
	std::vector<GLfloat> rotation;// [MAXBONES][3];//stored in zyx euler?
	std::vector<GLfloat> parent;// [MAXBONES][3];//stored in zyx euler?
	std::vector<glm::mat4> matrix; //[MAXBONES]; //should swap this out for a quaternian in the future
	
	GLint size = 0;
	joint * root;


	//Animation related things
	std::vector<GLfloat*> channelMap;
	std::vector<std::vector<GLfloat>> frames;


	//programs and tools?
	void draw(const glm::mat4 &objMat);
	void print();

	void setMatrices_simple();
	void setMatrices();

	bool loadBVHFull(const char *fileName);
	bool loadBVHArm(const char *fileName);
	bool loadBVHAnim(const char *fileName);
	bool loadBVHAnim_quick(const char *fileName);

	joint * getJoint(std::string name);
	joint * getJoint(int index);


private:
	//mostly recursive helper functions
	void setMatrices(joint *);
	joint * getJoint(joint *root, std::string name);
	joint * getJoint(joint *root, int index);
};

#endif