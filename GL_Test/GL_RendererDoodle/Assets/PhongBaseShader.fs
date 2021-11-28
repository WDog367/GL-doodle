#version 330

#define SHADOW_MAPPING

// list of defines:

// KD_TEXTURE
// KS_TEXTURE
// SHADOW_MAPPING
// NORMAL_MAPPING

in VsOutFsIn {
	vec3 position_ES; // Eye-space position
	vec3 normal_ES;   // Eye-space normal
    vec2 uv;

    vec3 tangent_ES;
	vec3 bitangent_ES;

    vec3 position_MS; // model-space position
} fs_in;

out vec4 fragColour;

const int lightNum = 50;
struct LightSource {
    vec3 position;
    vec3 colour;
    float attenuation[3];
};
uniform LightSource light[lightNum];
uniform int activeLights;

struct GlobalLight {
#ifdef SHADOW_MAPPING
    mat4 matrix;
    sampler2D depthMap;
#endif

    vec3 direction;
    vec3 colour;
};
uniform GlobalLight globalLight;

struct Material {
    float shininess;
};
uniform Material material;

// Ambient light intensity for each RGB component.
uniform vec3 ambientIntensity;

uniform samplerCube environment_map;

uniform mat3 InverseView;

#define ADDITIONS

// FUNCTIONS --------------------------------------------------
vec3 getKd();
vec3 getKs();
vec3 getNormal();
float getShininess();

float getShininess() {
    return material.shininess;
}

vec3 Kd = getKd();
vec3 Ks = getKs();
vec3 n = getNormal();
float shininess = getShininess();

// phong lighting model
vec3 phongLight(in vec3 l, in vec3 rgbIntensity, 
        in vec3 fragNormal, in vec3 v) {
    float n_dot_l = max(dot(fragNormal, l), 0.0);

    vec3 color = Kd * rgbIntensity * n_dot_l;

    if (n_dot_l > 0.0) {
        // Halfway vector. TODO: don't do blinn-phong
        vec3 h = normalize(v + l);
        float n_dot_h = max(dot(fragNormal, h), 0.0);

        color  += Ks * rgbIntensity * pow(n_dot_h, shininess);
    }

    return color;
}

vec3 lights(vec3 fragPosition, vec3 fragNormal) {
    // Direction from fragment to viewer (origin - fragPosition).
    vec3 v = normalize(-fragPosition.xyz);
    vec3 r = reflect(-v, fragNormal);

    vec3 diffuse = vec3(0.0);
    vec3 specular = vec3(0.0);

    vec3 color = vec3(0.0);

    
    // scene point lights
    for (int i = 0; i < activeLights; i++ ) {
        LightSource light = light[i];

        // Direction from fragment to light source.
        vec3 l = light.position - fragPosition;
        float d = length(l);
        l = l / d;

        float falloff = light.attenuation[0] + d * light.attenuation[1] + d*d* light.attenuation[2];
        vec3 rgbIntensity = light.colour / falloff;
		
		//todo: measure if this is effective ... it relies on GPU having warp branch detection
		//		-- no noticable effect
		if (any(greaterThan(Kd * rgbIntensity, vec3(0.01, 0.01, 0.01)))
		    || any(greaterThan(Ks * rgbIntensity, vec3(0.01, 0.01, 0.01)))) {
		    color += phongLight(l, rgbIntensity, fragNormal, v);
		}
    }

    // Global directional light (with shadowing)
	if (false)
    {
        vec3 l = -globalLight.direction;
        vec3 rgbIntensity = globalLight.colour;
#ifdef SHADOW_MAPPING
        vec3 pos_ls = (globalLight.matrix * vec4(fragPosition, 1)).xyz;

        pos_ls = pos_ls * 0.5 + 0.5;

        const float depthMapOffset = 50 * (200.0 / 16777215.0); //todo: make offset a uniform

        if((pos_ls.x < 0 || pos_ls.y < 0 || pos_ls.z < 0 ||
            pos_ls.x  > 1 || pos_ls.y > 1 || pos_ls.z > 1 ) ||
           (texture(globalLight.depthMap, pos_ls.xy).r + depthMapOffset >= pos_ls.z)) {
               color += phongLight(l, rgbIntensity, fragNormal, v);
        }
#else
        color += phongLight(l, rgbIntensity, fragNormal, v);
#endif
    }


    // environment mapping
    vec3 ambientReflectionStrength;
#if 1 // ENVIRONMENT_MAPPING
    {

        vec3 reflectionStrength = Ks * 0.25 * length(getKs());
        color += reflectionStrength * texture(environment_map, InverseView * r).rgb;
#endif // ENVIRONMENT_MAPPING
    }

    color += Kd * ambientIntensity;

    return color;
}

void main() {
    vec3 color = lights(fs_in.position_ES, getNormal());
	fragColour = vec4(color, 1.0);
}
