#ifndef WDB_ARM_UTIL
#define WDB_ARM_UTIL

#include <vector>
#include <string>
#include "glm/glm.hpp"//includes most (if not all?) of GLM library

//might make this a proper class
enum { x, y, z };
struct joint { //storing skeletons as linked lists, maybe will do different later
	float offset[3];
	float rotation[3];
	
	std::string name;

	joint* Parent;
	std::vector<joint*> Child;
};

struct Arm {
	joint *root;
	std::vector<float*> channelMap;
	std::vector<std::vector<float>> frames;
};

void loadBVH(Arm &arm, const char *fileName);
void drawArm(Arm &arm, const glm::mat4 &objMat);
void printArm(joint *arm, int);
void nukeArm(Arm &arm);
#endif