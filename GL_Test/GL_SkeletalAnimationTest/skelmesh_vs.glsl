#version 430 core
layout(location = 0) in vec4 coord;

in vec4 boneIndex;//fuck this
in vec4 boneWeight;
in int boneNum;

uniform mat4 mvp;
uniform mat4 boneMatrices[200];

void main(void) {
	vec4 pos;
	
	for(int i = 0; i < boneNum; i++){
			pos = pos + (boneMatrices[int(boneIndex[i])]*coord)*boneWeight[i];
	}

	gl_Position = mvp*pos;
}