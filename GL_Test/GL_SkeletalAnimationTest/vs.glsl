#version 430 core

layout(location = 0) in vec4 coord;
uniform mat4 mvp;

void main(void) {
  gl_Position = mvp*coord;
  gl_PointSize = 5;
}