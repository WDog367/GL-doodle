#version 430 core

layout(location = 0) in vec4 coord;

out vec4 frag_colour;
out float z;

uniform mat4 mvp;
uniform int collision;
uniform vec3 colour;

void main(void) {
  frag_colour = vec4(colour, 1.0);

  gl_Position = mvp*coord;
  gl_PointSize = 10;

  z = gl_Position.z;

  //some exponential falloff
  float maxdist = 512;
  float factor = z/maxdist;

  factor = factor;

  factor = factor < 1.0 ? factor : 1.0;

  frag_colour = (1 - factor)*frag_colour +(factor)*vec4(.99, .99, 1.0, 1.0);
  //frag_colour = vec4(.95, .95, 1.0, 1.0);
}