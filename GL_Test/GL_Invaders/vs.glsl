#version 430 core

in vec3 coord;
uniform mat4 mvp;

void main(){
	gl_Position = mvp*vec4(coord, 1.0);
}