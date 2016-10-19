#version 430 core

out vec4 fColor;
in vec4 frag_colour;

void main(){
		fColor = frag_colour;
}