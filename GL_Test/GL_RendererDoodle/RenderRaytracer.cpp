
#include "RenderRaytracer.h"

#include <glm/glm.hpp>
#include <glm/gtx/scalar_multiplication.hpp>

#include "GL/glew.h"
#include "SDL.h"

#include <vector>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <unordered_map>

#include "Scene.h"
#include "SceneUtil.h"
#include "Material.h"
#include "Image.h"
#include "BVHTree.h"
#include "FlatSceneNode.h"
#include "Camera.h"

#include "RenderOpenGL.h" // for GLR_Texture
#include "shader_utils.h"

#include "Asset.h"

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

//todo: divide into per-thread work buffers instead of single work buffer: may get perf improvement
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

		// note: no mutex locking, since this is a single producer buffer

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

	window = SDL_CreateWindow("Render Display", 100, 100, 640, 640, SDL_WINDOW_OPENGL);
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

	SDL_GL_MakeCurrent(window, gl_context);

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

	Texture renderTarget(image, false); //todo: generating mipmaps here crashes?
	GLR_Texture GPU_renderTarget(&renderTarget);
	GPU_renderTarget.genTexture();

	Image progressImage(image.w, image.h);
	// todo: zero progress image
	Texture progressTarget(progressImage, false);
	GLR_Texture GPU_progressTarget(&progressTarget);
	GPU_progressTarget.genTexture();

	// setup shader program
	shaderInfo shaders[] =
	{ {GL_VERTEX_SHADER, "screenquad.vs"},
	{GL_FRAGMENT_SHADER, "screenquadv2.fs" }
	};
	GLuint shader_program = LoadProgram(shaders, 2);
	glUseProgram(shader_program);

	GLint uniform = glGetUniformLocation(shader_program, "image");
	glUniform1i(uniform, 0);

	uniform = glGetUniformLocation(shader_program, "progressImage");
	glUniform1i(uniform, 1);
	
	uniform = glGetUniformLocation(shader_program, "width");
	glUniform1i(uniform, image.w);

	uniform = glGetUniformLocation(shader_program, "height");
	glUniform1i(uniform, image.h);


	// render scene
	// RenderRaytracer(root, image, eye, view, up, fovy, ambient, lights);

	bool render_done = false;
	auto render_work = [&]() {
		RenderRaytracer(root, image, eye, view, up, fovy, ambient, lights, &progressImage);
		render_done = true;
	};
	std::thread render_worker(render_work);


	// NOTE: need to acknowledge any window events that happened during render
	// , or Windows won't update the window surface
	while (!render_done) {
		{
			SDL_Event event;
			while (SDL_PollEvent(&event)) {
				// printf("EVENT 0x%x", event.type);
				///if (event.type == SDL_WINDOWEVENT) {
				// 	printf(" window 0x%x : 0x%x", event.window.windowID, event.window.event);
				// }
				// printf("\n");
			}
		}

		// draw to screen
		GPU_renderTarget.uploadTexture(image);
		GPU_progressTarget.uploadTexture(progressImage);

		GPU_renderTarget.bindTexture(0);	
		GPU_progressTarget.bindTexture(1);

		glClearColor(0, 0, 0, 1);
		glClear(GL_COLOR_BUFFER_BIT);

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glDrawArrays(GL_TRIANGLES, 0, 6);

		SDL_GL_SwapWindow(window);

		glFinish();
	}

	getGLErrors();

	render_worker.join();

#if 0


	// openGL context
	SDL_GL_DeleteContext(gl_context);

	// SDL
	SDL_DestroyWindow(window);
#endif
}

int renderToWindowv1(
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

	window = SDL_CreateWindow("Render Display", 100, 100, 640, 640, SDL_WINDOW_OPENGL);
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

	SDL_GL_MakeCurrent(window, gl_context);

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

	Texture renderTarget(image, false); //todo: generating mipmaps here crashes?
	GLR_Texture GPU_renderTarget(&renderTarget);

	// generate result render target
	GPU_renderTarget.genTexture();

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
	// RenderRaytracer(root, image, eye, view, up, fovy, ambient, lights);
	
	bool render_done = false;
	auto render_work = [&]() {
		RenderRaytracer(root, image, eye, view, up, fovy, ambient, lights);
		render_done = true;
	};
	std::thread render_worker(render_work);


	// NOTE: need to acknowledge any window events that happened during render
	// , or Windows won't update the window surface
	while (!render_done) {
		{
			SDL_Event event;
			while (SDL_PollEvent(&event)) {
				// printf("EVENT 0x%x", event.type);
				///if (event.type == SDL_WINDOWEVENT) {
				// 	printf(" window 0x%x : 0x%x", event.window.windowID, event.window.event);
				// }
				// printf("\n");
			}
		}

		// draw to screen
		GPU_renderTarget.uploadTexture(image);
		GPU_renderTarget.bindTexture(0);
		//glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 1, 1, GL_RGB, GL_FLOAT, &color);

		glClearColor(0, 0, 0, 1);
		glClear(GL_COLOR_BUFFER_BIT);

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glDrawArrays(GL_TRIANGLES, 0, 6);

		SDL_GL_SwapWindow(window);

		glFinish();
	}

	render_worker.join();

	getGLErrors();

#if 0
	

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
		const std::list<Light *> & lights,
		Image* progressImage
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

	// generate a list of all indices
	std::vector<size_t> indices(h * w);
	size_t ci = 0;
	for (size_t& i : indices) { i = ci; ci++; }
	std::random_shuffle(indices.begin(), indices.end());

	ci = 0;
	//for (uint32_t y = 0; y < h; ++y) {
	//	for (uint32_t x = 0; x < w; ++x) {
	for (size_t i : indices ) {
		{
			uint32_t x = i % w;
			uint32_t y = i / w;

			ci++;

			auto pixel_func = [=, &cam, &image, &finished]() {

				CameraRay ray = cam.getRay(x, y);

				debug = DebugInfo();
				debug.x = x;
				debug.y = y;

				glm::vec3 frag_color = rayColor(sceneToRender, ambient, complete_lights, ray);
				image(x, y) = frag_color;

				if (progressImage) {
					(*progressImage)(x, y) = glm::vec3(1, 1, 1);
				}

				finished++;
				return true;
			};

			queue.produce(pixel_func);

			// progress bar
			float progress = float(ci + 1 - max_threads) / float(w * h);
			progressBar.update(progress);

		}
	}

	// send poison pills
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
