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
    // vec3 d = normalize(InverseView * fs_in.position_ES);
	vec3 d = normalize(fs_in.position_MS);
    vec3 c1 = vec3(0, 0, 1);
    vec3 c2 = 0.25 * vec3(1, 0.4, 0.9);

    float mix_amount = dot(d, normalize(vec3(0, -1, 0)));
    mix_amount = max(0.0, mix_amount * 4.0 / 3.0 + 1.0 / 3.0);

    c1 = mix(c2, c1, mix_amount);

    fragColour = vec4(c1, 1.0);
}

