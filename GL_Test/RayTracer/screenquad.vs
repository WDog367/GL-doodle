#version 330 core

out vec2 uv;

vec3 positions[] = {
	vec3(-1, -1, 0),
	vec3( 1, -1, 0),
	vec3( 1,  1, 0),

	vec3(-1,  1, 0),
	vec3(-1, -1, 0),
	vec3( 1,  1, 0)
	};

void main()
{
    gl_Position = vec4(positions[gl_VertexID], 1.0);
	uv = 0.5*vec2(positions[gl_VertexID]) + vec2(0.5);
}