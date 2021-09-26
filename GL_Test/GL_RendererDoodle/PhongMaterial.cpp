// Fall 2019

#include "PhongMaterial.h"

#include "glm/glm.hpp"
#include "glm/gtx/associated_min_max.hpp"
#include "glm/gtx/extended_min_max.hpp"]
#include "glm/gtc/random.hpp"
#include "glm/gtc/type_ptr.hpp"

#include <algorithm>

#include "RenderRaytracer.h"
#include "Image.h"
#include "Sampler.h"
#include "Asset.h"

#ifdef SOFT_SHADOWS
// Allow defining values for default light size
#ifndef SOFT_SHADOW_RADIUS
// default: modulate light size based on distance from scene origin
#define SOFT_SHADOW_RADUIS (glm::length(light->position) / 10.f)
#endif 
#endif // SOFT_SHADOWS

#ifndef SOFT_SHADOW_MIN_SAMPLES
#define SOFT_SHADOW_MIN_SAMPLES (64)
#endif

// Allow defining values for default reflection size
#ifdef SOFT_REFLECTIONS
#ifndef SOFT_REFLECT_ANGLE
// default: modulate reflection clarity based on shininess
#define SOFT_REFLECT_ANGLE (glm::mix(90, 0, glm::pow(glm::clamp(shininess/128.0, 0.0, 1.0), 0.125f)) ) 
#endif
#endif // SOFT_REFLECTIONS

#ifndef SOFT_REFLECT_MIN_SAMPLES
#define SOFT_REFLECT_MIN_SAMPLES (32)
#endif

inline bool planeIntersect(glm::vec3 &out_i, const glm::vec3 &r, const glm::vec3 &l, const glm::vec3 &o, const glm::vec3 &n) {
		float denominator = glm::dot(l, n);
		if (denominator == 0) {
				return false;
		}

		float t = glm::dot((o - r), n) / denominator;
		out_i = r + t * l;
		return true;
}


//todo: sample rate doesn't seem to work at non-oblique angle
// need anisotropic filtering, and try to match to opengl mip-level algorithm
inline glm::vec2 sampleRate(const CameraRay& view, glm::vec3 p, glm::vec3 n, glm::vec3 t, glm::vec3 b) {
		glm::vec3 px;
		glm::vec3 py;

		if (!(view.hasDifferentials && planeIntersect(px, view.dx.o, view.dx.l, p, n) && planeIntersect(py, view.dy.o, view.dy.l, p, n)))
		{
				return glm::vec2(0, 0);
		}


		// px == p + (d_u)*(dp/du) + (d_v)*(dp_dv)
		// py == p + (d_u)*(dp/du) + (d_v)*(dp_dv)

		unsigned char max = 0;
		for (int i = 1; i < 3; i++) {
				if (n[max] < n[i]) { max = i; }
		}

		unsigned char a1 = (max + 1) % 3;
		unsigned char a2 = (max + 2) % 3;

		glm::vec2 px_(px[a1] - p[a1], px[a2] - p[a2]);
		glm::mat2 d_mat{ t[a1], t[a2],
											b[a1], b[a2] };
		//glm::mat2 d_mat{ t[a1], b[a1],
		//									t[a2], b[a2] };
		glm::vec2 duv_x = glm::inverse(d_mat) * px_;

		glm::vec2 py_(py[a1] - p[a1], py[a2] - p[a2]);
		glm::vec2 duv_y = glm::inverse(d_mat) * py_;

		return glm::vec2(
				std::max(std::abs(duv_x.x), std::abs(duv_y.x)),
				std::max(std::abs(duv_x.y), std::abs(duv_y.y)));
}

inline float shadow(SceneNode *root, 
                glm::vec3 pos, glm::vec3 dir,
                float dist) {
		IntersectResult shadowIntersection;
		if (intersectScene(shadowIntersection, root, 
			  Ray{pos, dir},
			  0.01, dist)) {
			return 0.f;
		} else {
      return 1.f;
    }
}

void getLocalVectors(glm::vec3 &out_tangent, glm::vec3 &out_bitangent, const glm::vec3 &normal) {
	if (normal.x == glm::min(normal.x, normal.y, normal.z)) {
		out_tangent = glm::vec3(1, 0, 0);
	} else if (normal.y == glm::min(normal.x, normal.y, normal.z)) {
		out_tangent = glm::vec3(0, 1, 0);
	} else if (normal.z == glm::min(normal.x, normal.y, normal.z)) {
		out_tangent = glm::vec3(0, 0, 1);
	}

	out_tangent = glm::normalize(glm::cross(normal, out_tangent));
	out_bitangent = glm::normalize(glm::cross(out_tangent, normal));
}

inline float softShadow(SceneNode *root, 
                glm::vec3 pos, glm::vec3 dir,
                float dist, float lightRad, float attenuation) {

    IntersectResult shadowIntersection;

	if (lightRad > dist) {
		// inside the light, don't need to do shadows, (unless we want weird stuff)
		return 1.0;
	}

	float radius = lightRad / dist; 
	float angle = glm::atan(radius);

	const int minSamples = SOFT_SHADOW_MIN_SAMPLES;

	return randomSampling<float>(
		[&](int sampleNum)->float { 
			glm::vec3 l = glm::ballRand(radius); 
			return intersectScene(shadowIntersection, root, Ray{pos, dir + l}, 0.01, dist) ? 0.f : 1.f;},
		[&](const float &l, const float &r) { return std::abs( l - r);},
		minSamples * attenuation, 2048 * attenuation, 0.005 / attenuation );
}


glm::vec3 phongShading(const glm::vec3 &kd, const glm::vec3 &ks, double shininess,
	glm::vec3 n,
	SceneNode* root, const glm::vec3 & ambient, const std::list<Light *> & lights,
	const CameraRay &view, IntersectResult* fragmentIntersection,
	double attenuation) {
		glm::vec3 view_pos = view.r.o;
	glm::vec3 color = kd * ambient;
	n = glm::normalize(n);
	glm::vec3 v = glm::normalize(view_pos - fragmentIntersection->position);

	for (Light* light : lights ) {
		glm::vec3 lightVec = light->position - fragmentIntersection->position;
		double dist = glm::length(lightVec);
		glm::vec3 l = lightVec / float(dist);

		float diffuse_strength = glm::max(0.f, glm::dot(n, l));

		glm::vec3 r = -glm::normalize(glm::reflect(l, n)); // negate, so reflected vector points away from surface
		float specular_strength = (float)glm::pow(glm::clamp(glm::dot(r, v), 0.f, 1.f), shininess);

		float falloff = 1 / (light->falloff[0] + light->falloff[1] * dist + light->falloff[2] * dist * dist);

		glm::vec3 diff_color = kd * light->colour * diffuse_strength * falloff;
		glm::vec3 spec_color = ks * light->colour * specular_strength * falloff;

		//const float low_diff = nextafterf(0, 1.0);
		const float low_diff = 0.004;
		glm::vec3 low_diff_vec = glm::vec3(low_diff, low_diff, low_diff);

		// early exit to catch lights that won't contribute to final color
		if (glm::all(glm::lessThan(diff_color * float(attenuation), low_diff_vec)) && glm::all(glm::lessThan(spec_color * float(attenuation), low_diff_vec))) {
			continue;
		}

#ifdef SOFT_SHADOWS	// soft shadows
		float lightRadius = SOFT_SHADOW_RADUIS;
		float shad = softShadow(root, fragmentIntersection->position, l, dist, lightRadius, attenuation);
		if (shad == 0.0) {
			continue;
		}
#else	// hard shadows
		float shad = shadow(root, fragmentIntersection->position, l, dist);
		if (shad == 0.0) {
			continue;
		}
#endif
	
		color += shad * diff_color;
		color += shad * spec_color;
	}
#ifndef DISABLE_REFLECTIONS
	double strength = 0.25 * glm::length(ks);
	if (attenuation > glm::pow(strength, 4)) {
		glm::vec3 v_reflected = glm::reflect(-v, n);
#ifndef SOFT_REFLECTIONS
		color +=
				float(1/attenuation) * rayColor(root, ambient, lights, 
						Ray{fragmentIntersection->position + v_reflected * 0.01f, v_reflected}, 
						attenuation * strength);
#else

	float angle = glm::radians(float(SOFT_REFLECT_ANGLE));
	angle = glm::clamp(angle, glm::radians(0.f), glm::radians(180.f));
	float radius = glm::sin(angle / 2);

	const int nSamples = (SOFT_REFLECT_MIN_SAMPLES) * attenuation;
	strength = strength / float(nSamples);

	color += randomSampling<glm::vec3>(
		[&](int sampleNum)->glm::vec3{
			glm::vec3 dir =  glm::ballRand(radius);
			return float(1.f/attenuation) * rayColor(root, ambient, lights, 
					Ray{fragmentIntersection->position + v_reflected * 0.01f, 
					v_reflected + dir}, 
					attenuation * strength) * float(nSamples); },
		[&](const glm::vec3 &l, const glm::vec3 &r) { return glm::length(l - r);},
		nSamples, nSamples * 16, 0.001 / attenuation);
#endif
	}
#endif

	return color * float(attenuation);
}

PhongMaterial::PhongMaterial(
	const glm::vec3& kd, const glm::vec3& ks, double shininess )
	: m_kd(kd)
	, m_ks(ks)
	, m_shininess(shininess)
{}

PhongMaterial::~PhongMaterial()
{}

std::string PhongMaterial::getId() const {
	static std::string id = "__phong_material";
	return id;
}

TexturedPhongMaterial::TexturedPhongMaterial(
	const std::string &textureName, const glm::vec3& ks, double shininess )
	: m_kd(Image::loadPng(searchForAsset(textureName)))
	, m_ks(ks)
	, m_shininess(shininess)
{
	m_imageResources.emplace_back(textureName, &m_kd);
}

TexturedPhongMaterial::~TexturedPhongMaterial()
{}

std::string TexturedPhongMaterial::getId() const {
	static std::string id = "__texturedPhong_material";
	return id;
}


NormalTexturedPhongMaterial::NormalTexturedPhongMaterial(
	const std::string &textureName, const std::string &normalMapName, const glm::vec3& ks, double shininess )
	: m_kd(Image::loadPng(searchForAsset(textureName)))
	, m_normalMap(Image::loadPng(searchForAsset(normalMapName)))
	, m_ks(ks)
	, m_shininess(shininess)
{
	m_imageResources.emplace_back(textureName, &m_kd);
	m_imageResources.emplace_back(normalMapName, &m_normalMap);
}

NormalTexturedPhongMaterial::~NormalTexturedPhongMaterial()
{}

std::string NormalTexturedPhongMaterial::getId() const {
	static std::string id = "__normalTexturedPhong_material";
	return id;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

PBRMaterial::PBRMaterial(const glm::vec3 &kd, float roughness, float metal) 
  : m_kd(kd)
  , m_metal(metal)
  , m_roughness(roughness)
{
}

PBRMaterial::~PBRMaterial()
{}

std::string PBRMaterial::getId() const {
	static std::string id = "__PBR_material";
	return id;
}


// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

NormalTexturedPBRMaterial::NormalTexturedPBRMaterial(const std::string &textureName, const std::string &normalMapName, float roughness, float metal) 
  : m_kd(Image::loadPng(searchForAsset(textureName)))
  , m_normalMap(Image::loadPng(searchForAsset(normalMapName)))
  , m_metal(metal)
  , m_roughness(roughness)
{
	m_imageResources.emplace_back(textureName, &m_kd);
	m_imageResources.emplace_back(normalMapName, &m_normalMap);
}

NormalTexturedPBRMaterial::~NormalTexturedPBRMaterial()
{}

std::string NormalTexturedPBRMaterial::getId() const {
	static std::string id = "__normalTexturedPBR_material";
	return id;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

glm::vec3 PhongMaterial::shade(
	SceneNode* root, const glm::vec3 & ambient, const std::list<Light *> & lights,
	const CameraRay &view, IntersectResult* fragmentIntersection,
	double attenuation) {

	return phongShading(m_kd, m_ks, m_shininess, fragmentIntersection->normal, root, ambient, lights, view, fragmentIntersection, attenuation);
}

glm::vec3 colours[] = {
		glm::vec3(0, 0, 1),
		glm::vec3(0, 1, 0),
		glm::vec3(1, 0, 0),
		glm::vec3(0, 1, 1),
		glm::vec3(1, 1, 0),
		glm::vec3(1, 0, 1),
		glm::vec3(1, 1, 1),
};

#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))

// todo: doesn't match openGl image axes
glm::dvec3& Texture::operator() (glm::vec3 d, unsigned int level) {
		assert(layers == 6);

		uint8_t a = 0;
		for (uint8_t i = 1; i < 3; i++) {
				a = (std::abs(d[a]) > std::abs(d[i])) ? a : i;
		}

		unsigned int layer = a * 2 + (d[a] > 0 ? 0 : 1);

		d = d / std::abs(d[a]);
		float u = d[(a + 1) % 3] * 0.5 + 0.5;
		float v = d[(a + 2) % 3] * 0.5 + 0.5;

		Image& image = images[level * layers + layer];
		uint32_t x = u * (image.width());
		uint32_t y = v * (image.height());

		x = std::min(x, image.width() - 1);
		y = std::min(y, image.height() - 1);

		return (glm::dvec3&)image(x, y, 0);
}

glm::vec3 sampleTexture(const Texture &texture, const glm::vec2 &uv, size_t layer = 0) {
	glm::vec3 tex_color;
	const Image &tex = texture.images[std::min(layer, texture.images.size() - 1)];
	
	if (layer > 0) {
	//		return  colours[(layer - 1) % ARRAY_SIZE(colours)];
	}

	glm::vec2 uv_whole;
	glm::vec2 uv_part;

	uv_part.x = modff(uv.x, &uv_whole.x);
	uv_part.y = modff(uv.y, &uv_whole.y);
	
	glm::ivec2 tex_coord = glm::vec2(tex.width(), tex.height()) * uv_part;
	tex_coord.y = tex.height() - 1 - tex_coord.y;

	tex_coord.x = tex_coord.x % tex.width();
	tex_coord.y = tex_coord.y % tex.height();

	tex_color.x = tex(tex_coord.x, tex_coord.y, 0);
	tex_color.y = tex(tex_coord.x, tex_coord.y, 1);
	tex_color.z = tex(tex_coord.x, tex_coord.y, 2);

	return tex_color;
}

glm::vec3 TexturedPhongMaterial::shade(
	SceneNode* root, const glm::vec3 & ambient, const std::list<Light *> & lights,
	const CameraRay &view, IntersectResult* fragmentIntersection,
	double attenuation) {

	glm::vec3 tex_color = sampleTexture(m_kd, fragmentIntersection->surface_uv);

	return phongShading(tex_color, m_ks, m_shininess, fragmentIntersection->normal, root, ambient, lights, view, fragmentIntersection, attenuation);
}

glm::vec3 NormalTexturedPhongMaterial::shade(
	SceneNode* root, const glm::vec3 & ambient, const std::list<Light *> & lights,
	const CameraRay &view, IntersectResult* fragmentIntersection,
	double attenuation) {

	glm::vec3 normal = fragmentIntersection->normal;
	// todo: should these be normalized? does it need to be an orthonormal basis, or should it be stretched if the UV space is stretched?
	glm::vec3 tangent = glm::normalize(fragmentIntersection->tangent);
	glm::vec3 bitangent = glm::normalize(fragmentIntersection->bitangent);

	glm::vec2 sr = sampleRate(view, fragmentIntersection->position, normal, fragmentIntersection->tangent, fragmentIntersection->bitangent);

	// sampleRate * (m_kd.images[0].width() / (2^layer)) == 1

	float du = glm::log2(sr.x * m_kd.images[0].width());
	float dv = glm::log2(sr.y * m_kd.images[0].height());

	size_t layer = std::max(0, (int)std::floor(std::min(du, dv)));

	glm::vec3 tex_color = sampleTexture(m_kd, fragmentIntersection->surface_uv, layer);
	glm::vec3 tex_normal = sampleTexture(m_normalMap, fragmentIntersection->surface_uv, layer);

	tex_normal = (tex_normal - 0.5f) * 2.f;

	normal = normal*tex_normal.z + tangent*tex_normal.x + bitangent*tex_normal.y;

	return phongShading(tex_color, m_ks, m_shininess, normal, root, ambient, lights, view, fragmentIntersection, attenuation);
}



glm::vec3 PBRMaterial::shade(
	SceneNode* root, const glm::vec3 & ambient, const std::list<Light *> & lights,
	const CameraRay &view, IntersectResult* fragmentIntersection,
	double attenuation) {

	return phongShading(m_kd, glm::mix(glm::vec3(1, 1, 1), m_kd, m_metal), (1 - m_roughness) * 128, fragmentIntersection->normal, root, ambient, lights, view, fragmentIntersection, attenuation);
}

glm::vec3 NormalTexturedPBRMaterial::shade(
	SceneNode* root, const glm::vec3 & ambient, const std::list<Light *> & lights,
	const CameraRay &view, IntersectResult* fragmentIntersection,
	double attenuation) {

	glm::vec3 normal = fragmentIntersection->normal;
	// todo: should these be normalized?
	glm::vec3 tangent = glm::normalize(fragmentIntersection->tangent);
	glm::vec3 bitangent = glm::normalize(fragmentIntersection->bitangent);

	glm::vec2 sr = sampleRate(view, fragmentIntersection->position, normal, fragmentIntersection->tangent, fragmentIntersection->bitangent);

	// sampleRate * (m_kd.images[0].width() / (2^layer)) == 1
	size_t layer = std::max(0, (int)std::floor(std::min(glm::log2(sr.x * m_kd.images[0].width()), glm::log2(sr.y * m_kd.images[0].height()))));

	glm::vec3 tex_color = sampleTexture(m_kd, fragmentIntersection->surface_uv, layer);
	glm::vec3 tex_normal = sampleTexture(m_normalMap, fragmentIntersection->surface_uv, layer);

	tex_normal = (tex_normal - 0.5f) * 2.f;

	// getLocalVectors(tangent, bitangent, normal);

	normal = normal*tex_normal.z + tangent*tex_normal.x + bitangent*tex_normal.y;

	return phongShading(tex_color, glm::mix(glm::vec3(1, 1, 1), tex_color, m_metal), (1 - m_roughness) * 128, normal, root, ambient, lights, view, fragmentIntersection, attenuation);
}