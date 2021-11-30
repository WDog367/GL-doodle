// main go
#include "RenderOpenGL.h"

#include "Scene.h"
#include "SceneUtil.h"
#include "Texture.h"
#include "Asset.h"
#include "Camera.h"
#include "shader_utils.h"

#include "GL/glew.h"
#include "SDL.h"
#include "glm/glm.hpp"
#include "glm/gtx/transform.hpp"
#include "glm/gtc/type_ptr.hpp"

#include <iostream>
#include <chrono>
#include <thread>
#include <unordered_map>

bool getGLErrors() {
	GLenum ecode = GL_NO_ERROR;
	bool error = false;
	while (GL_NO_ERROR != (ecode = glGetError())) {
		error = true;
		const GLubyte* str = gluErrorString(ecode);
		printf("** GL ERROR ** 0x%x : %s\n", ecode, str);
	}
	return error;
}

GLR_Texture::GLR_Texture(const Texture* src) : m_src(src), m_id(0) {}

bool GLR_Texture::genTexture() {
	glGenTextures(1, &m_id);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_id);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);

	return true;
}

bool GLR_Texture::uploadTexture(const Image& image) {
	glBindTexture(GL_TEXTURE_2D, m_id);

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, image.w, image.h, 0, GL_RGB, GL_FLOAT, image.data);

	//glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 1, 1, GL_RGB, GL_FLOAT, &color);

	return true;
}

bool GLR_Texture::bindTexture(uint32_t textureSlot) const {
	glActiveTexture(GL_TEXTURE0 + textureSlot);
	glBindTexture(GL_TEXTURE_2D, m_id);

	return true;
}

void GLR_Texture::deleteTexture() {
	glDeleteTextures(1, &m_id);
}

GLR_Texture::~GLR_Texture() {
	deleteTexture();
}

GLint GLR_ShaderProgram::getUniformLocation(const std::string& name) {
	return glGetUniformLocation(m_gl_id, name.c_str());
}

GLR_ShaderProgram::GLR_ShaderProgram(const std::string& vs, const std::string& fs, const std::string& gs) {
	if (gs == "") {
		shaderInfo shaders[] =
		{ {GL_VERTEX_SHADER, vs.c_str()},
			{GL_FRAGMENT_SHADER, fs.c_str() }
		};
		m_gl_id = LoadProgram(shaders, 2);
	}
	else {
		shaderInfo shaders[] =
		{ {GL_VERTEX_SHADER, vs.c_str()},
			{GL_FRAGMENT_SHADER, fs.c_str()},
			{GL_GEOMETRY_SHADER, gs.c_str()},
		};
		m_gl_id = LoadProgram(shaders, 3);
	}
}

// generate_id:

// the reason to pass in the direct filename instead of the shader id
// is so that we can check internally for a matching shader
// before doing the file preprocessing step 
std::string generateShaderProgramId(
	const std::list<const std::map<std::string, std::string>*>& define_sets,
	const std::string& vs, const std::string& fs,
	const std::string& gs = "") {
	// add filenames
	std::string shaderId = vs + "." + fs;
	// add optional filenames
	if (gs != "") {
		shaderId += ".gs";
	}
	// add defines
	for (const auto& defines : define_sets) {
		for (const auto& define : *defines) {
			shaderId += "." + define.first + ".";
		}
	}

	return shaderId;
}


#include "Primitive.h"
#include "Mesh.h"

void generateSphere(unsigned int& out_count, std::vector<glm::vec3>& out_vertices, std::vector<glm::vec3>& out_normals, std::vector<glm::vec2>& out_uvs, std::vector<glm::vec3>& out_tangents, std::vector<glm::vec3>& out_bitangents, int rows = 16, int segs = 32);
void generateCube(unsigned int& out_count, std::vector<glm::vec3>& out_vertices, std::vector<glm::vec3>& out_normals, std::vector<glm::vec2>& out_uvs, std::vector<glm::vec3>& out_tangents, std::vector<glm::vec3>& out_bitangents);

struct GPURenderOptions {
	bool shadowMapping = true;
	bool environmentMapping = true;
	bool drawSkybox = true;

	bool forcePhong = false;
	bool forcePBR = false;

	bool iblPrefiltered = true;
	bool iblRealtime = false;


	std::unordered_map<unsigned int, struct GLR_Texture*> textures = {};
	std::unordered_map<unsigned int, struct GLR_Material*> materials = {};
	std::unordered_map<std::string, GLR_MeshData*> meshData = {};
	std::unordered_map<std::string, GLR_Mesh*> meshes = {};
	std::map<std::string, std::string> shaderDefines = {};
	std::unordered_map<std::string, struct GLR_ShaderProgram*> shaderPrograms = {};

	GLR_Texture& addTexture(const Texture& tex) {
		auto itr = textures.find(tex.id.id);
		if (itr == textures.end()) {
			itr = textures.emplace(tex.id.id, new GLR_Texture(&tex)).first;

			// upload texture to GPU
			// todo: layers
			GLR_Texture& gl_tex = *itr->second;
			gl_tex.genTexture();
			gl_tex.uploadTexture(tex.images[0]);
		}
		return *itr->second;
	}

	GLenum texSlot = 0;

	GLint resetTextures() {
		texSlot = 0;
		return 0;
	}

	GLint bindTexture(const GLR_Texture& tex) {
		GLenum usedSlot = texSlot;
		tex.bindTexture(usedSlot);
		texSlot++;
		return usedSlot;
	}

	bool hasShader(
		const std::map<std::string, std::string>& defines,
		const std::string& vs, const std::string& fs,
		const std::string& additions = "",
		const std::string& gs = "") {

		std::string shaderId = generateShaderProgramId({ &defines, &shaderDefines }, vs, fs, gs);

		auto itr = shaderPrograms.find(shaderId);

		// if shader hasn't been loaded, load it
		if (itr == shaderPrograms.end()) {
			return false;
		}
		else {
			return true;
		}

	}

	GLR_ShaderProgram& addShader(
		const std::map<std::string, std::string>& defines,
		const std::string& vs, const std::string& fs,
		const std::string& additions = "",
		const std::string& gs = "") {

		std::string shaderId = generateShaderProgramId({ &defines, &shaderDefines }, vs, fs, gs);

		auto itr = shaderPrograms.find(shaderId);

		// if shader hasn't been loaded, load it
		if (itr == shaderPrograms.end()) {
			if (gs != "") {
				auto res = shaderPrograms.emplace(shaderId, new GLR_ShaderProgram(
					preprocessShader(vs, shaderId, defines),
					preprocessShader(fs, shaderId, defines, additions),
					preprocessShader(gs, shaderId, defines)));
				itr = res.first;
			}
			else {
				auto res = shaderPrograms.emplace(shaderId, new GLR_ShaderProgram(
					preprocessShader(vs, shaderId, defines),
					preprocessShader(fs, shaderId, defines, additions)));
				itr = res.first;
			}

			// assign uniform locations
			GLR_ShaderProgram& shader = *itr->second;
			shader.locPerspective = shader.getUniformLocation("Perspective");
			shader.locModelView = shader.getUniformLocation("ModelView");
			shader.locInverseView = shader.getUniformLocation("InverseView");
			shader.locNormalMatrix = shader.getUniformLocation("NormalMatrix");
			int i = 0;
			for (i = 0; i < 50; i++) {
				std::string lightidx = "light[" + std::to_string(i) + "]";
				shader.lights[i].locPosition = shader.getUniformLocation((lightidx + ".position").c_str());
				shader.lights[i].locColor = shader.getUniformLocation((lightidx + ".colour").c_str());
				shader.lights[i].locAttenuation = shader.getUniformLocation((lightidx + ".attenuation").c_str());
				i++;
			}
			shader.locActiveLights = shader.getUniformLocation("activeLights");
			shader.locAmbientIntensity = shader.getUniformLocation("ambientIntensity");
			shader.locEnvironmentMap = shader.getUniformLocation("environment_map");

		}

		return *itr->second;
	}

	GLR_Material& addMaterial(Material& material) {
		auto itr = materials.find(material.id.id);
		if (itr == materials.end()) {
			itr = materials.emplace(material.id.id, GLR_makeMaterial(material)).first;

			// upload texture to GPU
			// todo: layers
			GLR_Material& gl_mat = *itr->second;
		}
		return *itr->second;
	}

	GLR_Mesh& addMesh(Primitive& primitive) {
		decltype(meshes)::iterator itr;

		if (dynamic_cast<Mesh*>(&primitive)) {
			Mesh& prim = (Mesh&)primitive;
			itr = meshes.find(prim.m_name);
			if (itr == meshes.end()) {
				GLR_MeshData& mesh_data = *(new GLR_MeshData{});
				GLR_Mesh& mesh = *(new GLR_Mesh{});

				if (prim.m_normals.size() == 0) {
					prim.generateNormals();
				}

				// extract mesh data
				for (auto face : prim.m_faces) {
					for (auto v : { face.v1, face.v2, face.v3 }) {
						mesh_data.vertices.push_back(prim.m_vertices[v]);
						mesh_data.normals.push_back(prim.m_normals[v]);
					}
				}

				if (prim.m_uvMap.size() == prim.m_faces.size()) {
					for (auto& face : prim.m_uvMap) {
						for (auto v : { face.v1, face.v2, face.v3 }) {
							mesh_data.uv.push_back(prim.m_uvs[v]);
							mesh_data.tangent.push_back(prim.m_tangents[v]);
							mesh_data.bitangent.push_back(prim.m_bitangents[v]);
						}
					}
				}
				else {
					for (auto& face : prim.m_faces) {
						for (auto v : { face.v1, face.v2, face.v3 }) {
							mesh_data.uv.push_back(glm::vec2(0.0, 0.0));
							mesh_data.tangent.push_back(glm::vec3(0.0, 0.0, 0.0));
							mesh_data.bitangent.push_back(glm::vec3(0.0, 0.0, 0.0));
						}
					}
				}

				mesh.vertexCount = mesh_data.vertices.size();

				// upload to GPU
				glGenBuffers(1, &mesh.vbo_coord);
				glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo_coord);
				glBufferData(GL_ARRAY_BUFFER, mesh_data.vertices.size() * sizeof(mesh_data.vertices[0]), mesh_data.vertices.data(), GL_STATIC_DRAW);

				glGenBuffers(1, &mesh.vbo_normal);
				glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo_normal);
				glBufferData(GL_ARRAY_BUFFER, mesh_data.normals.size() * sizeof(mesh_data.normals[0]), mesh_data.normals.data(), GL_STATIC_DRAW);

				glGenBuffers(1, &mesh.vbo_uv);
				glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo_uv);
				glBufferData(GL_ARRAY_BUFFER, mesh_data.uv.size() * sizeof(mesh_data.uv[0]), mesh_data.uv.data(), GL_STATIC_DRAW);

				glGenBuffers(1, &mesh.vbo_tan);
				glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo_tan);
				glBufferData(GL_ARRAY_BUFFER, mesh_data.tangent.size() * sizeof(mesh_data.tangent[0]), mesh_data.tangent.data(), GL_STATIC_DRAW);

				glGenBuffers(1, &mesh.vbo_bitan);
				glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo_bitan);
				glBufferData(GL_ARRAY_BUFFER, mesh_data.bitangent.size() * sizeof(mesh_data.bitangent[0]), mesh_data.bitangent.data(), GL_STATIC_DRAW);

				glGenVertexArrays(1, &mesh.vao);
				glBindVertexArray(mesh.vao);

				glEnableVertexAttribArray(0);
				glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo_coord);
				glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

				glEnableVertexAttribArray(1);
				glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo_normal);
				glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);

				glEnableVertexAttribArray(2);
				glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo_uv);
				glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, 0);

				glEnableVertexAttribArray(3);
				glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo_tan);
				glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 0, 0);

				glEnableVertexAttribArray(4);
				glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo_bitan);
				glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, 0, 0);

				glBindVertexArray(0);
				glBindBuffer(GL_ARRAY_BUFFER, 0);

				meshData.emplace(prim.m_name, &mesh_data);
				itr = meshes.emplace(prim.m_name, &mesh).first;
			}
		}
		else if (dynamic_cast<Sphere*>(&primitive) || dynamic_cast<NonhierSphere*>(&primitive)) {
			itr = meshes.find("sphere");
			if (itr == meshes.end()) {
				GLR_MeshData& sphere_data = *(new GLR_MeshData{});
				GLR_Mesh& sphere_mesh = *(new GLR_Mesh{});

				generateSphere(sphere_mesh.vertexCount, sphere_data.vertices, sphere_data.normals, sphere_data.uv, sphere_data.tangent, sphere_data.bitangent);

				// upload to GPU
				glGenBuffers(1, &sphere_mesh.vbo_coord);
				glBindBuffer(GL_ARRAY_BUFFER, sphere_mesh.vbo_coord);
				glBufferData(GL_ARRAY_BUFFER, sphere_data.vertices.size() * sizeof(sphere_data.vertices[0]), sphere_data.vertices.data(), GL_STATIC_DRAW);

				glGenBuffers(1, &sphere_mesh.vbo_normal);
				glBindBuffer(GL_ARRAY_BUFFER, sphere_mesh.vbo_normal);
				glBufferData(GL_ARRAY_BUFFER, sphere_data.normals.size() * sizeof(sphere_data.normals[0]), sphere_data.normals.data(), GL_STATIC_DRAW);

				glGenBuffers(1, &sphere_mesh.vbo_uv);
				glBindBuffer(GL_ARRAY_BUFFER, sphere_mesh.vbo_uv);
				glBufferData(GL_ARRAY_BUFFER, sphere_data.uv.size() * sizeof(sphere_data.uv[0]), sphere_data.uv.data(), GL_STATIC_DRAW);

				glGenBuffers(1, &sphere_mesh.vbo_tan);
				glBindBuffer(GL_ARRAY_BUFFER, sphere_mesh.vbo_tan);
				glBufferData(GL_ARRAY_BUFFER, sphere_data.tangent.size() * sizeof(sphere_data.tangent[0]), sphere_data.tangent.data(), GL_STATIC_DRAW);

				glGenBuffers(1, &sphere_mesh.vbo_bitan);
				glBindBuffer(GL_ARRAY_BUFFER, sphere_mesh.vbo_bitan);
				glBufferData(GL_ARRAY_BUFFER, sphere_data.bitangent.size() * sizeof(sphere_data.bitangent[0]), sphere_data.bitangent.data(), GL_STATIC_DRAW);

				glGenVertexArrays(1, &sphere_mesh.vao);
				glBindVertexArray(sphere_mesh.vao);

				glEnableVertexAttribArray(0);
				glBindBuffer(GL_ARRAY_BUFFER, sphere_mesh.vbo_coord);
				glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

				glEnableVertexAttribArray(1);
				glBindBuffer(GL_ARRAY_BUFFER, sphere_mesh.vbo_normal);
				glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);

				glEnableVertexAttribArray(2);
				glBindBuffer(GL_ARRAY_BUFFER, sphere_mesh.vbo_uv);
				glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, 0);

				glEnableVertexAttribArray(3);
				glBindBuffer(GL_ARRAY_BUFFER, sphere_mesh.vbo_tan);
				glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 0, 0);

				glEnableVertexAttribArray(4);
				glBindBuffer(GL_ARRAY_BUFFER, sphere_mesh.vbo_bitan);
				glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, 0, 0);

				glBindVertexArray(0);
				glBindBuffer(GL_ARRAY_BUFFER, 0);

				meshData.emplace("sphere", &sphere_data);
				itr = meshes.emplace("sphere", &sphere_mesh).first;
			}
		}
		else { // if (dynamic_cast<Cube*>(&primitive) || dynamic_cast<NonhierBox*>(&primitive)) {
			itr = meshes.find("box");
			if (itr == meshes.end()) {
				GLR_MeshData& box_data = *(new GLR_MeshData{});
				GLR_Mesh& box_mesh = *(new GLR_Mesh{});

				generateCube(box_mesh.vertexCount, box_data.vertices, box_data.normals, box_data.uv, box_data.tangent, box_data.bitangent);

				// upload to GPU
				glGenBuffers(1, &box_mesh.vbo_coord);
				glBindBuffer(GL_ARRAY_BUFFER, box_mesh.vbo_coord);
				glBufferData(GL_ARRAY_BUFFER, box_data.vertices.size() * sizeof(box_data.vertices[0]), box_data.vertices.data(), GL_STATIC_DRAW);

				glGenBuffers(1, &box_mesh.vbo_normal);
				glBindBuffer(GL_ARRAY_BUFFER, box_mesh.vbo_normal);
				glBufferData(GL_ARRAY_BUFFER, box_data.normals.size() * sizeof(box_data.normals[0]), box_data.normals.data(), GL_STATIC_DRAW);

				glGenBuffers(1, &box_mesh.vbo_uv);
				glBindBuffer(GL_ARRAY_BUFFER, box_mesh.vbo_uv);
				glBufferData(GL_ARRAY_BUFFER, box_data.uv.size() * sizeof(box_data.uv[0]), box_data.uv.data(), GL_STATIC_DRAW);

				glGenBuffers(1, &box_mesh.vbo_tan);
				glBindBuffer(GL_ARRAY_BUFFER, box_mesh.vbo_tan);
				glBufferData(GL_ARRAY_BUFFER, box_data.tangent.size() * sizeof(box_data.tangent[0]), box_data.tangent.data(), GL_STATIC_DRAW);

				glGenBuffers(1, &box_mesh.vbo_bitan);
				glBindBuffer(GL_ARRAY_BUFFER, box_mesh.vbo_bitan);
				glBufferData(GL_ARRAY_BUFFER, box_data.bitangent.size() * sizeof(box_data.bitangent[0]), box_data.bitangent.data(), GL_STATIC_DRAW);

				glGenVertexArrays(1, &box_mesh.vao);
				glBindVertexArray(box_mesh.vao);

				glEnableVertexAttribArray(0);
				glBindBuffer(GL_ARRAY_BUFFER, box_mesh.vbo_coord);
				glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

				glEnableVertexAttribArray(1);
				glBindBuffer(GL_ARRAY_BUFFER, box_mesh.vbo_normal);
				glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);

				glEnableVertexAttribArray(2);
				glBindBuffer(GL_ARRAY_BUFFER, box_mesh.vbo_uv);
				glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, 0);

				glEnableVertexAttribArray(3);
				glBindBuffer(GL_ARRAY_BUFFER, box_mesh.vbo_tan);
				glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 0, 0);

				glEnableVertexAttribArray(4);
				glBindBuffer(GL_ARRAY_BUFFER, box_mesh.vbo_bitan);
				glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, 0, 0);

				glBindVertexArray(0);
				glBindBuffer(GL_ARRAY_BUFFER, 0);

				meshData.emplace("box", &box_data);
				itr = meshes.emplace("box", &box_mesh).first;
			}
		}

		return *itr->second;
	}

	GPURenderOptions& operator=(const GPURenderOptions& other) {
		shadowMapping = other.shadowMapping;
		environmentMapping = other.environmentMapping;
		drawSkybox = other.drawSkybox;

		forcePhong = other.forcePhong;
		forcePBR = other.forcePBR;

		iblPrefiltered = other.iblPrefiltered;
		iblRealtime = other.iblRealtime;

		return *this;
	}
};

static std::string makeFuncName(const std::string name) {
	std::string funcname = name;
	funcname[0] = toupper(funcname[0]);
	funcname = "get" + funcname;
	return funcname;
}

std::string removeSpaces(std::string input)
{
	input.erase(std::remove(input.begin(), input.end(), ' '), input.end());
	return input;
}

template <typename T>
struct GLR_T_MaterialValue : public GLR_MaterialValue {
	MaterialValue* m_src;

	inline T* mat() const { return static_cast<T*>(m_src); }
	virtual std::string id() const override { return removeSpaces(typeid(T).name()); }
};

struct GLR_UnassignedMaterialValue : public GLR_T_MaterialValue<UnassignedMaterialValue> {
	std::string m_name;
	GLR_UnassignedMaterialValue(UnassignedMaterialValue* mat, const std::string& name) : m_name(name) { m_src = mat; }
	virtual std::string getShaderCode(struct GPURenderOptions& options) const override {
		std::string funcname = makeFuncName(m_name);
		return
			"vec3 " + funcname + "() {\n" +
			"    return vec3(1.0, 0.0, 1.0);\n" +
			"}\n"
			;
	}

	virtual void uploadUniformsToGPU(GPURenderOptions& options, GLR_ShaderProgram& shader) const override
	{
	}
};

struct GLR_ColorMaterialValue : public GLR_T_MaterialValue<ColorMaterialValue> {
	std::string m_name;
	GLR_ColorMaterialValue(ColorMaterialValue* mat, const std::string& name) : m_name(name) { m_src = mat; }
	virtual std::string getShaderCode(struct GPURenderOptions& options) const override {
		std::string funcname = makeFuncName(m_name);
		return
			"uniform vec3 " + m_name + ";\n" +
			"\n"
			"vec3 " + funcname + "() {\n" +
			"    return " + m_name + ";\n" +
			"}\n"
			;
	}

	virtual void uploadUniformsToGPU(GPURenderOptions& options, GLR_ShaderProgram& shader) const override
	{
		GLint location;
		location = shader.getUniformLocation(m_name);
		glUniform3fv(location, 1, glm::value_ptr(mat()->color));
	}

};

struct GLR_TextureMaterialValue : public GLR_T_MaterialValue<TextureMaterialValue> {
	std::string m_name;
	GLR_TextureMaterialValue(TextureMaterialValue* mat, const std::string& name) : m_name(name) { m_src = mat; }

	virtual std::string getShaderCode(struct GPURenderOptions& options) const override {
		std::string funcname = makeFuncName(m_name);
		return
			"uniform sampler2D " + m_name + ";\n" +
			"\n"
			"vec3 " + funcname + "() {\n" +
			"    return texture(" + m_name + ", fs_in.uv).rgb;\n" +
			"}\n"
			;
	}

	virtual void uploadUniformsToGPU(GPURenderOptions& options, GLR_ShaderProgram& shader) const override
	{
		GLint location;
		location = shader.getUniformLocation(m_name);
		GLR_Texture& kd_tex = options.addTexture(mat()->tex);
		glUniform1i(location, options.bindTexture(kd_tex));
	}
};

struct GLR_TextureNormalMaterialValue : public GLR_T_MaterialValue<TextureNormalMaterialValue> {
	std::string m_name;
	GLR_TextureNormalMaterialValue(TextureNormalMaterialValue* mat, const std::string& name) : m_name(name) { m_src = mat; }

	virtual std::string getShaderCode(struct GPURenderOptions& options) const override {
		std::string funcname = makeFuncName(m_name);
		return
			"uniform sampler2D " + m_name + ";\n" +
			"\n"
			"vec3 " + funcname + "() {\n" +
			"    vec3 tex_normal_ = texture(" + m_name + ", fs_in.uv).rgb * 2 - 1;\n" +
			"    vec3 normal_ = normalize(fs_in.normal_ES);\n" +
			"    vec3 tangent_ = fs_in.tangent_ES;\n" +
			"    vec3 bitangent_ = fs_in.bitangent_ES;\n" +
			"    return normalize(normal_*tex_normal_.z + tangent_*tex_normal_.x + bitangent_*tex_normal_.y);\n" +
			"}\n"
			;
	}

	virtual void uploadUniformsToGPU(GPURenderOptions& options, GLR_ShaderProgram& shader) const override
	{
		GLint location;
		location = shader.getUniformLocation(m_name);
		GLR_Texture& kd_tex = options.addTexture(mat()->tex);
		glUniform1i(location, options.bindTexture(kd_tex));
	}
};

struct GLR_MeshNormalMaterialValue : public GLR_T_MaterialValue<MeshNormalMaterialValue> {
	std::string m_name;
	GLR_MeshNormalMaterialValue(MeshNormalMaterialValue* mat, const std::string& name) : m_name(name) { m_src = mat; }
	virtual std::string getShaderCode(struct GPURenderOptions& options) const override {
		std::string funcname = makeFuncName(m_name);
		return
			"uniform vec3 " + m_name + ";\n" +
			"\n"
			"vec3 " + funcname + "() {\n" +
			"    return normalize(fs_in.normal_ES);\n" +
			"}\n"
			;
	}

	virtual void uploadUniformsToGPU(GPURenderOptions& options, GLR_ShaderProgram& shader) const override
	{
	}

};

GLR_MaterialValue* GLR_makeMaterialValue(MaterialValue& mat, const std::string &name) {
	if (dynamic_cast<TextureMaterialValue*>(&mat)) {
		return new GLR_TextureMaterialValue{ (TextureMaterialValue*)&mat, name };
	}
	else if (dynamic_cast<ColorMaterialValue*>(&mat)) {
		return new GLR_ColorMaterialValue{ (ColorMaterialValue*)&mat, name };
	}
	else if (dynamic_cast<TextureNormalMaterialValue*>(&mat)) {
		return new GLR_TextureNormalMaterialValue{ (TextureNormalMaterialValue*)&mat, name };
	}
	else if (dynamic_cast<MeshNormalMaterialValue*>(&mat)) {
		return new GLR_MeshNormalMaterialValue{ (MeshNormalMaterialValue*)&mat, name };
	}
	else {
		std::cerr << "unimplemented material type for: " << name << "\n";
		return new GLR_UnassignedMaterialValue{ (UnassignedMaterialValue*)&mat, name };
	}
}

template <typename T>
struct GLR_T_Material : public GLR_Material {
public:
	inline T* mat() const { return static_cast<T*>(m_src); }
	inline std::string id() const { return typeid(T).name(); }
};

#include "PhongMaterial.h"

struct GLR_PhongMaterial : public GLR_T_Material<PhongMaterial> {
	std::vector<GLR_MaterialValue*> m_values;
	GLR_PhongMaterial(PhongMaterial* mat) { 
		m_src = mat; 
		m_values.push_back(GLR_makeMaterialValue(*mat->m_kd, "kd"));
		m_values.push_back(GLR_makeMaterialValue(*mat->m_ks, "ks"));
		m_values.push_back(GLR_makeMaterialValue(*mat->m_norm, "normal"));
	}
	virtual GLR_ShaderProgram& reserveShader(GPURenderOptions& options) const override;
	virtual void uploadUniformsToGPU(GPURenderOptions& options, GLR_ShaderProgram& shader) const override;
};

struct GLR_PBRMaterial : public GLR_T_Material<PBRMaterial> {
	std::vector<GLR_MaterialValue*> m_values;
	GLR_PBRMaterial(PBRMaterial* mat) { 
		m_src = mat;
		m_values.push_back(GLR_makeMaterialValue(*mat->m_kd, "kd"));
		m_values.push_back(GLR_makeMaterialValue(*mat->m_norm, "normal"));
	}
	virtual GLR_ShaderProgram& reserveShader(GPURenderOptions& options) const override;
	virtual void uploadUniformsToGPU(GPURenderOptions& options, GLR_ShaderProgram& shader) const override;
};


GLR_Material* GLR_makeMaterial(Material& mat) {
  if (dynamic_cast<PhongMaterial*>(&mat)) {
		return new GLR_PhongMaterial{ (PhongMaterial*)&mat };
	}
	else if (dynamic_cast<PBRMaterial*>(&mat)) {
		return new GLR_PBRMaterial{ (PBRMaterial*)&mat };
	}
}

GLR_ShaderProgram& GLR_PhongMaterial::reserveShader(GPURenderOptions& options) const {
	std::map<std::string, std::string> defines = { };
	for (auto& mat : m_values) {
		defines.emplace(mat->id(), "");
	}

	//TODO: hack here, improve system for tagging + adding shaders
	if (!options.hasShader(defines, "PhongBaseShader.vs", "PhongBaseShader.fs")) {
		std::string additions = "";
		for (auto& mat : m_values) {
			additions += mat->getShaderCode(options);
		}
		return options.addShader(defines, "PhongBaseShader.vs", "PhongBaseShader.fs", additions);
	}
	else {
		return options.addShader(defines, "PhongBaseShader.vs", "PhongBaseShader.fs");
	}
}

void GLR_PhongMaterial::uploadUniformsToGPU(GPURenderOptions& options, GLR_ShaderProgram& shader) const
{
	for (auto& mat : m_values) {
		mat->uploadUniformsToGPU(options, shader);
	}

	GLint location = shader.getUniformLocation("material.shininess");
	glUniform1f(location, mat()->m_shininess);
	//TODO: CHECK FOR ERRORS
};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

GLR_ShaderProgram& GLR_PBRMaterial::reserveShader(GPURenderOptions& options) const {
	std::map<std::string, std::string> defines = { };
	for (auto& mat : m_values) {
		defines.emplace(mat->id(), "");
	}

	//TODO: hack here, improve system for tagging + adding shaders
	if (!options.hasShader(defines, "PBRBaseShader.vs", "PBRBaseShader.fs")) {
		std::string additions = "";
		for (auto& mat : m_values) {
			additions += mat->getShaderCode(options);
		}
		return options.addShader(defines, "PBRBaseShader.vs", "PBRBaseShader.fs", additions);
	}
	else {
		return options.addShader(defines, "PBRBaseShader.vs", "PBRBaseShader.fs");
	}
}

void GLR_PBRMaterial::uploadUniformsToGPU(GPURenderOptions& options, GLR_ShaderProgram& shader) const
{
	for (auto& mat : m_values) {
		mat->uploadUniformsToGPU(options, shader);
	}

	GLint location;

	location = shader.getUniformLocation("material.metal");
	glUniform1f(location, mat()->m_metal);
	//TODO: CHECK FOR ERRORS

	location = shader.getUniformLocation("material.roughness");
	glUniform1f(location, mat()->m_roughness);
	//TODO: CHECK FOR ERRORS
};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

static GPURenderOptions glr_render = GPURenderOptions{};

void GLR_gatherMeshes(SceneNode* node) {
	if (node->m_nodeType == NodeType::GeometryNode) {
		GeometryNode* geom = static_cast<GeometryNode*>(node);
		glr_render.addMesh(*geom->m_primitive);
	}

	for (SceneNode* child : node->children) {
		GLR_gatherMeshes(child);
	}
}

static std::list<Light*>* current_lights = nullptr;
static glm::vec3 ambientIntensity = { 0,0,0 };
static float global_fov = 50;

static GLuint s_cubeMapId = 0;

static void GLR_updateGlobalUniforms(
	GLR_ShaderProgram& shader,
	const glm::mat4& modelMatrix,
	const glm::mat4& viewMatrix
) {

	glUseProgram(shader.m_gl_id);
	GLint location;

	location = shader.locPerspective;
	glm::mat4 projection = glm::perspective(glm::radians(global_fov), 1.f, 0.1f, 2000.f);
	glUniformMatrix4fv(location, 1, GL_FALSE, value_ptr(projection));

	location = shader.locModelView;
	glm::mat4 modelView = viewMatrix * modelMatrix;
	glUniformMatrix4fv(location, 1, GL_FALSE, value_ptr(modelView));

	location = shader.locInverseView;
	glm::mat3 inverseView = glm::inverse(glm::mat3(viewMatrix));
	glUniformMatrix3fv(location, 1, GL_FALSE, value_ptr(inverseView));

	location = shader.locNormalMatrix;
	glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(modelView)));
	glUniformMatrix3fv(location, 1, GL_FALSE, value_ptr(normalMatrix));

	int i = 0;
#if 1 // todo: CPU bottleneck here: should be updating shared info once per frame for each shader
	for (auto light : *current_lights) {
		if (i >= 50) {
			break;
		}

		location = shader.lights[i].locPosition;
		glUniform3fv(location, 1, value_ptr(glm::vec3(viewMatrix * glm::vec4(light->position, 1.0))));

		location = shader.lights[i].locColor;
		glUniform3fv(location, 1, value_ptr(light->colour));

		location = shader.lights[i].locAttenuation;
		glUniform1fv(location, 3, light->falloff);
		// todo: check for errors

		i++;
	}
#endif
	location = shader.locActiveLights;
	glUniform1i(location, i);

	location = shader.locAmbientIntensity;
	glUniform3fv(location, 1, glm::value_ptr(ambientIntensity));

	//todo: replace with call to options.bindTexture()
	GLenum usedSlot = glr_render.texSlot;
	glActiveTexture(GL_TEXTURE0 + usedSlot);
	glBindTexture(GL_TEXTURE_CUBE_MAP, s_cubeMapId);
	glr_render.texSlot++;

	location = shader.locEnvironmentMap;
	glUniform1i(location, usedSlot);

}


// gather the things that need to be rendered
// to build a render graph 
// right now, runs with the assumption that
// todo: (simultaneously gather assets?)
void GLR_gatherRenderables(SceneNode* node, const glm::mat4& view, const glm::mat4& parentTransform = glm::mat4(1.0)) {
	// todo: first render pass (no shadow maps yet)
	glm::mat4 nodeTransform = parentTransform * node->localTransform();

	// todo: maybe handle encountering a BVH node as well...
	if (node->m_nodeType == NodeType::LightNode) {
	}
	else if (node->m_nodeType == NodeType::GeometryNode) {
		GeometryNode* geom = static_cast<GeometryNode*>(node);
		GLR_Mesh& mesh = glr_render.addMesh(*geom->m_primitive);

		// adjust transform for model
		if (dynamic_cast<NonhierBox*>(geom->m_primitive)) {
			NonhierBox* box = (NonhierBox*)geom->m_primitive;
			nodeTransform = nodeTransform
				* glm::translate(box->min())
				* glm::scale(glm::vec3(box->size()));
		}
		else if (dynamic_cast<NonhierSphere*>(geom->m_primitive)) {
			NonhierSphere* sphere = (NonhierSphere*)geom->m_primitive;
			nodeTransform = nodeTransform
				* glm::translate(sphere->getPos())
				* glm::scale(glm::vec3(sphere->getRad()));
		}

		GLR_Material& material = glr_render.addMaterial(*geom->m_material);
		GLR_ShaderProgram& shader = material.reserveShader(glr_render);

		glUseProgram(shader.m_gl_id);

		glr_render.resetTextures();
		material.uploadUniformsToGPU(glr_render, shader);
		GLR_updateGlobalUniforms(shader, nodeTransform, view);

		//todo: brdf lookups

		glBindVertexArray(mesh.vao);
		glDrawArrays(GL_TRIANGLES, 0, mesh.vertexCount);
	}
	else if (node->m_nodeType == NodeType::ParticleSystemNode) {
	}
	else if (node->m_nodeType == NodeType::JointNode) {
		// nothing to do?
	}

	for (SceneNode* child : node->children) {
		GLR_gatherRenderables(child, view, nodeTransform);
	}
}

void GLR_drawSkybox(const glm::mat4& view, const glm::mat4& parentTransform = glm::mat4(1.0)) {
	NonhierBox skyboxPrim({ -1, -1, -1 }, { 2, 2, 2 });
	GLR_Mesh& mesh = glr_render.addMesh(skyboxPrim);

	// adjust transform for model
	glm::mat4 nodeTransform =
		glm::translate(skyboxPrim.min())
		* glm::scale(glm::vec3(skyboxPrim.size()));

	//GLR_Material& material = glr_render.addMaterial(*geom->m_material);
	GLR_ShaderProgram& shader = glr_render.addShader({}, "skyboxShader.vs", "skyboxShaderGradient.fs");

	glUseProgram(shader.m_gl_id);

	glr_render.resetTextures();
	GLR_updateGlobalUniforms(shader, nodeTransform, view);

	glBindVertexArray(mesh.vao);
	glDrawArrays(GL_TRIANGLES, 0, mesh.vertexCount);
}

#if 1
int GLR_renderToCubemap(SceneNode* root
	, int w, int h
	, const glm::vec3& eye
	, const glm::vec3& view
	, const glm::vec3& up
	, double fovy
	, const glm::vec3& ambient
	, const std::list<Light*>& lights
) {
	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

	static int dim = 64;
	static int levels = 1;

	static GLuint depth;

	// generate the cubemap
	if (s_cubeMapId == 0) {
		glGenTextures(1, &s_cubeMapId);
		glBindTexture(GL_TEXTURE_CUBE_MAP, s_cubeMapId);
		// intel GPU has problems with 32F being a framebuffer target...
		glTexStorage2D(GL_TEXTURE_CUBE_MAP, 1, GL_RGB16F, dim, dim);

		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_LEVEL, 0);

		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		// todo: Check for GL errors;

		glGenTextures(1, &depth);
		glBindTexture(GL_TEXTURE_2D, depth);
		glTexStorage2D(GL_TEXTURE_2D, 1, GL_DEPTH_COMPONENT24, dim, dim);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	}

	// Render skybox into cubemap
	{
		GLuint tempFbo;
		glm::vec3 pos = view;
		glGenFramebuffers(1, &tempFbo);

		// -X, +X, -Y, +Y, -Z, +Z 
		glm::mat4 projection = glm::perspective(glm::radians(90.f), 1.f, 0.1f, 10.f);
		glm::mat4 views[] = {
			glm::lookAt(pos, pos + glm::vec3(-1, 0, 0), glm::vec3(0,-1, 0)),
			glm::lookAt(pos, pos + glm::vec3(1, 0, 0), glm::vec3(0,-1, 0)),
			glm::lookAt(pos, pos + glm::vec3(0,-1, 0), glm::vec3(0, 0,-1)),
			glm::lookAt(pos, pos + glm::vec3(0, 1, 0), glm::vec3(0, 0, 1)),
			glm::lookAt(pos, pos + glm::vec3(0, 0,-1), glm::vec3(0,-1, 0)),
			glm::lookAt(pos, pos + glm::vec3(0, 0, 1), glm::vec3(0,-1, 0))
		};
		GLenum targets[] = {
			GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
			GL_TEXTURE_CUBE_MAP_POSITIVE_X,
			GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
			GL_TEXTURE_CUBE_MAP_POSITIVE_Y,
			GL_TEXTURE_CUBE_MAP_NEGATIVE_Z,
			GL_TEXTURE_CUBE_MAP_POSITIVE_Z,
		};

		// do the draws
		glBindFramebuffer(GL_FRAMEBUFFER, tempFbo);

		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depth, 0);

		int faces = sizeof(views) / sizeof(views[0]);
		for (int i = 0; i < faces; i++) {
			for (int mip = 0; mip < levels; mip++) {
				unsigned int cur_dim = dim / glm::pow(2, mip);
				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, targets[i], s_cubeMapId, mip);

				global_fov = 90;
				glViewport(0, 0, cur_dim, cur_dim);
				// todo: check for gl errors;

				glClear(GL_DEPTH_BUFFER_BIT);

				// skybox first, because no depth buffer in framebuffer
				glCullFace(GL_FRONT);
				glDepthFunc(GL_LEQUAL);
				glEnable(GL_DEPTH_CLAMP);
				GLR_drawSkybox(views[i]);

				//glCullFace(GL_BACK);
				//glDepthFunc(GL_LEQUAL);
				//GLR_gatherRenderables(root, views[i]);
			}
		}

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glDeleteFramebuffers(1, &tempFbo);
		// todo: check for gl errors;
	}

	// profit
	return 0;
}
#endif

int GLR_renderToWindow(SceneNode* root
	, int w, int h
	, const glm::vec3& eye
	, const glm::vec3& view
	, const glm::vec3& up
	, double fovy
	, const glm::vec3& ambient
	, const std::list<Light*>& lights
) {
	Camera cam(eye, view, up, fovy, w, h);
	ambientIntensity = ambient;
	global_fov = fovy;

	// gather all lights from inside the hierarchy
	std::vector<Light> hierarchal_lights;
	gatherLightNodes(hierarchal_lights, root);

	std::list<Light*> complete_lights;
	for (Light& light : hierarchal_lights) {
		complete_lights.push_back(&light);
	}
	for (Light* light : lights) {
		complete_lights.push_back(light);
	}

	//todo: this is obviously bad
	current_lights = &complete_lights;

	SceneNode* sceneToRender = root;
	glm::mat4 mat = glm::lookAt(eye, view, up);

	GLR_renderToCubemap(root, w, h, eye, view, up, 90, ambient, lights);

	global_fov = fovy;
	glViewport(0, 0, w, h);

	glCullFace(GL_BACK);
	glDepthFunc(GL_LEQUAL);
	GLR_gatherRenderables(root, mat);

	glCullFace(GL_FRONT);
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_DEPTH_CLAMP);
	GLR_drawSkybox(mat);

	return 0;
}