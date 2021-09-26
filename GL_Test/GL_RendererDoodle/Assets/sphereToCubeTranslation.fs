#version 330

// Model-Space coordinates
in VsOutFsIn {
	vec3 position_ES; // Eye-space position
    vec3 position_MS; // Model-space position
} fs_in;

out vec4 fragColour;

// Uniforms -------------------
uniform sampler2D sphereMap;

// Constants ------------------
const float ONE_OVER_TWO_PI = 0.15915494309;
const float ONE_OVER_PI = 0.31830988618;

// main -----------------------
void main() {
    vec3 dir = normalize(fs_in.position_MS);

    // convert dir into flat coordinates

    vec2 texCoord = vec2(
        atan(dir.z, dir.x) * ONE_OVER_TWO_PI, 
        acos(dir.y) * ONE_OVER_PI
    );

    texCoord.x += 0.5; // shift result from [-0.5, 0.5] to [0, 1]

    vec3 color = texture(sphereMap, texCoord).rgb;
    fragColour = vec4(color, 1.0);
    //fragColour = mix(vec4((dir*0.5 + 0.5), 1.0), vec4(color, 1.0), 0.001);
    //fragColour = mix(vec4(texCoord, 0.0, 1.0), vec4(color, 1.0), 0.001);

    //fragColour = mix(vec4((dir), 1.0), vec4(color, 1.0), 0.001);
}
