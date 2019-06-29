/* simple raytracer based off Peter Shirley's "Ray Tracing in One Weekend" */

#include <iostream>
#include <fstream>
#include <cmath>
#include <limits>
#include <vector>
#include <random>
#include <algorithm>

#include "SDL.h"
#include "GL/glew.h"
#include "glm/glm.hpp"
#include "glm/matrix.hpp"
#include "glm/gtx/transform.hpp"

#include "shader_utils.h"

#define M_PI       3.14159265358979323846   // pi

class vec3 {
	float e[3];
public:
	inline float x() const { return e[0]; }
	inline float y() const { return e[1]; }
	inline float z() const { return e[2]; }
	inline float w() const { return 1.f;  }
	inline float r() const { return e[0]; }
	inline float g() const { return e[1]; }
	inline float b() const { return e[2]; }
	inline float a() const { return 1.f; }

	inline float squared_magnitude() const { return e[0]*e[0] + e[1]* e[1] + e[2]* e[2]; }
	inline float magnitude() const { return sqrt(squared_magnitude()); }

	inline vec3 operator -() const { return vec3(-e[0], -e[1], -e[2]); }

	inline vec3 operator +(const vec3 &rhs) const { return vec3(e[0] + rhs.x(), e[1] + rhs.y(), e[2] + rhs.z()); }
	inline vec3 operator -(const vec3 &rhs) const { return vec3(e[0] - rhs.x(), e[1] - rhs.y(), e[2] - rhs.z()); }
	inline vec3 operator *(const vec3 &rhs) const { return vec3(e[0] * rhs.x(), e[1] * rhs.y(), e[2] * rhs.z()); }
	inline vec3 operator /(const vec3 &rhs) const { return vec3(e[0] / rhs.x(), e[1] / rhs.y(), e[2] / rhs.z()); }

	inline vec3& operator +=(const vec3 &lhs) { e[0] += lhs.x(); e[1] += lhs.y(); e[2] += lhs.z(); return *this; }
	inline vec3& operator -=(const vec3 &lhs) { e[0] -= lhs.x(); e[1] -= lhs.y(); e[2] -= lhs.z(); return *this; }
	inline vec3& operator *=(const vec3 &lhs) { e[0] *= lhs.x(); e[1] *= lhs.y(); e[2] *= lhs.z(); return *this; }
	inline vec3& operator /=(const vec3 &lhs) { e[0] /= lhs.x(); e[1] /= lhs.y(); e[2] /= lhs.z(); return *this; }

	vec3() {e[0] = 0; e[1] = 0; e[2] = 0; }
	vec3(float x, float y, float z) { e[0] = x; e[1] = y; e[2] = z; }
	vec3(float n) { e[0] = e[1] = e[2] = n; }

	vec3(const glm::vec3& vec) { e[0] = vec.x; e[1] = vec.y; e[2] = vec.z; }
	vec3(const glm::vec4& vec) { e[0] = vec.x; e[1] = vec.y; e[2] = vec.z; }

	inline glm::vec3 to_glm_vec3() {
		return glm::vec3(x(), y(), z());
	}

	inline glm::vec4 to_glm_point() {
		return glm::vec4(x(), y(), z(), 1.0);
	}

	inline glm::vec4 to_glm_vector() {
		return glm::vec4(x(), y(), z(), 0.0);
	}
};

inline vec3 operator *(float t, const vec3 &vec) {
	return vec3(t*vec.x(), t*vec.y(), t*vec.z());
}

inline vec3 operator /(const vec3 &vec, float t) {
	return vec3(vec.x()/t, vec.y()/t, vec.z()/t);
}

inline vec3 operator -(const vec3 &vec, float t) {
	return vec3(vec.x() - t, vec.y() - t, vec.z() - t);
}

inline vec3 operator -(float t, const vec3 &vec) {
	return vec3(t - vec.x(), t - vec.y(), t - vec.z());
}

inline vec3 unit_vector(const vec3 & v){
	return v / v.magnitude();
}

inline float dot(const vec3 &v1, const vec3 &v2){
	return v1.x() * v2.x() + v1.y() * v2.y() + v1.z() * v2.z();
}

inline vec3 cross(const vec3 &v1, const vec3 &v2) {
	return vec3(v1.y()*v2.z() - v1.z()*v2.y(), v1.z()*v2.x() - v1.x()*v2.z(), v1.x()*v2.y() - v1.y()*v2.x());
}

template <class T> T lerp(float t, const T &a, const T &b) {
	return (1 - t)*a + t*b;
}

class ray
{
public:
	ray(const vec3& origin, const vec3& direction) :ori(origin), dir(direction) {};

	vec3 origin() const				{ return ori; }
	vec3 direction()const			{ return dir; }
	vec3 point_at_t(float t) const	{ return ori + t*dir; }

	vec3 ori;
	vec3 dir;
};

inline float frand(){
	return float(rand()) / RAND_MAX;
}

float hit_sphere(const vec3& center, float radius, const ray& r){
	vec3 oc = r.origin() - center;
	float a = r.direction().squared_magnitude();
	float b = 2.0 * dot(oc, r.direction());
	float c = oc.squared_magnitude() - radius*radius;
	float discriminant = b*b - 4 * a*c;
	
	if (discriminant < 0) {
		return -1.0f;
	}
	else {
		return (-b - sqrt(discriminant)) / (2.0*a);
	}
}

vec3 random_in_unit_sphere() {
	vec3 result;
	do {
		result = 2 * vec3(frand(), frand(), frand()) - vec3(1, 1, 1);
	} while (result.squared_magnitude() > 1);
	return result;
}

vec3 random_in_unit_disc() {
	vec3 result;
	do {
		result = 2 * vec3(frand(), frand(), 0) - vec3(1, 1, 0);
	} while (result.squared_magnitude() > 1);
	return result;
}

vec3 reflect(const vec3& vec, const vec3& norm) {
	return vec - 2 * dot(vec, norm)*norm; 

	/* division provides correct results if norm isn't unit vector, not strictly necessary */
	//return vec - 2 * dot(vec, norm)*norm / norm.squared_magnitude();
}

bool refract(const vec3& vec, const vec3& norm, float ni_over_nt, vec3& out_refracted) {
	vec3 uv = unit_vector(vec);
	float dt = dot(uv, norm);
	float discriminant = 1.0 - ni_over_nt*ni_over_nt*(1 - dt*dt);
	if (discriminant > 0) {
		out_refracted = ni_over_nt*(uv - norm*dt) - norm*sqrt(discriminant);
		return true;
	}
	return false;
}

template <class T> 
T fresnelSchlick(float cosTheta, T F0)
{
	return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

class camera {
public:
	camera(vec3 eye, vec3 lookat, vec3 up, float vfov, float aspect, float focus_dist, float aperture) : focus_dist(focus_dist) {
		float theta = vfov*M_PI / 180; /* convert into radians */

		lens_radius = aperture / 2;

		half_height = tan(theta / 2);
		half_width = aspect*half_height;

		origin = eye;
		w = unit_vector(eye - lookat);
		u = unit_vector(cross(up, w));
		v = cross(w, u);


	}

	ray get_ray(float s, float t) {
		vec3 rd = lens_radius*random_in_unit_disc();
		vec3 offset = u*rd.x() + v*rd.y();
		return ray(origin + offset, (s * 2 * half_width - half_width)*focus_dist*u + (t * 2 * half_height - half_height)*focus_dist*v - focus_dist*w - offset);
	}

	void orbit(vec3 axis, float angle) {
		glm::vec3 focal_point = origin.to_glm_vec3() - (w*focus_dist).to_glm_vec3();
		glm::mat4 transform = glm::translate(focal_point) * glm::rotate(angle, axis.to_glm_vec3()) * glm::translate(-focal_point);

		origin = vec3(transform*origin.to_glm_point());
		u = vec3(transform*u.to_glm_vector());
		w = vec3(transform*w.to_glm_vector());
		v = vec3(transform*v.to_glm_vector());
	}

	vec3 origin;

	float focus_dist;
	float lens_radius;

	vec3 u, v, w; /* basis vectors */
	float half_width;
	float half_height;
};

struct hit_record {
	const ray* ray;
	float t;
	vec3 p;
	vec3 normal;
	class material* material;
};

class material {
public:
	virtual bool scatter(const ray& r, const hit_record& record, vec3& out_attenuation, ray& out_scattered) const = 0;
};

class lambertian : public material {
public:
	lambertian(const vec3 & albedo) : albedo(albedo) {}

	virtual bool scatter(const ray& r, const hit_record& record, vec3& out_attenuation, ray& out_scattered) const {
		out_scattered = ray(record.p, record.normal + random_in_unit_sphere());
		out_attenuation = albedo;
		return true;
	}

	vec3 albedo;
};

class metal : public material {
public:
	metal(const vec3 & albedo, float roughness = 0.5) : albedo(albedo), roughness(roughness) {}

	virtual bool scatter(const ray& r, const hit_record& record, vec3& out_attenuation, ray& out_scattered) const {
		out_scattered = ray(record.p, reflect(r.direction(), record.normal) + roughness*random_in_unit_sphere());
		out_attenuation = albedo;
		return dot(out_scattered.direction(), record.normal) > 0;
	}
	vec3 albedo;
	float roughness;
};

class dielectric : public material {
public: 

	dielectric(float ref_idx) : ref_idx(ref_idx) {}

	virtual bool scatter(const ray& r, const hit_record& record, vec3& out_attenuation, ray& out_scattered) const {
		vec3 outward_normal;
		vec3 reflected = reflect(r.direction(), record.normal);
		vec3 refracted;
		float ni_over_nt;
		float cosine;
		float reflect_prob = 0.f;
		
		out_attenuation = vec3(0.95, 0.95, 0.95);

		cosine = dot(r.direction(), record.normal);

		/* ray is entering dielectric */
		if (cosine > 0) {
			outward_normal = -record.normal;
			ni_over_nt = ref_idx;
			cosine = ref_idx * cosine / r.direction().magnitude();
		}
		/* ray is exiting dielectric */
		else {
			outward_normal = record.normal;
			ni_over_nt = 1.0 / ref_idx;
			cosine = -cosine / r.direction().magnitude();
		}

		if (refract(r.direction(), outward_normal, ni_over_nt, refracted))
		{
			reflect_prob = fresnelSchlick(cosine, (1 - ref_idx) / (1 + ref_idx));

		}

		if(frand() > reflect_prob) {
			out_scattered = ray(record.p, refracted);
		}
		else {
			out_scattered = ray(record.p, reflected);
		}
		return true;
	}

	float ref_idx;
};

class entity {
public:
	virtual bool hit(const ray& r, float t_min, float t_max, hit_record& out_record) const = 0;
	virtual ~entity() {}
};

class sphere : public entity {
public:
	sphere(const vec3& center, float radius, material* material) : center(center), radius(radius), material(material) {}

	vec3 center;
	float radius;
	material* material;

	virtual bool hit(const ray& r, float t_min, float t_max, hit_record& out_record) const override {
		vec3 oc = r.origin() - center;
		float a = r.direction().squared_magnitude();
		float b = 2.0 * dot(oc, r.direction());
		float c = oc.squared_magnitude() - radius*radius;
		float discriminant = b*b - 4 * a*c;
		if (discriminant > 0) {
			float soln = (-b - sqrt(discriminant)) / (2.0*a);
			if (soln > t_max || soln < t_min) {
				soln = (-b + sqrt(discriminant)) / (2.0*a);
			}

			if (soln < t_max && soln > t_min) {
				out_record.ray = &r;
				out_record.t = soln;
				out_record.p = r.point_at_t(soln);
				out_record.normal = (out_record.p - center) / radius;
				out_record.material = material;
				return true;
			}
		}
		return false;
	}
};

class scene : public entity {
public:
	scene(entity** list, unsigned int count) : list(list), count(count) {}
	entity **list;
	unsigned int count;

	virtual bool hit(const ray& r, float t_min, float t_max, hit_record& out_record) const override {
		hit_record temp_record = { 0 };
		bool result = false;
		double closest = t_max;
		for (int i = 0; i < count; i++)
		{
			if (list[i]->hit(r, t_min, closest, temp_record)) {
				result = true;
				closest = temp_record.t;
				out_record = temp_record;
			}
		}
		return result;
	}
};

/* todo: make color out parameter; use it as early exit instead of arbitrary depth */
/* actually, keep arbitrary depth as well */
vec3 ray_color(const ray& r, const entity &world, int depth = 0){
	hit_record record = { 0 };

	if (depth > 50) {
		return vec3(0, 0, 0);
	}
	else if(world.hit(r, 0.001, FLT_MAX, record)){
		/* bounce the ray */
		vec3 attenuation;
		ray scattered(vec3(0,0,0), vec3(0,0,0));
		
		vec3 ray = record.material->scatter(r, record, attenuation, scattered);

		return attenuation*ray_color(scattered, world, depth + 1);
	}
	else{
		vec3 unit_direction = unit_vector(r.direction());
		float t = 0.5*(unit_direction.y() + 1.0);
		return lerp(t, vec3(1.0, 1.0, 1.0), vec3(0.5, 0.7, 1.0));
	}
}

#define shader(x) " #version 330 core\n" #x

const char*  vert_shader = shader(
#include "screenquad.vs"
);

const char* frag_shader = shader(
#include "screenquad.fs"
);

#if 0
static const unsigned int nx = 1024;
static const unsigned int ny = 512;
#else 
static const unsigned int nx = 256;
static const unsigned int ny = 128;
#endif
static const unsigned int ns = 20;
static float bitmap[ny][nx][3];

int main(int argc, char ** argv)
{
	vec3 eye(3, 3, 2);
	vec3 target(0, 0, -1);

	camera cam(eye, target, vec3(0, 1, 0), 20, float(nx)/float(ny), (target - eye).magnitude(), 0.05f);

	std::vector<material*> material_list;
	material_list.push_back(new lambertian(vec3(0.6, 0.4, 0.8)));
	material_list.push_back(new lambertian(vec3(0.8, 0.8, 0.0)));
	material_list.push_back(new metal(vec3(0.8, 0.6, 0.2), 1.0));
	material_list.push_back(new dielectric(1.5));

	std::vector<entity*> list;
	list.push_back(new sphere(vec3(0, 0, -1), .5, material_list[0]));
	list.push_back(new sphere(vec3(0, -100.5, -1), 100, material_list[1]));
	list.push_back(new sphere(vec3(1, 0, -1), 0.5, material_list[2]));
	list.push_back(new sphere(vec3(-1, 0, -1), 0.45, material_list[3]));

	entity* world = new scene(list.data(), list.size());

	//initializing SDL window
	SDL_Init(SDL_INIT_VIDEO);
	SDL_Window* window = SDL_CreateWindow("Ray Tracer", 100, 100, nx, ny, SDL_WINDOW_OPENGL);
	//SDL_SetRelativeMouseMode(SDL_TRUE);

	//initializing Opengl stuff
	SDL_GL_CreateContext(window);
	glewExperimental = GL_TRUE;
	GLenum result = glewInit();
	
	/* texures */
	GLuint render_target_tex = 0xffff;
	unsigned int num_levels = ceil(log2(nx > ny ? nx : ny)) + 1;

	memset(bitmap, 255u, ny*nx * 3);

	glGenTextures(1, &render_target_tex);
	glBindTexture(GL_TEXTURE_2D, render_target_tex);
#if 1
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
#else
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
#endif
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, num_levels - 1);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, nx, ny, 0, GL_RGB, GL_UNSIGNED_BYTE, bitmap);
	
	glGenerateMipmap(GL_TEXTURE_2D); /*TODO: figure out why my mipmap generation isn't working */

	//for (int level = 1; level < num_levels; level++)
	//{
	//	int x = floor(float(nx) / pow(2.f, level));
	//	int y = floor(float(ny) / pow(2.f, level));
	//
	//	glTexImage2D(GL_TEXTURE_2D, level, GL_RGB, x, y, 0, GL_RGB, GL_UNSIGNED_BYTE, bitmap);
	//}




	/* framebuffer */
	GLuint render_target_fbo = 0xffff;
	glGenFramebuffers(1, &render_target_fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, render_target_fbo);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, render_target_tex, 0);
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		std::cerr << "whoopsie\n";
	}


	/* shader program */
	GLuint shader_program = 0;
	shaderInfo shaders[] = { {GL_VERTEX_SHADER, "screenquad.vs"},
							{GL_FRAGMENT_SHADER, "screenquad.fs" } };
	shader_program = LoadProgram(shaders, 2);
	glUseProgram(shader_program);
	GLint uniform = glGetUniformLocation(shader_program, "texture");
	glUniform1i(uniform, 0);

	/* render loop */
	bool scene_dirty = true;
	bool new_input = false;
	bool exit = false;

	float translation_inc = 0.25;
	float rotation_inc = 10 * M_PI/180;

	while (!exit)
	{
		//polling for events
		SDL_Event event = SDL_Event();
		while (new_input || SDL_PollEvent(&event)) {
			new_input = false;

			if (event.type == SDL_QUIT) exit = true;
			if (event.type == SDL_KEYUP) {
				switch (event.key.keysym.sym) {
					case 'A' :
					case 'a' : 
						cam.orbit(vec3(0, 1, 0), -rotation_inc);
						break;
					case 'd' :
					case 'D' : 
						cam.orbit(vec3(0, 1, 0), rotation_inc);
						break;
					case 'W':
					case 'w':
						cam.orbit(cam.u, -rotation_inc);
						break;
					case 'S':
					case 's':
						cam.orbit(cam.u, rotation_inc);
						break;
					case 'C':
					case 'c':
						cam.origin += vec3(translation_inc, 0, 0);
						break;
					case 'Z' :
					case 'z' :
						cam.origin -= vec3(translation_inc, 0, 0);
						break;
					case 'R':
					case 'r':
						cam.origin += vec3(0, translation_inc, 0);
						break;
					case 'F':
					case 'f':
						cam.origin -= vec3(0, translation_inc, 0);
						break;
					case 'E':
					case 'e':
						cam.origin += vec3(0, 0, translation_inc);
						break;
					case 'Q':
					case 'q':
						cam.origin -= vec3(0, 0, translation_inc);
						break;
				}
				scene_dirty = true;
			}
		}

		/* restart loop if scene doesn't need to be drawn */
		if (!scene_dirty)
		{
			continue;
		}

		scene_dirty = false;
		
		/* show blank scene while drawing */
		SDL_GL_SwapWindow(window);

		unsigned int level = num_levels - 2;
		unsigned int total_samples = 0;
		while(true) {
		//for (unsigned int level = num_levels - 2; level < num_levels; level--){
			int x = floor(float(nx) / pow(2.f, level));
			int y = floor(float(ny) / pow(2.f, level));

			for (unsigned int j = y - 1; j < y && !new_input; j--) {
				for (unsigned int i = 0; i < x && !new_input; i++) {

					vec3 color;

					for (int s = 0; s < ns; s++) {
						float u = (float(i) + frand()) / float(x);
						float v = (float(j) + frand()) / float(y);

						ray r = cam.get_ray(u, v);
						color += ray_color(r, *world);
					}
					

					if (level == 0 && total_samples > 0){
						/* todo: use GPU for blending */
						vec3 &dstColor = *((vec3*)((float*)bitmap + (j*x * 3) + (i * 3) + 0));
						color = dstColor*(float(total_samples)/float(total_samples + ns)) + ((color) / float(total_samples + ns));
						//color = dstColor + (color - dstColor*ns) / float(total_samples + ns);
					}
					else {
						color = color / ns;
					}

					/* gamma correction done in shader */
					//color = vec3(sqrt(color.r()), sqrt(color.g()), sqrt(color.b()));

#if 1 
					*((float*)bitmap + (j*x * 3) + (i * 3) + 0) = color.r();
					*((float*)bitmap + (j*x * 3) + (i * 3) + 1) = color.g();
					*((float*)bitmap + (j*x * 3) + (i * 3) + 2) = color.b();
#else
					unsigned int ir = int(255.99*(color.r() > 1 ? 1 : color.r()));
					unsigned int ig = int(255.99*(color.g() > 1 ? 1 : color.g()));
					unsigned int ib = int(255.99*(color.b() > 1 ? 1 : color.b()));

					//bitmap[j][i][0] = ir;
					//bitmap[j][i][1] = ig;
					//bitmap[j][i][2] = ib;
					
					*((unsigned char*)bitmap + (j*x*3) + (i * 3) + 0) = ir;
					*((unsigned char*)bitmap + (j*x*3) + (i * 3) + 1) = ig;
					*((unsigned char*)bitmap + (j*x*3) + (i * 3) + 2) = ib;
#endif

					/* check for input, if there is some that we want to handle, flag it so we can skip to rendering the next frame */
					if (SDL_PollEvent(&event)) {
						if(	event.type == SDL_KEYUP ||
							event.type == SDL_QUIT)
						new_input = true;
					}
				}
			}
			
			if (new_input) {
				break;
			}

			glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
			glTexSubImage2D(GL_TEXTURE_2D, level, 0, 0, x, y, GL_RGB, GL_FLOAT, bitmap);

			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, level);
			glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

			SDL_GL_SwapWindow(window);

			glClear(GL_COLOR_BUFFER_BIT);
			if (level > 0) {
				level--;
			}
			else {
				total_samples += ns;
			}
			
		}
	}

	for (entity* e : list) {
		delete e;
	}

	for (material* m : material_list) {
		delete m;
	}

	delete world;

	/* output to file */
	std::fstream file;

	file.open("image.ppm", std::ios_base::out);
	file << "P3\n" << nx << " " << ny << "\n255\n";
	for (unsigned int j = ny - 1; j < ny; j--) {
		for (unsigned int i = 0; i < nx; i++) {
			file << int(255.99/bitmap[j][i][0]) << " " << int(255.99 / bitmap[j][i][1]) << " " << int(255.99 / bitmap[j][i][2]) << "\n";
		}
	}
	file.close();

	return 0;
}