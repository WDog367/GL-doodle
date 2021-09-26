#version 330

// Model-Space coordinates
layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 uv;

layout(location = 3) in vec3 tangent;
layout(location = 4) in vec3 bitangent;

uniform mat4 ModelView;
uniform mat4 Perspective;

// Remember, this is transpose(inverse(ModelView)).  Normals should be
// transformed using this matrix instead of the ModelView matrix.
uniform mat3 NormalMatrix;


out VsOutFsIn {
	vec3 position_ES; // Eye-space position
	vec3 normal_ES;   // Eye-space normal
    vec2 uv;

	vec3 tangent_ES;
	vec3 bitangent_ES;

	vec3 position_MS; // model-space position
} vs_out;


void main() {
	vec4 pos4 = vec4(position, 1.0);

	vs_out.position_MS = position;

	//-- Convert position and normal to Eye-Space:
	vs_out.position_ES = (ModelView * pos4).xyz;
	vs_out.normal_ES = normalize(NormalMatrix * normal);
	vs_out.tangent_ES = normalize(NormalMatrix * tangent);
	vs_out.bitangent_ES = normalize(NormalMatrix * bitangent);

    vs_out.uv = uv;

	gl_Position = Perspective * ModelView * vec4(position, 1.0);
}
