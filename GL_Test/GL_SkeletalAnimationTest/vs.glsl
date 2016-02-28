#version 430 core

layout(location = 0) in vec4 coord;
uniform mat4 matrix;

void main(void) {
  gl_Position = matrix*coord;
  gl_PointSize = 5;
}