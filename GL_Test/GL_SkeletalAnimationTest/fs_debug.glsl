#version 430 core

out vec4 fColor;
in vec4 color;

void main(){
	if (gl_PointCoord.s < 0.1 || gl_PointCoord.s > 0.9)
		fColor = vec4(0.0, 0.0, 0.0, 1.0);
	else if (gl_PointCoord.t < 0.1 || gl_PointCoord.t > 0.9)
		fColor = vec4(1.0, 1.0, 1.0, 1.0);
	else
		fColor = color;
}