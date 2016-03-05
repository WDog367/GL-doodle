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


//TODO
//Stop using globals (hahaha)

using namespace std;//mmmmmmmmmmmmmmmmmmmmmm
GLfloat X = 0.0, Y = -20.0, Z = -50;
std::string command = "";

glm::mat4 mvp;
int curFrame = 0;
int curAnim = 0;
Armature armature;
int c1, c2;
string fileName;
int curGroup =2 , curFile = 1;


//const int maxfile = 124;
bool repeat = false;

unsigned int timeLast, timeCurr;

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
bool init() {
	//fileName = new char[9];
	fileName = "01_01.bvh";

	cout << "loading file" << endl;
	armature.loadBVHArm("walk.bvh");

	armature.print();

	timeLast = SDL_GetTicks();
	return true;
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

	if (timeLast - timeCurr > 500) {
		curFrame++;
		//passed the max
		if (curFrame >= armature.frames.size()) {
			curFrame = 0;
			if (!repeat) {
				curFile++;

				armature.loadBVHAnim_quick((getFile(curGroup, curFile)).c_str());
			}
			cout << curGroup << "_" << curFile << endl;
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

	//glutPostRedisplay();
}

void display(SDL_Window *window) {
	glClearColor(1.0, 1.0, 1.0, 1.0);
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

	armature.draw(mvp);
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
	cout << ntos(123456789) << endl;

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