#version 430 core

layout(location = 0) in vec4 coord;

in float index;
out vec4 color;

uniform mat4 mvp;

void main(void) {

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

  gl_Position = mvp*coord;
  gl_PointSize = 10;
}