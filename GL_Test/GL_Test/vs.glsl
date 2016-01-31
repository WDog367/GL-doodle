#version 430 core

layout(location = 0) in vec2 coord2d;
in vec3 colour;

out vec3 frag_colour;

void main(void) {
  gl_Position = vec4(coord2d, 0.0, 1.0);
  frag_colour = colour;
}