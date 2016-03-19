#include <iostream>
#include <fstream>
#include <sstream>//allows you to make streams from strings, saves code in .obj file reading
#include <string>
#include <vector>
#include "math.h"
#include "GL/glew.h"
#include "glm/glm.hpp"//includes most (if not all?) of GLM library
#include "glm/gtx/transform.hpp"
#include "glm/gtc/type_ptr.hpp"

//#include "GL/freeglut.h"//gonna stop using GLUT
#include "SDL.h"
#include "SDL_keyboard.h"
#include "SDL_keycode.h"
///skeletal stuff

//custom headers
#include "file_utils.h"
#include "shader_utils.h"
#include "armature_utils.h"
#include "mesh_utils.h"
//TODO
//Stop using globals (hahaha)
using namespace std;//mmmmmmmmmmmmmmmmmmmmmm
GLfloat X = 0.0, Y = -20.0, Z = -50, A = 0;
std::string command = "";

glm::mat4 mvp;
int curFrame = 0;
int curAnim = 0;
Armature armature;
int c1, c2;
string fileName;
int curGroup =2 , curFile = 1;

Skeletal_Mesh_simple *skelly;
Skeletal_Mesh *skeletor;

Mesh *testMesh;

GLuint program;
GLuint skel_program;

//const int maxfile = 124;
bool repeat = false;

unsigned int timeLast, timeCurr;

void genMesh(std::vector<GLfloat> &vertices, std::vector<GLuint> &elements, Armature * armature) {
	vertices.resize(0);
	elements.resize(0);

	for (int i = 0; i < armature->size; i++) {
		GLfloat baseVertices[] = {
			// front
			-1.0, -1.0,  1.0,
			1.0, -1.0,  1.0,
			1.0,  1.0,  1.0,
			-1.0,  1.0,  1.0,
			// back
			-1.0, -1.0, -1.0,
			1.0, -1.0, -1.0,
			1.0,  1.0, -1.0,
			-1.0,  1.0, -1.0,
		};
		GLuint baseElements[] = {
			// front
			0, 1, 2,
			2, 3, 0,
			// top
			1, 5, 6,
			6, 2, 1,
			// back
			7, 6, 5,
			5, 4, 7,
			// bottom
			4, 0, 3,
			3, 7, 4,
			// left
			4, 5, 1,
			1, 0, 4,
			// right
			3, 2, 6,
			6, 7, 3,
		};

		//glm::mat4 transform = glm::translate(glm::vec3(armature->loc[i * 3], armature->loc[i * 3 + 1], armature->loc[i * 3 + 2]));
		glm::mat4 transform = armature->matrix[i];

		glm::vec4 temp = glm::vec4(0, 0, 0, 1);

		temp = transform*temp;
		cout << temp.x << ", " << temp.y << ", " << temp.z << ", " << "; ";
		cout << armature->loc[i*3 +0] << ", " << armature->loc[i * 3 + 1] << ", " << armature->loc[i * 3 +2] << ", " << endl;
		int size = sizeof(baseVertices) / sizeof(baseVertices[0]);

		for (int i = 0; i < size; i += 3) {
			glm::vec4 tempVec = transform*glm::vec4(baseVertices[i], baseVertices[i + 1], baseVertices[i + 2], 1);
			vertices.push_back(tempVec.x);
			vertices.push_back(tempVec.y);
			vertices.push_back(tempVec.z);
		}

		for (int j = 0; j < sizeof(baseElements) / sizeof(baseElements[0]); j++) {
			elements.push_back(baseElements[j] + i* size / 3);
		}
	}
}

bool init() {
	//fileName = new char[9];
	fileName = "01_01.bvh";

	cout << "loading file" << endl;
	armature.loadBVHFull("walk.bvh");

	vector<GLfloat> vertices;
	vector<GLuint> elements;

	//loadObj(vertices, elements, "object");
	armature.setMatrices_simple();
	genMesh(vertices, elements, &armature);
	struct shaderInfo shaders[] = { { GL_VERTEX_SHADER, "vs.glsl" },
	{ GL_FRAGMENT_SHADER, "fs.glsl" } };

	program = LoadProgram(shaders, 2);

	struct shaderInfo skel_shaders[] = { { GL_VERTEX_SHADER, "skelmesh_vs.glsl" },
	{ GL_FRAGMENT_SHADER, "skelmesh_fs.glsl" } };

	skel_program = LoadProgram(skel_shaders, 2);
	//glUseProgram(program);

	skelly = new Skeletal_Mesh_simple(&armature, program);
	skeletor = new Skeletal_Mesh(vertices.data(), vertices.size(), elements.data(), elements.size(), &armature, skel_program);
	testMesh = new Mesh(vertices.data(), vertices.size(), elements.data(), elements.size(), program);

	timeLast = SDL_GetTicks();
	return true;
}


char * ntos(int num) {
	int size = 0;
	int digit = 1;
	char *result;

	while (num / digit > 0) {
		size++;
		digit = digit * 10;
	}

	if (size == 0) {
		result = new char[2];
		result[0] = '0'; result[1] = '\0';
	}
	else {
		result = new char[size + 1];

		for (int i = 0; i < size; i++) {
			num = num % digit;
			digit = digit / 10;
			result[i] = (num / digit) + '0';
		}
		result[size] = '\0';
	}

	return result;
}

std::string getFile(int &curGroup, int &curFile){
	using namespace std;
	string fileName = "";
	ifstream testFile;
	bool success = false;
	int iterations = 0;

	do {
		fileName = "";

		if (curGroup < 10) fileName.append("0");
		fileName.append(ntos(curGroup));
		fileName.append("/");
		if (curGroup < 10) fileName.append("0");
		fileName.append(ntos(curGroup));
		fileName.append("_");
		if (curFile < 10) fileName.append("0");
		fileName.append(ntos(curFile));
		fileName.append(".bvh");

		testFile.open(fileName.c_str());
		if (!testFile) {
			if (iterations == 0) {
				curFile++;
				iterations++;
			}
			else if (iterations == 1) {
				curFile = 1;
				curGroup++;
				iterations++;
			}
			else if (iterations == 2) {
				curGroup = 1;
				iterations++;
			}else{	
				success = true;
				fileName = "";
			}
			testFile.close();
		}
		else {
			testFile.close();
			success = true;
		}

	} while (!success);
		
	return fileName;
}

void idle() {
	//updating the matrics (i.e. animation) done in the idle function

	//shouldn't use GLUT
	timeLast = timeCurr;
	timeCurr = SDL_GetTicks();
	char* fileName;

	armature.tick(timeCurr - timeLast);

	//float angle = (timeCurr - timeLast) / 1000.0*(3.14159265358979323846) / 4;
	
	//model matrix: transforms local model coordinates into global world coordinates
	glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(X, Y, Z))*glm::rotate(A, glm::vec3(0, 1, 0));
	//	*glm::rotate(angle, glm::vec3(0.0, 1.0, 0.0));

	//view matrix: transforms world coordinates into relative-the-camera view-space coordinates
	glm::mat4 view = glm::lookAt(glm::vec3(0.0, 0.0, 0.0), glm::vec3(0.0, 0.0, -4.0), glm::vec3(0.0, 1.0, 0.0));//I don't necessarily like this

	//projection matrix: transforms view space coordinates into 2d screen coordinates
	glm::mat4 projection = glm::perspective(45.0f, 1.0f * 600 / 480, 0.1f, 1000.0f);

	mvp = projection*view*model;//combination of model, view, projection matrices
	//mvp = model;
	
	if (command.find("p") != -1) {
		cout << "command string: " << command << endl;
	}
	if (command.find(" ") != -1) {
		cout << "Select Group -- File" << endl;
		cin >> curGroup;
		cin >> curFile;
		curFile--;
		curFrame = 65535;

	}
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
	else if (command.find("z") != -1) {
		A -= 3.14159*(timeCurr - timeLast) / 1000;
	}
	else if (command.find("c") != -1) {
		A += 3.14159*(timeCurr - timeLast) / 1000;
	}
	//glutPostRedisplay();
}

void display(SDL_Window *window) {
	glClearColor(1.0, 1.0, 1.0, 1.0);
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

	//skelly->Draw(mvp);
	skeletor->Draw(mvp);
	//glDisable(GL_DEPTH_TEST);
	//armature.draw(mvp);
	//glEnable(GL_DEPTH_TEST);
	//testMesh->Draw(mvp);
	//glFlush(); //??

	SDL_GL_SwapWindow(window);
}

void free_res() {
}

void handleKeyDown(const char key) {
	if (command.find((char)key) == -1) {
		command.push_back(key);
		cout << "key down: " << key << " command string: " << command << endl;
	}

}
void handleKeyUp(const char key) {
	command.erase(command.find(key), 1);
	cout << "key up: " << key << " command string: " << command << endl;
}

void mainLoop(SDL_Window *window) {
	while (true) {
		SDL_Event ev;
		while (SDL_PollEvent(&ev)) {
			if (ev.type == SDL_QUIT) return;
			if (ev.type == SDL_KEYDOWN) handleKeyDown(ev.key.keysym.sym);
			if (ev.type == SDL_KEYUP) handleKeyUp(ev.key.keysym.sym);
		}
		idle();
		display(window);
	}
}

int main(int argc, char *argv[]) {

	SDL_Window *window;

	SDL_Init(SDL_INIT_VIDEO );
	window = SDL_CreateWindow(
		"Animation Test",						//title
		SDL_WINDOWPOS_CENTERED,					//X-position
		SDL_WINDOWPOS_CENTERED,					//Y-position
		800,									//width
		600,									//height
		SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL);//flags

	SDL_GL_CreateContext(window);

	glewExperimental = true;
	if (glewInit()) {
		cerr << "Unable to initialize GLEW\n";
	}

	cout << glGetString(GL_VERSION) << endl;

	init();
	//maybe put this in init
	glEnable(GL_DEPTH_TEST);
	
	mainLoop(window);

	free_res();

	return 0;
	
}