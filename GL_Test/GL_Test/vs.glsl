#version 430 core

layout(location = 0) in vec2 coord2d;

void main(void) {
  gl_Position = vec4(coord2d, 0.0, 1.0);
}