#version 430 core

in vec3 frag_colour;

out vec4 fColor;

void main(){
	fColor = vec4(frag_colour, 1.0);
}