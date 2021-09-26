#pragma once

#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/scalar_multiplication.hpp>

struct Ray {
	glm::vec3 o; // origin
	glm::vec3 l; // length

	inline Ray transformed(const glm::mat4& trans) const {
		return Ray{ glm::vec3(trans * glm::vec4(o, 1.f)),  glm::vec3(trans * glm::vec4(l, 0.f)) };
	}

	friend Ray operator*(const glm::mat4& trans, const Ray& ray);

	Ray() = delete; // no default constructor

};

inline Ray operator*(const glm::mat4& trans, const Ray& ray) {
	return ray.transformed(trans);
}

struct CameraRay {
	Ray r;
	bool hasDifferentials = false;
	Ray dx;
	Ray dy;

	CameraRay(Ray r) : r(r), hasDifferentials(false), dx{ {0,0,0},{0,0,0} }, dy{ {0,0,0},{0,0,0} } {}
};

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
