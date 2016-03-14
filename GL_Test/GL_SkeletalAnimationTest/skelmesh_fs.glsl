#version 430 core

out vec4 fColor;
in float index;

void main(){
	if(index > 5)
		fColor = vec4(0.0, 1.0, 1.0, 1.0);
	else
		fColor = vec4(0.0, 0.0, 0.0, 1.0);
}