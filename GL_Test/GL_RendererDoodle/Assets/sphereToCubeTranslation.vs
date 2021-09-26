#version 330

// Model-Space coordinates
in vec3 position;

uniform mat4 ModelView;
uniform mat4 projection;

out VsOutFsIn {
	vec3 position_ES; // Eye-space position
	vec3 position_MS; // Model-space position
} vs_out;


void main() {
	vec4 pos4 = vec4(position, 1.0);

	vs_out.position_MS = position.xyz;
	vs_out.position_ES = (ModelView * pos4).xyz;
	gl_Position = projection * ModelView * pos4;
}
