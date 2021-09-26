#version 330

out vec4 fragColour;

in vec3 particleColour;
in vec2 particleUv;

uniform vec3 ambientIntensity;

void main() {
    float d = length(particleUv - vec2(0.5, 0.5));
    if (d > 0.5) {
        discard;
    }
    vec3 colour = particleColour + (vec3(1, 1, 1) * pow(10 * (0.5 - d), 0.25));


    fragColour = gl_FragCoord.z * vec4(mix(colour, ambientIntensity, 0.001), 1); 
}