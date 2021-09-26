#version 330

#define SHADOW_MAPPING

// list of defines:

// KD_TEXTURE
// KS_TEXTURE
// SHADOW_MAPPING
// NORMAL_MAPPING

// PBR_SHADING
// SPECULAR_IBL
// SPECULAR_IBL_REALTIME

//#define SPECULAR_IBL
//#define SPECULAR_IBL_REALTIME

#if defined PBR_SHADING && !defined(PHONG_LIGHTING)
#define PBR_LIGHTING
#endif

#define SPECULAR_IBL
//#define SPECULAR_IBL_REALTIME

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
#ifdef KD_TEXTURE
    sampler2D kd;
#else
    vec3 kd;
#endif

#ifdef PBR_SHADING
    float roughness;
    float metal;
#else
#ifdef KS_TEXTURE
    sampler2D ks;
#else 
    vec3 ks;
#endif

    float shininess;
#endif // PBR_SHADING
};
uniform Material material;

#ifdef NORMAL_MAPPING
uniform sampler2D normal_map;
#endif //NORMAL_MAPPING

#ifdef SPECULAR_IBL
uniform samplerCube prefiltered_specular;
const uint max_prefiltered_lod = 5u; 
uniform sampler2D brdf_lut; 
#endif // SPECULAR_IBL

// Ambient light intensity for each RGB component.
uniform vec3 ambientIntensity;

uniform samplerCube environment_map;

uniform mat3 InverseView;

// FUNCTIONS --------------------------------------------------
const float gamma = 2.2;
const float PI = 3.14159;

vec3 getKd();
vec3 getKs();
vec3 getNormal();
float getShininess();
float getRoughness();
float getMetal();

vec3 getKd() {
#ifdef KD_TEXTURE
    return texture(material.kd, fs_in.uv).rgb;
#else 
    return material.kd;
#endif
}
#if defined PBR_LIGHTING
vec3 Kd = pow(getKd(), vec3(gamma));
#else
vec3 Kd = getKd();
#endif

vec3 getKs() {
#ifdef PBR_SHADING
    return mix(vec3(0.04), getKd(), getMetal());
#elif defined KS_TEXTURE
    return texture(material.ks, fs_in.uv).rgb;
#else 
    return material.ks;
#endif
}
vec3 Ks = getKs();

vec3 getNormal() {
#ifdef NORMAL_MAPPING
    vec3 tex_normal = texture(normal_map, fs_in.uv).rgb * 2 - 1;
    vec3 normal = normalize(fs_in.normal_ES);
    vec3 tangent = fs_in.tangent_ES;
    vec3 bitangent = fs_in.bitangent_ES;

    return normalize(normal*tex_normal.z + tangent*tex_normal.x + bitangent*tex_normal.y);
#else
    return normalize(fs_in.normal_ES);
#endif
}
vec3 n = getNormal();

float getShininess() {
#ifdef PBR_SHADING
    return (1.0 - material.roughness) * 128.0;
#else
    return material.shininess;
#endif
}
float shininess = getShininess();

float getRoughness() {
#ifdef PBR_SHADING 
    return material.roughness;
#else 
    return 1.0 - clamp((material.shininess + 1) / 128, 0.0, 1.0);
#endif
}
float roughness = getRoughness();

float getMetal() {
#ifdef PBR_SHADING
    return material.metal;
#else
    return pow(clamp(dot(Kd, Ks), 0.0, 1.0), 0.5);
#endif
}
float metal = getMetal();

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

// PBR lighting model 

// Schlick Fresnel
vec3 f0 = mix(vec3(0.04), Kd, metal);

vec3 fresnel(vec3 v, vec3 h) {
    float v_dot_h = max(dot(v, h), 0);
    return f0 + (1 - f0) * pow(2, (-5.55473*(v_dot_h) - 6.98316) * (v_dot_h));
}

vec3 fresnelRoughness(float n_dot_v, float roughness) {
    return f0 + (max(vec3(1.0 - roughness), f0) - f0) * pow(1.0 - n_dot_v, 5.0);
}

// Schlick Geometric
float G1(float n_dot_v, float k) {
    return n_dot_v / (n_dot_v * (1 - k) + k);
}

float Geometry(float n_dot_l, float n_dot_v) {
    // float a = ((roughness + 1) / 2)^2
    // float k = a / 2
    float k = (roughness + 1)*(roughness + 1) / 8;
    return G1(n_dot_v, k) * G1(n_dot_l, k);
}

// GGX Normal Distribution Function
float NDF(float n_dot_h) {
    float a = roughness * roughness;
    float denominator = (n_dot_h*n_dot_h) * (a*a - 1) + 1;
    return a*a / (PI * denominator * denominator);
}

// PBR Lighting function
vec3 PBRLight(in vec3 l, in vec3 rgbIntensity, 
        in vec3 fragNormal, in vec3 v) {
    
    vec3 h = normalize(v + l);
    float n_dot_l = max(dot(fragNormal, l), 0.0);
    float n_dot_h = max(dot(fragNormal, h), 0.0);
    float n_dot_v = max(dot(fragNormal, v), 0.0);

    float D = NDF(n_dot_h);
    vec3 F = fresnel(v, h);
    float G = Geometry(n_dot_l, n_dot_v);

    // Cook-Torrance BRDF function
    float denominator = max(4 * n_dot_l * n_dot_v, 0.00001);
    
    vec3 specularBRDF = D * F * G / denominator;
    vec3 diffuse = Kd / PI * ( 1.0 - F) * (1 - metal);
    
    return PI * rgbIntensity * n_dot_l * (diffuse + specularBRDF); 
}

// Hmmersley sequence taken from: https://learnopengl.com/PBR/IBL/Specular-IBL
float RadicalInverse_VdC(uint bits) 
{
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10; // / 0x100000000
}
// ----------------------------------------------------------------------------
vec2 Hammersley(uint i, uint N)
{
    return vec2(float(i)/float(N), RadicalInverse_VdC(i));
}  

// ImportanceSampleGGX taken from: https://cdn2.unrealengine.com/Resources/files/2013SiggraphPresentationsNotes-26915738.pdf
vec3 ImportanceSampleGGX(vec2 Xi, float roughness, vec3 N) {
    float a = roughness * roughness;

    float Phi = 2 * PI * Xi.x;
    float cosTheta = sqrt( (1 - Xi.y) / ( 1 + (a*a - 1) * Xi.y ) );
    float sinTheta = sqrt( 1 - cosTheta * cosTheta );

    vec3 H;
    H.x = sinTheta * cos  ( Phi );
    H.y = sinTheta * sin  ( Phi );
    H.z = cosTheta;

    return H;
}

// Uses a different approximation for a, appropriate for sampling from env map
float Geometry_Schlick(float n_dot_l, float n_dot_v, float roughness) {
    // float a = (roughness)^2
    // float k = a / 2
    float k = roughness * roughness / 2.0;
    return G1(n_dot_v, k) * G1(n_dot_l, k);
}

float computeLod(float n_dot_h, float h_dot_v, uint numSamples) {
    float k = roughness * roughness / 2;
    float D = G1(n_dot_h, k);
    float pdf = (D * n_dot_h / (4 * h_dot_v)) + 0.001f;

    float saTexel = 4.0 * PI / (6.0 * 512 * 512);
    float saSample = 1.0 / (float(numSamples) * pdf + 0.0001);

    return (roughness == 0.0 ? 0.0 : 0.5 * log2(saSample / saTexel));
}

vec3 lights(vec3 fragPosition, vec3 fragNormal) {
#ifdef PBR_LIGHTING
    #define LIGHTFUNC PBRLight
#else 
    #define LIGHTFUNC phongLight
#endif

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
		    color += LIGHTFUNC(l, rgbIntensity, fragNormal, v);
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
               color += LIGHTFUNC(l, rgbIntensity, fragNormal, v);
        }
#else
        color += LIGHTFUNC(l, rgbIntensity, fragNormal, v);
#endif
    }


    // environment mapping

    vec3 ambientReflectionStrength;

    {
#if defined PBR_LIGHTING && defined SPECULAR_IBL_REALTIME
        vec3 n = fragNormal;
        
        // get tangent + bitangent to translate H into worldspace
        vec3 up = abs(n.z) < 0.999 ? vec3(0, 0, 1) : vec3(1, 0, 0);
        vec3 tanx = normalize(cross(up, n));
        vec3 bitan = cross(n, tanx);

        vec3 envColor = vec3(0);

        const uint numSamples = 30u;
        float totalWeight = 0;


        float k = (roughness * roughness) / 2.0;

        for (uint i = 0u; i < numSamples; i++) {
            vec2 xi = Hammersley(i, numSamples);
            vec3 Li = ImportanceSampleGGX(xi, roughness, n); 
            vec3 h = tanx * Li.x + bitan * Li.y + n * Li.z;
            vec3 l = 2 * dot(v, h) * h - v;

            float n_dot_l = clamp(dot(n, l), 0, 1);
            float n_dot_h = clamp(dot(n, h), 0, 1);
            float h_dot_v = clamp(dot(h, v), 0, 1);
            float n_dot_v = clamp(dot(n, v), 0, 1);
            float lod = computeLod(n_dot_h, h_dot_v, numSamples);

            vec3 F = fresnelRoughness(n_dot_h, roughness);
            float G = Geometry_Schlick(n_dot_l, n_dot_v, roughness);
            vec3 c = textureLod(environment_map, InverseView * l, lod).rgb;

            float weight = G * h_dot_v / (n_dot_h * n_dot_v);
            envColor += F * c * weight;
            totalWeight += weight;
        } 

        color +=  (envColor / float(numSamples));

#elif defined SPECULAR_IBL && defined PBR_LIGHTING
        float n_dot_v = clamp(dot(fragNormal, v), 0.0, 1.0);
        vec3 h = normalize(v + r);

        vec3 prefilteredColour = textureLod(prefiltered_specular, InverseView * r, roughness * max_prefiltered_lod).rgb;
        vec2 envBRDF = texture(brdf_lut, vec2(n_dot_v, roughness)).rg;

        vec3 reflectionStrength = (fresnelRoughness(max(dot(v, h), 0), roughness) * envBRDF.x + envBRDF.y);
        color += prefilteredColour * reflectionStrength;

#else // SPECULAR_IBL
      // ENVIRONMENT_MAPPING
#ifdef PBR_LIGHTING
        vec3 reflectionStrength = (1 - roughness) * fresnelRoughness(max(dot(v, n), 0), roughness);
#else
        vec3 reflectionStrength = Ks * 0.25 * length(getKs());
#endif
        color += reflectionStrength * texture(environment_map, InverseView * r).rgb;
#endif // ENVIRONMENT_MAPPING
    }

    color += Kd * ambientIntensity;

    return color;
}

void main() {
    vec3 color = lights(fs_in.position_ES, getNormal());
#ifdef PBR_LIGHTING
    color = pow(color, vec3(1.0 / gamma)); // gamma correction
#endif
	fragColour = vec4(color, 1.0);
}
