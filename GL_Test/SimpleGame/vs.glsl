#version 430 core

layout(location = 0) in vec4 coord;
out vec4 color;
out float z;

uniform mat4 mvp;
uniform int collision;

void main(void) {
  color = vec4(1.0, (collision == 0 ? 0.0 : 1.0) , 0.0, 1.0);

  gl_Position = mvp*coord;
  gl_PointSize = 10;

  z = gl_Position.z;
}