#include <iostream>
#include "GL/glew.h"
#include "GL/freeglut.h"//may want to start using "GL/glut.h" to wean off of freeglut exclusive stuff?


void Display() {
	glClear(GL_COLOR_BUFFER_BIT);
	glFlush();
}

int main(int argc, char* argv[]) {
	//init glut
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGBA);
	glutInitWindowSize(600, 480);
	glutInitContextVersion(4, 3);//freeglut stuff
	glutInitContextProfile(GLUT_CORE_PROFILE);//freeglut stuff
	glutCreateWindow("Test Window");

	if (glewInit()) {
		std::cerr << "Unable to initialize GLEW????\n";
		return 0;
	}
	
	glutDisplayFunc(Display);

	glutMainLoop();

}