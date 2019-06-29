#include "SDL.h"
//#include "SDL_keyboard.h"
//#include "SDL_keycode.h"

#include <ft2build.h>

#include <vector>
#include <queue>
#include <list>
#include "math.h"
#include "GL/glew.h"
#include "glm/glm.hpp"//includes most (if not all?) of GLM library
#include "glm/gtx/transform.hpp"
#include "glm/gtc/type_ptr.hpp"

#include "text_utils.h"

#include "mesh_utils.h"
#include "shader_utils.h"

#include <iostream>

#include <random>

#include "collision_utils.h"
#include "OctTree.h"
#include "Entity.h"

//todo: get rid of global variables
//maybe use singleton game class?

Collider *player;
Collider *opponent;

Entity* player_entity;

Mesh* ship_mesh;
Mesh* shpere_mesh;
OBB *ship_OBB;
std::vector<glm::vec3> dots;

glm::vec3 playerpos = glm::vec3(0, 0, 0);

glm::vec3 cameraPos = glm::vec3(0, 0, 10);
glm::quat cameraRot = glm::quat(0, 0, 0, 1);

void drawLine(const glm::vec3 pos, glm::vec3 pos2, glm::mat4 &mvp, glm::vec3 colour = glm::vec3(0, 0, 0)) {
	using namespace std;

	GLuint vao;
	GLuint vbo;
	GLuint vbo_index;
	GLuint program;
	GLuint attrib_coord;
	GLuint attrib_index;
	GLuint uniform_matrix;

	GLfloat vertices[] = {
		pos.x, pos.y, pos.z,
		pos2.x, pos2.y, pos2.z,
	};

	std::vector<GLfloat> skeleton;

	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	struct shaderInfo shaders[] = { { GL_VERTEX_SHADER, "vs_wireframe.glsl" },
	{ GL_FRAGMENT_SHADER, "fs_wireframe.glsl" } };

	program = LoadProgram(shaders, 2);
	glUseProgram(program);

	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	attrib_coord = getAttrib(program, "coord");
	glEnableVertexAttribArray(attrib_coord);
	glVertexAttribPointer(
		attrib_coord,	//attrib 'index'
		3,				//pieces of data per index
		GL_FLOAT,		//type of data
		GL_FALSE,		//whether to normalize
		0,				//space b\w data
		0				//offset from buffer start
		);

	uniform_matrix = getUniform(program, "mvp");

	GLuint uniform_colour = getUniform(program, "colour");
	glUniform3fv(uniform_colour, 1, glm::value_ptr(colour));

	glUniformMatrix4fv(uniform_matrix, 1, GL_FALSE, glm::value_ptr(mvp));

	int size;  glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &size);

	glDrawArrays(GL_LINES, 0, 2);

	glDeleteBuffers(1, &vbo);
	glDeleteVertexArrays(1, &vao);
	glDeleteProgram(program);

}
void drawDot(const glm::vec3 pos, const glm::mat4 &mvp, glm::vec3 colour = glm::vec3(0, 0, 0)) {
	using namespace std;

	GLuint vao;
	GLuint vbo;
	GLuint vbo_index;
	GLuint program;
	GLuint attrib_coord;
	GLuint attrib_index;
	GLuint uniform_matrix;

	GLfloat vertices[] = {
		pos.x, pos.y, pos.z,
	};

	std::vector<GLfloat> skeleton;

	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	struct shaderInfo shaders[] = { { GL_VERTEX_SHADER, "vs_wireframe.glsl" },
	{ GL_FRAGMENT_SHADER, "fs_wireframe.glsl" } };

	program = LoadProgram(shaders, 2);
	glUseProgram(program);

	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	attrib_coord = getAttrib(program, "coord");
	glEnableVertexAttribArray(attrib_coord);
	glVertexAttribPointer(
		attrib_coord,	//attrib 'index'
		3,				//pieces of data per index
		GL_FLOAT,		//type of data
		GL_FALSE,		//whether to normalize
		0,				//space b\w data
		0				//offset from buffer start
		);

	uniform_matrix = getUniform(program, "mvp");

	GLuint uniform_colour = getUniform(program, "colour");
	glUniform3fv(uniform_colour, 1, glm::value_ptr(colour));

	glUniformMatrix4fv(uniform_matrix, 1, GL_FALSE, glm::value_ptr(mvp));

	int size;  glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &size);

	glDrawArrays(GL_POINTS, 0, 1);

	glDeleteBuffers(1, &vbo);
	glDeleteVertexArrays(1, &vao);
	glDeleteProgram(program);

}

void Init_shapes() {
	opponent = new OBB(glm::vec3(0, 0, 0), glm::vec3(1, 1, 1));
	player = new OBB(glm::vec3(0, 0, 0), glm::vec3(1, 1, 1));

	std::vector<float> verts;
	std::vector<unsigned int> elem;
	std::vector<glm::vec3> nermals;
	loadObj(verts, elem, "sphere.obj", nermals);

	player = new ColliderGroup(glm::mat4(1.0f));
	static_cast<ColliderGroup*>(player)->AddChild(new ConvexHull((glm::vec3*)verts.data(), verts.size() / 3, glm::mat4(1.0f)));
	static_cast<ColliderGroup*>(player)->AddChild(new OBB(glm::vec3(-.5, -3, -.5), glm::vec3(.5, -1, .5)));
	//player = new ConvexHull((glm::vec3*)verts.data(), verts.size()/3, glm::mat4(1.0f));

	struct shaderInfo shaders[] = { { GL_VERTEX_SHADER, "vs.glsl" },
	{ GL_FRAGMENT_SHADER, "fs.glsl" } };

	GLuint prog = LoadProgram(shaders, 2);
	ship_mesh = new Mesh("ship.obj", prog);
	shpere_mesh = new Mesh("sphere.obj", prog);

	//read the vertices of the ship, and use them to initialize an OBB, for collision
	GLfloat* vertices;
	int size;

	glBindBuffer(GL_COPY_READ_BUFFER, ship_mesh->vbo_coord);
	vertices = (GLfloat*)glMapBuffer(GL_COPY_READ_BUFFER, GL_READ_ONLY);
	glGetBufferParameteriv(GL_COPY_READ_BUFFER, GL_BUFFER_SIZE, &size);

	ship_OBB = new OBB((glm::vec3*)vertices, size / sizeof(glm::vec3));
	//player = new OBB((glm::vec3*)vertices, size / sizeof(glm::vec3));

	glUnmapBuffer(GL_COPY_READ_BUFFER);
	glBindBuffer(GL_COPY_READ_BUFFER, -1);
}

//fills the level region with randomly placed cubes of size (2,2,2)
std::vector<Entity*> fill_level(const glm::vec3 &levelBound, int num) {
	std::vector<Entity*> result(num);

	for (int i = 0; i < num; i++) {

		glm::vec3 min(rand() % (int)levelBound.x, rand() % (int)levelBound.y, rand() % (int)levelBound.z);
		glm::vec3 max = min + glm::vec3(2, 2, 2);
		
		if (i < 10) {
			std::vector<float> verts;
			std::vector<unsigned int> elem;
			std::vector<glm::vec3> nermals;
			loadObj(verts, elem, "sphere.obj", nermals);

			ColliderGroup* group = new ColliderGroup(glm::mat4(1.0f));
			group->AddChild(new ConvexHull((glm::vec3*)verts.data(), verts.size() / 3, glm::mat4(1.0f)));
			//group->AddChild(new OBB(glm::vec3(-.5, -3, -.5), glm::vec3(.5, -1, .5)));

			result[i] = new Entity(group);
		}
		else {
			if (i % 2 == 0) {
				result[i] = new Entity(new OBB(min, max));
			}
			else {
				std::vector<float> verts;
				std::vector<unsigned int> elem;
				std::vector<glm::vec3> nermals;
				loadObj(verts, elem, "sphere.obj", nermals);

				result[i] = new Entity(new ConvexHull((glm::vec3*)verts.data(), verts.size() / 3, glm::translate(min)));
			}
		}
	}

	return result;
}

void handleKeyUp(const char key) {

}

bool drawOctree = false;
bool drawColliders = false;
bool drawBounds = false;
bool drawVisible = true;

AABB levelRegion(glm::vec3(-256, -256, -256), glm::vec3(256, 256, 256));
OctTree *collisionTree;
std::vector<Entity*> Entitys;
std::vector<CollisionInfo> collisionList;
int Entitynum = 500;

glm::mat4 model(1.0f);

//key presses
void handleKeyDown(const char key) {
	switch (key) {
	case 'W':
	case 'w':player->translate(cameraRot*glm::vec3(0, 0, -1)); playerpos += cameraRot*glm::vec3(0, 0, -1); break;
	case 'S':
	case 's':player->translate(cameraRot*glm::vec3(0, 0, 1));  playerpos += cameraRot*glm::vec3(0, 0, 1);  break;
	case 'A':
	case 'a':player->translate(cameraRot*glm::vec3(-1, 0, 0)); playerpos += cameraRot*glm::vec3(-1, 0, 0);  break;
	case 'D':
	case 'd':player->translate((cameraRot*glm::vec3(1, 0, 0))); playerpos += cameraRot*glm::vec3(1, 0, 0);  break;
	case 'Q':
	case 'q':player->translate(cameraRot*glm::vec3(0, -1, 0)); playerpos += cameraRot*glm::vec3(0, -1, 0); break;
	case 'E':
	case 'e':player->translate(cameraRot*glm::vec3(0, 1, 0)); playerpos += cameraRot*glm::vec3(0, 1, 0);  break;
	case 'Z':
	case 'z':player->rotate(glm::quat(cos(M_PI / 12), (float)sin(M_PI / 12)*(glm::vec3(0, 1, 0)))); break;
	case 'C':
	case 'c':player->rotate(glm::quat(-cos(M_PI / 12), (float)sin(M_PI / 12)*(glm::vec3(0, 1, 0)))); break;
	case 'R':
	case 'r': player->rotate(glm::quat(cos(M_PI / 12), (float)sin(M_PI / 12)*(glm::vec3(1, 0, 0))));  break;
	case 'F':
	case 'f': player->rotate(glm::quat(-cos(M_PI / 12), (float)sin(M_PI / 12)*(glm::vec3(1, 0, 0))));  break;
	case '1': drawOctree = !drawOctree; break;
	case '2': drawColliders = !drawColliders; break;
	case '3': drawBounds = !drawBounds; break;
	case '4': drawVisible = !drawVisible; break;
	case 'v' :
	case 'V':	for (auto e : Entitys) { 
					e->velocity = glm::vec3(128, 128, 128) - glm::vec3(e->collider->getTransform()[3]);
					float l = e->velocity.length();
					e->velocity.x /= l;
					e->velocity.y /= l;
					e->velocity.z /= l;

				} 
				break;
			
	}
}

//mouse movemet
void handleMotion(int x, int y) {
	float dx = M_PI / 180 * x / 10;
	float dy = M_PI / 180 * y / 10;

	cameraRot = glm::quat(-cos(dx / 2), (float)sin(dx / 2)*(cameraRot*glm::vec3(0, 1, 0)))*cameraRot;
	cameraRot = glm::quat(-cos(dy / 2), (float)sin(dy / 2)*(cameraRot*glm::vec3(1, 0, 0)))*cameraRot;

	player->rotate(glm::quat(-cos(dx / 2), (float)sin(dx / 2)*(glm::vec3(0, 1, 0))));
	player->rotate(glm::quat(-cos(dy / 2), (float)sin(dy / 2)*(glm::vec3(1, 0, 0))));

}

#if 0

int main(int argc, char *argv[]) {
	SDL_Window * window;
	SDL_Init(SDL_INIT_VIDEO);
	window = SDL_CreateWindow("GLTest", 50, 50, 800, 600, SDL_WINDOW_OPENGL);

	//initialize OpenGL
	SDL_GL_CreateContext(window);
	glewInit();

	glClearColor(1.0, 1.0, 1.0, 1.0);

	Text_Renderer Text;

	while (1) {
		//polling for events
		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT) return 0;
		}

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		Text.RenderText("Hello World", 100, 100);
		
		SDL_GL_SwapWindow(window);
	}
}

#elif 0

struct Character {
	GLuint     TextureID;  // ID handle of the glyph texture
	glm::ivec2 Size;       // Size of glyph
	glm::ivec2 Bearing;    // Offset from baseline to left/top of glyph
	GLuint     Advance;    // Offset to advance to next glyph
};

std::map<GLchar, Character> Characters;

int main(int argc, char *argv[]) {
	SDL_Window * window;
	SDL_Init(SDL_INIT_VIDEO);
	window = SDL_CreateWindow("GLTest", 50, 50, 800, 600, SDL_WINDOW_OPENGL);

	//initialize OpenGL
	SDL_GL_CreateContext(window);
	glewInit();

	//init FT
	FT_Library ft;
	FT_Face face;

	if (FT_Init_FreeType(&ft)) {
		std::cerr << "Error initializing freetype" << std::endl;
	}

	if (FT_New_Face(ft, "arial.ttf", 0, &face)) {
		std::cerr << "Error loading font" << std::endl;
	}

	FT_Set_Pixel_Sizes(face, 0, 48);

	//does this make it less efficient?
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	for (GLubyte i = 0; i < 128; i++) {
		std::cout << i << "; " << static_cast<int>(i) << std::endl;
		if (FT_Load_Char(face, i, FT_LOAD_RENDER)) {
			std::cerr << "Error loading character: " << i << std::endl;
		}

		//initialize the character, and add it to our map
		GLuint texture;
		glGenTextures(1, &texture);
		glBindTexture(GL_TEXTURE_2D, texture);
		glTexImage2D(
			GL_TEXTURE_2D,
			0,
			GL_RED,
			face->glyph->bitmap.width,
			face->glyph->bitmap.rows,
			0,
			GL_RED,
			GL_UNSIGNED_BYTE,
			face->glyph->bitmap.buffer
		);

		// Set texture options
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		//store
		Character character = {
			texture,
			glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
			glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
			face->glyph->advance.x
		};
		Characters.emplace(i, character);

	}

	//cleanup
	FT_Done_Face(face);
	FT_Done_FreeType(ft);

	//end init FT

	GLfloat vertices[] = {
		200.0f, 50.0f, 0.0f, 0.0f,
		600.f, 50.f,  1.0f, 0.0f,
		400.f,  500.f, 0.5f, 1.0f
	};

	GLuint VAO, VBO, program;

	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);

	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*4*6, vertices, GL_STATIC_DRAW);

	struct shaderInfo shaders[] = { { GL_VERTEX_SHADER, "text_vs.glsl" },
	{ GL_FRAGMENT_SHADER, "text_fs.glsl" } };

	program = LoadProgram(shaders, 2);
	glUseProgram(program);

	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, NULL);
	glEnableVertexAttribArray(0);

	glm::mat4 projection = glm::ortho(0.0f, 800.0f, 0.0f, 600.0f);

	glUniformMatrix4fv(getUniform(program, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

	glUniform1ui(getUniform(program, "text"), 0);
	glActiveTexture(0);
	glBindTexture(GL_TEXTURE_2D, Characters['a'].TextureID);

	glClearColor(1.0, 1.0, 1.0, 1.0);


	std::string text = "hello world";
	int x = 100;
	int y = 100;
	int i = 0;

	while (1) {
		//polling for events
		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT) return 0;
		}

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		x = 100;
		// draw the word
		for (auto c = text.begin(); c != text.end(); c++) {
			Character ch = Characters[*c];

			GLfloat xpos = x + ch.Bearing.x;
			GLfloat ypos = y - (ch.Size.y - ch.Bearing.y);

			GLfloat w = ch.Size.x;
			GLfloat h = ch.Size.y;

			GLfloat vertices[6][4] = {
				{ xpos,     ypos + h,   0.0, 0.0 },
				{ xpos,     ypos,       0.0, 1.0 },
				{ xpos + w, ypos,       1.0, 1.0 },

				{ xpos,     ypos + h,   0.0, 0.0 },
				{ xpos + w, ypos,       1.0, 1.0 },
				{ xpos + w, ypos + h,   1.0, 0.0 }
			};

			glBindTexture(GL_TEXTURE_2D, ch.TextureID);

			if (i = glGetError()) {
				std::cout << "error 1: " << i << std::endl;
			}
			else {
				std::cout << " no error " << std::endl;
			}

			glBindBuffer(GL_ARRAY_BUFFER, VBO);
			glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);

			if (i = glGetError()) {
				std::cout << "error: " << i << std::endl;
			}

			glDrawArrays(GL_TRIANGLES, 0, 6);

			x += (ch.Advance >> 6);
		}
		SDL_GL_SwapWindow(window);
	}
}

#else
int main(int argc, char *argv[]) {
	SDL_Window *window;
	OBB ball(glm::vec3(-1, -1, -1), glm::vec3(1, 1, 1));

	//initializing SDL window
	SDL_Init(SDL_INIT_VIDEO);
	window = SDL_CreateWindow("Super Amazing Fun Times", 100, 100, 800, 600, SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL);
	SDL_SetRelativeMouseMode(SDL_TRUE);

	//initializing Opengl stuff
	SDL_GL_CreateContext(window);
	glewExperimental = GL_TRUE;
	glewInit();
	glEnable(GL_DEPTH_TEST);
	
	//initialize the models
	Init_shapes();

	Entitys = fill_level(levelRegion.max, Entitynum);

	player_entity = new Entity(player);
	player_entity->mesh = ship_mesh;
	Entitys.push_back(player_entity);

	collisionTree = new OctTree(levelRegion);
	for (Entity* entity : Entitys) {
		collisionTree->Insert(entity->collider);
		entity->mesh = shpere_mesh;
	}
	collisionTree->UpdateTree();

	int timeCurr = SDL_GetTicks();
	int timeLast = timeCurr;

	//main loop
	glEnable(GL_PROGRAM_POINT_SIZE);
	glPointSize(10);

	Text_Renderer tr;
	tr.SetColour(glm::vec3(0.f, 1.f, 0.f));
	while (1) {
		timeLast = timeCurr;
		timeCurr = SDL_GetTicks();

		//polling for events
		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT) return 0;
			if (event.type == SDL_KEYDOWN) handleKeyDown(event.key.keysym.sym);
			if (event.type == SDL_KEYUP) handleKeyUp(event.key.keysym.sym);
			if (event.type == SDL_MOUSEMOTION) handleMotion(event.motion.xrel, event.motion.yrel);
		}

		//process the collision
		collisionList.clear();

		collisionTree->UpdateTree();
		collisionTree->CheckCollision(collisionList);


		//render the scene
		glClearColor(1.0, 1.0, 1.0, 1.0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		//calculate camera position
		cameraPos = glm::vec3(glm::translate(glm::mat4(1.0), glm::vec3(Entitys.back()->collider->getTransform()[3]))*(cameraRot*glm::vec4(0, 0, 10, 1.0)));

		glm::mat4 view = glm::inverse(glm::mat4_cast(cameraRot)) * glm::translate(glm::mat4(1.0), -cameraPos);
		glm::mat4 projection = glm::perspective(45.0f, 1.0f * 600 / 480, 0.1f, 1000.0f);
		glm::mat4 vp = projection * view;//combination of model, view, projection matrices

									   //process collision results
		for (int i = 0; i < collisionList.size(); i++) {
			collisionList[i].member[0]->owner->bound.colour = glm::vec3(1, 1, 0);
			collisionList[i].member[1]->owner->bound.colour = glm::vec3(1, 1, 0);
			collisionList[i].member[0]->owner->translate(-collisionList[i].collision.normal*collisionList[i].collision.depth);
			collisionList[i].member[0]->owner->velocity = (-collisionList[i].collision.normal*collisionList[i].collision.depth);
		}

		if (drawOctree) {
			//draw the OctTree
			collisionTree->draw(vp);
		}

		for (int i = 0; i < Entitys.size(); i++) {
			Entitys[i]->Tick(((float)(timeCurr - timeLast))/1000);
			if (drawVisible) {
				Entitys[i]->draw(vp);
			}
			if (drawColliders) {
				Entitys[i]->collider->draw(vp);
			}
			if (drawBounds) {
				Entitys[i]->collider->getBounds().draw(vp);
			}
		}
		Entitys.back()->velocity = glm::vec3(0, 0, 0);

		for (int i = 0; i < dots.size(); i++) {
			drawDot(dots[i], vp);
		}

		tr.RenderText("FPS: " + std::to_string(1000 / (timeCurr - timeLast)) + " ; " + std::to_string(timeCurr - timeLast) + "ms", 0, 0);

		SDL_GL_SwapWindow(window);

	}

	//	return 0;
}
#endif