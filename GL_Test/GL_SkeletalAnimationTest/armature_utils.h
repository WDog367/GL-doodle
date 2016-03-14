#ifndef WDB_ARM_UTIL
#define WDB_ARM_UTIL

#include <vector>
#include <string>
#include "GL/glew.h"
#include "glm/glm.hpp"//includes most (if not all?) of GLM library

#define MAXBONES 50//might use arrays at some point; not today, suckkaah
#define MAXCHANNEL 155//should be around MAXBONES*3+3 plus a few more for kicks, why not?

class Interpolator {
	//will be used in various thing so that I don't need different classes for different types of interpolation

	virtual float Interpolate(float v1, float v2, int t) = 0;
};

//function that returns what the
class Animation {
protected:
	Animation();

	std::vector<GLfloat> rotation;
	std::vector<GLfloat> offset;//probably won't end up storing this guy in the end

	int curTime;

public:
	//Updates the status of the animation, based on how much time has passed
	virtual void tick(int dt) =0;
	GLfloat* getRot() { return rotation.data(); }
	GLfloat* getOff() { return offset.data(); }
	//might update to include bone-specific accessors
};

//Simple keyframe animation
class Animation_Simple : public Animation{
	//better way to store this?
public:
	std::vector<GLfloat*> channelMap;
	std::vector<std::vector<GLfloat>> frames;

	Animation_Simple(const int mapSize, const std::string *channelMapType, const int *channelMapIndex, const std::vector<std::vector<GLfloat>> &frames, const int frameTime,
		const int size, const GLfloat rot[], const GLfloat loc[]);

	virtual void tick(int dt);
	int frameLength;

	int curFrame;
	int nextFrame;
};

//rotation is interpolated (linearly) between current frame and next frame
class Animation_Interpolating {
	struct Frame {
		std::vector<GLfloat> pose;
		int length;
	};

	int lastTime;

	int curFrame;
	int nextFrame;
};

class Armature {
public://everything public, because I'm a hack
	struct joint { 
		//these are a very bad idea while everything is stored in vectors;
		//the vector could get resized, and then these will be pointing to the wrong spot
		//float* offset;//point to the spot in the offset array
		//float* rotation;//ditto to the rotation array
		//glm::mat4* matrix;

		//this should be used more than the pointers; I may get rid of the pointers
		int index;
		std::string name;

		joint* Parent;
		std::vector<joint*> Child;
	};

	//bone related things, stored in array for quick access, easy transfer to shaders; could do vector, potentially, but meh
	std::vector<GLfloat> loc;//[MAXBONES][3];
	//std::vector<GLfloat> base;
	std::vector<GLfloat> offset;// [MAXBONES][3];
	std::vector<GLfloat> rotation;// [MAXBONES][3];//stored in zyx euler?
	std::vector<GLfloat> parent;// [MAXBONES][3];//stored in zyx euler?
	std::vector<glm::mat4> matrix; //[MAXBONES]; //should swap this out for a quaternian in the future
	
	std::vector <GLfloat> matrices;//[MAXBONES][16]confusing, I know; but this is the data passable to openGL; the glm::mat4 stuff isn't

	GLint size = 0;
	joint * root;

	//Animation related things
	//std::vector<GLfloat*> channelMap;
	//std::vector<std::vector<GLfloat>> frames;
	Animation *anim;

	//programs and tools?
	void tick(int dt);

	void draw(const glm::mat4 &objMat);
	void print();

	void setMatrices_simple();
	void setMatrices();

	bool loadBVHFull(const char *fileName);
	bool loadBVHArm(const char *fileName);
	bool loadBVHAnim(const char *fileName);

	joint * getJoint(std::string name);
	joint * getJoint(int index);


private:
	//mostly recursive helper functions
	joint * getJoint(joint *root, std::string name);
	joint * getJoint(joint *root, int index);
};

#endif