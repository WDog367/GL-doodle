#version 430 core
layout(location = 0) in vec4 coord;

in vec4 boneIndex;//fuck this
in vec4 boneWeight;
in int boneNum;

uniform mat4 mvp;
uniform mat4 boneMatrices[200];


out vec4 color;
out float i2;

void main(void) {

int check = 0;
float index = float(boneIndex[0]);

if(check == 0){
	if(index < .5)
		color = vec4(0.0, 0.0, 0.0, 1.0);
	else if(index < 1.5)
		color = vec4(1.0, 0.0, 0.0, 1.0);
	else if(index < 2.5)
		color = vec4(1.0, 0.5, 0.0, 1.0);
	else if(index  < 3.5)
		color = vec4(1.0, 1.0, 1.0, 1.0);
	else if(index  < 4.5)
		color = vec4(0.5, 1.0, 0.0, 1.0);
	else if (index < 5.5)
		color = vec4(0.0, 1.0, 0.0, 1.0);
	else if( index < 6.5)
		color = vec4(0.0, 1.0, 0.5, 1.0);
	else if (index < 7.5)
		color = vec4(0.0, 1.0, 1.0, 1.0);
	else if (index < 8.5)
		color = vec4(0.0, 0.5, 1.0, 1.0);
	else if (index < 9.5)
		color = vec4(0.0, 0.0, 1.0, 1.0);
	else if (index < 10.5)
		color = vec4(0.5, 0.0, 1.0, 1.0);
	else if (index < 11.5)
		color = vec4(1.0, 0.0, 1.0, 1.0);
	else if(index < 12.5)
		color = vec4(1.0, 0.0, 0.5, 1.0);
	else
		color = vec4(0.5, 0.5, 0.5, 1.0);


	//color = 0.5*color;
	color.w = 1.0;
}
else if(check == 1){
	color = vec4(float(boneNum)/4, 0.0, 0.0, 1.0);
}
else if(check == 2){
	if(boneNum == 0)
		color = vec4(0.0, 0.0, 0.0, 1.0);
	else if (boneNum == 1)
		color = vec4(1.0, 0.0, 0.0, 1.0); 
	else if (boneNum == 2)
		color = vec4(0.0, 1.0, 0.0, 1.0);
	else if (boneNum == 3)
		color = vec4(0.0, 0.0, 1.0, 1.0);
	else if (boneNum == 4)
		color = vec4(1.0, 1.0, 0.0, 1.0);
	else
		color = vec4(0.5, 0.5, 0.5, 1.0);
}

	vec4 pos;
	mat4 trans;
	
	for(int i = 0; i < boneNum; i++){
			pos = pos + (boneMatrices[int(boneIndex[i])]*coord)*boneWeight[i];
	}

	gl_Position = mvp*pos;
}