#version 330

// defines
//  BRDF_LUT
//  ENVMAP_PREFILTER

// Model-Space coordinates
in VsOutFsIn {
	vec3 position_ES; // Eye-space position
    vec3 position_MS; // Model-space position
} fs_in;

out vec4 fragColour;

// Constants ------------------
const float ONE_OVER_TWO_PI = 0.15915494309;
const float ONE_OVER_PI = 0.31830988618;
const float PI = 3.14159;

// Functions ------------------

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

// main -----------------------


// Schlick Geometric
float G1(float n_dot_v, float k) {
    return n_dot_v / (n_dot_v * (1.0 - k) + k);
}

float Geometry_Schlick(float n_dot_l, float n_dot_v, float roughness) {
    // float a = (roughness)^2
    // float k = a / 2
    float k = roughness * roughness / 2.0;
    return G1(n_dot_v, k) * G1(n_dot_l, k);
}

// IntegrateBRDF taken from: https://cdn2.unrealengine.com/Resources/files/2013SiggraphPresentationsNotes-26915738.pdf
vec2 integrateBRDF(float n_dot_v, float roughness) {
    vec3 V = vec3(
        sqrt( 1.0 - n_dot_v * n_dot_v), //sin
        0.0,
        n_dot_v                         //cos
    );

    float A = 0;
    float B = 0;

    vec3 N = vec3(0, 0, 1);

    // get tangent + bitangent to translate H into worldspace
    vec3 up = abs(N.z) < 0.999 ? vec3(0, 0, 1) : vec3(1, 0, 0);
    vec3 tanx = normalize(cross(up, N));
    vec3 bitan = cross(N, tanx);

    const uint numSamples = 1024u;
    for (uint i = 0u; i < numSamples; i++) {
        vec2 xi = Hammersley(i, numSamples);
        vec3 Li = ImportanceSampleGGX(xi, roughness, N);
        vec3 h = tanx * Li.x + bitan * Li.y + N * Li.z;
        vec3 l = 2 * dot(V, h) * h - V;

        float n_dot_l = clamp(l.z, 0, 1);
        float n_dot_h = clamp(h.z, 0, 1);
        float v_dot_h = clamp(dot(V, h), 0, 1);

        if (n_dot_l > 0 ) {
            float G = Geometry_Schlick(n_dot_l, n_dot_v, roughness);

            float G_Vis = G * v_dot_h / (n_dot_h * n_dot_v);
            float Fc = pow(1 - v_dot_h,  5);
            A += (1 - Fc) * G_Vis;
            B += Fc * G_Vis;
        }
    }

    return vec2(A, B) / numSamples;
}

uniform samplerCube envMap;
const float max_lod = 5.0;

float computeLod(float n_dot_h, float h_dot_v, uint numSamples, float roughness) {
    float k = roughness * roughness / 2;
    float D = G1(n_dot_h, k);
    float pdf = (D * n_dot_h / (4 * h_dot_v)) + 0.001f;

    float saTexel = 4.0 * PI / (6.0 * 512 * 512);
    float saSample = 1.0 / (float(numSamples) * pdf + 0.0001);

    return (roughness == 0.0 ? 0.0 : 0.5 * log2(saSample / saTexel));
}

vec3 prefilterEnvMap(float roughness, vec3 r) {
    vec3 n = r;
    vec3 v = r;

    // get tangent + bitangent to translate H into worldspace
    vec3 up = abs(n.z) < 0.999 ? vec3(0, 0, 1) : vec3(1, 0, 0);
    vec3 tanx = normalize(cross(up, n));
    vec3 bitan = cross(n, tanx);

    vec3 prefilteredColour = vec3(0);

    const uint numSamples = 32u;
    float totalWeight = 0;


    float k = (roughness * roughness) / 2.0;

    for (uint i = 0u; i < numSamples; i++) {
        vec2 xi = Hammersley(i, numSamples);
        vec3 Li = ImportanceSampleGGX(xi, roughness, n); 
        vec3 h = tanx * Li.x + bitan * Li.y + n * Li.z;
        vec3 l = 2 * dot(v, h) * h - v;

        float n_dot_l = clamp(dot(n, l), 0, 1);
        if (n_dot_l > 0) {

            float n_dot_h = clamp(dot(n, h), 0, 1);
            float h_dot_v = clamp(dot(h, v), 0, 1);

            float miplevel = computeLod(n_dot_h, h_dot_v, numSamples, roughness);

            prefilteredColour += textureLod(envMap, l, miplevel).rgb * n_dot_l;
            totalWeight += n_dot_l;
        }
    } 

    return prefilteredColour / totalWeight;
}

#ifdef BRDF_LUT
void main() {
    vec2 uv = fs_in.position_MS.xy * 0.5 + 0.5;
    vec2 color = integrateBRDF(uv.x, uv.y);

    fragColour = vec4(color, 0, 1);
}

#elif defined ENVMAP_PREFILTER
uniform float roughness;
void main() {
    vec3 dir = normalize(fs_in.position_MS);

    vec3 color = prefilterEnvMap(roughness, dir);
    fragColour = vec4(color, 1.0);
}
#endif
