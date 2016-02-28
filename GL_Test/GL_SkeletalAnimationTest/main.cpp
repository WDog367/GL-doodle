#include <iostream>
#include <fstream>
#include <sstream>//allows you to make streams from strings, saves code in .obj file reading
#include <string>
#include <vector>
#include "math.h"
#include "GL/glew.h"
#include "GL/freeglut.h"//may want to start using "GL/glut.h" to wean off of freeglut exclusive stuff?
#include "glm/glm.hpp"//includes most (if not all?) of GLM library
#include "glm/gtx/transform.hpp"
#include "glm/gtc/type_ptr.hpp"

///skeletal stuff

//custom headers
#include "file_utils.h"
#include "shader_utils.h"
#include "armature_utils.h"


//TODO
//Stop using globals (hahaha)

using namespace std;//mmmmmmmmmmmmmmmmmmmmmm
GLuint program;

GLuint vbo_coord;
GLuint vbo_colour;//will probably put these in an array, or some better storage system
GLuint ibo_elements;
GLuint uniform_matrix;
GLuint vao;//VAOs appear to be necessary in OpenGL 4.3, aww nuts

GLfloat X = 0.0, Y = -20.0, Z = -50;
std::string command = "";

glm::mat4 mvp;
int curFrame = 0;
int curAnim = 0;
struct Arm armature;

string FUCKYOUCPP[] = { "02_01.bvh", "02_02.bvh" , "02_03.bvh" , "02_04.bvh" , "02_05.bvh" , "02_06.bvh" , "02_07.bvh" , "02_08.bvh","02_09.bvh" };
unsigned int timeLast, timeCurr;

bool init() {
	//Setting up the render program (frag/vert shaders
	struct shaderInfo shaders[] =	{ { GL_VERTEX_SHADER, "vs.glsl" },
									{ GL_FRAGMENT_SHADER, "fs.glsl" } };

	program = LoadProgram(shaders, 2);

	//Setting up the buffers storing data for vertex attribs
	vector <GLfloat> vertices;
	vector <GLfloat> colours;
	vector <GLuint> elements;
	/*
	vertices = {
		0.0f,  0.8f, 0.0f,
		-0.8f, -0.8f, 0.0f,
		0.8f, -0.8f, 0.0f,
		0.0f, 0.0f, -2.0f
	};

	elements = {
		0, 1, 2,
		0, 1, 3,
		1, 2, 3,
		2, 0, 3
	};

	*/
	loadObj(vertices, elements, "object");

	colours = {
		0.0f, 0.0f, 1.0f,
		0.0f, 1.0f, 0.0f,
		1.0f, 0.0f, 0.0f,
		1.0f, 1.0f, 0.0f
	};
	glGenBuffers(1, &vbo_coord);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_coord);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices[0])*vertices.size(), vertices.data(), GL_STATIC_DRAW);


	glGenBuffers(1, &vbo_colour);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_colour);
	glBufferData(GL_ARRAY_BUFFER, sizeof(colours[0])*colours.size(), colours.data(), GL_STATIC_DRAW);
	

	glGenBuffers(1, &ibo_elements);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_elements);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(elements[0])*elements.size(), elements.data(), GL_STATIC_DRAW);

	//setting up the vertex attribute/arrays
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	glBindBuffer(GL_ARRAY_BUFFER, vbo_coord);
	GLint attrib_vcoord = getAttrib(program, "coord");
	glVertexAttribPointer(
		attrib_vcoord,	//attrib 'index'
		4,				//pieces of data per index
		GL_FLOAT,		//type of data
		GL_FALSE,		//whether to normalize
		0,				//space b\w data
		0				//offset from buffer start
		);
	glEnableVertexAttribArray(attrib_vcoord);

	glBindBuffer(GL_ARRAY_BUFFER, vbo_colour);
	GLint attrib_vcolour = getAttrib(program, "colour");
	glVertexAttribPointer(
		attrib_vcolour,	//attrib 'index'
		3,				//pieces of data per index
		GL_FLOAT,		//type of data
		GL_FALSE,		//whether to normalize
		0,				//space b\w data
		0				//offset from buffer start
		);
	glEnableVertexAttribArray(attrib_vcolour);

	uniform_matrix = getUniform(program, "matrix");

	//setting the program to use
	glUseProgram(program);
	

	loadBVH(armature, "walk.bvh");
	printArm(armature.root, 0);

	timeLast = glutGet(GLUT_ELAPSED_TIME);
	return true;
}

void idle() {
	//updating the matrics (i.e. animation) done in the idle function

	timeLast = timeCurr;
	timeCurr = glutGet(GLUT_ELAPSED_TIME);

	if (timeLast - timeCurr > 500) {
		curFrame++;
		if (curFrame >= armature.frames.size()) {
			curFrame = 0;

			curAnim++;
			if (curAnim > 8) curAnim = 0;

			nukeArm(armature);
			loadBVH(armature, FUCKYOUCPP[curAnim].c_str());
		}

		//posing the skelly
		for (int i = 0; i < armature.channelMap.size(); i++) {
			*(armature.channelMap[i]) = armature.frames[curFrame][i];
		}
	}
	//float angle = (timeCurr - timeLast) / 1000.0*(3.14159265358979323846) / 4;
	
	//model matrix: transforms local model coordinates into global world coordinates
	glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(X, Y, Z));
	//	*glm::rotate(angle, glm::vec3(0.0, 1.0, 0.0));

	//view matrix: transforms world coordinates into relative-the-camera view-space coordinates
	glm::mat4 view = glm::lookAt(glm::vec3(0.0, 0.0, 0.0), glm::vec3(0.0, 0.0, -4.0), glm::vec3(0.0, 1.0, 0.0));//I don't necessarily like this

	//projection matrix: transforms view space coordinates into 2d screen coordinates
	glm::mat4 projection = glm::perspective(45.0f, 1.0f * 600 / 480, 0.1f, 1000.0f);

	mvp = projection*view*model;//combination of model, view, projection matrices

	glUniformMatrix4fv(uniform_matrix, 1, GL_FALSE, glm::value_ptr(mvp));

	cout << "command string: " << command << endl;

	if (command.find("a") != -1) {
		X -= 50.0*(timeCurr - timeLast) / 1000;
	}
	else if (command.find("d") != -1) {
		X += 50.0*(timeCurr - timeLast) / 1000;
	}else if (command.find("w") != -1) {
		Y -= 50.0*(timeCurr - timeLast) / 1000;
	}
	else if (command.find("s") != -1) {
		Y += 50.0*(timeCurr - timeLast) / 1000;
	}
	else if (command.find("r") != -1) {
		Z -= 50.0*(timeCurr - timeLast) / 1000;
	}
	else if (command.find("f") != -1) {
		Z += 50.0*(timeCurr - timeLast) / 1000;
	}

	glutPostRedisplay();
}

void display() {
	glClearColor(1.0, 1.0, 1.0, 1.0);
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
	
	glBindVertexArray(vao);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_elements);//not sure if 'glbindVertexArray' will include this guy
	
	glDrawElements(GL_TRIANGLES, 12, GL_UNSIGNED_INT, NULL);

	drawArm(armature, mvp);
	glFlush();
}

void free_res() {
	glDeleteProgram(program);
	glDeleteBuffers(1, &vbo_coord);
	glDeleteBuffers(1, &vbo_colour);
}

void handleKeyDown(unsigned char key, int x, int y) {
	if (command.find((char)key) == -1) {
		command.push_back(key);
		cout << "key down: " << key << " command string: " << command << endl;
	}

}
void handleKeyUp(unsigned char key, int x, int y) {
	command.erase(command.find(key), 1);
	cout << "key up: " << key << " command string: " << command << endl;
}

int main(int argc, char *argv[]) {
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGBA);
	glutInitWindowSize(600, 480);
	glutInitContextVersion(4, 3);
	glutInitContextProfile(GLUT_CORE_PROFILE);
	glutCreateWindow("Test Display");

	glewExperimental = true;
	if (glewInit()) {
		cerr << "Unable to initialize GLEW\n";
	}

	cout << glGetString(GL_VERSION) << endl;

	init();

	glEnable(GL_DEPTH_TEST);
	glutDisplayFunc(display);
	glutKeyboardFunc(handleKeyDown);
	glutKeyboardUpFunc(handleKeyUp);
	glutIdleFunc(idle);

	glutMainLoop();
	glutPostRedisplay();
	free_res();
}