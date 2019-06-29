#include "PROJECT_OPTIONS.h"

#include "text_utils.h"
#include "shader_utils.h"

/// Implementation details taken from learnopengl.com
//	https://learnopengl.com/#!In-Practice/Text-Rendering

Text_Renderer::Text_Renderer() {

	FT_Library ft;
	FT_Face face;

	if (FT_Init_FreeType(&ft)) {
		std::cerr << "Error initializing freetype" << std::endl;
	}

	if (FT_New_Face(ft, RESOURCE_DIR "/" "arial.ttf", 0, &face)) {
		std::cerr << "Error loading font" << std::endl;
	}

	FT_Set_Pixel_Sizes(face, 0, 48);

	//does this make it less efficient?
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	for (GLubyte i = 0; i < 128; i++) {
		if (FT_Load_Char(face, i, FT_LOAD_RENDER)) {
			std::cerr << "Error loading character: " << (char)i << std::endl;
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

	//load the shaders
	struct shaderInfo shaders[] = { { GL_VERTEX_SHADER, "text_vs.glsl" },
	{ GL_FRAGMENT_SHADER, "text_fs.glsl" } };
	program = LoadProgram(shaders, 2);

	//setup the projection matrix
	//todo: actually get screen size
	projection = glm::ortho(0.0f, 800.0f, 0.0f, 600.0f);

	GLuint uniform_projection = getUniform(program, "projection");
	GLuint uniform_textColour = getUniform(program, "textColor");
	GLuint uniform_text = getUniform(program, "text");

	glUseProgram(program);
	glUniformMatrix4fv(uniform_projection, 1, GL_FALSE, glm::value_ptr(projection));
	glUniform3fv(uniform_textColour, 1, (GLfloat*)&glm::vec3(0, 0, 1));
	glUniform1i(uniform_text, 0);

	//load the VBO
	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);

	glGenBuffers(1, &VBO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 4 * 6, NULL, GL_DYNAMIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), 0);

	//cleanup
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

void Text_Renderer::SetColour(glm::vec3 colour) {
	glUseProgram(program);
	glUniform3fv(getUniform(program, "textColor"), 1, (float*)&colour);
}

void Text_Renderer::RenderText(string text, GLfloat x, GLfloat y) {
	GLuint uniform_projection = getUniform(program, "projection");
	GLuint uniform_textColour = getUniform(program, "textColor");
	GLuint uniform_text = getUniform(program, "text");

	glUseProgram(program);
	glUniformMatrix4fv(uniform_projection, 1, GL_FALSE, glm::value_ptr(projection));
	//glUniform3fv(uniform_textColour, 1, (GLfloat*)&glm::vec3(0, 0, 0));
	glUniform1i(uniform_text, 0);

	GLfloat scale = 1.0;

	bool doEnableBlend = glIsEnabled(GL_BLEND) == GL_FALSE;

	if (doEnableBlend) {
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}

	glActiveTexture(GL_TEXTURE0);
	glBindVertexArray(VAO);

	using std::cout;
	using std::endl;
	int i = 0;

	for (auto c = text.begin(); c != text.end(); c++) {
		Character ch = Characters[*c];

		GLfloat xpos = x + ch.Bearing.x * scale ;
		GLfloat ypos = y - (ch.Size.y - ch.Bearing.y)  *scale ;

		GLfloat w = ch.Size.x;
		GLfloat h = ch.Size.y;

		// update VBO for char
		GLfloat vertices[6][4] = {
			{ xpos,     ypos + h,   0.0, 0.0 },
			{ xpos,     ypos,       0.0, 1.0 },
			{ xpos + w, ypos,       1.0, 1.0 },

			{ xpos,     ypos + h,   0.0, 0.0 },
			{ xpos + w, ypos,       1.0, 1.0 },
			{ xpos + w, ypos + h,   1.0, 0.0 }
		};

		glBindTexture(GL_TEXTURE_2D, ch.TextureID);

		//move the vertices into the VBO

		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		//render the character
		glDrawArrays(GL_TRIANGLES, 0, 6);

		//move the x coord forward
		x += (ch.Advance >> 6); // Bitshift by 6 to get value in pixels (2^6 = 64)
	}

	glBindVertexArray(0);
	glBindTexture(GL_TEXTURE_2D, 0);

	if (doEnableBlend) {
		glDisable(GL_BLEND);
	}
}

