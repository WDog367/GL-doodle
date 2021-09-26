#version 330

// Model-Space coordinates
in VsOutFsIn {
	vec3 position_ES; // Eye-space position
    vec3 position_MS; // Model-space position
	vec3 position_WS; // World-space position
} fs_in;

out vec4 fragColour;

// Uniforms -------------------
uniform mat3 InverseView;
uniform samplerCube environment_map;
uniform samplerCube prefiltered_specular;

// main -----------------------
void main() {
    vec3 dir = normalize(InverseView * fs_in.position_ES);
    vec3 color = textureLod(prefiltered_specular, dir, 1).rgb;
    fragColour = vec4(color, 1.0);
}

