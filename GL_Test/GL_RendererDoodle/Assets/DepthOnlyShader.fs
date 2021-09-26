#version 330

out vec4 fragColour;

void main() {
	fragColour = vec4(vec3(gl_FragDepth), 1.0);
}
