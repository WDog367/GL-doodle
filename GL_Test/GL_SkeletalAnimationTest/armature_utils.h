#ifndef WDB_ARM_UTIL
#define WDB_ARM_UTIL

#include <vector>
#include <string>
#include <map>
#include "GL/glew.h"
#include "glm/glm.hpp"//includes most (if not all?) of GLM library
#include "glm/gtc/type_ptr.hpp"

#define MAXBONES 50//might use arrays at some point; not today, suckkaah
#define MAXCHANNEL 155//should be around MAXBONES*3+3 plus a few more for kicks, why not?

class Animation {
protected:
	Animation();

	std::vector<glm::vec3> rotation;
	std::vector<glm::vec3> offset;//probably won't end up storing this guy in the end
	std::vector<glm::mat4> transform;

	int curTime;

public:
	std::vector<int> channelMap;
	//Updates the status of the animation, based on how much time has passed
	virtual void tick(int dt) =0;
	glm::vec3* getRot() { return rotation.data(); }
	glm::vec3* getOff() { return offset.data(); }
	glm::mat4* getTrans() { return transform.data();};
	//might update to include bone-specific accessors
};


class Sampler {
public:
	//BST might be better than a vector?
	std::vector<GLfloat> keyTime;
	std::vector<glm::mat4> keyTransform;

	int frameNum;
	GLfloat length;

public:
	virtual glm::mat4 getMatrix(float t);
	virtual void addKey(const GLfloat &newTime, const glm::mat4 &newTransform);

	Sampler();
	Sampler(GLfloat* time, glm::mat4* transform, int frameNum);
	Sampler(GLfloat* time, glm::mat4* transform, int frameNum, GLfloat length);
	Sampler(std::vector<GLfloat> time, std::vector<glm::mat4> transform);
	Sampler(std::vector<GLfloat> time, std::vector<glm::mat4> transform, GLfloat length);
};

class Sampler_Slerp: public Sampler {
	std::vector<float> angle;
public:
	virtual glm::mat4 getMatrix(float t);
	virtual void addKey(const GLfloat &newTime, const glm::mat4 &newTransform);
};

class Animation_Sampler :public Animation {
public://everything public until I buuild a proper constructor
	std::vector<Sampler*> sampler;
	std::vector<std::string> channelName;

	int channelNum;
	float length;//might not need this? or might defer to this instead of the local one

public:
	virtual void tick(int dt);
	void addSampler(Sampler* sampler, std::string ChannelName);

	Animation_Sampler();
	Animation_Sampler(std::vector<Sampler*> sampler, std::vector<std::string> channelName);
	Animation_Sampler(std::vector<Sampler*> sampler, std::vector<std::string> channelName, float length);
	Animation_Sampler(Sampler** sampler, std::string* channelNames, int channelNum, float length);
	~Animation_Sampler();
};

//Simple keyframe animation
class Animation_Simple : public Animation{
	//better way to store this?
public:
	std::vector<GLfloat*> simpleChannelMap;
	std::vector<std::vector<GLfloat>> frames;

	Animation_Simple(const int mapSize, const std::string *channelMapType, const int *channelMapIndex, const std::vector<std::vector<GLfloat>> &frames, const int frameTime,
		const int size, const GLfloat rot[], const GLfloat loc[]);

	virtual void tick(int dt);
	int frameLength;

	int curFrame;
	int nextFrame;
};

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
class Armature {
public://everything public, because I'm a hack
	struct joint { 
		int index;
		std::string name;

		joint* Parent;
		std::vector<joint*> Child;
	};

	//bone related things, stored in array for quick access, easy transfer to shaders; could do vector, potentially, but meh
	//stored in stimultaneus arrays, rather than structs, to make it easier to pass into openGL shaders
	std::vector<GLfloat> offset;// [MAXBONES][3];
	std::vector<GLfloat> rotation;// [MAXBONES][3];//stored in zyx euler?//should swap this out for a quaternian in the future
	
	std::vector<glm::mat4> baseTrans;//transform of base pose
	std::vector<glm::mat4> transform;//current transformation within bonespace
	std::vector<GLint> parent;// [MAXBONES];
	
	std::vector<glm::mat4> invBasePose;//inverse of base pose in bind space; used in skinning
	std::vector<glm::mat4> matrix; //[MAXBONES]; 

	GLint size = 0;
	joint * root;

	Animation *anim;

	Armature();
	Armature(std::vector<glm::mat4> baseTrans, std::vector<GLint> parent, std::vector<std::string> name);

	void addBone(std::string name, int parentIndex, const glm::mat4 &trans);
	void addBone(std::string name, std::string parentName, const glm::mat4 &trans);

	void tick(int dt);

	void draw(const glm::mat4 &objMat);
	void print();

	void setMatrices_simple();
	void setMatrices();


	//move these outside of this class
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