#version 330 core

in vec2 uv;

uniform sampler2D image;

void main()
{
	//gl_FragColor = texture(texture, uv);
	gl_FragColor = texture(image, uv);
}