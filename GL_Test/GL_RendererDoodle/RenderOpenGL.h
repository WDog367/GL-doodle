#pragma once

#include "GL/glew.h" // for GL types

#include "Texture.h"

// get error codes and debug (not needed with callbacks)
bool getGLErrors();

class GLR_Texture {
	const Texture* m_src = nullptr;	// doesn't own
	GLuint m_id = 0; // owns 

	// todo: fill from `Texture`
	struct GLR_TexInfo {

	};
public:
	GLR_Texture(const Texture* src);

	// OpenGL Texture lifecycle
	bool genTexture();
	bool uploadTexture(const Image& image);
	bool bindTexture(uint32_t textureSlot = 0) const;
	void deleteTexture();

	~GLR_Texture();

};

struct GLR_Mesh {
	GLuint vbo_coord;
	GLuint vbo_normal;
	GLuint vbo_uv;
	GLuint vbo_tan;
	GLuint vbo_bitan;

	GLuint vao;

	unsigned int vertexCount;
};

struct GLR_MeshData {
	std::vector<glm::vec3> vertices;
	std::vector<glm::vec3> normals;
	std::vector<unsigned int> indices;
	std::vector<glm::vec2> uv;
	std::vector<glm::vec3> tangent;
	std::vector<glm::vec3> bitangent;
};

struct GLR_ShaderProgram {
	GLuint m_gl_id;

	GLint getUniformLocation(const std::string& name);

	GLR_ShaderProgram(const std::string& vs, const std::string& fs, const std::string& gs = "");
};

#include "Material.h"

struct GLR_MaterialValue {
	MaterialValue* m_src;

	virtual std::string id() const = 0;
	virtual std::string getShaderCode(struct GPURenderOptions& options) const = 0;
	virtual void uploadUniformsToGPU(struct GPURenderOptions& options, GLR_ShaderProgram& shader) const = 0;
};

// Material separate from shader ...
//	binds shader with textures, etc.
struct GLR_Material {
	Material* m_src;

	// m_shaderModel;
	// GLR_Texture getTextureFrominput();
	// GLR_ShaderProgram getShader();

	virtual GLR_ShaderProgram& reserveShader(struct GPURenderOptions& options) const = 0;
	virtual void uploadUniformsToGPU(struct GPURenderOptions& options, GLR_ShaderProgram& shader) const = 0;

};

GLR_Material* GLR_makeMaterial(Material& mat);

// about equivalent to GeometryNode ..
struct GLR_Model {
	GLR_Mesh& m_mesh;
	GLR_Material& m_mat;
};