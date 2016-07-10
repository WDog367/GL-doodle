#ifndef WDB_DAE_UTIL
#define WDB_DAE_UTIL
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


void loadDAE(Mesh* mesh, const char *filename);
Animation_Sampler * loadDAEAnim(const char *filename);
Skeletal_Mesh* loadDAESkelMesh(const char *filename, GLuint program);

#endif