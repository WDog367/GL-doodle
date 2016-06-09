#include <vector>
#include <string>
#include <sstream>
#include <iostream>

#include "GL/glew.h"
#include "glm/glm.hpp"
#include "glm/gtx/transform.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "math.h"

#include "armature_utils.h"
#include "mesh_utils.h"

#include "tinyxml2.h"

//For debugging
void printmat4(glm::mat4 &a) {

	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			std::cout << a[j][i] << ", ";
		}
		std::cout << std::endl;
	}
}
void printvec4(glm::vec4 &a) {

	for (int i = 0; i < 4; i++) {
		std::cout << a[i] << ", ";
	}
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~Useful XML utilities~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
/** returns the first node with the name (type) passed to it, that is a child of the node passed, with a depth first search*/
tinyxml2::XMLElement* XMLSearchName(tinyxml2::XMLElement *node, const char* id) {
	using namespace tinyxml2;
	XMLElement *curNode = node;
	XMLElement *result = NULL;
	while (curNode) {
		if (strcmp(curNode->Name(), id) == 0) return curNode;
		result = XMLSearchName(curNode->FirstChildElement(), id);
		if (result != NULL) return result;

		curNode = curNode->NextSiblingElement();
	}
	return NULL;
}

/** returns the first node with the name (type) passed to it in the document, with a depth first search*/
tinyxml2::XMLElement* XMLSearchDocName(tinyxml2::XMLDocument &doc, const char* id) {
	using namespace tinyxml2;
	return XMLSearchName(doc.FirstChildElement(), id);
}

/** returns the first node with the id passed to it, that is a child of the node passed, with a depth first search
	Nodes without an ID attribute are passed over */
tinyxml2::XMLElement* XMLSearchID(tinyxml2::XMLElement *node, const char* id) {
	using namespace tinyxml2;
	XMLElement *curNode = node;
	XMLElement *result = NULL;
	while (curNode) {
		if (curNode->Attribute("id", id)) return curNode;
		result = XMLSearchID(curNode->FirstChildElement(), id);
		if (result != NULL) return result;

		curNode = curNode->NextSiblingElement();
	}
	return NULL;
}

/** returns the first node with the id passed to it in the document, with a depth first search
	Nodes without an ID attribute are passed over*/
tinyxml2::XMLElement* XMLSearchDocID(tinyxml2::XMLDocument &doc, const char* id) {
	using namespace tinyxml2;
	return XMLSearchID(doc.FirstChildElement(), id);
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~Reading DAE Files~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
struct DAESource {
	std::string id;
	std::vector<std::string> data;//will need to be cast into real data
	int size;

	//accsessor information
	int count;
	int stride;
	std::vector<std::string> name;
	std::vector<std::string> type;
};


void DAEReadSource(DAESource &out, tinyxml2::XMLElement *node) {
	assert(strcmp((node->Name()), "source") == 0);
	using namespace tinyxml2;
	using namespace std;
	XMLElement *curNode = NULL;
	std::istringstream sdata;
	int count;

	out.id = std::string(node->Attribute("id"));

	curNode = node->FirstChildElement("float_array");
	if (!curNode) curNode = node->FirstChildElement("int_array");
	if (!curNode) curNode = node->FirstChildElement("bool_array");
	if (!curNode) curNode = node->FirstChildElement("Name_array");
	if (!curNode) curNode = node->FirstChildElement("IDREF_array");

	out.size = std::stoi(curNode->Attribute("count"));
	sdata = std::istringstream(curNode->GetText());

	out.data.resize(out.size);
	for (int i = 0; i < out.size; i++) {
		sdata >> out.data[i];
	}

	curNode = node->FirstChildElement("technique_common")->FirstChildElement("accessor");
	if (!curNode) curNode = node->FirstChildElement("accessor");

	out.count = std::stoi(curNode->Attribute("count"));
	out.stride = std::stoi(curNode->Attribute("stride"));
	curNode = curNode->FirstChildElement("param");
	while (curNode) {
		out.name.push_back(curNode->Attribute("name"));
		out.type.push_back(curNode->Attribute("type"));
		curNode = curNode->NextSiblingElement("param");
	}
}

void DAELoadGeometry(std::vector<GLfloat> &vertices, std::vector<GLuint> &elements, std::vector<GLfloat> &normals, tinyxml2::XMLElement *geometry) {
	using namespace std;
	using namespace tinyxml2;

	XMLElement *curNode;
	XMLElement *input;
	XMLElement *polyList;
	std::vector<DAESource> sources;//might be better as a map
	DAESource source;

	DAESource *posSource = NULL, *normSource = NULL;
	std::string search;
	int posOffset, normOffset, stride;
	int polyCount;
	std::istringstream polyData;

	if (geometry == NULL) {
		std::cerr << "Node type: " << "NULL" << " passed to LoadGeometry" << std::endl;
		return;
	}else if(std::strcmp(geometry->Name(), "geometry") != 0) {
		std::cerr << "Node type: " << geometry->Name() << " passed to LoadGeometry" << std::endl;
		return;
	}

	polyList = geometry->FirstChildElement("mesh")->FirstChildElement("polylist");
	polyCount = stoi(polyList->Attribute("count"));
	input = polyList->FirstChildElement("input");//should look for "vertex" node to get per-vertex properties
												//then look for either polyList, or triangle

	stride = 0;
	while (input != NULL) {
		search = string(input->Attribute("source")).substr(1);
		curNode = XMLSearchID(geometry, search.c_str());//this will be a <vertices> node

		if (input->Attribute("semantic", "VERTEX")) {
			posOffset = input->IntAttribute("offset");

			//the source given in the input leads to another input, not directly to a source
			search = string(curNode->FirstChildElement("input")->Attribute("source")).substr(1);
			curNode = XMLSearchID(geometry, search.c_str());

			DAEReadSource(source, curNode);

			vertices.resize(source.data.size());
			for (int i = 0; i < vertices.size(); i++) {
				vertices[i] = stof(source.data[i].c_str());
			}
		}
		else if (input->Attribute("semantic", "NORMAL")) {
			normOffset = input->IntAttribute("offset");

			DAEReadSource(source, curNode);

			normals.resize(source.data.size());
			for (int i = 0; i < normals.size(); i++) {
				normals[i] = stof(source.data[i].c_str());
			}
		}

		stride++;
		input = input->NextSiblingElement("input");
	}
	int dataToken;

	polyData = istringstream(polyList->FirstChildElement("p")->GetText());
	for (int i = 0; i < polyCount*stride * 3; i += stride) {
		for (int j = 0; j < stride; j++) {
			polyData >> dataToken;
			if (j == posOffset) {
				elements.push_back(dataToken);
			}
		}
	}
}

//could do this with just arrays, don't need to build the linked joint structure
void DAELoadArm(std::vector<Armature::joint*> &sibling, std::vector<glm::mat4> &transform, int &index, tinyxml2::XMLElement *daeRoot, Armature::joint *root = NULL) {
	Armature::joint *curJoint;
	std::vector<Armature::joint*> joints;
	glm::mat4 curTrans;
	std::istringstream data;
	tinyxml2::XMLElement *curNode = daeRoot;

	sibling.resize(0);
	while (curNode) {
		data = std::istringstream(curNode->FirstChildElement("matrix")->GetText());
		for (int i = 0; i < 4; i++) {
			for (int j = 0; j < 4; j++) {
				data >> curTrans[j][i];
			}
		}
		transform.push_back(curTrans);

		curJoint = new Armature::joint;
		curJoint->name = curNode->Attribute("id");
		curJoint->index = index;
		curJoint->Parent = root;
		DAELoadArm(curJoint->Child, transform, ++index, curNode->FirstChildElement("node"), curJoint);

		sibling.push_back(curJoint);
		curNode = curNode->NextSiblingElement("node");
	}
}

void DAELoadArm(std::vector<glm::mat4> &transform, std::vector<GLint> &parent, std::vector<std::string> &name, int &index, tinyxml2::XMLElement *daeRoot, int curParent = -1) {
	glm::mat4 curTrans;
	std::string curName;
	GLint curIndex;
	std::istringstream data;
	tinyxml2::XMLElement *curJoint = daeRoot;
	
	while (curJoint) {
		curName = curJoint->Attribute("id");
		
		//load the transformation matrix
		data = std::istringstream(curJoint->FirstChildElement("matrix")->GetText());
		for (int i = 0; i < 4; i++) {
			for (int j = 0; j < 4; j++) {
				data >> curTrans[j][i];
			}
		}

		transform.push_back(curTrans);
		parent.push_back(curParent);
		name.push_back(curName);

		index += 1;

		DAELoadArm(transform, parent, name, index, curJoint->FirstChildElement("node"), index -1);

		curJoint = curJoint->NextSiblingElement("node");
	}
}

void DAELoadAnim(std::vector<GLfloat> &time, std::vector<glm::mat4> &trans, std::string &name, tinyxml2::XMLElement *animation) {
	using namespace tinyxml2;
	XMLElement *input;
	XMLElement *curNode;
	std::string search;
	DAESource source;

	//check that animation is in fact an animation node
	if (std::strcmp(animation->Name(), "animation") != 0) {
		std::cerr << "Node type: " << animation->Name() << " passed to LoadAnim" << std::endl;
		return;
	}

	input = animation->FirstChildElement("sampler")->FirstChildElement("input");
	time.resize(0);
	trans.resize(0);
	while (input) {

		search = std::string(input->Attribute("source")).substr(1);
		curNode = XMLSearchID(animation, search.c_str());
		DAEReadSource(source, curNode);

		if (input->Attribute("semantic", "INPUT")) {
			for (int i = 0; i < source.size; i++) {
				time.push_back(std::stof(source.data[i]));
			}
		}
		else if (input->Attribute("semantic", "OUTPUT")) {
			for (int i = 0; i < source.size; i += 16) {
				glm::mat4 curTrans;
				for (int j = 0; j < 4; j++) {
					for (int k = 0; k < 4; k++) {
						curTrans[k][j] = std::stof(source.data[i + j * 4 + k]);
					}
				}
				trans.push_back(curTrans);
			}
		}
		input = input->NextSiblingElement("input");
	}

	search = std::string(animation->FirstChildElement("channel")->Attribute("target"));
	search = search.substr(0, search.find('/'));
	name = search;
}

void DAELoadAnimLibrary(std::vector<std::vector<GLfloat>> &times, std::vector<std::vector<glm::mat4>> &transforms, std::vector<std::string> names, tinyxml2::XMLElement *library) {
	using namespace tinyxml2;
	XMLElement *animation;
	XMLElement *input;
	XMLElement *curNode;

	std::vector<GLfloat> time;
	std::vector<glm::mat4> trans;
	std::vector<Sampler*> sampler;
	std::string name;

	std::string search;
	DAESource source;

//	doc.LoadFile(filename);
//	library = doc.RootElement()->FirstChildElement("library_animations");
	animation = library->FirstChildElement("animation");

	while (animation) {
		DAELoadAnim(time, trans, name, animation);

		times.push_back(time);
		transforms.push_back(trans);
		names.push_back(name);
		
		animation = animation->NextSiblingElement("animation");
	}
}

// loads skel mesh, and necessary information from the controller.
void DAELoadSkeletalMeshController(std::vector<std::string> &jointNames,
												std::vector<glm::mat4> &jointInvMatrices,
												std::vector<GLuint> &bIndex,
												std::vector<GLfloat> &bWeight,
												std::vector<GLuint> &bNum,
												glm::mat4 &bindSpaceMatrix, 
												tinyxml2::XMLElement *controller) {
	using namespace tinyxml2;
	using namespace std;
	if (controller == NULL) {
		std::cerr << "NULL ptr " << "expecting " << "controller" << std::endl;
	}

	if (std::strcmp(controller->Name(), "controller") != 0){
		std::cerr << "Node type: " << controller->Name() << " passed, expecting " << "controller" << std::endl;
		return;
	}

	XMLElement *joints;
	XMLElement *input;
	XMLElement *curNode;

	DAESource source;
	std::string search;

	std::vector<GLfloat> jointWeights;

	std::istringstream vcountData;
	std::istringstream vData;
	std::istringstream bindSpaceMatrixData;

	//moving the controller to point to it's <skin> child
	//this actually contains all of the data, so it let's me type a bit less
	controller = controller->FirstChildElement("skin");
	if (controller == NULL) {
		std::cerr << "controller has no skin node" << std::endl;
		return;
	}

	//First load the info stored in the <joints> node
	//will be: the inv bind pose matrices, and the names of the joints used
	joints = controller->FirstChildElement("joints");
	input = joints->FirstChildElement("input");
	while (input) {
		//read the data from wherever the input points to into source
		search = std::string(input->Attribute("source")).substr(1);
		curNode = XMLSearchID(controller->FirstChildElement(), search.c_str());
		DAEReadSource(source, curNode);

		//the joint names
		if (input->Attribute("semantic", "JOINT")) {
			jointNames.resize(source.data.size());
			for (int i = 0; i < jointNames.size(); i++) {
				jointNames[i] = source.data[i];
			}
		}
		//the pose matrices
		else if (input->Attribute("semantic", "INV_BIND_MATRIX")) {
			jointInvMatrices.resize(source.count);
			for (int i = 0; i < source.size; i += 16) {
				for (int j = 0; j < 4; j++) {
					for (int k = 0; k < 4; k++) {
						jointInvMatrices[i / 16][k][j] = std::stof(source.data[i + j * 4 + k]);
					}
				}
			}
		}
		input = input->NextSiblingElement("input");
	}

	int vertCount;//the number of vertices
	int joff; //the offset in <v> of the joint index
	int weightoff; //the offset in <v> of the weight offset
	int stride; // the number of parameters for each vertex in <v>
				//even if we don't read them all, we need to know the number to transverse <v> properly

	//Now load the info stored in the <vertex_weights> node
	//will be: the number of bones per vert, the bone indices, and the bone weights
	curNode = controller->FirstChildElement("vertex_weights");
	input = curNode->FirstChildElement("input");
	stride = 0;
	while (input) {
		stride++;//stride keeps track of the number of parameters loaded

		//first, read whatever's inside the soruce that our input points to
		search = std::string(input->Attribute("source")).substr(1);
		curNode = XMLSearchID(controller->FirstChildElement(), search.c_str());//fetching the node that this input points to
		DAEReadSource(source, curNode);

		//the names of the joints, but they are already loaded into jointNames
		if (input->Attribute("semantic", "JOINT")) {
			joff = std::stoi(input->Attribute("offset"));

		}
		//the weights of the joints
		else if (input->Attribute("semantic", "WEIGHT")) {
			weightoff = std::stoi(input->Attribute("offset"));
			jointWeights.resize(source.data.size());
			for (int i = 0; i < jointWeights.size(); i++) {
				jointWeights[i] = stof(source.data[i]);
			}
		}
		input = input->NextSiblingElement("input");
	}

	//the data in <vertex_weights> references those inputs, and is stored in <v> and <vcount> arrays
	//preparing to read the indexed data from <vcount> and <v>
	vertCount = std::stoi(controller->FirstChildElement("vertex_weights")->Attribute("count"));
	vcountData = istringstream(controller->FirstChildElement("vertex_weights")->FirstChildElement("vcount")->GetText());
	vData = istringstream(controller->FirstChildElement("vertex_weights")->FirstChildElement("v")->GetText());


	bNum.resize(vertCount);//we know how many verts, but the size of bWeight and bIndex depend on the values in bNum
	bWeight.resize(0);
	bIndex.resize(0);

	int token;
	for (int i = 0; i < vertCount; i++) {
		//pulling the number of bones for this vertex
		vcountData >> bNum[i];
		
		for (int j = 0; j < bNum[i]; j++) {//for each bone tied to the vertex
			for (int k = 0; k < stride; k++) {//for each property stored per bone
				//token is used to pull the next number from the string stream
				vData >> token;

				if (k == joff) {
					bIndex.push_back(token);
				}
				else if (k == weightoff) {
					bWeight.push_back(jointWeights[token]);
				}
			}
		}
	}

	//Final bit of data
	//the bind space matrix for the controller
	bindSpaceMatrixData = istringstream(controller->FirstChildElement("bind_shape_matrix")->GetText());
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			bindSpaceMatrixData >> bindSpaceMatrix[j][i];
		}
	}
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~The unclean~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void loadDAE(std::vector<GLfloat> &vertices, std::vector<GLuint> &elements, std::vector<GLfloat> &normals, const char *filename) {
	using namespace std;
	using namespace tinyxml2;

	XMLDocument doc;
	XMLElement *geometry;

	doc.LoadFile(filename);
	geometry = doc.FirstChildElement()->FirstChildElement("library_geometries")->FirstChildElement("geometry");
	DAELoadGeometry(vertices, elements, normals, geometry);
}


Armature* loadDAEArm(tinyxml2::XMLElement *daeRoot) {
	std::vector<glm::mat4> trans;
	std::vector<GLint> parent;
	std::vector<std::string> name;
	int size = 0;

	DAELoadArm(trans, parent, name, size, daeRoot);

	return new Armature(trans, parent, name);

}

Animation_Sampler * loadDAEAnim(const char *filename) {
	using namespace tinyxml2;
	XMLDocument doc;
	XMLElement *animation;
	XMLElement *library;
	XMLElement *input;
	XMLElement *curNode;


	std::vector<Sampler*> sampler;

	std::vector<std::vector<glm::mat4>> trans;
	std::vector<std::vector<GLfloat>> time;
	std::vector <std::string> names;


	std::string search;
	DAESource source;

	doc.LoadFile(filename);
	library = doc.RootElement()->FirstChildElement("library_animations");

	DAELoadAnimLibrary(time, trans, names, library);
	
	//maybe check that name, time, trans all have same size

	for (int i = 0; i < time.size(); i++) {
		sampler.push_back(new Sampler(time[i], trans[i]));
	}

	Animation_Sampler *anim = new Animation_Sampler(sampler, names);
	anim->channelMap.resize(anim->channelNum);

	//setting up the "channel map" should be done elsewhere
	for (int i = 0; i < anim->channelNum; i++) {
		anim->channelMap[i] = i;
	}

	return anim;
}

Skeletal_Mesh* loadDAESkelMesh(const char *filename, GLuint program) {
	using namespace tinyxml2;
	using namespace std;

	Animation_Sampler *anim;
	Armature *arm;
	XMLDocument doc;
	XMLElement *library;
	XMLElement *controller;
	XMLElement *asset;
	XMLElement *root;

	XMLElement * instance;

	DAESource source;
	std::string search;
	std::vector<std::string> jointNames;
	std::vector<glm::mat4> jointInvMatrices;

	std::vector<GLfloat> vertices;
	std::vector<GLuint> elements;
	std::vector<GLfloat> normals;

	std::vector<GLuint> bIndex;
	std::vector<GLfloat> bWeight;
	std::vector<GLuint> bNum;

	glm::mat4 bindSpaceMatrix;

	bool zUp;

	Skeletal_Mesh* mesh;

	//the meat of it
	doc.LoadFile(filename);

	//getting general information about the file
	asset = doc.FirstChildElement()->FirstChildElement("asset");
	zUp = strcmp("Z_UP", asset->FirstChildElement("up_axis")->GetText()) == 0;


	//Finding the controller; it holds the information about the skeletal mehs
	library = doc.FirstChildElement()->FirstChildElement("library_controllers");
	controller = library->FirstChildElement("controller");


	//Finding the controller; it holds the information about the skeletal mesh
	instance = XMLSearchDocName(doc, "instance_controller");
	search = std::string(instance->FirstChildElement("skeleton")->GetText());
	root = XMLSearchDocID(doc, search.substr(1).c_str());

	arm = loadDAEArm(root);
	anim = loadDAEAnim(filename);
	arm->anim = anim;

	//load the mesh
	loadDAE(vertices, elements, normals, filename);

	//load the controller information

	std::vector<GLfloat> weights;
	std::vector<GLuint> indices;
	std::vector<GLuint> num;
	DAELoadSkeletalMeshController(jointNames, jointInvMatrices, indices, weights, num, bindSpaceMatrix,	controller);

	bWeight.resize(num.size() * Skeletal_Mesh::maxBNum);
	bIndex.resize(num.size() * Skeletal_Mesh::maxBNum);
	bNum.resize(num.size());

	int k = 0;
	for (int i = 0; i < bNum.size(); i++) {
		bNum[i] = num[i];
		for (int j = 0; j < bNum[i]; j++) {
			bWeight[i*Skeletal_Mesh::maxBNum + j] = weights[k];
			bIndex[i*Skeletal_Mesh::maxBNum + j] = arm->getJoint(jointNames[indices[k]])->index;
			k++;
		}
	}

	//assemble it all
	mesh = new Skeletal_Mesh(vertices.data(), vertices.size(), elements.data(), elements.size(), bIndex.data(), bWeight.data(), bNum.data(), bindSpaceMatrix, arm, program);

	return mesh;

}