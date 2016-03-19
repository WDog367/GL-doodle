#version 430 core
layout(location = 0) in vec4 coord;

in int boneIndex[5];
in float boneWeight[5];
in int boneNum;

uniform mat4 mvp;
uniform mat4 boneMatrices[200];

out float index;

void main(void) {
	index = float(boneNum);
	//for (int i = 0; i < boneNum; i ++){
	//	gl_Position = boneWeight[i]*mvp*boneMatrices[boneIndex[i]]*coord;
	//}
	gl_Position = mvp*boneWeight[0]*boneMatrices[boneIndex[0]]*coord;
	gl_PointSize = 5;
}