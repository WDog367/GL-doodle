#version 330

// Model-Space coordinates
in vec3 position;
in vec3 normal;
in vec2 uv;

in vec3 tangent;
in vec3 bitangent;

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
} vs_out;


void main() {
	vec4 pos4 = vec4(position, 1.0);
	gl_Position = Perspective * ModelView * vec4(position, 1.0);
}
