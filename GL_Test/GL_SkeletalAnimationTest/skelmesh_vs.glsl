#version 430 core
layout(location = 0) in vec4 coord;

in int boneIndex;
uniform mat4 mvp;
uniform mat4 boneMatrices[200];

out float index;

void main(void) {
	index = boneIndex;
  gl_Position = mvp*boneMatrices[boneIndex]*coord;
  index = boneIndex;
  gl_PointSize = 5;
}