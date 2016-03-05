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


//custom headers
#include "shader_utils.h"

//TODO
//Stop using globals (hahaha)
//As far as I know, using GLUT means using globals, I've heard of SDL2 to use instead, or perhaps windows API

using namespace std;//mmmmmmmmmmmmmmmmmmmmmm.
class Aliens;
class Mesh;

GLuint program;
unsigned int timeLast = 0, timeCurr, lastInput = 0;
unsigned int inputDelay = 500;

Mesh *bullet;
Mesh *player;
float pX, pY;

float bX, bY;
float bW = 0.01, bH = 0.02;
bool bulletFired = false;

string command = "";//this is a reaaly dumb way of doing input, will probably change

Aliens *aliens;

void loadObj(vector <float> &vertices, vector <unsigned int> &elements, char *fileName) {
	ifstream file;
	//istringstream sline;
	string line;
	string token;

	int in_int;
	float in_float;

	file.open(fileName);
	if (!fileName) {
		cerr << "Error loading .obj: " << fileName << endl;
		return;
	}
	//.obj format:
	//v lines have vertices
	//f lines have face (indices)
	while (getline(file, line)) {

		if (line.substr(0, 2) == "v ") {
			//Loading a vertex
			istringstream sline(line.substr(2));
			//getting the three vertex coordinates on this line
			for (int i = 0; i < 3; i++) {//there might be a fourth component, but it's not necessarily useful
				sline >> in_float;
				vertices.push_back(in_float);
			}

		}
		else if (line.substr(0, 2) == "f ") {
			//Loading a face
			istringstream sline(line.substr(2));;

			for (int i = 0; i < 3; i++) {
				string temp;

				sline >> token;
				if (token.find("/") != -1) {
					in_int = atoi(token.substr(0, token.find("/")).c_str());
				}
				else {
					in_int = atoi(token.c_str());
				}
				elements.push_back(in_int);

			}
		}
	}


}

class Mesh {
	GLuint vao;
	GLuint vbo_coord;
	GLuint ibo_elements;
	GLuint uniform_Matrix;
	GLuint program;

public:
	Mesh(char * fileName, GLuint prog);
	~Mesh();

public:
	void Draw(const glm::mat4 &mvp);
};
Mesh::Mesh(char *fileName, GLuint prog) {
	vector <GLfloat> verticies;
	vector <GLuint> elements;

	GLuint attrib_vertices;

	program = prog;

	loadObj(verticies, elements, fileName);

	glGenBuffers(1, &ibo_elements);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_elements);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, elements.size()*sizeof(elements[0]), elements.data(), GL_STATIC_DRAW);

	glGenBuffers(1, &vbo_coord);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_coord);
	glBufferData(GL_ARRAY_BUFFER, verticies.size()*sizeof(verticies[0]), verticies.data(), GL_STATIC_DRAW);

	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	glBindBuffer(GL_ARRAY_BUFFER, vbo_coord); //not needed, emphasizing that we're using this buffer
	attrib_vertices = getAttrib(program, "coord");
	glVertexAttribPointer(attrib_vertices, 3, GL_FLOAT, GL_FALSE, 0, NULL);
	glEnableVertexAttribArray(attrib_vertices);

	uniform_Matrix = getUniform(program, "mvp");
}
Mesh::~Mesh() {
	glDeleteBuffers(1, &vbo_coord);
	glDeleteBuffers(1, &ibo_elements);
	glDeleteVertexArrays(1, &vao);
}
void Mesh::Draw(const glm::mat4 &mvp) {
	glBindVertexArray(vao);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_elements);
	glUniformMatrix4fv(uniform_Matrix, 1, GL_FALSE, glm::value_ptr(mvp));

	int size;  glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &size);

	glDrawElements(GL_TRIANGLES, size / sizeof(GLuint), GL_UNSIGNED_INT, NULL);
}

class Aliens {
	float edgeSize = 0.25;
	float spacing = 0.12;
	float moveDist = 0.1;
	unsigned int moveDelay = 1000;
	unsigned int lastMove = 0;
	bool moveRight = true;
	float X = -1 + edgeSize + 0.5*spacing;
	float Y = 1 - spacing;

	Mesh* model[3];

	glm::mat4 rotMat;
	glm::mat4 transMat;

public:
	Aliens();

	unsigned int alienNum = 0;

	unsigned int col;
	unsigned int row;

	vector <bool> isDead;

	void Move(unsigned int time);
	void Draw();
	bool CheckHit(float x, float y, float w, float h);


};

Aliens::Aliens()
{
	model[0] = new Mesh("obj_pyramid", program);
	model[1] = new Mesh("obj_diamond", program);
	model[2] = new Mesh("obj_rect", program);

	col = (1.0 - edgeSize) * 2 / spacing; //(used fraction of screen)*2/spacing 
	row = 7;
	isDead.resize(col*row, false);
	alienNum = col*row;

	for (int i = 0; i < col; i++) {//this would be more useful if more information was stored on a per ship basis
		for (int j = 0; j < row; j++) {
			isDead[(int)(i*row + j)] = false;// I think this is done in the resize above? meh
		}
	}
}
void Aliens::Draw() {

	glm::mat4 matBase = glm::rotate((float)((timeCurr) / ((float)moveDelay)*(3.14159265358979323846)/4.0), glm::vec3(0.0, 1.0, 0.0))*glm::scale(glm::vec3(spacing, spacing, spacing));
	glm::mat4 matModel;

	for (int i = 0; i < col; i++) {
		for (int j = 0; j < row; j++) {
			if (!isDead[row*i + j]) {
				matModel = glm::translate(glm::mat4(1.0f), glm::vec3(spacing*i + X, Y - spacing*j, -0.0)) * matBase;
				if (j > 3)
					model[2]->Draw(matModel);
				else if (j > 1)
					model[1]->Draw(matModel);
				else
					model[0]->Draw(matModel);
			}
		}
	}
}
void Aliens::Move(unsigned int time) {
	if (time - lastMove >= moveDelay) {
		lastMove = time;
		X += moveDist * (moveRight ? 1 : -1);

		for (int i = 0; i < col; i++) {
			for (int j = 0; j < row; j++) {
				if (!isDead[(int)(i*row + j)] && abs(X + spacing*i) > 1) {
					moveRight = !moveRight;
					Y -= 0.8*spacing;
					moveDelay *= 0.9;
					X += moveDist * (moveRight ? 1 : -1);
					return;
				}
			}
		}

	}
}
bool Aliens::CheckHit(float x, float y, float w, float h) {
	//the laziest hit detection
	for (int i = 0; i < col; i++) {
		for (int j = 0; j < row; j++) {
			if (!isDead[row*i + j] &&
				abs(spacing*i + X - x) <= spacing / 2 &&
				abs(Y - spacing*j - y) <= spacing/ 2) {
				isDead[row*i + j] = true;
				return true;
			}
		}
	}

	return false;
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

bool init() {
	struct shaderInfo shaders[] = { { GL_VERTEX_SHADER, "vs.glsl" },
	{ GL_FRAGMENT_SHADER, "fs.glsl" } };
	program = LoadProgram(shaders, 2);

	player = new Mesh("obj_rect", program);
	bullet = new Mesh("obj_cube", program);
	pX = 0;
	pY = -1 + 0.2;
	command = "";

	aliens = new Aliens();

	glUseProgram(program);
	timeLast = glutGet(GLUT_ELAPSED_TIME);

	return true;
}
//recompile
void idle() {
	timeCurr = glutGet(GLUT_ELAPSED_TIME);

	aliens->Move(timeCurr);
	if (bulletFired) {
		bulletFired = !aliens->CheckHit(bX, bY, 0.01, 0.2);
		
		if (bY > 1.0) {
			bY = pY;
			bulletFired = false;
		}else
			bY += 1.0*(timeCurr - timeLast) / 1000;
	}

	if (command.find("a") != -1) {
		pX -= 1.0*(timeCurr - timeLast)/1000;
	}
	else if (command.find("d") != -1) {
		pX += 1.0*(timeCurr - timeLast)/1000;
	}

	if (command.find(" ") != -1) {
		if (!bulletFired) {
			cout << "firing!";
			bulletFired = true;
			bX = pX;
			bY = pY + 0.2;
		}
	}

	timeLast = timeCurr;

	glutPostRedisplay();
}

void display() {
	glClearColor(0.0, 0.0, 0.0, 1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	aliens->Draw();
	player->Draw(glm::translate(glm::vec3(pX, pY, 0.0)) * glm::scale(glm::vec3(0.2, 0.2, 0.2)));
	if (bulletFired) {
		bullet->Draw(glm::translate(glm::vec3(bX, bY, 0.0)) * glm::scale(glm::vec3(bW, bH, 0.2)));
	}
	glFlush();
}

void free_res() {
	glDeleteProgram(program);
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
	glutIdleFunc(idle);
	glutKeyboardFunc(handleKeyDown);
	glutKeyboardUpFunc(handleKeyUp);


	glutMainLoop();
	glutPostRedisplay();
	free_res();
}
