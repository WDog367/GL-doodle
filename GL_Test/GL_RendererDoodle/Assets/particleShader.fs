#version 330

out vec4 fragColour;

in vec3 particleColour;

uniform vec3 ambientIntensity;

void main() {
    vec3 colour = mix(particleColour, ambientIntensity, 0.001);
    fragColour = gl_FragCoord.z * vec4(colour, 1); 
}