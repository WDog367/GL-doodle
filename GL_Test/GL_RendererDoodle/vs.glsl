#version 130 core

in vec3 coord;

uniform mat4 modelMatrix;
uniform mat4 cameraMatrix;

out vec4 worldPos;

void main(void) {
	worldPos = modelMatrix * vec4(coord, 1.0);
	gl_Position = cameraMatrix * worldPos;
}