#version 330

// Model-Space coordinates
in vec3 position;

uniform mat3 InverseView;
uniform mat4 ModelView;
uniform mat4 Perspective;

out VsOutFsIn {
	vec3 position_ES; // Eye-space position
    vec3 position_MS; // Model-space position
	vec3 position_WS; // World-space position
} vs_out;


// todo: get proper box in CPU application
vec3 verts[] = vec3[](
	vec3(-1, -1, -1), // 0
	vec3(-1, 1, -1), // 2 
	vec3(1, -1, -1), // 1
	vec3(1, -1, -1), // 1
	vec3(-1, 1, -1), // 2
	vec3(1, 1, -1), // 3

	vec3(1, -1, -1), // 1
	vec3(1, 1, -1), // 3
	vec3(1, -1, 1), // 5
	vec3(1, -1, 1), // 5
	vec3(1, 1, -1), // 3
	vec3(1, 1, 1), // 7

	vec3(1, -1, 1), // 5
	vec3(1, 1, 1), // 7
	vec3(-1, -1, 1), // 4
	vec3(-1, -1, 1), // 4
	vec3(1, 1, 1), // 7
	vec3(-1, 1, 1), // 6

	vec3(-1, -1, 1), // 4
	vec3(-1, 1, 1), // 6
	vec3(-1, -1, -1), // 0
	vec3(-1, -1, -1), // 0
	vec3(-1, 1, 1), // 6
	vec3(-1, 1, -1), // 2

	vec3(-1, 1, -1), // 2
	vec3(-1, 1, 1), // 6
	vec3(1, 1, -1), // 3
	vec3(1, 1, -1), // 3
	vec3(-1, 1, 1), // 6
	vec3(1, 1, 1), // 7

	vec3(1, -1, 1), // 5
	vec3(1, -1, -1), // 1
	vec3(-1, -1, 1), // 4
	vec3(-1, -1, 1), // 4
	vec3(1, -1, -1), // 1
	vec3(-1, -1, -1) // 0
	);
void main() {
	vec4 pos4 = vec4(200 * verts[gl_VertexID], 0.0);

	vs_out.position_MS = pos4.xyz;
	vs_out.position_ES = (ModelView * pos4).xyz;
	vs_out.position_WS = InverseView * (ModelView * pos4).xyz;
	gl_Position = Perspective * ModelView * pos4;
}
