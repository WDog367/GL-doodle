#pragma once
#include <ft2build.h>
#include FT_FREETYPE_H  

#include <string>
#include <iostream>
#include <map>

#include "GL/glew.h"
#include "glm/glm.hpp"//includes most (if not all?) of GLM library
#include "glm/gtx/transform.hpp"
#include "glm/gtc/type_ptr.hpp"

using std::string;

/// Implementation details taken from learnopengl.com
//	https://learnopengl.com/#!In-Practice/Text-Rendering

class Text_Renderer {
public:
	struct Character {
		GLuint TextureID;
		glm::ivec2 Size;
		glm::ivec2 Bearing;
		GLuint Advance;
	};

	std::map<GLchar, Character> Characters;

	glm::mat4 projection;
	GLuint program;
	GLuint VAO;
	GLuint VBO;

	Text_Renderer();
	void SetColour(glm::vec3 colour);
	void RenderText(string text, GLfloat x, GLfloat y);

};