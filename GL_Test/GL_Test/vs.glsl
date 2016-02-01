#version 430 core

layout(location = 0) in vec3 coord;
in vec3 colour;

out vec3 frag_colour;

uniform mat4 matrix;

void main(void) {
  gl_Position = matrix*vec4(coord, 1.0);
  frag_colour = colour;
}