#include <stdio.h>
#include <conio.h>
#include <Python.h>

#include <iostream>
#include <fstream>
#include <sstream>//allows you to make streams from strings, saves code in .obj file reading
#include <string>
#include <vector>
#include <math.h>
#include <list>

#include "GL/glew.h"
#include "SDL.h"
#include "glm/glm.hpp"
#include "glm/gtx/transform.hpp"
#include "glm/gtc/type_ptr.hpp"

// imgui
#include "imgui.h"

#include "backends/imgui_impl_sdl.h"
#include "backends/imgui_impl_sdl.cpp"

#include "backends/imgui_impl_opengl3.h"
#include "backends/imgui_impl_opengl3.cpp"


// stb
#include "stb_image.h"
#include "stb_image_write.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#undef STB_IMAGE_IMPLEMENTATION

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"""
#undef STB_IMAGE_WRITE_IMPLEMENTATION

//custom headers
#include "PROJECT_OPTIONS.h"
#include "shader_utils.h"

#include "Scene.h"

using namespace std;

struct RenderableMesh {
private:
		GLuint vbo_coord;
		GLuint ibo_elements;
		GLuint vao;

		unsigned int vertexCount;
		unsigned int elementCount;

		friend class RenderingDevice;
};

struct Mesh {
		std::vector<float> vertices;
		std::vector<unsigned int> indices;
		std::vector<float> uv;
		std::vector<float> colors;
};

struct GPUMaterial {
public:
		enum materialType {
				NONE,
				BLINN_PHONG,
		} type = NONE;
};

// do materials need to be a class?
// could just be a few name-value dictionaries + some shader code
struct BlinnPhongMaterial : GPUMaterial {
		BlinnPhongMaterial(glm::vec3 color = glm::vec3(0, 0, 0),
				glm::vec3 specular = glm::vec3(0, 0, 0),
				glm::vec3 emissive = glm::vec3(0, 0, 0),
				float shininess = 0) {
				this->color = color;
				this->specular = specular;
				this->emissive = emissive;
				this->shininess = shininess;
				type = BLINN_PHONG;
		}

		glm::vec3 color;
		glm::vec3 specular;
		glm::vec3 emissive;
		float shininess;
};

class Model {
public:
		std::string name;

		RenderableMesh rd_mesh;
		std::unique_ptr<GPUMaterial> material = std::unique_ptr<GPUMaterial>(new GPUMaterial());

		std::vector<float> vertices;
		std::vector<unsigned int> indices;

		glm::mat4 transform;

		glm::vec3 location = glm::vec3(0, 0, 0);
		glm::vec3 rotation = glm::vec3(0, 0, 0);

		void prepare() {
				transform = glm::translate(location) *
						glm::rotate(rotation.y, glm::vec3(0, 1, 0)) *
						glm::rotate(rotation.x, glm::vec3(1, 0, 0)) *
						glm::rotate(rotation.z, glm::vec3(0, 0, 1));

		}
};

// 
class RenderingDevice {
public:
		struct BasicShader {
				GLuint program;
				GLuint uniform_matrix;
				GLuint uniform_cameraMatrix;
		};

		BasicShader debugShader;
		BasicShader blinnPhongShader;

		GLuint program;
		struct shaderInfo shaders[2] = { { GL_VERTEX_SHADER, "vs.glsl" },
											{ GL_FRAGMENT_SHADER, "fs.glsl"} };
		GLuint uniform_matrix;
		GLuint uniform_cameraMatrix;

		glm::mat4 camera_matrix;

		// rendering device owns and tracks all rendering resources
		// Ownership here is purely to track resources, not intended as a representation of the scene
		std::vector<RenderableMesh> meshes;

		RenderingDevice() {

		}

		RenderableMesh createRenderableMesh(const std::vector<float> vertices, const std::vector<unsigned int> elements) {
				meshes.emplace_back();
				RenderableMesh& mesh = meshes.back();

				mesh.vertexCount = vertices.size();
				mesh.elementCount = elements.size();

				glGenBuffers(1, &mesh.vbo_coord);
				glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo_coord);
				glBufferData(GL_ARRAY_BUFFER, sizeof(vertices[0]) * vertices.size(), vertices.data(), GL_STATIC_DRAW);

				glGenBuffers(1, &mesh.ibo_elements);
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.ibo_elements);
				glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(elements[0]) * elements.size(), elements.data(), GL_STATIC_DRAW);

				//setting up the vertex attribute/arrays
				glGenVertexArrays(1, &mesh.vao);
				glBindVertexArray(mesh.vao);

				glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo_coord);
				GLint attrib_vcoord = getAttrib(program, "coord");
				glVertexAttribPointer(
						attrib_vcoord,	//attrib 'index'
						3,				//pieces of data per index
						GL_FLOAT,		//type of data
						GL_FALSE,		//whether to normalize
						0,				//space b\w data
						0				//offset from buffer start
				);
				glEnableVertexAttribArray(attrib_vcoord);

				glBindVertexArray(0);

				return meshes.back();
		}

		void destroyRenderableMesh(const RenderableMesh& mesh) {

				glDeleteBuffers(1, &mesh.vbo_coord);
				glDeleteBuffers(1, &mesh.ibo_elements);
		}

		void startFrame() {
				glClearColor(1.f, 1.f, 1.f, 1.f);
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

				glUseProgram(program);
		}

		void setCameraMatrix(glm::mat4 matrix) {
				glUniformMatrix4fv(uniform_cameraMatrix, 1, GL_FALSE, glm::value_ptr(matrix));
				camera_matrix = matrix;
		}

		void drawMesh(const RenderableMesh& mesh, const GPUMaterial& material, const glm::mat4 transform) {
				switch (material.type) {
				case GPUMaterial::NONE: {
						glUseProgram(debugShader.program);
						glUniformMatrix4fv(debugShader.uniform_cameraMatrix, 1, GL_FALSE, glm::value_ptr(camera_matrix));
						glUniformMatrix4fv(debugShader.uniform_matrix, 1, GL_FALSE, glm::value_ptr(transform));

				}
													 break;
				case GPUMaterial::BLINN_PHONG: {
						const BlinnPhongMaterial& m = *reinterpret_cast<const BlinnPhongMaterial*>(&material);
						glUseProgram(blinnPhongShader.program);
						glUniformMatrix4fv(debugShader.uniform_cameraMatrix, 1, GL_FALSE, glm::value_ptr(camera_matrix));
						glUniformMatrix4fv(blinnPhongShader.uniform_matrix, 1, GL_FALSE, glm::value_ptr(transform));

						glUniform3fv(glGetUniformLocation(blinnPhongShader.program, "diffuse"), 1, glm::value_ptr(m.color));
						glUniform3fv(glGetUniformLocation(blinnPhongShader.program, "specular"), 1, glm::value_ptr(m.specular));
						glUniform3fv(glGetUniformLocation(blinnPhongShader.program, "emissive"), 1, glm::value_ptr(m.emissive));
						glUniform1fv(glGetUniformLocation(blinnPhongShader.program, "shininess"), 1, &(m.shininess));
				}
																	break;
				};

				glBindVertexArray(mesh.vao);
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.ibo_elements);
				glDrawElements(GL_TRIANGLES, mesh.elementCount, GL_UNSIGNED_INT, NULL);
		}


		void endFrame() {
				glFlush();
		}

		void init() {
				// initialize GL resources the rendering device needs

				// shaders
				struct shaderInfo normalShaders[2] = { { GL_VERTEX_SHADER, "vs.glsl" },
										{ GL_FRAGMENT_SHADER, "fs.glsl"} };
				debugShader.program = LoadProgram(shaders, 2);
				debugShader.uniform_matrix = getUniform(debugShader.program, "modelMatrix");
				debugShader.uniform_cameraMatrix = getUniform(debugShader.program, "cameraMatrix");

				struct shaderInfo blinnPhongShaders[] = { { GL_VERTEX_SHADER, "BlinnPhong.vs" },
														 { GL_FRAGMENT_SHADER, "BlinnPhong.fs"} };
				blinnPhongShader.program = LoadProgram(blinnPhongShaders, 2);
				blinnPhongShader.uniform_matrix = getUniform(blinnPhongShader.program, "modelMatrix");
				blinnPhongShader.uniform_cameraMatrix = getUniform(blinnPhongShader.program, "cameraMatrix");

				// gl state
				glEnable(GL_DEPTH_TEST);
		}

		void cleanUp() {
				glDeleteProgram(program);
				for (const auto& mesh : meshes) {
						destroyRenderableMesh(mesh);
				}
		}
};

unsigned int timeLast, timeCurr;
RenderingDevice rd;
std::vector<Model> scene;
glm::mat4 viewProj;

bool loadObj(vector <float>& vertices, vector <unsigned int>& elements, const char* fileName) {
		ifstream file;
		//istringstream sline;
		string line;
		string token;

		int in_int;
		float in_float;

		string full_filename = string(RESOURCE_DIR "/") + string(fileName);
		file.open(full_filename);
		if (!file) {
				cerr << "Error loading .obj: " << fileName << endl;
				return false;
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

						std::vector<int> face;

						while (sline >> token) {
								string temp;

								if (token.find("/") != -1) {
										// ignore parameters other than the first, the position
										face.push_back(atoi(token.substr(0, token.find("/")).c_str()));
								}
								else {
										face.push_back(atoi(token.c_str()));
								}
						}

						if (face.size() == 3) {
								for (int i : {0, 1, 2}) {
										elements.push_back(face[i] - 1);
								}
						}
						else if (face.size() == 4) {
								for (int i : {0, 1, 2, 2, 3, 0}) {
										elements.push_back(face[i] - 1);
								}
						}
						// else discard;

				}
		}

		return true;
}


bool init() {
		rd.init();

		timeLast = SDL_GetTicks();
		return true;
}

float camera_yaw = 0.f;
float camera_pitch = 0.f;
float camera_truck = 10.0;
glm::mat4 camera_matrix{ 1.f };

glm::vec3 center{ 0.f, 0.f, 0.f };

std::ostream& operator<<(std::ostream& o, glm::vec3 v) {
		o << v.x << ", " << v.y << ", " << v.z;
		return o;
}

void idle() {
		//updating the matrics (i.e. animation) done in the idle function

		timeCurr = SDL_GetTicks();

		//view matrix: transforms world coordinates into relative-the-camera view-space coordinates
		glm::mat4 view = glm::lookAt(glm::vec3(0.0, 0.0, -4.0), glm::vec3(0.0, 0.0, 0.0), glm::vec3(0.0, 1.0, 0.0));
		camera_matrix = glm::rotate(camera_pitch, glm::vec3(1.f, 0.f, 0.f)) * glm::rotate(camera_yaw, glm::vec3(0.f, 1.f, 0.f));
		view = glm::translate(glm::vec3{ 0.f, 0.f, -camera_truck }) * camera_matrix * glm::translate(-center);

		//projection matrix: transforms view space coordinates into 2d screen coordinates
		glm::mat4 projection = glm::perspective(45.0f, 1.0f * 600 / 480, 0.1f, 100.0f);

		viewProj = projection * view;
}

void display() {
		rd.startFrame();
		rd.setCameraMatrix(viewProj);
		for (auto& model : scene) {
				model.prepare();
		}

		for (const auto& model : scene) {
				rd.drawMesh(model.rd_mesh, *model.material, model.transform);
		}

		rd.endFrame();
}

void gui() {
		static float f = 0.0f;
		static int counter = 0;
		static bool show_demo_window = false;
		static bool show_another_window = false;
		static float clear_color[3] = { 0.f, 0.f, 1.f };
		static int modelToEdit = -1;
		static bool showLoadFileDialog = false;
		static char textInput[256] = {};
		static string errorMessage = "";

		ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.

		ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
		ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state
		ImGui::Checkbox("Another Window", &show_another_window);

		ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
		ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color

		if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
				counter++;
		ImGui::SameLine();
		ImGui::Text("counter = %d", counter);

		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
		ImGui::End();


		// Object list
		if (ImGui::Begin("Object List")) {
				for (int i = 0; i < scene.size(); i++) {
						ImGui::PushID(i);
						Model& model = scene[i];
						if (ImGui::Button("edit")) {
								modelToEdit = i; // should add a dialog to the scene instead
						}
						ImGui::SameLine();
						ImGui::Text(model.name.c_str());
						ImGui::PopID();
				}
		}
		ImGui::End();

		if (modelToEdit >= 0) {
				if (ImGui::Begin("Edit")) {
						Model& model = scene[modelToEdit];

						ImGui::Text(model.name.c_str());
						ImGui::NewLine();

						ImGui::Text("Location");
						ImGui::PushItemWidth(ImGui::GetWindowWidth() / 4);
						ImGui::InputFloat("x", &model.location.x, 0.1);
						ImGui::SameLine();
						ImGui::InputFloat("y", &model.location.y, 0.1);
						ImGui::SameLine();
						ImGui::InputFloat("z", &model.location.z, 0.1);
						ImGui::PopItemWidth();

						ImGui::Text("Rotation");
						ImGui::PushItemWidth(ImGui::GetWindowWidth() / 4);
						ImGui::InputFloat("yaw", &model.rotation.x, glm::one_over_two_pi<float>());
						ImGui::SameLine();
						ImGui::InputFloat("pitch", &model.rotation.y, glm::one_over_two_pi<float>());
						ImGui::SameLine();
						ImGui::InputFloat("roll", &model.rotation.z, glm::one_over_two_pi<float>());
						ImGui::PopItemWidth();

						if (ImGui::Button("done")) {
								modelToEdit = -1;
						}
				}
				ImGui::End();
		}

		bool loadFile = false;
		std::string fileToLoad = "";

		// menu bar
		if (ImGui::BeginMainMenuBar()) {
				if (ImGui::BeginMenu("File")) {
						if (ImGui::MenuItem("Load File")) {
								memset(textInput, '\0', sizeof(textInput));
								showLoadFileDialog = true;
						}
						ImGui::EndMenu();
				}
				ImGui::EndMainMenuBar();
		}

		// load file dialog
		if (showLoadFileDialog) {
				ImGui::Begin("Load File", &showLoadFileDialog);
				ImGui::Text("Load file from resource dir");
				if (ImGui::InputText("file: ", textInput, sizeof(textInput), ImGuiInputTextFlags_EnterReturnsTrue)) {
						fileToLoad = std::string(textInput);
						loadFile = true;
				}
				if (ImGui::Button("open")) {
						fileToLoad = std::string(textInput);
						loadFile = true;
				}
				if (errorMessage.length() > 0) {
						ImGui::TextColored({ 1, 0, 0, 1 }, errorMessage.c_str());
				}
				ImGui::End();

				// handle the file loading ; xxx maybe move out
				if (loadFile) {
						scene.emplace_back();
						Model& newModel = scene.back();

						std::vector<float>& vertices = newModel.vertices;
						std::vector<unsigned int>& indices = newModel.indices;

						bool success = loadObj(vertices, indices, fileToLoad.c_str());
						if (success) {
								showLoadFileDialog = false;
								newModel.rd_mesh = rd.createRenderableMesh(newModel.vertices, newModel.indices);
								newModel.transform = glm::mat4(1.f);
								newModel.name = fileToLoad;
								newModel.material = std::unique_ptr<GPUMaterial>(new BlinnPhongMaterial{ glm::vec3{1, 0, 0}, glm::vec3{1, 1, 1}, glm::vec3{0.1, 0, 0}, 1 });
						}
						else {
								scene.pop_back();
								errorMessage = "failed to load file";
						}

						loadFile = false;
				}
		}
}

void free_res() {
		rd.cleanUp();
}

// Image.hpp

// app.hpp

// main.hpp
class SDLInputHandler {
		public:
				virtual bool handleEvent(SDL_Event& baseEvent) {
						switch (baseEvent.type) {
						case SDL_KEYDOWN:
						case SDL_KEYUP:
						{
								SDL_KeyboardEvent& event = baseEvent.key;
								return onKeyPress(event);
								break;
						}
						case SDL_MOUSEBUTTONDOWN:
						case SDL_MOUSEBUTTONUP:
						{
								SDL_MouseButtonEvent& event = baseEvent.button;
								return onMouseButton(event);
								break;
						}
						case SDL_MOUSEMOTION:
						{
								SDL_MouseMotionEvent& event = baseEvent.motion;
								return onMouseMove(event);
								break;
						}
						case SDL_MOUSEWHEEL:
						{
								SDL_MouseWheelEvent& event = baseEvent.wheel;
								return onMouseWheel(event);
								break;
						}
						case SDL_CONTROLLERBUTTONDOWN:
						case SDL_CONTROLLERBUTTONUP:
						{
								SDL_ControllerButtonEvent& event = baseEvent.cbutton;
								return onControllerButton(event);
								break;
						}
						default:
								return false;
						}
				}

				virtual bool onMouseMove(const SDL_MouseMotionEvent& event) { return false; };
				virtual bool onMouseWheel(const SDL_MouseWheelEvent& event) { return false; };

				virtual bool onKeyPress(const SDL_KeyboardEvent& event) { return false; };
				virtual bool onMouseButton(const SDL_MouseButtonEvent& event) { return false; };
				virtual bool onControllerButton(const SDL_ControllerButtonEvent& event) { return false; };
};


SceneNode* defaultScene();

int main(int argc, char* argv[]) {
		SDL_Window* window = nullptr;
		SDL_GLContext gl_context = NULL;
		std::string window_title = "GL_RenderDoodle";

		// initialize libraries
		//	SDL
		if (0 != SDL_Init(SDL_INIT_VIDEO)) {
				SDL_Log("Unable to initialize SDL: %s", SDL_GetError());
				return 1;
		}
		window = SDL_CreateWindow("Test Display", 100, 100, 640, 480, SDL_WINDOW_OPENGL);
		if (!window) {
				SDL_Log("Unable to create window");
				return 1;
		}

		// openGL context
		gl_context = SDL_GL_CreateContext(window);
		if (!gl_context) {
				SDL_Log("Unable to create GL context");
				return 1;
		}
		cout << glGetString(GL_VERSION) << endl;

		// glew (must be done with OpenGL context)
		glewExperimental = true; // needed for some extensions
		GLenum glew_err = glewInit();
		if (GLEW_OK != glew_err) {
				SDL_Log("Unable to initialize GLEW: %s", glewGetErrorString(glew_err));
				return 1;
		}

		// Python
		PyObject* pInt;
		Py_Initialize();
		PyRun_SimpleString("print('Hello World from Embedded Python!!!')");


		// init dear-imgui
		IMGUI_CHECKVERSION();
		ImGuiContext* imgui_ctx = ImGui::CreateContext();
		ImGuiIO& imgui_io = ImGui::GetIO();
		ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
		ImGui_ImplOpenGL3_Init("#version 330");


		// setup scene 
		defaultScene();

		SDL_GL_MakeCurrent(window, gl_context);

		// init app
		init();

		// APP MAIN LOOP
		bool run = true;
		while (run) {
				//polling for events
				SDL_Event event;
				while (SDL_PollEvent(&event)) {
						ImGui_ImplSDL2_ProcessEvent(&event);
						if (event.type == SDL_QUIT) run = false;

						if (!imgui_io.WantCaptureMouse 
								/* && !io_2.WantCaptureMouse*/
								) {
								// xxx todo: make input handler class
								static bool b_down[3] = { false, false };
								static int b_x_last[3] = { 0, 0 };
								static int b_y_last[3] = { 0, 0 };

								if (event.type == SDL_MOUSEBUTTONDOWN) {
										int b_index = event.button.button - 1;
										b_down[b_index] = true;
										b_x_last[b_index] = event.button.x;
										b_y_last[b_index] = event.button.y;
								}
								else if (event.type == SDL_MOUSEBUTTONUP) {
										int b_index = event.button.button - 1;
										b_down[b_index] = false;
								}
								else if (event.type == SDL_MOUSEMOTION) {
										if (b_down[0]) {
												center -= float(event.motion.xrel) * 0.01f * glm::vec3(glm::inverse(camera_matrix)[0]);
												center += float(event.motion.yrel) * 0.01f * glm::vec3(glm::inverse(camera_matrix)[1]);
										}
										if (b_down[2]) {
												camera_yaw += float(event.motion.xrel) * 0.01;
												camera_pitch += float(event.motion.yrel) * 0.01;
										}
								}
								else if (event.type == SDL_MOUSEWHEEL) {
										camera_truck -= event.wheel.y;
										camera_truck = std::abs(camera_truck);
								}
						}

						if (!imgui_io.WantCaptureKeyboard) {

						}
				}

				// per-frame tick
				idle();
#if 0
				SDL_GL_MakeCurrent(window, gl_context);
				ImGui::SetCurrentContext(imgui_ctx_1);
#endif
				// draw scene
				display();

				// begin ui_frame
				ImGui_ImplOpenGL3_NewFrame();
				ImGui_ImplSDL2_NewFrame(window);
				ImGui::NewFrame();

				// IMGUI
				gui();

				// draw ui
				ImGui::Render();
				ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

				// swap buffer
				SDL_GL_SwapWindow(window);
		}

		// clean application resources
		free_res();

		// Shutdown libraries
		// imgui
		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplSDL2_Shutdown();
		ImGui::DestroyContext();

		// python
		Py_Finalize();

		// openGL context
		SDL_GL_DeleteContext(gl_context);

		// SDL
		SDL_DestroyWindow(window);

		return 0;
}