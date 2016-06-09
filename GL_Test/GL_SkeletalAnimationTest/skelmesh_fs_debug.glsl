#version 430 core

out vec4 fColor;
in vec4 color;
in float i2;

void main(){
	fColor = color;
//	fColor = vec4(i2, 0.0, 0.0, 1.0);
}