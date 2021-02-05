
#include "RenderRaytracer.h"

#include <glm/glm.hpp>
#include <glm/gtx/scalar_multiplication.hpp>

#include <vector>
#include <iostream>
#include <iomanip>
#include <sstream>

#include "Scene.h"
#include "Material.h"
#include "Image.h"
#include "BVHTree.h"
#include "FlatSceneNode.h"

// Helper Classes

class ProgressBar {
	ScopedTimer timer;
public:
	ProgressBar() {}

	void update(float progress)
	{
		int resolution = 50;
		int fill = progress * resolution;

		clock_t time = clock() - timer.startTime();
		clock_t seconds = time / CLOCKS_PER_SEC;

		static int lastFill = -1;
		static float lastProgress = -1;
		static int lastSeconds = -1;

		if (fill != lastFill ||
			lastSeconds != seconds ||
			std::abs(progress - lastProgress) > 1.f / resolution) {
			lastFill = fill;
			lastProgress = progress;
			lastSeconds = seconds;

			std::stringstream progressBar;

			progressBar << std::internal << std::setw(2)
				<< std::fixed << std::setprecision(0) << std::setfill('0')
				<< progress * 100 << "% ";
			progressBar << "[";

			int i = 1;
			for (; i <= fill; i++) {
				progressBar << "=";
			} for (; i <= resolution; i++) {
				progressBar << " ";
			}
			progressBar << "]";

			progressBar << " "
				<< std::setw(2) << std::setfill('0') <<
				seconds / 60 / 60 << std::setw(1) << ":" << std::setw(2) <<
				(seconds / 60) % 60 << std::setw(1) << ":" << std::setw(2) <<
				seconds % 60 << std::setw(1);

			std::cout << "\r" << progressBar.str() << std::flush;
		}
	}

	~ProgressBar() {
		std::cout << std::endl;
	}

};


#include <atomic>
#include <condition_variable>
#include <mutex>
#include <functional>

class semaphore {
	std::mutex m;
	std::condition_variable cv;
	uint32_t counter = 0;

public:
	semaphore(unsigned int counter = 1) : counter(counter) { }

	void acquire()
	{
		std::unique_lock<std::mutex> lock(m);
		while (counter == 0)
		{
			cv.wait(lock);
		}
		counter--;
	}

	void release()
	{
		std::unique_lock<std::mutex> lock(m);
		counter++;
		cv.notify_one();
	}
};

template<typename T, unsigned int size>
class mt_buffer_single_producer
{
	std::mutex m;
	semaphore filled;
	semaphore empty;
	std::array<T, size> data;

	unsigned int write_idx = 0;
	unsigned int read_idx = 0;

public:
	mt_buffer_single_producer() : filled(0), empty(size) {}

	void produce(T item)
	{
		empty.acquire();

		data[write_idx] = item;
		write_idx = (write_idx + 1) % size;

		filled.release();
	}

	T consume()
	{
		T result;

		filled.acquire();
		m.lock();

		result = data[read_idx];
		read_idx = (read_idx + 1) % size;

		m.unlock();
		empty.release();
		return result;
	}
};

// helper functions

bool intersectScene(IntersectResult& out_result,
	SceneNode* node,
	const Ray& r,
	double min_t, double max_t) {
	glm::mat4 nodeTransform = node->localTransform();
	glm::mat4 nodeInv = glm::inverse(nodeTransform);

	Ray local_r = nodeInv * r;

	bool hasIntersection = false;

	if (node->m_nodeType == NodeType::GeometryNode) {
		GeometryNode* geom = static_cast<GeometryNode*>(node);

		if (geom->m_primitive->intersect(out_result, local_r, min_t, max_t)) {
			out_result.material = geom->m_material;
			max_t = out_result.t;
			hasIntersection = true;
		}
	}
// todo: add intersect function to geometry node ... so special case isn't needed ... 
#if 1
	else if (node->m_nodeType == NodeType::FlattenedSceneNode) {
		FlattenedSceneNode* flat = static_cast<FlattenedSceneNode*>(node);
		if (flat->intersect(out_result, local_r, min_t, max_t)) {
			max_t = out_result.t;
			hasIntersection = true;
		}
	}
#endif

	for (SceneNode* child : node->children) {
		if (intersectScene(out_result, child, local_r, min_t, max_t)) {
			max_t = out_result.t;
			hasIntersection = true;
		}
	}

	if (hasIntersection) {
		glm::mat3 invTransform(glm::transpose(glm::mat3(nodeInv)));
		out_result.position = glm::vec3(nodeTransform * glm::vec4(out_result.position, 1));
		out_result.normal = glm::vec3(invTransform * out_result.normal);
		out_result.tangent = glm::vec3(invTransform * out_result.tangent);
		out_result.bitangent = glm::vec3(invTransform * out_result.bitangent);
	}

	return hasIntersection;
}

glm::vec3 backgroundColor(const glm::vec3& dir, double attenuation) {
#if 0
	static std::mutex m;
	if (envmap.images.empty()) {
		std::unique_lock<std::mutex> lock(m);
		if (envmap.images.empty()) {
			LoadEnvironmentMap_cpu(envMapPath);
		}
	}

	return envmap(dir, 0);

#else
	glm::vec3 d = glm::normalize(dir);
	glm::vec3 c1 = glm::vec3(0, 0, 1);
	glm::vec3 c2 = 0.25f * glm::vec3(1, 0.4, 0.9);

	float mix = glm::dot(d, glm::normalize(glm::vec3(0, -1, 0)));
	mix = glm::max(0.f, mix * 4.f / 3.f + 1.f / 3.f);

	c1 = glm::mix(c2, c1, mix);

	glm::vec3 result = c1;

	return attenuation * glm::max(result, glm::vec3(0, 0, 0));
#endif
}

struct DebugInfo {
	int rayDepth = 0;

	int x = 0;
	int y = 0;
};

thread_local static DebugInfo debug;

glm::vec3 rayColor(SceneNode* root,
	const glm::vec3& ambient, const std::list<Light*>& lights,
	const CameraRay& r,
	double attenuation) {

	IntersectResult res;
	glm::vec3 color = glm::vec3(1, 1, 1);
	debug.rayDepth++;

	if (debug.rayDepth > 10) {
		return glm::vec3(1, 0, 1);
		color = attenuation * color;
	}
	else if (intersectScene(res, root, r.r)) {
		color = res.material->shade(root, ambient, lights, r, &res, attenuation);
	}
	else {
		color = backgroundColor(r.r.l, attenuation);
	}

	debug.rayDepth--;
	return color;
}

static void gatherLightNodes(std::vector<Light>& out_lights, SceneNode* node, const glm::mat4& parentTransform = glm::mat4(1.0)) {
	glm::mat4 nodeTransform = parentTransform * node->localTransform();

	// todo: maybe handle encountering a BVH node as well...
	if (node->m_nodeType == NodeType::LightNode) {

		LightNode* light_node = static_cast<LightNode*>(node);
		out_lights.emplace_back(*light_node->m_light);
		out_lights.back().position = glm::vec3(nodeTransform * glm::vec4(light_node->m_light->position, 1.f));
	}

	for (SceneNode* child : node->children) {
		gatherLightNodes(out_lights, child, nodeTransform);
	}
}


class Camera /* : public SceneNode */ {
	glm::vec3 eye;
	glm::vec3 view;
	glm::vec3 up;

	uint32_t width;
	uint32_t height;

	// helpers for ray generation
	glm::vec3 cam_dir;
	glm::vec3 d_v;
	glm::vec3 x_v;
	glm::vec3 y_v;

	double fovy;
	double aspect;
	float half_height;
	float half_width;

	glm::vec3 bottom_left;
	glm::vec3 top_left;

	double pixel_width;
	double pixel_height;

public:
	Camera(const glm::vec3& eye, const glm::vec3& view, const glm::vec3& up, double fovy = 90, uint32_t width = 256, uint32_t height = 256)
		: eye(eye)
		, view(view)
		, up(up)
		, fovy(fovy)
		, width(width)
		, height(height)
	{
		double aspect = double(height) / double(width);

		cam_dir = view - eye;
		d_v = glm::normalize(cam_dir);
		x_v = glm::normalize(glm::cross(d_v, up));
		y_v = glm::normalize(glm::cross(x_v, d_v));

		half_height = tan(glm::radians(fovy / 2.0));
		half_width = half_height / aspect;

		pixel_width = 2.0 * half_width / double(width);
		pixel_height = 2.0 * half_height / double(height);

		bottom_left = eye + d_v
			- half_width * x_v
			- half_height * y_v;

		top_left = eye + d_v
			- half_width * x_v
			+ half_height * y_v;
	}

	CameraRay getRay(uint32_t x, uint32_t y) {
		// note: add 0.5 so that ray is in pixel center
		double s = (0.5 + double(x)) / double(width);
		double t = (0.5 + double(y)) / double(height);

		double s_n = double(0.5 + x + ((x % 2) ? 1 : -1)) / double(width);
		double t_n = double(0.5 + y + ((y % 2) ? 1 : -1)) / double(height);

		double diff_x = 1.0 / width;
		double diff_y = 1.0 / height;

		assert(s_n != s);
		assert(t_n != t);

		glm::vec3 frag_dir = top_left
			+ x_v * (2.0 * half_width * s)
			- y_v * (2.0 * half_height * t)
			- eye;


		glm::vec3 d_x = top_left
			+ x_v * (2.0 * half_width * s_n)
			- y_v * (2.0 * half_height * t)
			- eye;

		glm::vec3 d_y = top_left
			+ x_v * (2.0 * half_width * s)
			- y_v * (2.0 * half_height * t_n)
			- eye;

		CameraRay ray(Ray{ eye, frag_dir });
		ray.dx = Ray{ eye, d_x };
		ray.dy = Ray{ eye, d_y };
		ray.hasDifferentials = true;

		return ray;
	}

	CameraRay getRay(double s, double t) {
		return getRay(
			glm::clamp(static_cast<uint32_t>(s * width - 0.5), 0u, width - 1),
			glm::clamp(static_cast<uint32_t>(t * height - 0.5), 0u, height - 1)
		);
	}

};


// main go
#include "GL/glew.h"
#include "SDL.h"
#include "glm/glm.hpp"
#include "glm/gtx/transform.hpp"
#include "glm/gtc/type_ptr.hpp"

#include <iostream>
#include <chrono>
#include <thread>


#include "shader_utils.h"

int renderToWindow(
	SceneNode* root,
	Image& image,
	const glm::vec3& eye,
	const glm::vec3& view,
	const glm::vec3& up,
	double fovy,
	const glm::vec3& ambient,
	const std::list<Light*>& lights
) {
	SDL_Window* window = nullptr;
	SDL_GLContext gl_context = NULL;

	// ASSUMING SDL_INIT Already Called ... */

	window = SDL_CreateWindow("Render Display", 100, 100, 640, 480, SDL_WINDOW_OPENGL);
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
	
#if 1
	// glew (must be done with OpenGL context)
	glewExperimental = true; // needed for some extensions
	GLenum glew_err = glewInit();
	if (GLEW_OK != glew_err) {
		SDL_Log("Unable to initialize GLEW: %s", glewGetErrorString(glew_err));
		return 1;
	}
#endif

	GLuint render_target_tex = 0xffffffff;
	unsigned int num_levels = ceil(log2(glm::max(image.width(), image.height()))) + 1;

	// generate result render target
	glGenTextures(1, &render_target_tex);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, render_target_tex);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);

	// setup shader program
	shaderInfo shaders[] =
	{ {GL_VERTEX_SHADER, "screenquad.vs"},
	{GL_FRAGMENT_SHADER, "screenquad.fs" }
};
	GLuint shader_program = LoadProgram(shaders, 2);
	GLint uniform = glGetUniformLocation(shader_program, "image");
	glUseProgram(shader_program);
	glUniform1i(uniform, 0);

	// render scene
	RenderRaytracer(root, image, eye, view, up, fovy, ambient, lights);

	// draw to screen
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, image.w, image.h, 0, GL_RGB, GL_FLOAT, image.data);
	//glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 1, 1, GL_RGB, GL_FLOAT, &color);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
	glDrawArrays(GL_TRIANGLES, 0, 6);

	SDL_GL_SwapWindow(window);

#if 0
	std::this_thread::sleep_for(std::chrono::milliseconds(5000));

	// openGL context
	SDL_GL_DeleteContext(gl_context);

	// SDL
	SDL_DestroyWindow(window);
#endif
}

void RenderRaytracer(
		SceneNode * root,
		Image & image,
		const glm::vec3 & eye,
		const glm::vec3 & view,
		const glm::vec3 & up,
		double fovy,
		const glm::vec3 & ambient,
		const std::list<Light *> & lights
	){
	size_t h = image.height();
	size_t w = image.width();

	Camera cam(eye, view, up, fovy, w, h);

	// gather all lights from inside the hierarchy

	std::vector<Light> hierarchal_lights;
	gatherLightNodes(hierarchal_lights, root);

	std::list<Light*> complete_lights(lights);
	for (Light &light : hierarchal_lights) {
		complete_lights.push_back(&light);
	}

	SceneNode* sceneToRender = root;

//todo: bvh trees
	FlattenedSceneNode* rootBVH = nullptr;

#if 0 // ndef DISABLE_SCENE_BVH 
	{
		ScopedTimer t("Building Scene BVH", 5);
		rootBVH = new FlattenedSceneNode(root);
		sceneToRender = rootBVH;
	}
#else
	std::cout << ("BVH Generation Disabled for Scene") << std::endl;
#endif

	ScopedTimer renderTimer("Render Time");
	ProgressBar progressBar;

	const unsigned int max_threads = 8;
	std::array<std::thread, max_threads> threads;

	mt_buffer_single_producer<std::function<bool()>, max_threads * 2> queue;
	std::atomic_int finished = 0;

	auto worker_thread = [&]() {
			while ((queue.consume())()) {}
	};

	for (int i = 0; i < max_threads; i++)
	{
			threads[i] = std::thread(worker_thread);
	}

	for (uint32_t y = 0; y < h; ++y) {
		for (uint32_t x = 0; x < w; ++x) {

			auto pixel_func = [=, &cam, &image, &finished]() {

				CameraRay ray = cam.getRay(x, y);

				debug = DebugInfo();
				debug.x = x;
				debug.y = y;

				glm::vec3 frag_color = rayColor(sceneToRender, ambient, complete_lights, ray);
				image(x, y) = frag_color;

				finished++;
				return true;
			};

			queue.produce(pixel_func);

			// progress bar
			float progress = float(y * w + x + 1 - max_threads) / float(w * h);
			progressBar.update(progress);

		}
	}

	for (int i = 0; i < max_threads; i++)
	{
			queue.produce([]() {return false; });
	}
	for (int i = 0; i < max_threads; i++)
	{
			threads[i].join();
			float progress = float(w * h - max_threads + i) / float(w * h);
			progressBar.update(progress);
			std::cout << std::flush << "\r";
	}

	std::cout << std::endl;

	delete rootBVH;
}

void RenderRaytracer_ST(
	SceneNode* root,
	Image& image,
	const glm::vec3& eye,
	const glm::vec3& view,
	const glm::vec3& up,
	double fovy,
	const glm::vec3& ambient,
	const std::list<Light*>& lights
) {
	size_t h = image.height();
	size_t w = image.width();

	Camera cam(eye, view, up, fovy, w, h);

	// gather all lights from inside the hierarchy
	std::vector<Light> hierarchal_lights;
	gatherLightNodes(hierarchal_lights, root);

	std::list<Light*> complete_lights(lights);
	for (Light& light : hierarchal_lights) {
		complete_lights.push_back(&light);
	}

	SceneNode* sceneToRender = root;

	FlattenedSceneNode* rootBVH = nullptr;

#ifndef DISABLE_SCENE_BVH
	{
		ScopedTimer t("Building Scene BVH", 5);
		rootBVH = new FlattenedSceneNode(root);
		sceneToRender = rootBVH;
	}
#else
	std::cout << ("BVH Generation Disabled for Scene") << std::endl;
#endif

	ScopedTimer renderTimer("Render Time");
	ProgressBar progressBar;

	for (uint32_t y = 0; y < h; ++y) {
		for (uint32_t x = 0; x < w; ++x) {

			CameraRay ray = cam.getRay(x, y);

			glm::vec3 frag_color = rayColor(sceneToRender, ambient, complete_lights, ray);
			image(x, y) = frag_color;

			// progress bar
			float progress = float(y * w + x) / float(w * h);
			progressBar.update(progress);

		}
	}

	delete rootBVH;
}
